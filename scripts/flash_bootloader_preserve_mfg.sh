#!/usr/bin/env bash
# Flash an OSTC4 bootloader and force Bluetooth-name reinitialisation.
set -euo pipefail

MFG_ADDR=0x0800A040
MFG_SIZE=64
SECTOR_ADDR=0x08008000
SECTOR_SIZE=$((16 * 1024))
MFG_OFFSET=$((MFG_ADDR - SECTOR_ADDR))
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
BOOTLOADER=$REPO_ROOT/RefPrj/BootLoader/Release/OSTC4_BootLoader.bin
BACKUP=
CONFIRMED=0
SECTOR_DUMP=
VERIFY_DUMP=

usage() {
	cat <<EOF
Usage: $0 --yes --backup PATH [--bootloader PATH]

Flash a bootloader through ST-Link while preserving the target's manufacturing
data. The primary and secondary Bluetooth-name flags are set to 0xFF so the
bootloader reinitialises the Bluetooth module on its next boot.

Options:
  --backup PATH       New file where the original 64-byte manufacturing block is saved.
  --bootloader PATH   Bootloader binary (default: $BOOTLOADER).
  --yes               Acknowledge the bootloader and manufacturing-flash writes.
  -h, --help          Show this help text.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--backup)
			[ "$#" -ge 2 ] || { usage >&2; exit 2; }
			BACKUP=$2
			shift 2
			;;
		--bootloader)
			[ "$#" -ge 2 ] || { usage >&2; exit 2; }
			BOOTLOADER=$2
			shift 2
			;;
		--yes)
			CONFIRMED=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Error: unknown option: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

[ "$CONFIRMED" -eq 1 ] || { echo "Error: --yes is required." >&2; exit 2; }
[ -n "$BACKUP" ] || { echo "Error: --backup is required." >&2; exit 2; }
[ -f "$BOOTLOADER" ] || { echo "Error: bootloader binary not found: $BOOTLOADER" >&2; exit 1; }
[ ! -e "$BACKUP" ] || { echo "Error: backup already exists: $BACKUP" >&2; exit 1; }
[ -d "$(dirname "$BACKUP")" ] || { echo "Error: backup directory does not exist: $(dirname "$BACKUP")" >&2; exit 1; }

for command in st-flash xxd dd cmp mktemp; do
	command -v "$command" >/dev/null || { echo "Error: required command not found: $command" >&2; exit 1; }
done

cleanup() {
	rm -f "$SECTOR_DUMP" "$VERIFY_DUMP"
}
trap cleanup EXIT

SECTOR_DUMP=$(mktemp /tmp/ostc4-sector2.XXXXXX)
VERIFY_DUMP=$(mktemp /tmp/ostc4-sector2-verify.XXXXXX)

echo "Reading manufacturing data into $BACKUP"
st-flash read "$BACKUP" "$MFG_ADDR" "$MFG_SIZE"
[ "$(wc -c < "$BACKUP")" -eq "$MFG_SIZE" ] || {
	echo "Error: manufacturing-data backup is not $MFG_SIZE bytes." >&2
	exit 1
}

echo "Original manufacturing data:"
xxd -l "$MFG_SIZE" "$BACKUP"

echo "Flashing bootloader: $BOOTLOADER"
st-flash --connect-under-reset write "$BOOTLOADER" 0x08000000

echo "Reading the bootloader's Sector 2"
st-flash read "$SECTOR_DUMP" "$SECTOR_ADDR" "$SECTOR_SIZE"
[ "$(wc -c < "$SECTOR_DUMP")" -eq "$SECTOR_SIZE" ] || {
	echo "Error: Sector 2 dump is not $SECTOR_SIZE bytes." >&2
	exit 1
}

dd if="$BACKUP" of="$SECTOR_DUMP" bs=1 seek="$MFG_OFFSET" count="$MFG_SIZE" conv=notrunc
printf '\xff' | dd of="$SECTOR_DUMP" bs=1 seek="$((MFG_OFFSET + 7))" count=1 conv=notrunc
printf '\xff' | dd of="$SECTOR_DUMP" bs=1 seek="$((MFG_OFFSET + 59))" count=1 conv=notrunc

echo "Writing complete Sector 2 with Bluetooth-name flags reset"
st-flash --connect-under-reset write "$SECTOR_DUMP" "$SECTOR_ADDR"

echo "Verifying complete Sector 2"
st-flash read "$VERIFY_DUMP" "$SECTOR_ADDR" "$SECTOR_SIZE"
cmp -s "$SECTOR_DUMP" "$VERIFY_DUMP" || {
	echo "Error: Sector 2 verification failed. Original backup remains at $BACKUP" >&2
	exit 1
}

echo "Done. Power-cycle the device so the bootloader reinitialises Bluetooth."
