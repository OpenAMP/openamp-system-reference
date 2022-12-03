# app: echo_test

## Description:

  This demo uses kernel rpmsg framework to send various size of data buffer to remote
  processor and validates integrity of received buffer from remote processor.
  If buffer data does not match, then number of different bytes are reported on
  console

## Remote processor firmware: Xilinx ZynqMP cortex-r5 platform

  https://github.com/OpenAMP/open-amp/blob/main/apps/examples/echo/rpmsg-echo.c

## How to Build for Xilinx ZynqMP platform
  * https://github.com/OpenAMP/open-amp#example-to-compile-zynq-ultrascale-mpsoc-r5-genericbaremetal-remote
  * RPU firmware elf file is expected in sdk at path: /lib/firmware/
  * This build step needs Xilinx Vendor specific toolchain xsdb

## How to run on zcu102 board/QEMU:

Assume all the binaries are zcu102 board specific.

   ##### Specify Echo Test Firmware to be loaded.
   `echo image_echo_test > /sys/class/remoteproc/remoteproc0/firmware`

   ##### Load and start target Firmware onto remote processor
   `echo start > /sys/class/remoteproc/remoteproc0/state`

   ##### check remote processor state
   `cat /sys/class/remoteproc/remoteproc0/state`

   ##### Run echo_test application
   `echo_test`

   ##### stop target firmware
   `echo stop > /sys/class/remoteproc/remoteproc0/state`
