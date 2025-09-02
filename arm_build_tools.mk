ARM_TOOLCHAIN_VERSION ?= 9-2020-q2-update
ARM_TOOLCHAIN_OS ?= x86_64-linux
ARM_TOOLCHAIN_URL ?= https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/$(ARM_TOOLCHAIN_ARCHIVE)
ARM_TOOLCHAIN_SHA256 ?=

ifndef BUILD_TOOLS_DIR
ifndef BUILD_DIR
  $(error BUILD_TOOLS_DIR or BUILD_DIR must be set before including arm_build_tools.mk)
endif

BUILD_TOOLS_DIR := $(BUILD_DIR)/tools
endif

ARM_TOOLCHAIN_ARCHIVE := gcc-arm-none-eabi-$(ARM_TOOLCHAIN_VERSION)-$(ARM_TOOLCHAIN_OS).tar.bz2
TOOLCHAIN_DIR := $(BUILD_TOOLS_DIR)/gcc-arm-none-eabi-$(ARM_TOOLCHAIN_VERSION)
TOOLCHAIN_LINK := $(BUILD_TOOLS_DIR)/gcc-arm-none-eabi
export PATH := $(abspath $(TOOLCHAIN_LINK)/bin):$(PATH)

arm_tools: $(TOOLCHAIN_LINK)

$(TOOLCHAIN_LINK):
	$(MAKE) arm_tools_clean
	@set -eu; \
	mkdir -p "$(BUILD_TOOLS_DIR)/dist"; \
	if [ ! -d "$(TOOLCHAIN_DIR)" ]; then \
	  echo "Downloading $(ARM_TOOLCHAIN_URL)"; \
	  curl -fL --retry 3 -z "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" -o "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" "$(ARM_TOOLCHAIN_URL)"; \
	  if [ -n "$(ARM_TOOLCHAIN_SHA256)" ]; then \
	    echo "$(ARM_TOOLCHAIN_SHA256)  $(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" | sha256sum -c -; \
	  fi; \
	  echo "Unpacking $(ARM_TOOLCHAIN_ARCHIVE)"; \
	  tar -xjf "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" -C "$(BUILD_TOOLS_DIR)"; \
	fi; \
	ln -sfn "$(TOOLCHAIN_DIR)" "$(TOOLCHAIN_LINK)"

arm_tools_version:
	@echo $(ARM_TOOLCHAIN_VERSION)

arm_tools_clean:
	rm -rf "$(TOOLCHAIN_LINK)" "$(TOOLCHAIN_DIR)"

distclean: arm_tools_clean

.PHONY: arm_tools arm_tools_version arm_tools_clean distclean
