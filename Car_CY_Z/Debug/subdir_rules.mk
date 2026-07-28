################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System/MSPM0" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver/OLED" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver" -I"E:/MCU/MSP/My_WorkSpace/Car_Test" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Debug" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/third_party/CMSIS/Core/Include" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1437949074: ../empty.syscfg
	@echo 'SysConfig - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/utils/sysconfig_1.28.0/sysconfig_cli.bat" -s "D:/DOWDLOAD/CCSTUDIO/Sample_Code/.metadata/product.json" --script "E:/MCU/MSP/My_WorkSpace/Car_Test/empty.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-1437949074 ../empty.syscfg
device.opt: build-1437949074
device.cmd.genlibs: build-1437949074
ti_msp_dl_config.c: build-1437949074
ti_msp_dl_config.h: build-1437949074
Event.dot: build-1437949074

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System/MSPM0" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver/OLED" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver" -I"E:/MCU/MSP/My_WorkSpace/Car_Test" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Debug" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/third_party/CMSIS/Core/Include" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System/MSPM0" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/System" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver/OLED" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Driver" -I"E:/MCU/MSP/My_WorkSpace/Car_Test" -I"E:/MCU/MSP/My_WorkSpace/Car_Test/Debug" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source/third_party/CMSIS/Core/Include" -I"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


