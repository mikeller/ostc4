################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
#$(BUILD_DIR)/BootLoader/Src/startup_stm32f429xx.s 

C_SRCS += \
$(BUILD_DIR)/BootLoader/Src/base_bootlader.c \
$(BUILD_DIR)/BootLoader/Src/display_mini.c \
$(BUILD_DIR)/BootLoader/Src/externCPU2bootloader_mini.c \
$(BUILD_DIR)/BootLoader/Src/externLogbookFlash_mini.c \
$(BUILD_DIR)/BootLoader/Src/gfx_engine_mini.c \
$(BUILD_DIR)/BootLoader/Src/ostc_mini.c \
$(BUILD_DIR)/BootLoader/Src/settings_bootloader.c \
$(BUILD_DIR)/BootLoader/Src/tComm_mini.c \
$(BUILD_DIR)/BootLoader/Src/tInfoBootloader.c

OBJS += \
./BootLoader/Src/base_bootlader.o \
./BootLoader/Src/display_mini.o \
./BootLoader/Src/externCPU2bootloader_mini.o \
./BootLoader/Src/externLogbookFlash_mini.o \
./BootLoader/Src/gfx_engine_mini.o \
./BootLoader/Src/ostc_mini.o \
./BootLoader/Src/settings_bootloader.o \
./BootLoader/Src/tComm_mini.o \
./BootLoader/Src/tInfoBootloader.o

C_DEPS += \
./BootLoader/Src/base_bootlader.d \
./BootLoader/Src/display_mini.d \
./BootLoader/Src/externCPU2bootloader_mini.d \
./BootLoader/Src/externLogbookFlash_mini.d \
./BootLoader/Src/gfx_engine_mini.d \
./BootLoader/Src/ostc_mini.d \
./BootLoader/Src/settings_bootloader.d \
./BootLoader/Src/tComm_mini.d \
./BootLoader/Src/tInfoBootloader.d


# Each subdirectory must supply rules for building sources it contributes
BootLoader/Src/base_bootlader.o: $(BUILD_DIR)/BootLoader/Src/base_bootlader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/display_mini.o: $(BUILD_DIR)/BootLoader/Src/display_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/externCPU2bootloader_mini.o: $(BUILD_DIR)/BootLoader/Src/externCPU2bootloader_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/externLogbookFlash_mini.o: $(BUILD_DIR)/BootLoader/Src/externLogbookFlash_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/gfx_engine_mini.o: $(BUILD_DIR)/BootLoader/Src/gfx_engine_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/ostc_mini.o: $(BUILD_DIR)/BootLoader/Src/ostc_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/settings_bootloader.o: $(BUILD_DIR)/BootLoader/Src/settings_bootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/tComm_mini.o: $(BUILD_DIR)/BootLoader/Src/tComm_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/tInfoBootloader.o: $(BUILD_DIR)/BootLoader/Src/tInfoBootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
