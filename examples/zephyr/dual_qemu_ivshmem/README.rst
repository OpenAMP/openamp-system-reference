OpenAMP RPMSG over IVSHMEM system reference sample
##################################################
This application sample implements an RPMSg echo communication between
two QEMU instances based on the ARM Cortex A53 CPU, to use the RPMsg protocol
from the openAMP a backend was built on top of the Zephyr Inter-VM Shared
Memory (IVSHMEM) driver is implemented. A simple shell command provides to the
user an interaction with the application allowing to send data to a number
of times and receiving that data echoed from the other QEMU instance.

Prerequisites
*************

* Tested with Zephyr version 3.4.0

* Tested with Zephyr SDK 0.16.1

* QEMU needs to available.

Note:
*****

* The Vendor-ID 0x1AF4 is being used from all other Zephyr in-tree samples, currently there
is no Vendor allocated with the selected number.

ivshmem-server needs to be available and running. The server is available in
Zephyr SDK or pre-built in some distributions. Otherwise, it is available in
QEMU source tree.

Optionally the ivshmem-client, if available, can help to troubleshoot the
IVSHMEM communication between QEMU instances.

Preparing IVSHMEM server before doing anything:
***********************************************

#. The ivshmem-server utillity for QEMU can be found into the zephyr sdk
   directory, in:
   ``/path/to/your/zephyr-sdk/zephyr-<version>/sysroots/x86_64-pokysdk-linux/usr/xilinx/bin/``

#. You may also find ivshmem-client utillity, it can be useful to debug if everything works
   as expected.

#. Run ivshmem-server. For the ivshmem-server, both number of vectors and
   shared memory size are decided at run-time (when the server is executed).
   For Zephyr, the number of vectors and shared memory size of ivshmem are
   decided at compile-time and run-time, respectively.For Arm64 we use
   vectors == 2 for the project configuration in this sample. Here is an example:

   .. code-block:: console

      # n = number of vectors
      $ sudo ivshmem-server -n 2
      $ *** Example code, do not use in production ***

#. Appropriately set ownership of ``/dev/shm/ivshmem`` and
   ``/tmp/ivshmem_socket`` for your deployment scenario. For instance:

   .. code-block:: console

      $ sudo chgrp $USER /dev/shm/ivshmem
      $ sudo chmod 060 /dev/shm/ivshmem
      $ sudo chgrp $USER /tmp/ivshmem_socket
      $ sudo chmod 060 /tmp/ivshmem_socket

Building and Running
********************
There are host and remote side projects, and they should be built individually, for the host side
open a terminal and then type:

   .. code-block:: console

      $ cd path/to/this-repo/examples/zephyr/dual_qemu_ivshmem/host
      $ west build -pauto -bqemu_cortex_a53

For the remote side, open another terminal window and then type:

   .. code-block:: console

      $ cd path/to/this-repo/examples/zephyr/dual_qemu_ivshmem/remote
      $ west build -pauto -bqemu_cortex_a53

* Note: Warnings that appear are from ivshmem-shell subsystem and can be ignored.

After getting the both applications built, open two terminals and run each
instance separately, please note the Host instance ``MUST`` be run ``FIRST`` and remote
instance ``AFTER``, this is needed in order to make both instances to know what is the
other peer IVSHMEM ID.

For example to run host instance:

   .. code-block:: console

      $ cd path/to/this-repo/examples/zephyr/dual_qemu_ivshmem/host
      $ west build -t run

For the remote instance, just go to the remote side directory in another terminal:

   .. code-block:: console

      $ cd path/to/this-repo/examples/zephyr/dual_qemu_ivshmem/remote
      $ west build -t run

Expected output:
****************
After running both host and remote QEMU instances in their own terminal tabs, and
in the ``RIGHT ORDER``, that is it, first the host instance followed by remote instance
go to the host instance terminal, you should see something like this:

   .. code-block:: console

      uart:~$ *** Booting Zephyr OS build v3.4.0-rc2-91-gbf0f58d69816 ***
      Hello qemu_cortex_a53 - Host Side, the communication over RPMsg is ready to use!

If nothing appears, make sure you are running the remote instance after this one, because
the host side after started to run, wait for the remote one to get running, and after
this it becomes ready to use.

Having the initial boot message, go to the remote instance, and check it initial message
on console you may see something like this:

   .. code-block:: console

      *** Booting Zephyr OS build v3.4.0-rc2-91-gbf0f58d69816 ***
      Hello qemu_cortex_a53 - Remote Side, the communication over RPMsg is ready to use!

Then go back to the host side terminal window, and issue the custom shell command
``rpmsg_ivshmem send`` and you should see how to use that:

   .. code-block:: console

      uart:~$ rpmsg_ivshmem send
      send: wrong parameter count
      send - Usage: rpmsg_ivshmem send <string> <number of messages>

Send a string to the remote side, specify also how many times it should be sent,
this command will send the data over RPMsg-IVSHMEM backend and the remote side
will reply back echoing the sent string, on the host terminal this should take
an output similar like the shown below:

   .. code-block:: console

      uart:~$ rpmsg_ivshmem send "RPMsg over IVSHMEM" 10
      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 1

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 2

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 3

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 4

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 5

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 6

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 7

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 8

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 9

      Remote side echoed the string back:
      [ RPMsg over IVSHMEM ]
      at message number 10

On the remote side terminal window is possible also to check the messages
arriving from host:

   .. code-block:: console

      *** Booting Zephyr OS build v3.4.0-rc2-91-gbf0f58d69816 ***
      Hello qemu_cortex_a53 - Remote Side, the communication over RPMsg is ready to use!


      uart:~$ Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

      Host side sent a string:
      [ RPMsg over IVSHMEM ]
      Now echoing it back!

This sample supports huge message number in order to do stress testing, something like
``rpmsg_ivshmem send "Test String" 10000000000000``, can be used for that, notice that
this command is blocking and have a 5 second timeout, returning if something goes wrong,
for example shutdown the remote side unexpectedly:

   .. code-block:: console

      uart:~$ rpmsg_ivshmem send "RPMsg over IVSHMEM" 10
      Remote side response timed out!
      uart:~$

Known limitation:
*****************
The limitation of this sample is in respect to the instances shutdown, if for some
reason host side or remote side get turned-off it ``MUST NOT`` be reinitialized individually,
in case of occurrence, both instances should be stopped and re-initialized following the
order constraints mentioned before (first run the host side followed by the remote side).