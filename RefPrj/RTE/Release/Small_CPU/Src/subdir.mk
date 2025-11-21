################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
$(BUILD_DIR)/Small_CPU/Src/GNSS.c \
$(BUILD_DIR)/Small_CPU/Src/RTE_FlashAccess.c \
$(BUILD_DIR)/Small_CPU/Src/adc.c \
$(BUILD_DIR)/Small_CPU/Src/baseCPU2.c \
$(BUILD_DIR)/Small_CPU/Src/batteryCharger.c \
$(BUILD_DIR)/Small_CPU/Src/batteryGasGauge.c \
$(BUILD_DIR)/Small_CPU/Src/compass.c \
$(BUILD_DIR)/Small_CPU/Src/dma.c \
$(BUILD_DIR)/Small_CPU/Src/externalInterface.c \
$(BUILD_DIR)/Small_CPU/Src/gpio.c \
$(BUILD_DIR)/Small_CPU/Src/i2c.c \
$(BUILD_DIR)/Small_CPU/Src/pressure.c \
$(BUILD_DIR)/Small_CPU/Src/rtc.c \
$(BUILD_DIR)/Small_CPU/Src/scheduler.c \
$(BUILD_DIR)/Small_CPU/Src/spi.c \
$(BUILD_DIR)/Small_CPU/Src/stm32f4xx_hal_msp_v3.c \
$(BUILD_DIR)/Small_CPU/Src/stm32f4xx_it_v3.c \
$(BUILD_DIR)/Small_CPU/Src/system_stm32f4xx.c \
$(BUILD_DIR)/Small_CPU/Src/tm_stm32f4_otp.c \
$(BUILD_DIR)/Small_CPU/Src/uart.c \
$(BUILD_DIR)/Small_CPU/Src/uartProtocol_Co2.c \
$(BUILD_DIR)/Small_CPU/Src/uartProtocol_GNSS.c \
$(BUILD_DIR)/Small_CPU/Src/uartProtocol_O2.c \
$(BUILD_DIR)/Small_CPU/Src/uartProtocol_Sentinel.c \
$(BUILD_DIR)/Small_CPU/Src/uart_Internal.c 

S_UPPER_SRCS += \
$(BUILD_DIR)/Small_CPU/Src/startup_stm32f4xx.S 

OBJS += \
./Small_CPU/Src/GNSS.o \
./Small_CPU/Src/RTE_FlashAccess.o \
./Small_CPU/Src/adc.o \
./Small_CPU/Src/baseCPU2.o \
./Small_CPU/Src/batteryCharger.o \
./Small_CPU/Src/batteryGasGauge.o \
./Small_CPU/Src/compass.o \
./Small_CPU/Src/dma.o \
./Small_CPU/Src/externalInterface.o \
./Small_CPU/Src/gpio.o \
./Small_CPU/Src/i2c.o \
./Small_CPU/Src/pressure.o \
./Small_CPU/Src/rtc.o \
./Small_CPU/Src/scheduler.o \
./Small_CPU/Src/spi.o \
./Small_CPU/Src/startup_stm32f4xx.o \
./Small_CPU/Src/stm32f4xx_hal_msp_v3.o \
./Small_CPU/Src/stm32f4xx_it_v3.o \
./Small_CPU/Src/system_stm32f4xx.o \
./Small_CPU/Src/tm_stm32f4_otp.o \
./Small_CPU/Src/uart.o \
./Small_CPU/Src/uartProtocol_Co2.o \
./Small_CPU/Src/uartProtocol_GNSS.o \
./Small_CPU/Src/uartProtocol_O2.o \
./Small_CPU/Src/uartProtocol_Sentinel.o \
./Small_CPU/Src/uart_Internal.o 

S_UPPER_DEPS += \
./Small_CPU/Src/startup_stm32f4xx.d 

C_DEPS += \
./Small_CPU/Src/GNSS.d \
./Small_CPU/Src/RTE_FlashAccess.d \
./Small_CPU/Src/adc.d \
./Small_CPU/Src/baseCPU2.d \
./Small_CPU/Src/batteryCharger.d \
./Small_CPU/Src/batteryGasGauge.d \
./Small_CPU/Src/compass.d \
./Small_CPU/Src/dma.d \
./Small_CPU/Src/externalInterface.d \
./Small_CPU/Src/gpio.d \
./Small_CPU/Src/i2c.d \
./Small_CPU/Src/pressure.d \
./Small_CPU/Src/rtc.d \
./Small_CPU/Src/scheduler.d \
./Small_CPU/Src/spi.d \
./Small_CPU/Src/stm32f4xx_hal_msp_v3.d \
./Small_CPU/Src/stm32f4xx_it_v3.d \
./Small_CPU/Src/system_stm32f4xx.d \
./Small_CPU/Src/tm_stm32f4_otp.d \
./Small_CPU/Src/uart.d \
./Small_CPU/Src/uartProtocol_Co2.d \
./Small_CPU/Src/uartProtocol_GNSS.d \
./Small_CPU/Src/uartProtocol_O2.d \
./Small_CPU/Src/uartProtocol_Sentinel.d \
./Small_CPU/Src/uart_Internal.d 


# Each subdirectory must supply rules for building sources it contributes
Small_CPU/Src/GNSS.o: $(BUILD_DIR)/Small_CPU/Src/GNSS.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/RTE_FlashAccess.o: $(BUILD_DIR)/Small_CPU/Src/RTE_FlashAccess.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/adc.o: $(BUILD_DIR)/Small_CPU/Src/adc.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/baseCPU2.o: $(BUILD_DIR)/Small_CPU/Src/baseCPU2.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/batteryCharger.o: $(BUILD_DIR)/Small_CPU/Src/batteryCharger.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/batteryGasGauge.o: $(BUILD_DIR)/Small_CPU/Src/batteryGasGauge.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/compass.o: $(BUILD_DIR)/Small_CPU/Src/compass.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/dma.o: $(BUILD_DIR)/Small_CPU/Src/dma.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/externalInterface.o: $(BUILD_DIR)/Small_CPU/Src/externalInterface.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/gpio.o: $(BUILD_DIR)/Small_CPU/Src/gpio.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/i2c.o: $(BUILD_DIR)/Small_CPU/Src/i2c.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/pressure.o: $(BUILD_DIR)/Small_CPU/Src/pressure.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/rtc.o: $(BUILD_DIR)/Small_CPU/Src/rtc.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/scheduler.o: $(BUILD_DIR)/Small_CPU/Src/scheduler.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/spi.o: $(BUILD_DIR)/Small_CPU/Src/spi.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/startup_stm32f4xx.o: $(BUILD_DIR)/Small_CPU/Src/startup_stm32f4xx.S
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/stm32f4xx_hal_msp_v3.o: $(BUILD_DIR)/Small_CPU/Src/stm32f4xx_hal_msp_v3.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/stm32f4xx_it_v3.o: $(BUILD_DIR)/Small_CPU/Src/stm32f4xx_it_v3.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/system_stm32f4xx.o: $(BUILD_DIR)/Small_CPU/Src/system_stm32f4xx.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/tm_stm32f4_otp.o: $(BUILD_DIR)/Small_CPU/Src/tm_stm32f4_otp.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uart.o: $(BUILD_DIR)/Small_CPU/Src/uart.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uartProtocol_Co2.o: $(BUILD_DIR)/Small_CPU/Src/uartProtocol_Co2.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uartProtocol_GNSS.o: $(BUILD_DIR)/Small_CPU/Src/uartProtocol_GNSS.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uartProtocol_O2.o: $(BUILD_DIR)/Small_CPU/Src/uartProtocol_O2.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uartProtocol_Sentinel.o: $(BUILD_DIR)/Small_CPU/Src/uartProtocol_Sentinel.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Small_CPU/Src/uart_Internal.o: $(BUILD_DIR)/Small_CPU/Src/uart_Internal.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -ffunction-sections -fdata-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


