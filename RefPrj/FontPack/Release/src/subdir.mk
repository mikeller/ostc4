################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
$(BUILD_DIR)/FontPack/startup_stm32f429xx.s 

C_SRCS += \
$(BUILD_DIR)/FontPack/base_upperRegion.c 

OBJS += \
./src/base_upperRegion.o \
./src/startup_stm32f429xx.o 

C_DEPS += \
./src/base_upperRegion.d 


# Each subdirectory must supply rules for building sources it contributes
src/base_upperRegion.o: $(BUILD_DIR)/FontPack/base_upperRegion.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DBUILD_LIBRARY -DSTM32 -DSTM32F4 -DSTM32F429IITx -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Inc" -Os -Wall -ffunction-sections -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/startup_stm32f429xx.o: $(BUILD_DIR)/FontPack/startup_stm32f429xx.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


