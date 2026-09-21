# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2026, Advanced Micro Devices, Inc.

prefix ?= /usr
bindir ?= $(prefix)/bin
udevdir ?= /etc/udev/rules.d

INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL) -m 0755
INSTALL_DATA ?= $(INSTALL) -m 0644

SCRIPTS := \
	rpmsg_add_ept_symlink.sh \
	rpmsg_create_channel.sh \
	rpmsg_remove_ept_symlink.sh

UDEV_RULES := 99-rpmsg.rules

.PHONY: all install uninstall clean

all:

install:
	$(INSTALL) -d "$(DESTDIR)$(bindir)"
	$(INSTALL) -d "$(DESTDIR)$(udevdir)"
	$(INSTALL_PROGRAM) $(SCRIPTS) "$(DESTDIR)$(bindir)"
	$(INSTALL_DATA) $(UDEV_RULES) "$(DESTDIR)$(udevdir)"

uninstall:
	$(RM) $(addprefix "$(DESTDIR)$(bindir)"/,$(SCRIPTS))
	$(RM) $(addprefix "$(DESTDIR)$(udevdir)"/,$(UDEV_RULES))

clean:
