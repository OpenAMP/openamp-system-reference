# IRQ Shared Memory Demo

This README captures information that is shared between the host and remote
machine configurations for the IRQ shared memory demo.

## Shared Memory Layout

Both sides of the demo operate on the same shared-memory window. Offsets below
reflect the AMD reference design; adjust the base address to match the target
platform.

| Offset Range        | Description                                                          |
|---------------------|----------------------------------------------------------------------|
| 0x00000 – 0x00003   | Number of Host-to-Remote buffers available to the remote             |
| 0x00004 – 0x00007   | Number of Host-to-Remote buffers consumed by the remote              |
| 0x00008 – 0x03FFC   | Address array for Host-to-Remote shared buffers                      |
| 0x04000 – 0x04003   | Number of Remote-to-Host buffers available to the host               |
| 0x04004 – 0x04007   | Number of Remote-to-Host buffers consumed by the host                |
| 0x04008 – 0x07FFC   | Address array for Remote-to-Host shared buffers                      |
| 0x08000 – 0x27FFF   | Host-to-Remote payload buffers                                       |
| 0x28000 – 0x47FFF   | Remote-to-Host payload buffers                                       |

