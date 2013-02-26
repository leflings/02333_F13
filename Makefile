# The following variable Optimization options
OPTIMIZATIONFLAGS ?= -O3

# The following variable holds compiler options
CFLAGS = -pedantic -msoft-float -fno-exceptions -fno-common -Isrc/include -g -ggdb

# The following variable holds the path to the generated kernel image
KERNEL := "${PWD}/objects/kernel/kernel.stripped"

include ../../bochs/Makefile.mk

src/include/scwrapper.h: src/include/sysdefines.h

src/kernel/kernel.h: src/include/sysdefines.h

objects/kernel/kernel: objects/kernel/boot32.o objects/kernel/relocate.o objects/kernel/kernel64.o src/kernel/link32.ld | objects/kernel
	x86_64-unknown-elf-ld  --no-warn-mismatch -z max-page-size=4096 -Tsrc/kernel/link32.ld -o objects/kernel/kernel objects/kernel/boot32.o objects/kernel/relocate.o objects/kernel/kernel64.o

$(KERNEL): objects/kernel/kernel | objects/kernel
	x86_64-unknown-elf-strip -o objects/kernel/kernel.stripped objects/kernel/kernel

objects/kernel/kernel64.o: objects/kernel/kernel64.stripped | objects/kernel
	x86_64-unknown-elf-objcopy -I binary -O elf32-i386 -B i386 --set-section-flags .data=alloc,contents,load,readonly,data objects/kernel/kernel64.stripped objects/kernel/kernel64.o

objects/kernel/kernel64.stripped: objects/kernel/kernel64 | objects/kernel
	x86_64-unknown-elf-strip -o objects/kernel/kernel64.stripped objects/kernel/kernel64

objects/kernel/kernel64: objects/kernel/boot64.o objects/kernel/enter.o objects/kernel/kernel.o objects/kernel/syscall.o objects/program_0/executable.o objects/program_1/executable.o objects/program_2/executable.o src/kernel/link64.ld | objects/kernel
	x86_64-unknown-elf-ld  -z max-page-size=4096 -Tsrc/kernel/link64.ld -o objects/kernel/kernel64 objects/kernel/boot64.o objects/kernel/enter.o objects/kernel/kernel.o objects/kernel/syscall.o objects/program_0/executable.o objects/program_1/executable.o objects/program_2/executable.o

objects/kernel/boot32.o: src/kernel/boot32.s | objects/kernel
	x86_64-unknown-elf-as --32 -o objects/kernel/boot32.o src/kernel/boot32.s

objects/kernel/relocate.o: src/kernel/relocate.s | objects/kernel
	x86_64-unknown-elf-as --64 -o objects/kernel/relocate.o src/kernel/relocate.s

objects/kernel/boot64.o: src/kernel/boot64.s  | objects/kernel
	x86_64-unknown-elf-as --64 -o objects/kernel/boot64.o src/kernel/boot64.s

objects/kernel/enter.o: src/kernel/enter.s | objects/kernel
	x86_64-unknown-elf-as --64 -o objects/kernel/enter.o src/kernel/enter.s

objects/kernel/kernel.o: src/kernel/kernel.c src/kernel/kernel.h | objects/kernel
	x86_64-unknown-elf-gcc -m64 $(CFLAGS) $(OPTIMIZATIONFLAGS) -c -o objects/kernel/kernel.o src/kernel/kernel.c

objects/kernel/syscall.o: src/kernel/syscall.c src/kernel/kernel.h | objects/kernel
	x86_64-unknown-elf-gcc -m64 $(CFLAGS) $(OPTIMIZATIONFLAGS) -c -o objects/kernel/syscall.o src/kernel/syscall.c

objects/program_startup_code/startup.o: src/program_startup_code/startup.s | objects/program_startup_code
	x86_64-unknown-elf-as --64 -o objects/program_startup_code/startup.o src/program_startup_code/startup.s

objects/program_0/main.o: src/program_0/main.c src/include/scwrapper.h | objects/program_0
	x86_64-unknown-elf-gcc -fPIE -m64 $(CFLAGS)  $(OPTIMIZATIONFLAGS) -c -o objects/program_0/main.o src/program_0/main.c

objects/program_0/executable: objects/program_startup_code/startup.o objects/program_0/main.o src/program_startup_code/program_link.ld | objects/program_0
	x86_64-unknown-elf-ld  -z max-page-size=4096 -static -Tsrc/program_startup_code/program_link.ld -o objects/program_0/executable objects/program_startup_code/startup.o objects/program_0/main.o

objects/program_0/executable.stripped: objects/program_0/executable | objects/program_0
	x86_64-unknown-elf-strip -o objects/program_0/executable.stripped objects/program_0/executable

objects/program_0/executable.o: objects/program_0/executable.stripped | objects/program_0
	x86_64-unknown-elf-objcopy  -I binary -O elf64-x86-64 -B i386:x86-64 --set-section-flags .data=alloc,contents,load,readonly,data objects/program_0/executable.stripped objects/program_0/executable.o

objects/program_1/main.o: src/program_1/main.c src/include/scwrapper.h | objects/program_1
	x86_64-unknown-elf-gcc -fPIE -m64 $(CFLAGS)  $(OPTIMIZATIONFLAGS) -c -o objects/program_1/main.o src/program_1/main.c

objects/program_1/executable: objects/program_startup_code/startup.o objects/program_1/main.o src/program_startup_code/program_link.ld | objects/program_1
	x86_64-unknown-elf-ld  -z max-page-size=4096 -static -Tsrc/program_startup_code/program_link.ld -o objects/program_1/executable objects/program_startup_code/startup.o objects/program_1/main.o

objects/program_1/executable.stripped: objects/program_1/executable | objects/program_1
	x86_64-unknown-elf-strip -o objects/program_1/executable.stripped objects/program_1/executable

objects/program_1/executable.o: objects/program_1/executable.stripped | objects/program_1
	x86_64-unknown-elf-objcopy  -I binary -O elf64-x86-64 -B i386:x86-64 --set-section-flags .data=alloc,contents,load,readonly,data objects/program_1/executable.stripped objects/program_1/executable.o

objects/program_2/main.o: src/program_2/main.c src/include/scwrapper.h | objects/program_2
	x86_64-unknown-elf-gcc -fPIE -m64 $(CFLAGS) $(OPTIMIZATIONFLAGS) -c -o objects/program_2/main.o src/program_2/main.c

objects/program_2/executable: objects/program_startup_code/startup.o objects/program_2/main.o src/program_startup_code/program_link.ld | objects/program_2
	x86_64-unknown-elf-ld  -z max-page-size=4096 -static -Tsrc/program_startup_code/program_link.ld -o objects/program_2/executable objects/program_startup_code/startup.o objects/program_2/main.o

objects/program_2/executable.stripped: objects/program_2/executable | objects/program_2
	x86_64-unknown-elf-strip -o objects/program_2/executable.stripped objects/program_2/executable

objects/program_2/executable.o: objects/program_2/executable.stripped | objects/program_2
	x86_64-unknown-elf-objcopy  -I binary -O elf64-x86-64 -B i386:x86-64 --set-section-flags .data=alloc,contents,load,readonly,data objects/program_2/executable.stripped objects/program_2/executable.o

clean:
	-rm -rf objects

objects/kernel:
	-mkdir -p objects/kernel

objects/program_0:
	-mkdir -p objects/program_0

objects/program_1:
	-mkdir -p objects/program_1

objects/program_2:
	-mkdir -p objects/program_2

objects/program_startup_code:
	-mkdir -p objects/program_startup_code

compile: $(KERNEL)

all: boot
