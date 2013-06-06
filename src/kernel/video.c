#include "kernel.h"

#define SCROLL_OFF (1)

struct screen* const
screen_pointer = (struct screen*) 0xB8000;

/* Structure to imitate a cursor which can hold the row and column position
  and the background/foreground of that position.
 */
struct cursor
{
  unsigned int row;
  unsigned int col;
  unsigned int background;
  unsigned int foreground;
} cur;

/*! Clears the screen. */
void clear_screen(void)
{
 register int row, column;

 for(row=0; row<MAX_ROWS; row++)
 {
  for(column=0; column<MAX_COLS; column++)
  {
    screen_pointer->positions[row][column].attribute = 7; /* clear colors */
    screen_pointer->positions[row][column].character=' ';
  }
 }

 /* reset the cursor */
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
  int accumulator; /* accumulator for argument scanning */
  int tens;        /* used to keep track of number of 10^0, 10^1, 10^2 etc. */
  int argument;    /* used to store first argument, if multiple */
  char temp;       /* holds temp character*/

  if (curr)
  {
    switch (curr) {
    case '\n':
      line_break();
      break;

    /* escape character */
    case '\033':
      /* should be followed by [ */
      if(*string++ == '[') {
        accumulator = tens = argument = 0;
        /* We know escape sequence has been correctly entered */

        /* Continue forward until null character or end of sequence */
        while((temp = *string++)) {

          /* Clear screen (presumably) received) */
          if(temp == 'J') {
            if(accumulator == 2) {
              clear_screen();
            }
            break;
          }

          /* Cursor home received */
          else if (temp == 'H') {
            if(argument >= 1 && argument <= 25 && accumulator >= 1 && accumulator <= 80) {
              cur.row = argument - 1; /* adjust to zero-indexing */
              cur.col = accumulator - 1;
            }
            break;
          }

          /* Set attribute mode received */
          else if (temp == 'm') {
            if(accumulator == 0) {
              /* Reset */
              cur.background = 0; cur.foreground = 4|2|1;
            } else {
              /* Adjust color of the cursor */
              cur.foreground = code2rgb(accumulator % 10);
            }
            break;
          }

          /* Argument seperator */
          else if (temp == ';') {
            /* Save and reset accumulator */
            argument = accumulator;
            accumulator = tens = 0;
          }

          /* parses numbers */
          else if(temp >= '0' && temp <= '9') {
            if(tens++) {
              accumulator *= 10;
            }
            accumulator += (temp - '0'); /* add decimal value of number char */
          }
        }
      }
      break;

    default:
      write(curr);
      break;
    }

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
 }
}

void write(char c) {
  screen_pointer->positions[cur.row][cur.col].character = c;
  screen_pointer->positions[cur.row][cur.col].attribute = (cur.background << 4) + cur.foreground;
  cursor_forward();
}

void line_break() {
  if(++cur.row >= MAX_ROWS) {
    scroll_down(SCROLL_OFF);
    cur.row -= SCROLL_OFF;
  }
  cur.col = 0;
}

void cursor_forward() {
  if(++cur.col >= MAX_COLS) {
    line_break();
   }
}

void scroll_down(int lines) {
  int r, c, i;
  for(r=0; r < MAX_ROWS-lines;r++) {
    i = r+lines;
    for(c=0; c < MAX_COLS; c++) {
      screen_pointer->positions[r][c].attribute= screen_pointer->positions[i][c].attribute;
      screen_pointer->positions[r][c].character = screen_pointer->positions[i][c].character;
    }
  }
  for(r=MAX_ROWS-lines; r < MAX_ROWS;r++) {
    for(c=0; c < MAX_COLS; c++) {
      screen_pointer->positions[r][c].attribute = 7;
      screen_pointer->positions[r][c].character = ' ';
    }
  }
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

