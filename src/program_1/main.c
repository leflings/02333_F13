/*! \file
 *      \brief The second user program -
 *             You can add your own code here if you wish.
 */

#include <scwrapper.h>

void 
main(int argc, char* argv[])
{
  int i = 0;
  prints("Exec 1 start\n");

  for(i=0;i<8;i++) {
    if(0 != createprocess(2)) {
      prints("Failed creating process");
      break;
    }
  }
  prints("Exec 1 end\n");
}
