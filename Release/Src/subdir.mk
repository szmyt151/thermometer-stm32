################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/dallas.c \
../Src/flash_db.c \
../Src/main.c \
../Src/onewire.c \
../Src/opendrain.c \
../Src/ram_db.c \
../Src/serial.c \
../Src/stm32f1xx_hal_msp.c \
../Src/stm32f1xx_it.c \
../Src/system_stm32f1xx.c 

OBJS += \
./Src/dallas.o \
./Src/flash_db.o \
./Src/main.o \
./Src/onewire.o \
./Src/opendrain.o \
./Src/ram_db.o \
./Src/serial.o \
./Src/stm32f1xx_hal_msp.o \
./Src/stm32f1xx_it.o \
./Src/system_stm32f1xx.o 

C_DEPS += \
./Src/dallas.d \
./Src/flash_db.d \
./Src/main.d \
./Src/onewire.d \
./Src/opendrain.d \
./Src/ram_db.d \
./Src/serial.d \
./Src/stm32f1xx_hal_msp.d \
./Src/stm32f1xx_it.d \
./Src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F103xB -I"D:/STM32_F103_Dallas/Inc" -I"D:/STM32_F103_Dallas/Drivers/STM32F1xx_HAL_Driver/Inc" -I"D:/STM32_F103_Dallas/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"D:/STM32_F103_Dallas/Drivers/CMSIS/Device/ST/STM32F1xx/Include" -I"D:/STM32_F103_Dallas/Drivers/CMSIS/Include"  -Og -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


