Overview
********

This readme is about the OpenAMP matrix_multiply demo.
The matrix_multiply is about one processor generates two matrices, and sends them to the one,
The other processor calculates the matrix multiplication and returns the result matrix.

Compilation
***********

See `README.md <../../README.md>`_ for details.

Run demonstration on a Linux PC
*******************************

It is possible to run the application on a Linux PC to communicate between two Linux processes.

.. code-block:: shell

    cd $PROJECT_ROOT/target
    LD_LIBRARY_PATH=./lib ./bin/matrix_multiplyd-static &
    sleep 1
    LD_LIBRARY_PATH=./lib ./bin/matrix_multiply-static 1
