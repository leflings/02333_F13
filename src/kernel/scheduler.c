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
  if(schedule)
  {
    /* in this case we must reschedule */
    if(!thread_queue_is_empty(&ready_queue))
    {
      /* a thread is ready */
      cpu_private_data.ticks_left_of_time_slice = MAX_TICKS;
      cpu_private_data.thread_index = thread_queue_dequeue(&ready_queue);
    }
    else
    {
      /* no thread ready, go to idle */
      cpu_private_data.ticks_left_of_time_slice = 9999;
      cpu_private_data.thread_index = -1;
    }
    return;
  }
}

void
scheduler_called_from_timer_interrupt_handler(const register int thread_changed)
{
  if(thread_changed)
  {
    /* We must reset the time slice */
    cpu_private_data.ticks_left_of_time_slice = MAX_TICKS;
    return;
  }
  else
  {
    /* decrement time slice and check it it's expired */
    if(--cpu_private_data.ticks_left_of_time_slice < 1)
    {
      /* it's expired */
      if(!thread_queue_is_empty(&ready_queue))
      {
        /* we only want to fetch a new thread, if there's actually one ready.
         * Otherwise we just expand the current ones' time slice */
        thread_queue_enqueue(&ready_queue, cpu_private_data.thread_index);
        cpu_private_data.thread_index = thread_queue_dequeue(&ready_queue);
      }
      cpu_private_data.ticks_left_of_time_slice = MAX_TICKS;
    }
  }
}
