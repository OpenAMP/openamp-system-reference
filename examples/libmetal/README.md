# Libmetal Sample Applications

## Overview
The `examples/libmetal` tree hosts the libmetal reference demos that exercise
shared memory, inter-processor interrupts, and atomics. The application logic is
lifted from the upstream libmetal repository so the OpenAMP System Reference
contains a one-stop showcase for both Linux userspace and bare-metal/RTOS
environments. Every demo is split into a *host* role (typically Linux) and a
*remote* role (an auxiliary core or MCU).

## Directory Layout
- `demos/<demo>/<role>` – vendor-neutral demo sources. They only depend on the
  thin platform APIs declared in `machine/<role>/<machine>/common.h`, so they
  can be reused on any SoC once that layer is implemented.
- `machine/host/<machine>` – platform glue for the host role. It maps the shared
  memory window, programs the interrupt controller, and provides helper
  utilities such as timers or logging hooks.
- `machine/remote/<machine>` – platform glue for the remote role. It sets up the
  local run-time (bare metal or RTOS), exports the same helpers as the host
  layer, and links in board support libraries.
- `toolchain_file` – example CMake toolchain snippet that injects platform
  macros (e.g. `-DPLATFORM_ZYNQMP`) so the machine layer can pick the correct
  peripheral map.

## AMD Reference Port
The repository currently ships one working port:

- `machine/host/amd_linux_userspace` targets AMD Zynq UltraScale+, Versal, and
  Versal Net devices. It assumes Linux exposes the shared memory, IPI, and TTC
  peripherals through userspace drivers (UIO or misc devices).
- `machine/remote/amd_rpu` targets the matching Cortex-R5 RPU running on
  bare-metal or FreeRTOS. It links against the AMD BSP libraries listed in
  `build_deps.cmake` and uses the same helpers that the upstream libmetal demo
  expects.

Although these directories carry AMD-specific defaults (device names,
interrupt masks, linker script, etc.), the core demos under `demos/` remain
unchanged from the upstream project.

## Porting to Other Vendors
To reuse the demos on a different SoC or operating system:

1. **Create a host machine module.** Copy
   `machine/host/amd_linux_userspace` as a starting point, rename the directory
   (e.g. `machine/host/acme_linux_userspace`), and update:
   - `common.h` with your device identifiers (shared-memory device name, IPI
     path, timer base addresses, interrupt masks, etc.).
   - `sys_init.c` to open and map those peripherals using your OS/BSP APIs.
   - The local `CMakeLists.txt` to pull in any extra sources or libraries that
     your port requires.
2. **Create a remote machine module.** Mirror the process under
   `machine/remote/<new_machine>`, providing:
   - `common.h` and `sys_init.*` that describe the remote-side peripherals.
   - A `system/<rtos>` subdirectory if you need RTOS glue (see the FreeRTOS
     example for structure).
   - A linker script or scatter file that matches your memory map.
3. **Select your machine at configure time.** Point CMake to the new directories
   with:
   ```bash
   cmake -DDEMO=irq_shmem_demo \
         -DROLE=host \
         -DPROJECT_MACHINE=acme_linux_userspace \
         …
   ```
   and similarly for the remote role. If you have multiple RTOS options, also
   set `-DPROJECT_SYSTEM=<system_subdir>`.
4. **Adjust platform macros and dependencies.** Update your toolchain file (or
   pass `-DPLATFORM_*` definitions) so the machine layer selects the correct
   peripheral map. Add any BSP or HAL libraries via `CMAKE_LIBRARY_PATH`,
   `USER_LINK_DIRECTORIES`, or the machine-specific `CMakeLists.txt`.

Once those layers are in place, the existing demo sources build and run without
code changes, providing a consistent validation of libmetal primitives on your
hardware.
