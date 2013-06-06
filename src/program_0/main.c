/*! \file
 *      \brief The first user program - tests the memory allocation routines
 *             by allocating and de-allocating random memory blocks.
 *
 */
#include <scwrapper.h>


int
main(int argc, char* argv[])
{
  int i;
  prints("Exec 0 start\n");
  for(i=0; i<6; i++) {
    if(0 != createprocess(1)) {
      prints("Failed creating process 1\n");
      break;
    }
  }
  prints("Exec 0 end\n");

}
