# Steps to generate inputs for AMD-Xilinx RPU Firmware Demos

Dependencies:
1. Lopper : https://github.com/devicetree-org/lopper.git
2. System Device Tree generated from design : https://docs.amd.com/r/en-US/ug1647-porting-embeddedsw-components/Generating-a-System-Device-Tree-Using-SDTGen

Below is sample run for Versal Gen 1 platform
### Generate OpenAMP RPU Device Tree

SDT is the System Device Tree generated from design
```sh
export LOPPER_DTC_FLAGS="-b 0 -@"

python3 lopper.py -f --enhanced \
  -x '*.yaml' \
  -i $YAML $SDT yaml_applied.dts

python3 lopper.py -f --enhanced \
  yaml_applied.dts rpu.dts \
  -- gen_domain_dts psu_cortexr5_0   --openamp_no_header
```
The above Device Tree "rpu.dts" will be used for configuration of the app's interrupts, shared memory and linker script.

### Generate OpenAMP App config header

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
export CONFIG_DTFILE=rpu.dts

cd openamp-system-reference/examples/legacy_apps/machine/zynqmp_r5
python3 lopper.py -O -f -v --enhanced  --permissive \
  -O . ${CONFIG_DTFILE} -- openamp --openamp_header_only \
  --openamp_output_filename=amd_platform_info.h \
  --openamp_remote=psv_cortexr5_0
cd -
```
The output amd_platform_info.h needs to be in the location denoted above of "openamp-system-reference/examples/legacy_apps/machine/zynqmp_r5" BEFORE
cmake configure step.

### Generate RPU Application Linker config object

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
export CONFIG_DTFILE=rpu.dts
python3 lopper.py -O ${S} rpu.dts \
  -- baremetallinker_xlnx psv_cortexr5_0 <output location> openamp
```
The RPU Application Linker config object needs to be pointed to with cmake variable LINKER_METADATA_FILE at cmake configure step.
