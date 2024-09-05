/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RPMSG_ECHO_H
#define RPMSG_ECHO_H

#define RPMSG_SERVICE_NAME         "rpmsg-openamp-demo-channel"

int app(struct rpmsg_device *rdev, void *priv);

#endif /* RPMSG_ECHO_H */
