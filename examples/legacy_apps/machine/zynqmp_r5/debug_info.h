/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       debug_info.h
 *
 * DESCRIPTION
 *
 *	This file defines structures and a function signature for
 *	OpenAMP Debug information used in the demo.
 *
 **************************************************************************/

#ifndef DEBUG_INFO_H_
#define DEBUG_INFO_H_

#define SRC_AGENT_OFFSET	0x200U
#define DST_AGENT_OFFSET	0x40U
#define IPI_BUFFER_RESP_OFFSET	0x20U

/**
 * struct xlnx_ipi_reg_info - Config object to use to display OpenAMP
			      Application Debug Information
 * @param base_addr IPI Address.
 * @param bitmask_position bitmask positions that correspond to the offsets
 *			   in IPI registers.
 * @param vector_id ID for GIC Interrupt Vectors.
 */
struct xlnx_ipi_reg_info {
	const unsigned int base_addr;
	const unsigned int bitmask_position;
	const unsigned int vector_id;
};

/**
 * @brief Debug Information logging function
 */
void debug_info(void);

/**
 * @brief Return IPI Message Buffer value.
 *
 * Note that for some SOC's there are bufferless IPI's. In that case
 * the routine will return 0.
 */
int get_ipi_msg_buf(unsigned int src, unsigned int dst);

#endif /* DEBUG_INFO_H_ */
