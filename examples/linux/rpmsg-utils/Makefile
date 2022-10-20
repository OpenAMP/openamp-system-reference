EPT := rpmsg_export_ept

CFLAGS := -Wall -g -O2
LDFLAGS := 
prefix := /usr/local

SRCS := rpmsg_export_ept.c
OBJS := $(SRCS:.c=.o)

$(EPT): rpmsg_export_ept.o
	$(CC) $(LDFLAGS) -o $@ $^

all: $(EPT)
.PHONY : all

	$(CC) $(LDFLAGS) -o $@ $^

install: $(EPT)
	install -D -m 755 $(EPT) $(DESTDIR)$(prefix)/bin/$(EPT)

clean:
	rm -f $(EPT) $(OBJS)

distclean:
	rm -f $(DESTDIR)$(prefix)/bin/$(EPT)
