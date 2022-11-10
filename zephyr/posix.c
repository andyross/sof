#include <sof/ipc/driver.h>
#include <sof/schedule/task.h>
#include <sof/platform.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/schedule/ll_schedule.h>
#include <sof/lib/agent.h>

struct ll_schedule_domain *ll_timer_init(void);
struct ll_schedule_domain *ll_dma_init(void);

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
        printk("=== %s()\n", __func__);
	return 0;
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
        printk("=== %s()\n", __func__);
	return 0;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);
}

enum task_state ipc_platform_do_cmd(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);
	return SOF_TASK_STATE_COMPLETED;
}

struct ipc_data_host_buffer *ipc_platform_get_host_buffer(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);
	return NULL;
}

void mtrace_event(const char *data, uint32_t length)
{
        printk("=== %s()\n", __func__);
}

int platform_context_save(struct sof *sof)
{
        printk("=== %s()\n", __func__);
	return 0;
}

int platform_init(struct sof *sof)
{
        printk("=== %s()\n", __func__);

        // FIXME: need to initialize sof->clocks[0] (indexed by CPU,
        // we have only one) with frequency data & callbacks that
        // match some kind of hardware.

        // FIXME: wire up runtime PM?

	/* All this seems to be generic boilerplate duplicated in all
	 * platform_init() mathods?
	 */
        scheduler_init_edf();

        sof->platform_timer_domain = ll_timer_init();
	scheduler_init_ll(sof->platform_timer_domain);

        sa_init(sof, CONFIG_SYSTICK_PERIOD);

        sof->platform_dma_domain = ll_dma_init();
        scheduler_init_ll(sof->platform_dma_domain);

        ipc_init(sof);

	return 0;
}

int platform_boot_complete(uint32_t boot_message)
{
        printk("=== %s()\n", __func__);
        return 0;
}
