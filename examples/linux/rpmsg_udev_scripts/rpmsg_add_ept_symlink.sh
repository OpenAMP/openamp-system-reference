#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2026, Advanced Micro Devices, Inc.

# valid for kernel 6.18 and later
rpmsg_dev=$1
max_tries=10
retry_delay=0.1
try=1

# find control device for this channel
# e.g. /sys/bus/rpmsg/devices/virtio0.rpmsg_ctrl.0.0/rpmsg/rpmsg_ctrl0/rpmsg0
while [ "$try" -le "$max_tries" ]; do
	rpmsg_dev_dir=$(find -L /sys/bus/rpmsg/devices/ -maxdepth 4 -name "$rpmsg_dev" 2>/dev/null)

	if [ -n "$rpmsg_dev_dir" ] &&
	   [ -r "$rpmsg_dev_dir/name" ] &&
	   [ -r "$rpmsg_dev_dir/src" ] &&
	   [ -r "$rpmsg_dev_dir/dst" ]; then
		ept_name=$(cat "$rpmsg_dev_dir/name")  # e.g. rpmsg-openamp-demo-channel
		ept_src=$(cat "$rpmsg_dev_dir/src")    # e.g. -1
		ept_dst=$(cat "$rpmsg_dev_dir/dst")    # e.g. 1024

		if [ -n "$ept_name" ] &&
		   [ -n "$ept_src" ] &&
		   [ -n "$ept_dst" ]; then
			break
		fi
	fi

	if [ "$try" -eq "$max_tries" ]; then
		echo "failed to find ready sysfs attributes for $rpmsg_dev" >&2
		exit 1
	fi

	sleep "$retry_delay"
	try=$((try + 1))
done

ept_symlink_name="rpmsg_$ept_name.$ept_src.$ept_dst.$rpmsg_dev"
ept_dev_path="/dev/$rpmsg_dev"
ept_symlink_path="/dev/$ept_symlink_name"

# create user mode accessible symlink

# create symlink for apps to use. /dev/rpmsg_rpmsg-openamp-demo-channel.-1.1024.rpmsg0
if [ -L "$ept_symlink_path" ]; then
	if [ "$(readlink "$ept_symlink_path")" = "$ept_dev_path" ]; then
		exit 0
	fi

	unlink "$ept_symlink_path"
elif [ -e "$ept_symlink_path" ]; then
	echo "$ept_symlink_path exists and is not a symlink" >&2
	exit 1
fi

ln -s "$ept_dev_path" "$ept_symlink_path"
