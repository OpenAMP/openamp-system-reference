# Shared Memory Throughput Demo

The throughput demo streams larger payloads between the host and remote to
measure aggregate bandwidth in both directions. It is derived from the legacy
`zynqmp_amp_demo/shmem_throughput_demo` sample and updated to integrate with
the common libmetal platform scaffolding used by the IRQ shared-memory port.

Workflow overview:

1. Host and remote reset their TTC counters and clear the shared-memory
   descriptors.
2. Upload phase: one side repeatedly fills buffers and notifies the other via
   IPI. Each endpoint records TTC counter deltas for 1000 transfers at multiple
   payload sizes.
3. Download phase: roles reverse so both directions are characterised. Counter
   results are exchanged through the shared buffer to allow final reporting.
4. Once measurements complete the initiator writes a completion flag into
   shared memory and sends the shutdown notification.

Refer to the machine READMEs for the precise descriptor layout and device
requirements:

- Host: `machine/host/amd_linux_userspace/README.md`
- Remote: `machine/remote/amd_rpu/README.md`

Configure CMake with `-DDEMO=shmem_throughput_demo` and select the role via
`-DROLE=host` or `-DROLE=remote` to build each application.
