# Build Devicetree Binaries and Devicetree Binary Overlays

DTBS := zcu102-openamp-lockstep.dtb zcu102-openamp-split.dtb
DTBS += kria-openamp-lockstep.dtb kria-openamp-split.dtb
DTBS += zynqmp-smk-k26-revA.dtb zynqmp-zcu102-rev1.0.dtb

DTBOS := zynqmp-split.dtbo zcu102-openamp.dtbo zynqmp-openamp.dtbo
DTBOS += zynqmp-sck-kv-g-revB.dtbo

# any file to test we have a valid kernel source dir
XILINX_DTS_DIR := $(LINUX_SRC_DIR)/arch/arm64/boot/dts/xilinx
LINUX_INC_DIR := $(LINUX_SRC_DIR)/include/

SHELL := /bin/bash

all: $(DTBS) $(DTBOS) $(DTBCS)

.linux-src-check:
	@if [ ! -d $(XILINX_DTS_DIR) ]; then \
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

DTB_KERNEL_SRC := \
	zynqmp-zcu102-rev1.0.dts \
	zynqmp-smk-k26-revA.dts \
	zynqmp-sck-kv-g-revB.dtso

$(DTBS) $(DTBOS) $(DTB_KERNEL_SRC):  .linux-src-check

# copy needed sources from Kernel
zynqmp-zcu102-rev1.0.dts: $(XILINX_DTS_DIR)/zynqmp-zcu102-rev1.0.dts
	cp $< $@

zynqmp-smk-k26-revA.dts: $(XILINX_DTS_DIR)/zynqmp-smk-k26-revA.dts
	cp $< $@

zynqmp-sck-kv-g-revB.dtso: $(XILINX_DTS_DIR)/zynqmp-sck-kv-g-revB.dtso
	cp $< $@

zcu102-openamp-lockstep.dtb: \
	zynqmp-zcu102-rev1.0.dtb \
	zynqmp-openamp.dtbo \
	zcu102-openamp.dtbo
	fdtoverlay -o $@ -i $(filter-out .linux-src-check,$^)

zcu102-openamp-split.dtb: \
	zynqmp-zcu102-rev1.0.dtb \
	zynqmp-openamp.dtbo \
	zcu102-openamp.dtbo \
	zynqmp-split.dtbo
	fdtoverlay -o $@ -i $(filter-out .linux-src-check,$^)

kv260-openamp-lockstep.dtb: \
	zynqmp-smk-k26-revA.dtb zynqmp-sck-kv-g-revB.dtbo \
	zynqmp-openamp.dtbo
	fdtoverlay -o $@ -i $(filter-out .linux-src-check,$^)

kv260-openamp-split.dtb: \
	zynqmp-smk-k26-revA.dtb zynqmp-sck-kv-g-revB.dtbo \
	zynqmp-openamp.dtbo \
	zynqmp-split.dtbo
	fdtoverlay -o $@ -i $(filter-out .linux-src-check,$^)

clean:
	rm -f $(DTBS) $(DTBOS) $(DTBCS) *.pp .linux-src-check $(DTB_KERNEL_SRC)
