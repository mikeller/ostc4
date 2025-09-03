#
# Download URL: https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
#

ARM_TOOLCHAIN_VERSION ?= 9-2020-q2-update
ARM_TOOLCHAIN_PREFIX ?= gcc-arm-none-eabi

ARM_TOOLCHAIN_OS ?=
ifeq ($(strip $(ARM_TOOLCHAIN_OS)),)
  UNAME_S := $(shell uname -s 2>/dev/null)
  ifeq ($(UNAME_S),Linux)
    ARM_TOOLCHAIN_OS := x86_64-linux
  else ifeq ($(UNAME_S),Darwin)
    ARM_TOOLCHAIN_OS := mac
  else ifneq ($(filter MSYS_NT-% MINGW%,$(UNAME_S)),)
    ARM_TOOLCHAIN_OS := win32
  else ifdef OS
  # Basic Windows detection
    ARM_TOOLCHAIN_OS := win32
  else
    ARM_TOOLCHAIN_OS := x86_64-linux
  endif
endif

ARM_TOOLCHAIN := $(ARM_TOOLCHAIN_PREFIX)-$(ARM_TOOLCHAIN_VERSION)-$(ARM_TOOLCHAIN_OS)
ARM_TOOLCHAIN_ARCHIVE := $(ARM_TOOLCHAIN).tar.bz2
ARM_TOOLCHAIN_URL ?= https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/$(ARM_TOOLCHAIN_ARCHIVE)

ifndef BUILD_TOOLS_DIR
ifndef BUILD_DIR
  $(error BUILD_TOOLS_DIR or BUILD_DIR must be set before including arm_build_tools.mk)
endif

BUILD_TOOLS_DIR := $(BUILD_DIR)/tools
endif

TOOLCHAIN_DIR := $(BUILD_TOOLS_DIR)/$(ARM_TOOLCHAIN_PREFIX)-$(ARM_TOOLCHAIN_VERSION)
export PATH := $(abspath $(TOOLCHAIN_DIR)/bin):$(PATH)

arm_tools_install: $(TOOLCHAIN_DIR)

$(TOOLCHAIN_DIR):
	$(MAKE) arm_tools_uninstall
	@set -eu; \
	mkdir -p "$(BUILD_TOOLS_DIR)/dist"; \
	if [ ! -d "$(TOOLCHAIN_DIR)" ]; then \
	  echo "Downloading $(ARM_TOOLCHAIN_URL)"; \
	  curl -fL --retry 3 -z "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" -o "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" "$(ARM_TOOLCHAIN_URL)"; \
	  echo "Unpacking $(ARM_TOOLCHAIN_ARCHIVE)"; \
	  tar -xjf "$(BUILD_TOOLS_DIR)/dist/$(ARM_TOOLCHAIN_ARCHIVE)" -C "$(BUILD_TOOLS_DIR)"; \
	fi; \

arm_tools_version:
	@echo $(ARM_TOOLCHAIN_VERSION)

arm_tools_uninstall:
	$(RM) -rf "$(TOOLCHAIN_DIR)"

distclean: arm_tools_uninstall
	$(RM) -rf "$(BUILD_TOOLS_DIR)/dist"

.PHONY: arm_tools_install arm_tools_version arm_tools_uninstall distclean
