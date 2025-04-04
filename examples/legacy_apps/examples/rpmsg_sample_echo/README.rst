Overview
********

This readme is about the Linux sample echo demo.
A `rpmsg_client_sample` driver is available by default in the Linux kernel. This example offers the
pending implementation for the remote processor, but also an alternative to generate the echo sample
in the Linux user space or on baremetal.

Compilation
***********
Add cmake option `-DWITH_APPS=ON` to build the system reference demonstration applications.

Baremetal Compilation
=====================

.. code-block:: shell

    cmake  -DCMAKE_TOOLCHAIN_FILE=zynqmp_r5_generic -DWITH_APPS=ON


See `README.md <../../README.md>`_ for details.

Linux Compilation
=================

Linux kernel Compilation
------------------------

Enable the `CONFIG_SAMPLE_RPMSG_CLIENT` to build the `rpmsg_sample_client` module

Linux Userspace Compilation
---------------------------

See `README.md <../../README.md>`_ to generate the `rpmsg-sample-ping` application.

Run demonstration on a Linux PC
*******************************

It is possible to run the application on a Linux PC to communicate between two Linux processes.

.. code-block:: shell

    cd $PROJECT_ROOT/target
    LD_LIBRARY_PATH=./lib ./bin/rpmsg-sample-echo-static &
    sleep 1
    LD_LIBRARY_PATH=./lib ./bin/rpmsg-sample-ping-static 1
