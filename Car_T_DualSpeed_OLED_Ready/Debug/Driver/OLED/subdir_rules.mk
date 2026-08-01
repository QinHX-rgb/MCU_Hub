################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Driver/OLED/%.o: ../Driver/OLED/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready/System" -I"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready/Driver/OLED" -I"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready/Driver" -I"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready" -I"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready/Debug" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/third_party/CMSIS/Core/Include" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -g -Wall -MMD -MP -MF"Driver/OLED/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


