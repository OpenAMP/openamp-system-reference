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
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* rpmsg_hdr.len is 16-bit; buffer size itself is configurable via virtio config space */
#define RPMSG_MAX_MESSAGE_SIZE UINT16_MAX

static void usage(void)
{
	extern char *__progname;

	fprintf(stderr, "%s <dev>\n", __progname);
	exit(1);
}

int main(int argc, char **argv)
{
	char *buffer;
	int ret;
	int fd;

	if (argc != 2)
		usage();

	fd = open(argv[1], O_RDWR);
	if (fd < 0)
		err(1, "failed to open %s\n", argv[1]);

	buffer = malloc(RPMSG_MAX_MESSAGE_SIZE + 1);
	if (!buffer)
		err(1, "failed to allocate message buffer");

	ret = snprintf(buffer, RPMSG_MAX_MESSAGE_SIZE + 1, "ping %s",  argv[1]);
	if (ret < 0 || ret > RPMSG_MAX_MESSAGE_SIZE)
		errx(1, "failed to format ping message for %s", argv[1]);

	ret = write(fd, buffer, (size_t)ret);
	if (ret < 0) {
		fprintf(stderr, "failed to write endpoint  %s", argv[1]);
		free(buffer);
		exit(1);
	}

	ret = read(fd, buffer, RPMSG_MAX_MESSAGE_SIZE);
	if (ret < 0) {
		fprintf(stderr, "failed to read endpoint %s", argv[1]);
		free(buffer);
		exit(1);
	}
	buffer[ret] = '\0';
	fprintf(stderr, "message received: \"%.*s\"\n", ret, buffer);

	close(fd);
	free(buffer);

	return 0;
}
