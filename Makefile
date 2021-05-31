OBJECTS=main.o user_defined.o multi.elf
MAP=main.map
MAKEFILE=Makefile

ifeq ($(OS),Windows_NT)
	ifeq ($(shell uname -o),Cygwin)
		RM= rm -rf
	else
		RM= del /q
	endif
else
	RM= rm -rf
endif

GCC_DIR = /home/ruan_fc/msp430-gcc/bin
SUPPORT_FILE_DIRECTORY = /home/ruan_fc/msp430-gcc/include

DEVICE  = MSP430G2553
CC      = $(GCC_DIR)/msp430-elf-gcc
GDB     = $(GCC_DIR)/msp430-elf-gdb

CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -Og -Wall -g
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -Wl,-Map,$(MAP),--gc-sections
ASFLAGS = -D_GNU_ASSEMBLER_ -x assembler-with-cpp

%.elf: %.S
	$(CC) $(CFLAGS) $(ASFLAGS) -o $@ -c $?
	# $(CC) $(CFLAGS) $^ -c -o $*.o

all: ${OBJECTS}
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $(DEVICE).out

clean: 
	$(RM) $(OBJECTS)
	$(RM) $(MAP)
	$(RM) *.out
	
debug: all
	$(GDB) $(DEVICE).out
