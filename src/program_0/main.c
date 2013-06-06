/*! \file
 *      \brief The first user program - tests the memory allocation routines
 *             by allocating and de-allocating random memory blocks.
 *
 */
#include <scwrapper.h>

void
main(int argc, char* argv[])
{
  int i;
  for(i = 0; i < 3; i++) {
  if(0 != createprocess(1)) {
    prints("error creating process 1");
  }
  }
  terminate();

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
