SCRIPT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR ?= $(SCRIPT_DIR)
export BUILD_DIR

FIRMWARE_INSTALLER := subsurface-downloader

INSTALL_TARGET := firmware
MODEL := "OSTC 4/5"
DEVICE := /dev/rfcomm0
FORCE :=

NUM_CORES := $(shell \
  command -v nproc >/dev/null 2>&1 && nproc || \
  getconf _NPROCESSORS_ONLN 2>/dev/null || \
  sysctl -n hw.ncpu 2>/dev/null || echo 1)

MAKEFLAGS += j$(NUM_CORES)
# Default target

firmware: firmware_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-rte --no-fonts

fontpack: firmware_binary fontpack_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-rte

rte: firmware_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-fonts

bootloader: bootloader_binary packer
	$(eval VERSION := $(shell BUILD_DIR=$(BUILD_DIR) NO_DATE=$(NO_DATE) ostc4pack/create_full_update_bin.sh --version --print-version-only))
	$(eval VERSION_STRING := $(if $(VERSION),_$(VERSION),))
	ostc4pack/create_full_update_bin.sh --version --no-rte --no-fonts --do-bootloader
	mv Release/OSTC4update_fontpack$(VERSION_STRING).bin Release/OSTC4update_bootloader$(VERSION_STRING).bin

all: firmware_binary fontpack_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh --version

fontpack_library: arm_tools_install
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile

firmware_binary: arm_tools_install fontpack_library
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile

fontpack_binary: arm_tools_install fontpack_library
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile

rte_binary: arm_tools_install fontpack_library
	$(MAKE) -C RefPrj/RTE/Release -f Makefile

bootloader_binary: arm_tools_install fontpack_library
	$(MAKE) -C RefPrj/BootLoader/Release -f Makefile

print_version:
	@ostc4pack/create_full_update_bin.sh --version --no-date --print-version-only

install: $(INSTALL_TARGET)
	$(eval VERSION := $(shell BUILD_DIR=$(BUILD_DIR) NO_DATE=$(NO_DATE) ostc4pack/create_full_update_bin.sh --version --print-version-only))
	$(eval VERSION_STRING := $(if $(VERSION),_$(VERSION),))
	$(eval FILENAME := "Release/OSTC4update_$(INSTALL_TARGET)$(VERSION_STRING).bin")
	$(FIRMWARE_INSTALLER) --update-firmware --dc-vendor="Heinrichs Weikamp" --dc-product=$(MODEL) --device=$(DEVICE) $(if $(FORCE),--force-update-firmware) --firmware-file=$(FILENAME)

packer:
	$(MAKE) -C ostc4pack/src

packer_clean:
	$(MAKE) -C ostc4pack/src clean

clean:
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile clean
	$(MAKE) -C RefPrj/RTE/Release -f Makefile clean
	$(MAKE) -C RefPrj/BootLoader/Release -f Makefile clean
	$(RM) -f Release/OSTC4_Firmware*.bin Release/OSTC4_FontPack*.bin Release/OSTC4_RTE*.bin

distclean:: clean packer_clean
	$(RM) -f Release/OSTC4update_*.bin

test:
	@echo "No tests defined"

include arm_build_tools.mk

.PHONY: firmware fontpack rte bootloader fontpack_library firmware_binary fontpack_binary rte_binary bootloader_binary all install clean print_version packer packer_clean distclean test
