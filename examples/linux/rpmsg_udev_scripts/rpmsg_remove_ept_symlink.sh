#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2026, Advanced Micro Devices, Inc.

set -e
set +x

# e.g. "rpmsg0"
rpmsg_dev=$1

set +e

# remove symlink ending with this rpmsg device name
for rpmsg_dev_symlink in /dev/rpmsg_*."$rpmsg_dev"; do
	[ -L "$rpmsg_dev_symlink" ] || continue
	unlink "$rpmsg_dev_symlink" 2>/dev/null
done
