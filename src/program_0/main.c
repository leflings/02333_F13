/*! \file \brief The first user program - it tests the video routines
 *      by writing to different places on the screen using different
 *      colors.
 */

#include <scwrapper.h>

int main()
{
 /* Clear screen escape sequence */
 const char *cls = "\033[2J";

 /* Set color to red */
 const char *red = "\033[31m";

 /* Set color to green */
 const char *green = "\033[32m";

 /* Set color to blue */
 const char *blue = "\033[34m";

 /* Reset color */
 const char *reset = "\033[0m";

 /* Move cursor, the underscores are replaced in the loop */
 char g[] = "\033[_;_H";

 int n;

 prints(cls);

 for (n=0; n < 9; n++)
 {
  /* Goto row, column. Note that the offsets are 1-based and we
   * modify the string in situ! */
  g[2] = '0' + n + 1;
  g[4] = '0' + (n % 3) + 1;

  prints(g);

  /* Print out some colorful text! */
  prints(red);
  prints("Red ");

  prints(green);
  prints("Green ");

  prints(blue);
  prints("Blue\n");
 }

 /* Reset the color to defaults and print again. */
 prints(reset);
 prints("Back to normal\n");

 /* Test case for across multiple rows */
 prints("\033[16;40H");
 prints("Test: This is a very long string, broken over two lines");

 /* Test case for printhex */
 prints("\033[14;40H");
 prints("Printhex: ");
 printhex(256);

 prints("\033[13;40H");
 prints("In-string\033[32m-shift\033[31m-of-co\033[0mlors");

 /* We're done */
 prints("\033[19;1H");
 while (1)
 {
  register long scan_code=getscancode();
  if (0x1c==scan_code)
  {
   prints("Enter pressed\n ");
  } else if (0x9c==scan_code)
  {
   prints("Enter released\n");
  }
 }
}
