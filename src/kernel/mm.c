/*! \file 
    \brief Holds the implementation of the memory sub-system. */

#include "kernel.h"
#include "mm.h"

/* Variable definitions. */

struct page_frame
page_frame_table[MAX_NUMBER_OF_FRAMES];

/* Set when the system is initialized. */
unsigned long memory_pages;

/* The following three variables are set by the assembly code. */
unsigned long first_available_memory_byte;

unsigned long memory_size;

unsigned long kernel_page_table_root;

/* Function definitions. */

/* Change this function in task B4. */
extern long
kalloc(const register unsigned long length,
       const register unsigned int  process,
       const register unsigned long flags)
{
 return ERROR;
}

/* Change this function in task B4. */
long
kfree(const register unsigned long address)
{
 return ERROR;
}

/* Change this function in task A4. */
extern void
update_memory_protection(const register unsigned long page_table,
                         const register unsigned long start_address,
                         const register unsigned long length,
                         const register unsigned long flags)
{
}

/* Change this function in task A4. */
extern void
initialize_memory_protection()
{
}

/* Put any code you need to add to implement tasks B4 and A4 here. */
