################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/coreMQTT/source/core_mqtt.c \
../src/coreMQTT/source/core_mqtt_serializer.c \
../src/coreMQTT/source/core_mqtt_state.c 

C_DEPS += \
./src/coreMQTT/source/core_mqtt.d \
./src/coreMQTT/source/core_mqtt_serializer.d \
./src/coreMQTT/source/core_mqtt_state.d 

OBJS += \
./src/coreMQTT/source/core_mqtt.o \
./src/coreMQTT/source/core_mqtt_serializer.o \
./src/coreMQTT/source/core_mqtt_state.o 


# Each subdirectory must supply rules for building sources it contributes
src/coreMQTT/source/%.o: ../src/coreMQTT/source/%.c src/coreMQTT/source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c11 -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\lpc_chip_15xx\inc" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\FreeRTOS\inc" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\FreeRTOS\src\include" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\FreeRTOS\src\portable\GCC\ARM_CM3" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\coreMQTT\source\include" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\coreMQTT\source\interface" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\backoffAlgorithm\source\include" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\networking" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\uart" -I"C:\Users\aivua\Documents\MCUXpressoIDE_11.6.0_8187\example\mqtt_freertos\esp_socket\src\modbus" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m3 -mthumb -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-coreMQTT-2f-source

clean-src-2f-coreMQTT-2f-source:
	-$(RM) ./src/coreMQTT/source/core_mqtt.d ./src/coreMQTT/source/core_mqtt.o ./src/coreMQTT/source/core_mqtt_serializer.d ./src/coreMQTT/source/core_mqtt_serializer.o ./src/coreMQTT/source/core_mqtt_state.d ./src/coreMQTT/source/core_mqtt_state.o

.PHONY: clean-src-2f-coreMQTT-2f-source

