################################################################################
# Copy of an automatically-generated file.
################################################################################

-include ../makefile.init

# All Target
all: OSTC4_RTE.elf

# All of the sources participating in the build are defined here
-include sources.mk
-include Small_CPU/Src/subdir.mk
-include Common/Src/subdir.mk
-include Common/Drivers/Src/subdir.mk
-include Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/subdir.mk
-include Common/Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
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
OSTC4_RTE.elf: $(OBJS) $(USER_OBJS) $(BUILD_DIR)/RefPrj/RTE/../../Small_CPU/CPU2-RTE.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Xlinker --print-memory-usage -T"$(BUILD_DIR)/Small_CPU/CPU2-RTE.ld" -Wl,-Map=output.map -Wl,--gc-sections -Wl,-z,muldefs -o "OSTC4_RTE.elf" @"objects.list" $(USER_OBJS) $(LIBS) -lm
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
	arm-none-eabi-objcopy -O binary "OSTC4_RTE.elf" "OSTC4_RTE.bin"
	arm-none-eabi-size -B "OSTC4_RTE.elf"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
