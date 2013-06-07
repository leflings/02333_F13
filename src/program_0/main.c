/*! \file
 *      \brief The first user program - 
 *             the test case follows the structure of the producer consumer 
 *             example in Tanenbaum&Woodhull's book.
 *
 */

#include <scwrapper.h>

long buffer[16];
int  head=0;
int  tail=0;

long  counter=0;

long empty_semaphore_handle;
long mutex_semaphore_handle;
long full_semaphore_handle;


void consumer(void)
{
  char* chg_color = "\033[3_m";
 /* This is the consumer. */
 while(1)
 {

  long value;

  if (ALL_OK != semaphoredown(full_semaphore_handle))
  {
   prints("semaphoredown failed!\n");
   break;
  }
  if (ALL_OK != semaphoredown(mutex_semaphore_handle))
  {
   prints("semaphoredown failed!\n");
   break;
  }

  value=buffer[tail];
  tail=(tail+1)&15;

  if (ALL_OK != semaphoreup(mutex_semaphore_handle))
  {
   prints("semaphoreup failed!\n");
   break;
  }

  if (ALL_OK != semaphoreup(empty_semaphore_handle))
  {
   prints("semaphoreup failed!\n");
   break;
  }
  chg_color[3] = '1' + (value&7);
  prints(chg_color);
  printhex(value);
  prints("\n");

 }
 terminate();
}

void producer(void)
{
 /* This is the producer. */
  while(1)
 {


  if (ALL_OK != semaphoredown(empty_semaphore_handle))
  {
   prints("semaphoredown failed!\n");
   break;
  }

  if (ALL_OK != semaphoredown(mutex_semaphore_handle))
  {
   prints("semaphoredown failed!\n");
   break;
  }

  buffer[head]=counter++;
  head=(head+1)&15;

  if (ALL_OK != semaphoreup(mutex_semaphore_handle))
  {
   prints("semaphoreup failed!\n");
   break;
  }

  if (ALL_OK != semaphoreup(full_semaphore_handle))
  {
   prints("semaphoreup failed!\n");
   break;
  }

 }
 terminate();
}

void
main(int argc, char* argv[])
{
 register long  thread_stack;

 empty_semaphore_handle=createsemaphore(16);
 if (empty_semaphore_handle<0)
 {
  prints("createsemaphore failed!\n");
  return;
 }

 full_semaphore_handle=createsemaphore(0);
 if (full_semaphore_handle<0)
 {
  prints("createsemaphore failed!\n");
  return;
 }

 mutex_semaphore_handle=createsemaphore(1);
 if (mutex_semaphore_handle<0)
 {
  prints("createsemaphore failed!\n");
  return;
 }

 /* Create producer 1 */
 thread_stack=alloc(4096, 0);
 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(producer, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

 /* Create consumer 1 */
 thread_stack=alloc(4096, 0);
 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(consumer, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

 /* Create producer 2 */
 thread_stack=alloc(4096, 0);
 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(producer, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

 /* Create consumer 2 */
 thread_stack=alloc(4096, 0);
 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(consumer, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

}
