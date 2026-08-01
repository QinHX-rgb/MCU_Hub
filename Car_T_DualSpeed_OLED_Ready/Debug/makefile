################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS

GEN_OPTS__FLAG := @"./device.opt" 
GEN_CMDS__FLAG := -Wl,-l"./device_linker.cmd" 

ORDERED_OBJS += \
"./empty.o" \
"./ti_msp_dl_config.o" \
"./startup_mspm0g350x_ticlang.o" \
"./Driver/Encoder.o" \
"./Driver/Motor_Control.o" \
"./Driver/PID.o" \
"./Driver/grayscale_sensor.o" \
"./Driver/motor.o" \
"./Driver/OLED/OLED.o" \
"./System/Key.o" \
"./System/clock.o" \
"./System/delay.o" \
"./System/interrupt.o" \
"./System/uart.o" \
$(GEN_CMDS__FLAG) \
-Wl,-ldevice.cmd.genlibs \
-Wl,-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Driver/subdir_vars.mk
-include Driver/OLED/subdir_vars.mk
-include System/subdir_vars.mk
-include subdir_rules.mk
-include Driver/subdir_rules.mk
-include Driver/OLED/subdir_rules.mk
-include System/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
Car_T.out 

EXE_OUTPUTS__QUOTED += \
"Car_T.out" 


# All Target
all: $(OBJS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "Car_T.out"

# Tool invocations
Car_T.out: $(OBJS) $(GEN_CMDS)
	@echo 'Arm Linker - building target: "$@"'
	"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -g -Wall -Wl,-m"Car_T_DualSpeed_OLED_Ready.map" -Wl,-i"D:/DOWDLOAD/CCSTUDIO/Sample_Code/source" -Wl,-i"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready" -Wl,-i"E:/MCU/MSP/26NUEDC/Car_T_DualSpeed_OLED_Ready/Debug/syscfg" -Wl,-i"D:/DOWDLOAD/CCSTUDIO/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/lib" -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="Car_T_DualSpeed_OLED_Ready_linkInfo.xml" -Wl,--rom_model -o "Car_T.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(GEN_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "empty.o" "ti_msp_dl_config.o" "startup_mspm0g350x_ticlang.o" "Driver\Encoder.o" "Driver\Motor_Control.o" "Driver\PID.o" "Driver\grayscale_sensor.o" "Driver\motor.o" "Driver\OLED\OLED.o" "System\Key.o" "System\clock.o" "System\delay.o" "System\interrupt.o" "System\uart.o" 
	-$(RM) "empty.d" "ti_msp_dl_config.d" "startup_mspm0g350x_ticlang.d" "Driver\Encoder.d" "Driver\Motor_Control.d" "Driver\PID.d" "Driver\grayscale_sensor.d" "Driver\motor.d" "Driver\OLED\OLED.d" "System\Key.d" "System\clock.d" "System\delay.d" "System\interrupt.d" "System\uart.d" 
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

