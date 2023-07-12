// SPDX-License-Identifier: BSD-3-Clause

#include <linux/rpmsg.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "common.h"

int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo)
{
	int ret;

	ret = ioctl(rpfd, RPMSG_CREATE_EPT_IOCTL, eptinfo);
	if (ret)
		perror("Failed to create endpoint.\n");
	return ret;
}
