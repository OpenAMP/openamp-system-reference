# Copyright (C) 2026, Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#!/bin/bash

# valid for kernel 6.18 and later

rpmsg_dev=$1

# find control device for this channel
# e.g. /sys/bus/rpmsg/devices/virtio0.rpmsg_ctrl.0.0/rpmsg/rpmsg_ctrl0/rpmsg0
rpmsg_dev_dir=$(find -L /sys/bus/rpmsg/devices/ -maxdepth 4 -name $rpmsg_dev 2>/dev/null)

# Get the ept name, src and dst
ept_name=$(cat $rpmsg_dev_dir/name)  # e.g. rpmsg-openamp-demo-channel
ept_src=$(cat $rpmsg_dev_dir/src)    # e.g. -1
ept_dst=$(cat $rpmsg_dev_dir/dst)    # e.g. 1024
ept_symlink_name="$ept_name.$ept_src.$ept_dst"

# create user mode accessible symlink

# create symlink for apps to use. /dev/rpmsg-openamp-demo-channel.-1.1024
ln -s /dev/$rpmsg_dev /dev/$ept_symlink_name

# map rpmsg device with symlink name by creating another symlink that has actual
# rpmsg device name in it. e.g. /dev/rpmsg0.rpmsg-openamp-demo-channel.-1.1024
ln -s /dev/$ept_symlink_name /dev/$rpmsg_dev.$ept_symlink_name
