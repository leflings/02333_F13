/*! \file syscall.c 

 This files holds the implementations of all system calls.

 */

#include "kernel.h"

int
system_call_implementation(void)
{
 register int schedule=0;
 /*!< System calls may set this variable to 1. The variable is used as
      input to the scheduler to indicate if scheduling is necessary. */
 switch(SYSCALL_ARGUMENTS.rax)
 {
  case SYSCALL_PRINTS:
  {
   kprints((char*) (SYSCALL_ARGUMENTS.rdi));
   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_PRINTHEX:
  {
   kprinthex(SYSCALL_ARGUMENTS.rdi);
   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_DEBUGGER:
  {
   /* Enable the bochs iodevice and force a return to the debugger. */
   outw(0x8a00, 0x8a00);
   outw(0x8a00, 0x8ae0);

   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_CREATEPROCESS:
  {
    int process_number;
    int thread_number = -1;
    struct prepare_process_return_value prepare_process_ret_val;

    long int executable_number = SYSCALL_ARGUMENTS.rdi;

    /* Search process_table for a slot */
    grab_lock_rw(&process_table_lock);
    for(process_number = 0;; process_number++)
    {
      if(process_number < MAX_NUMBER_OF_PROCESSES
                      && process_table[process_number].threads > 0)
      {
        continue;
      }
      break;
    }

    if(process_number == MAX_NUMBER_OF_PROCESSES)
    {
      SYSCALL_ARGUMENTS.rax = ERROR;
      kprints("Max number of processes reached\n");
    }
    else
    {
      prepare_process_ret_val = prepare_process(
                      executable_table[executable_number].elf_image,
                      process_number,
                      executable_table[executable_number].memory_footprint_size);

      if(0 == prepare_process_ret_val.first_instruction_address)
      {
              kprints("Error starting image\n");
      }

      grab_lock_rw(&thread_table_lock);

      thread_number = allocate_thread();

      if(thread_number == -1) {
        kprints("Error allocating thread\n");
        SYSCALL_ARGUMENTS.rax = ERROR;
      } else {
        process_table[process_number].parent = thread_table[get_current_thread()].data.owner;
        process_table[process_number].threads = 1;
        thread_table[thread_number].data.owner = process_number;


        SYSCALL_ARGUMENTS.rax = ALL_OK;
      }

      release_lock(&thread_table_lock);

    }
    release_lock(&process_table_lock);
    if(thread_number != -1) {

      process_table[process_number].page_table_root = prepare_process_ret_val.page_table_address;
        thread_table[thread_number].data.registers.integer_registers.rflags = 0x200;
        thread_table[thread_number].data.registers.integer_registers.rip =
                        prepare_process_ret_val.first_instruction_address;

      grab_lock_rw(&ready_queue_lock);
      thread_queue_enqueue(&ready_queue, thread_number);
      release_lock(&ready_queue_lock);
    }
    break;
  }

  case SYSCALL_TERMINATE:
  {
    grab_lock_rw(&process_table_lock);
    grab_lock_rw(&thread_table_lock);
    int i;
    int owner_process = thread_table[get_current_thread()].data.owner;
    int parent_process = process_table[owner_process].parent;
    int tmp_thread;

    thread_table[get_current_thread()].data.owner = -1;
    if(--process_table[owner_process].threads < 1)
    {
      cleanup_process(owner_process);
    }


    release_lock(&thread_table_lock);
    release_lock(&process_table_lock);
    schedule = 1;

    break;

  }

  /* Do not touch any lines above or including this line. */

  /* Add the implementation of more system calls here. */


  /* Do not touch any lines below or including this line. */
  default:
  {
   /* No system call defined. */
   SYSCALL_ARGUMENTS.rax = ERROR_ILLEGAL_SYSCALL;
  }
 }

 return schedule;
}
