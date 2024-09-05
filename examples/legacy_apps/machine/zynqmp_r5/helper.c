/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (c) 2021-2022 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xil_cache.h"
#include <metal/sys.h>
#include <metal/irq.h>
#include "platform_info.h"

/*
 * A circular buffer for libmetal log. Need locks if ported to MT world.
 * c_buf - pointer to the buffer referenced in the resource table
 * c_len - size of the buffer
 * c_pos - next rext record position
 * c_cnt - free running count of records to help sorting in case of overrun
 */
extern char *get_rsc_trace_info(unsigned int *);

/*
 * app_gic_initialize - Interrupt Controller setup
 */
extern int app_gic_initialize(void);

static struct {
	char * c_buf;
	unsigned int c_len;
	unsigned int c_pos;
	unsigned int c_cnt;
} circ;

static void rsc_trace_putchar(char c)
{
	if (circ.c_pos >= circ.c_len)
		circ.c_pos = 0;
	circ.c_buf[circ.c_pos++] = c;
}

static void rsc_trace_logger(enum metal_log_level level,
			   const char *format, ...)
{
	char msg[128];
	char *p;
	unsigned int len;
	va_list args;

	/* prefix "cnt L6 ": record count and log level */
	len = sprintf(msg, "%u L%u ", circ.c_cnt, level);
	if (len < 0 || len >= sizeof(msg))
		len = 0;
	circ.c_cnt++;

	va_start(args, format);
	vsnprintf(msg + len, sizeof(msg) - len, format, args);
	va_end(args);

	/* copy at most sizeof(msg) to the circular buffer */
	for (len = 0, p = msg; *p && len < sizeof(msg); ++len, ++p)
		rsc_trace_putchar(*p);
	/* Remove this xil_printf to stop printing to console */
	xil_printf("%s", msg);
}

/* Main hw machinery initialization entry point, called from main()*/
/* return 0 on success */
int init_system(void)
{
	int ret;
	struct metal_init_params metal_param = METAL_INIT_DEFAULTS;

	circ.c_buf = get_rsc_trace_info(&circ.c_len);
	if (circ.c_buf && circ.c_len){
		metal_param.log_handler = rsc_trace_logger;
		metal_param.log_level = METAL_LOG_DEBUG;
		circ.c_pos = circ.c_cnt = 0;
	};

	/* Low level abstraction layer for openamp initialization */
	metal_init(&metal_param);

	/* configure the global interrupt controller */
	app_gic_initialize();

	/* Initialize metal Xilinx IRQ controller */
	ret = metal_xlnx_irq_init();
	if (ret) {
		metal_err("metal_xlnx_irq_init failed.\r\n");
	}

	metal_dbg("c_buf,c_len = %p,%u\r\n", circ.c_buf, circ.c_len);
	return ret;
}

void cleanup_system()
{
	metal_finish();

	Xil_DCacheDisable();
	Xil_ICacheDisable();
	Xil_DCacheInvalidate();
	Xil_ICacheInvalidate();
}
