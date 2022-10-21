/*
 * Copyright (C) 2022, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct rpmsg_endpoint_info {
	char name[32];
	uint32_t src;
	uint32_t dst;
};

#define RPMSG_CREATE_DEV_IOCTL	_IOW(0xb5, 0x3, struct rpmsg_endpoint_info)
#define RPMSG_RELEASE_DEV_IOCTL	_IOW(0xb5, 0x4, struct rpmsg_endpoint_info)

static void usage(void)
{
	extern char *__progname;

	fprintf(stderr, "%s <ctrl> [-d] <name> [<src> <dst>]\n", __progname);
	exit(1);
}

int main(int argc, char **argv)
{
	struct rpmsg_endpoint_info ept;
	int ret, fd;
	int release = 0;
	char *endptr;

	if (argv[2][0] == '-') {
		if (argv[1][1] == 'd') {
			if (argc != 4 && argc != 6)
				goto error_usage;
			release = 1;
		} else if (argv[1][1] == 'h') {
			usage();
			return 0;
		} else {
			printf("Invalid option.\n");
			goto error_usage;
		}
	} else if (argc != 3 && argc != 5) {
		goto error_usage;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0)
		err(1, "failed to open %s\n", argv[1]);

	strncpy(ept.name, argv[2 + release], sizeof(ept.name));
	ept.name[sizeof(ept.name) - 1] = '\0';

	if (argc >= 5) {
		ept.src = strtoul(argv[3 + release], &endptr, 10);

		if (*endptr)
			usage();

		ept.dst = strtoul(argv[4 + release], &endptr, 10);

		if (*endptr)
			usage();
	}

	if (!release)
		ret = ioctl(fd, RPMSG_CREATE_DEV_IOCTL, &ept);
	else
		ret = ioctl(fd, RPMSG_RELEASE_DEV_IOCTL, &ept);
	if (ret < 0)
		fprintf(stderr, "failed to %s rpmsg device\n", release ? "release" : "create");
	close(fd);
	return ret;

error_usage:
	usage();
	return 2;
}
