/**
 * \file startap.s This file contains the bootstrap code for application 
 *  processors. The code starts the processor up in long mode.
 */

 .text
 .global start_application_processor
 .global start_application_processor_end
 .global gdt_32
 .extern AP_init
 .extern AP_boot_stack
 .type AP_init,@function
 .set start_address, 0x10000
 .code16

/**
 *  Start of the code that starts an application processor up in long mode.
 */
start_application_processor:
 # Just in case, turn of interrupts. We can still get NMIs but we will just
 # ignore them for now.
 cli

 # We are in real mode which means we can use some 32-bit instructions.

 # Check CPUID for compliancy
 xor     %eax,%eax
 cpuid
 cmpl    $0,%eax
 jle     halt_cpu
 movl    $1,%eax
 cpuid

 andl    $0x0380a971,%edx
 cmpl    $0x0380a971,%edx
 jne     halt_cpu
 # Great all basic functions we need are there!
 # Now check if we can get into 64-bit mode
 mov     $0x80000000,%eax
 cpuid
 cmpl    $0x80000000,%eax
 jbe     halt_cpu         # No. CPUID can not give any information about the
                          # 64-bit mode.
 movl    $0x80000001,%eax
 cpuid
 bt      $29,%edx
 jnc     halt_cpu         # No. We can not go into 64-bit mode
 bt      $20,%edx
 jnc     halt_cpu         # NX bit is not supported and we use it...

 # Set up a selector register so that we can access the data
 movw    $(start_address>>4),%ax
 mov     %ax,%ds
 lgdt    %ds:gdt_32-start_application_processor

 # Turn on protected mode
 mov     $0x11,%eax
 mov     %eax, %cr0

 # Jump to 32-bit protected mode. We go through 32-bit mode so that we can
 # reuse code from the BSP start.
 .byte   0xea
 .short  protected_mode_entry-start_application_processor
 .short  16*16+5*8

halt_cpu:
 hlt
 jmp     halt_cpu
	
protected_mode_entry:
 .code32

 # First enable 64-bit page table entries by setting the PAE bit
 movl    %cr4,%ecx
 bts     $5,%ecx   # Enable 64-bit page table entries
 bts     $9,%ecx   # Enable 128-bit floating point instructions
 movl    %ecx,%cr4

 # Use a 32-bit data segment
 movw    $16*16+6*8,%ax
 mov     %ax,%ds

 # Set the root of the page table tree. Taking the lowest 32-bit of a 64-bit
 # pointer is dodgy but works here. The page tables are rather low down in
 # memory
 mov     kernel_page_table_root,%ebp
 mov     %ebp,%cr3

 # Enable long mode. This is done by setting a bit in the EFER register
 # The EFER register is accessed indirectly. We also enable syscall/sysret
 # and the use of the NX bit.
 movl    $0xc0000080,%ecx
 rdmsr   # Read EFER
 orl     $0x901,%eax
 wrmsr   # Write EFER.

 # Enable paging, write protection etc. This will also activate 64-bit mode.
 movl    $0x80010033,%ecx
 movl    %ecx,%cr0

 # We can now do a long jump into the 64-bit code
 ljmp    $24,$(start_of_64bit_code-start_application_processor+start_address)

start_of_64bit_code:
 .code64
 
 # Reload segment registers
 # We reload segment registers so that we use 64-bit space for everything
 mov     $32,%ecx
 mov     %ecx,%ss
 mov     %ecx,%ds
 mov     %ecx,%es
 mov     %ecx,%fs
 mov     %ecx,%gs

 # Set up the stack
 movq    AP_boot_stack,%rsp

 # Set all flags to a well defined state
 pushq   $0
 popfq

 # Jump to the kernel's AP starting point to set the processor up. We will not
 # return.
 mov     $AP_init,%rax
 call    *%rax

 .align 8
gdt_32:
 .word  16*16+7*8-1
 .int   0

start_application_processor_end:
