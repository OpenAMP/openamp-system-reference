# AMD Linux Userspace Host Platform

## Overview
This document captures the platform-specific details needed to run the IRQ
shared-memory demo on a Linux host processor. The host application cooperates
with the remote firmware at `demos/irq_shmem_demo/remote/irq_shmem_demo.c`,
using a shared-memory window and IPI notifications to exchange timestamped
messages.

> Historical documentation may refer to the host processor as the “APU” and the
> remote processor as the “RPU”. Within this guide we consistently use
> Host/Remote terminology.

## Host Demo Behaviour
- Maps shared payload memory, both descriptor regions, the TTC timer, and IPI devices.
- Sends 1,024 timestamped messages to the remote, notifying it via IPI.
- Reads and verifies the echoed messages against the original payloads.
- Reports the average timing value and sends a `shutdown` message to the remote.
- Disables interrupts and releases the mapped devices.

## Prerequisites
- Linux kernel exposes the shared memory carveouts and descriptor UIOs to
  userspace with stable logical names:
  `libmetal-data`, `libmetal-desc0`, `libmetal-desc1`, `libmetal-ipi`, and
  `libmetal-timer`.
- The host IPI UIO node carries a `libmetal,uio-ipi-bitmask` device-tree
  property so the demo can discover the platform-specific interrupt bit at
  runtime.
- libmetal (and dependent libraries) installed on the host system, as well as
  the `metal_xlnx_extension` library when required by the platform glue.
- Remote firmware is already loaded and waiting for interrupts before the host
  demo starts.

## Host Device-Tree Example
The Linux host path opens UIO devices by their `linux,uio-name` values, then
reads the remote kick bit from the backing IPI node's
`libmetal,uio-ipi-bitmask` property. On the referenced
`versal-2ve-2vm-vek385-revb-multidomain/cortexa78-linux.dts`, the host-facing
pieces look like this:

```dts
reserved-memory {
        libmetal_desc0: libmetal_desc0@99c8000 {
                reg = <0x0 0x99c8000 0x0 0x4000>;
                no-map;
                label = "libmetal_desc0";
        };

        libmetal_desc1: libmetal_desc1@99cc000 {
                reg = <0x0 0x99cc000 0x0 0x4000>;
                no-map;
                label = "libmetal_desc1";
        };

        libmetal_data: libmetal_data@99d0000 {
                reg = <0x0 0x99d0000 0x0 0x40000>;
                no-map;
                label = "libmetal_data";
        };
};

axi {
        libmetal_uio_desc0@99c8000 {
                reg = <0x0 0x99c8000 0x0 0x4000>;
                compatible = "uio";
                linux,uio-name = "libmetal-desc0";
        };

        libmetal_uio_desc1@99cc000 {
                reg = <0x0 0x99cc000 0x0 0x4000>;
                compatible = "uio";
                linux,uio-name = "libmetal-desc1";
        };

        libmetal_uio_data@99d0000 {
                reg = <0x0 0x99d0000 0x0 0x40000>;
                compatible = "uio";
                linux,uio-name = "libmetal-data";
        };

        timer@f1e90000 {
                compatible = "uio";
                linux,uio-name = "libmetal-timer";
        };

        mailbox@eb360000 {
                reg = <0x0 0xeb360000 0x0 0x10000
                       0x0 0xeb3f0a00 0x0 0x200>;
                interrupts = <0x0 0x3c 0x4>;
                compatible = "uio";
                linux,uio-name = "libmetal-ipi";
                libmetal,uio-ipi-bitmask = <0x10>;
        };
};
```

For this demo, the important part is the host sees five stable UIO names in
`/sys/class/uio/uio*/name`. The shared-memory payload and descriptor region
sizes come from the mapped UIO regions at runtime. Both sides must agree on
physical addresses, descriptor sizes, and the payload split: the host uses the
first half for transmission and the remote uses the second half. The remote's
generated configuration must match these mappings; runtime discovery on the
host does not negotiate a layout with the remote. The UIO names and host IPI
bitmask must also match the hardware design.

The host validates capacity before clearing or writing shared memory. The
1,024-message flood needs at least 4,108 bytes in desc0 (including shutdown),
4,104 bytes in desc1, and 32,800 bytes of payload memory split evenly between
directions. Descriptor sizes must be multiples of four bytes. Each mapping
size must fit in 32 bits. The demo retains all transmitted payloads for later
verification, so undersized mappings are rejected rather than reused as a ring.

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
2. Confirm the expected logical UIO names are visible:
   ```bash
   cat /sys/class/uio/uio*/name
   ```
3. Launch the host binary (root/sudo may be required for IPI device access):
   ```bash
   ./irq_shmem_demo-static
   ```
4. Observe the console output for packet progress and the final average
   round-trip latency.

## [Shared Memory Layout](../../../demos/irq_shmem_demo/README.md#shared-memory-layout)
Shared buffer map used by both sides of the demo.

## Troubleshooting
- **Hangs waiting for notification**: ensure the host IPI UIO node exposes
  `libmetal,uio-ipi-bitmask`, that the host process can write to the IPI
  device,
  and that the remote firmware uses the matching interrupt bit.
- **Shared-memory access errors**: confirm the UIO entries expose the expected
  descriptor and payload regions with read/write permissions for the demo user.
- **Mismatched payloads**: verify both sides agree on descriptor offsets and
  the `PKGS_TOTAL` value compiled into each binary.
