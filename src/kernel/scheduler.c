/*!
 * \file scheduler.c
 * \brief
 *  This file holds the scheduler code. 
 */

#include "kernel.h"
int last_switch = 0;

void
scheduler_called_from_system_call_handler(const register int schedule)
{
  if(schedule)
  {
    last_switch = system_time;
    cpu_private_data.thread_index = thread_queue_dequeue(&ready_queue);
    return;
  }
  else
  {
    if(system_time - last_switch > 10)
    {
      last_switch = system_time;
      thread_queue_enqueue(&ready_queue, cpu_private_data.thread_index);
      cpu_private_data.thread_index = thread_queue_dequeue(&ready_queue);
      return;
    }
  }
}

void
scheduler_called_from_timer_interrupt_handler(const register int thread_changed)
{
}
