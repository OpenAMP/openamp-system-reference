# AMD MicroBlaze-V OpenAMP remote (mbv32)

Bare-metal MicroBlaze-V machine for the legacy echo, matrix-multiply and RPC
demos. Linux is the remoteproc and RPMsg host: it loads the firmware into
the MicroBlaze-V BRAM through an AXI BRAM controller, releases it from reset
through an AXI GPIO, and exchanges virtqueue notifications with it through
an AXI Mailbox. The vrings and RPMsg buffers are in DDR that the MicroBlaze-V
reaches through its address map.

## Configuration

Everything design-specific comes from Lopper, generated from the system
device tree (SDT) of the design and the MicroBlaze-V OpenAMP domain YAMLs.
There are no built-in defaults; the build fails if an input is missing.

| Input | Provides | Used by |
|---|---|---|
| `amd_platform_info.h` | `SHARED_MEM_PA`, `SHARED_MEM_SIZE`, `SHARED_BUF_OFFSET`, `RING_TX`, `RING_RX`, `MBOX_BASE_ADDR` | `platform_config.h` |
| Linker metadata (`*Example.cmake`) | memories, `CODE` and `DATA` regions, stack and heap sizes | `../linker_files/lscript_mb_riscv.ld.in` |

`amd_platform_info.h` is read from this directory, or from the file named by
`-DMBV32_PLATFORM_INFO=<path>`. The mailbox interrupt comes from the BSP's
configuration of that mailbox. The stack and heap sizes come from the
MicroBlaze-V domain's `linker: { stack_size, heap_size }`; without them,
Lopper gives a MicroBlaze-V with an RPMsg relation a 0x4000 heap.

The Vitis/Empyro flow generates both inputs when the domain YAMLs are in the
BSP's `hw_artifacts` directory. To generate them by hand, with the
MicroBlaze-V domain YAMLs (for example `versal-2ve-2vm-mbv-multidomain-base.yaml`
and `versal-2ve-2vm-mbv-openamp-overlay.yaml` in Lopper's
`demos/openamp/inputs`):

```sh
lopper -f --enhanced -x '*.yaml' \
    -i versal-2ve-2vm-mbv-multidomain-base.yaml \
    -i versal-2ve-2vm-mbv-openamp-overlay.yaml \
    system-top.dts domains.dts
lopper -f --enhanced domains.dts unused.dts -- openamp \
    --openamp_header_only --openamp_remote=microblaze_riscv_0 \
    --openamp_output_filename=amd_platform_info.h
lopper -f --enhanced -O <app source directory> domains.dts unused.dts -- \
    baremetallinker_xlnx microblaze_riscv_0 <app source directory>
```

The addresses are as the MicroBlaze-V sees them, translated through its
`address-map`. The same domain YAMLs generate the Linux device tree overlay
(`openamp cortexa78_0 linux_dt`, then `xlnx_overlay_pl_dt`), so firmware and
Linux use the same carveouts.
