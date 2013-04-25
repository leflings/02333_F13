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

int count=0;
int max=15;

long empty_semaphore_handle;
long mutex_semaphore_handle;
long full_semaphore_handle;

long mutex_handle;
long empty_condition_variable;
long full_condition_variable;

void thread(void)
{
 /* This is the consumer. */
 while(1)
 {

  long value;
   pause(50);

   if(ALL_OK != mutex_lock(mutex_handle)) {
     prints("mutex_lock failed\n");
     break;
   }
   while(count == 0) {
     conditionvariablewait(empty_condition_variable, mutex_handle);
   }
   prints("-  ");
   printhex(count--);
   prints(" -> ");
   printhex(count);
   prints("\n");
   if(count == max-1) {
     conditionvariablesignal(full_condition_variable);
   }
   if(ALL_OK != mutex_unlock(mutex_handle)) {
     prints("mutex_unlock failed\n");
     break;
   }

 }
 terminate();
}

void thread3(void)
{
 /* This is the consumer. */
 while(1)
 {

  long value;
   pause(150);

   if(ALL_OK != mutex_lock(mutex_handle)) {
     prints("mutex_lock failed\n");
     break;
   }
   while(count == 0) {
     conditionvariablewait(empty_condition_variable, mutex_handle);
   }
   prints("-- ");
   printhex(count--);
   prints(" -> ");
   printhex(count);
   prints("\n");
   if(count == max-1) {
     conditionvariablesignal(full_condition_variable);
   }
   if(ALL_OK != mutex_unlock(mutex_handle)) {
     prints("mutex_unlock failed\n");
     break;
   }

 }
 terminate();
}

void thread2(void)
{
 /* This is the second producer. */
 while(1)
 {
   pause(100);

   if(ALL_OK != mutex_lock(mutex_handle)) {
     prints("mutex_lock failed\n");
     break;
   }
   while(count == max) {
     conditionvariablewait(full_condition_variable, mutex_handle);
   }
   prints("++ ");
   printhex(count++);
   prints(" -> ");
   printhex(count);
   prints("\n");
   if(count == 1) {
     conditionvariablesignal(empty_condition_variable);
   }
   if(ALL_OK != mutex_unlock(mutex_handle)) {
     prints("mutex_unlock failed\n");
     break;
   }

 }
 terminate();
}
void
main(int argc, char* argv[])
{
 register long  counter=0;
 register long  thread_stack;

 mutex_handle=createmutex();
 if(mutex_handle < 0) {
   prints("createmutex failes!\n");
   return;
 }

 full_condition_variable=createconditionvariable();
 if(full_condition_variable < 0) {
   prints("createconditionvariable failed!\n");
 }
 empty_condition_variable=createconditionvariable();
 if(empty_condition_variable < 0) {
   prints("createconditionvariable failed!\n");
 }

 thread_stack=alloc(4096, 0);

 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(thread, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

 thread_stack=alloc(4096, 0);

 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(thread2, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }

 thread_stack=alloc(4096, 0);

 if (0 >= thread_stack)
 {
  prints("Could not allocate the thread's stack!\n");
  return;
 }

 if (ALL_OK != createthread(thread3, thread_stack+4096))
 {
  prints("createthread failed!\n");
  return;
 }



 /* This is the producer. */
 while(1)
 {
   pause(counter++%200);

   if(ALL_OK != mutex_lock(mutex_handle)) {
     prints("mutex_lock failed\n");
     break;
   }
   while(count == max) {
     conditionvariablewait(full_condition_variable, mutex_handle);
   }
   prints("+  ");
   printhex(count++);
   prints(" -> ");
   printhex(count);
   prints("\n");
   if(count == 1) {
     conditionvariablesignal(empty_condition_variable);
   }
   if(ALL_OK != mutex_unlock(mutex_handle)) {
     prints("mutex_unlock failed\n");
     break;
   }


 }
}
