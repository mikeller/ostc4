################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
$(BUILD_DIR)/Common/Drivers/Src/syscalls.c 

OBJS += \
./Common/Drivers/Src/syscalls.o 

C_DEPS += \
./Common/Drivers/Src/syscalls.d 


# Each subdirectory must supply rules for building sources it contributes
Common/Drivers/Src/syscalls.o: $(BUILD_DIR)/Common/Drivers/Src/syscalls.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


