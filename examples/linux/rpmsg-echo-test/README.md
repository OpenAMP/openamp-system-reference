# Demo: Echo Test

  This demo uses the Linux kernel rpmsg framework to send various size of data buffer to remote
  processor and validates integrity of received buffer from remote processor.
  If buffer data does not match, then number of different bytes are reported on
  console.

  Platform: Xilinx Zynq UltraScale+ MPSoC(a.k.a ZynqMP)

  Board: ZynqMP Zcu102

  ## Remote Processor Firmware (image_echo_test)

  * Remote processor firmware for Xilinx ZynqMP cortex-r5 platform based on: [rpmsg-echo.c](https://github.com/OpenAMP/open-amp/blob/main/apps/examples/echo/rpmsg-echo.c)

  * Instructions to compile: [ZynqMP r5f generic baremetal](https://github.com/OpenAMP/open-amp/blob/main/README.md#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote)

  * RPU firmware elf file is expected in sdk at path: /lib/firmware/

  * Xilinx Vendor specific SDK is required to build RPU firmware: [Xilinx Petalinux](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html)

  * More information is provided here: [Xilinx Wiki page for OpenAMP](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841718/OpenAMP)

  ## Run the Demo

  Assume all the binaries are board specific.

  ```shell
  # Specify Remote Processor Firmware to Be Loaded.
  echo image_echo_test > /sys/class/remoteproc/remoteproc0/firmware

  # Load and Start Target Firmware Onto Remote Processor
  echo start > /sys/class/remoteproc/remoteproc0/state

  # Check Remote Processor State
  cat /sys/class/remoteproc/remoteproc0/state

  # Load rpmsg_char Driver
  modprobe rpmsg_char

  # Load rpmsg_ctrl Driver
  modprobe rpmsg_ctrl

  # Run echo_test Application on Host Processor
  rpmsg_echo_test

  # Unload rpmsg_ctrl Driver
  modprobe -r rpmsg_ctrl

  #unload rpmsg_char Driver
  modprobe -r rpmsg_char

  # Stop Remote Processor
  echo stop > /sys/class/remoteproc/remoteproc0/state
  ```
