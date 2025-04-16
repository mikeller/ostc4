################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.c 

OBJS += \
./Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.o 

C_DEPS += \
./Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.d 


# Each subdirectory must supply rules for building sources it contributes
Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.o: /home/mikeller/git/ostc4/Common/Drivers/STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_can.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"/home/mikeller/git/ostc4/Small_CPU/Inc" -I"/home/mikeller/git/ostc4/Discovery/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx/Include" -I"/home/mikeller/git/ostc4/Common/Drivers/CMSIS/Include" -I"/home/mikeller/git/ostc4/Common/Inc" -O2 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


