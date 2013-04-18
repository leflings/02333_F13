#include "kernel.h"

struct screen* const
screen_pointer = (struct screen*) 0xB8000;
struct cursor
{
  unsigned int row;
  unsigned int col;
  unsigned int background;
  unsigned int foreground;
} cur;

struct screen_position sp[25][80];
/*! Clears the screen. */
void clear_screen(void)
{
 register int row, column;

 for(row=0; row<MAX_ROWS; row++)
 {
  for(column=0; column<MAX_COLS; column++)
  {
    screen_pointer->positions[row][column].attribute = 7;
   screen_pointer->positions[row][column].character=' ';
  }
 }

 cur.col = 0;
 cur.row = 0;
 cur.background = 0;
 cur.foreground = 7;
}

/* Modify these function in task B7. */

void
kprints(const char* string)
{
 /* Loop until we have found the null character. */
 while(1)
 {
  register const char curr = *string++;
  int acc, tens, number;
  int arguments[2];
  char temp;

  if (curr)
  {
    switch (curr) {
    case '\n':
      lineBreak();
      break;
    case '\033':
      if(*string++ == '[') {
        while((temp = *string++)) {
          if(temp == 'J') {
            if(acc == 2) {
              clear_screen();
            }
            break;
          } else if (temp == 'H') {
            cur.row = number - 1;
            cur.col = acc - 1;
            break;
          } else if (temp == 'm') {
            if(acc == 0) {
              cur.background = 0; cur.foreground = 7;
            } else {
              cur.foreground = code2rgb(acc % 10);
            }
            break;
          } else if (temp == ';') {
            number = acc;
            acc = tens = 0;
          } else if(temp >= '0' && temp <= '9') {
            if(tens++) {
              acc *= 10;
            }
            acc += (temp - '0');
          }
        }
      }
      break;
    default:
      write(curr);
      break;
    }

   /*outb(0xe9, curr); */
  }
  else
  {
   return;
  }
 }
}

void
kprinthex(const register long value)
{
 const static char hex_helper[16]="0123456789abcdef";
 register int      i;

 /* Print each character of the hexadecimal number. This is a very inefficient
    way of printing hexadecimal numbers. It is, however, very compact in terms
    of the number of source code lines. */
 for(i=15; i>=0; i--)
 {
   write(hex_helper[(value>>(i*4))&15]);
   cursorForward();
 }
}

void lineBreak() {
  cur.row++;
  if(cur.row > 24) {
    scrollDown(5);
    cur.row -= 5;
  }
  cur.col = 0;
}

void cursorForward() {
  if(cur.col++ >= 80) {
    cur.row++;
    cur.col = 0;
  }
  if(cur.row > 24) {
    scrollDown(5);
    cur.row -= 5;
  }
}

void scrollDown(int lines) {
  int r, c, i;
  for(r = 0; r < 25; r++) {
    for(c = 0; c < 80; c++) {
      sp[r][c].attribute = screen_pointer->positions[r][c].attribute;
      sp[r][c].character = screen_pointer->positions[r][c].character;
    }
  }
  for(r = 0; r< 25 - lines; r++) {
    i = r+lines;
    for(c = 0; c < 80; c++) {
      screen_pointer->positions[r][c].attribute = sp[i][c].attribute;
      screen_pointer->positions[r][c].character = sp[i][c].character;
    }

  }
  for(r = 25-lines;r < 25; r++) {
    for(c = 0; c < 80; c++) {
      screen_pointer->positions[r][c].attribute = 7;
      screen_pointer->positions[r][c].character = ' ';
    }
  }
}

void write(char c) {
  cursorForward();
  screen_pointer->positions[cur.row][cur.col].character = c;
  screen_pointer->positions[cur.row][cur.col].attribute = (cur.background << 4) + cur.foreground;
}

int code2rgb(int code) {
  switch (code) {
  case 0: /* black */
    return 0;
  case 1: /* red */
    return 4;
  case 2: /* green */
    return 2;
  case 3: /* yellow */
    return 4 | 2;
  case 4: /* blue */
    return 1;
  case 5: /* magenta */
    return 4 | 1;
  case 6: /* cyan */
    return 2 | 1;
  case 7: /* white */
    return 4 | 2 | 1;
  default:
    return 7;
  }
}

