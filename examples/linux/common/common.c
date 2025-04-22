// SPDX-License-Identifier: BSD-3-Clause

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/rpmsg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "common.h"

int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo)
{
	int ret;

	ret = ioctl(rpfd, RPMSG_CREATE_EPT_IOCTL, eptinfo);
	if (ret)
		perror("Failed to create endpoint.\n");
	return ret;
}

char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
			     const char *ept_name,
			     char *ept_dev_name)
{
	char sys_rpmsg_ept_name_path[64];
	char svc_name[64];
	char *sys_rpmsg_path = "/sys/class/rpmsg";
	FILE *fp;
	int i;
	int ept_name_len;

	for (i = 0; i < 128; i++) {
		sprintf(sys_rpmsg_ept_name_path, "%s/%s/rpmsg%d/name",
			sys_rpmsg_path, rpmsg_char_name, i);
		printf("checking %s\n", sys_rpmsg_ept_name_path);
		if (access(sys_rpmsg_ept_name_path, F_OK) < 0)
			continue;
		fp = fopen(sys_rpmsg_ept_name_path, "r");
		if (!fp) {
			printf("failed to open %s\n", sys_rpmsg_ept_name_path);
			break;
		}
		fgets(svc_name, sizeof(svc_name), fp);
		fclose(fp);
		printf("svc_name: %s.\n",svc_name);
		ept_name_len = strlen(ept_name);
		if (ept_name_len > sizeof(svc_name))
			ept_name_len = sizeof(svc_name);
		if (!strncmp(svc_name, ept_name, ept_name_len)) {
			sprintf(ept_dev_name, "rpmsg%d", i);
			return ept_dev_name;
		}
	}

	printf("Not able to RPMsg endpoint file for %s:%s.\n",
	       rpmsg_char_name, ept_name);
	return NULL;
}

int bind_rpmsg_chrdev(const char *rpmsg_dev_name)
{
	char fpath[256];
	const char *rpmsg_chdrv = "rpmsg_chrdev";
	char drv_override[64] = {0};
	int fd;
	int ret;

	/* rpmsg dev overrides path */
	sprintf(fpath, "%s/devices/%s/driver_override",
		RPMSG_BUS_SYS, rpmsg_dev_name);
	printf("open %s\n", fpath);
	fd = open(fpath, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
			fpath, strerror(errno));
		return -EINVAL;
	}

	ret = read(fd, drv_override, sizeof(drv_override));
	if (ret < 0) {
		fprintf(stderr, "Failed to read %s (%s)\n",
			fpath, strerror(errno));
		close(fd);
		return ret;
	}

	printf("current drv override = %s\n", drv_override);

	/*
	 * Check driver override. If "rpmsg_chrdev" string is
	 * found, then don't attempt to bind. If null string is found,
	 * then no driver is bound, and attempt to bind rpmsg char driver.
	 * Any other case, fail binding driver, as device is busy.
	 */
	if (strncmp(drv_override, rpmsg_chdrv, strlen(rpmsg_chdrv)) == 0) {
		close(fd);
		return 0;
	} else if (strncmp(drv_override, "(null)", strlen("(null)")) != 0) {
		fprintf(stderr, "error: device %s is busy, drv bind=%s\n",
		       rpmsg_dev_name, drv_override);
		close(fd);
		return -EBUSY;
	}

	ret = write(fd, rpmsg_chdrv, strlen(rpmsg_chdrv) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
			rpmsg_chdrv, fpath, strerror(errno));
		close(fd);
		return -EINVAL;
	}
	close(fd);

	/* bind the rpmsg device to rpmsg char driver */
	sprintf(fpath, "%s/drivers/%s/bind", RPMSG_BUS_SYS, rpmsg_chdrv);
	fd = open(fpath, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
			fpath, strerror(errno));
		return -EINVAL;
	}
	printf("write %s to %s\n", rpmsg_dev_name, fpath);
	ret = write(fd, rpmsg_dev_name, strlen(rpmsg_dev_name) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
			rpmsg_dev_name, fpath, strerror(errno));
		close(fd);
		return -EINVAL;
	}
	close(fd);
	return 0;
}

int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name, char *rpmsg_ctrl_name)
{
	char dpath[2*NAME_MAX];
	DIR *dir;
	struct dirent *ent;
	int fd;

	sprintf(dpath, "%s/devices/%s/rpmsg", RPMSG_BUS_SYS, rpmsg_dev_name);
	printf("opendir %s\n", dpath);
	dir = opendir(dpath);
	if (dir == NULL) {
		fprintf(stderr, "opendir %s, %s\n", dpath, strerror(errno));
		return -EINVAL;
	}
	while ((ent = readdir(dir)) != NULL) {
		if (!strncmp(ent->d_name, "rpmsg_ctrl", 10)) {
			sprintf(dpath, "/dev/%s", ent->d_name);
			closedir(dir);
			printf("open %s\n", dpath);
			fd = open(dpath, O_RDWR | O_NONBLOCK);
			if (fd < 0) {
				fprintf(stderr, "open %s, %s\n",
					dpath, strerror(errno));
				return fd;
			}
			sprintf(rpmsg_ctrl_name, "%s", ent->d_name);
			return fd;
		}
	}

	fprintf(stderr, "No rpmsg_ctrl file found in %s\n", dpath);
	closedir(dir);
	return -EINVAL;
}

static void set_src_dst(char *out, struct rpmsg_endpoint_info *pep)
{
	long dst = 0;
	char *lastdot = strrchr(out, '.');

	if (lastdot == NULL)
		return;
	dst = strtol(lastdot + 1, NULL, 10);
	if ((errno == ERANGE && (dst == LONG_MAX || dst == LONG_MIN))
	    || (errno != 0 && dst == 0)) {
		return;
	}
	pep->dst = (unsigned int)dst;
}

/*
 * return the first dirent matching rpmsg-openamp-demo-channel
 * in /sys/bus/rpmsg/devices/ E.g.:
 *	virtio0.rpmsg-openamp-demo-channel.-1.1024
 */
int lookup_channel(char *out, struct rpmsg_endpoint_info *pep)
{
	char dpath[] = RPMSG_BUS_SYS "/devices";
	struct dirent *ent;
	DIR *dir = opendir(dpath);

	if (dir == NULL) {
		fprintf(stderr, "opendir %s, %s\n", dpath, strerror(errno));
		return -EINVAL;
	}
	while ((ent = readdir(dir)) != NULL) {
		if (strstr(ent->d_name, pep->name)) {
			strncpy(out, ent->d_name, NAME_MAX);
			set_src_dst(out, pep);
			printf("using dev file: %s\n", out);
			closedir(dir);
			return 0;
		}
	}
	closedir(dir);
	fprintf(stderr, "No dev file for %s in %s\n", pep->name, dpath);
	return -EINVAL;
}
