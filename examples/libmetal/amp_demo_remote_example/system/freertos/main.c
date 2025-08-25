/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 /***************************************************************************
  * main.c
  *
  * This file simply wires the FreeRTOS running task to call the demo.
  */

#include "common.h"

static TaskHandle_t comm_task;
static SemaphoreHandle_t xSemaphore;

/**
 * @brief    demo application main processing task
 *           Here are the steps for the main function:
 *           * Run the libmetal demo
 *           * self killing task.
 * @return   0 - succeeded, non-zero for failures.
 */
static void processing(void *unused_arg)
{
	(void)unused_arg;

	xSemaphore = xSemaphoreCreateBinary();
	if ( xSemaphore == NULL ) {
		xil_printf("Failed to create semaphore\n");
		goto out;
	}

	/* libmetal_amp_demo_remote takes in OS specific arg for channel */
	libmetal_amp_demo_remote(&xSemaphore);

out:
	/* Terminate this task */
	vTaskDelete(NULL);
}

/**
 * @brief    main function of the demo application.
 *           It starts the processing task and go wait forever.
 * @return   0 - succeeded, but in reality will never return.
 */

int main(void)
{
	BaseType_t stat;

	Xil_ExceptionDisable();

	/* Create the tasks */
	stat = xTaskCreate(processing, ( const char * ) "HW",
				1024, NULL, 2, &comm_task);
	if (stat != pdPASS) {
		LPERROR("Cannot create task\n");
	} else {
		/* Start running FreeRTOS tasks */
		vTaskStartScheduler();
	}

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for ( ;; );

	/* suppress compilation warnings*/
	return 0;
}
