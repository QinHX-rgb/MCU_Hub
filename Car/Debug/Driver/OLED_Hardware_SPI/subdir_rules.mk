################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Driver/OLED_Hardware_SPI/%.o: ../Driver/OLED_Hardware_SPI/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/34716/workspace_ccstheia/Car/Driver/WIT" -I"C:/Users/34716/workspace_ccstheia/Car/Driver/OLED_Hardware_SPI" -I"C:/Users/34716/workspace_ccstheia/Car/Driver/MSPM0" -I"C:/Users/34716/workspace_ccstheia/Car/System" -I"C:/Users/34716/workspace_ccstheia/Car" -I"C:/Users/34716/workspace_ccstheia/Car/Debug" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/third_party/CMSIS/Core/Include" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -g -Wall -MMD -MP -MF"Driver/OLED_Hardware_SPI/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


