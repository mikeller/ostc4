################################################################################
# Copy of an automatically-generated file.
################################################################################

-include ../makefile.init

# All Target
all: libOSTC4_FontPack.a

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
libOSTC4_FontPack.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Archiver'
	arm-none-eabi-ar -r -P  "libOSTC4_FontPack.a" @"objects.list" $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-find . ! -type d ! -name Makefile ! -name \*.mk ! -name objects.list -exec rm -f {} \;
	-find . -mindepth 1 -type d | xargs  rmdir --ignore-fail-on-non-empty
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
