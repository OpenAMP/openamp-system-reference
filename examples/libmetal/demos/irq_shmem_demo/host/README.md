# IRQ Shared Memory Demo – Host Side

The host application publishes timestamped packets into a shared-memory window,
signals the remote processor via IPI, and waits for the echoed payloads to
measure round-trip latency. After the final message it sends a `"shutdown"`
marker to stop the remote task.

## Supported Platforms
- [AMD Linux Userspace](../../../machine/host/amd_linux_userspace/README.md)

Platform-specific prerequisites, build instructions, and troubleshooting tips
live in the linked machine documentation.
