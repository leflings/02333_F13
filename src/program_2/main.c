/*! \file
 *      \brief The third user program - 
 *             You can add your own code here if you wish.
 *             
 *
 */

#include <scwrapper.h>

void
main(int argc, char* argv[])
{
  unsigned long i, k, j;
//  signed long i = 0;
//  prints("Exec 2 start: ");
//  printhex(time());
//  prints("\n");
//  prints("process 2\n");
  while(i < 100000000000000) {
    for(k=0;k < 100000000000000;k++)
      j++;
    for(k=0;k < 100000000000000;k++)
      j--;
    for(k=0;k < 100000000000000;k++)
      j++;
    for(k=0;k < 100000000000000;k++)
      j--;
    i++;
  }
  while(1);
//  prints("Exec 2 end:   ");
//  printhex(time());
//  prints("\n");
//  terminate();


//  unsigned reps = 0;
//  while(1)
//   {
//    unsigned long curr_time = 0;
//    if(reps++ > 5) {
//      pause(10);
//      reps = 0;
//    } else {
//      while(curr_time++ < 100000000);
//    }
//    prints("Pang\n");
//   }
}
