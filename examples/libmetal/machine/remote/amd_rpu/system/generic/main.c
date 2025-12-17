/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "common.h"

int main(void)
{
	int ret;

	ret = demo(NULL);
	if (ret)
		metal_err("REMOTE: Demo exited with %d\n", ret);

	return ret;
}
