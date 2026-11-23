# Steps to Generate Inputs for AMD-Xilinx RPU Firmware Demos

Below is sample run for SOM KV260 platform

## Pick Up Domain YAMLs

```sh
git clone https://github.com/Xilinx/meta-xilinx.git -b rel-v2026.1
export OPENAMP_OVERLAY_YAML=$PWD/meta-xilinx/meta-xilinx-standalone-sdt/conf/domainyaml/zynqmp-openamp-overlay.yaml
export BASE_YAML=$PWD/meta-xilinx/meta-xilinx-standalone-sdt/conf/domainyaml/zynqmp-multidomain-base.yaml
```

## Pick Up System Device Tree

```sh
wget https://edf.amd.com/sswreleases/rel-v2026.1/sdt/2026.1/2026.1_0609_1_06092108/external/k26-smk-kv-sdt/k26-smk-kv-sdt_2026.1_0609_1_06092108.tar.gz
tar xvf k26-smk-kv-sdt_2026.1_0609_1_06092108.tar.gz
export SDT=$PWD/k26-smk-kv-sdt_2026.1_0609_1_06092108/system-top.dts

```

## Set Up Lopper

```sh
git clone https://github.com/devicetree-org/lopper.git -b master

cd lopper
git checkout d12bd4e28388d193512a3e573b7baad69783984c -b system_ref_demo
cd -

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
python3 $LOPPER_PY -f --enhanced   -x '*.yaml' -i $BASE_YAML -i $OPENAMP_OVERLAY_YAML $SDT rpu.dts
export RPU_DTS=$PWD/rpu.dts
```
The above Device Tree "rpu.dts" will be used for configuration of the app's interrupts, shared memory and linker script.

## Generate OpenAMP App Config Header

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
python3 $LOPPER_PY --enhanced  --permissive  -O . ${RPU_DTS} -- openamp --openamp_header_only \
 --openamp_output_filename=amd_platform_info.h --openamp_remote=psu_cortexr5_0
```
The output amd_platform_info.h needs to be in the location denoted above of "openamp-system-reference/examples/legacy_apps/machine/zynqmp_r5" BEFORE
cmake configure step.

## Generate RPU Application Linker Config Object

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
python3 $LOPPER_PY -O . $RPU_DTS -- baremetallinker_xlnx psu_cortexr5_0 . openamp
```
The RPU Application Linker config object needs to be pointed to with cmake variable LINKER_METADATA_FILE at cmake configure step.

## Generate Linux Device Tree With OpenAMP Nodes

Generate the OpenAMP-processed System Device Tree for the APU, then prune it to the Linux domain. This uses `rpu.dts`, which already contains the domain and OpenAMP YAML content applied above.

```sh
export LOPPER_DTC_FLAGS="-b 0 -@"
export APU_PROCESSOR=psu_cortexa53_0
export LINUX_OPENAMP_OUTPUT=$PWD/linux-openamp
export LOPPER_ROOT=$(dirname "$LOPPER_PY")
mkdir -p "$LINUX_OPENAMP_OUTPUT"

python3 "$LOPPER_PY" -O "$LINUX_OPENAMP_OUTPUT" -f --enhanced \
 "$RPU_DTS" "$LINUX_OPENAMP_OUTPUT/$APU_PROCESSOR-openamp.dts" \
 -- openamp "$APU_PROCESSOR" linux_dt

python3 "$LOPPER_PY" -O "$LINUX_OPENAMP_OUTPUT" -f --enhanced \
 -i "$LOPPER_ROOT/lopper/lops/lop-a53-imux.dts" \
 "$LINUX_OPENAMP_OUTPUT/$APU_PROCESSOR-openamp.dts" \
 "$LINUX_OPENAMP_OUTPUT/cortexa53-linux-openamp.dts" \
 -- gen_domain_dts "$APU_PROCESSOR" linux_dt
```

The generated Linux Device Tree with the OpenAMP remoteproc and reserved-memory nodes is `linux-openamp/cortexa53-linux-openamp.dts`.
