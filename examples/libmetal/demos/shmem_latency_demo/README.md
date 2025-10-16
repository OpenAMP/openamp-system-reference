# Shared Memory Latency Demo

This demo pair measures round-trip latency between the host and remote by
exchanging timestamped payloads over shared memory while using IPI for
notifications. It mirrors the behaviour of the legacy
`zynqmp_amp_demo/shmem_latency_demo` sample from libmetal, updated to rely on
the common platform helpers introduced for the IRQ shared-memory port.

Both sides use the shared memory window and offsets documented under the
machine-specific READMEs:

- Host layout details: `machine/host/amd_linux_userspace/README.md`
- Remote layout details: `machine/remote/amd_rpu/README.md`

Key workflow:

1. Host posts a "demo started" marker in shared memory, resets TTC counters, and
   pushes timestamped payloads to the remote.
2. Remote copies payloads back after stopping/starting its counters to capture
   RTT information.
3. Host aggregates the collected statistics and writes the results back into
   shared memory before signalling completion.

Select the demo by configuring CMake with `-DDEMO=shmem_latency_demo` and pass
`-DROLE=host` or `-DROLE=remote` to build either endpoint.
