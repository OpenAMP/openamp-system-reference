Overview
********

This readme is about the OpenAMP echo demonstration using the RPmsg no copy API.
The demonstration is about one processor sending a message to the other one, and the other one
echoing back the message. The processor that sends the message will verify the echoed message.

Compilation
***********

See `README.md <../../README.md>`_ for details.

Run demonstration on a Linux PC
*******************************

It is possible to run the application on a Linux PC to communicate between two Linux processes.

.. code-block:: shell

    LD_LIBRARY_PATH=./lib ./bin/rpmsg-nocopy-echo-static &
    sleep 1
    LD_LIBRARY_PATH=./lib ./bin/rpmsg-nocopy-ping-static 1