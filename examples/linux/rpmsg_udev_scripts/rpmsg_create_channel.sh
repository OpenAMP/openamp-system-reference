#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2026, Advanced Micro Devices, Inc.

# e.g. virtio0.rpmsg-openamp-demo-channel.-1.1024
rpmsg_dev=$1

# Split from the end so channel names can contain dots.
ch_dest="${rpmsg_dev##*.}"
rpmsg_dev_rest="${rpmsg_dev%.*}"
ch_src="${rpmsg_dev_rest##*.}"
rpmsg_dev_rest="${rpmsg_dev_rest%.*}"
virtio_name="${rpmsg_dev_rest%%.*}"
ch_name="${rpmsg_dev_rest#*.}"

#find control device for this channel e.g. /dev/rpmsg_ctrl0
rpmsg_ctrl_dev=$(ls "/sys/bus/rpmsg/devices/$rpmsg_dev/subsystem/devices/$virtio_name.rpmsg_ctrl.0.0/rpmsg/")

# create endpoint for this channel e.g. /dev/rpmsg0
res=$(rpmsg_export_ept "/dev/$rpmsg_ctrl_dev" "$ch_name" "$ch_src" "$ch_dest")
