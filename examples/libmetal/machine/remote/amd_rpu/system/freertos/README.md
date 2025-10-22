# AMD RPU FreeRTOS System Port

This directory contains the OS glue that lets the remote IRQ shared-memory demo
run as a FreeRTOS task on the AMD RPU.

- `main.c` creates the worker task and drives the FreeRTOS scheduler.
- `gic_init.c` installs the IPI ISR using the FreeRTOS interrupt helpers.
- `amp_demo_os.h` records the demo task handle so `system_suspend()` can call
  `vTaskSuspend(NULL)` while `system_resume()` wakes it with
  `xTaskResumeFromISR`, allowing the Idle task to run while the demo waits for
  a kick.

The top-level machine README covers configuration flags and build steps. When
`PROJECT_SYSTEM=freertos`, CMake automatically pulls in the sources from this
directory.
