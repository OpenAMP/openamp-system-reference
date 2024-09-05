/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       debug_info.c
 *
 * DESCRIPTION
 *
 *	This file defines debug routines to display Remoteproc, RPMsg and SOC
 *	information to user.
 *
 **************************************************************************/

#include "platform_info.h"
#include "rsc_table.h"
#include "debug_info.h"
#include <errno.h>

#ifdef DEBUG_INFO

/* These routines are set by plat_<SOC>.c */
extern const struct xlnx_ipi_reg_info ipi_agent_info[];

/**
 * @brief Ensure IPI are valid for app and set host and remote IPI
	  indices if so.
 *
 * @param info IPI table
 * @param host_ipi_idx Host IPI Index into IPI Table
 * @param remote_ipi_idx Remote IPI Index into IPI Table
 *
 * @return 0 for succeed, negative value for failure
 */
int retrieve_ipi(const struct xlnx_ipi_reg_info *info, unsigned int *host_ipi_idx,
		 unsigned int *remote_ipi_idx)
{
	int i;

	*remote_ipi_idx = 0U;
	*host_ipi_idx = 0U;
	for (i = FIRST_IPI; i <= LAST_IPI; i++) {
		if (info[i].base_addr != 0U &&
		    info[i].base_addr == POLL_BASE_ADDR &&
		    (*remote_ipi_idx == 0U))
			*remote_ipi_idx = i;
		if (info[i].base_addr != 0U &&
		    ((0x1U << info[i].bitmask_position) == IPI_CHN_BITMASK) &&
		    (*host_ipi_idx == 0U))
			*host_ipi_idx = i;
	}

	if (*host_ipi_idx == 0U) {
		metal_log(METAL_LOG_INFO,
			  "Debug Info: %s: Host IPI value %lx is invalid.\n",
			  __func__, IPI_CHN_BITMASK);
		return -EINVAL;
	}

	if (*remote_ipi_idx == 0U) {
		metal_log(METAL_LOG_INFO,
			  "Debug Info: %s: Remote IPI value %lx is invalid.\n",
			  __func__, POLL_BASE_ADDR);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief  SOC Information logging function
 *
 * @return 0 for succeed, negative value for failure
 */
int soc_info(void)
{
	metal_log(METAL_LOG_INFO, "OPENAMP_CONFIG_BANNER\n");
	metal_log(METAL_LOG_INFO, "## COMMON: ##\n");
	metal_log(METAL_LOG_INFO, "SOC = %s\n", SOC_STR);
	metal_log(METAL_LOG_INFO, "To get SOC model on linux run the following:\n");
	metal_log(METAL_LOG_INFO, "\tcat /proc/device-tree/compatible\n");

	return 0;
}

/**
 * @brief Remoteproc Debug Information logging function
 *
 * @param info IPI table
 * @param host_ipi_idx Host IPI Index into IPI Table
 * @param remote_ipi_idx Remote IPI Index into IPI Table
 *
 * @return 0 for succeed, negative value for failure
 */
int remoteproc_info(const struct xlnx_ipi_reg_info *info, unsigned int host_ipi_idx,
		    unsigned int remote_ipi_idx)
{
	metal_log(METAL_LOG_INFO, "## REMOTEPROC: ##\n");
	metal_log(METAL_LOG_INFO, "Host ipi:\tbase = 0x%x\t"
		  "xlnx,ipi-id = 0x%x\tmask = 0x%x\tsys irq = 0x%x\n",
		  info[host_ipi_idx].base_addr, host_ipi_idx,
		  (1U << info[host_ipi_idx].bitmask_position),
		  info[host_ipi_idx].vector_id);
	metal_log(METAL_LOG_INFO, "Remote ipi:\tbase = 0x%x\t"
		  "xlnx,ipi-id = 0x%x\tmask = 0x%x\tsys irq = 0x%x\n",
		  info[remote_ipi_idx].base_addr, remote_ipi_idx,
		  (1U << info[remote_ipi_idx].bitmask_position),
		  info[remote_ipi_idx].vector_id);
	metal_log(METAL_LOG_INFO, "Host IPI message buffers:\t"
		  "local_request_region = 0x%x\tlocal_response_region = 0x%x\n",
		  get_ipi_msg_buf(host_ipi_idx, remote_ipi_idx),
		  get_ipi_msg_buf(host_ipi_idx, remote_ipi_idx) + IPI_BUFFER_RESP_OFFSET);
	metal_log(METAL_LOG_INFO, "Remote IPI message buffers:\t"
		  "remote_request_region = 0x%x\tremote_response_region = 0x%x\n",
		  get_ipi_msg_buf(remote_ipi_idx, host_ipi_idx),
		  get_ipi_msg_buf(remote_ipi_idx, host_ipi_idx) + IPI_BUFFER_RESP_OFFSET);

	return 0;
}

/**
 * @brief RPMsg Debug Information logging function
 *
 * @return 0 for succeed, negative value for failure
 */
int rpmsg_info(void)
{
	void *rsc_table;
	struct remote_resource_table *resource_table;
	int rsc_size;

	rsc_table = get_resource_table(0, &rsc_size);
	resource_table = (struct remote_resource_table *)rsc_table;

	metal_log(METAL_LOG_INFO, "## Resource Table VRing Information: ##\n");
	metal_log(METAL_LOG_INFO, "VRING 0:  base address = 0x%x\n",
		  resource_table->rpmsg_vring0.da);
	metal_log(METAL_LOG_INFO, "VRING 1:  base address = 0x%x\n",
		  resource_table->rpmsg_vring1.da);

	return 0;
}

/**
 * @brief Debug Information logging function
 */
void debug_info(void)
{
	unsigned int host_ipi_idx, remote_ipi_idx;
	int ret;

	ret = retrieve_ipi(ipi_agent_info, &host_ipi_idx, &remote_ipi_idx);
	if (ret)
		return ret;

	ret = soc_info();
	if (ret)
		return ret;

	ret = remoteproc_info(ipi_agent_info, host_ipi_idx, remote_ipi_idx);
	if (ret)
		return ret;

	ret = rpmsg_info();
	if (ret)
		return ret;

	return 0;
}

#endif /* DEBUG_INFO */
