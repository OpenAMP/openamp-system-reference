# Demo: proxy_app

  This app demonstrates two functionality
  1) Use of host processor's file system by remote processor
  2) remote processor's standard IO redirection to host processor's standard IO


  case 1: This app allows remote processor to use file system of host processor. Host
  processor file system acts as proxy of remote file system. Remote processor
  can use open, read, write, close calls to interact with files on host
  processor.

  File "remote.file" is available after app exits on host side that is created by
  remote processor that contains string "This is a test string being written to
  file.." written by remote firmware. This demonstrates remote firmware can
  create and write files on host side.

  case 2: This application also demonstrates redirection of standard IO.
  Remote processor can use host processor's stdin and stdout via proxy service
  that is implemented on host side. This is achieved with open-amp proxy
  service implemented here: [rpmsg_retarget.c](https://github.com/OpenAMP/open-amp/blob/main/lib/proxy/rpmsg_retarget.c)
  Remote side firmware uses two types of output functions to print message on
  console 1) xil_printf i.e. using same UART console as of APU and 2) Standard
  "printf" function that is re-directed to standard output of Host. Both function
  uses different ways to output messages, but using same console.

  This is interactive demo:
  1. When the remote application prompts you to Enter name, enter any string without space.
  2. When the remote application prompts you to Enter age , enter any integer.
  3. When the remote application prompts you to Enter value for pi, enter any floating
     point number.
  After this, remote application will print all the inputs entered by user on console
  of host processor.

  Remote firmware's standard IO are redirected to host processor's standard IO.
  So, when remote uses "printf" and "scanf" functions actually host processor's
  console is used for printing output and scanning inputs. Host communicates with
  remote via rpmsg_char driver and Remote communicates to Host via redirected
  Standard IO.

  Platform: Xilinx Zynq UltraScale+ MPSoC(a.k.a ZynqMP) 

  Board: ZynqMP Zcu102

  ## Remote Processor firmware (image_rpc_demo)

  * Remote processor firmware for Xilinx ZynqMP cortex-r5 platform based on: [rpc_demo.c](https://github.com/OpenAMP/open-amp/blob/main/apps/examples/rpc_demo/rpc_demo.c)

  * Instructions to compile: [ZynqMP r5f generic baremetal](https://github.com/OpenAMP/open-amp/blob/main/README.md#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote)

  * RPU firmware elf file is expected in sdk at path: /lib/firmware/

  * Xilinx Vendor specific SDK is required to build RPU firmware: [Xilinx Petalinux](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html)

  * More information is provided here: [Xilinx Wiki page for OpenAMP](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841718/OpenAMP)

  ## Run the demo

  Assume all the binaries are zcu102 board specific.

  ```
  # Specify remote processor firmware to be loaded.
  echo image_rpc_demo > /sys/class/remoteproc/remoteproc0/firmware

  # Load and start target Firmware onto remote processor.
  echo start > /sys/class/remoteproc/remoteproc0/state

  # Run proxy application.
  proxy_app

  # Stop target firmware
  echo stop > /sys/class/remoteproc/remoteproc0/state
  ```
