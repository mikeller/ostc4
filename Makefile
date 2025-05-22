ARM_TOOLCHAIN_VERSION := 9-2020-q2-update

SCRIPT_DIR := $(shell pwd)

OSTC4_BUILD_DIR ?= $(SCRIPT_DIR)
export OSTC4_BUILD_DIR

BUILD_TOOLS_DIR ?= $(OSTC4_BUILD_DIR)/tools
export PATH := $(abspath $(BUILD_TOOLS_DIR)/gcc-arm-none-eabi/bin):$(PATH)

NUM_CORES := $(shell nproc)

# Default target

firmware: firmware_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-rte --no-fonts

fontpack: firmware_binary fontpack_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-rte

rte: firmware_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh --version --no-fonts

all: firmware_binary fontpack_binary rte_binary packer
	ostc4pack/create_full_update_bin.sh --version

fontpack_library: arm_tools
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile -j $(NUM_CORES)

firmware_binary: arm_tools fontpack_library
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile -j $(NUM_CORES)

fontpack_binary: arm_tools fontpack_library
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile -j $(NUM_CORES)

rte_binary: arm_tools fontpack_library
	$(MAKE) -C RefPrj/RTE/Release -f Makefile -j $(NUM_CORES)

clean:
	$(MAKE) -C RefPrj/Firmware/Release -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Library -f Makefile clean
	$(MAKE) -C RefPrj/FontPack/Release -f Makefile clean
	$(MAKE) -C RefPrj/RTE/Release -f Makefile clean
	rm -f Release/OSTC4_Firmware*.bin Release/OSTC4_FontPack*.bin Release/OSTC4_RTE*.bin

print_version:
	@ostc4pack/create_full_update_bin.sh --version --no-date --print-version-only

arm_tools: $(BUILD_TOOLS_DIR)/gcc-arm-none-eabi

$(BUILD_TOOLS_DIR)/gcc-arm-none-eabi:
	$(MAKE) arm_tools_clean
	mkdir -p $(BUILD_TOOLS_DIR)
	curl -L https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-$(ARM_TOOLCHAIN_VERSION)-x86_64-linux.tar.bz2 | tar -xj -C $(BUILD_TOOLS_DIR)
	cd $(BUILD_TOOLS_DIR); ln -s gcc-arm-none-eabi-* gcc-arm-none-eabi

arm_tools_version:
	@echo $(ARM_TOOLCHAIN_VERSION)

arm_tools_clean:
	rm -rf $(BUILD_TOOLS_DIR)/gcc-arm-none-eabi*

packer:
	$(MAKE) -C ostc4pack/src -j $(NUM_CORES)

packer_clean:
	$(MAKE) -C ostc4pack/src clean

distclean: clean packer_clean arm_tools_clean

.PHONY: firmware fontpack rte fontpack_library firmware_binary fontpack_binary rte_binary all clean print_version arm_tools arm_tools_version arm_tools_clean packer packer_clean distclean
