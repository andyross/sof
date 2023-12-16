// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2021 Google LLC.
//
// Author: Lionel Koenig <lionelk@google.com>
#include "google_rtc_audio_processing.h"
#include "google_rtc_audio_processing_sof_message_reader.h"

static int dummy_state;

static void SetFormats(GoogleRtcAudioProcessingState *const state,
		       int capture_sample_rate_hz,
		       int num_capture_input_channels,
		       int num_capture_output_channels,
		       int render_sample_rate_hz,
		       int num_render_channels)
{
}

void GoogleRtcAudioProcessingAttachMemoryBuffer(uint8_t *const buffer,
						int buffer_size)
{
}

void GoogleRtcAudioProcessingDetachMemoryBuffer(void)
{
}

GoogleRtcAudioProcessingState *GoogleRtcAudioProcessingCreateWithConfig(int capture_sample_rate_hz,
									int num_capture_input_channels,
									int num_capture_output_channels,
									int render_sample_rate_hz,
									int num_render_channels,
									const uint8_t *const config,
									int config_size)
{
	return &dummy_state;
}

GoogleRtcAudioProcessingState *GoogleRtcAudioProcessingCreate(void)
{
}

void GoogleRtcAudioProcessingFree(GoogleRtcAudioProcessingState *state)
{
}

int GoogleRtcAudioProcessingSetStreamFormats(GoogleRtcAudioProcessingState *const state,
					     int capture_sample_rate_hz,
					     int num_capture_input_channels,
					     int num_capture_output_channels,
					     int render_sample_rate_hz,
					     int num_render_channels)
{
	return 0;
}

int GoogleRtcAudioProcessingParameters(GoogleRtcAudioProcessingState *const state,
				       float *capture_headroom_linear,
				       float *echo_path_delay_ms)
{
	return 0;
}

int GoogleRtcAudioProcessingGetFramesizeInMs(GoogleRtcAudioProcessingState *state)
{
}

int GoogleRtcAudioProcessingReconfigure(GoogleRtcAudioProcessingState *const state,
					const uint8_t *const config,
					int config_size)
{
	return 0;
}

int GoogleRtcAudioProcessingProcessCapture_float32(GoogleRtcAudioProcessingState * const state,
						   const float * const *src,
						   float * const *dest)
{
	return 0;
}

int GoogleRtcAudioProcessingAnalyzeRender_float32(GoogleRtcAudioProcessingState * const state,
						  const float * const *data)
{
	return 0;
}

void GoogleRtcAudioProcessingParseSofConfigMessage(uint8_t *message,
						   size_t message_size,
						   uint8_t **google_rtc_audio_processing_config,
						   size_t *google_rtc_audio_processing_config_size,
						   int *num_capture_input_channels,
						   int *num_capture_output_channels,
						   float *aec_reference_delay,
						   float *mic_gain,
						   bool *google_rtc_audio_processing_config_present,
						   bool *num_capture_input_channels_present,
						   bool *num_capture_output_channels_present,
						   bool *aec_reference_delay_present,
						   bool *mic_gain_present)
{
}
