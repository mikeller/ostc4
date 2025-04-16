################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/mikeller/git/ostc4/Common/Src/calc_crush.c \
/home/mikeller/git/ostc4/Common/Src/decom.c 

OBJS += \
./Common/Src/calc_crush.o \
./Common/Src/decom.o 

C_DEPS += \
./Common/Src/calc_crush.d \
./Common/Src/decom.d 


# Each subdirectory must supply rules for building sources it contributes
Common/Src/calc_crush.o: /home/mikeller/git/ostc4/Common/Src/calc_crush.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"/home/mikeller/git/ostc4/Small_CPU/Inc" -I"/home/mikeller/git/ostc4/Discovery/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx/Include" -I"/home/mikeller/git/ostc4/Common/Drivers/CMSIS/Include" -I"/home/mikeller/git/ostc4/Common/Inc" -O2 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Common/Src/decom.o: /home/mikeller/git/ostc4/Common/Src/decom.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F411xE -I"/home/mikeller/git/ostc4/Small_CPU/Inc" -I"/home/mikeller/git/ostc4/Discovery/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/mikeller/git/ostc4/Common/Drivers/STM32F4xx/Include" -I"/home/mikeller/git/ostc4/Common/Drivers/CMSIS/Include" -I"/home/mikeller/git/ostc4/Common/Inc" -O2 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


