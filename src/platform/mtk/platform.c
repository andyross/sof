#include <sof/platform.h>
#include <sof/ipc/driver.h>

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
