all: OSTC4pack_V4 checksum_final_add_fletcher

clean:
	find . ! -type d ! -name Makefile ! -name \*.cpp ! -name \*.h -exec rm -f {} \;

checksum_final_add_fletcher: checksum_final_add_fletcher.o $(BUILD_DIR)/ostc4pack/src/settings.o
	gcc -dynamic-linker -Wl,--no-as-needed $^ -o "$@"

checksum_final_add_fletcher.o: checksum_final_add_fletcher.cpp
	g++ -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -DINCLUDE_VERSION_SUPPORT -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -c -o "$@" "$<"

$(BUILD_DIR)/ostc4pack/src/settings.o: $(BUILD_DIR)/Discovery/Src/settings.c
	gcc -DSTM32 -DSTM32F4 -DSTM32F429xx -DSTM32F429IITx -I"$(BUILD_DIR)/Discovery/Inc" -I"$(BUILD_DIR)/Common/Drivers/CMSIS/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx/Include" -I"$(BUILD_DIR)/Common/Drivers/STM32F4xx_HAL_Driver/Inc" -I"$(BUILD_DIR)/Common/Inc" -O2 -Wall -c -o "$@" "$<"

.PHONY: all clean
