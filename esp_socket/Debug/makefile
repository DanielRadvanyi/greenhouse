################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/uart/subdir.mk
-include src/networking/subdir.mk
-include src/mqtt_demo/subdir.mk
-include src/modbus/subdir.mk
-include src/coreMQTT/source/subdir.mk
-include src/backoffAlgorithm/source/subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := esp_socket
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: esp_socket.axf

# Tool invocations
esp_socket.axf: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU C++ Linker'
	arm-none-eabi-c++ -nostdlib -L"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\lpc_board_nxp_lpcxpresso_1549\Debug" -L"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\lpc_chip_15xx\Debug" -L"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\FreeRTOS\Debug" -Xlinker -Map="esp_socket.map" -Xlinker --cref -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb -T "esp_socket_Debug.ld" -o "esp_socket.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) esp_socket.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "esp_socket.axf"; # arm-none-eabi-objcopy -v -O binary "esp_socket.axf" "esp_socket.bin" ; # checksum -p LPC1549 -d "esp_socket.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
