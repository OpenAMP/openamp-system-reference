// SPDX-License-Identifier: BSD-3-Clause

#ifndef __COMMON__H__
#define __COMMON__H__

#include <linux/rpmsg.h>

#define RPMSG_BUS_SYS "/sys/bus/rpmsg"

int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo);
char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
                             const char *ept_name,
                             char *ept_dev_name);
int bind_rpmsg_chrdev(const char *rpmsg_dev_name);
int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name, char *rpmsg_ctrl_name);
int lookup_channel(char *out, struct rpmsg_endpoint_info *pep);

#endif /* __COMMON__H__ */
