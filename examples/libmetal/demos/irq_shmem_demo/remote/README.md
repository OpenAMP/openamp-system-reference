# IRQ Shared Memory Demo – Remote Side

The remote firmware receives timestamped packets from the host, mirrors each
payload into the return buffer, and notifies the host via IPI. When a
`"shutdown"` marker arrives it disables interrupts and exits the loop.

## Demo Sequence
1. Get the shared memory device I/O region.
2. Get the IRQ device I/O region.
3. Register the IRQ interrupt handler.
4. Wait for remote IRQ notification to receive a message.
5. When a message is received, check whether it is the shutdown marker.
6. If it is shutdown, clean up; otherwise, echo it back to the shared buffer.
7. Kick IRQ to notify the host that a message was written into shared memory.
8. Repeat step 4.
9. Clean up: disable the IRQ interrupt and deregister the handler.

## Supported Platforms
- [AMD RPU (FreeRTOS)](../../../machine/remote/amd_rpu/README.md)

Platform-specific prerequisites, build instructions, and troubleshooting tips
live in the linked machine documentation.
