/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This is a sample demonstration application that showcases usage of remoteproc
 * and rpmsg APIs on the remote core. This application is meant to run on the remote CPU
 * running baremetal code. This applicationr receives two matrices from the host,
 * multiplies them and returns the result to the host core.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <metal/log.h>
#include <openamp/open_amp.h>
#include "matrix_multiply.h"
#include "platform_info.h"

#define	MAX_SIZE		6
#define NUM_MATRIX		2

#define SHUTDOWN_MSG	0xEF56A55A

#define LPRINTF(fmt, ...) printf("%s():%u " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LPERROR(fmt, ...) LPRINTF("ERROR: " fmt, ##__VA_ARGS__)

typedef struct _matrix {
	unsigned int size;
	unsigned int elements[MAX_SIZE][MAX_SIZE];
} matrix;

/* Local variables */
static struct rpmsg_endpoint lept;
static int shutdown_req = 0;

#ifdef USE_FREERTOS
extern TaskHandle_t rpmsg_task;
#endif /* USE_FREERTOS */

/*-----------------------------------------------------------------------------*
 *  Calculate the Matrix
 *-----------------------------------------------------------------------------*/
static void Matrix_Multiply(const matrix *m, const matrix *n, matrix *r)
{
	unsigned int i, j, k;

	memset(r, 0x0, sizeof(matrix));
	r->size = m->size;

	for (i = 0; i < m->size; ++i) {
		for (j = 0; j < n->size; ++j) {
			for (k = 0; k < r->size; ++k) {
				r->elements[i][j] +=
					m->elements[i][k] * n->elements[k][j];
			}
		}
	}
}

/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	matrix matrix_array[NUM_MATRIX];
	matrix matrix_result;

	(void)priv;
	(void)src;

	if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
		metal_info("shutdown message is received.\r\n");
		shutdown_req = 1;
		return RPMSG_SUCCESS;
	}

	memcpy(matrix_array, data, len);
	/* Process received data and multiple matrices. */
	Matrix_Multiply(&matrix_array[0], &matrix_array[1], &matrix_result);

	/* Send the result of matrix multiplication back to host. */
	if (rpmsg_send(ept, &matrix_result, sizeof(matrix)) < 0) {
		metal_err("rpmsg_send failed\r\n");
	}
#ifdef USE_FREERTOS
	xTaskResumeFromISR(rpmsg_task);
#endif /* USE_FREERTOS */
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	metal_err("Endpoint is destroyed\r\n");
	shutdown_req = 1;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int app(struct rpmsg_device *rdev, void *priv)
{
	int ret;

	ret = rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		metal_err("Failed to create endpoint.\r\n");
		return -1;
	}

	metal_info("Waiting for events...\r\n");
	ret = platform_poll_on_vdev_reset(rdev, priv);

	return ret;
}
