
APP = mat_mul_demo
APP_OBJS = mat_mul_demo.o ../common/common.o

# Add any other object files to this list below


all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS) -lpthread

clean:
	rm -rf $(APP) $(APP_OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

