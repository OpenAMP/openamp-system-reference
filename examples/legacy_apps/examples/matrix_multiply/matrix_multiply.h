/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MATRIX_MULTIPLY_H
#define MATRIX_MULTIPLY_H

#define RPMSG_SERVICE_NAME         "rpmsg-openamp-demo-channel"

int rpmsg_matrix_app(struct rpmsg_device *rdev, void *priv);

#endif /* MATRIX_MULTIPLY_H */
