#include <sof/platform.h>
#include <sof/ipc/driver.h>
#include <kernel/ext_manifest.h>
#include <platform/lib/memory.h>

int platform_init(struct sof *sof)
{
	return 0;
}

int platform_boot_complete(uint32_t boot_message)
{
	return 0;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
}

// Extended manifest window record.  Note the alignment attribute is
// critical as rimage demands allocation in units of 16 bytes, yet
// other records emitted into the same section are not padded!
// (Really this is an rimage bug, it should separately validate each
// symbol in the section and re-pack the array instead of relying on
// the poor linker to do it).

#define WINDOW(region)				\
	{ .type = SOF_IPC_REGION_##region,	\
	  .size = _MTK_IPC_WIN_SIZE(region),	\
	  .offset = _MTK_IPC_WIN_BASE(region), }

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
