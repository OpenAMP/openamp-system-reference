# app: proxy_app

## Description:

  This app demonstrates two functionality
  1) Use of host processor's file system by remote processor
  2) remote processor's standard IO redirection to host processor's standard IO

  1) This app allows remote processor to use file system of host processor. Host
  processor file system acts as proxy of remote file system. Remote processor
  can use open, read, write, close calls to interact with files on host
  processor.

  File "remote.file" is available after app exits on host side that is created by
  remote processor that contains string "This is a test string being written to
  file.." written by remote firmware. This demonstrates remote firmware can
  create and write files on host side.

  2) This application also demonstrates redirection of standard IO.
  Remote processor can use host processor's stdin and stdout via proxy service
  that is implemented on host side. This is achieved with open-amp proxy
  service implemented here:
  https://github.com/OpenAMP/open-amp/blob/main/lib/proxy/rpmsg_retarget.c
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

## Remote processor firmware: Xilinx ZynqMP cortex-r5 platform

  https://github.com/OpenAMP/open-amp/blob/main/apps/examples/rpc_demo/rpc_demo.c
