EPT := rpmsg_export_ept
EPTD = rpmsg_destroy_ept
PING = rpmsg_ping
DEV  = rpmsg_export_dev

CFLAGS := -Wall -g -O2
LDFLAGS := 
prefix := /usr/local

SRCS := rpmsg_export_ept.c
SRCS += rpmsg_destroy_ept.c
SRCS += rpmsg_ping.c
SRCS += rpmsg_export_dev.c
OBJS := $(SRCS:.c=.o)

all: $(EPT) $(EPTD) $(PING) $(DEV)
.PHONY : all

$(EPT): rpmsg_export_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(EPTD): rpmsg_destroy_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(PING): rpmsg_ping.o
	$(CC) $(LDFLAGS) -o $@ $^


$(DEV): rpmsg_export_dev.o
	$(CC) $(LDFLAGS) -o $@ $^

install: $(EPT) $(EPTD) $(PING) $(DEV)
	install -D -m 755 $(EPT) $(DESTDIR)$(prefix)/bin/$(EPT)
	install -D -m 755 $(EPTD) $(DESTDIR)$(prefix)/bin/$(EPTD)
	install -D -m 755 $(PING) $(DESTDIR)$(prefix)/bin/$(PING)
	install -D -m 755 $(DEV) $(DESTDIR)$(prefix)/bin/$(DEV)

clean:
	rm -f $(EPT) $(EPTD) $(PING) $(OBJS) $(DEV)

distclean:
	rm -f $(DESTDIR)$(prefix)/bin/$(EPT)
	rm -f $(DESTDIR)$(prefix)/bin/$(EPTD)
	rm -f $(DESTDIR)$(prefix)/bin/$(PING)
	rm -f $(DESTDIR)$(prefix)/bin/$(DEV)
