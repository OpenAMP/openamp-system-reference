# AMD RPU Remote Platform

## Overview
This guide captures the platform-specific setup for running the IRQ shared
memory demo on an AMD RPU executing FreeRTOS or bare-metal firmware. The remote
binary at `demos/irq_shmem_demo/remote/irq_shmem_demod.c` services notifications
from the host, echoes payloads through shared memory, and exits when the host
sends a `"shutdown"` marker.

> Historical documentation may refer to the host processor as the “APU” and the
> remote processor as the “RPU”. Within this guide we consistently use
> Host/Remote terminology.

## Prerequisites
- Cortex-R5 cross toolchain and BSP that provide the required headers and
  libraries (`xilstandalone`, `xiltimer`, `xilpm`, etc.).
- libmetal and the Xilinx libmetal extension libraries available to the
  toolchain via the include/library search paths.
- The linker script and BSP expose the shared memory window at `0x09860000`
  (size `0x48000`) with three carveouts: desc0 at `0x09860000` (4 KiB), desc1 at
  `0x09864000` (4 KiB), and payload at `0x09868000` (256 KiB split evenly for
  RX/TX), together with the IPI and TTC peripherals described in `common.h`.
- The CMake toolchain file used for configuration defines the correct platform
  macro (for example, `-DPLATFORM_ZYNQMP`, `-Dversal`, or `-DVERSAL_NET`) so the
  peripheral map matches the target silicon.
- Start the host side after the remote firmware is loaded and waiting for
  notifications.

## Configure & Build
From `examples/libmetal`, configure CMake with your cross toolchain, BSP, and
library paths:

```bash
cmake -S . -B build_remote \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/rpu-toolchain.cmake \
  -DCMAKE_INCLUDE_PATH="${LIBMETAL_BUILD_DIR}/include;${BSP_DIR}/include" \
  -DCMAKE_LIBRARY_PATH="${LIBMETAL_BUILD_DIR}/lib;${BSP_DIR}/lib;${EXTENSION_LIB}" \
  -DDEMO=irq_shmem_demo \
  -DROLE=remote \
  -DPROJECT_SYSTEM=freertos \
  -DPROJECT_MACHINE=amd_rpu

cmake --build build_remote --target irq_shmem_demo.elf
```

Build artefacts:
- Executable ELF: `build_remote/machine/remote/amd_rpu/irq_shmem_demo.elf`
- Linker map: `build_remote/machine/remote/amd_rpu/irq_shmem_demo.map`

## Run
1. Load `irq_shmem_demo.elf` onto the RPU using your preferred loader (XSDB,
   OpenOCD, PLM, etc.) and start the core.
2. Monitor the remote console for the `"libmetal demo"` banner and echo-test
   progress. The firmware prints `"Received shutdown message"` when it exits.

## [Shared Memory Layout](../../../demos/irq_shmem_demo/README.md#shared-memory-layout)
Buffer layout shared between the host and remote binaries.

## Troubleshooting
- **No interrupts observed**: verify the platform macro in the toolchain file
  matches the target device so the correct IPI/TTC base addresses are compiled
  into the firmware.
- **Shared memory offset errors**: ensure the linker script and MPU/cache
  settings expose the `0x09860000` window coherently to both sides.
- **Linker failures**: confirm the cross toolchain can locate libmetal,
  `metal_xlnx_extension`, and BSP libraries through `CMAKE_INCLUDE_PATH` and
  `CMAKE_LIBRARY_PATH`.
