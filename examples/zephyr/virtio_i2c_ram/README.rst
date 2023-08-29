.. _openAMP_sample:

OpenAMP Sample Application
##########################

Overview
********

This application demonstrates how to use of virtio mmio to share an I2C bus between
Zephyr and a remote processor. It is designed to implement an memory device
connected on an I2C virtio bus.

Currently this implementation don't use the OpenAMP virtio as not ready.

requested Hardware
*************************

- compatible with:
  - STM32MP115c-dk2 board

Zephyr
======

.. code-block:: console

   west build -b <target board> openamp-system-reference/examples/zephyr/virtio_i2c_ram

Copy the binary file on the SDCard

Linux console
=============

Open a serial Linux terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: odd
- Stop bits: 1

Load and start the firmware:

.. code-block:: console

  echo -n <firmware_name.elf> > /sys/class/remoteproc/remoteproc0/firmware
  echo start >/sys/class/remoteproc/remoteproc0/state


This is the Linux console:

1. Verify that the virtio I2C bus is preent

.. code-block:: console

  root@stm32mp1-disco-oss:~# i2cdetect -l
  i2c-0   i2c             STM32F7 I2C(0x40012000)                 I2C adapter
  i2c-1   i2c             STM32F7 I2C(0x5c002000)                 I2C adapter
  i2c-2   i2c             i2c-0-mux (chan_id 0)                   I2C adapter
  i2c-3   i2c             i2c_virtio at virtio bus 0              I2C adapter

2. list devices on the virtio bus

Two memory devices should be connected:
- one 20 bytes memory at address 0x54 initialized with "123456789abcdefghij"
- one 20 bytes memory at address 0x56 initialized with "klmnopqrstuvwxyz!:;"
Verify that the virtio I2C bus is present

.. code-block:: console

  root@stm32mp1-disco-oss:~# i2cdetect -y 3
       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
  00:                         -- -- -- -- -- -- -- --
  10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  50: -- -- -- -- 54 -- 56 -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  70: -- -- -- -- -- -- -- --

3. Update RAM device 0x54 at address 0x14 with value 4

.. code-block:: console

  root@stm32mp1-disco-oss:~# i2cget -y 3 0x54 14
  0x66
  root@stm32mp1-disco-oss:~# i2cset -y 3 0x54 14 0x25
  root@stm32mp1-disco-oss:~# i2cget -y 3 0x54 14
  0x25
