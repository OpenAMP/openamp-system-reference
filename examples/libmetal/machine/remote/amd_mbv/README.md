# AMD MicroBlaze-V libmetal remote

Bare-metal MicroBlaze-V remote of `irq_shmem_demo`. It runs from the BRAM
that the Linux BRAM remoteproc driver loads, uses the MicroBlaze-V interface
of an AXI Mailbox for notifications, and shares DDR regions with the Linux
host (`../../host/amd_linux_userspace`, PL Mailbox backend).

## Configuration

Everything design-specific comes from Lopper, generated from the system
device tree (SDT) of the design and the MicroBlaze-V libmetal domain YAMLs.
There are no built-in defaults; CMake fails if an input is missing.

| Input | Provides |
|---|---|
| `DEMO_CFG_FILE` | shared-memory layout as the MicroBlaze-V addresses it, device names, `MBOX_BASE_ADDR`, `MBOX_SIZE` |
| `LINKER_METADATA_FILE` (`*Example.cmake`) | memories, `CODE` and `DATA` regions, stack and heap sizes, for `linker_files/lscript_mb_riscv.ld.in` |

Each is taken from the global property of that name, as the Vitis/Empyro
flow sets them, or from a `-D` variable. The mailbox interrupt comes from
the BSP's configuration of the mailbox at `MBOX_BASE_ADDR`. The stack and
heap sizes come from the MicroBlaze-V domain's
`linker: { stack_size, heap_size }`; without them, Lopper gives a
MicroBlaze-V with a libmetal relation a 0x4000 heap.

The Vitis/Empyro flow generates both files when the domain YAMLs are in the
BSP's `hw_artifacts` directory. To generate them by hand, with the
MicroBlaze-V domain YAMLs (for example `versal-2ve-2vm-mbv-multidomain-base.yaml`
and `versal-2ve-2vm-mbv-libmetal-overlay.yaml` in Lopper's
`demos/openamp/inputs`):

```sh
lopper -f --enhanced -x '*.yaml' \
    -i versal-2ve-2vm-mbv-multidomain-base.yaml \
    -i versal-2ve-2vm-mbv-libmetal-overlay.yaml \
    system-top.dts domains.dts
lopper -f --enhanced domains.dts unused.dts -- openamp \
    --libmetal_output_file --compatible-string=libmetal,ipc-v1 \
    --processor=microblaze_riscv_0 --os=baremetal_dt \
    --openamp_output_filename=mbv.cmake
mkdir -p linker
lopper -f --enhanced -O linker domains.dts unused.dts -- \
    baremetallinker_xlnx microblaze_riscv_0 linker
```

The addresses are as the MicroBlaze-V sees them, translated through its
`address-map`. Generate the Linux device tree overlay from the same YAMLs
(see the host README) so both sides agree.

## Build

From `examples/libmetal`, with the BSP's MicroBlaze-V toolchain file and a
standalone BSP that provides libxil, libmetal and libmetal_xlnx_extension:

```sh
cmake -S . -B build_remote \
  -DCMAKE_TOOLCHAIN_FILE=${BSP_DIR}/microblaze_riscv_toolchain.cmake \
  -DCMAKE_INCLUDE_PATH=${BSP_DIR}/include \
  -DCMAKE_LIBRARY_PATH=${BSP_DIR}/lib \
  -DDEMO=irq_shmem_demo -DROLE=remote \
  -DPROJECT_SYSTEM=generic -DPROJECT_MACHINE=amd_mbv \
  -DDEMO_CFG_FILE=$PWD/mbv.cmake \
  -DLINKER_METADATA_FILE=$(ls $PWD/linker/*Example.cmake)
cmake --build build_remote
```
