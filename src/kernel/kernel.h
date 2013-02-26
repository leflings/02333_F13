/*!
 \file kernel.h
 This file holds kernel wide macros and declarations.
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <sysdefines.h>

/* Type definitions */

/* Macros */

#define SYSCALL_ARGUMENTS (thread_table[current_thread].data.registers.\
                           integer_registers)
/*!< Macro used in the system call switch to access the arguments to the 
     system call. */

#define KERNEL_VERSION        (0x0000000100000000)
/*!< Kernel version number. */

#define MAX_NUMBER_OF_PROCESSES (16)
/*!< Size of the process_table. */
#define MAX_NUMBER_OF_THREADS   (256)
/*!< Size of the thread_table. */

/* Type declarations */

/*! Defines an execution context. */
struct context
{
 unsigned char fpu_context[512];
 /*!< Stores the fpu/mmx/sse registers */
 struct
 {
  long    rax;
  long    rbx;
  long    rcx;
  long    rdx;
  long    rdi;
  long    rsi;
  long    rbp;
  long    rsp;
  long    r8;
  long    r9;
  long    r10;
  long    r11;
  long    r12;
  long    r13;
  long    r14;
  long    r15;
  long    rflags;	/*!< status flags */
  long    rip;		/*!< instruction pointer */
 }        integer_registers;
 /*!< Stores all user visible integer registers. The segment registers are not
      stored as the user mode processes can only use a code and a data segment
      descriptor. We assume that the cs is set to the code and all the rest
      of the segment registers are set to the data segment all the time. */
};


/*! Defines a thread. */
union thread
{
 struct
 {
  struct context registers;     /*!< The context of the thread. Note: the
                                     context of the thread could include more
                                     than the accessible registers. */
  int            owner;         /*!< The index identifies the process that owns
                                     this thread. The owner can be retrieved from
                                     the process_table by using the index.  */
 }               data;
 char            padding[1024];
};

/*! Defines a process. */
struct process
{
 int             threads;        /*!< The number of threads running in this
                                      process. */
 int             parent;         /*!< This is an index into process_table. The
                                      index corresponds to the parent process. */
};

/* ELF image structures. The names from the ELF64 specification are used and
   the structs are derived from the ELF64 specification. */

#define EI_MAG0       0  /*!< The first four bytes in an ELF image is
                              0x7f 'E' 'L' 'F'. */
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4  /*!< The specific ELF image class. */
#define EI_DATA       5  /*!< Describes if the image is big- or little-
                              endian. */
#define EI_VERSION    6  /*!< The version of the ELF specification the
                              image adheres to. */
#define EI_OSABI      7  /*!< Type of OS the image can be run on. */
#define EI_ABIVERSION 8  /*!< Version of the ABI used. */
#define EI_PAD        9  /*!< First unused byte in the identification array.
                          */
#define EI_NIDENT     16 /*!< Number of entries in the identification array */

/*! Defines an ELF64 file header. */
struct Elf64_Ehdr
{
 unsigned char e_ident[EI_NIDENT]; /*!< Array of bytes that shows that this
                                        is an ELF image and what type of ELF
                                        image it is. */
 short         e_type;             /*!< The type of ELF executable image. */
 short         e_machine;          /*!< Identifies the type of machine that
                                        the image can execute on. */
 int           e_version;          /*!< The version of the ELF specification the
                                        image adheres to. */
 long          e_entry;            /*!< Start address of the executable. */
 long          e_phoff;            /*!< The offset into the image where the
                                        program header table is found. */
 long          e_shoff;            /*!< The offset into the image where the
                                        program header table is found. */
 int           e_flags;            /*!< Flags that are machine specific.
                                        These can be used to differentiate
                                        between similar machines. */
 short         e_ehsize;           /*!< The size, in bytes, of the header. */
 short         e_phentsize;        /*!< The size, in bytes, of each entry in
                                        the program header table. */
 short         e_phnum;            /*!< The size, in entries, of the program
                                        header table. */
 short         e_shentsize;        /*!< The size, in bytes, of each entry in
                                        the section header table. */
 short         e_shnum;            /*!< The size, in entries, of the section
                                        header table. */
 short         e_shstrndx;         /*!< The index, into the section table,
                                        of the section name string table.*/
};

/*! Defines an entry in the ELF program header table. Each entry corresponds
    to a segment. */
struct Elf64_Phdr
{
 int  p_type;   /*!< Segments can have several types. p_type holds the type. */
 int  p_flags;  /*!< The attribute flags of the segment. */
 long p_offset; /*!< Offset into the image of the first byte of the
                     segment */
 long p_vaddr;  /*!< The (virtual) address to which the segment is to be
                     loaded. */
 long p_paddr;  /*!< Not used. */
 long p_filesz; /*!< The number of bytes the segment occupies in the
                     image. */
 long p_memsz;  /*!< The number of bytes the segment occupies in memory. */
 long p_align;  /*!< The alignment the segment should have in memory. This
                     field is currently being ignored. */
};


/* Values used in p_type */
#define PT_NULL 0 /*!< The entry is not used. */
#define PT_LOAD 1 /*!< The segment can be loaded into memory. */
#define PT_PHDR 6 /*!< The segment only hold a program header table. */

/* Values used in p_flags */
#define PF_X        0x1        /*!< Segment can be executed.*/
#define PF_W        0x2        /*!< Segment can be written. */
#define PF_R        0x4        /*!< Segment can be read. */
#define PF_MASKPERM 0x0000FFFF /*!< Used to mask the permission bits */

/* Data structures describing the executable images embedded in the kernel
   image. */

/*! Defines an executable program. */
struct executable
{
 const struct Elf64_Ehdr* elf_image;             /*!< The start of the ELF
                                                      file header. */
 unsigned long            memory_footprint_size; /*!< Size in bytes of the
                                                      program's memory foot
                                                      print when loaded. */
};

/*! Defines an executable image embedded into the kernel image. The executable
    images are linked together into a linked list. The linked list is built at
    link time, see the kernel link script. */
struct executable_image
{
 const struct executable_image* const next; /*!< Points to the next
                                                 executable image. The last
                                                 executable image in the list
                                                 will have a next pointer
                                                 equal to 0, i.e., null. */
 const struct Elf64_Ehdr              elf_image; /*!< The ELF image file header. */
};

/* Variable declarations */

extern union thread
thread_table[MAX_NUMBER_OF_THREADS];
/*!< Array holding all threads in the systems. */

extern struct process
process_table[MAX_NUMBER_OF_PROCESSES];
/*!< Array holding all processes in the system. */

extern struct executable
executable_table[MAX_NUMBER_OF_PROCESSES];
/*!< Array holding descriptions of all executable programs. */

extern int
executable_table_size;
/*!< The number of executable programs in the executable_table */

extern int
current_thread;
/*!< The index, into thread_table, of the currently running thread. */

extern const struct executable_image*
ELF_images_start;
/*!< The first executable image in the linked list of executable images. */

extern const char*
ELF_images_end;
/*!< The address of the first byte after the end of the last ELF image. This
     variable is used for sanity checking. */

extern unsigned long
first_available_memory_byte;
/*!< The address of the first memory byte not used by any running process.
     This variable is used to implement a crude memory allocation scheme. */

extern unsigned long
memory_size;
/*!< Size, in bytes, of the memory. */

/* Function declarations */

/*! Helper struct that is used to return values from prepare_process. */
struct prepare_process_return_value
{
 unsigned long first_instruction_address
  /*!< The address of the first instruction in the prepared process image. */;
 unsigned long page_table_address
  /*!< Will not be used until databar assignment 4. */;
};

/*! Copies an ELF image to memory and prepares a process. prepare_process
    does some checks to avoid that corrupt images gets copied to memory.
    However, the checks are not as thorough as the check in initialize.
    \return A prepare_process_return_value struct holding the first address
            of the process image and an address to the page table for
            the process. */
extern struct prepare_process_return_value
prepare_process(const struct Elf64_Ehdr* elf_image
                 /*!< Points to the ELF image to copy. */,
                const unsigned int       process
                 /*!< The index of the process that is to be created. */,
                unsigned long            memory_footprint_size
                 /*!< Holds the maximum amount of memory, in bytes,
                      the image is allowed to use. */);

/*! This is the last thing that is run when a process terminates. It performs
    all cleanup activities. Right now it is empty but in assignment 4 it will,
    for example, release the memory owned by the process. */
extern void
cleanup_process(const int process /*!< The index, into process_table, of the
                                       terminating process. */);

/*! This function initializes the kernel after the assembly code portion has
    set the system and the CPU up. */
extern void
initialize(void);

/*! Allocate one thread. The allocated thread is not initialized.
    Rip and rflags need to be set for the thread to start properly.
    \return An index into thread_table or -1 if no thread could be allocated.*/
extern inline int
allocate_thread(void);

/*! This function gets called from the assembly code and responds to the
    system calls. */
extern void
system_call_handler(void);

/*! This function gets called from the system call handler and implements
    system calls. The return value is not used until assignment 3. */
extern int
system_call_implementation(void);

/*! Outputs a string to the bochs console. */
extern void
kprints(const char* const string
        /*!< points to a null terminated string */
        );

/*! Prints a long formatted as a hexadecimal number to the bochs console. */
extern void
kprinthex(const register long value
          /*!< the value to be written */);

/*! Wrapper for a byte out instruction. */
inline static void
outb(const register unsigned short port_number, 
     const register unsigned char output_value)
{
 __asm volatile("outb %%al,%%dx" : : "d" (port_number), "a" (output_value));
}

/*! Wrapper for a word out instruction. */
inline static void
outw(const register unsigned short port_number, 
     const register unsigned short output_value)
{
 __asm volatile("outw %%ax,%%dx" : : "d" (port_number), "a" (output_value));
}

#endif
