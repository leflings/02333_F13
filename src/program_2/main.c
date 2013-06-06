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
  unsigned long i;
  prints("Exec 2 start\n");
  while(i < 400000) {
    i++;
  }
  prints("Exec 2 end\n");
}
