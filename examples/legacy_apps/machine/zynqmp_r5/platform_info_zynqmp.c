/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       plat_zynqmp.c
 *
 * DESCRIPTION
 *
 *	Define IPI information for ZynqMP platform used in Debug Banner.
 *
 **************************************************************************/

#include "debug_info.h"
#include "platform_info_zynqmp.h"

#ifdef SDT
#include "bspconfig.h"
#endif

#if defined(PLATFORM_ZYNQMP) && defined(DEBUG_INFO)
const struct xlnx_ipi_reg_info ipi_agent_info[] = {
	/* IPI_ID_APU Not Supported */
	[IPI_ID_APU] = {},
	[IPI_ID_RPU0] = {
		.base_addr = IPI_ADDR_RPU0,
		.bitmask_position = IPI_CH1_BIT,
		.vector_id = IPI_VECT_ID_CH1,
	},
	[IPI_ID_RPU1] = {
		.base_addr = IPI_ADDR_RPU1,
		.bitmask_position = IPI_CH2_BIT,
		.vector_id = IPI_VECT_ID_CH2,
	},
	/* IPI_ID_PMU0 Not Supported */
	[IPI_ID_PMU0] = {},
	/* IPI_ID_PMU1 Not Supported */
	[IPI_ID_PMU1] = {},
	/* IPI_ID_PMU2 Not Supported */
	[IPI_ID_PMU2] = {},
	/* IPI_ID_PMU3 Not Supported */
	[IPI_ID_PMU3] = {},
	[IPI_ID_PL0] = {
		.base_addr = IPI_ADDR_PL0,
		.bitmask_position = IPI_CH7_BIT,
		.vector_id = IPI_VECT_ID_CH7,
	},
	[IPI_ID_PL1] = {
		.base_addr = IPI_ADDR_PL1,
		.bitmask_position = IPI_CH8_BIT,
		.vector_id = IPI_VECT_ID_CH8,
	},
	[IPI_ID_PL2] = {
		.base_addr = IPI_ADDR_PL2,
		.bitmask_position = IPI_CH9_BIT,
		.vector_id = IPI_VECT_ID_CH9,
	},
	[IPI_ID_PL3] = {
		.base_addr = IPI_ADDR_PL3,
		.bitmask_position = IPI_CH10_BIT,
		.vector_id = IPI_VECT_ID_CH10,
	},
};

int get_ipi_msg_buf(unsigned int src, unsigned int dst)
{
	return IPI_BUFFER_BASEADDR +
		src * SRC_AGENT_OFFSET +
		dst * DST_AGENT_OFFSET;
}
#endif
