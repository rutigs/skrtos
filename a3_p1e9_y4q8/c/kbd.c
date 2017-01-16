/*
 * kbd.c - keyboard drivers
 *
 * - int kbd_open( pcb *proc, devsw * dev );
 *
 *
 * - int kbd_close( pcb *proc, devsw * dev );
 *
 *
 * - int kbd_read( pcb *proc, devsw * dev, void *buf, int buflen);
 *
 *
 * - int kbd_write( pcb *proc, devsw * dev, void *buf, int buflen);
 *
 *
 * - int kbd_ioctl( pcb *proc, devsw * dev ,unsigned long command, va_list varargs);
 *
 *
 * - void notify_incoming_intr( unsigned int scan );
 *     Notify device upper half of incoming data. Update buffers
 *
 * - void keyboard_int_handler( void )
 *     ISR for keyboard interrupt. Reads data from keyboard if ready
 *     and notifies device upper half.
 *
 * - void unblock_proc( void );
 *     Put blocked_read back on ready queue and set
 *     return code to curr_buflen
 *
 * - void reset_state( kbdst* keyboard_state, Bool echo_on )
 *     Resets the keyboard state to default
 */

#include <kbd.h>

/* Flag to keep track of which keyboard is open */
static devsw *current_keyboard = (devsw *) NULL_DEVICE;


static int          scan_state;        /* the scan state of the keyboard    */
static pcb         *blocked_read;      /* Process blocked on read           */
static int          curr_buflen;       /* Current length of process buffer  */
static char         kernel_buffer[MAX_BUFF_CHAR]; /* Internal buffer        */
static unsigned int kernel_buflen;     /* Current length of internal buffer */



/* Internal Routines */
static void           notify_incoming_intr( unsigned int scan );
static void           reset_state( kbdst* keyboard_state, Bool echo_on );
static unsigned int   kbtoa( unsigned char code );
static int            extchar(unsigned char code);

/*
 * Initialize kernel data structures for devices
 */
void keyboards_init( void ) {
  devsw *reg_kbd = &dev_tab[0];
  reg_kbd->dev_name = "keyboard";
  reg_kbd->dev_num = 0;
  reg_kbd->dev_open = kbd_open;
  reg_kbd->dev_close = kbd_close;
  reg_kbd->dev_read = kbd_read;
  reg_kbd->dev_write = kbd_write;
  reg_kbd->dev_ioctl = kbd_ioctl;
  reg_kbd->dev_state = (void *) &keyboard_state[0];
  reset_state(&keyboard_state[0], FALSE);

  devsw *echo_kbd = &dev_tab[1];
  echo_kbd->dev_name = "echo_keyboard";
  echo_kbd->dev_num = 1;
  echo_kbd->dev_open = kbd_open;
  echo_kbd->dev_close = kbd_close;
  echo_kbd->dev_read = kbd_read;
  echo_kbd->dev_write = kbd_write;
  echo_kbd->dev_ioctl = kbd_ioctl;
  echo_kbd->dev_state = (void *) &keyboard_state[1];
  reset_state(&keyboard_state[1], TRUE);

  blocked_read = NULL;
}

/*
 * Opens the keyboard specified by dev
 *
 * Arguments
 *   the requesting process
 *   device structure for device we want to open
 *
 * Returns
 *   -1 if the device is already open, or another keyboard is open
 *    0 on success
 */
int kbd_open( pcb *proc, devsw *dev ) {

  //A keyboard is alreay open, return error
  if ( current_keyboard != (devsw *) NULL_DEVICE ) return -1;

  enable_irq(KEYBOARD_IRQ, 0);
  current_keyboard = dev;
  return 0;
}

/* ============================================================== */
/*                        Device Upper Level                      */
/* ============================================================== */

/*
 * Close the keyboard specified by dev
 *
 * Arguments
 *   the requesting process
 *   device structure for device we want to open
 *
 * Returns
 *   -1 if no keyboard is open
 *    0 on success
 */
int kbd_close( pcb *proc, devsw *dev ) {

  if ( current_keyboard == (devsw *) NULL_DEVICE ) return -1;


  enable_irq(KEYBOARD_IRQ, 1);

  kbdst* keyboard_state = (kbdst *) dev->dev_state;
  reset_state(keyboard_state, TRUE);

  current_keyboard = (devsw *) NULL_DEVICE;

  return 0;
}

/*
 * Read from keyboard to buf
 *
 * Arguments
 *   the requesting process
 *   device structure for device to read from
 *   pointer to buffer to read to
 *   length of buffer
 *
 * Returns
 *  0 if device has seen EOF
 *  BLOCK if proc should block
 *  # of bytes copied into buf otherwise
 */
int kbd_read(pcb *proc, devsw *dev, void *buf, int buflen) {

  kbdst* keyboard_state = (kbdst *) dev->dev_state;
  if ( keyboard_state->disabled ) return 0;


  unsigned int *proc_buffer = (unsigned int *) buf;

  if (buflen > kernel_buflen ) {

    blocked_read = proc;
    blocked_read->state = STATE_READ;
    blocked_read->buffer = buf;
    blocked_read->bufferlen = buflen;

    for(curr_buflen = 0; curr_buflen < kernel_buflen; curr_buflen++) {
      proc_buffer[curr_buflen] = kernel_buffer[curr_buflen];
    }

    kernel_buflen = 0;
    return BLOCK;
  } else {

    for(curr_buflen = 0; curr_buflen < buflen; curr_buflen++) {
      proc_buffer[curr_buflen] = kernel_buffer[curr_buflen];
    }

    kernel_buflen -= curr_buflen;
    int i;
    for(i = 0; i < kernel_buflen; i++) {
      kernel_buffer[i] = kernel_buffer[curr_buflen];
      curr_buflen++;
    }

    return buflen;
  }
}

/*
 * Write to keyboard not supported
 *
 * Arguments
 *   the requesting process
 *   device structure for device to write to
 *   pointer to buffer to write from
 *   length of buffer
 *
 * Returns
 *   -1 since write unsupported
 */
int kbd_write(pcb *proc, devsw * dev, void *buf, int buflen) {
  return -1;
}

/*
 * Configure Keyboard. Possible commands:
 *   command - 53, varargs - char : Change the EOF to char.
 *                                  char must be valid ASCII.
 *   command - 55 : turn echo off
 *   command - 56 : turn echo on
 *
 * Arguments
 *   the requesting process
 *   device structure for device to configure
 *   the option to configure; one of: CHANGE_EOF, ECHO_OFF, ECHO_ON
 *   additional args to configure device
 *
 * Returns
 *   -1 command fails or invalid
 *    0 on success
 */
int kbd_ioctl(pcb *proc, devsw *dev, unsigned long command, va_list varargs) {

  unsigned int eof_code;
  kbdst* keyboard_state = (kbdst *) dev->dev_state;

  switch ( command ) {

    case( CHANGE_EOF ):
      eof_code = va_arg( varargs, unsigned int );

      if( eof_code < 0 || eof_code > MAX_ASCII) {
        return -1;
      }
      keyboard_state->end_of_file = eof_code;
      return 0;

    case( ECHO_OFF ):
      keyboard_state->echo_on = FALSE;
      return 0;

    case( ECHO_ON ):
      keyboard_state->echo_on = TRUE;
      return 0;

    default:
      return -1;
  }
  return 0;
}

/*
 * Notify device upper half of incoming data. Update buffers
 * and blocked processes.
 *
 * Arguments
 *   the scan code read from device
 */
void notify_incoming_intr( unsigned int scan ) {

  kbdst* keyboard_state = (kbdst *) current_keyboard->dev_state;

  unsigned int int_code = kbtoa(scan);

  if (int_code > MAX_ASCII) return;

  char code = (char) int_code;

  if (code == keyboard_state->end_of_file ) {

    if( blocked_read ) unblock_proc();
    enable_irq(KEYBOARD_IRQ, 1);
    keyboard_state->disabled = TRUE;
  }

  if ( blocked_read ) {
    if (keyboard_state->echo_on) kprintf("%c",code);

    char *proc_buffer = (char *) blocked_read->buffer;
    proc_buffer[curr_buflen] = code;
    curr_buflen++;
    if (curr_buflen == blocked_read->bufferlen || code == '\n' )
      unblock_proc();

  } else if ( kernel_buflen < MAX_BUFF_CHAR ) {
    kernel_buffer[kernel_buflen] = code;
    kernel_buflen++;
    if (keyboard_state->echo_on) kprintf("%c",code);
  }

}


/*
 * Resets the keyboard state to default
 *
 * Arguments
 *   the state of the keyboard
 *   if echo should be turned on
 */
void reset_state( kbdst* keyboard_state, Bool echo_on ) {

  keyboard_state->echo_on = echo_on;
  keyboard_state->end_of_file = EOF_CODE;
  keyboard_state->disabled = FALSE;
  scan_state = 0;
  kernel_buflen = 0;
}

/*
 * Put blocked_read back on ready queue and set
 * return code to curr_buflen
 */
void unblock_proc( void ) {
  blocked_read->state = STATE_READY;
  blocked_read->ret = curr_buflen;
  ready(blocked_read);
  curr_buflen = 0;
  blocked_read = NULL;
}



/*
 * Zero out the EXTENDED bit
 *
 * Arguments
 *   the keyboard code
 *
 * Returns
 *    0
 */
static int extchar(unsigned char code)
{
  scan_state &= ~EXTENDED;
  return 0;
}

/*
 * Translate keyboard code to ascii
 *
 * Arguments
 *   the keyboard code
 *
 * Returns
 *    the ascii character interger code
 */
unsigned int kbtoa( unsigned char code )
{
  unsigned int    ch;

  if (scan_state & EXTENDED)
    return extchar(code);
  if (code & KEY_UP) {
    switch (code & 0x7f) {
    case LSHIFT:
    case RSHIFT:
      scan_state &= ~INSHIFT;
      break;
    case CAPSL:
      //kprintf("Capslock Key Released!\n");
      break;
    case LCTL:
      scan_state &= ~INCTL;
      break;
    case LMETA:
      scan_state &= ~INMETA;
      break;
    }

    return NOCHAR;
  }


  /* check for special keys */
  switch (code) {
  case LSHIFT:
  case RSHIFT:
    scan_state |= INSHIFT;
    //kprintf("shift detected!\n");
    return NOCHAR;
  case CAPSL:
    //kprintf("Capslock Pressed and Toggled!\n");
    scan_state ^= CAPSLOCK;
    return NOCHAR;
  case LCTL:
    scan_state |= INCTL;
    return NOCHAR;
  case LMETA:
    scan_state |= INMETA;
    return NOCHAR;
  case EXTESC:
    scan_state |= EXTENDED;
    return NOCHAR;
  }

  ch = NOCHAR;

  if (code < sizeof(kbcode)){
    if ( scan_state & CAPSLOCK )
      ch = kbshift[code];
    else
      ch = kbcode[code];
  }
  if (scan_state & INSHIFT) {
    if (code >= sizeof(kbshift))
      return NOCHAR;
    if ( scan_state & CAPSLOCK )
      ch = kbcode[code];
    else
      ch = kbshift[code];
  }
  if (scan_state & INCTL) {
    if (code >= sizeof(kbctl))
      return NOCHAR;
    ch = kbctl[code];
  }
  if (scan_state & INMETA)
    ch += 0x80;
  return ch;
}

/* ============================================================== */
/*                        Device Lower Level                      */
/* ============================================================== */

/*
 * ISR for keyboard interrupt. Reads data from keyboard if ready
 * and notifies device upper half.
 */
void keyboard_int_handler( void ) {

  unsigned int code = inb( CONTROL_PORT );
  if (!(code & READY) ) return;

  code = inb( DATA_PORT );

  notify_incoming_intr(code);
}

