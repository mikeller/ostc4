#
# Requires the arm-none-eabi toolchain to be installed and available in the PATH.
# Currently builds with version 9 of the toolchain, available from:
# https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
#

SCRIPT_DIR := $(shell pwd)

OSTC4_BUILD_DIR ?= $(SCRIPT_DIR)
export OSTC4_BUILD_DIR

NUM_CORES := $(shell nproc)

# Default target

test:
	@echo $(SCRIPT_DIR)
	@echo $(OSTC4_BUILD_DIR)

firmware: firmware_binary packer
	ostc4pack/create_full_update_bin.sh --no-rte --no-fonts

fontpack: firmware_binary fontpack_binary packer
	ostc4pack/create_full_update_bin.sh --no-rte

rte: firmware_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh --no-fonts

all: firmware_binary fontpack_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh

fontpack_library:
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile -j $(NUM_CORES)

firmware_binary: fontpack_library
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile -j $(NUM_CORES)

fontpack_binary: fontpack_library
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile -j $(NUM_CORES)

rte_binary: fontpack_library
	$(MAKE) -C RefPrj/RTE/Release -f Makefile -j $(NUM_CORES)

clean:
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile clean
	$(MAKE) -C RefPrj/RTE/Release -f Makefile clean
	rm -f Release/OSTC4_Firmware*.bin Release/OSTC4_FontPack*.bin Release/OSTC4_RTE*.bin

packer:
	$(MAKE) -C ostc4pack/src -j $(NUM_CORES)

packer_clean:
	$(MAKE) -C ostc4pack/src clean

distclean: clean packer_clean

.PHONY: firmware fontpack rte fontpack_library firmware_binary fontpack_binary rte_binary all clean packer packer_clean distclean
