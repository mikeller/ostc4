################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
$(OSTC4_BUILD_DIR)/Discovery/Src/startup_stm32f429xx.s 

C_SRCS += \
$(OSTC4_BUILD_DIR)/Discovery/Src/base.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/buehlmann.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/check_warning.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/crcmodel.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/data_central.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/data_exchange_main.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/demo.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/display.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/externCPU2bootloader.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/externLogbookFlash.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/firmwareJumpToApplication.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/gfx_colors.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/gfx_engine.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/gfx_fonts.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/logbook.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/logbook_miniLive.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/motion.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/ostc.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/settings.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/show_logbook.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/simulation.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/stm32f4xx_hal_msp_hw2.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/stm32f4xx_it.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/system_stm32f4xx_special_plus_256k.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/t3.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/t4_tetris.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/t5_gauge.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/t6_apnea.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/t7.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tCCR.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tComm.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tDebug.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tHome.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tInfo.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tInfoCompass.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tInfoLog.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tInfoPreDive.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tInfoSensor.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenu.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuCustom.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuCvOption.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuDeco.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuDecoParameter.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEdit.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditCustom.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditCvOption.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditDeco.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditDecoParameter.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditGasOC.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditHardware.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditPlanner.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditSetpoint.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditSystem.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditXtra.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuGas.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuHardware.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuPlanner.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuSetpoint.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuSystem.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/tMenuXtra.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/test_vpm.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/text_multilanguage.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/timer.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/unit.c \
$(OSTC4_BUILD_DIR)/Discovery/Src/vpm.c 

OBJS += \
./Discovery/Src/base.o \
./Discovery/Src/buehlmann.o \
./Discovery/Src/check_warning.o \
./Discovery/Src/crcmodel.o \
./Discovery/Src/data_central.o \
./Discovery/Src/data_exchange_main.o \
./Discovery/Src/demo.o \
./Discovery/Src/display.o \
./Discovery/Src/externCPU2bootloader.o \
./Discovery/Src/externLogbookFlash.o \
./Discovery/Src/firmwareJumpToApplication.o \
./Discovery/Src/gfx_colors.o \
./Discovery/Src/gfx_engine.o \
./Discovery/Src/gfx_fonts.o \
./Discovery/Src/logbook.o \
./Discovery/Src/logbook_miniLive.o \
./Discovery/Src/motion.o \
./Discovery/Src/ostc.o \
./Discovery/Src/settings.o \
./Discovery/Src/show_logbook.o \
./Discovery/Src/simulation.o \
./Discovery/Src/startup_stm32f429xx.o \
./Discovery/Src/stm32f4xx_hal_msp_hw2.o \
./Discovery/Src/stm32f4xx_it.o \
./Discovery/Src/system_stm32f4xx_special_plus_256k.o \
./Discovery/Src/t3.o \
./Discovery/Src/t4_tetris.o \
./Discovery/Src/t5_gauge.o \
./Discovery/Src/t6_apnea.o \
./Discovery/Src/t7.o \
./Discovery/Src/tCCR.o \
./Discovery/Src/tComm.o \
./Discovery/Src/tDebug.o \
./Discovery/Src/tHome.o \
./Discovery/Src/tInfo.o \
./Discovery/Src/tInfoCompass.o \
./Discovery/Src/tInfoLog.o \
./Discovery/Src/tInfoPreDive.o \
./Discovery/Src/tInfoSensor.o \
./Discovery/Src/tMenu.o \
./Discovery/Src/tMenuCustom.o \
./Discovery/Src/tMenuCvOption.o \
./Discovery/Src/tMenuDeco.o \
./Discovery/Src/tMenuDecoParameter.o \
./Discovery/Src/tMenuEdit.o \
./Discovery/Src/tMenuEditCustom.o \
./Discovery/Src/tMenuEditCvOption.o \
./Discovery/Src/tMenuEditDeco.o \
./Discovery/Src/tMenuEditDecoParameter.o \
./Discovery/Src/tMenuEditGasOC.o \
./Discovery/Src/tMenuEditHardware.o \
./Discovery/Src/tMenuEditPlanner.o \
./Discovery/Src/tMenuEditSetpoint.o \
./Discovery/Src/tMenuEditSystem.o \
./Discovery/Src/tMenuEditXtra.o \
./Discovery/Src/tMenuGas.o \
./Discovery/Src/tMenuHardware.o \
./Discovery/Src/tMenuPlanner.o \
./Discovery/Src/tMenuSetpoint.o \
./Discovery/Src/tMenuSystem.o \
./Discovery/Src/tMenuXtra.o \
./Discovery/Src/test_vpm.o \
./Discovery/Src/text_multilanguage.o \
./Discovery/Src/timer.o \
./Discovery/Src/unit.o \
./Discovery/Src/vpm.o 

C_DEPS += \
./Discovery/Src/base.d \
./Discovery/Src/buehlmann.d \
./Discovery/Src/check_warning.d \
./Discovery/Src/crcmodel.d \
./Discovery/Src/data_central.d \
./Discovery/Src/data_exchange_main.d \
./Discovery/Src/demo.d \
./Discovery/Src/display.d \
./Discovery/Src/externCPU2bootloader.d \
./Discovery/Src/externLogbookFlash.d \
./Discovery/Src/firmwareJumpToApplication.d \
./Discovery/Src/gfx_colors.d \
./Discovery/Src/gfx_engine.d \
./Discovery/Src/gfx_fonts.d \
./Discovery/Src/logbook.d \
./Discovery/Src/logbook_miniLive.d \
./Discovery/Src/motion.d \
./Discovery/Src/ostc.d \
./Discovery/Src/settings.d \
./Discovery/Src/show_logbook.d \
./Discovery/Src/simulation.d \
./Discovery/Src/stm32f4xx_hal_msp_hw2.d \
./Discovery/Src/stm32f4xx_it.d \
./Discovery/Src/system_stm32f4xx_special_plus_256k.d \
./Discovery/Src/t3.d \
./Discovery/Src/t4_tetris.d \
./Discovery/Src/t5_gauge.d \
./Discovery/Src/t6_apnea.d \
./Discovery/Src/t7.d \
./Discovery/Src/tCCR.d \
./Discovery/Src/tComm.d \
./Discovery/Src/tDebug.d \
./Discovery/Src/tHome.d \
./Discovery/Src/tInfo.d \
./Discovery/Src/tInfoCompass.d \
./Discovery/Src/tInfoLog.d \
./Discovery/Src/tInfoPreDive.d \
./Discovery/Src/tInfoSensor.d \
./Discovery/Src/tMenu.d \
./Discovery/Src/tMenuCustom.d \
./Discovery/Src/tMenuCvOption.d \
./Discovery/Src/tMenuDeco.d \
./Discovery/Src/tMenuDecoParameter.d \
./Discovery/Src/tMenuEdit.d \
./Discovery/Src/tMenuEditCustom.d \
./Discovery/Src/tMenuEditCvOption.d \
./Discovery/Src/tMenuEditDeco.d \
./Discovery/Src/tMenuEditDecoParameter.d \
./Discovery/Src/tMenuEditGasOC.d \
./Discovery/Src/tMenuEditHardware.d \
./Discovery/Src/tMenuEditPlanner.d \
./Discovery/Src/tMenuEditSetpoint.d \
./Discovery/Src/tMenuEditSystem.d \
./Discovery/Src/tMenuEditXtra.d \
./Discovery/Src/tMenuGas.d \
./Discovery/Src/tMenuHardware.d \
./Discovery/Src/tMenuPlanner.d \
./Discovery/Src/tMenuSetpoint.d \
./Discovery/Src/tMenuSystem.d \
./Discovery/Src/tMenuXtra.d \
./Discovery/Src/test_vpm.d \
./Discovery/Src/text_multilanguage.d \
./Discovery/Src/timer.d \
./Discovery/Src/unit.d \
./Discovery/Src/vpm.d 


# Each subdirectory must supply rules for building sources it contributes
Discovery/Src/base.o: $(OSTC4_BUILD_DIR)/Discovery/Src/base.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/buehlmann.o: $(OSTC4_BUILD_DIR)/Discovery/Src/buehlmann.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/check_warning.o: $(OSTC4_BUILD_DIR)/Discovery/Src/check_warning.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/crcmodel.o: $(OSTC4_BUILD_DIR)/Discovery/Src/crcmodel.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/data_central.o: $(OSTC4_BUILD_DIR)/Discovery/Src/data_central.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/data_exchange_main.o: $(OSTC4_BUILD_DIR)/Discovery/Src/data_exchange_main.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/demo.o: $(OSTC4_BUILD_DIR)/Discovery/Src/demo.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/display.o: $(OSTC4_BUILD_DIR)/Discovery/Src/display.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/externCPU2bootloader.o: $(OSTC4_BUILD_DIR)/Discovery/Src/externCPU2bootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/externLogbookFlash.o: $(OSTC4_BUILD_DIR)/Discovery/Src/externLogbookFlash.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/firmwareJumpToApplication.o: $(OSTC4_BUILD_DIR)/Discovery/Src/firmwareJumpToApplication.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/gfx_colors.o: $(OSTC4_BUILD_DIR)/Discovery/Src/gfx_colors.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/gfx_engine.o: $(OSTC4_BUILD_DIR)/Discovery/Src/gfx_engine.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/gfx_fonts.o: $(OSTC4_BUILD_DIR)/Discovery/Src/gfx_fonts.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/logbook.o: $(OSTC4_BUILD_DIR)/Discovery/Src/logbook.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/logbook_miniLive.o: $(OSTC4_BUILD_DIR)/Discovery/Src/logbook_miniLive.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/motion.o: $(OSTC4_BUILD_DIR)/Discovery/Src/motion.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/ostc.o: $(OSTC4_BUILD_DIR)/Discovery/Src/ostc.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/settings.o: $(OSTC4_BUILD_DIR)/Discovery/Src/settings.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/show_logbook.o: $(OSTC4_BUILD_DIR)/Discovery/Src/show_logbook.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/simulation.o: $(OSTC4_BUILD_DIR)/Discovery/Src/simulation.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/startup_stm32f429xx.o: $(OSTC4_BUILD_DIR)/Discovery/Src/startup_stm32f429xx.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/stm32f4xx_hal_msp_hw2.o: $(OSTC4_BUILD_DIR)/Discovery/Src/stm32f4xx_hal_msp_hw2.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/stm32f4xx_it.o: $(OSTC4_BUILD_DIR)/Discovery/Src/stm32f4xx_it.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/system_stm32f4xx_special_plus_256k.o: $(OSTC4_BUILD_DIR)/Discovery/Src/system_stm32f4xx_special_plus_256k.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/t3.o: $(OSTC4_BUILD_DIR)/Discovery/Src/t3.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/t4_tetris.o: $(OSTC4_BUILD_DIR)/Discovery/Src/t4_tetris.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/t5_gauge.o: $(OSTC4_BUILD_DIR)/Discovery/Src/t5_gauge.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/t6_apnea.o: $(OSTC4_BUILD_DIR)/Discovery/Src/t6_apnea.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/t7.o: $(OSTC4_BUILD_DIR)/Discovery/Src/t7.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tCCR.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tCCR.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tComm.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tComm.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tDebug.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tDebug.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tHome.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tHome.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tInfo.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tInfo.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tInfoCompass.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tInfoCompass.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tInfoLog.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tInfoLog.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tInfoPreDive.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tInfoPreDive.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tInfoSensor.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tInfoSensor.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenu.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenu.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuCustom.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuCustom.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuCvOption.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuCvOption.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuDeco.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuDeco.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuDecoParameter.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuDecoParameter.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEdit.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEdit.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditCustom.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditCustom.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditCvOption.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditCvOption.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditDeco.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditDeco.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditDecoParameter.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditDecoParameter.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditGasOC.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditGasOC.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditHardware.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditHardware.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditPlanner.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditPlanner.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditSetpoint.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditSetpoint.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditSystem.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditSystem.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuEditXtra.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuEditXtra.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuGas.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuGas.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuHardware.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuHardware.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuPlanner.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuPlanner.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuSetpoint.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuSetpoint.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuSystem.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuSystem.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/tMenuXtra.o: $(OSTC4_BUILD_DIR)/Discovery/Src/tMenuXtra.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/test_vpm.o: $(OSTC4_BUILD_DIR)/Discovery/Src/test_vpm.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/text_multilanguage.o: $(OSTC4_BUILD_DIR)/Discovery/Src/text_multilanguage.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/timer.o: $(OSTC4_BUILD_DIR)/Discovery/Src/timer.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/unit.o: $(OSTC4_BUILD_DIR)/Discovery/Src/unit.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Discovery/Src/vpm.o: $(OSTC4_BUILD_DIR)/Discovery/Src/vpm.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(OSTC4_BUILD_DIR)/Discovery/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(OSTC4_BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(OSTC4_BUILD_DIR)/Common/Inc" -O2 -Wall -fmessage-length=0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


