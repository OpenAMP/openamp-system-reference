//SPDX-License-Identifier: BSD-3-Clause

/*
 * Sample demo application that showcases inter processor
 * communication from linux userspace to a remote software
 * context. The application generates random matrices and
 * transmits them to the remote context over rpmsg. The
 * remote application performs multiplication of matrices
 * and transmits the results back to this application.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <linux/rpmsg.h>

#include "../common/common.h"

#define RPMSG_BUS_SYS "/sys/bus/rpmsg"

#define SHUTDOWN_MSG    0xEF56A55A
#define MATRIX_SIZE 6

struct _matrix {
	unsigned int size;
	unsigned int elements[MATRIX_SIZE][MATRIX_SIZE];
};

static void matrix_print(struct _matrix *m)
{
	int i, j;

	for (i = 0; i < m->size; ++i) {
		for (j = 0; j < m->size; ++j)
			printf(" %3d ", (unsigned int)m->elements[i][j]);
		printf("\r\n");
	}
	printf("\r\n");
}

static void generate_matrices(int num_matrices,
				unsigned int matrix_size, void *p_data)
{
	int	i, j, k;
	struct _matrix *p_matrix = p_data;
	time_t	t;
	unsigned long value;

	srand((unsigned) time(&t));

	for (i = 0; i < num_matrices; i++) {
		/* Initialize workload */
		p_matrix[i].size = matrix_size;

		for (j = 0; j < matrix_size; j++) {
			for (k = 0; k < matrix_size; k++) {

				value = (rand() & 0x7F);
				value = value % 10;
				p_matrix[i].elements[j][k] = value;
			}
		}

		printf(" \r\n Host : Linux : Input matrix %d \r\n", i);
		matrix_print(&p_matrix[i]);
	}

}

void matrix_mult(int fd)
{
	struct _matrix i_matrix[2];
	struct _matrix r_matrix;

	/* Generate two random matrices */
	generate_matrices(2, MATRIX_SIZE, i_matrix);

	printf("Sending RPMSG: %lu bytes\n", sizeof(i_matrix));
	ssize_t rc = write(fd, i_matrix, sizeof(i_matrix));
	if (rc < 0)
		fprintf(stderr, "write,errno = %ld, %d\n", rc, errno);

	do {
		rc = read(fd, &r_matrix, sizeof(r_matrix));
	} while (rc < (int)sizeof(r_matrix));
	printf("Received RPMSG: %lu bytes\n", rc);

	printf(" \r\n Host : Linux : Printing results \r\n");
	matrix_print(&r_matrix);
}

/* The firmware looks for SHUTDOWN_MSG in the first 32 bits */
void send_shutdown(int fd)
{
	uint32_t msg = SHUTDOWN_MSG;
	if (write(fd, &msg, sizeof(msg)) < 0)
		perror("write SHUTDOWN_MSG\n");
}

void print_help(void)
{
	extern char *__progname;
	printf("\r\nusage: %s [option: -d, -n, -s, -e]\r\n", __progname);
	printf("-d - rpmsg device name\r\n");
	printf("-n - number of times this demo is repeated\r\n");
	printf("-s - source end point address\r\n");
	printf("-e - destination end point address\r\n");
	printf("\r\n");
}

int main(int argc, char *argv[])
{
	int ntimes = 1;
	int opt, ret, fd, charfd = -1;
	char rpmsg_dev[NAME_MAX] = "virtio0.rpmsg-openamp-demo-channel.-1.0";
	char rpmsg_ctrl_dev_name[NAME_MAX] = "virtio0.rpmsg_ctrl.0.0";
	char rpmsg_char_name[16];
	char fpath[2*NAME_MAX];
	struct rpmsg_endpoint_info eptinfo = {
		.name = "rpmsg-openamp-demo-channel", .src = 0, .dst = 0
	};
	char ept_dev_name[16];
	char ept_dev_path[32];

	printf("Matrix multiplication demo start\n");
	lookup_channel(rpmsg_dev, &eptinfo);

	while ((opt = getopt(argc, argv, "d:n:s:e:")) != -1) {
		switch (opt) {
		case 'd':
			strncpy(rpmsg_dev, optarg, sizeof(rpmsg_dev));
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

	/* The Linux kernel >= 6.0 expects rpmsg_ctrl interface under virtio*.rpmsg_ctrl*.* dir */
	charfd = get_rpmsg_chrdev_fd(rpmsg_ctrl_dev_name, rpmsg_char_name);
	if (charfd < 0) {
		/* look for previous interface */
		charfd = get_rpmsg_chrdev_fd(rpmsg_dev, rpmsg_char_name);
		if (charfd < 0)
			return charfd;
	}

	/* Create endpoint from rpmsg char driver */
	printf("Creating RPMSG endpoint with name: %s, src=%#x, dst=%#x\n",
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

	printf("Start of Matrix multiplication demo with %d rounds\n", ntimes);
	for (int i = 0; i < ntimes; i++) {
		matrix_mult(fd);
		printf("End of Matrix multiplication demo round %d\n", i);
	}

	send_shutdown(fd);
	close(fd);
	if (charfd >= 0)
		close(charfd);

	printf("\r\n Quitting application .. \r\n");
	printf(" Matrix multiply application end \r\n");

	return 0;
}
