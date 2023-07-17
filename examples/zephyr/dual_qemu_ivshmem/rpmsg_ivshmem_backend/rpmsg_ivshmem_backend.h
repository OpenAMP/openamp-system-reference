/*
 * Copyright (c) 2023, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPMSG_IVHSMEM_BACKEND_H__
#define RPMSG_IVHSMEM_BACKEND_H__

#include <openamp/open_amp.h>
#include <metal/device.h>

/**
 * @brief Get RPMsg-IVSHMEM, destination address
 *
 * @return other side destination address, -1 if none is found;
 */
int get_rpmsg_ivshmem_ept_dest_addr(void);

/**
 * @brief Get RPMsg-IVSHMEM, initialized, backend device for RPMSg endpoint creation
 *
 * @return pointer to the initialized RPMsg device
 */
struct rpmsg_device *get_rpmsg_ivshmem_device(void);

#endif

