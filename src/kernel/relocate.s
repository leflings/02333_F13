/**
 * \file relocate.s
 * \brief This file contains 64-bit code that calls the kernels
 *  64-bit entry code.
 */

 .text
 .global start_of_64bit_code
 .align  16

/** Start of the 64-bit portion of the kernel. This code is used as a 
 *  trampoline to call the 64-bit kernel.
 */
start_of_64bit_code:
 # Reload segment registers
 # We reload segment registers so that we use 64-bit space for everything
 mov    $32,%ecx
 mov    %ecx,%ss
 mov    %ecx,%ds
 mov    %ecx,%es
 mov    %ecx,%fs
 mov    %ecx,%gs

 # Carry some 64-bit values over from the boot code to the 64-bit kernel
 movq   APIC_id_bit_field,%r14
 movq   pic_interrupt_bitfield,%r13
 movl   number_of_available_CPUs,%r12d
	
 # Jump to the main kernel's entry point.
 mov    $main_kernel,%rax
 mov    24(%rax),%rax
 jmp    *%rax

