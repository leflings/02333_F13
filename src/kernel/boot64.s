# This file holds long mode assembly code.
# WARNING: This code, like most assembly code, is far from easy to understand.
#  Take help from a teaching assistant!

 .text
 .global _start
 .global executable_table_size
 .global executable_table
# This is the 64-bit kernel entry point
_start:
 # We can now set the kernel stack
 mov    $stack,%rsp
 
 # Set all flags to a well defined state
 push   $0
 popf

 # Clear the bss segment
 mov    $start_of_bss,%rbp
 mov    $end_of_bss,%rax
 sub    %rbp,%rax
 xor    %rcx,%rcx
bss_clear_loop:
 mov    %rcx,(%rbp)
 add    $8,%rbp
 sub    $8,%rax
 jnz    bss_clear_loop
		
 # Set the FS base to 0
 mov    $0xc0000100,%ecx
 xor    %eax,%eax
 xor    %edx,%edx
 wrmsr  # must use the wrmsr instruction to write a 64-bit value to FS, GS 
        # and KernelGS
 
 # And the GS base to 0
 mov    $0xc0000101,%ecx
 xor    %eax,%eax
 xor    %edx,%edx
 wrmsr

 # And finally the KernelGS base to 0
 mov    $0xc0000102,%ecx
 xor    %eax,%eax
 xor    %edx,%edx
 wrmsr

 # And reload FS and GS
 mov    $32,%eax
 mov    %eax,%fs
 mov    %eax,%gs
 
 # We do not set a new GDT since all the descriptors we are interested in are
 # there already and there is no problem for us to have it below the 4Gbyte
 # barrier.
 
 # We set up the registers necessary to use syscall
 # The registers are STAR, LSTAR, CSTAR and SFMASK.
 # LSTAR holds the target address of system calls in
 # long mode so STAR and CSTAR just points to a dummy target.
 # See AMD64 Programmers Manual, Vol. 2
 
 # Write STAR
 mov    $0xc0000081,%ecx
 mov    $0x00030018,%edx
 mov    $syscall_dummy_target,%eax
 wrmsr

 # Write LSTAR
 mov    $0xc0000082,%ecx
 xor    %edx,%edx
 mov    $syscall_target,%eax
 wrmsr

 # Write CSTAR
 mov    $0xc0000083,%ecx
 xor    %edx,%edx
 mov    $syscall_dummy_target,%eax
 wrmsr
 
 # Write SFMASK
 mov    $0xc0000084,%ecx
 xor    %edx,%edx
 mov    $0x00000300,%eax
 wrmsr

 # Set the ELF_images_start variable to the start of the linked list of
 # executable images
 movq   $start_of_ELF_images,ELF_images_start

 # Set the ELF_images_end variable to the end of the linked list of
 # executable images
 movq   $end_of_ELF_images,ELF_images_end

 # Set the first_available_memory_byte variable to the address of the first
 # available memory byte. The address is rounded of to the nearest higher
 # address which is evenly dividable with 4096.
 mov    $end_of_bss,%rax
 add    $4096-1,%rax
 and    $-4096,%rax
 mov    %rax,first_available_memory_byte

 # Set the highest available memory address. For now this is just a hack.
 # The memory management code will be improved in later tasks.
 mov	$(32768-1024-64)*1024,%rax
 mov    %rax,memory_size

 # We can now switch to c!
 call   initialize

 jmp    return_to_user_mode
