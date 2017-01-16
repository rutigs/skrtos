#ifndef KBD_H
#define KBD_H

#include <xeroskernel.h>
#include <i386.h>

#define MAX_KEYBOARD   2
#define MAX_BUFF_CHAR  4
#define MAX_ASCII      127

/* keyboard state Constants */
#define CONTROL_D     4           /* Integer code for CONTROL_D   */
#define EOF_CODE      CONTROL_D   /* Default EOF_CODE is Control-D   */
#define NEW_LINE      

/* Commands */
#define CHANGE_EOF    53
#define ECHO_OFF      55
#define ECHO_ON       56

/* Keyboard Device Constants */
#define READY         0x01       /* The Keyboard has data        */
#define CONTROL_PORT  0x64       /* The port to read status from */
#define DATA_PORT     0x60       /* The port to read data from   */


typedef struct struct_kbdst kbdst;

struct struct_kbdst {
  Bool          echo_on;           /* Flag to indicate if echo is turned on */
  unsigned int  end_of_file;       /* Integer code of end of file character */
  Bool          disabled;          /* Flag for when EOF is reached and device disabled */
};

/* Keyboard State Table */
kbdst keyboard_state[MAX_KEYBOARD];


/* Upper Half Routines */
int kbd_open( pcb *proc, devsw * dev );
int kbd_close( pcb *proc, devsw * dev );
int kbd_read( pcb *proc, devsw * dev, void *buf, int buflen);
int kbd_write( pcb *proc, devsw * dev, void *buf, int buflen);
int kbd_ioctl( pcb *proc, devsw * dev ,unsigned long command, va_list varargs);

/* from given scan code file */

#define KEY_UP   0x80            /* If this bit is on then it is a key   */
                                 /* up event instead of a key down event */

/* Control code */
#define LSHIFT  0x2a
#define RSHIFT  0x36
#define LMETA   0x38

#define LCTL    0x1d
#define CAPSL   0x3a


/* scan state flags */
#define INCTL           0x01    /* control key is down          */
#define INSHIFT         0x02    /* shift key is down            */
#define CAPSLOCK        0x04    /* caps lock mode               */
#define INMETA          0x08    /* meta (alt) key is down       */
#define EXTENDED        0x10    /* in extended character mode   */ 

#define EXTESC          0xe0    /* extended character escape    */ 
#define NOCHAR  256  



/*  Normal table to translate scan code  */
unsigned char   kbcode[] = { 0,
          27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
         '0',  '-',  '=', '\b', '\t',  'q',  'w',  'e',  'r',  't',
         'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',
         's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',
         '`',    0, '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',
         ',',  '.',  '/',    0,    0,    0,  ' ' };

/* captialized ascii code table to tranlate scan code */
unsigned char   kbshift[] = { 0,
           0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',
         ')',  '_',  '+', '\b', '\t',  'Q',  'W',  'E',  'R',  'T',
         'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',    0,  'A',
         'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
         '~',    0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',
         '<',  '>',  '?',    0,    0,    0,  ' ' };
/* extended ascii code table to translate scan code */
unsigned char   kbctl[] = { 0,
           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
           0,   31,    0, '\b', '\t',   17,   23,    5,   18,   20,
          25,   21,    9,   15,   16,   27,   29, '\n',    0,    1,
          19,    4,    6,    7,    8,   10,   11,   12,    0,    0,
           0,    0,   28,   26,   24,    3,   22,    2,   14,   13 };



#endif
