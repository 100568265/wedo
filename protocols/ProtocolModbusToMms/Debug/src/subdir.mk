################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CModbusRTU.cpp 

OBJS += \
./src/CModbusRTU.o 

CPP_DEPS += \
./src/CModbusRTU.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-g++ -DMMS_LITE -I/opt/Mmslite.cpp/trunk/txjcode/includes -I/opt/Mmslite.cpp/trunk/inc -I/opt/Mmslite.cpp/trunk/mvl/usr/Lib61850 -I/opt/Mmslite.cpp/trunk/Mms.eclipse/ProtocolModbusToMms/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


