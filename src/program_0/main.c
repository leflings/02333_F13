/*! \file
 *      \brief The first user program -
 *             You can add your own code here if you wish.
 *
 */

#include <scwrapper.h>

void
main(int argc, char* argv[])
{
  int i = 0;
  prints("Exec 0 start\n");
  if(0 != createprocess(1)) {
    prints("Creation of process 1 failed\n");
  }
  prints("Exec 0 finished\n");
}
