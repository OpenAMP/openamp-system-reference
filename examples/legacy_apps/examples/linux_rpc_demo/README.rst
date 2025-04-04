This readme is about the OpenAMP linux_rpc_demo.

The linux_rpc_demo is about remote procedure calls between linux host and a linux
remote using rpmsg to perform
1. File operations such as open, read, write and close
2. I/O operation such as printf, scanf

Compilation
***********
Add cmake option `-DWITH_PROXY_APPS=ON` to build the system reference demonstration applications.

See `README.md <../../README.md>`_ for details.

Run demonstration on a Linux PC
*******************************

* Start rpc demo server on one console

.. code-block:: shell

    sudo LD_LIBRARY_PATH=<openamp_built>/usr/local/lib:<libmetal_built>/usr/local/lib \
    build-openamp/usr/local/bin/linux_rpc_demod-shared

* Run rpc demo client on another console to perform file and I/O operations on the server

.. code-block:: shell

    sudo LD_LIBRARY_PATH=<openamp_built>/usr/local/lib:<libmetal_built>/usr/local/lib \
    build-openamp/usr/local/bin/linux_rpc_demo-shared 1

Enter the inputs on the host side the same gets printed on the remote side. You will see
communication between the host and remote process using rpmsg calls.

Note:
*****
`sudo` is required to run the OpenAMP demos between Linux processes, as it doesn't work on
some systems if you are normal users.
