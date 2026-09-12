# OSTC4 Firmware

Firmware and bootloader source for the OSTC4 family of dive computers.

## Layout

- `BootLoader/`, `Common/`, `Discovery/`, and `RefPrj/` contain firmware source and project definitions.
- `Documentations/` and `wiki/` retain upstream and historical reference material. Keep their layout stable to reduce Mercurial merge conflicts.
- `scripts/` contains maintained developer and manufacturing utilities.
- `tools/` is an ignored local workspace for temporary tooling and experiments.
- `Release/`, `Debug/`, and binary build outputs are generated and ignored.

Run `make` to build the firmware. Run `make bootloader_binary` to build the bootloader only.
