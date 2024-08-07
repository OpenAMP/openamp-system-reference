//SPDX-License-Identifier: BSD-3-Clause
/*
 * echo_test.c
 *
 *  Created on: Oct 4, 2014
 *      Author: etsam
 */

/*
 * The application sends chunks of data to the
 * remote processor. The remote side echoes the data back
 * to application which then app validates the data returned.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <linux/rpmsg.h>

#include "../common/common.h"

struct _payload {
	unsigned long num;
	unsigned long size;
	char data[];
};

#define RPMSG_GET_KFIFO_SIZE 1
#define RPMSG_GET_AVAIL_DATA_SIZE 2
#define RPMSG_GET_FREE_SPACE 3

#define RPMSG_HEADER_LEN 16
#define MAX_RPMSG_BUFF_SIZE (512 - RPMSG_HEADER_LEN)
#define PAYLOAD_MIN_SIZE	1
#define PAYLOAD_MAX_SIZE	(MAX_RPMSG_BUFF_SIZE - 24)
#define NUM_PAYLOADS		(PAYLOAD_MAX_SIZE/PAYLOAD_MIN_SIZE)

#define RPMSG_BUS_SYS "/sys/bus/rpmsg"

#define PR_DBG(fmt, args ...) printf("%s():%u "fmt, __func__, __LINE__, ##args)
#define SHUTDOWN_MSG    0xEF56A55A
void send_shutdown(int fd)
{
	union {
		unsigned int n[8];
		struct _payload sdown;
	} umsg = {
		.n = {
			SHUTDOWN_MSG, SHUTDOWN_MSG, SHUTDOWN_MSG, SHUTDOWN_MSG,
			SHUTDOWN_MSG, SHUTDOWN_MSG, SHUTDOWN_MSG, SHUTDOWN_MSG,
		}
	};

	umsg.sdown.size = sizeof(umsg);
	if (write(fd, &umsg, sizeof(umsg)) < 0)
		perror("write SHUTDOWN_MSG\n");
}

void print_help(void)
{
	extern char *__progname;

	printf("\r\nusage: %s [option: -d, -c, -n, -s, -e]\r\n", __progname);
	printf("-d - rpmsg device name\r\n");
	printf("-c - rpmsg control device name\r\n");
	printf("-n - number of times payload will be sent\r\n");
	printf("-s - source end point address\r\n");
	printf("-e - destination end point address\r\n");
	printf("\r\n");
}

int main(int argc, char *argv[])
{
	int ret, i, j;
	int size, bytes_rcvd, bytes_sent, err_cnt = 0;
	int opt, charfd, fd;
	int ntimes = 1;
	char rpmsg_dev[NAME_MAX] = "virtio0.rpmsg-openamp-demo-channel.-1.0";
	char rpmsg_ctrl_dev_name[NAME_MAX] = "virtio0.rpmsg_ctrl.0.0";
	char rpmsg_char_name[16];
	char fpath[2*NAME_MAX];
	struct rpmsg_endpoint_info eptinfo = {
		.name = "rpmsg-openamp-demo-channel", .src = 0, .dst = 0
	};
	char ept_dev_name[16];
	char ept_dev_path[32];
	struct _payload *i_payload;
	struct _payload *r_payload;

	printf("\r\n Echo test start \r\n");
	lookup_channel(rpmsg_dev, &eptinfo);

	while ((opt = getopt(argc, argv, "d:c:n:s:e:")) != -1) {
		switch (opt) {
		case 'd':
			memset(rpmsg_dev, 0, sizeof(rpmsg_dev));
			strncpy(rpmsg_dev, optarg, sizeof(rpmsg_dev));
			break;
		case 'c':
			memset(rpmsg_ctrl_dev_name, 0, sizeof(rpmsg_ctrl_dev_name));
			strncpy(rpmsg_ctrl_dev_name, optarg, sizeof(rpmsg_ctrl_dev_name));
			break;
		case 'n':
			ntimes = strtol(optarg, NULL, 10);
			break;
		case 's':
			eptinfo.src = strtol(optarg, NULL, 10);
			break;
		case 'e':
			eptinfo.dst = strtol(optarg, NULL, 10);
			break;
		default:
			print_help();
			return -EINVAL;
		}
	}


	sprintf(fpath, RPMSG_BUS_SYS "/devices/%s", rpmsg_dev);
	if (access(fpath, F_OK)) {
		fprintf(stderr, "access(%s): %s\n", fpath, strerror(errno));
		return -EINVAL;
	}
	ret = bind_rpmsg_chrdev(rpmsg_dev);
	if (ret < 0)
		return ret;

	/* kernel >= 6.0 has new path for rpmsg_ctrl device */
	charfd = get_rpmsg_chrdev_fd(rpmsg_ctrl_dev_name, rpmsg_char_name);
	if (charfd < 0) {
		/* may be kernel is < 6.0 try previous path */
		charfd = get_rpmsg_chrdev_fd(rpmsg_dev, rpmsg_char_name);
		if (charfd < 0)
			return charfd;
	}

	/* Create endpoint from rpmsg char driver */
	PR_DBG("app_rpmsg_create_ept: %s[src=%#x,dst=%#x]\n",
		eptinfo.name, eptinfo.src, eptinfo.dst);
	ret = app_rpmsg_create_ept(charfd, &eptinfo);
	if (ret) {
		fprintf(stderr, "app_rpmsg_create_ept %s\n", strerror(errno));
		return -EINVAL;
	}
	if (!get_rpmsg_ept_dev_name(rpmsg_char_name, eptinfo.name,
				    ept_dev_name))
		return -EINVAL;
	sprintf(ept_dev_path, "/dev/%s", ept_dev_name);

	printf("open %s\n", ept_dev_path);
	fd = open(ept_dev_path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror(ept_dev_path);
		close(charfd);
		return -1;
	}

	i_payload = (struct _payload *)malloc(2 * sizeof(unsigned long) + PAYLOAD_MAX_SIZE);
	r_payload = (struct _payload *)malloc(2 * sizeof(unsigned long) + PAYLOAD_MAX_SIZE);

	if (i_payload == 0 || r_payload == 0) {
		printf("ERROR: Failed to allocate memory for payload.\n");
		return -1;
	}

	for (j=0; j < ntimes; j++){
		printf("\r\n **********************************");
		printf("****\r\n");
		printf("\r\n  Echo Test Round %d \r\n", j);
		printf("\r\n **********************************");
		printf("****\r\n");
		for (i = 0, size = PAYLOAD_MIN_SIZE; i < NUM_PAYLOADS;
		i++, size++) {
			int k;

			i_payload->num = i;
			i_payload->size = size;

			/* Mark the data buffer. */
			memset(&(i_payload->data[0]), 0xA5, size);

			printf("\r\n sending payload number");
			printf(" %ld of size %lu\r\n", i_payload->num,
			(2 * sizeof(unsigned long)) + size);

			bytes_sent = write(fd, i_payload,
			(2 * sizeof(unsigned long)) + size);

			if (bytes_sent <= 0) {
				printf("\r\n Error sending data");
				printf(" .. \r\n");
				break;
			}
			printf("echo test: sent : %d\n", bytes_sent);

			r_payload->num = 0;
			bytes_rcvd = read(fd, r_payload,
					(2 * sizeof(unsigned long)) + PAYLOAD_MAX_SIZE);
			while (bytes_rcvd <= 0) {
				usleep(10000);
				bytes_rcvd = read(fd, r_payload,
					(2 * sizeof(unsigned long)) + PAYLOAD_MAX_SIZE);
			}
			printf(" received payload number ");
			printf("%ld of size %d\r\n", r_payload->num, bytes_rcvd);

			/* Validate data buffer integrity. */
			for (k = 0; k < r_payload->size; k++) {

				if (r_payload->data[k] != 0xA5) {
					printf(" \r\n Data corruption");
					printf(" at index %d \r\n", k);
					err_cnt++;
					break;
				}
			}
			bytes_rcvd = read(fd, r_payload,
			(2 * sizeof(unsigned long)) + PAYLOAD_MAX_SIZE);

		}
		printf("\r\n **********************************");
		printf("****\r\n");
		printf("\r\n Echo Test Round %d Test Results: Error count = %d\r\n",
		j, err_cnt);
		printf("\r\n **********************************");
		printf("****\r\n");
	}

	send_shutdown(fd);
	free(i_payload);
	free(r_payload);

	close(fd);
	if (charfd >= 0)
		close(charfd);
	return 0;
}
