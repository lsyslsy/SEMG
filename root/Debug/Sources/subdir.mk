################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../Sources/collect.c \
../Sources/led.c \
../Sources/main.c \
../Sources/mytime.c \
../Sources/socket.c \
../Sources/process.c

OBJS += \
./Sources/collect.o \
./Sources/led.o \
./Sources/main.o \
./Sources/mytime.o \
./Sources/socket.o  \
./Sources/process.o

C_DEPS += \
./Sources/collect.d \
./Sources/led.d \
./Sources/main.d \
./Sources/mytime.d \
./Sources/socket.d \
./Sources/process.d


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CCC) -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

