This readme is about the OpenAMP rpc_demo demo.
The rpc_demo is about one processor uses the UART on the other processor and creates file on
the other processor's filesystem with file operations.

For now, It implements the remote processor running generic(baremetal) application accesses the
devices on the Linux.

Compilation
***********

Add cmake option `-DWITH_PROXY=ON -DWITH_APPS=ON` to build the system reference demonstration applications.

Baremetal Compilation
=====================

Here is an example to generate the to generate the `rpc_demo` application:

.. code-block:: shell

    cmake -DCMAKE_TOOLCHAIN_FILE=zynqmp_r5_generic -DWITH_APPS=ON -DWITH_PROXY=ON

See `README.md <../../README.md>`_ to generate the `rpc_demod` application.

Linux Compilation
=================

See `README.md <../../README.md>`_ to generate the `rpc_demod` application.

Run the Demo
************

The demo application will load the remoteproc module, then the proxy rpmsg module, will output
message sent from the other processor, send the console input back to the other processor.
When the demo application exits, it will unload the kernel modules.