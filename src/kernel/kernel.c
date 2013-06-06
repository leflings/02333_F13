/*!
 * \file kernel.c
 * \brief
 *  This is the main source code for the kernel.
 */

#include "kernel.h"
#include "threadqueue.h"
#include "mm.h"
#include "sync.h"
#include "network.h"

/* Note: Look in kernel.h for documentation of global variables and
   functions. */

/* Variables */

unsigned long
APIC_id_bit_field;

unsigned long
pic_interrupt_bitfield;

volatile unsigned int
screen_lock=0;

volatile unsigned int
page_frame_table_lock=0;

union thread
thread_table[MAX_NUMBER_OF_THREADS];

volatile unsigned int
thread_table_lock=0;

struct process
process_table[MAX_NUMBER_OF_PROCESSES];

volatile unsigned int
process_table_lock=0;

struct thread_queue
ready_queue;

volatile unsigned int
ready_queue_lock=0;

struct CPU_private
CPU_private_table[MAX_NUMBER_OF_CPUS];

unsigned int
number_of_available_CPUs=0;

volatile unsigned int
number_of_initialized_CPUs=0;

volatile unsigned int
CPU_private_table_lock=0;

unsigned int
pic_interrupt_map[12];

struct executable
executable_table[MAX_NUMBER_OF_EXECUTABLES];

int
executable_table_size = 0;

/* The following two variables are set by the assembly code. */

const struct executable_image* ELF_images_start;

const char* ELF_images_end;

/* Initialize the timer queue to be empty. */
int
timer_queue_head=-1;

volatile unsigned int
timer_queue_lock=0;

/* Initialize the system time to be 0. */
volatile long
system_time=0;

void*
AP_boot_stack;

unsigned int
GS_base;

unsigned int
TSS_selector;

/*! Size of the keyboard scan code buffer. */
#define KEYBOARD_BUFFER_SIZE 16

/*! Buffer which holds the keyboard scan codes coming from the keyboard
    interrupt handler. */
static unsigned char
keyboard_scancode_buffer[KEYBOARD_BUFFER_SIZE];

/*! Low water mark for the scan code buffer. */
static int
keyboard_scancode_low_marker = 0;

/*! High water mark for the scan code buffer. */
static int
keyboard_scancode_high_marker = 0;

/*! List of threads blocked waiting for scan codes. */
struct thread_queue
keyboard_blocked_threads;

volatile unsigned int
keyboard_scancode_buffer_lock=0;
/*!< Spin lock used to ensure mutual exclusion to the keyboard scan code
     buffer. */

/*! Points to the first byte after the area where the kernel stacks are
    located.  */
extern char
kernel_stack_base[1];

/* Function definitions */

struct prepare_process_return_value
prepare_process(const struct Elf64_Ehdr* elf_image,
                const unsigned int       process,
                unsigned long            memory_footprint_size)
{
 /* Get the address of the program header table. */
 int                program_header_index;
 struct Elf64_Phdr* program_header = ((struct Elf64_Phdr*)
                                       (((char*) (elf_image)) +
                                        elf_image->e_phoff));
 unsigned long      used_memory = 0;

 /* Allocate memory for the page table and for the process' memory. All of 
    this is allocated in a single memory block. The memory block is set up so
    that it cannot be de-allocated via kfree. */
 long               address_to_memory_block = 
  kalloc(memory_footprint_size+19*4*1024, process, ALLOCATE_FLAG_KERNEL);

 struct prepare_process_return_value ret_val = {0, 0};


 /* First check that we have enough memory. */
 if (0 >= address_to_memory_block)
 {
  /* No, we don't. */
  return ret_val;
 }

 ret_val.page_table_address = address_to_memory_block;

 {
  /* Create a page table for the process. */
  unsigned long* dst = (unsigned long*) address_to_memory_block;
  unsigned long* src = (unsigned long*) (kernel_page_table_root + 3*4*1024);
  register int i;

  /* Clear the first frames. */
  for(i=0; i<3*4*1024/8; i++)
  {
   *dst++ = 0;
  }

  /* Build the pml4 table. */
  dst = (unsigned long*) (address_to_memory_block);
  *dst = (address_to_memory_block+4096) | 7;

  /* Build the pdp table. */
  dst = (unsigned long*) (address_to_memory_block+4096);
  *dst = (address_to_memory_block+2*4096) | 7;
  /* Copy the APIC mapping. */
  *(dst+3) = *((unsigned long*) (kernel_page_table_root + 4096 + 24));

  /* Build the pd table. */
  dst = (unsigned long*) (address_to_memory_block+2*4096);
  for(i=0; i<16; i++)
  {
   *dst++ = (address_to_memory_block+(3+i)*4096) | 7;
  }

  /* Copy the rest of the kernel page table. */
  dst = (unsigned long*) (address_to_memory_block + 3*4*1024);
  for(i=0; i<(16*1024*4/8); i++)
  {
   *dst++ = *src++;
  }
 }

 /* Update the start of the block to be after the page table. */

 address_to_memory_block += 19*4*1024;

 /* Scan through the program header table and copy all PT_LOAD segments to
    memory. Perform checks at the same time.*/

 for (program_header_index = 0;
      program_header_index < elf_image->e_phnum;
      program_header_index++)
 {
  if (PT_LOAD == program_header[program_header_index].p_type)
  {
   /* Calculate destination adress. */
   unsigned long* dst = (unsigned long *) (address_to_memory_block + 
                                           used_memory);

   /* Check for odd things. */
   if (
       /* Check if the segment is contigous */
       (used_memory != program_header[program_header_index].p_vaddr) ||
       /* Check if the segmen fits in memory. */
       (used_memory + program_header[program_header_index].p_memsz >
        memory_footprint_size) ||
       /* Check if the segment has an odd size. We require the segement
          size to be an even multiple of 8. */
       (0 != (program_header[program_header_index].p_memsz&7)) ||
       (0 != (program_header[program_header_index].p_filesz&7)))
   {
    /* Something went wrong. Return an error. */
    return ret_val;
   }

   /* First copy p_filesz from the image to memory. */
   {
    /* Calculate the source address. */
    unsigned long* src = (unsigned long *) (((char*) elf_image)+
     program_header[program_header_index].p_offset);
    unsigned long count = program_header[program_header_index].p_filesz/8;

    for(; count>0; count--)
    {
     *dst++=*src++;
    }
   }


   /* Then write p_memsz-p_filesz bytes of zeros. This to pad the segment. */
   {
    unsigned long count = (program_header[program_header_index].p_memsz-
                           program_header[program_header_index].p_filesz)/8;

    for(; count>0; count--)
    {
     *dst++=0;
    }
   }

   /* Set the permission bits on the loaded segment. */
   update_memory_protection(ret_val.page_table_address,
                            program_header[program_header_index].p_vaddr+
                             address_to_memory_block,
                            program_header[program_header_index].p_memsz,
                            program_header[program_header_index].p_flags&7);

   /* Finally update the amount of used memory. */
   used_memory += program_header[program_header_index].p_memsz;
  }
 }

 /* Find out the address to the first instruction to be executed. */
 ret_val.first_instruction_address = address_to_memory_block +
                                     elf_image->e_entry;
 return ret_val;
}

void
cleanup_process(const int process)
{
 register unsigned int i;

 /* Obtain exclusive access to the page_frame_table. */
 grab_lock_rw(&page_frame_table_lock);

 for(i=0; i<memory_pages; i++)
 {
  if (page_frame_table[i].owner == process)
  {
   page_frame_table[i].owner=-1;
   page_frame_table[i].free_is_allowed=1;
  }
 }

 CPU_private_table[get_processor_index()].page_table_root =
  kernel_page_table_root;
 release_lock(&page_frame_table_lock);
}

void
send_IPI(register unsigned char const destination_processor_index,
         register unsigned int  const vector)
{
 /* Set destination. */
 *(LOCAL_APIC_BASE_ADDRESS + 0x310/4) =
   CPU_private_table[destination_processor_index].local_apic_id<<(56-32);
 /* And send interrupts. */
 *(LOCAL_APIC_BASE_ADDRESS + 0x300/4) = vector & 0xff;
}

/*! Initialize the local APIC for the current CPU. */
void
initialize_APIC(void)
{
 register unsigned int max_lvt_entries =
  (*(LOCAL_APIC_BASE_ADDRESS + 0x30/sizeof(unsigned int)) >> 16) & 0xff;

 while (5 != max_lvt_entries)
 {
  kprints("PANIC: APIC configuration not supported.\n");
 }

 /* Make sure the APIC is enabled. */
 *(LOCAL_APIC_BASE_ADDRESS + 0xf0/sizeof(unsigned int)) |= 0x100;

 /* Set task priority. */
 *(LOCAL_APIC_BASE_ADDRESS + 0x80/sizeof(unsigned int)) = 0;

 /* Mask all local sources. */
 *(LOCAL_APIC_BASE_ADDRESS + 0x320/sizeof(unsigned int)) |= 0x10000;
 *(LOCAL_APIC_BASE_ADDRESS + 0x330/sizeof(unsigned int)) |= 0x10000;
 *(LOCAL_APIC_BASE_ADDRESS + 0x340/sizeof(unsigned int)) |= 0x10000;
 *(LOCAL_APIC_BASE_ADDRESS + 0x350/sizeof(unsigned int)) |= 0x10000;
 *(LOCAL_APIC_BASE_ADDRESS + 0x360/sizeof(unsigned int)) |= 0x10000;
 *(LOCAL_APIC_BASE_ADDRESS + 0x370/sizeof(unsigned int)) |= 0x10000;

 number_of_initialized_CPUs++;
}

static unsigned int
read_io_apic_register(register unsigned int const register_number)
{
 *IOAPIC_BASE_ADDRESS = register_number;

 return(*(IOAPIC_BASE_ADDRESS + 16/sizeof(unsigned int)));
}

static void
write_io_apic_register(register unsigned int const register_number,
                       register unsigned int const value)
{
 *IOAPIC_BASE_ADDRESS = register_number;

 *(IOAPIC_BASE_ADDRESS + 16/sizeof(unsigned int)) = value;
}

void
initialize(void)
{
 register int i,j;

 /* Loop over all threads in the thread table and reset the owner. */
 for(i=0; i<MAX_NUMBER_OF_THREADS; i++)
 {
  thread_table[i].data.owner=-1; /* -1 is an illegal process_table index.
                                     We use that to show that the thread
                                     is dormant. */
 }

 /* Loop over all processes in the thread table and mark them as not
    executing. */
 for(i=0; i<MAX_NUMBER_OF_PROCESSES; i++)
 {
  process_table[i].threads=0;    /* No executing process has less than 1
                                    thread. */
 }

 /* Initialize the CPU_private_table. */

 for(i=0; i<MAX_NUMBER_OF_CPUS; i++)
 {
  CPU_private_table[i].stack =
   (unsigned long) &kernel_stack_base[-i*2*4096];
  CPU_private_table[i].page_table_root = kernel_page_table_root;
  CPU_private_table[i].thread_index = -1;
  CPU_private_table[i].CPU_index = i;
  CPU_private_table[i].ticks_left_of_time_slice = 1;
 }

 /* Set up the PIC interrupt map. */
 for(i=0; i<12; i++)
  pic_interrupt_map[i] = (pic_interrupt_bitfield >> (i*5)) & 31;

 /* Set up APIC ids by decoding the bitfield generated by the boot code. */
 j=0;
 for(i=0; i<64; i++)
  if (APIC_id_bit_field & (1 << i))
   CPU_private_table[j++].local_apic_id=i;
 /* Initialize the ready queue. */
 thread_queue_init(&ready_queue);

 /* Initialize the list of blocked threads waiting for the keyboard. */
 thread_queue_init(&keyboard_blocked_threads);

 /* Calculate the number of pages. */
 memory_pages = memory_size/(4*1024);

 if (memory_pages > MAX_NUMBER_OF_FRAMES)
  memory_pages = MAX_NUMBER_OF_FRAMES;

 {
  /* Calculate the number of frames occupied by the kernel and executable 
     images. */
  const register int k = first_available_memory_byte/(4*1024);

  /* Mark the pages that are used by the kernel or executable images as taken
    by the kernel (-2 in the owner field). */
  for(i=0; i<k; i++)
  {
   page_frame_table[i].owner = -2;
   page_frame_table[i].free_is_allowed = 0;
  }

  /* Loop over all the rest page frames and mark them as free (-1 in owner
     field). */
  for(i=k; i<memory_pages; i++)
  {
   page_frame_table[i].owner = -1;
   page_frame_table[i].free_is_allowed = 1;
  }

  /* Mark any unusable pages as taken by the kernel. */
  for(i=memory_pages; i<MAX_NUMBER_OF_FRAMES; i++)
  {
   page_frame_table[i].owner = -2;
   page_frame_table[i].free_is_allowed = 0;
  }
 }

 /* Go through the linked list of executable images and verify that they
    are correct. At the same time build the executable_table. */
 {
  const struct executable_image* image;

  for (image=ELF_images_start; 0!=image; image=image->next)
  {
   unsigned long      image_size;

   /* First calculate the size of the image. */
   if (0 != image->next)
   {
    image_size = ((char *) (image->next)) - ((char *) image) -1;
   }
   else
   {
    image_size = ((char *) ELF_images_end) - ((char *) image) - 1;
   }

   /* Check that the image is an ELF image and that it is of the
      right type. */
   if (
       /* EI_MAG0 - EI_MAG3 have to be 0x7f 'E' 'L' 'F'. */
       (image->elf_image.e_ident[EI_MAG0] != 0x7f) ||
       (image->elf_image.e_ident[EI_MAG1] != 'E') ||
       (image->elf_image.e_ident[EI_MAG2] != 'L') ||
       (image->elf_image.e_ident[EI_MAG3] != 'F') ||
       /* Check that the image is a 64-bit image. */
       (image->elf_image.e_ident[EI_CLASS] != 2) ||
       /* Check that the image is a little endian image. */
       (image->elf_image.e_ident[EI_DATA] != 1) ||
       /* And that the version of the image format is correct. */
       (image->elf_image.e_ident[EI_VERSION] != 1) ||
       /* NB: We do not check the ABI or ABI version. We really should
          but currently those fields are not set properly by the build
          tools. They are both set to zero which means: System V ABI,
          third edition. However, the ABI used is clearly not System V :-) */

       /* Check that the image is executable. */
       (image->elf_image.e_type != 2) ||
       /* Check that the image is executable on AMD64. */
       (image->elf_image.e_machine != 0x3e) ||
       /* Check that the object format is correct. */
       (image->elf_image.e_version != 1) ||
       /* Check that the processor dependent flags are all reset. */
       (image->elf_image.e_flags != 0) ||
       /* Check that the length of the header is what we expect. */
       (image->elf_image.e_ehsize != sizeof(struct Elf64_Ehdr)) ||
       /* Check that the size of the program header table entry is what
          we expect. */
       (image->elf_image.e_phentsize != sizeof(struct Elf64_Phdr)) ||
       /* Check that the number of entries is reasonable. */
       (image->elf_image.e_phnum < 0) ||
       (image->elf_image.e_phnum > 8) ||
       /* Check that the entry point is within the image. */
       (image->elf_image.e_entry < 0) ||
       (image->elf_image.e_entry >= image_size) ||
       /* Finally, check that the program header table is within the image. */
       (image->elf_image.e_phoff > image_size) ||
       ((image->elf_image.e_phoff +
         image->elf_image.e_phnum * sizeof(struct Elf64_Phdr)) > image_size )
      )

   {
    /* There is something wrong with the image. */
    while (1)
    {
     kprints("Kernel panic! Corrupt executable image.\n");
    }
    continue;
   }

   /* Now check the program header table. */
   {
    int                program_header_index;
    struct Elf64_Phdr* program_header = ((struct Elf64_Phdr*)
                                         (((char*) &(image->elf_image)) +
                                          image->elf_image.e_phoff));
    unsigned long      memory_footprint_size = 0;

    for (program_header_index = 0;
         program_header_index < image->elf_image.e_phnum;
         program_header_index++)
    {
     /* First sanity check the entry. */
     if (
         /* Check that the segment is a type we can handle. */
         (program_header[program_header_index].p_type < 0) ||
         (!((program_header[program_header_index].p_type == PT_NULL) ||
            (program_header[program_header_index].p_type == PT_LOAD) ||
            (program_header[program_header_index].p_type == PT_PHDR))) ||
         /* Look more carefully into loadable segments. */
         ((program_header[program_header_index].p_type == PT_LOAD) &&
           /* Check if any flags that we can not handle is set. */
          (((program_header[program_header_index].p_flags & ~7) != 0) ||
           /* Check if sizes and offsets look sane. */
           (program_header[program_header_index].p_offset < 0) ||
           (program_header[program_header_index].p_vaddr < 0) ||
           (program_header[program_header_index].p_filesz < 0) ||
           (program_header[program_header_index].p_memsz < 0) ||
          /* Check if the segment has an odd size. We require the
             segment size to be an even multiple of 8. */
           (0 != (program_header[program_header_index].p_memsz&7)) ||
           (0 != (program_header[program_header_index].p_filesz&7)) ||
           /* Check if the segment goes beyond the image. */
           ((program_header[program_header_index].p_offset +
             program_header[program_header_index].p_filesz) > image_size)))
        )
     {
      while (1)
      {
       kprints("Kernel panic! Corrupt segment.\n");
      }
     }

     /* Check that all PT_LOAD segments are contiguous starting from
        address 0. Also, calculate the memory footprint of the image. */
     if (program_header[program_header_index].p_type == PT_LOAD)
     {
      if (program_header[program_header_index].p_vaddr !=
          memory_footprint_size)
      {
       while (1)
       {
        kprints("Kernel panic! Executable image has illegal memory layout.\n");
       }
      }

      memory_footprint_size += program_header[program_header_index].p_memsz;
     }
    }

    executable_table[executable_table_size].memory_footprint_size =
     memory_footprint_size;
   }

   executable_table[executable_table_size].elf_image = &(image->elf_image);
   executable_table_size += 1;

   kprints("Found an executable image.\n");

   if (executable_table_size >= MAX_NUMBER_OF_EXECUTABLES)
   {
    while (1)
    {
     kprints("Kernel panic! Too many executable images found.\n");
    }
   }
  }
 }

 /* Check that actually some executable files are found. Also check that the
    thread structure is of the right size. The assembly code will break if it
    is not. */

 if ((0 >= executable_table_size) || (1024 != sizeof(union thread)))
 {
  while (1)
  {
   kprints("Kernel panic! Can not boot.\n");
  }
 }

 /* Copy the application processor bootstrap code into place. */
 {
  register char*       dst = (char*) 0x10000;
  register const char* src = start_application_processor;

  for(; src<start_application_processor_end;)
  {
   *dst++=*src++;
  }
 }

 initialize_memory_protection();
 initialize_ports();
 initialize_thread_synchronization();
 initialize_ne2k();
 initialize_network();

 /* All sub-systems are now initialized. Kernel areas can now get the right
    memory protection. */

 {
  /* Use the kernel's ELF header. */
  extern struct Elf64_Ehdr kernel64_start[1];

  struct Elf64_Phdr* program_header = ((struct Elf64_Phdr*)
                                       (((char*) (kernel64_start)) +
                                        kernel64_start->e_phoff));

  /* Traverse the program header. */
  short              number_of_program_header_entries = 
                      kernel64_start->e_phnum;
  int                i;
  for(i=0; i<number_of_program_header_entries; i++)
  {
   if (PT_LOAD == program_header[i].p_type)
   {
    /* Set protection on each segment. */

    update_memory_protection(kernel_page_table_root,
                             program_header[i].p_vaddr,
                             program_header[i].p_memsz,
                             (program_header[i].p_flags&7) | PF_KERNEL);
   }
  }

  /* Set the protection for the bootstrap code. */
  update_memory_protection(kernel_page_table_root,
                           0x10000,
                           4096,
                           PF_X | PF_R | PF_KERNEL);

  /* Reload the kernel page table. */
  __asm volatile("movq %0,%%cr3" : :
                 "r" (kernel_page_table_root));
 }

 /* Now we set up the 8259A interrupt controller to not send interrupts. */
 outb(0x20, 0x11);
 outb(0xA0, 0x11);

 outb(0x21, 0x20);
 outb(0xA1, 0x28);

 outb(0x21, 1<<2);
 outb(0xA1, 2);

 outb(0x21, 1);
 outb(0xA1, 1);

 outb(0x21, 0xff);
 outb(0xA1, 0xff);

 /* Set up the timer hardware to generate interrupts 200 times a second. */
 outb(0x43, 0x36);
 outb(0x40, 78);
 outb(0x40, 23);

 /* Initialize IO APIC disabling all interrupts. */
 {
  register int i;
  for(i=0; i<24; i++)
  {
   register unsigned int const io_apic_low_bits =
    read_io_apic_register(0x10 + 2*i);
   write_io_apic_register(0x10 + 2*i, io_apic_low_bits | 0x10000);
  }
 }

 /* Route NMIs and 8259 interrupts through the APIC. */
 outb(0x22, 0x70);
 outb(0x23, 1);

 /* Bootstrap all processors. */
 {
  register int processor_index;

  initialize_APIC();
  number_of_initialized_CPUs = 1;

  kprints("BSP initialized.\n");

  for(processor_index = 1;
      processor_index<number_of_available_CPUs;
      processor_index++)
  {
   /* Set up information to be sent to the application processor boot strap. */

   TSS_selector = 40 + processor_index*16;
   GS_base =
    (unsigned int) (((unsigned long)(&CPU_private_table[processor_index])) &
                    0xffffffff);
   AP_boot_stack = &kernel_stack_base[-processor_index*2*4096];

   kprints("Attempting to start AP");

   /* Application processor. Send IPI to wake it up. */

   /*! \todo The SIPI procedure is not bullet proof. It works with bochs but
      to be fully working on all CPU types and revisions there should be a
      timeout and a second try if the first does not work. */

   kprinthex(CPU_private_table[processor_index].local_apic_id);
   kprints(".\n\0");

   /* Send INIT command. */
   *(LOCAL_APIC_BASE_ADDRESS + 0x310/4) =
    CPU_private_table[processor_index].local_apic_id<<(56-32);
   *(LOCAL_APIC_BASE_ADDRESS + 0x300/4) = 0x510;

   /* Pause for a moment, using busy loop and CPU0's APIC timer. */
   {
    /* Set divider to 2. */
    *(LOCAL_APIC_BASE_ADDRESS + 0x3e0/4) = 0x00;

    /* Set initial count */
    *(LOCAL_APIC_BASE_ADDRESS + 0x380/4) = 1000000;

    while (*(LOCAL_APIC_BASE_ADDRESS + 0x390/4) > 0);
   }

   /* Send SIPI. */
   *(LOCAL_APIC_BASE_ADDRESS + 0x300/4) = 0x610;

   /* Spin until the application processor is woken up. */
   while((processor_index+1) != number_of_initialized_CPUs);

   kprints("AP initialized.\n\0");
  }
 }

 /* Start running the first program in the executable table. */

 /* Use the ELF program header table and copy the right portions of the
    image to memory. This is done by prepare_process. */
 {
  struct prepare_process_return_value prepare_process_ret_val = 
   prepare_process(executable_table[0].elf_image,
                   0,
                   executable_table[0].memory_footprint_size);

  if (0 == prepare_process_ret_val.first_instruction_address)
  {
   while (1)
   {
    kprints("Kernel panic! Can not start process 0!\n");
   }
  }

  /* Start executable program 0 as process 0. At this point, there are no
     processes so we can just grab entry 0 and use it. */
  process_table[0].parent=-1;    /* We put -1 to indicate that there is no
                                    parent process. */
  process_table[0].threads=1;

  /*  all processes should start with an allocated port with id zero */
  if (-1 == allocate_port(0,0))
  {
   while(1)
   {
    kprints("Kernel panic! Can not initialize the IPC system!\n");
   }
  }

  /* Set the page table address. */
  process_table[0].page_table_root =
   prepare_process_ret_val.page_table_address;
  CPU_private_table[0].page_table_root =
   prepare_process_ret_val.page_table_address;

  /* We need a thread. We just take the first one as no threads are running or
     have been allocated at this point. */
  thread_table[0].data.owner=0;  /* 0 is the index of the first process. */

  /* We reset all flags and enable interrupts */
  thread_table[0].data.registers.integer_registers.rflags=0x200;

  /* And set the start address. */
  thread_table[0].data.registers.integer_registers.rip =
   prepare_process_ret_val.first_instruction_address;

  /* Finally we set the current thread. */
  CPU_private_table[0].thread_index=0;
 }

 /* Set up the keyboard controller. */
 
 /* Empty the keyboard buffer. */
 {
  register unsigned char status_byte;
  do
  {
   status_byte=inb(0x64);
   if ((status_byte&3)==1)
   {
    inb(0x60);
   }
  } while((status_byte&0x3)!=0x0);
 }

 /* Change the command byte to enable interrupts. */
 outb(0x64, 0x20);
 {
  register unsigned char keyboard_controller_command_byte;

  {
   register unsigned char status_byte;
   do
   {
    status_byte=inb(0x64);
   } while((status_byte&3)!=1);
  }

  keyboard_controller_command_byte=inb(0x60);

  /* Enable keyboard interrupts. */
  keyboard_controller_command_byte|=1;

  kprints("Keyboard controller command byte:");
  kprinthex(keyboard_controller_command_byte);
  kprints("\n");

  outb(0x64, 0x60);
  outb(0x60, keyboard_controller_command_byte);

  /* Wait until command is done. */
  {
   register unsigned char status_byte;
   do
   {
    status_byte=inb(0x64);
   } while((status_byte&0x2)!=0x0);
  }
 } 

 clear_screen();

 kprints("\n\n\nThe kernel has booted!\n\n\n");
 kprinthex(number_of_initialized_CPUs);
 kprints("\n\n");

 /* Enable timer, keyboard and ne2k interrupts. */

 {
  register unsigned int timer_gsi = pic_interrupt_map[0];
  /* Send timer interrupts to all cpus. */
  write_io_apic_register(0x11 + timer_gsi*2, 0xff000000);
  write_io_apic_register(0x10 + timer_gsi*2, 0x00000020);
 }

 {
  register unsigned int keyboard_gsi = pic_interrupt_map[1];
  /* Send keyboard interrupts to the BSP. */
  write_io_apic_register(0x11 + keyboard_gsi*2,
   CPU_private_table[0].local_apic_id << 24);
  write_io_apic_register(0x10 + keyboard_gsi*2, 0x00000021);
 }

 {
  register unsigned int ne2k_gsi = pic_interrupt_map[3];
  /* Send keyboard interrupts to the BSP. */
  write_io_apic_register(0x11 + ne2k_gsi*2,
   CPU_private_table[0].local_apic_id << 24);
  write_io_apic_register(0x10 + ne2k_gsi*2, 0x00000023);
 }
 /* Now go back to the assembly language code and let the process run. */
}

int
allocate_thread(void)
{
 register int i;
 /* loop over all threads and find a free thread. */
 for(i=0; i<MAX_NUMBER_OF_THREADS; i++)
 {
  /* An owner index of -1 means that the thread is available. */
  if (-1 == thread_table[i].data.owner)
  {
   return i;
  }
 }

 /* We return -1 to indicate that there are no available threads. */
 return -1;
}

extern void
system_call_handler(void)
{
 register int schedule = 0;
 /*!< System calls may set this variable to 1. The variable is used as
      input to the scheduler to indicate that scheduling is not necessary. */

 /* Reset the interrupt flag indicating that the context of the caller was
    saved by the system call routine. */
 thread_table[get_current_thread()].data.registers.from_interrupt=0;

 switch(SYSCALL_ARGUMENTS.rax)
 {
  case SYSCALL_PAUSE:
  {
   register int  tmp_thread_index;
   unsigned long timer_ticks=SYSCALL_ARGUMENTS.rdi;

   /* Set the return value before doing anything else. We will switch to a new
      thread very soon! */
   SYSCALL_ARGUMENTS.rax=ALL_OK;

   if (0 == timer_ticks)
   {
    /* We should not wait if we are asked to wait for less then one tick. */
    break;
   }

   /* Get the current thread. */
   tmp_thread_index=get_current_thread();

   /* Force a re-schedule. */
   schedule=1;

   /* And insert the thread into the timer queue. */

   /* The timer queue is a linked list of threads. The head (first entry)
      (thread) in the list has a list_data field that holds the number of
      ticks to wait before the thread is made ready. The next entries (threads)
      has a list_data field that holds the number of ticks to wait after the
      previous thread is made ready. This is called to use a delta-time and
      makes the code to test if threads should be made ready very quick. It
      also, unfortunately, makes the code that insert code into the queue
      rather complex. */

   /* Grab the locks we need. */
   grab_lock_rw(&timer_queue_lock);

   /* If the queue is empty put the thread as only entry. */
   if (-1 == timer_queue_head)
   {
    thread_table[tmp_thread_index].data.next=-1;
    thread_table[tmp_thread_index].data.list_data=timer_ticks;
    timer_queue_head=tmp_thread_index;
   }
   else
   {
    /* Check if the thread should be made ready before the head of the
       previous timer queue. */
    register int curr_timer_queue_entry=timer_queue_head;

    if (thread_table[curr_timer_queue_entry].data.list_data>timer_ticks)
    {
     /* If so set it up as the head in the new timer queue. */

     thread_table[curr_timer_queue_entry].data.list_data-=timer_ticks;
     thread_table[tmp_thread_index].data.next=curr_timer_queue_entry;
     thread_table[tmp_thread_index].data.list_data=timer_ticks;
     timer_queue_head=tmp_thread_index;
    }
    else
    {
     register int prev_timer_queue_entry = curr_timer_queue_entry;

     /* Search until the end of the queue or until we found the right spot. */
     while((-1 != thread_table[curr_timer_queue_entry].data.next) &&
           (timer_ticks>=thread_table[curr_timer_queue_entry].data.list_data))
     {
      timer_ticks-=thread_table[curr_timer_queue_entry].data.list_data;
      prev_timer_queue_entry=curr_timer_queue_entry;
      curr_timer_queue_entry=thread_table[curr_timer_queue_entry].data.next;
     }


     if (timer_ticks>=thread_table[curr_timer_queue_entry].data.list_data)
     {
      /* Insert the thread into the queue after the existing entry. */
      thread_table[tmp_thread_index].data.next=
       thread_table[curr_timer_queue_entry].data.next;
      thread_table[curr_timer_queue_entry].data.next=tmp_thread_index;
      thread_table[tmp_thread_index].data.list_data=timer_ticks-
       thread_table[curr_timer_queue_entry].data.list_data;
     }
     else
     {
      /* Insert the thread into the queue before the existing entry. */
      thread_table[tmp_thread_index].data.next=
       curr_timer_queue_entry;
      thread_table[prev_timer_queue_entry].data.next=tmp_thread_index;
      thread_table[tmp_thread_index].data.list_data=timer_ticks;
      thread_table[curr_timer_queue_entry].data.list_data-=timer_ticks;
     }
    }
   }

   /* We are done accessing the timer queue so we can release the lock. */
   release_lock(&timer_queue_lock);
   break;
  }

  case SYSCALL_TIME:
  {
   /* Returns the current system time to the program. */

   /* Grabs the lock with read permissions so that the time will not change
      when reading it. */
   grab_lock_r(&timer_queue_lock);
   SYSCALL_ARGUMENTS.rax=system_time;
   release_lock(&timer_queue_lock);
   break;
  }

  case SYSCALL_FREE:
  {
   SYSCALL_ARGUMENTS.rax=kfree(SYSCALL_ARGUMENTS.rdi);
   break;
  }

  case SYSCALL_ALLOCATE:
  {
   /* Check the flags. */
   if (0!=SYSCALL_ARGUMENTS.rsi & ~(ALLOCATE_FLAG_READONLY|ALLOCATE_FLAG_EX))
   {
    /* Return if the flags were not properly set. */
    SYSCALL_ARGUMENTS.rax = ERROR;
    break;
   }


   SYSCALL_ARGUMENTS.rax=kalloc(
           SYSCALL_ARGUMENTS.rdi,
           thread_table[get_current_thread()].data.owner,
           SYSCALL_ARGUMENTS.rsi & (ALLOCATE_FLAG_READONLY|ALLOCATE_FLAG_EX));
   break;
  }

  case SYSCALL_ALLOCATEPORT:
  {
   int port=allocate_port(SYSCALL_ARGUMENTS.rdi,
                          thread_table[get_current_thread()].
                           data.owner);

   /* Return an error if a port cannot be allocated. */
   if (port<0)
   {
    SYSCALL_ARGUMENTS.rax=ERROR;
    break;
   }

   SYSCALL_ARGUMENTS.rax=port;
   break;
  }

  case SYSCALL_FINDPORT:
  {
   int port;
   unsigned long process=SYSCALL_ARGUMENTS.rsi;

   /* Return an error if the process argument is wrong. */
   if (process >= MAX_NUMBER_OF_PROCESSES)
   {
    SYSCALL_ARGUMENTS.rax=ERROR;
    break;
   }

   port=find_port(SYSCALL_ARGUMENTS.rdi, 
                  process);

   if (port<0)
   {
    SYSCALL_ARGUMENTS.rax=ERROR;
    break;
   }

   SYSCALL_ARGUMENTS.rax=port;
   break;
  }
  
  case SYSCALL_GETPID:
  {
   grab_lock_r(&thread_table_lock);
   SYSCALL_ARGUMENTS.rax = thread_table[get_current_thread()].data.owner;
   release_lock(&thread_table_lock);
  break;
  }

  case SYSCALL_GETSCANCODE:
  {
   /* Grab spin lock. */
   grab_lock_rw(&keyboard_scancode_buffer_lock);

   /* Check if there is data in the scan code buffer. */
   if (keyboard_scancode_high_marker!=keyboard_scancode_low_marker)
   {
    /* There is data in the buffer. Get it! */
    SYSCALL_ARGUMENTS.rax=keyboard_scancode_buffer
     [(keyboard_scancode_low_marker++)&(KEYBOARD_BUFFER_SIZE-1)];
   }
   else
   {
    /* No data in the buffer. We will block waiting for data. */
    register int current_thread_index;

    /* Set the default return value to be an error. */
    SYSCALL_ARGUMENTS.rax=ERROR;
    current_thread_index=get_current_thread();
    /* Tell the scheduler that it will have to reschedule. */
    schedule=1;
    thread_queue_enqueue(&keyboard_blocked_threads, current_thread_index);
   }

   /* Release spin lock. */
   release_lock(&keyboard_scancode_buffer_lock);
   break;
  }

  default:
  {
   schedule = system_call_implementation();
   break;
  }
 }

 scheduler_called_from_system_call_handler(schedule);
}

static void
timer_interrupt_handler(void)
{
 register int thread_changed=0;
 /*!< Interrupt hander code may set this variable to 0. The variable is
      used as input to the scheduler to indicate if the interrupt code has
      updated scheduling data structures. */
 /* Only the BSP should maintain the timer queue and system time. */
 if (0 == get_processor_index())
 {
  /* Grab the locks so that we can access the data structures freely. */
  grab_lock_rw(&timer_queue_lock);

  /* Increment system time. */
  system_time++;

  /* Check if there are any thread that we should make ready.
     First check if there are any threads at all in the timer
     queue. */
  if (-1 != timer_queue_head)
  {
   /* Then decrement the list_data in the head. */
   thread_table[timer_queue_head].data.list_data-=1;

   /* Then remove all elements including with a list_data equal to zero
      and insert them into the ready queue. These are the threads that
      should be woken up. */
   while((-1 != timer_queue_head) &&
         /* We remove all entries less than or equal to 0. Equality should be
            enough but checking with less than or equal may hide the symptoms
            of some bugs and make the system more stable. */
         (thread_table[timer_queue_head].data.list_data<=0))
   {
    register int tmp_thread_index=timer_queue_head;
    /* Remove the head element.*/
    timer_queue_head=thread_table[tmp_thread_index].data.next;

    /* Let the woken thread run if the CPU is not running any thread. */
    if (-1 == get_current_thread())
    {
     CPU_private_table[get_processor_index()].thread_index = tmp_thread_index;
     thread_changed=1;
     CPU_private_table[get_processor_index()].page_table_root =
      process_table[thread_table[tmp_thread_index].data.owner].
       page_table_root;
    }
    else
    {
     /* Or insert it into the ready queue. */
     grab_lock_rw(&ready_queue_lock);
     thread_queue_enqueue(&ready_queue, tmp_thread_index);
     release_lock(&ready_queue_lock);
    }
   }
  }
  /* Done updating the structures. */
  release_lock(&timer_queue_lock);
 }
 scheduler_called_from_timer_interrupt_handler(thread_changed);

}

/*! Keyboard interrupt handler. */
static inline void
keyboard_interrupt_handler(void)
{
 register unsigned char status_byte=inb(0x64);

 if ((status_byte&1)==1)
 {
  register unsigned char data=inb(0x60);

  /* Grab the buffer lock. */
  grab_lock_rw(&keyboard_scancode_buffer_lock);

  /* Is a thread waiting for data? */
  if (thread_queue_is_empty(&keyboard_blocked_threads))
  {
   /* Store scan code in the buffer if there is space in the buffer. */
   register int buffer_size=keyboard_scancode_high_marker-
                            keyboard_scancode_low_marker;
   if (buffer_size<KEYBOARD_BUFFER_SIZE)
   {
    keyboard_scancode_buffer
     [(keyboard_scancode_high_marker++)&(KEYBOARD_BUFFER_SIZE-1)]=data;
   }
  }
  else
  {
   /* Let the first blocked thread get the scan code. */
   register int blocked_thread_index=
    thread_queue_dequeue(&keyboard_blocked_threads);
   thread_table[blocked_thread_index].data.registers.integer_registers.rax=
    data;

   /* Let the woken thread run if the CPU is not running any thread. */
   if (-1 == get_current_thread())
   {
    CPU_private_table[get_processor_index()].thread_index = blocked_thread_index;

    CPU_private_table[get_processor_index()].page_table_root =
     process_table[thread_table[blocked_thread_index].data.owner].
      page_table_root;
   }
   else
   {
    /* Or insert it into the ready queue. */
    grab_lock_rw(&ready_queue_lock);
    thread_queue_enqueue(&ready_queue, blocked_thread_index);
    release_lock(&ready_queue_lock);
   }
  }

  /* Release buffer lock. */
  release_lock(&keyboard_scancode_buffer_lock);
 }
}

extern void
interrupt_dispatcher(const unsigned long interrupt_number)
{
 /* Select a handler based on interrupt source. */
 switch(interrupt_number)
 {
  case 32:
  {
   timer_interrupt_handler();
   break;
  }

  case 33:
  {
   keyboard_interrupt_handler();
   break;
  }

  case 35:
  {
   ne2k_interrupt_handler();
   break;
  }

  case 255:
  case 39:
  {
   /* Spurious interrupt occurred. This could happen if we spend too long 
      time with interrupts disabled. */
   break;
  }

  case 240:
  {
      int next_thread;
   /* Dummy IPI handler. */
//   scheduler_called_from_system_call_handler(1);

    if(get_current_thread() == -1) {

      grab_lock_rw(&ready_queue_lock);
      /* either we'll get next thread or we'll get -1
       * no need to check for empty and/or then dequeue */
      next_thread = thread_queue_dequeue(&ready_queue);
      release_lock(&ready_queue_lock);



      if(next_thread != -1)
      {
        CPU_private_table[get_processor_index()].thread_index = next_thread;
        CPU_private_table[get_processor_index()].page_table_root = process_table[thread_table[next_thread].data.owner].page_table_root;
        CPU_private_table[get_processor_index()].ticks_left_of_time_slice = 10;
#if DEBUG_ON
        kprints("Interrupted schedule on CPU: ");
        kprinthex(get_processor_index());
        kprints(" -> thread: ");
        kprinthex(next_thread);
        kprints("\n");
#endif
      } else {
        kprints("Woken CPU got no thread\n");
      }
    }

   break;
  }

  default:
  {
   kprints("Unknown interrupt. Vector:");
   kprinthex(interrupt_number);
   kprints("\n");
   while(1)
   {
    outw(0x8a00, 0x8a00);
    outw(0x8a00, 0x8ae0);
   }
  }
 }

 /* Acknowledge interrupt so that new interrupts can be sent to the CPU. */
 if (interrupt_number >= 32)
 {
  /* Do an EOI procedure on the local APIC. */
  *(LOCAL_APIC_BASE_ADDRESS + 0xb0/sizeof(unsigned int)) = 0;
 }
}

void
initialize_network(void)
{
 /* This fuction is empty for now. We only support ARP and ICMP Echo which
    do not need any elaborate initialization. */
}

#define ETHERNET_FRAME_HEADER_LENGTH (14)
#define ARP_PACKET_LENGTH (28)
#define IPv4_HEADER_LENGTH (20)

static unsigned short
calculate_checksum(register const unsigned char* const data,
                   register const unsigned int length)
{
 register unsigned int checksum = 0;
 register int i = 0;

 for(; i < (length -1); i+=2)
 {
  checksum += *((unsigned short *)(&data[i]));
 }

 if (length != i)
  checksum += data[length];

 return (checksum & 0xffff) + (checksum >> 16);
}

void
network_handle_frame(register const unsigned char* const frame,
                     register const unsigned int length)
{
 /* This is not a full or even very compliant network stack. We make a lot of
    assumptions and for example ignore IEEE 802.2 LLC and SNAP frame formats. */

 /* Sanity check. Should not happen. */
 if (ETHERNET_FRAME_HEADER_LENGTH > length)
  return;

 /* Ethernet frame too long. */
 if (1518 < length)
  return;

 /* Check if the frame was broadcast. */
 if ((0xff == frame[0]) &&
     (0xff == frame[1]) &&
     (0xff == frame[2]) &&
     (0xff == frame[3]) &&
     (0xff == frame[4]) &&
     (0xff == frame[5]))
 {
  /* Very likely an ARP request. Check typelen. */
  if ((8 == frame[12]) &&
      (6 == frame[13]) &&
      ((ETHERNET_FRAME_HEADER_LENGTH + ARP_PACKET_LENGTH) <= length))
  {
   /* We have found a likely ARP request. Check it. */

   if (/* Check various header fields such as address lengths and types. */
       (            0 == frame[ETHERNET_FRAME_HEADER_LENGTH]) &&
       (            1 == frame[ETHERNET_FRAME_HEADER_LENGTH + 1]) &&
       (          0x8 == frame[ETHERNET_FRAME_HEADER_LENGTH + 2]) &&
       (            0 == frame[ETHERNET_FRAME_HEADER_LENGTH + 3]) &&
       (            6 == frame[ETHERNET_FRAME_HEADER_LENGTH + 4]) &&
       (            4 == frame[ETHERNET_FRAME_HEADER_LENGTH + 5]) &&
       /* Check that we got an ARP request. */
       (            0 == frame[ETHERNET_FRAME_HEADER_LENGTH + 6]) &&
       (            1 == frame[ETHERNET_FRAME_HEADER_LENGTH + 7]) &&
       /* Check that it was for us. ARP requests are broadcast so we will get
          requests for everyone on our subnet. */
       ( IP_ADDRESS_0 == frame[ETHERNET_FRAME_HEADER_LENGTH + 24]) &&
       ( IP_ADDRESS_1 == frame[ETHERNET_FRAME_HEADER_LENGTH + 25]) &&
       ( IP_ADDRESS_2 == frame[ETHERNET_FRAME_HEADER_LENGTH + 26]) &&
       ( IP_ADDRESS_3 == frame[ETHERNET_FRAME_HEADER_LENGTH + 27])
      )
   {
    /* Everything looks good. Create a reply. */
    unsigned char transmitt_buffer[60];
    register int i;

    /* Create the Ethernet frame header. */
    transmitt_buffer[0] = frame[6];
    transmitt_buffer[1] = frame[7];
    transmitt_buffer[2] = frame[8];
    transmitt_buffer[3] = frame[9];
    transmitt_buffer[4] = frame[10];
    transmitt_buffer[5] = frame[11];

    /* The driver will fill in the source MAC address! */

    transmitt_buffer[12] = 8;
    transmitt_buffer[13] = 6;

    /* Create the ARP reply packet. */
    /* Address types and lengths. */
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 0] = 0;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 1] = 1;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 2] = 8;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 3] = 0;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 4] = 6;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 5] = 4;
    /* ARP reply. */
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 6] = 0;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 7] = 2;
    /* Senders (our) hardware address. */
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 8]  = MAC_ADDRESS_0;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 9]  = MAC_ADDRESS_1;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 10] = MAC_ADDRESS_2;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 11] = MAC_ADDRESS_3;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 12] = MAC_ADDRESS_4;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 13] = MAC_ADDRESS_5;
    /* Senders IP address. */
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 14] = IP_ADDRESS_0;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 15] = IP_ADDRESS_1;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 16] = IP_ADDRESS_2;
    transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 17] = IP_ADDRESS_3;
    /* Copy the destination MAC and IP addresses from the request. */
    for(i = 0; i < 10; i++)
     transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 18 + i] =
      frame[ETHERNET_FRAME_HEADER_LENGTH + 8 + i];

    /* Clear the padding. */
    for(i = ETHERNET_FRAME_HEADER_LENGTH + ARP_PACKET_LENGTH;
        i < 60;
        i++)
     transmitt_buffer[i] = 0;

    ne2k_send_frame(transmitt_buffer, 60);
   }
  }
  return;
 }

 if ((MAC_ADDRESS_0 == frame[0]) &&
     (MAC_ADDRESS_1 == frame[1]) &&
     (MAC_ADDRESS_2 == frame[2]) &&
     (MAC_ADDRESS_3 == frame[3]) &&
     (MAC_ADDRESS_4 == frame[4]) &&
     (MAC_ADDRESS_5 == frame[5]))
 {
  /* ICMP Echo could have been sent to us. We check typelen and if there is
     an IP header. */
  if ((0x08 == frame[12]) &&
      (0x00 == frame[13]) &&
      ((ETHERNET_FRAME_HEADER_LENGTH + IPv4_HEADER_LENGTH) <= length))
  {
   /* IPv4 header. Check if it is for us and if it is an ICMP Echo. */
   if (/* Check the destination IP address to see if the IP packet is to us. */
       (IP_ADDRESS_0 == frame[ETHERNET_FRAME_HEADER_LENGTH + 16]) &&
       (IP_ADDRESS_1 == frame[ETHERNET_FRAME_HEADER_LENGTH + 17]) &&
       (IP_ADDRESS_2 == frame[ETHERNET_FRAME_HEADER_LENGTH + 18]) &&
       (IP_ADDRESS_3 == frame[ETHERNET_FRAME_HEADER_LENGTH + 19]) &&
       /* Check if it is an ICMP packet. */
       (0x1 == frame[ETHERNET_FRAME_HEADER_LENGTH + 9]) &&
       /* Check various header fields. */
       (0x45 == frame[ETHERNET_FRAME_HEADER_LENGTH + 0]) &&
       (0 == (0xbf & frame[ETHERNET_FRAME_HEADER_LENGTH + 6])) &&
       (0 == frame[ETHERNET_FRAME_HEADER_LENGTH + 7])
      )
   {
    /* So far things looks ok. We need to check the checksum though. */
    if (0xffff == calculate_checksum(&frame[ETHERNET_FRAME_HEADER_LENGTH],
                                     IPv4_HEADER_LENGTH))
    {
     register const unsigned int ip_packet_length =
      (frame[ETHERNET_FRAME_HEADER_LENGTH + 2] << 8) |
      frame[ETHERNET_FRAME_HEADER_LENGTH + 3];
     /* checksum is fine. Continue checking the frame length and the ICMP
        header. */
     if (((ip_packet_length + ETHERNET_FRAME_HEADER_LENGTH) <= length) &&
         ((IPv4_HEADER_LENGTH + 4) <= ip_packet_length) &&
         /* Check the ICMP type and code. */
         (8 == frame[ETHERNET_FRAME_HEADER_LENGTH + IPv4_HEADER_LENGTH]) &&
         (0 == frame[ETHERNET_FRAME_HEADER_LENGTH + IPv4_HEADER_LENGTH + 1]))
     {
      /* We have an ICMP Echo packet. Check its checksum. */
      if (0xffff == calculate_checksum(&frame[ETHERNET_FRAME_HEADER_LENGTH +
                                              IPv4_HEADER_LENGTH],
                                       ip_packet_length - IPv4_HEADER_LENGTH))
      {
       /* So far everything looks ok. Build a reply. */
       unsigned char transmitt_buffer[1518];
       register unsigned int frame_length = ETHERNET_FRAME_HEADER_LENGTH +
                                            ip_packet_length;
       register unsigned short checksum;
       register int i;

       /* Create the Ethernet frame header. */
       transmitt_buffer[0] = frame[6];
       transmitt_buffer[1] = frame[7];
       transmitt_buffer[2] = frame[8];
       transmitt_buffer[3] = frame[9];
       transmitt_buffer[4] = frame[10];
       transmitt_buffer[5] = frame[11];

       /* The driver will fill in the source MAC address. */

       transmitt_buffer[12] = 0x08;
       transmitt_buffer[13] = 0;

       /* Clear the IP header. */
       for(i = 0; i < IPv4_HEADER_LENGTH; i++)
        transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + i] = 0;

       /* Write IP header. */
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 0] = 0x45;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 8] = 64;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 9] = 1;

       /* IP packet length is the same as for the request. */
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 2] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 2];
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 3] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 3];

       /* Set source IP address. */
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 12] = IP_ADDRESS_0;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 13] = IP_ADDRESS_1;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 14] = IP_ADDRESS_2;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 15] = IP_ADDRESS_3;

       /* Copy destination IP address. */
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 16] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 12];
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 17] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 13];
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 18] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 14];
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH + 19] =
        frame[ETHERNET_FRAME_HEADER_LENGTH + 15];

       /* Write IP checksum. */
       checksum = calculate_checksum(
                   &transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH],
                   IPv4_HEADER_LENGTH);
       *((unsigned short*) &transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                                             10]) = 0xffff ^ checksum;

       /* Write an ICMP Echo reply header. */
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                        IPv4_HEADER_LENGTH] = 0;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                        IPv4_HEADER_LENGTH + 1] = 0;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                        IPv4_HEADER_LENGTH + 2] = 0;
       transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                        IPv4_HEADER_LENGTH + 3] = 0;

       /* Copy ICMP data. */
       for(i = 0; i < (ip_packet_length - IPv4_HEADER_LENGTH - 4); i++)
        transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                         IPv4_HEADER_LENGTH + 4 + i] =
         frame[ETHERNET_FRAME_HEADER_LENGTH + IPv4_HEADER_LENGTH + 4 + i];

       /* Write ICMP checksum. */
       checksum = calculate_checksum(
                   &transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                                     IPv4_HEADER_LENGTH],
                   ip_packet_length - IPv4_HEADER_LENGTH);
       *((unsigned short*) &transmitt_buffer[ETHERNET_FRAME_HEADER_LENGTH +
                                             IPv4_HEADER_LENGTH + 2]) =
                            0xffff ^ checksum;

       /* Clear any padding. */
       for(i = frame_length; i < 60; i++)
        transmitt_buffer[i] = 0;

       if (60 > frame_length)
        frame_length = 60;

       ne2k_send_frame(transmitt_buffer, frame_length);
      }
     }
    }
   }
  }
 }
}
