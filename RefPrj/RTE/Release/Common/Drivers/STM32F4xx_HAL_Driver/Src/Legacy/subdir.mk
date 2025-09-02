################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.c 

OBJS += \
./Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.o 

C_DEPS += \
./Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.d 


# Each subdirectory must supply rules for building sources it contributes
Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.o: $(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"$(BUILD_DIR)/Small_CPU/Inc" -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


