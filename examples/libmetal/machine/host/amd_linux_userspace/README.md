# AMD Linux Userspace Host Platform

## Overview
This document captures the platform-specific details needed to run the IRQ
shared-memory demo on a Linux host processor. The host application cooperates
with the remote firmware at `demos/irq_shmem_demo/remote/irq_shmem_demod.c`,
using a shared-memory window and IPI notifications to exchange timestamped
messages.

> Historical documentation may refer to the host processor as the “APU” and the
> remote processor as the “RPU”. Within this guide we consistently use
> Host/Remote terminology.

## Host Demo Behaviour
- Maps the shared memory, TTC timer, and IPI UIO devices required to exchange messages with the remote firmware.
- Registers the IPI interrupt handler, posts a “demo started” marker in shared memory, and enables IPI delivery.
- For each latency sample (default 1000 iterations), resets the host-to-remote timer, triggers an IPI, waits for the remote response, and records both directions of travel time.
- Aggregates the collected counter values into average latency metrics and writes them back into the shared buffer for the remote to read.
- Signals completion via IPI and then disables the interrupt and releases the mapped devices.

## Prerequisites
- Linux kernel exposes the shared memory carveouts and descriptor UIOs
  (`9868000.shm`, `9860000.shm_desc`, `9864000.shm_desc` by default; adjust to
  the platform-specific devices/offsets), along with IPI and TTC peripherals,
  to userspace with permissions suitable for the demo binary.
- libmetal (and dependent libraries) installed on the host system, as well as
  the `metal_xlnx_extension` library when required by the platform glue.
- The project toolchain file defines the correct platform macro (for example,
  `-DPLATFORM_ZYNQMP`) so `common.h` selects the matching peripheral map.
- Remote firmware is already loaded and waiting for interrupts before the host
  demo starts.

## Configure & Build
From `examples/libmetal`, configure CMake with the desired output directory and
library/include search paths:

```bash
cmake -S . -B build_host \
  -DCMAKE_TOOLCHAIN_FILE=$(pwd)/toolchain_file \
  -DCMAKE_INCLUDE_PATH="/path/to/libmetal/include" \
  -DCMAKE_LIBRARY_PATH="/path/to/libmetal/lib" \
  -DDEMO=irq_shmem_demo \
  -DROLE=host \
  -DPROJECT_MACHINE=amd_linux_userspace

cmake --build build_host --target irq_shmem_demo-static
```

The static executable is emitted at
`build_host/machine/host/amd_linux_userspace/irq_shmem_demo-static`.

## Run
1. Start the remote firmware so it sits in the notification loop.
2. Launch the host binary (root/sudo may be required for IPI device access):
   ```bash
   ./irq_shmem_demo-static
   ```
3. Observe the console output for packet progress and the final average
   round-trip latency.

## [Shared Memory Layout](../../../demos/irq_shmem_demo/README.md#shared-memory-layout)
Shared buffer map used by both sides of the demo.

## Troubleshooting
- **Hangs waiting for notification**: ensure the IPI mask configured in
  `common.h` (or overridden via the optional demo config file) matches the
  remote firmware and that the host process can write to the IPI device.
- **Shared-memory access errors**: confirm the UIO entries expose the expected
  descriptor/payload ranges (`0x09860000` base) with read/write permissions for
  the demo user.
- **Mismatched payloads**: verify both sides agree on descriptor offsets and
  the `PKGS_TOTAL` value compiled into each binary.
