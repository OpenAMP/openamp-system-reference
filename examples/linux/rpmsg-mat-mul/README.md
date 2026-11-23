# Demo: Matrix Multiply

  This example demonstrate interprocessor communication using rpmsg framework
  in the Linux kernelspace. Host (this) application generates two random matrices and send
  them to remote processor using rpmsg framework in the Linux kernelspace and waits for
  the response. Remote processor firmware receives both matrices and
  multiplies them and sends result back to host processor.
  Host processor prints the result on console after receiveing it.
  If -n <number> option is passed, then above demo runs <number> times.
  User can also pass custom endpoint information with -s (source address)
  and -e (destination address) options as well.

  Platform: Xilinx Zynq UltraScale+ MPSoC(a.k.a ZynqMP) 

  Board: ZynqMP Zcu102

  ## Remote Processor Firmware (image_matrix_multiply)

  * Remote processor firmware for Xilinx ZynqMP cortex-r5 platform based on: [matrix_multiply.c](https://github.com/OpenAMP/open-amp/blob/main/apps/examples/matrix_multiply/matrix_multiply.c)

  * Instructions to compile: [ZynqMP r5f generic baremetal](https://github.com/OpenAMP/open-amp/blob/main/README.md#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote)

  * RPU firmware elf file is expected in sdk at path: /lib/firmware/

  * Xilinx Vendor specific SDK is required to build RPU firmware: [Xilinx Petalinux](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html)
  * More information is provided here: [Xilinx Wiki page for OpenAMP](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841718/OpenAMP)

  ## Run the Demo

  Assume all the binaries are board specific.

  ```shell
  # Specify Remote Processor Firmware to Be Loaded.
  echo image_matrix_multiply > /sys/class/remoteproc/remoteproc0/firmware

  # Load and Start Target Firmware Onto Remote Processor
  echo start > /sys/class/remoteproc/remoteproc0/state

  # Check Remote Processor State
  cat /sys/class/remoteproc/remoteproc0/state

  # Load rpmsg_char Driver
  modprobe rpmsg_char

  # Load rpmsg_ctrl Driver
  modprobe rpmsg_ctrl

  # Run Matrix Multiplication Application on Host Processor
  rpmsg_matrix_multiply_demo

  # Unload rpmsg_ctrl Driver
  modprobe -r rpmsg_ctrl

  # Unload rpmsg_char Driver
  modprobe -r rpmsg_char

  # Stop Remote Processor
  echo stop > /sys/class/remoteproc/remoteproc0/state
  ```
