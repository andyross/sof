#include <sof/ipc/driver.h>
#include <sof/schedule/task.h>
#include <sof/platform.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/lib/agent.h>

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

/* Ditto for symbols in .trace_ctx */
__asm__(".globl _trace_ctx_start\n"
	"_trace_ctx_start:\n"
	".globl _trace_ctx_end\n"
	"_trace_ctx_end:\n");


int platform_ipc_init(struct ipc *ipc)
{
	return 0;
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
	return 0;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
}

enum task_state ipc_platform_do_cmd(struct ipc *ipc)
{
	return SOF_TASK_STATE_COMPLETED;
}

struct ipc_data_host_buffer *ipc_platform_get_host_buffer(struct ipc *ipc)
{
	return NULL;
}

void mtrace_event(const char *data, uint32_t length)
{
}

int dmac_init(struct sof *sof)
{
        return 0;
}

int dai_init(struct sof *sof)
{
        return 0;
}

int platform_context_save(struct sof *sof)
{
	return 0;
}

void platform_clock_init(struct sof *sof)
{
	// FIXME: seems like this has to initialize sof->clocks */
}

int platform_init(struct sof *sof)
{
	/* All this seems to be generic boilerplate duplicated in all
	 * platform_init() mathods?
	 */
        platform_clock_init(sof);
        scheduler_init_edf();
        sa_init(sof, CONFIG_SYSTICK_PERIOD);
        dmac_init(sof);
        ipc_init(sof);
        dai_init(sof);

	return 0;
}

int platform_boot_complete(uint32_t boot_message)
{
        return 0;
}
