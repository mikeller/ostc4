################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
/home/mikeller/git/ostc4/FontPack/startup_stm32f429xx.s 

C_SRCS += \
/home/mikeller/git/ostc4/FontPack/base_upperRegion.c 

OBJS += \
./src/base_upperRegion.o \
./src/startup_stm32f429xx.o 

C_DEPS += \
./src/base_upperRegion.d 


# Each subdirectory must supply rules for building sources it contributes
src/base_upperRegion.o: /home/mikeller/git/ostc4/FontPack/base_upperRegion.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DBUILD_LIBRARY -DSTM32 -DSTM32F4 -DSTM32F429IITx -I"/home/mikeller/git/ostc4/Discovery/Inc" -I"/home/mikeller/git/ostc4/Common/Inc" -Os -Wall -fmessage-length=0 -u symbol -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/startup_stm32f429xx.o: /home/mikeller/git/ostc4/FontPack/startup_stm32f429xx.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


