// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2021 Google LLC.
//
// Author: Lionel Koenig <lionelk@google.com>
#include <errno.h>
#include <ipc/control.h>
#include <ipc/stream.h>
#include <ipc/topology.h>
#include <ipc4/aec.h>
#include <sof/audio/module_adapter/module/generic.h>
#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/data_blob.h>
#include <sof/audio/format.h>
#include <sof/audio/kpb.h>
#include <sof/audio/pipeline.h>
#include <sof/common.h>
#include <rtos/panic.h>
#include <sof/ipc/msg.h>
#include <rtos/alloc.h>
#include <rtos/init.h>
#include <sof/lib/memory.h>
#include <sof/lib/notifier.h>
#include <sof/lib/uuid.h>
#include <rtos/wait.h>
#include <sof/list.h>
#include <sof/math/numbers.h>
#include <rtos/string.h>
#include <sof/trace/trace.h>
#include <sof/ut.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <user/trace.h>

#include <google_rtc_audio_processing.h>
#include <google_rtc_audio_processing_platform.h>
#include <google_rtc_audio_processing_sof_message_reader.h>

#define GOOGLE_RTC_AUDIO_PROCESSING_FREQENCY_TO_PERIOD_FRAMES 100
#define GOOGLE_RTC_NUM_INPUT_PINS 2

LOG_MODULE_REGISTER(google_rtc_audio_processing, CONFIG_SOF_LOG_LEVEL);

/* b780a0a6-269f-466f-b477-23dfa05af758 */
DECLARE_SOF_RT_UUID("google-rtc-audio-processing", google_rtc_audio_processing_uuid,
					0xb780a0a6, 0x269f, 0x466f, 0xb4, 0x77, 0x23, 0xdf, 0xa0,
					0x5a, 0xf7, 0x58);

DECLARE_TR_CTX(google_rtc_audio_processing_tr, SOF_UUID(google_rtc_audio_processing_uuid),
			   LOG_LEVEL_INFO);

#ifndef __ZEPHYR__
/* Zephyr provides uncached memory for static variables on SMP, but we
 * are single-core component and know we can safely use the cache for
 * AEC work.  XTOS SOF is cached by default, so stub the Zephyr API.
 */
#define arch_xtensa_cached_ptr(p) (p)
#endif

static __aligned(PLATFORM_DCACHE_ALIGN)
uint8_t aec_mem_blob[CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_MEMORY_BUFFER_SIZE_BYTES];

#define NUM_FRAMES (CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ \
		    / GOOGLE_RTC_AUDIO_PROCESSING_FREQENCY_TO_PERIOD_FRAMES)
#define REF_CHAN_MAX CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_NUM_AEC_REFERENCE_CHANNELS
#define MIC_CHAN_MAX CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_NUM_CHANNELS
#define REFOUT_CHAN MAX(REF_CHAN_MAX, MIC_CHAN_MAX)

static __aligned(PLATFORM_DCACHE_ALIGN)
int16_t refoutbuf[sizeof(uint16_t) * NUM_FRAMES * REF_CHAN_MAX];

static __aligned(PLATFORM_DCACHE_ALIGN)
int16_t micbuf[sizeof(uint16_t) * NUM_FRAMES * REFOUT_CHAN];

struct google_rtc_audio_processing_comp_data {
#if CONFIG_IPC_MAJOR_4
	struct sof_ipc4_aec_config config;
#endif
	uint32_t num_frames;
	int num_aec_reference_channels;
	int num_capture_channels;
	GoogleRtcAudioProcessingState *state;
	int aec_reference_frame_index;
	int16_t *raw_mic_buffer;
	int raw_mic_buffer_frame_index;
	int16_t *refout_buffer;
	int output_buffer_frame_index;
	struct comp_data_blob_handler *tuning_handler;
	bool reconfigure;
	int aec_reference_source;
	int raw_microphone_source;
	struct comp_buffer *ref_comp_buffer;
};

void *GoogleRtcMalloc(size_t size)
{
	return rballoc(0, SOF_MEM_CAPS_RAM, size);
}

void GoogleRtcFree(void *ptr)
{
	return rfree(ptr);
}

#if CONFIG_IPC_MAJOR_4
static void google_rtc_audio_processing_params(struct processing_module *mod)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);
	struct sof_ipc_stream_params *params = mod->stream_params;
	struct comp_buffer *sinkb, *sourceb;
	struct list_item *source_list;
	struct comp_dev *dev = mod->dev;

	ipc4_base_module_cfg_to_stream_params(&mod->priv.cfg.base_cfg, params);
	component_set_nearest_period_frames(dev, params->rate);

	list_for_item(source_list, &dev->bsource_list) {
		sourceb = container_of(source_list, struct comp_buffer, sink_list);
		if (IPC4_SINK_QUEUE_ID(buf_get_id(sourceb)) == SOF_AEC_FEEDBACK_QUEUE_ID)
			ipc4_update_buffer_format(sourceb, &cd->config.reference_fmt);
		else
			ipc4_update_buffer_format(sourceb, &mod->priv.cfg.base_cfg.audio_fmt);
	}

	sinkb = list_first_item(&dev->bsink_list, struct comp_buffer, source_list);
	ipc4_update_buffer_format(sinkb, &mod->priv.cfg.base_cfg.audio_fmt);
}
#endif

static int google_rtc_audio_processing_reconfigure(struct processing_module *mod)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);
	struct comp_dev *dev = mod->dev;
	uint8_t *config;
	size_t size;
	int ret;

	comp_dbg(dev, "google_rtc_audio_processing_reconfigure()");

	if (!comp_is_current_data_blob_valid(cd->tuning_handler) &&
	    !comp_is_new_data_blob_available(cd->tuning_handler)) {
		/*
		 * The data blob hasn't been available once so far.
		 *
		 * This looks redundant since the same check will be done in
		 * comp_get_data_blob() below. But without this early return,
		 * hundreds of warn message lines are produced per second by
		 * comp_get_data_blob() calls until the data blob is arrived.
		 */
		return 0;
	}

	config = comp_get_data_blob(cd->tuning_handler, &size, NULL);
	if (size == 0) {
		/* No data to be handled */
		return 0;
	}

	if (!config) {
		comp_err(dev, "google_rtc_audio_processing_reconfigure(): Tuning config not set");
		return -EINVAL;
	}

	comp_info(dev, "google_rtc_audio_processing_reconfigure(): New tuning config %p (%zu bytes)",
		  config, size);

	cd->reconfigure = false;

	uint8_t *google_rtc_audio_processing_config;
	size_t google_rtc_audio_processing_config_size;
	int num_capture_input_channels;
	int num_capture_output_channels;
	float aec_reference_delay;
	float mic_gain;
	bool google_rtc_audio_processing_config_present;
	bool num_capture_input_channels_present;
	bool num_capture_output_channels_present;
	bool aec_reference_delay_present;
	bool mic_gain_present;

	GoogleRtcAudioProcessingParseSofConfigMessage(config, size,
						      &google_rtc_audio_processing_config,
						      &google_rtc_audio_processing_config_size,
						      &num_capture_input_channels,
						      &num_capture_output_channels,
						      &aec_reference_delay,
						      &mic_gain,
						      &google_rtc_audio_processing_config_present,
						      &num_capture_input_channels_present,
						      &num_capture_output_channels_present,
						      &aec_reference_delay_present,
						      &mic_gain_present);

	if (google_rtc_audio_processing_config_present) {
		comp_info(dev,
			  "google_rtc_audio_processing_reconfigure(): Applying config of size %zu bytes",
			  google_rtc_audio_processing_config_size);

		ret = GoogleRtcAudioProcessingReconfigure(cd->state,
							  google_rtc_audio_processing_config,
							  google_rtc_audio_processing_config_size);
		if (ret) {
			comp_err(dev, "GoogleRtcAudioProcessingReconfigure failed: %d",
				 ret);
			return ret;
		}
	}

	if (num_capture_input_channels_present || num_capture_output_channels_present) {
		if (num_capture_input_channels_present && num_capture_output_channels_present) {
			if (num_capture_input_channels != num_capture_output_channels) {
				comp_err(dev, "GoogleRtcAudioProcessingReconfigure failed: unsupported channel counts");
				return -EINVAL;
			}
			cd->num_capture_channels = num_capture_input_channels;
		} else if (num_capture_input_channels_present) {
			cd->num_capture_channels = num_capture_output_channels;
		} else {
			cd->num_capture_channels = num_capture_output_channels;
		}
		comp_info(dev,
			  "google_rtc_audio_processing_reconfigure(): Applying num capture channels %d",
			  cd->num_capture_channels);


		ret = GoogleRtcAudioProcessingSetStreamFormats(cd->state,
							       CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ,
							       cd->num_capture_channels,
							       cd->num_capture_channels,
							       CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ,
							       cd->num_aec_reference_channels);

		if (ret) {
			comp_err(dev, "GoogleRtcAudioProcessingSetStreamFormats failed: %d",
				 ret);
			return ret;
		}
	}

	if (aec_reference_delay_present || mic_gain_present) {
		float *capture_headroom_linear_use = NULL;
		float *echo_path_delay_ms_use = NULL;

		if (mic_gain_present) {
			capture_headroom_linear_use = &mic_gain;

			/* Logging of linear headroom, using integer workaround to the broken printout of floats */
			comp_info(dev,
				  "google_rtc_audio_processing_reconfigure(): Applying capture linear headroom: %d.%d",
				  (int)mic_gain, (int)(100 * mic_gain) - 100 * ((int)mic_gain));
		}
		if (aec_reference_delay_present) {
			echo_path_delay_ms_use = &aec_reference_delay;

			/* Logging of delay, using integer workaround to the broken printout of floats */
			comp_info(dev,
				  "google_rtc_audio_processing_reconfigure(): Applying aec reference delay: %d.%d",
				  (int)aec_reference_delay,
				  (int)(100 * aec_reference_delay) -
				  100 * ((int)aec_reference_delay));
		}

		ret = GoogleRtcAudioProcessingParameters(cd->state,
							 capture_headroom_linear_use,
							 echo_path_delay_ms_use);

		if (ret) {
			comp_err(dev, "GoogleRtcAudioProcessingParameters failed: %d",
				 ret);
			return ret;
		}
	}

	return 0;
}

#if CONFIG_IPC_MAJOR_3
static int google_rtc_audio_processing_cmd_set_data(struct processing_module *mod,
						    struct sof_ipc_ctrl_data *cdata)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);
	int ret;

	switch (cdata->cmd) {
	case SOF_CTRL_CMD_BINARY:
		ret = comp_data_blob_set_cmd(cd->tuning_handler, cdata);
		if (ret)
			return ret;
		/* Accept the new blob immediately so that userspace can write
		 * the control in quick succession without error.
		 * This ensures the last successful control write from userspace
		 * before prepare/copy is applied.
		 * The config blob is not referenced after reconfigure() returns
		 * so it is safe to call comp_get_data_blob here which frees the
		 * old blob. This assumes cmd() and prepare()/copy() cannot run
		 * concurrently which is the case when there is no preemption.
		 */
		if (comp_is_new_data_blob_available(cd->tuning_handler)) {
			comp_get_data_blob(cd->tuning_handler, NULL, NULL);
			cd->reconfigure = true;
		}
		return 0;
	default:
		comp_err(mod->dev,
			 "google_rtc_audio_processing_ctrl_set_data(): Only binary controls supported %d",
			 cdata->cmd);
		return -EINVAL;
	}
}

static int google_rtc_audio_processing_cmd_get_data(struct processing_module *mod,
						    struct sof_ipc_ctrl_data *cdata,
						    size_t max_data_size)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);

	comp_info(mod->dev, "google_rtc_audio_processing_ctrl_get_data(): %u", cdata->cmd);

	switch (cdata->cmd) {
	case SOF_CTRL_CMD_BINARY:
		return comp_data_blob_get_cmd(cd->tuning_handler, cdata, max_data_size);
	default:
		comp_err(mod->dev,
			 "google_rtc_audio_processing_ctrl_get_data(): Only binary controls supported %d",
			 cdata->cmd);
		return -EINVAL;
	}
}
#endif

static int google_rtc_audio_processing_set_config(struct processing_module *mod, uint32_t param_id,
						  enum module_cfg_fragment_position pos,
						  uint32_t data_offset_size,
						  const uint8_t *fragment,
						  size_t fragment_size, uint8_t *response,
						  size_t response_size)
{
#if CONFIG_IPC_MAJOR_4
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);
	int ret;

	switch (param_id) {
	case SOF_IPC4_SWITCH_CONTROL_PARAM_ID:
	case SOF_IPC4_ENUM_CONTROL_PARAM_ID:
		comp_err(mod->dev, "google_rtc_audio_processing_ctrl_set_data(): Only binary controls supported");
		return -EINVAL;
	}

	ret = comp_data_blob_set(cd->tuning_handler, pos, data_offset_size,
				 fragment, fragment_size);
	if (ret)
		return ret;

	/* Accept the new blob immediately so that userspace can write
	 * the control in quick succession without error.
	 * This ensures the last successful control write from userspace
	 * before prepare/copy is applied.
	 * The config blob is not referenced after reconfigure() returns
	 * so it is safe to call comp_get_data_blob here which frees the
	 * old blob. This assumes cmd() and prepare()/copy() cannot run
	 * concurrently which is the case when there is no preemption.
	 *
	 * Note from review: A race condition is possible and should be
	 * further investigated and fixed.
	 */
	if (comp_is_new_data_blob_available(cd->tuning_handler)) {
		comp_get_data_blob(cd->tuning_handler, NULL, NULL);
		cd->reconfigure = true;
	}

	return 0;
#elif CONFIG_IPC_MAJOR_3
	struct sof_ipc_ctrl_data *cdata = (struct sof_ipc_ctrl_data *)fragment;

	return google_rtc_audio_processing_cmd_set_data(mod, cdata);
#endif
}

static int google_rtc_audio_processing_get_config(struct processing_module *mod,
						  uint32_t param_id, uint32_t *data_offset_size,
						  uint8_t *fragment, size_t fragment_size)
{
#if CONFIG_IPC_MAJOR_4
	comp_err(mod->dev, "google_rtc_audio_processing_ctrl_get_config(): Not supported");
	return -EINVAL;
#elif CONFIG_IPC_MAJOR_3
	struct sof_ipc_ctrl_data *cdata = (struct sof_ipc_ctrl_data *)fragment;

	return google_rtc_audio_processing_cmd_get_data(mod, cdata, fragment_size);
#endif
}

static int google_rtc_audio_processing_init(struct processing_module *mod)
{
	struct module_data *md = &mod->priv;
	struct comp_dev *dev = mod->dev;
	struct google_rtc_audio_processing_comp_data *cd;
	int ret;

	comp_info(dev, "google_rtc_audio_processing_init()");

	/* Create private component data */
	cd = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM, sizeof(*cd));
	if (!cd) {
		ret = -ENOMEM;
		goto fail;
	}

	md->private = cd;

#if CONFIG_IPC_MAJOR_4
	const struct ipc4_base_module_extended_cfg *base_cfg = md->cfg.init_data;
	struct ipc4_input_pin_format reference_fmt, output_fmt;
	const size_t size = sizeof(struct ipc4_input_pin_format);

	cd->config.base_cfg = base_cfg->base_cfg;

	/* Copy the reference format from input pin 1 format */
	memcpy_s(&reference_fmt, size,
		 &base_cfg->base_cfg_ext.pin_formats[size], size);
	memcpy_s(&output_fmt, size,
		 &base_cfg->base_cfg_ext.pin_formats[size * GOOGLE_RTC_NUM_INPUT_PINS], size);

	cd->config.reference_fmt = reference_fmt.audio_fmt;
	cd->config.output_fmt = output_fmt.audio_fmt;
#endif

	cd->tuning_handler = comp_data_blob_handler_new(dev);
	if (!cd->tuning_handler) {
		ret = -ENOMEM;
		goto fail;
	}

	cd->num_aec_reference_channels = CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_NUM_AEC_REFERENCE_CHANNELS;
	cd->num_capture_channels = CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_NUM_CHANNELS;
	cd->num_frames = NUM_FRAMES;

	/* Giant blob of scratch memory. */
	GoogleRtcAudioProcessingAttachMemoryBuffer(arch_xtensa_cached_ptr(&aec_mem_blob[0]),
						   sizeof(aec_mem_blob));

	cd->state = GoogleRtcAudioProcessingCreateWithConfig(CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ,
							     cd->num_capture_channels,
							     cd->num_capture_channels,
							     CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ,
							     cd->num_aec_reference_channels,
							     /*config=*/NULL, /*config_size=*/0);

	if (!cd->state) {
		comp_err(dev, "Failed to initialized GoogleRtcAudioProcessing");
		ret = -EINVAL;
		goto fail;
	}

	float capture_headroom_linear = CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_MIC_HEADROOM_LINEAR;
	float echo_path_delay_ms = CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_ECHO_PATH_DELAY_MS;
	ret = GoogleRtcAudioProcessingParameters(cd->state,
						 &capture_headroom_linear,
						 &echo_path_delay_ms);

	if (ret < 0) {
		comp_err(dev, "Failed to apply GoogleRtcAudioProcessingParameters");
		goto fail;
	}

	cd->raw_mic_buffer = &micbuf[0];
	cd->refout_buffer = &refoutbuf[0];

#ifdef __ZEPHYR__
	cd->raw_mic_buffer = arch_xtensa_cached_ptr(cd->raw_mic_buffer);
	cd->refout_buffer = &refoutbuf[0];
#endif

	cd->raw_mic_buffer_frame_index = 0;
	cd->aec_reference_frame_index = 0;
	cd->output_buffer_frame_index = 0;

	/* comp_is_new_data_blob_available always returns false for the first
	 * control write with non-empty config. The first non-empty write may
	 * happen after prepare (e.g. during copy). Default to true so that
	 * copy keeps checking until a non-empty config is applied.
	 */
	cd->reconfigure = true;

	/* Mic and reference, needed for audio stream type copy module client */
	mod->max_sources = 2;

	comp_dbg(dev, "google_rtc_audio_processing_init(): Ready");
	return 0;

fail:
	comp_err(dev, "google_rtc_audio_processing_init(): Failed");
	if (cd) {
		if (cd->state) {
			GoogleRtcAudioProcessingFree(cd->state);
		}
		GoogleRtcAudioProcessingDetachMemoryBuffer();
		comp_data_blob_handler_free(cd->tuning_handler);
		rfree(cd);
	}

	return ret;
}

static int google_rtc_audio_processing_free(struct processing_module *mod)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);

	comp_dbg(mod->dev, "google_rtc_audio_processing_free()");

	GoogleRtcAudioProcessingFree(cd->state);
	cd->state = NULL;
	GoogleRtcAudioProcessingDetachMemoryBuffer();
	comp_data_blob_handler_free(cd->tuning_handler);
	rfree(cd);
	return 0;
}

static int google_rtc_audio_processing_prepare(struct processing_module *mod,
					       struct sof_source **sources,
					       int num_of_sources,
					       struct sof_sink **sinks,
					       int num_of_sinks)
{
	struct comp_dev *dev = mod->dev;
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);
	struct list_item *source_buffer_list_item;
	struct comp_buffer *output;
	unsigned int aec_channels = 0, frame_fmt, rate;
	int microphone_stream_channels = 0;
	int output_stream_channels;
	int ret;
	int i = 0;

	comp_info(dev, "google_rtc_audio_processing_prepare()");

#if CONFIG_IPC_MAJOR_4
	google_rtc_audio_processing_params(mod);
#endif

	/* searching for stream and feedback source buffers */
	list_for_item(source_buffer_list_item, &dev->bsource_list) {
		struct comp_buffer *source = container_of(source_buffer_list_item,
							  struct comp_buffer, sink_list);
#if CONFIG_IPC_MAJOR_4
		if (IPC4_SINK_QUEUE_ID(buf_get_id(source)) ==
			SOF_AEC_FEEDBACK_QUEUE_ID) {
#else
		if (source->source->pipeline->pipeline_id != dev->pipeline->pipeline_id) {
#endif
			cd->aec_reference_source = i;
			cd->ref_comp_buffer = source;
			aec_channels = audio_stream_get_channels(&source->stream);
			comp_dbg(dev, "reference index = %d, channels = %d", i, aec_channels);
		} else {
			cd->raw_microphone_source = i;
			microphone_stream_channels = audio_stream_get_channels(&source->stream);
			comp_dbg(dev, "microphone index = %d, channels = %d", i,
				 microphone_stream_channels);
		}

		audio_stream_init_alignment_constants(1, 1, &source->stream);
		i++;
	}

	output = list_first_item(&dev->bsink_list, struct comp_buffer, source_list);

	/* On some platform the playback output is left right left right due to a crossover
	 * later on the signal processing chain. That makes the aec_reference be 4 channels
	 * and the AEC should only use the 2 first.
	 */
	if (cd->num_aec_reference_channels > aec_channels) {
		comp_err(dev, "unsupported number of AEC reference channels: %d",
			 aec_channels);
		return -EINVAL;
	}

	audio_stream_init_alignment_constants(1, 1, &output->stream);
	frame_fmt = audio_stream_get_frm_fmt(&output->stream);
	rate = audio_stream_get_rate(&output->stream);
	output_stream_channels = audio_stream_get_channels(&output->stream);

	if (cd->num_capture_channels > microphone_stream_channels) {
		comp_err(dev, "unsupported number of microphone channels: %d",
			 microphone_stream_channels);
		return -EINVAL;
	}

	if (cd->num_capture_channels > output_stream_channels) {
		comp_err(dev, "unsupported number of output channels: %d",
			 output_stream_channels);
		return -EINVAL;
	}

	switch (frame_fmt) {
#if CONFIG_FORMAT_S16LE
	case SOF_IPC_FRAME_S16_LE:
		break;
#endif /* CONFIG_FORMAT_S16LE */
	default:
		comp_err(dev, "unsupported data format: %d", frame_fmt);
		return -EINVAL;
	}

	if (rate != CONFIG_COMP_GOOGLE_RTC_AUDIO_PROCESSING_SAMPLE_RATE_HZ) {
		comp_err(dev, "unsupported samplerate: %d", rate);
		return -EINVAL;
	}

	/* Blobs sent during COMP_STATE_READY is assigned to blob_handler->data
	 * directly, so comp_is_new_data_blob_available always returns false.
	 */
	ret = google_rtc_audio_processing_reconfigure(mod);
	if (ret)
		return ret;

	return 0;
}

static int trigger_handler(struct processing_module *mod, int cmd)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);

	/* Ignore and halt propagation if we get a trigger from the
	 * playback pipeline: not for us.
	 */
	if (cd->ref_comp_buffer->walking)
		return PPL_STATUS_PATH_STOP;

	/* Note: not module_adapter_set_state().  With IPC4 those are
	 * identical, but IPC3 has some odd-looking logic that
	 * validates that no sources are active when receiving a
	 * PRE_START command, which obviously breaks for our reference
	 * stream if playback was already running when our pipeline
	 * started
	 */
	return comp_set_state(mod->dev, cmd);
}

static int google_rtc_audio_processing_reset(struct processing_module *mod)
{
	comp_dbg(mod->dev, "google_rtc_audio_processing_reset()");
	return 0;
}

/* FunctionMostlyExistsToKeepLineLengthsUnderControl */
static inline void execute_aec(struct google_rtc_audio_processing_comp_data *cd)
{
	/* FIXME: sample/frame format is platform dependent, these are
	 * hard-configured format APIs and need indirection.  Note
	 * that the calling code in process() is format-independent.
	 */
	/* Note that reference input and mic output share the same
	 * buffer for efficiency
	 */
	GoogleRtcAudioProcessingAnalyzeRender_int16(cd->state,
						    cd->refout_buffer);
	GoogleRtcAudioProcessingProcessCapture_int16(cd->state,
						     cd->raw_mic_buffer,
						     cd->refout_buffer);
	cd->raw_mic_buffer_frame_index = 0;
}

static void source_copy(struct sof_source *src, int frames, int16_t *dst)
{
	size_t chan = source_get_channels(src);
	size_t samples = frames * chan;
	size_t bytes = samples * sizeof(int16_t);
	const int16_t *buf, *bufstart, *bufend;
	int i, c, err;
	size_t bufsz;

	err = source_get_data(src, bytes, (void *)&buf, (void *)&bufstart, &bufsz);
	assert(err == 0);
	bufend = &bufstart[bufsz];

	for (i = 0; i < frames; i++) {
		for  (c = 0; c < chan; c++) {
			*dst++ = *buf++;
			if (buf >= bufend)
				buf = bufstart;
		}
	}
	source_release_data(src, bytes);
}

static void sink_copy(struct sof_sink *sink, int frames, int16_t *src)
{
	size_t chan = sink_get_channels(sink);
	size_t samples = frames * chan;
	size_t bytes = samples * sizeof(int16_t);
	int16_t *buf, *bufstart, *bufend;
	int i, c, err;
	size_t bufsz;

	err = sink_get_buffer(sink, bytes, (void *)&buf, (void *)&bufstart, &bufsz);
	assert(err == 0);
	bufend = &bufstart[bufsz];

	for (i = 0; i < frames; i++) {
		for  (c = 0; c < chan; c++) {
			*buf++ = *src++;
			if (buf >= bufend)
				buf = bufstart;
		}
	}
	sink_commit_buffer(sink, bytes);
}

static int mod_process(struct processing_module *mod, struct sof_source **sources,
		       int num_of_sources, struct sof_sink **sinks, int num_of_sinks)
{
	struct google_rtc_audio_processing_comp_data *cd = module_get_private_data(mod);

	if (cd->reconfigure)
		google_rtc_audio_processing_reconfigure(mod);

	struct sof_source *mic = sources[cd->raw_microphone_source];
	struct sof_source *ref = sources[cd->aec_reference_source];
	struct sof_sink *out = sinks[0];

	bool ref_ok = cd->ref_comp_buffer->source->state == COMP_STATE_ACTIVE;

	/* Would be cleaner to store a bit of state to elide a bzero
	 * we already did, but we'd be doing the copy of real data in
	 * the ref_ok state anyway.
	 */
	if (!ref_ok)
		bzero(refoutbuf, sizeof(refoutbuf));

	int fmic = source_get_data_frames_available(mic);
	int fref = source_get_data_frames_available(ref);
	int frames = ref_ok ? MIN(fmic, fref) : fmic;
	int n, frames_rem;

	/* If fref > fmic (common at pipeline startup if
	 * playback was already active), we should consume the early
	 * samples so AEC compares the most recent values.
	 */
	if (ref_ok && fref > fmic)
		source_release_data(ref, (fref - fmic) * source_get_frame_bytes(ref));

	for (frames_rem = frames; frames_rem; frames_rem -= n) {
		n = MIN(frames, cd->num_frames - cd->raw_mic_buffer_frame_index);

		/* sample indices */
		int smic = cd->raw_mic_buffer_frame_index * source_get_channels(mic);
		int sref = cd->aec_reference_frame_index * source_get_channels(ref);

		source_copy(mic, n, &cd->raw_mic_buffer[smic]);

		if (ref_ok)
			source_copy(ref, n, &cd->refout_buffer[sref]);

		cd->raw_mic_buffer_frame_index += n;

		if (cd->raw_mic_buffer_frame_index >= cd->num_frames) {
			execute_aec(cd);
			sink_copy(out, n, cd->refout_buffer);
		}
	}
	return 0;
}

static struct module_interface google_rtc_audio_processing_interface = {
	.init  = google_rtc_audio_processing_init,
	.free = google_rtc_audio_processing_free,
	.process = mod_process,
	.prepare = google_rtc_audio_processing_prepare,
	.set_configuration = google_rtc_audio_processing_set_config,
	.get_configuration = google_rtc_audio_processing_get_config,
	.trigger = trigger_handler,
	.reset = google_rtc_audio_processing_reset,
};

DECLARE_MODULE_ADAPTER(google_rtc_audio_processing_interface,
		       google_rtc_audio_processing_uuid, google_rtc_audio_processing_tr);
SOF_MODULE_INIT(google_rtc_audio_processing,
		sys_comp_module_google_rtc_audio_processing_interface_init);
