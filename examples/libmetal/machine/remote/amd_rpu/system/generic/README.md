AMD RPU Baremetal System Port

This directory holds the minimal OS layer that lets the remote IRQ
shared-memory demo run without an RTOS on the AMD RPU.

- `main.c` jumps directly into the demo entry point and parks in a `wfi` loop.
- `gic_init.c` initializes the XScuGic controller and registers the IPI ISR.
- `amp_demo_os.h` augments `channel_s` with an `atomic_flag` so
  `system_suspend()` can busy-wait on interrupts while `system_resume()`
  clears the flag inside the ISR.

Select this port by configuring CMake with `-DPROJECT_SYSTEM=baremetal`. The
rest of the build prerequisites match the FreeRTOS configuration described in
the machine README.
