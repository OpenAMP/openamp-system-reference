.. _openAMP_rsc_table_sample:

OpenAMP multi services sample Application using resource table
##############################################################

Overview
********

This application demonstrates how to use OpenAMP with Zephyr based on a resource
table. It is designed to respond to the:

* `Linux rpmsg client sample <https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c>`_
* `Linux rpmsg tty driver <https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c>`_
* `Linux rpmsg char driver <https://elixir.bootlin.com/linux/latest/source/drivers/rpmsg/rpmsg_char.c>`_



This sample implementation is compatible with platforms that embed
a Linux kernel OS on the main processor and a Zephyr application on
the co-processor.


Tested on board:

* `Lstm32mp157C_dk2 <https://docs.zephyrproject.org/latest/boards/arm/stm32mp157c_dk2/doc/stm32mp157_dk2.html>`_
* `Lstm32mp157F_dk2 <https://docs.zephyrproject.org/latest/boards/arm/stm32mp157c_dk2/doc/stm32mp157_dk2.html>`_

Building the application
*************************

Zephyr
======

.. code-block:: console

   west build -b <target board> openamp-system-reference/examples/zephyr/rpmsg_multi_services

Linux
=====

Enable:

- the SAMPLE_RPMSG_CLIENT configuration to build and install
  the rpmsg_client_sample.ko module on the target,
- the RPMSG_TTY configuration to build and install the
  rpmsg_tty.ko module on the target
- the RPMSG_CHAR configuration to build and install the
  rpmsg_char.ko module on the target
- build and install the
  `rpmsg-utils <https://github.com/OpenAMP/openamp-system-reference/tree/main/examples/linux/rpmsg-utils>`_
  binaries

Running the sample
*******************

Zephyr console
==============

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board.

Linux console
=============

Open a Linux shell (minicom, ssh, etc.)

* Insert a module into the Linux Kernel:

.. code-block:: console

   root@linuxshell: insmod rpmsg_client_sample.ko rpmsg_tty.ko rpmsg_char.ko rpmsg_ctrl.ko

* Start the demo environment

First copy the rpmsg_multi_services.elf file on the target rrottfs in /lib/firmware folder.
Then start the firmware:

.. code-block:: console

  root@linuxshell: echo rpmsg_multi_services.elf > /sys/class/remoteproc/remoteproc0/firmware
  root@linuxshell: echo start >/sys/class/remoteproc/remoteproc0/state

Result on Zephyr console on boot
================================

The following messages will appear on the corresponding Zephyr console

.. code-block:: console

  [   54.495343] virtio_rpmsg_bus virtio0: rpmsg host is online
  [   54.500044] virtio_rpmsg_bus virtio0: creating channel rpmsg-client-sample addr 0x400
  [   54.507923] virtio_rpmsg_bus virtio0: creating channel rpmsg-tty addr 0x401
  [   54.514795] virtio_rpmsg_bus virtio0: creating channel rpmsg-raw addr 0x402
  [   54.548954] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: new channel: 0x402 -> 0x400!
  [   54.557337] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 1 (src: 0x400)
  [   54.565532] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 2 (src: 0x400)
  [   54.581090] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 3 (src: 0x400)
  [   54.588699] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 4 (src: 0x400)
  [   54.599424] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 5 (src: 0x400)
  ...

This inform that following rpmsg channels devices have been created:

* a rpmsg-client-sample device

  .. code-block:: console

    root@linuxshell: dmesg
    ...
    [   54.500044] virtio_rpmsg_bus virtio0: creating channel rpmsg-client-sample addr 0x400
    ...

* a rpmsg-tty device

  .. code-block:: console

    root@linuxshell: ls /dev/ttyRPMSG*
    /dev/ttyRPMSG0


* a rpmsg-raw device

  .. code-block:: console

    root@linuxshell: ls /dev/rpmsg?
    /dev/rpmsg0

The following messages will appear on the corresponding Zephyr console or
in the remoteproc trace buffer depending on the Hardware.

.. code-block:: console

  root@linuxshell:  cat /sys/kernel/debug/remoteproc/remoteproc0/trace0
  *** Booting Zephyr OS build zephyr-v3.2.0-1-g6b49008b6b83  ***
  Starting application threads!

  OpenAMP[remote]  linux responder demo started

  OpenAMP[remote] Linux sample client responder started

  OpenAMP[remote] Linux tty responder started

  OpenAMP[remote] Linux raw data responder started

  OpenAMP[remote] create a endpoint with address and dest_address set to 0x1
  OpenAMP Linux sample client responder ended


Demo 1: rpmsg-client-sample device
==================================

Principle
-----------

  This demo is automatically run when the co-processor firmware is started. It confirms that the rpmsg
  and virtio protocols are working properly. The Zephyr requests the creation of the
  rpmsg-client-sample channel to the Linux rpmsg framework using the "name service announcement"
  rpmsg. On message reception the Linux rpmsg bus creates an associated device and probes the
  rpmsg-client-sample driver. The Linux rpmsg-client-sample driver sent 100 messages to the remote
  processor, which answers to each message. After answering to each rpmsgs the Zephyr destroys the
  channel.

Associated traces
-----------------

  .. code-block:: console

    [   54.548954] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: new channel: 0x402 -> 0x400!
    [   54.557337] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 1 (src: 0x400)
    [   54.565532] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 2 (src: 0x400)

      ...

    [   55.436401] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 99 (src: 0x400)
    [   55.445343] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 100 (src: 0x400)
    [   55.454280] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: goodbye!
    [   55.461424] virtio_rpmsg_bus virtio0: destroying channel rpmsg-client-sample addr 0x400
    [   55.469707] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: rpmsg sample client driver is removed


Demo 2: rpmsg-tty device
========================

Principle
---------

  This channel allows to create a /dev/ttyRPMSGx for terminal based communication with Zephyr.

Demo
----

1. Check presence of the /dev/ttyRPMSG0

  By default the Zephyr has created a rpmsg-tty channel

  .. code-block:: console

    [   54.507923] virtio_rpmsg_bus virtio0: creating channel rpmsg-tty addr 0x401
    root@linuxshell: ls /dev/ttyRPMSG*
    /dev/ttyRPMSG0

2. Send and receive messages on /dev/ttyRPMSG0

  The zephyr is programmed to resent received messages with a prefixed "TTY 0: ", 0 is the instance of
  the tty link

  .. code-block:: console

    root@linuxshell: cat /dev/ttyRPMSG0 &
    root@linuxshell: echo "Hello Zephyr" >/dev/ttyRPMSG0
    TTY 0: Hello Zephyr
    root@linuxshell: echo "Goodbye Zephyr" >/dev/ttyRPMSG0
    TTY 0: Goodbye Zephyr

Demo 3: dynamic creation/release of a rpmsg-tty device
======================================================

Principle
---------

  This demo is based on the rpmsg_ctrl IOCtrls:

* RPMSG_CREATE_DEV_IOCTL : to create a local rpmsg device and to send a name service creation
  announcement to the remote processor
* RPMSG_RELEASE_DEV_IOCTL: release the local rpmsg device and to send a name service destroy
  announcement to the remote processor

Demo
----

1. Prerequisite

  * Due to a limitation in the rpmsg protocol, the zephyr does not know the existence of the
    /dev/ttyRPMG0 until the Linux sends it a first message. Creating a new channel before this first one
    is well establish leads to bad endpoints association. To avoid this, just send a message on
    /dev/ttyRPMSG0

    .. code-block:: console

      root@linuxshell: cat /dev/ttyRPMSG0 &
      root@linuxshell: echo "Hello Zephyr" >/dev/ttyRPMSG0
      TTY 0: Hello Zephyr

  * Check if the rpmsg-utils tools are installed on your platform.

    .. code-block:: console

      root@linuxshell: rpmsg_ping


  * If the rpmsg_ping application does not exist:

    * Download `rpmsg-utils <https://github.com/OpenAMP/openamp-system-reference/tree/main/examples/linux/rpmsg-utils>`_
      tools
    * Cross-compile it and install it on the target device.


  * optional: enable rpmsg bus trace to observe RPmsg in kernel trace:

    .. code-block:: console

      root@linuxshell: echo -n 'file virtio_rpmsg_bus.c +p' > /sys/kernel/debug/dynamic_debug/control

2. create a new TTY channel

  Create a rpmsg-tty channel from Linux with local address set to 257 and undefined remote address -1.

  .. note::

     Current Linux implementation has a limitation. When it initiates a name service announcement,
     It is not able to associate the remote endpoint to the created channel.
     Following patch has to be applied on top waiting a upstreamed solution:

     <https://lore.kernel.org/lkml/20220316153001.662422-1-arnaud.pouliquen@foss.st.com/>

  .. code-block:: console

    root@linuxshell: ./rpmsg_export_dev /dev/rpmsg_ctrl0 rpmsg-tty 257 -1

  The /dev/ttyRPMSG1 is created

  .. code-block:: console

    root@linuxshell: ls /dev/ttyRPMSG*
    /dev/ttyRPMSG0  /dev/ttyRPMSG1

  A name service announcement has been sent to Zephyr, which has created a local endpoint (@ 0x400),
  and sent a "bound" message to the /dev/ttyRPMG1 (@ 257)

  .. code-block:: console

    root@linuxshell: dmesg
    [  115.757439] rpmsg_tty virtio0.rpmsg-tty.257.-1: TX From 0x101, To 0x35, Len 40, Flags 0, Reserved 0
    [  115.757497] rpmsg_virtio TX: 01 01 00 00 35 00 00 00 00 00 00 00 28 00 00 00  ....5.......(...
    [  115.757514] rpmsg_virtio TX: 72 70 6d 73 67 2d 74 74 79 00 00 00 00 00 00 00  rpmsg-tty.......
    [  115.757528] rpmsg_virtio TX: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    [  115.757540] rpmsg_virtio TX: 01 01 00 00 00 00 00 00                          ........
    [  115.757568] remoteproc remoteproc0: kicking vq index: 1
    [  115.757590] stm32-ipcc 4c001000.mailbox: stm32_ipcc_send_data: chan:1
    [  115.757850] stm32-ipcc 4c001000.mailbox: stm32_ipcc_tx_irq: chan:1 tx
    [  115.757906] stm32-ipcc 4c001000.mailbox: stm32_ipcc_rx_irq: chan:0 rx
    [  115.757969] remoteproc remoteproc0: vq index 0 is interrupted
    [  115.757994] virtio_rpmsg_bus virtio0: From: 0x400, To: 0x101, Len: 6, Flags: 0, Reserved: 0
    [  115.758022] rpmsg_virtio RX: 00 04 00 00 01 01 00 00 00 00 00 00 06 00 00 00  ................
    [  115.758035] rpmsg_virtio RX: 62 6f 75 6e 64 00                                bound.
    [  115.758077] virtio_rpmsg_bus virtio0: Received 1 messages

3. Play with /dev/ttyRPMSG0 and /dev/ttyRPMSG1

  .. code-block:: console

    root@linuxshell: cat /dev/ttyRPMSG0 &
    root@linuxshell: cat /dev/ttyRPMSG1 &
    root@linuxshell: echo hello dev0 >/dev/ttyRPMSG0
    TTY 0: hello dev0
    root@linuxshell: echo hello dev1 >/dev/ttyRPMSG1
    TTY 1: hello dev1

4. Destroy RPMSG TTY devices

  Destroy the /dev/ttyRPMSG1

  .. code-block:: console

    root@linuxshell: ./rpmsg_export_dev /dev/rpmsg_ctrl0 -d rpmsg-tty 257 -1

  Destroy the /dev/ttyRPMSG0
  * Get the source address

  .. code-block:: console

    root@linuxshell: cat /sys/bus/rpmsg/devices/virtio0.rpmsg-tty.-1.*/src
    0x402

  * Destroy the /dev/ttyRPMSG0 specifying the address 1026 (0x402)

  .. code-block:: console

    root@linuxshell: ./rpmsg_export_dev /dev/rpmsg_ctrl0 -d rpmsg-tty 1026 -1

  The /dev/ttyRPMGx devices no more exists

Demo 4: rpmsg-char device
=========================

Principle
---------

  This channel allows to create a /dev/rpmsgX for character device based communication with Zephyr.

Demo
----

1. Prerequisite

  Download rpmsg-utils tools relying on the /dev/rpmsg_ctrl, an compile it in an arm environment
  using make instruction and install it on target

  optional: enable rpmsg bus trace to observe rp messages in kernel trace:

  .. code-block:: console

    echo -n 'file virtio_rpmsg_bus.c +p' > /sys/kernel/debug/dynamic_debug/control

2. Check presence of the /dev/rpmsg0

  By default the Zephyr has created a rpmsg-raw channel

  .. code-block:: console

    [   54.514795] virtio_rpmsg_bus virtio0: creating channel rpmsg-raw addr 0x402

3. Check device exists

  .. code-block:: console

    root@linuxshell: ls /dev/rpmsg?
    /dev/rpmsg0

4. Send and receive messages on /dev/rpmsg0

  The zephyr is programmed to resent received message with a prefixed "from ept 0x0402: ", 0x0402 is
  the zephyr endpoint address

  .. code-block:: console

    root@linuxshell: ./rpmsg_ping /dev/rpmsg0
    message for /dev/rpmsg0: "from ept 0x0402: ping /dev/rpmsg0"

Demo 5: Multi endpoints demo using rpmsg-ctrl device
====================================================

Principle
---------

  Use the rpmsg_ctrl RPMSG_CREATE_EPT_IOCTL IoCtrl to instantiate endpoints on Linux side. Theses
  endpoints will not be associated to a channel but will communicate with a predefined remote proc
  endpoint. For each endpoint created, a /dev/rpmsg sysfs interface will be created On Zephyr side, an
  endpoint with a prefixed address 0x1 has been created. When it receives a message it re-sends a the
  message to the Linux sender endpoint, prefixed by "from ept 0x0001:"

Demo
----

1. Prerequisite

  Download rpmsg-util tools relying on the /dev/rpmsg_ctrl, an compile it in an arm environment
  using make instruction and install it on target

  optional: enable rpmsg bus trace to observe rp messages in kernel trace:

  .. code-block:: console

    echo -n 'file virtio_rpmsg_bus.c +p' > /sys/kernel/debug/dynamic_debug/control

2. Check presence of the /dev/rpmsg0

  By default the Zephyr has created a rpmsg-raw channel

  .. code-block:: console

    [   54.514795] virtio_rpmsg_bus virtio0: creating channel rpmsg-raw addr 0x402

3. Check device exists

  .. code-block:: console

    root@linuxshell: ls /dev/rpmsg*
    /dev/rpmsg0       /dev/rpmsg_ctrl0

4. Create 3 new endpoints

  .. code-block:: console

    root@linuxshell: ./rpmsg_export_ept /dev/rpmsg_ctrl0 my_endpoint1 100 1
    root@linuxshell: ./rpmsg_export_ept /dev/rpmsg_ctrl0 my_endpoint2 101 1
    root@linuxshell: ./rpmsg_export_ept /dev/rpmsg_ctrl0 my_endpoint2 103 1
    root@linuxshell: ls /dev/rpmsg?
    /dev/rpmsg0  /dev/rpmsg1  /dev/rpmsg2  /dev/rpmsg3

5. Test them

  .. code-block:: console

    root@linuxshell: ./rpmsg_ping  /dev/rpmsg0
    message for /dev/rpmsg0: "from ept 0x0402: ping /dev/rpmsg0"
    root@linuxshell: ./rpmsg_ping  /dev/rpmsg1
    message for /dev/rpmsg1: "from ept 0x0001: ping /dev/rpmsg1"
    root@linuxshell: ./rpmsg_ping  /dev/rpmsg2
    message for /dev/rpmsg2: "from ept 0x0001: ping /dev/rpmsg2"
    root@linuxshell: ./rpmsg_ping  /dev/rpmsg3
    message for /dev/rpmsg3: "from ept 0x0001: ping /dev/rpmsg3"

6. Destroy them

  .. code-block:: console

    root@linuxshell: ./rpmsg_destroy_ept /dev/rpmsg1
    root@linuxshell: ./rpmsg_destroy_ept /dev/rpmsg2
    root@linuxshell: ./rpmsg_destroy_ept /dev/rpmsg3
    root@linuxshell: ls /dev/rpmsg?
    /dev/rpmsg0
