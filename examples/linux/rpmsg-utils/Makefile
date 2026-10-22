EPT := rpmsg_export_ept
EPTD = rpmsg_destroy_ept
PING = rpmsg_ping
DEV  = rpmsg_export_dev
UDEV_DIR = rpmsg_udev_scripts
UDEV_RULE = $(UDEV_DIR)/99-rpmsg.rules
UDEV_SCRIPT = $(UDEV_DIR)/rpmsg_create_channel.sh

CFLAGS := -Wall -g -O2
LDFLAGS := 
prefix := /usr/local
udevrulesdir := /etc/udev/rules.d
udevscriptdir := /usr/bin

SRCS := rpmsg_export_ept.c
SRCS += rpmsg_destroy_ept.c
SRCS += rpmsg_ping.c
SRCS += rpmsg_export_dev.c
OBJS := $(SRCS:.c=.o)

all: $(EPT) $(EPTD) $(PING) $(DEV)
.PHONY : all install install-rpmsg-udev clean distclean

$(EPT): rpmsg_export_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(EPTD): rpmsg_destroy_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(PING): rpmsg_ping.o
	$(CC) $(LDFLAGS) -o $@ $^


$(DEV): rpmsg_export_dev.o
	$(CC) $(LDFLAGS) -o $@ $^

install: $(EPT) $(EPTD) $(PING) $(DEV) install-rpmsg-udev
	install -D -m 755 $(EPT) $(DESTDIR)$(prefix)/bin/$(EPT)
	install -D -m 755 $(EPTD) $(DESTDIR)$(prefix)/bin/$(EPTD)
	install -D -m 755 $(PING) $(DESTDIR)$(prefix)/bin/$(PING)
	install -D -m 755 $(DEV) $(DESTDIR)$(prefix)/bin/$(DEV)

install-rpmsg-udev:
	install -D -m 644 $(UDEV_RULE) $(DESTDIR)$(udevrulesdir)/99-rpmsg.rules
	install -D -m 755 $(UDEV_SCRIPT) $(DESTDIR)$(udevscriptdir)/rpmsg_create_channel.sh

clean:
	rm -f $(EPT) $(EPTD) $(PING) $(OBJS) $(DEV)

distclean:
	rm -f $(DESTDIR)$(prefix)/bin/$(EPT)
	rm -f $(DESTDIR)$(prefix)/bin/$(EPTD)
	rm -f $(DESTDIR)$(prefix)/bin/$(PING)
	rm -f $(DESTDIR)$(prefix)/bin/$(DEV)
	rm -f $(DESTDIR)$(udevrulesdir)/99-rpmsg.rules
	rm -f $(DESTDIR)$(udevscriptdir)/rpmsg_create_channel.sh
