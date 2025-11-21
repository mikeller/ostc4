################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS +=

C_SRCS += \
$(BUILD_DIR)/BootLoader/Src/base_bootlader.c \
$(BUILD_DIR)/BootLoader/Src/display_mini.c \
$(BUILD_DIR)/BootLoader/Src/externCPU2bootloader_mini.c \
$(BUILD_DIR)/BootLoader/Src/externLogbookFlash_mini.c \
$(BUILD_DIR)/BootLoader/Src/gfx_engine_mini.c \
$(BUILD_DIR)/BootLoader/Src/ostc_mini.c \
$(BUILD_DIR)/BootLoader/Src/settings_bootloader.c \
$(BUILD_DIR)/BootLoader/Src/tComm_mini.c \
$(BUILD_DIR)/BootLoader/Src/tInfoBootloader.c \
$(BUILD_DIR)/OtherSources/firmwareEraseProgram.c \
$(BUILD_DIR)/OtherSources/data_central_mini.c \
$(BUILD_DIR)/OtherSources/data_exchange_main_mini.c \
$(BUILD_DIR)/OtherSources/stm32f4xx_hal_msp.c \
$(BUILD_DIR)/OtherSources/system_stm32f4xx.c

OBJS += \
./BootLoader/Src/base_bootlader.o \
./BootLoader/Src/display_mini.o \
./BootLoader/Src/externCPU2bootloader_mini.o \
./BootLoader/Src/externLogbookFlash_mini.o \
./BootLoader/Src/gfx_engine_mini.o \
./BootLoader/Src/ostc_mini.o \
./BootLoader/Src/settings_bootloader.o \
./BootLoader/Src/tComm_mini.o \
./BootLoader/Src/tInfoBootloader.o \
./BootLoader/Src/firmwareEraseProgram.o \
./BootLoader/Src/data_central_mini.o \
./BootLoader/Src/data_exchange_main_mini.o \
./BootLoader/Src/stm32f4xx_hal_msp.o \
./BootLoader/Src/system_stm32f4xx.o

C_DEPS += \
./BootLoader/Src/base_bootlader.d \
./BootLoader/Src/display_mini.d \
./BootLoader/Src/externCPU2bootloader_mini.d \
./BootLoader/Src/externLogbookFlash_mini.d \
./BootLoader/Src/gfx_engine_mini.d \
./BootLoader/Src/ostc_mini.d \
./BootLoader/Src/settings_bootloader.d \
./BootLoader/Src/tComm_mini.d \
./BootLoader/Src/tInfoBootloader.d \
./BootLoader/Src/firmwareEraseProgram.d \
./BootLoader/Src/data_central_mini.d \
./BootLoader/Src/data_exchange_main_mini.d \
./BootLoader/Src/stm32f4xx_hal_msp.d \
./BootLoader/Src/system_stm32f4xx.d


# Each subdirectory must supply rules for building sources it contributes
BootLoader/Src/base_bootlader.o: $(BUILD_DIR)/BootLoader/Src/base_bootlader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/display_mini.o: $(BUILD_DIR)/BootLoader/Src/display_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/externCPU2bootloader_mini.o: $(BUILD_DIR)/BootLoader/Src/externCPU2bootloader_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/externLogbookFlash_mini.o: $(BUILD_DIR)/BootLoader/Src/externLogbookFlash_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/gfx_engine_mini.o: $(BUILD_DIR)/BootLoader/Src/gfx_engine_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/ostc_mini.o: $(BUILD_DIR)/BootLoader/Src/ostc_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/settings_bootloader.o: $(BUILD_DIR)/BootLoader/Src/settings_bootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/tComm_mini.o: $(BUILD_DIR)/BootLoader/Src/tComm_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/tInfoBootloader.o: $(BUILD_DIR)/BootLoader/Src/tInfoBootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/firmwareEraseProgram.o: $(BUILD_DIR)/OtherSources/firmwareEraseProgram.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/data_central_mini.o: $(BUILD_DIR)/OtherSources/data_central_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/data_exchange_main_mini.o: $(BUILD_DIR)/OtherSources/data_exchange_main_mini.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/stm32f4xx_hal_msp.o: $(BUILD_DIR)/OtherSources/stm32f4xx_hal_msp.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

BootLoader/Src/system_stm32f4xx.o: $(BUILD_DIR)/OtherSources/system_stm32f4xx.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/BootLoader/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

