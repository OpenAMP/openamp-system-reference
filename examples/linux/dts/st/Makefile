# Build Devicetree Binaries and Devicetree Binary Overlays

DTBS := stm32mp157c-dk2.dtb stm32mp157c-dk2-no-audio.dtb

DTBOS := st-audio-disable.dtbo

# any file to test we have a valid kernel source dir
ST_DTS_DIR := $(LINUX_SRC_DIR)/arch/arm/boot/dts/st
LINUX_INC_DIR := $(LINUX_SRC_DIR)/include/

SHELL := /bin/bash

all: $(DTBS) $(DTBOS)

.linux-src-check:
	@if [ ! -d $(ST_DTS_DIR) ]; then \
		echo "LINUX_SRC_DIR must point to a Linux source directory"; \
		echo "LINUX_SRC_DIR=$(LINUX_SRC_DIR)"; \
		exit 2; \
	fi
	touch .linux-src-check

.PHONY : all

DTC_CPP_FLAGS= -E -x assembler-with-cpp -nostdinc -undef -D__DTS__

# Note: -@ includes symbols which is need to apply overlays
# We always enable overlays
%.dtb: %.dts
	# Linux DTS uses C preprocessor first
	$(CC) $(DTC_CPP_FLAGS) \
		-I $(ST_DTS_DIR) \
		-I $(LINUX_INC_DIR) \
		-o $<.pp $<
	dtc -@ -I dts -O dtb -o $@ $<.pp

%.dtbo: %.dtso
	# Linux DTS uses C preprocessor first
	$(CC) $(DTC_CPP_FLAGS) \
		-I $(ST_DTS_DIR) \
		-I $(LINUX_INC_DIR) \
		-o $<.pp $<
	dtc -@ -I dts -O dtb -o $@ $<.pp

DTB_KERNEL_SRC := \
	stm32mp157c-dk2.dts

$(DTBS) $(DTBOS) $(DTB_KERNEL_SRC):  .linux-src-check

# copy needed sources from Kernel
stm32mp157c-dk2.dts: $(ST_DTS_DIR)/stm32mp157c-dk2.dts
	cp $< $@

stm32mp157c-dk2-no-audio.dtb: \
	stm32mp157c-dk2.dtb \
	st-audio-disable.dtbo
	fdtoverlay -o $@ -i $(filter-out .linux-src-check,$^)

clean:
	rm -f $(DTBS) $(DTBOS) $(DTBCS) *.pp .linux-src-check $(DTB_KERNEL_SRC)
