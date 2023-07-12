
APP = proxy_app
APP_OBJS = proxy_app.o ../common/common.o

# Add any other object files to this list below


all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	rm -rf $(APP) $(APP_OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

