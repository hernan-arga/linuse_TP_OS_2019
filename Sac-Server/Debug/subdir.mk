################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../conexionServer.c \
../estructuras.c \
../operaciones.c \
../sac-server.c 

OBJS += \
./conexionServer.o \
./estructuras.o \
./operaciones.o \
./sac-server.o 

C_DEPS += \
./conexionServer.d \
./estructuras.d \
./operaciones.d \
./sac-server.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


