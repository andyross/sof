#include <rtos/clk.h>
#include <platform/lib/memory.h>
#include <kernel/ext_manifest.h>
#include <sof/platform.h>
#include <sof/debug/debug.h>
#include <sof/ipc/driver.h>
#include <sof/ipc/msg.h>
#include <sof/lib/agent.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/notifier.h>
#include <sof/schedule/ll_schedule_domain.h>
#include <sof/schedule/ll_schedule.h>
#include <sof/schedule/edf_schedule.h>
#include <sof_versions.h>

void mtk_dai_init(struct sof *sof);

#define MBOX0 DEVICE_DT_GET(DT_INST(0, mediatek_mbox))
#define MBOX1 DEVICE_DT_GET(DT_INST(1, mediatek_mbox))

static void mtk_ipc_send(const void *msg, size_t sz)
{
        mailbox_dspbox_write(0, msg, sz);
        mtk_adsp_mbox_signal(MBOX0, 0);
}

int platform_ipc_init(struct ipc *ipc)
{
        printk("ANDY %s:%d (FIXME: register mbox handlers)\n", __func__, __LINE__);
        // FIXME
        return 0;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
        mtk_adsp_mbox_signal(MBOX1, 1);
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
        printk("ANDY %s:%d\n", __func__, __LINE__);
        struct ipc *ipc = ipc_get();

        if (ipc->is_notification_pending)
                return -EBUSY;

        ipc->is_notification_pending = true;
        mtk_ipc_send(msg->tx_data, msg->tx_size);
        return 0;
}

static int set_cpuclk(int clock, int hz)
{
        return clock == 0 && hz == CONFIG_XTENSA_CCOUNT_HZ ? 0 : -EINVAL;
}

void clocks_init(struct sof *sof)
{
        // Dummy CPU clock driver that supports one known frequency.
        // This hardware has clock scaling support, but it hasn't
        // historically been exercised so we have nothing to test
        // against.
        static struct freq_table freqs[] = {
                { .freq = CONFIG_XTENSA_CCOUNT_HZ,
                  .ticks_per_msec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000, }
        };
        static struct clock_info clks[] = {
                { .freqs_num = ARRAY_SIZE(freqs),
                  .freqs = freqs,
                  .notification_id = NOTIFIER_ID_CPU_FREQ,
                  .notification_mask = NOTIFIER_TARGET_CORE_MASK(0),
                  .set_freq = set_cpuclk, },
        };
        sof->clocks = clks;
}

int platform_init(struct sof *sof)
{
        int ret;
        printk("ANDY %s:%d\n", __func__, __LINE__);

        clocks_init(sof);

        sof->platform_timer_domain = zephyr_domain_init(PLATFORM_DEFAULT_CLOCK);

        ipc_init(sof);

        mtk_dai_init(sof);

        scheduler_init_edf();
        scheduler_init_ll(sof->platform_timer_domain);

        sa_init(sof, CONFIG_SYSTICK_PERIOD); // watchdoggy thingy

	return 0;
}

int platform_boot_complete(uint32_t boot_message)
{
        printk("ANDY %s:%d\n", __func__, __LINE__);
        static const struct sof_ipc_fw_ready fw_ready_cmd = {
                .hdr.cmd = SOF_IPC_FW_READY,
                .hdr.size = sizeof(struct sof_ipc_fw_ready),
                .version = {
                        .hdr.size = sizeof(struct sof_ipc_fw_version),
                        .micro = SOF_MICRO,
                        .minor = SOF_MINOR,
                        .major = SOF_MAJOR,
                        .tag = SOF_TAG,
                        .abi_version = SOF_ABI_VERSION,
                        .src_hash = SOF_SRC_HASH,
                },
                .flags = DEBUG_SET_FW_READY_FLAGS,
        };

        mtk_ipc_send(&fw_ready_cmd, sizeof(fw_ready_cmd));
	return 0;
}

// Extended manifest window record.  Note the alignment attribute is
// critical as rimage demands allocation in units of 16 bytes, yet the
// C struct records emitted into the section are not in general padded
// and will pack tighter than that!  (Really this is an rimage bug, it
// should separately validate each symbol in the section and re-pack
// the array instead of relying on the poor linker to do it).

#define WINDOW(region)				\
	{ .type = SOF_IPC_REGION_##region,	\
	  .size = MTK_IPC_WIN_SIZE(region),	\
	  .offset = MTK_IPC_WIN_OFF(region), }

struct ext_man_windows mtk_man_win __section(".fw_metadata") __aligned(EXT_MAN_ALIGN) = {
        .hdr = {
                .type = EXT_MAN_ELEM_WINDOW,
                .elem_size = ROUND_UP(sizeof(struct ext_man_windows), EXT_MAN_ALIGN)
        },
	.window = {
		.ext_hdr = {
                        .hdr.cmd = SOF_IPC_FW_READY,
                        .hdr.size = sizeof(struct sof_ipc_window),
                        .type = SOF_IPC_EXT_WINDOW,
                },
                .num_windows = 6,
                .window = {
			// Order doesn't match memory layout for historical
			// reasons.  Shouldn't matter, but don't rock the boat...
			WINDOW(UPBOX),
			WINDOW(DOWNBOX),
			WINDOW(DEBUG),
			WINDOW(TRACE),
			WINDOW(STREAM),
			WINDOW(EXCEPTION),
		},
	},
};
