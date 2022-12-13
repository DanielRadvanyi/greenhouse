################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/DigitalIoPin.cpp \
../src/Fmutex.cpp \
../src/Greenhouse.cpp \
../src/I2C.cpp \
../src/LiquidCrystal.cpp \
../src/LpcUart.cpp \
../src/ModbusMaster.cpp \
../src/ModbusRegister.cpp \
../src/SerialPort.cpp \
../src/cr_cpp_config.cpp \
../src/cr_startup_lpc15xx.cpp \
../src/retarget_uart.cpp \
../src/serial_port.cpp 

C_SRCS += \
../src/PlaintextMQTTExample.c \
../src/backoff_algorithm.c \
../src/core_mqtt.c \
../src/core_mqtt_serializer.c \
../src/crp.c \
../src/esp8266_socket.c \
../src/heap_lock_monitor.c \
../src/sysinit.c \
../src/using_plaintext.c 

OBJS += \
./src/DigitalIoPin.o \
./src/Fmutex.o \
./src/Greenhouse.o \
./src/I2C.o \
./src/LiquidCrystal.o \
./src/LpcUart.o \
./src/ModbusMaster.o \
./src/ModbusRegister.o \
./src/PlaintextMQTTExample.o \
./src/SerialPort.o \
./src/backoff_algorithm.o \
./src/core_mqtt.o \
./src/core_mqtt_serializer.o \
./src/cr_cpp_config.o \
./src/cr_startup_lpc15xx.o \
./src/crp.o \
./src/esp8266_socket.o \
./src/heap_lock_monitor.o \
./src/retarget_uart.o \
./src/serial_port.o \
./src/sysinit.o \
./src/using_plaintext.o 

CPP_DEPS += \
./src/DigitalIoPin.d \
./src/Fmutex.d \
./src/Greenhouse.d \
./src/I2C.d \
./src/LiquidCrystal.d \
./src/LpcUart.d \
./src/ModbusMaster.d \
./src/ModbusRegister.d \
./src/SerialPort.d \
./src/cr_cpp_config.d \
./src/cr_startup_lpc15xx.d \
./src/retarget_uart.d \
./src/serial_port.d 

C_DEPS += \
./src/PlaintextMQTTExample.d \
./src/backoff_algorithm.d \
./src/core_mqtt.d \
./src/core_mqtt_serializer.d \
./src/crp.d \
./src/esp8266_socket.d \
./src/heap_lock_monitor.d \
./src/sysinit.d \
./src/using_plaintext.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=c++11 -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\lpc_chip_15xx\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\src\include" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\src\portable\GCC\ARM_CM3" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m3 -mthumb -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c11 -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\lpc_chip_15xx\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\inc" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\src\include" -I"C:\Users\Administrator\OneDrive\Documents\Metropolia\HACKATHON\greenhouse_proj\FreeRTOS\src\portable\GCC\ARM_CM3" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m3 -mthumb -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


