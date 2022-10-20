EPT := rpmsg_export_ept
EPTD = rpmsg_destroy_ept

CFLAGS := -Wall -g -O2
LDFLAGS := 
prefix := /usr/local

SRCS := rpmsg_export_ept.c
SRCS += rpmsg_destroy_ept.c
OBJS := $(SRCS:.c=.o)

all: $(EPT) $(EPTD)
.PHONY : all

$(EPT): rpmsg_export_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

$(EPTD): rpmsg_destroy_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

install: $(EPT) $(EPTD)
	install -D -m 755 $(EPT) $(DESTDIR)$(prefix)/bin/$(EPT)
	install -D -m 755 $(EPTD) $(DESTDIR)$(prefix)/bin/$(EPTD)

clean:
	rm -f $(EPT) $(EPTD) $(OBJS)

distclean:
	rm -f $(DESTDIR)$(prefix)/bin/$(EPT)
	rm -f $(DESTDIR)$(prefix)/bin/$(EPTD)