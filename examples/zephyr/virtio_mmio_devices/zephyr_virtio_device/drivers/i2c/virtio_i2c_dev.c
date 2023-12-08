/*
 * Copyright (c) 2023, STMicroelectronics
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "virtio_i2c_dev.h"

int virtio_i2c_configure(struct virtio_i2c_dev *dev,
			 struct virtio_device *vdev,
			 i2c_callback callback,
			 void *callback_data)
{
	if (!dev || !vdev || !callback)
		return -1;

	dev->vdev = vdev;

	dev->callback = callback;
	dev->callback_data = callback_data;

	/** Set VirtIO I2C specific data for the VirtIO device driver
	 * It also make the VirtIO device ready to be used
	 */
	vdev->func->configure_device(vdev, VIRTIO_I2C_F_ZERO_LENGTH_REQUEST,
				     VIRTIO_ID_I2C_ADAPTER, 1, 16);

	return 0;
}

bool virtio_i2c_ready(struct virtio_i2c_dev *dev)
{
	uint8_t status;
	if(virtio_get_status(dev, &status)) {
		return false;
	}

	return status == VIRTIO_CONFIG_STATUS_READY;
}

int virtio_i2c_configure_avail_callback(struct virtio_i2c_dev *dev, vq_callback callback,
					void *callback_arg)
{
	/** VirtIO i2c only uses one virtqueue */
	int ret = dev->vdev->func->create_virtqueues(dev->vdev,
												NULL,
												1,
												&"RX",
												&callback,
												callback_arg);

	if (ret)
		return ret;

	/** The callback function won't be able to know wich device the callback is for
	 * This is why we need to put the target device in the virtqueue
	 */
	dev->vdev->vrings_info[0].vq->priv = callback_arg;
	return 0;
}

int virtio_i2c_handle_avail(struct virtio_i2c_dev *dev)
{
	/** we use the first virtqueue as virtio i2c only need one */
	struct virtqueue *vq = dev->vdev->vrings_info[0].vq;
	void *descs[VIRTIO_I2C_BUFFERS];
	uint16_t idxs[VIRTIO_I2C_BUFFERS];
	uint16_t lens[VIRTIO_I2C_BUFFERS];
	struct virtio_i2c_out_hdr *header;
	uint8_t *returnCode;
	int used = 0, ret;
	bool read;

	if (!vq)
		return -1;

	/** We use all available buffers */
	while (virtqueue_get_desc_size(vq)) {

		/** get the first buffer of the chain */
		descs[0] = virtqueue_get_available_buffer(vq, &idxs[0], &lens[0]);

		/** get the rest of the chain */
		for (int i = 1; i < VIRTIO_I2C_BUFFERS; i++) {
			idxs[i] = idxs[i-1];
			descs[i] = virtqueue_get_available_buffer(vq, &idxs[i], &lens[i]);
		}

		/** we need at least 2 buffers */
		if (!descs[VIRTIO_I2C_HEADER_BUF] ||
		    !descs[VIRTIO_I2C_DATA_BUF])
			return -2;

		/** we may not have the data buffer if it's a zero length requests,
		 * so in that case the second buffer is the return buffer
		 */
		if (!descs[VIRTIO_I2C_RETURN_BUF]) {
			descs[VIRTIO_I2C_RETURN_BUF] = descs[VIRTIO_I2C_DATA_BUF];
			idxs[VIRTIO_I2C_RETURN_BUF] = idxs[VIRTIO_I2C_DATA_BUF];
			lens[VIRTIO_I2C_RETURN_BUF] = lens[VIRTIO_I2C_DATA_BUF];
			descs[VIRTIO_I2C_DATA_BUF] = NULL;
		}

		/** check if i2c header is valid */
		if (lens[VIRTIO_I2C_HEADER_BUF] != sizeof(struct virtio_i2c_out_hdr))
			return -3;

		/** check if the return buf is valid */
		if (!descs[VIRTIO_I2C_RETURN_BUF])
			return -4;

		/** We extract the data from the buffers */
		header = (struct virtio_i2c_out_hdr *)descs[VIRTIO_I2C_HEADER_BUF];
		returnCode = (uint8_t *)descs[VIRTIO_I2C_RETURN_BUF];

		/** check if this is a read or write request */
		read = (VIRTIO_I2C_MSG_READ & header->flags);

		/** if we have a data buffer, then it should be write only for read request
		 * and read only for write request
		 */
		if (descs[VIRTIO_I2C_DATA_BUF])
			return -5;

		struct virtio_i2c_msg msg = {
			.addr = header->addr >> 1,
			.buf = descs[VIRTIO_I2C_DATA_BUF],
			.len = descs[VIRTIO_I2C_DATA_BUF] ? lens[VIRTIO_I2C_DATA_BUF] : 0,
			.flags = ((read ? VIRTIO_I2C_MSG_READ : VIRTIO_I2C_MSG_WRITE) |
				  VIRTIO_I2C_MSG_STOP),
		};

		/** send request on the bus */
		ret = dev->callback(dev->callback_data, msg);

		/** 1 is fail, 0 is success */
		*returnCode = ret ? 1 : 0;

		virtqueue_add_consumed_buffer(vq, idxs[VIRTIO_I2C_HEADER_BUF],
					      (read && !ret) ? msg.len + 1 : 1);
		used++;
	}

	if (used)
		virtqueue_kick(vq);

	return used;
}
