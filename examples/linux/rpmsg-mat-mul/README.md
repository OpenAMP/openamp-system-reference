# Demo: matrix multiply

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

  ## Remote Processor firmware (image_matrix_multiply)

  * Remote processor firmware for Xilinx ZynqMP cortex-r5 platform based on: [matrix_multiply.c](https://github.com/OpenAMP/open-amp/blob/main/apps/examples/matrix_multiply/matrix_multiply.c)

  * Instructions to compile: [ZynqMP r5f generic baremetal](https://github.com/OpenAMP/open-amp/blob/main/README.md#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote)

  * RPU firmware elf file is expected in sdk at path: /lib/firmware/

  * Xilinx Vendor specific SDK is required to build RPU firmware: [Xilinx Petalinux](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html)
  * More information is provided here: [Xilinx Wiki page for OpenAMP](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841718/OpenAMP)

  ## Run the demo

  Assume all the binaries are board specific.

  ```
  # Specify remote processor firmware to be loaded.
  echo image_matrix_multiply > /sys/class/remoteproc/remoteproc0/firmware

  # Load and start target Firmware onto remote processor
  echo start > /sys/class/remoteproc/remoteproc0/state

  # check remote processor state
  cat /sys/class/remoteproc/remoteproc0/state

  # load rpmsg_char driver
  modprobe rpmsg_char

  # load rpmsg_ctrl driver
  modprobe rpmsg_ctrl

  # Run Matrix multiplication application on host processor
  mat_mul_demo

  # unload rpmsg_ctrl driver
  modprobe -r rpmsg_ctrl

  #unload rpmsg_char driver
  modprobe -r rpmsg_char

  # Stop remote processor
  echo stop > /sys/class/remoteproc/remoteproc0/state
  ```
