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
  int pfte; /* page frame table entry */
  long return_value;
  int first_page_frame;
  short pages_needed = ((length+4095)>>12);

  grab_lock_rw(&page_frame_table_lock);
//  kprinthex(memory_size);
//  kprints("\n");


  first_page_frame = find_contiguous_region(pages_needed);
  if(first_page_frame < 0)
  {
    return_value = ERROR;
    kprints("Failed allocating pages: ");
    kprinthex(pages_needed);
    kprints("\n");
  }
  else
  {
    for(i = 0; i < pages_needed; i++)
    {
      pfte = first_page_frame + i;
      page_frame_table[pfte].start = first_page_frame;
      page_frame_table[pfte].owner = process;
      page_frame_table[pfte].free_is_allowed = 1; /*!(flags & ALLOCATE_FLAG_KERNEL);*/
    }
    return_value = first_page_frame*4*1024;
  }

    kprints("Return value: ");
    kprinthex(return_value);
    kprints("\n");
  release_lock(&page_frame_table_lock);

  return return_value;
}

/* Change this function in task B4. */
long
kfree(const register unsigned long address)
{
  int pf = address >> 12;
  int i;
  long return_value;

  grab_lock_rw(&page_frame_table_lock);
  if( /* free is allowed */
      page_frame_table[pf].free_is_allowed
      /* block is allocated  */
      && page_frame_table[pf].owner != -1
      /* block is owned by current process*/
      && page_frame_table[pf].owner
        == thread_table[get_current_thread()].data.owner)
    // TODO: Need read lock for thread table?
  {
    for(i = pf;
        i < MAX_NUMBER_OF_FRAMES && page_frame_table[i].start == pf;
        i++)
    {
      page_frame_table[i].owner = -1;
      page_frame_table[i].free_is_allowed = 1;
    }
    return_value = ALL_OK;
  }
  else
  {
    return_value = ERROR;
  }

  release_lock(&page_frame_table_lock);

  return return_value;
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
int find_contiguous_region(short pages_needed) {
  short i, k;
  for(i = 0; i < MAX_NUMBER_OF_FRAMES; i++)
  {
    if(page_frame_table[i].owner == -1)
    {
      for(k=0;
          k < pages_needed
          && i+k < MAX_NUMBER_OF_FRAMES
          && page_frame_table[i+k].owner == -1;
          k++)
      {}
      if(k >= pages_needed)
      {
        return i;
      }
      /* No need to have outer loop check those pages */
      i += k;
    }
  }
  return -1;

}
