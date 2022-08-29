#ifndef PLATFORM_POSIX_DRIVERS_IDC_H
#define PLATFORM_POSIX_DRIVERS_IDC_H

#include <sof/drivers/idc.h>

struct idc_msg;

static inline int idc_send_msg(struct idc_msg *msg, uint32_t mode)
{
	__ASSERT_NO_MSG(false);
	return 0;
}

#endif /* PLATFORM_POSIX_DRIVERS_IDC_H */
