# Build Devicetree Binaries and Devicetree Binary Overlays

DTBS := zcu102-openamp-lockstep.dtb zcu102-openamp-split.dtb
DTBS += kria-openamp-lockstep.dtb kria-openamp-split.dtb

#DTBOS := zynqmp-split.dtbo

# DTBC = DTBs as a result of combining a base with one or more overlays
DTBCS := kv260-openamp-lockstep.dtb kv260-openamp-split.dtb


# any file to test we have a valid kernel source dir
XILINX_DTS_DIR := $(LINUX_SRC_DIR)/arch/arm64/boot/dts/xilinx
LINUX_INC_DIR := $(LINUX_SRC_DIR)/include/

SHELL := /bin/bash

all: $(DTBS) $(DTBOS) $(DTBCS)

check-linux-dir:
	@if [ ! -d $(XILINX_DTS_DIR) ]; then \
		echo "LINUX_SRC_DIR must point to a Linux source directory"; \
		echo "LINUX_SRC_DIR=$(LINUX_SRC_DIR)"; \
		exit 2; \
	fi

.PHONY : all check-linux-dir

DTC_CPP_FLAGS= -E -x assembler-with-cpp -nostdinc -undef -D__DTS__

# Note: -@ includes symbols which is need to apply overlays
%.dtb: %.dts
	# Linux DTS uses C preprocessor first
	$(CC) $(DTC_CPP_FLAGS) \
		-I $(XILINX_DTS_DIR) \
		-I $(LINUX_INC_DIR) \
		-o $<.pp $<
	dtc -@ -I dts -O dtb -o $@ $<.pp

%.dtbo: %.dtso
	# Linux DTS uses C preprocessor first
	$(CC) $(DTC_CPP_FLAGS) \
		-I $(XILINX_DTS_DIR) \
		-I $(LINUX_INC_DIR) \
		-o $<.pp $<
	dtc -@ -I dts -O dtb -o $@ $<.pp

$(DTBS): check-linux-dir

zynqmp-sck-kv-g-revB.dtso: $(XILINX_DTS_DIR)/zynqmp-sck-kv-g-revB.dtso
	cp $< $@

kv260-openamp-lockstep.dtb: kria-openamp-lockstep.dtb zynqmp-sck-kv-g-revB.dtbo
	fdtoverlay -o $@ -i $^

kv260-openamp-split.dtb: kria-openamp-split.dtb zynqmp-sck-kv-g-revB.dtbo
	fdtoverlay -o $@ -i $^

clean:
	rm -f $(DTBS) $(DTBOS) $(DTBCS) *.pp
