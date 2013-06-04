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
  prints("Running exec 0\n");
  if(0 != createprocess(1)) {
    prints("Hej");
  }
//  if(0 != createprocess(1)) {
//    prints("Hej");
//  }
  prints("Exec 0 finished\n");
  while(i);
//  if (0 != createprocess(1))
//   {
//    prints("createprocess of program 1 failed.\n");
//    return;
//   }
//   if (0 != createprocess(2))
//   {
//    prints("createprocess of program 2 failed.\n");
//    return;
//   }
//  if (0 != createprocess(1))
//   {
//    prints("createprocess of program 1 failed.\n");
//    return;
//   }
//   if (0 != createprocess(2))
//   {
//    prints("createprocess of program 2 failed.\n");
//    return;
//   }
//   while(1)
//   {
//    pause(100);
//    prints("Ping\n");
//   }
}
