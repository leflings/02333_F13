# The following variable hold optimization options
OPTIMIZATIONFLAGS ?= -O3

# The following variable holds compiler options
CFLAGS = -pedantic -msoft-float -fno-exceptions -fno-common -g -ggdb

# The following variable holds the path to the generated kernel image
KERNEL := "${PWD}/objects/kernel"

include ../../bochs/Makefile.mk

$(KERNEL) : objects/boot32.o objects/relocate.o objects/kernel64.o src/link32.ld | objects
	x86_64-unknown-elf-ld  --no-warn-mismatch -z max-page-size=4096 -Tsrc/link32.ld -o objects/kernel objects/boot32.o objects/relocate.o objects/kernel64.o

objects/kernel64.o: objects/kernel64 | objects
	x86_64-unknown-elf-objcopy -I binary -O elf32-i386 -B i386 \
         --set-section-flags .data=alloc,contents,load,readonly,data \
         objects/kernel64 objects/kernel64.o

objects/kernel64: objects/boot64.o objects/enter.o objects/kernel.o objects/syscall.o src/link64.ld | objects
	x86_64-unknown-elf-ld  -z max-page-size=4096 -Tsrc/link64.ld -o objects/kernel64 objects/boot64.o objects/enter.o objects/kernel.o objects/syscall.o

objects/boot32.o: src/boot32.s | objects
	x86_64-unknown-elf-as --32 -o objects/boot32.o src/boot32.s

objects/relocate.o: src/relocate.s | objects
	x86_64-unknown-elf-as --64 -o objects/relocate.o src/relocate.s

objects/boot64.o: src/boot64.s | objects
	x86_64-unknown-elf-as --64 -o objects/boot64.o src/boot64.s

objects/enter.o: src/enter.s | objects
	x86_64-unknown-elf-as --64 -o objects/enter.o src/enter.s

objects/kernel.o: src/kernel.c src/kernel.h | objects
	x86_64-unknown-elf-gcc -m64 $(CFLAGS) $(OPTIMIZATIONFLAGS) -c -o objects/kernel.o src/kernel.c

objects/syscall.o: src/syscall.c src/kernel.h | objects
	x86_64-unknown-elf-gcc -m64 $(CFLAGS) $(OPTIMIZATIONFLAGS) -c -o objects/syscall.o src/syscall.c

objects:
	mkdir objects

clean:
	-rm -rf objects

compile: $(KERNEL)

all: boot
