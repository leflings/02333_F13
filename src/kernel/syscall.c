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
