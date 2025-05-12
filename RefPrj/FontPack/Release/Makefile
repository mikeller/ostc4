################################################################################
# Copy of an automatically-generated file.
################################################################################

-include ../makefile.init

# All Target
all: OSTC4_FontPack.elf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# Tool invocations
OSTC4_FontPack.elf: $(OBJS) $(USER_OBJS) $(OSTC4_BUILD_DIR)/RefPrj/FontPack/../../Common/CPU1-F429.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -nostartfiles -nodefaultlibs -nostdlib -L"$(OSTC4_BUILD_DIR)/RefPrj/FontPack/Library" -Xlinker --print-memory-usage -T"$(OSTC4_BUILD_DIR)/Common/CPU1-F429.ld" -Wl,-Map=output.map -o "OSTC4_FontPack.elf" @"objects.list" $(USER_OBJS) $(LIBS) -lOSTC4_FontPack
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-find . ! -type d ! -name Makefile ! -name \*.mk ! -name objects.list -exec rm -f {} \;
	-find . -mindepth 1 -type d | xargs  rmdir --ignore-fail-on-non-empty
	-@echo ' '

post-build:
	-@echo 'Generating binary and Printing size information:'
	arm-none-eabi-objcopy -j.font_firmware_data -j .upper* -O binary "OSTC4_FontPack.elf" "OSTC4_FontPack.bin"
	arm-none-eabi-size "OSTC4_FontPack.elf"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
