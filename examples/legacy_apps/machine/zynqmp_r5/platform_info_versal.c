/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       platform_info_versal.c
 *
 * DESCRIPTION
 *
 *	Define IPI information for Versal platform used in Debug Banner.
 *
 **************************************************************************/

#include "debug_info.h"
#include "platform_info_versal.h"

#ifdef SDT
#include "bspconfig.h"
#endif

#if defined(VERSAL) && defined(DEBUG_INFO)
const struct xlnx_ipi_reg_info ipi_agent_info[] = {
	[IPI_ID_PMC] = {
		.base_addr = IPI_ADDR_PMC,
		.bitmask_position = IPI_VECT_ID_PMC,
		.vector_id = IPI_VECT_ID_PMC,
	},
	[IPI_ID_0] = {
		.base_addr = IPI_ADDR_0,
		.bitmask_position = IPI_ID_0,
		.vector_id = IPI_VECT_ID_IPI0,
	},
	[IPI_ID_1] = {
		.base_addr = IPI_ADDR_1,
		.bitmask_position = IPI_ID_1,
		.vector_id = IPI_VECT_ID_IPI1,
	},
	[IPI_ID_2] = {
		.base_addr = IPI_ADDR_2,
		.bitmask_position = IPI_ID_2,
		.vector_id = IPI_VECT_ID_IPI2,
	},
	[IPI_ID_3] = {
		.base_addr = IPI_ADDR_3,
		.bitmask_position = IPI_ID_3,
		.vector_id = IPI_VECT_ID_IPI3,
	},
	[IPI_ID_4] = {
		.base_addr = IPI_ADDR_4,
		.bitmask_position = IPI_ID_4,
		.vector_id = IPI_VECT_ID_IPI4,
	},
	[IPI_ID_5] = {
		.base_addr = IPI_ADDR_5,
		.bitmask_position = IPI_ID_5,
		.vector_id = IPI_VECT_ID_IPI5,

	},
	[IPI_ID_PMCNOBUF] = {
		.base_addr = IPI_ADDR_PMCNOBUF,
		.bitmask_position = IPI_VECT_ID_PMCNOBUF,
		.vector_id = IPI_VECT_ID_PMCNOBUF,
	},
	[IPI_ID_6] = {
		.base_addr = IPI_ADDR_6,
		.bitmask_position = IPI_ID_6,
		.vector_id = IPI_VECT_ID_IPI6,
	},
};

int get_ipi_msg_buf(unsigned int src, unsigned int dst)
{
	/* NOBUF IPI's are PMC_NOBUF and IPI6 */
	if (src > IPI_ID_5 || dst > IPI_ID_5)
		return 0;
	return IPI_BUFFER_BASEADDR +
		src * SRC_AGENT_OFFSET +
		dst * DST_AGENT_OFFSET;
}
#endif
