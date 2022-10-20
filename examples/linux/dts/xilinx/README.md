#### zcu102-openamp-lockstep.dts
  - Base dts from Yocto 2022.1_update3 release: https://github.com/Xilinx/yocto-manifests/tree/xlnx-rel-v2022.1_update3
  - `MACHINE=zcu102-zynqmp bitbake device-tree` will generate system-top.dtb
  - Remoteproc nodes are available to configure RPU and TCM in lockstep mode using Xilinx downstream driver
  - Board: Xilinx zcu102
  - Works for QEMU
  - Generate DTB: `dtc -I dts -O dtb -o zcu102-openamp-lockstep.dtb zcu102-openamp-lockstep.dts`

#### zcu102-openamp-split.dts
  - Base dts from Yocto 2022.1_update3 release: https://github.com/Xilinx/yocto-manifests/tree/xlnx-rel-v2022.1_update3
  - `MACHINE=zcu102-zynqmp bitbake device-tree` will generate system-top.dtb that is base dtb
  - Added Remoteproc nodes to configure RPU and TCM in split mode using Xilinx downstream driver
  - Board: Xilinx zcu102
  - Works for QEMU
  - Generate DTB: `dtc -I dts -O dtb -o zcu102-openamp-split.dtb zcu102-openamp-split.dts`
