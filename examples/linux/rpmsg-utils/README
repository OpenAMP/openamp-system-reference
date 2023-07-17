rpmsgexport
===========

rpmsg_export_ept implements RPMSG_CREATE_EPT_IOCTL for usage in udev rules to
automatically create endpoint devices as remoteproc devices are booted.

An example of udev-rule that automatically exposes the APPS_RIVA_CTRL channel
to user space as the pronto core comes up:

ACTION=="add", SUBSYSTEM=="rpmsg", \
	       KERNEL=="rpmsg_ctrl[0-9]*", \
	       ATTRS{rpmsg_name}=="pronto", \
	       RUN+="rpmsg_export_ept /dev/$name APPS_RIVA_CTRL"

Which together with the following two udev rules creates a nice directory
structure of rpmsg endpoint devices under /dev/rpmsg:

SUBSYSTEM=="rpmsg", KERNEL=="rpmsg_ctrl[0-9]*", \
		    ATTRS{rpmsg_name}=="?*", \
		    SYMLINK+="rpmsg/$attr{rpmsg_name}/ctrl"
SUBSYSTEM=="rpmsg", KERNEL=="rpmsg[0-9]*", \
		    ATTR{name}=="?*", \
		    ATTRS{rpmsg_name}=="?*", \
		    SYMLINK+="rpmsg/$attr{rpmsg_name}/$attr{name}"

eptdestroy
====
"rpmsg_destroy_ept" implements RPMSG_DESTROY_EPT_IOCTL to destroy an endpoint created by the RPMSG_CREATE_EPT_IOCTL.

ping
====
"rpmsg_ping" is a test binary that opens a rpmsg chardev for an echo test (write a pattern and wait an answer)

rpmsgexportdev
==============
"rpmsg_export_dev" implements RPMSG_CREATE_DEV_IOCTL to create local RPMsg device
as remoteproc devices are booted.
Adding option -d as first argument release the device using RPMSG_RELEASE_DEV_IOCTL control.