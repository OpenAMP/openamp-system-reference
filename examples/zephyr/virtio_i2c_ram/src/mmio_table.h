/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023, STMicroelectronics
 */

#include <openamp/virtio.h>

#define STATUS_ACK		1
#define STATUS_DRIVER		2
#define STATUS_DRIVER_OK	4
#define STATUS_FEATURES_OK	8
#define STATUS_RESET_DEVICE	64
#define STATUS_FAILED		128

/* Virtqueue descriptors: 16 bytes.
 * These can chain together via "next".
 */
struct virtq_desc {
	/* Address (guest-physical). */
	volatile uint64_t addr;
	/* Length. */
	volatile uint32_t len;
	/* The flags as indicated above. */
	volatile uint16_t flags;
	/* We chain unused descriptors via this, too */
	volatile uint16_t next;
};

struct virtq_avail {
	volatile uint16_t flags;
	volatile uint16_t idx;
	volatile uint16_t ring[];
	/* Only if VIRTIO_F_EVENT_IDX: le16 used_event; */
};

/* le32 is used here for ids for padding reasons. */
struct virtq_used_elem {
	/* Index of start of used descriptor chain. */
	volatile uint32_t id;
	/* Total length of the descriptor chain which was written to. */
	volatile uint32_t len;
};

struct virtq_used {
	volatile uint16_t flags;
	volatile uint16_t idx;
	volatile struct virtq_used_elem ring[];
	/* Only if VIRTIO_F_EVENT_IDX: le16 avail_event; */
};

struct fw_mmio_table {

	/* 0x00 R should be 0x74726976 */
	uint32_t magic;

	/* 0x04 R */
	uint32_t version;

	/* 0x08 R 34 for i2c */
	uint32_t deviceType;

	/* 0x0c R */
	uint32_t vendoId;

	/* 0x10 R flags to represent features supported by the device */
	uint32_t deviceFeatures;

	/* 0x14 W selected features from the device */
	volatile uint32_t deviceFeaturesSel;

	uint32_t reserved[2];

	/* 0x20 W flags to represent features supported by the driver */
	volatile uint32_t driverFeatures;

	/* 0x24 W selected features from the driver */
	volatile uint32_t driverFeaturesSel;

	uint32_t reserved2[2];

	/* 0x30 W virtual queue index */
	volatile uint32_t queueSel;

	/* 0x34 R maximum virtual queue size */
	uint32_t queueNumMax;

	/* 0x38 W virtual queue size */
	volatile uint32_t queueNum;

	uint32_t reserved3[2];

	/* 0x44 RW virtual queue ready bit */
	volatile uint32_t queueReady;

	uint32_t reserved4[2];

	/* 0x50 W queue notifier */
	volatile uint32_t queueNotify;

	uint32_t reserved5[3];

	/* 0x60 R bit mask of events that caused the device interrupt to be asserted */
	volatile uint32_t interruptStatus;

	/* 0x64 W bit mask notifies the device that events causing the interrupt,
	 * have been handled.
	 */
	volatile uint32_t interruptACK;

	uint32_t reserved6[2];

	/* 0x70 RW Writing non-zero values to this register sets the status flags,
	 * indicating the driver progress.
	 * Writing zero (0x0) to this register triggers a device reset.
	 */
	volatile uint32_t status;

	uint32_t reserved7[3];

	/* 0x80 W notifies the device about location of the Descriptor Area of the queue,
	 * selected by writing to QueueSel register
	 */
	volatile uint64_t queueDesc;

	uint32_t reserved8[2];

	/* 0x90 W notifies the device about location of the Driver Area of the queue selected,
	 * by writing to QueueSel.
	 */
	volatile uint64_t queueDriver;

	uint32_t reserved9[2];

	/* 0xa0 W notifies the device about location of the Device Area of the queue selected,
	 * by writing to QueueSel.
	 */
	volatile uint64_t queueDevice;

	uint32_t reserved10[23];

	/* 0xfc R value describing a version of the device-specific configuration space */
	uint32_t configGeneration;
};

void mmio_table_get(struct fw_mmio_table **table_ptr);
