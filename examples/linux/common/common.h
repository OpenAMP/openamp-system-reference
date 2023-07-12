// SPDX-License-Identifier: BSD-3-Clause

#ifndef __COMMON__H__
#define __COMMON__H__

#include <linux/rpmsg.h>

int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo);

#endif /* __COMMON__H__ */
