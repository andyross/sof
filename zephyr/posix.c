#include <sof/ipc/driver.h>
#include <sof/platform.h>

uint8_t posix_hostbox[MAILBOX_HOSTBOX_SIZE];
uint8_t posix_dspbox[MAILBOX_DSPBOX_SIZE];
uint8_t posix_stream[MAILBOX_STREAM_SIZE];
uint8_t posix_trace[MAILBOX_TRACE_SIZE];

/* This seems like a vestige.  Existing Zephyr platforms are emitting
 * these markers in their linker scripts, and wrapper.c code iterates
 * over the list, but no data gets placed there anywhere?  Note that
 * Zephyr has a proper STRUCT_SECTION_ITERABLE API for this kind of
 * trick...
 *
 * Just emit two identical symbols to make the existing code work
 */
__asm__(".globl _module_init_start\n"
	"_module_init_start:\n"
	".globl _module_init_end\n"
	"_module_init_end:\n");

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
	return 0;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
}

int platform_init(struct sof *sof)
{
	return 0;
}

int platform_boot_complete(uint32_t boot_message)
{
}
