#!/usr/bin/env bash
#
# DT probe: discover libmetal irq_shmem_demo config at runtime.
#
# Usage:
#   ./discover_platform.sh [-o /path/to/config] [--dt-base <path>] [--debug]

set -euo pipefail

DT_BASE_DEFAULT="/proc/device-tree"
UIO_BASE="/sys/class/uio"

die() {
	echo "ERROR: $*" >&2
	exit 1
}

usage() {
	cat >&2 <<'EOF'
Usage:
  discover_platform.sh [-o /path/to/config] [--dt-base <path>] [--debug]
EOF
	exit 1
}

read_dt_string() {
	tr -d '\000' < "$1"
}

read_dt_u32() {
	local path=$1
	local -a b
	local hex

	if command -v od >/dev/null 2>&1; then
		read -r -a b < <(od -An -N4 -tx1 "$path" 2>/dev/null || true)
		if [[ ${#b[@]} -ge 4 ]]; then
			printf "%u" "$(( (16#${b[0]} << 24) | (16#${b[1]} << 16) | (16#${b[2]} << 8) | 16#${b[3]} ))"
			return 0
		fi
	fi

	hex=$(hexdump -v -n 4 -e '1/1 "%02x"' "$path" 2>/dev/null || true)
	if [[ ${#hex} -lt 8 ]]; then
		if command -v xxd >/dev/null 2>&1; then
			hex=$(xxd -p -l 4 "$path" 2>/dev/null | tr -d '\n')
		fi
	fi
	[[ ${#hex} -ge 8 ]] || return 1
	printf "%u" "$((16#$hex))"
}

read_dt_u32_array() {
	local path=$1
	local count=${2:-}
	local hex cells n i chunk
	local -a b
	local total_bytes

	if command -v od >/dev/null 2>&1; then
		read -r -a b < <(od -An -tx1 "$path" 2>/dev/null || true)
		if [[ ${#b[@]} -ge 4 ]]; then
			total_bytes=${#b[@]}
			cells=$(( total_bytes / 4 ))
			if [[ -n "$count" && $cells -lt $count ]]; then
				die "property $path has $cells cells, expected $count"
			fi
			if [[ -n "$count" ]]; then
				n=$count
			else
				n=$cells
			fi
			for ((i = 0; i < n; i++)); do
				printf "%u\n" "$(( (16#${b[i*4]} << 24) | (16#${b[i*4+1]} << 16) | (16#${b[i*4+2]} << 8) | 16#${b[i*4+3]} ))"
			done
			return 0
		fi
	fi

	hex=$(hexdump -v -e '1/1 "%02x"' "$path" 2>/dev/null || true)
	[[ -n "$hex" ]] || return 1
	cells=$(( ${#hex} / 8 ))
	if [[ -n "$count" && $cells -lt $count ]]; then
		die "property $path has $cells cells, expected $count"
	fi
	if [[ -n "$count" ]]; then
		n=$count
	else
		n=$cells
	fi
	for ((i = 0; i < n; i++)); do
		chunk=${hex:$((i * 8)):8}
		printf "%u\n" "$((16#$chunk))"
	done
}

read_dt_reg() {
	local node_path=$1
	local addr_cells=${2:-2}
	local size_cells=${3:-2}
	local entry_len=$((addr_cells + size_cells))
	local -a cells
	local i c addr size

	mapfile -t cells < <(read_dt_u32_array "$node_path/reg")
	for ((i = 0; i + entry_len - 1 < ${#cells[@]}; i += entry_len)); do
		addr=0
		for ((c = 0; c < addr_cells; c++)); do
			addr=$(( (addr << 32) | cells[i + c] ))
		done
		size=0
		for ((c = 0; c < size_cells; c++)); do
			size=$(( (size << 32) | cells[i + addr_cells + c] ))
		done
		printf "%u %u\n" "$addr" "$size"
	done
}

normalize_addr() {
	local addr
	addr=$(echo "$1" | tr 'A-Z' 'a-z')
	addr=$(echo "$addr" | sed 's/^0*//')
	if [[ -z "$addr" ]]; then
		addr="0"
	fi
	echo "$addr"
}

node_unit_addr() {
	local node_path=$1
	local name addr

	name=$(basename -- "$node_path")
	if [[ "$name" == *"@"* ]]; then
		addr=${name#*@}
		echo "$addr"
		return 0
	fi
	die "node $name has no unit address"
}

node_to_dev_name() {
	local node_path=$1
	local base name addr

	base=$(basename -- "$node_path")
	if [[ "$base" == *"@"* ]]; then
		name=${base%@*}
		addr=${base#*@}
		printf "%s.%s" "$addr" "$name"
		return 0
	fi
	echo "$base"
}

find_uio_by_addr() {
	local unit_addr=$1
	local entry name_path uio_name prefix

	unit_addr=$(normalize_addr "$unit_addr")
	[[ -d "$UIO_BASE" ]] || die "$UIO_BASE not found"

	for entry in "$UIO_BASE"/uio*; do
		[[ -d "$entry" ]] || continue
		name_path="$entry/name"
		[[ -f "$name_path" ]] || continue
		uio_name=$(cat "$name_path")
		prefix=${uio_name%%.*}
		prefix=$(normalize_addr "$prefix")
		if [[ "$prefix" == "$unit_addr" ]]; then
			echo "$uio_name"
			return 0
		fi
	done

	die "no UIO device found with unit address $unit_addr"
}

resolve_phandle() {
	local dt_base=$1
	local target=$2
	local file dir
	local -a b
	local be le
	local files

	files=$(find /proc/device-tree/  | grep phandle)
	if [[ -z "$files" ]]; then
		die "no phandle files found under $dt_base"
	fi
	if [[ -n "${DEBUG:-}" ]]; then
		echo "DEBUG: scanning phandle files for target=0x$(printf '%x' "$target")" >&2
	fi

	while IFS= read -r file; do
		read -r -a b < <(od -An -N4 -tx1 "$file" 2>/dev/null || true)
		if [[ ${#b[@]} -lt 4 ]]; then
			continue
		fi
		be=$(( (16#${b[0]} << 24) | (16#${b[1]} << 16) | (16#${b[2]} << 8) | 16#${b[3]} ))
		le=$(( (16#${b[3]} << 24) | (16#${b[2]} << 16) | (16#${b[1]} << 8) | 16#${b[0]} ))
		if [[ -n "${DEBUG:-}" ]]; then
			printf "DEBUG: %s -> be=0x%x le=0x%x (target=0x%x)\n" \
				"$file" "$be" "$le" "$target" >&2
		fi
		if [[ $be -eq $target || $le -eq $target ]]; then
			dir=$(dirname -- "$file")
			if [[ -n "${DEBUG:-}" ]]; then
				if [[ $le -eq $target && $be -ne $target ]]; then
					printf "DEBUG: matched LE 0x%x -> %s\n" "$le" "$dir" >&2
				else
					printf "DEBUG: matched 0x%x -> %s\n" "$be" "$dir" >&2
				fi
			fi
			echo "$dir"
			return 0
		fi
	done <<< "$files"

	die "phandle 0x$(printf '%x' "$target") not found under $dt_base"
}

find_libmetal_relation() {
	local domain_path=$1
	local lm_rel="$domain_path/domain-to-domain/libmetal-relation"
	local compat_path compat entry rel_path

	[[ -d "$lm_rel" ]] || die "no libmetal-relation under $domain_path"

	compat_path="$lm_rel/compatible"
	if [[ -f "$compat_path" ]]; then
		compat=$(read_dt_string "$compat_path")
		if [[ -n "${DEBUG:-}" ]]; then
			echo "DEBUG: $compat_path='$compat'" >&2
		fi
		if [[ "$compat" != "libmetal,ipc-v1" ]]; then
			die "unexpected compatible '$compat' (expected libmetal,ipc-v1)"
		fi
	fi

	while IFS= read -r entry; do
		rel_path="$lm_rel/$entry"
		if [[ -d "$rel_path" && "$entry" == relation* ]]; then
			echo "$rel_path"
			return 0
		fi
	done < <(ls -1 "$lm_rel" | sort)

	die "no relation node under $lm_rel"
}

find_linux_domain() {
	local dt_base=$1
	local domains_dir="$dt_base/domains"
	local entry dom_path os_type_path os_type

	[[ -d "$domains_dir" ]] || die "no domains/ directory in $dt_base"

	while IFS= read -r entry; do
		dom_path="$domains_dir/$entry"
		[[ -d "$dom_path" ]] || continue
		os_type_path="$dom_path/os,type"
		[[ -f "$os_type_path" ]] || continue
		os_type=$(read_dt_string "$os_type_path")
		if [[ -n "${DEBUG:-}" ]]; then
			echo "DEBUG: $dom_path os,type='$os_type'" >&2
		fi
		if [[ "$os_type" == "linux" ]]; then
			echo "$dom_path"
			return 0
		fi
	done < <(ls -1 "$domains_dir" | sort)

	die "no domain with os,type=linux found"
}

main() {
	local dt_base="$DT_BASE_DEFAULT"
	local output=""
	local cfg_text

	while [[ $# -gt 0 ]]; do
		case "$1" in
		-o|--output)
			[[ $# -ge 2 ]] || usage
			output=$2
			shift 2
			;;
		--dt-base)
			[[ $# -ge 2 ]] || usage
			dt_base=$2
			shift 2
			;;
		--debug)
			DEBUG=1
			shift
			;;
		-h|--help)
			usage
			;;
		*)
			usage
			;;
		esac
	done

	linux_dom=$(find_linux_domain "$dt_base")
	if [[ -n "${DEBUG:-}" ]]; then
		echo "DEBUG: Linux domain found: $linux_dom" >&2
	fi

	rel_path=$(find_libmetal_relation "$linux_dom")
	if [[ -n "${DEBUG:-}" ]]; then
		echo "DEBUG: libmetal relation found: $rel_path" >&2
	fi

	mapfile -t carveout_phs < <(read_dt_u32_array "$rel_path/carveouts" 3)
	mbox_ph=$(read_dt_u32 "$rel_path/mbox")
	timer_ph=$(read_dt_u32 "$rel_path/timer")

	if [[ -n "${DEBUG:-}" ]]; then
		printf "DEBUG: carveouts phandles: 0x%x 0x%x 0x%x\n" \
			"${carveout_phs[0]}" "${carveout_phs[1]}" "${carveout_phs[2]}" >&2
		printf "DEBUG: mbox phandle: 0x%x\n" "$mbox_ph" >&2
		printf "DEBUG: timer phandle: 0x%x\n" "$timer_ph" >&2
	fi

	carveout0_node=$(resolve_phandle "$dt_base" "${carveout_phs[0]}")
	carveout1_node=$(resolve_phandle "$dt_base" "${carveout_phs[1]}")
	carveout2_node=$(resolve_phandle "$dt_base" "${carveout_phs[2]}")
	mbox_node=$(resolve_phandle "$dt_base" "$mbox_ph")
	timer_node=$(resolve_phandle "$dt_base" "$timer_ph")

	ipi_parent=$(dirname -- "$mbox_node")
	ipi_mask=$(read_dt_u32 "$mbox_node/xlnx,ipi-bitmask")
	ipi_dev_name=$(node_to_dev_name "$ipi_parent")
	ttc_dev_name=$(node_to_dev_name "$timer_node")

	reg0=$(read_dt_reg "$carveout0_node" | head -n 1 || true)
	reg1=$(read_dt_reg "$carveout1_node" | head -n 1 || true)
	reg2=$(read_dt_reg "$carveout2_node" | head -n 1 || true)
	desc0_size=$(echo "$reg0" | awk '{print $2}')
	desc1_size=$(echo "$reg1" | awk '{print $2}')
	payload_size=$(echo "$reg2" | awk '{print $2}')
	[[ -n "$desc0_size" ]] || desc0_size=0
	[[ -n "$desc1_size" ]] || desc1_size=0
	[[ -n "$payload_size" ]] || payload_size=0

	shm0_desc_dev_name=$(node_to_dev_name "$carveout0_node")
	shm1_desc_dev_name=$(node_to_dev_name "$carveout1_node")
	shm_dev_name=$(node_to_dev_name "$carveout2_node")

	cfg_text=$(
		printf "SHM0_DESC_DEV_NAME=%s\n" "$shm0_desc_dev_name"
		printf "SHM1_DESC_DEV_NAME=%s\n" "$shm1_desc_dev_name"
		printf "SHM_DEV_NAME=%s\n" "$shm_dev_name"
		printf "IPI_DEV_NAME=%s\n" "$ipi_dev_name"
		printf "TTC_DEV_NAME=%s\n" "$ttc_dev_name"
		printf "IPI_MASK=0x%x\n" "$ipi_mask"
		printf "DESC0_SIZE=0x%x\n" "$desc0_size"
		printf "DESC1_SIZE=0x%x\n" "$desc1_size"
		printf "SHM_PAYLOAD_SIZE=0x%x\n" "$payload_size"
	)

	if [[ -n "$output" ]]; then
		printf "%s\n" "$cfg_text" > "$output"
		echo "Config written to $output" >&2
	else
		printf "%s\n" "$cfg_text"
	fi
}

main "$@"
