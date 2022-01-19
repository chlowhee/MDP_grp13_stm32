################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../PeripheralDrivers/src/oled.c 

OBJS += \
./PeripheralDrivers/src/oled.o 

C_DEPS += \
./PeripheralDrivers/src/oled.d 


# Each subdirectory must supply rules for building sources it contributes
PeripheralDrivers/src/%.o: ../PeripheralDrivers/src/%.c PeripheralDrivers/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I"D:/MDPWorkSpace/MDP_grp13/PeripheralDrivers/inc" -I"D:/MDPWorkSpace/MDP_grp13/PeripheralDrivers" -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-PeripheralDrivers-2f-src

clean-PeripheralDrivers-2f-src:
	-$(RM) ./PeripheralDrivers/src/oled.d ./PeripheralDrivers/src/oled.o

.PHONY: clean-PeripheralDrivers-2f-src

