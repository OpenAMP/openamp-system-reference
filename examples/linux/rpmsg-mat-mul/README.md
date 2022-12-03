# app: mat_mul_demo

## Description:

  This example demonstrate interprocessor communication using rpmsg framework
  in the Linux kernelspace. Host (this) application generates two random matrices and send
  them to remote processor using rpmsg framework in the Linux kernelspace and waits for
  the response. Remote processor firmware receives both matrices and
  multiplies them and sends result back to host processor.
  Host processor prints the result on console after receiveing it.
  If -n <number> option is passed, then above demo runs <number> times.
  User can also pass custom endpoint information with -s (source address)
  and -e (destination address) options as well.

## Remote processor firmware: Xilinx ZynqMP cortex-r5 platform

  https://github.com/OpenAMP/open-amp/blob/main/apps/examples/matrix_multiply/matrix_multiply.c
