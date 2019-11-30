################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: PruebasConexion

dependents:
	-cd /home/utnso/Escritorio/tp-2019-2c-Cbados/Libmuse/Debug && $(MAKE) clean all

# Tool invocations
PruebasConexion: $(OBJS) $(USER_OBJS) /home/utnso/Escritorio/tp-2019-2c-Cbados/Libmuse/Debug/libLibmuse.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/Escritorio/tp-2019-2c-Cbados/Libmuse/Debug" -o "PruebasConexion" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) PruebasConexion
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/Escritorio/tp-2019-2c-Cbados/Libmuse/Debug/libLibmuse.so:

-include ../makefile.targets
