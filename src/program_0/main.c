/*! \file
 *      \brief The first user program -
 *             You can add your own code here if you wish.
 *
 */

#include <scwrapper.h>

void
main(int argc, char* argv[])
{
  prints("Running exec 0\n");
  if(0 != createprocess(1)) {
    prints("Hej");
  }
  terminate();
}
