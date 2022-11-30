#include <sof/lib/uuid.h>
#include <sof/ipc/msg.h>
#include <sof/lib/mailbox.h>
#include <sof/ipc/common.h>
#include <sof/ipc/schedule.h>
#include <sof/schedule/edf_schedule.h>

// 6c8f0d53-ff77-4ca1-b825-c0c4e1b0d322
DECLARE_SOF_UUID("posix-ipc-task", ipc_task_uuid,
		 0x6c8f0d53, 0xff77, 0x4ca1,
		 0xb8, 0x25, 0xc0, 0xc4, 0xe1, 0xb0, 0xd3, 0x22);

static struct ipc *global_ipc;

// Not an ISR, called from the native_posix fuzz interrupt.  Left
// alone for general hygiene.  This is how a IPC interrupt would look
// if we had one.
static void posix_ipc_isr(void *arg)
{
	ipc_schedule_process(global_ipc);
}

extern uint8_t *posix_fuzz_buf, posix_fuzz_sz;

static uint8_t fuzz_in[65536];
static uint8_t fuzz_in_sz;

// The protocol here is super simple: the first byte is a message size
// in units of 16 bits (the buffer maximum defaults to 384 bytes, and
// I didn't want to waste space early in the buffer lest I confuse the
// fuzzing heuristics).  We then copy that much of the input buffer
// (subject to clamping obviously) into the incoming IPC message
// buffer and invoke the ISR.  Any remainder will be delivered
// synchronously as another message after receipt of "complete_cmd()"
// from the SOF engine, etc...  Eventually we'll receive another fuzz
// input after some amount of simulated time has passed (c.f.
// CONFIG_ARCH_POSIX_FUZZ_TICKS)
static void fuzz_isr(const void *arg)
{
	if (fuzz_in_sz == 0) {
		// The fuzzer does indeed present empty input buffers,
		// I guess to test the rig?  We pass!
		return;
	}

	memset(global_ipc->comp_data, 0, SOF_IPC_MSG_MAX_SIZE);

	size_t n = MIN(fuzz_in_sz, SOF_IPC_MSG_MAX_SIZE);
	size_t rem = fuzz_in_sz - n;

	for (int i = 0; i < n; i++) {
		uint8_t *cmd = global_ipc->comp_data; // why is it a void*?

		cmd[i] = fuzz_in[i];
	}
	memmove(&fuzz_in[0], &fuzz_in[n], rem);
	fuzz_in_sz = rem;

	posix_ipc_isr(NULL);
}

// This API is a little confounded by its history.  The job of this
// function is to get a newly-received IPC message into the comp_data
// buffer on the IPC object and then call ipc_cmd() with the same
// pointer (it does NOT work to place the data anywhere else!).  With
// existing IPC3 platforms, this is done outside (?!) the platform
// layer by mailbox_validate() (with IPC4 mailbox_validate() is a
// noop).
enum task_state ipc_platform_do_cmd(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);
	struct ipc_cmd_hdr *hdr;

	hdr = mailbox_validate();
	ipc_cmd(hdr);
	return SOF_TASK_STATE_COMPLETED;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);
	// This API signals the host side that processing for an IPC
	// command is complete.  It's a noop here.
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
        printk("=== %s()\n", __func__);
	// There is no host, just write to the mailbox to validate the buffer
	mailbox_dspbox_write(0, msg->tx_data, msg->tx_size);
	return 0;
}

int platform_ipc_init(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	schedule_task_init_edf(&ipc->ipc_task, SOF_UUID(ipc_task_uuid),
			       &ipc_task_ops, ipc, 0, 0);

	return 0;
}
