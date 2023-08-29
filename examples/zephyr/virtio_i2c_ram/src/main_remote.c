// SPDX-License-Identifier: Apache-2.0

/* Copyright (c) 2023, STMicroelectronics */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/kernel.h>

#include "mmio_table.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(virtio_i2c_ram, LOG_LEVEL_DBG);

#define I2C_REMOTE_DISP_ADDR 0x3D

/* constant derivated from linker symbols */

#define APP_TASK_STACK_SIZE (4096)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static const struct device *const ipm_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static K_SEM_DEFINE(data_sem, 0, 1);

#define RAM_0_ADDR	0x54
#define RAM_1_ADDR	0x56

#define I2C_RAM_SIZE 20
#define I2C_NUM_RAM  2

/**** MMIO ****/

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT	0
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE	1

#define VIRTIO_I2C_MSG_OK 0
#define VIRTIO_I2C_MSG_ERR 1

#define BitSet(BIT, VALUE) (VALUE & (1 << BIT))

uint64_t features = 1 | ((uint64_t)1 << 32) | ((uint64_t)1 << 33);
uint64_t driver_features;

K_SEM_DEFINE(my_sem, 0, 1);

struct virtio_i2c_out_hdr {
	uint16_t addr;
	uint16_t padding;
	uint32_t flags;
};

typedef struct {
	uint8_t i2c_addr;
	uint8_t padding;
	uint8_t offset;
	uint8_t buff[I2C_RAM_SIZE];

} i2c_ram_t;

static i2c_ram_t rams[I2C_NUM_RAM] = {
	{
		.i2c_addr = 0x54,
		.buff = "123456789abcdefghij",
	},
	{
		.i2c_addr = 0x56,
		.buff = "klmnopqrstuvwxyz!:;",
	}
};

void mmio_interrupt(void)
{
	static struct fw_mmio_table *mmio;
	static uint32_t last_status;

	if (!mmio)
		mmio_table_get(&mmio);

	if (last_status != mmio->status) {
		LOG_DBG("MMIO status: %d\n", mmio->status);
		last_status = mmio->status;
	}

	if (mmio->status == 0) {
		driver_features = 0;
		mmio->queueReady = 0;
	} else if (mmio->status == 3) {
		mmio->deviceFeatures = (uint32_t)(features >> (mmio->deviceFeaturesSel * 32));
		driver_features |=
			((uint64_t)mmio->driverFeatures << (mmio->driverFeaturesSel * 32));
	} else if (mmio->status == 11) {
		if (driver_features != features) {
			LOG_ERR("DriverFeatures does not match our features :(\n");
			mmio->status &= ~(uint32_t)8;
			return;
		}

		mmio->queueNumMax = 16;

	} else if (mmio->status == 15) {
		printk("%s:%d\n", __func__, __LINE__);
		k_sem_give(&my_sem);
	}
}

static int mmio_i2c_read(struct virtio_i2c_out_hdr *header, struct virtq_desc *secdesc,
			  struct virtq_desc *lastdesc)
{
	int i, len  = -1;

	for (i = 0; i < I2C_NUM_RAM; i++) {
		if ((header->addr >> 1) == rams[i].i2c_addr) {
			if (secdesc->len + rams[i].offset > I2C_RAM_SIZE)
				continue;

			printk("read RAM %#x, offset %x length %d\n",
				rams[i].i2c_addr, rams[i].offset,
				secdesc->len);
			memcpy((uint8_t *)(uintptr_t)secdesc->addr,
			       &rams[i].buff[rams[i].offset],
			       secdesc->len);
			len = secdesc->len + 1;
			break;
		}
	}

	return len;
}

static int mmio_i2c_write(struct virtio_i2c_out_hdr *header, struct virtq_desc *secdesc,
			  struct virtq_desc *lastdesc)
{
	int i, len  = -1;

	printk("ping I2C add %#x\n", header->addr >> 1);
	for (i = 0; i < I2C_NUM_RAM; i++) {
		if ((header->addr >> 1) == rams[i].i2c_addr) {
			if (!len) {
				printk("discover RAM %#x\n", rams[i].i2c_addr);
				/* Just return ack */
			} else if (len == 1) {
				/* update read ram offset*/
				rams[i].offset = *(uint8_t *)(uintptr_t)secdesc->addr;
				printk("set RAM %#x offset at %#x\n",
				       rams[i].i2c_addr, rams[i].offset);
			} else {
				/* write in memory */
				if (secdesc->len + rams[i].offset > I2C_RAM_SIZE)
					break;
				rams[i].offset = *(uint8_t *)(uintptr_t)secdesc->addr;
				printk("write RAM %#x, offset %#x length %d\n",
					rams[i].i2c_addr, rams[i].offset,
					secdesc->len - 1);
				memcpy(&rams[i].buff[rams[i].offset],
				       ((uint8_t *)(uintptr_t)secdesc->addr) + 1, secdesc->len - 1);
			}
			len = 1;
			break;
		}
	}

	return len;
}

void mmio_task(void *u1, void *u2, void *u3)
{
	struct virtq_desc *descs;
	struct virtq_avail *avails;
	struct virtq_used *useds;

	static struct fw_mmio_table *mmio;
	struct virtio_i2c_out_hdr *header;
	struct virtq_desc *secdesc;
	struct virtq_desc *lastdesc;
	int len = 0;
	uint8_t *returnCode;

	mmio_table_get(&mmio);

init:

	while (true) {
		uint16_t used = 0;

		if (mmio->status != 15)
			goto init;

		descs = (struct virtq_desc *)(uintptr_t)mmio->queueDesc;
		avails = (struct virtq_avail *)(uintptr_t)mmio->queueDriver;
		useds = (struct virtq_used *)(uintptr_t)mmio->queueDevice;


		while (avails->idx != (uint16_t)(useds->idx + used)) {
			uint16_t currentIDX = (uint16_t)(useds->idx + used) % mmio->queueNum;
			struct virtq_desc *headerDesc = &descs[avails->ring[currentIDX]];

			if (!headerDesc->addr || BitSet(VIRTQ_DESC_F_WRITE, headerDesc->flags)
				|| headerDesc->len != sizeof(struct virtio_i2c_out_hdr)
				|| !BitSet(VIRTQ_DESC_F_NEXT, headerDesc->flags)) {
				LOG_ERR("First buffer is not a valid i2c_out_hdr\n");
				mmio->status |= 64;
				break;
			}

			header = (struct virtio_i2c_out_hdr *)(uintptr_t)headerDesc->addr;
			secdesc = &descs[headerDesc->next];
			lastdesc = BitSet(VIRTQ_DESC_F_NEXT, secdesc->flags) ?
							&descs[secdesc->next] : NULL;

			if (BitSet(1, header->flags)) {	/* read */

				returnCode = (uint8_t *)(uintptr_t)lastdesc->addr;
				*returnCode = VIRTIO_I2C_MSG_ERR;

				if (!lastdesc ||
				    !BitSet(VIRTQ_DESC_F_WRITE, secdesc->flags) ||
				    !BitSet(VIRTQ_DESC_F_WRITE, lastdesc->flags)) {
					LOG_ERR("Read with invalid buffer\n");
					mmio->status |= 64;
					len = 1;
					break;
				}

				len = mmio_i2c_read(header, secdesc, lastdesc);
				if (len < 0)
					len = 1;
				else
					*returnCode = VIRTIO_I2C_MSG_OK;
			} else {	/* write */
				if (!lastdesc) {
					/* Zero length write */
					if (!BitSet(VIRTQ_DESC_F_WRITE, secdesc->flags)) {
						LOG_ERR("Zero length write with invalid buffer\n");
						mmio->status |= 64;
						break;
					}
					returnCode = (uint8_t *)(uintptr_t)secdesc->addr;
					len = 0;
				} else {
					if (BitSet(VIRTQ_DESC_F_WRITE, secdesc->flags)
						|| !BitSet(VIRTQ_DESC_F_WRITE, lastdesc->flags)) {
						LOG_ERR("Write with invalid buffer\n");
						mmio->status |= 64;
						break;
					}
					returnCode = (uint8_t *)(uintptr_t)lastdesc->addr;
					len = secdesc->len;
				}


				len = mmio_i2c_write(header, secdesc, lastdesc);
				if (len < 0) {
					*returnCode = VIRTIO_I2C_MSG_ERR;
					len = 1;
				} else {
					*returnCode = VIRTIO_I2C_MSG_OK;
				}
			}
			useds->ring[currentIDX].id = avails->ring[currentIDX];
			useds->ring[currentIDX].len = len;

			used++;
		}
		if (used) {
			useds->idx = (uint16_t)(useds->idx + used);

			/* Used buffer notification */
			mmio->interruptStatus = 1;
			ipm_send(ipm_handle, 0, 4, NULL, 0);
		}

		k_sem_take(&my_sem, K_FOREVER);
	}
}

/**** ENDMMIO ****/

static void platform_ipm_callback(const struct device *dev,
				  void *context, uint32_t id, volatile void *data)
{
	if (id == 5) {
		mmio_interrupt();
		return;
	}

	LOG_ERR("IPM IRQ\n");
	k_sem_give(&data_sem);
}

static void cleanup_system(void)
{
	ipm_set_enabled(ipm_handle, 0);
}

int platform_init(void)
{
	int status;

	/* setup IPM */
	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device is not ready\n");
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status) {
		LOG_ERR("ipm_set_enabled failed\n");
		return -1;
	}

	return 0;
}

static int openamp_init(void)
{
	int ret;

	/* Initialize platform */
	ret = platform_init();
	if (ret) {
		LOG_ERR("Failed to initialize platform\n");
		goto error_case;
	}

	return 0;

error_case:
	cleanup_system();

	return 1;
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)mmio_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}

SYS_INIT(openamp_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
