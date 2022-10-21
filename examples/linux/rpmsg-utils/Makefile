EPT := rpmsg_export_ept
EPTD = rpmsg_destroy_ept
PING = rpmsg_ping

CFLAGS := -Wall -g -O2
LDFLAGS := 
prefix := /usr/local

SRCS := rpmsg_export_ept.c
SRCS += rpmsg_destroy_ept.c
SRCS += rpmsg_ping.c
OBJS := $(SRCS:.c=.o)

all: $(EPT) $(EPTD) $(PING)
.PHONY : all

$(EPT): rpmsg_export_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(EPTD): rpmsg_destroy_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(PING): rpmsg_ping.o
	$(CC) $(LDFLAGS) -o $@ $^

install: $(EPT) $(EPTD) $(PING)
	install -D -m 755 $(EPT) $(DESTDIR)$(prefix)/bin/$(EPT)
	install -D -m 755 $(EPTD) $(DESTDIR)$(prefix)/bin/$(EPTD)
	install -D -m 755 $(PING) $(DESTDIR)$(prefix)/bin/$(PING)

clean:
	rm -f $(EPT) $(EPTD) $(PING) $(OBJS)

distclean:
	rm -f $(DESTDIR)$(prefix)/bin/$(EPT)
	rm -f $(DESTDIR)$(prefix)/bin/$(EPTD)
	rm -f $(DESTDIR)$(prefix)/bin/$(PING)