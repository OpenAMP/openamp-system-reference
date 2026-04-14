# Steps to generate inputs for AMD-Xilinx RPU Firmware Demos

Below is sample run for SOM KV260 platform

## Pick up System Device Tree

```sh
wget https://edf.amd.com/sswreleases/rel-v2025.2/sdt/2025.2/2025.2_1115_1_11150857/external/k26-smk-kv-sdt/k26-smk-kv-sdt_2025.2_1115_1_11150857.tar.gz
tar xvf k26-smk-kv-sdt_2025.2_1115_1_11150857.tar.gz
export SDT=$PWD/k26-smk-kv-sdt_2025.2_1115_1_11150857/system-top.dts
```

## Set up Lopper

```sh
git clone https://github.com/devicetree-org/lopper.git
python3 -m venv .venv
source .venv/bin/activate
pip3 install -r lopper/requirements.txt
pip3 install lopper

export LOPPER_PY=$PWD/lopper/lopper.py
```

## Apply Domain YAML to System Device Tree
SDT is the System Device Tree generated from design

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
python3 $LOPPER_PY -f --enhanced   -x '*.yaml' -i $YAML $SDT yaml_applied.dts
python3 $LOPPER_PY -f --enhanced yaml_applied.dts rpu.dts -- gen_domain_dts psu_cortexr5_0   --openamp_no_header

export RPU_DTS=$PWD/rpu.dts
```
The above Device Tree "rpu.dts" will be used for configuration of the app's interrupts, shared memory and linker script.

## Generate OpenAMP App config header

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
python3 lopper.py -O -f --enhanced  --permissive  -O . ${RPU_DTS} -- openamp --openamp_header_only \
 --openamp_output_filename=amd_platform_info.h --openamp_remote=psu_cortexr5_0
```
The output amd_platform_info.h needs to be in the location denoted above of "openamp-system-reference/examples/legacy_apps/machine/zynqmp_r5" BEFORE
cmake configure step.

## Generate RPU Application Linker config object

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
python3 lopper.py -O . $RPU_DTS -- baremetallinker_xlnx psu_cortexr5_0 . openamp
```
The RPU Application Linker config object needs to be pointed to with cmake variable LINKER_METADATA_FILE at cmake configure step.
