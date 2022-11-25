#include <sof/lib/uuid.h>
#include <sof/ipc/msg.h>
#include <sof/lib/mailbox.h>
#include <sof/ipc/common.h>
#include <sof/schedule/edf_schedule.h>

// 6c8f0d53-ff77-4ca1-b825-c0c4e1b0d322
DECLARE_SOF_UUID("posix-ipc-task", ipc_task_uuid,
		 0x6c8f0d53, 0xff77, 0x4ca1,
		 0xb8, 0x25, 0xc0, 0xc4, 0xe1, 0xb0, 0xd3, 0x22);

void posix_ipc_isr(void *arg)
{
	// The struct ipc is from the argument to platform_ipc_init()
	//
	//struct ipc *ipc = arg;
	//ipc_schedule_process(ipc);
}

// This API is a little confounded by its history.  The job of this
// function is to get a newly-received IPC message into the comp_data
// buffer on the IPC object and then call ipc_cmd() with the same
// pointer (it does NOT work to place the data anywhere else!).  With
// existing IPC3 platforms, this is done outside the platform layer by
// mailbox_validate() (with IPC4 mailbox_validate() is a noop), but we
// can skip that as long as we set the memory up correctly.
enum task_state ipc_platform_do_cmd(struct ipc *ipc)
{
	struct ipc_cmd_hdr *hdr;

	hdr = mailbox_validate();
	ipc_cmd(hdr);
	return SOF_TASK_STATE_COMPLETED;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
	// This API signals the host side that processing for an IPC
	// command is complete.  It's a noop here.
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
	// There is no host, just write to the mailbox to validate the buffer
	mailbox_dspbox_write(0, msg->tx_data, msg->tx_size);
	return 0;
}

int platform_ipc_init(struct ipc *ipc)
{
        printk("=== %s()\n", __func__);

	schedule_task_init_edf(&ipc->ipc_task, SOF_UUID(ipc_task_uuid),
			       &ipc_task_ops, ipc, 0, 0);

	return 0;
}
