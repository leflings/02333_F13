/*!
 * \file
 * \brief
 *  This is the main source code for the kernel. Here all important variables
 *  will be initialized.
 */
#include "kernel.h"

/* Note: Look in kernel.h for documentation of global variables and functions. */

/* Variables */

struct context
saved_registers;

/* Function definitions */

void
kprints(const char* string)
{
 /* Loop until we have found the null character. */
 while(1)
 {
  register const char curr = *string++;

  if (curr)
  {
   outb(0xe9, curr);
  }
  else
  {
   return;
  }
 }
}

void
kprinthex(const register long value)
{
 const static char hex_helper[16]="0123456789abcdef";
 register int      i;

 /* Print each character of the hexadecimal number. This is a very inefficient
    way of printing hexadecimal numbers. It is, however, very compact in terms
    of number of source code lines. */
 for(i=15; i>=0; i--)
 {
  outb(0xe9, hex_helper[(value>>(i*4))&15]);
 }
}

void
initialize(void)
{
 kprints("\n\n\nThe kernel has booted!\n\n\n");
}

extern void
system_call_handler(void)
{
 system_call_implementation();
}
