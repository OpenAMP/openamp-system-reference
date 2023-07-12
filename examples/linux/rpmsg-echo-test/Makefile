
APP = echo_test
APP_OBJS = echo_test.o ../common/common.o

# Add any other object files to this list below


all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	rm -rf $(APP) $(APP_OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

