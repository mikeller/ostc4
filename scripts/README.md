# Maintained Scripts

This directory contains versioned developer and manufacturing utilities supported by this repository.

`ostc4_service.py` accesses the OSTC4 service protocol over a direct Classic Bluetooth RFCOMM address or an existing serial-port mapping. From the repository root, run `python3 scripts/ostc4_service.py --help` for usage. Manufacturing write operations and Bluetooth-name reset affect physical devices; verify the target device and serial number before running them.

`flash_bootloader_preserve_mfg.sh` is an ST-Link recovery operation. It flashes a bootloader, saves a required manufacturing-data backup, resets the Bluetooth-name flags, and verifies the final sector write. From the repository root, run `./scripts/flash_bootloader_preserve_mfg.sh --help` before use.
