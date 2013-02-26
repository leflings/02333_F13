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
  int i;
  int pfe; /* page frame table entry */
  short pages_needed = ((length+4095)>>12);
  kprints("alloc called - pages needed: ");
  kprinthex(pages_needed);
  kprints("\n");
  int first_page_frame = find_contigous_region(pages_needed);
  if(first_page_frame < 0) {
    return ERROR;
  }
  for(i = 0; i < pages_needed; i++) {
    pfe = first_page_frame + i;
   /* kprints("Allocating page frame: ");
    kprinthex(pfe);
    kprints(" - #");
    kprinthex(i);
    kprints("\n");*/
    page_frame_table[pfe].start = first_page_frame;
    page_frame_table[pfe].owner = process;
    page_frame_table[pfe].free_is_allowed = (flags & ALLOCATE_FLAG_KERNEL);
  }
  kprints("Allocated from: ");
  kprinthex(first_page_frame);
  kprints("\n");
  return first_page_frame*4*1024;
}

/* Change this function in task B4. */
long
kfree(const register unsigned long address)
{
  int pf = address >> 12;
  int i;
    kprints("Free called with address: ");
    kprinthex(address);
    kprints(" - which is page_frame: ");
    kprinthex(pf);
    kprints("\n");
  if(page_frame_table[pf].free_is_allowed && page_frame_table[pf].owner != -1
      /* free is allowed and block is not allocated */
      && page_frame_table[pf].owner
        == thread_table[cpu_private_data.thread_index].data.owner
        /* block is owned by current process*/) {
    for(i = pf; i < MAX_NUMBER_OF_FRAMES && page_frame_table[i].start == pf; i++) {
      page_frame_table[i].owner = -1;
      page_frame_table[i].free_is_allowed = 1;
    }
    return ALL_OK;
  }
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
int find_contigous_region(short pages_needed) {
  short i, k;
  for(i = 0; i < MAX_NUMBER_OF_FRAMES; i++)
  {
    if(page_frame_table[i].owner == -1) {
      for(k=0; ((i+k) < MAX_NUMBER_OF_FRAMES) && page_frame_table[i+k].owner == -1; k++) {
      }
      if(k >= pages_needed) {
        /*kprints("Found sufficient frames at ");
        kprinthex(i);
        kprints(" - returning\n");*/

        return i;
      }
      i += k;
    }
  }
  return -1;

}
