/*! \file
 *      \brief The second user program -
 *             You can add your own code here if you wish.
 */

#include <scwrapper.h>

void 
main(int argc, char* argv[])
{
  int i = 0;
//  prints("Exec 1 start\n");

  for(i=0;i<8;i++) {
//    prints("Creating -> ");
    if(0 != createprocess(2)) {
      prints("Failure 1\n");
    }
//    prints("creeated...\n");
  }
//  prints("Terminating p1\n");
  while(1);
//  terminate();
//  if(0 != createprocess(2)) {
//    prints("Failure 4\n");
//  }
//  if(0 != createprocess(2)) {
//    prints("Failure 5\n");
//  }
//  if(0 != createprocess(2)) {
//    prints("Failure 6\n");
//  }

//  prints("Exec 1 end\n");

//  unsigned int reps = 0;
//  while(1){
//    unsigned long int i = 0;
//    if(reps++ > 5)
//    {
//      while(i++ < 1000000000);
//      reps = 0;
//    }
//    else
//    {
//      pause(5);
//    }
//    prints("Pong\n");
//  }
}
