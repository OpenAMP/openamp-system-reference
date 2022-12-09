# Demo: echo_test

  This demo uses the Linux kernel rpmsg framework to send various size of data buffer to remote
  processor and validates integrity of received buffer from remote processor.
  If buffer data does not match, then number of different bytes are reported on
  console.

  Platform: Xilinx Zynq UltraScale+ MPSoC(a.k.a ZynqMP)

  Board: ZynqMP Zcu102

  ## Remote Processor firmware (image_echo_test)

  * Remote processor firmware for Xilinx ZynqMP cortex-r5 platform based on: [rpmsg-echo.c](https://github.com/OpenAMP/open-amp/blob/main/apps/examples/echo/rpmsg-echo.c)

  * Instructions to compile: [ZynqMP r5f generic baremetal](https://github.com/OpenAMP/open-amp/blob/main/README.md#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote)

  * RPU firmware elf file is expected in sdk at path: /lib/firmware/

  * Xilinx Vendor specific SDK is required to build RPU firmware: [Xilinx Petalinux](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html)

  * More information is provided here: [Xilinx Wiki page for OpenAMP](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841718/OpenAMP)

  ## Run the demo

  Assume all the binaries are board specific.

  ```
  # Specify remote processor firmware to be loaded.
  echo image_echo_test > /sys/class/remoteproc/remoteproc0/firmware

  # Load and start target firmware onto remote processor
  echo start > /sys/class/remoteproc/remoteproc0/state

  # check remote processor state
  cat /sys/class/remoteproc/remoteproc0/state

  # Run echo_test application on host processor
  echo_test

  # Stop remote processor
  echo stop > /sys/class/remoteproc/remoteproc0/state
  ```
