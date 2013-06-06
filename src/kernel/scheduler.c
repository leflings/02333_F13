/*!
 * \file scheduler.c
 * \brief
 *  This file holds the scheduler code. 
 */

#include "kernel.h"
#define MAX_TICKS 10

void
scheduler_called_from_system_call_handler(const register int schedule)
{
  int next_thread;
  if(schedule)
    {
      /* in this case we must reschedule */

      grab_lock_rw(&ready_queue_lock);
      /* either we'll get next thread or we'll get -1
       * no need to check for empty and/or then dequeue */
      next_thread = thread_queue_dequeue(&ready_queue);
      release_lock(&ready_queue_lock);


      CPU_private_table[get_processor_index()].thread_index = next_thread;

      if(next_thread != -1)
      {
        CPU_private_table[get_processor_index()].page_table_root = process_table[thread_table[next_thread].data.owner].page_table_root;
        CPU_private_table[get_processor_index()].ticks_left_of_time_slice = MAX_TICKS;
      }
#if DEBUG_ON
        if(next_thread != -1) {
          kprints("System call schedule on CPU:  ");
        } else {
          kprints("Idle thread scheduled on CPU: ");
        }
        kprinthex(get_processor_index());
        kprints(" -> thread: ");
        kprinthex(get_current_thread());
        kprints("\n");
#endif


      return;
    }
}

void
scheduler_called_from_timer_interrupt_handler(const register int thread_changed)
{
  if(thread_changed)
   {
     /* We must reset the time slice */
     CPU_private_table[get_processor_index()].ticks_left_of_time_slice = MAX_TICKS;

#if DEBUG_ON
        kprints("Forced thread scheduled on CPU: ");
        kprinthex(get_processor_index());
        kprints(" -> thread: ");
        kprinthex(CPU_private_table[get_processor_index()].thread_index);
        kprints("\n");
#endif
   }
   else if(get_current_thread() != -1)
   {
     grab_lock_rw(&ready_queue_lock);
     /* decrement time slice and check it it's expired */
     if(--CPU_private_table[get_processor_index()].ticks_left_of_time_slice < 1)
     {
       /* it's expired */
       if(!thread_queue_is_empty(&ready_queue))
       {
         int next_thread;
         /* we only want to fetch a new thread, if there's actually one ready.
          * Otherwise we just expand the current ones' time slice */
         thread_queue_enqueue(&ready_queue, CPU_private_table[get_processor_index()].thread_index);
         CPU_private_table[get_processor_index()].thread_index = thread_queue_dequeue(&ready_queue);
         CPU_private_table[get_processor_index()].page_table_root = process_table[thread_table[get_current_thread()].data.owner].page_table_root;


#if DEBUG_ON
        kprints("Preemptive schedule on CPU: ");
        kprinthex(get_processor_index());
        kprints(" -> thread: ");
        kprinthex(get_current_thread());
        kprints("\n");
#endif

       }
       CPU_private_table[get_processor_index()].ticks_left_of_time_slice = MAX_TICKS;

     }

     if(!thread_queue_is_empty(&ready_queue)) {
       int i;
       for(i = 0; i < 4; i++) {
         if(CPU_private_table[i].thread_index == -1) {
           send_IPI(i, 240);
           break;
         }
       }
     }
     release_lock(&ready_queue_lock);
   }
}
