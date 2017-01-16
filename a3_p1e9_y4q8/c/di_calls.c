/*
 * di_calls.c - device independent calls
 *
 * if any upperlevel code returns BLOCK, returns true to block the process
 * passes proc and dev to all upper level funcntions
 *
 * void devices_init() - initializes device
 *
 * Bool di_open(pcb *proc, int device_no);
 *   Open a connection with the specified device, and sets
 *   the return code on the pcb
 *
 * Bool di_close(pcb *proc, int fd);
 *   Closes the device connection, and sets the process return code.
 *
 * Bool di_read(pcb *proc, int fd, void *buf, int buflen);
 *   Read data from a device and sets the pcb return code.
 *
 * Bool di_write(pcb *proc, int fd, void *buf, int buflen);
 *   Write to a device and sets the return code
 *
 * Bool di_ioctl(pcb *proc, int fd, unsigned long command, va_list varargs) {
 *   Handles device specific commands and sets the pcb return code
 *
 * Bool verify_buffer(void *buffer, int buflen)
 *   Verifies buffer is in valid location and buflen is valid
*/

#include <xeroskernel.h>
#include <i386.h>


extern char *maxaddr;

/* Internal Helpers   */
static Bool verify_buffer(void *buf, int buflen);


/*
 * Initialize devices
 */
void devices_init() {
  keyboards_init();
}

/*
 * Open a connection with the specified device, and sets
 * the return code on the pcb
 *
 * Arguments:
 *   pcb that is opening the device
 *   device num for device we want to open
 *
 * Returns:
 *   TRUE if p should block
 *   FALSE if not.
 */
Bool di_open(pcb *proc, int device_no) {

  proc->ret = -1;

  // device number is invalid
  if ( device_no < 0 || device_no >= MAX_KERN_DEVICES ) {
    return FALSE;
  }

  int fd;
  for ( fd = 0; fd < MAX_PROC_DEVICES; fd++ ) {
    // find empty fd in pcb table
    if ( proc->fd_tab[fd] == (devsw *) NULL_DEVICE ) {
      // open device connection
      int result = dev_tab[device_no].dev_open(proc, &dev_tab[device_no]);

      if ( result == BLOCK ) {
        // If dev_open passes back BLOCK, block the process
        return TRUE;
      } else if ( result < 0) {
        return FALSE;
      }

      proc->fd_tab[fd] = &dev_tab[device_no];
      proc->ret = fd;
      return FALSE;
    }
  }

  return FALSE;
}

/*
 * Closes the device connection, and sets the process return code.
 *
 * Arguments:
 *   pcb device we want to close
 *   fd of the device we want to close
 *
 * Returns:
 *   TRUE if p should block
 *   FALSE if not.
 */
Bool di_close(pcb *proc, int fd) {

  proc->ret = -1;

  // check if the fd is valid
  if ( fd < 0 || fd > MAX_PROC_DEVICES ) {
    return FALSE;
  }

  // check if the device is actually open
  if ( proc->fd_tab[fd] == (devsw *) NULL_DEVICE ) {
    return FALSE;
  }

  // call the device specific close
  devsw *dev = proc->fd_tab[fd];
  int result = (dev->dev_close)(proc, dev);

  // If dev_close passes back BLOCK, block the process
  if (result == BLOCK ) return TRUE;

  // reset file descriptor
  proc->fd_tab[fd] = (devsw *) NULL_DEVICE;

  proc->ret = 0;
  return FALSE;
}

/*
 * Read data from a device and sets the pcb return code.
 *
 * Arguments:
 *   pcb of process
 *   fd of device to read from
 *   buffer to read into
 *   length to read
 *
 * Returns:
 *   TRUE if p should block
 *   FALSE if not.
 *
 */
Bool di_read(pcb *proc, int fd, void *buf, int buflen) {

  proc->ret = -1;

  // check if the fd is valid
  if ( fd < 0 || fd > MAX_PROC_DEVICES ) {
    return FALSE;
  }

  // check if the device is actually open
  if ( proc->fd_tab[fd] == (devsw *) NULL_DEVICE ) {
    return FALSE;
  }

  // check if buffer and buflen is valid
  if ( !verify_buffer(buf, buflen) ) {
    return FALSE;
  }

  devsw *dev = proc->fd_tab[fd];
  int result = (dev->dev_read)(proc, dev, buf, buflen);

  // If BLOCK status is returned, block the process
  if (result == BLOCK ) return TRUE;

  proc->ret = result;
  return FALSE;
}

/*
 * Write to a device and sets the return code
 *
 * Arguments:
 *   process this is working on
 *   file descriptor for device
 *   buffer to read into
 *   buffer length to read
 *
 * Returns:
 *   TRUE if p should block
 *   FALSE if not.
 *
 */
Bool di_write(pcb *proc, int fd, void *buf, int buflen) {

  proc->ret = -1;

  // check if the fd is valid
  if ( fd < 0 || fd > MAX_PROC_DEVICES ) {
    return FALSE;
  }

  // check if the device is actually open
  if ( proc->fd_tab[fd] == (devsw *) NULL_DEVICE ) {
    return FALSE;
  }

  // check if buffer and buflen is valid
  if ( !verify_buffer(buf, buflen) ) {
    return FALSE;
  }

  devsw *dev = proc->fd_tab[fd];
  int result = (dev->dev_write)(proc, dev, buf, buflen);

  // If BLOCK status is returned, block the process
  if (result == BLOCK ) return TRUE;

  proc->ret = result;
  return FALSE;
}

/*
 * Handles device specific commands and sets the pcb return code
 *
 * Arguments:
 *   pcb using device
 *   device file descriptor
 *   device command and args
 *
 * Returns:
 *   TRUE if p should block
 *   FALSE if not.
 */
Bool di_ioctl(pcb *proc, int fd, unsigned long command, va_list varargs) {

  proc->ret = -1;

  // check if the fd is valid
  if ( fd < 0 || fd > MAX_PROC_DEVICES ) {
    return FALSE;
  }

  // check if the device is actually open
  if ( proc->fd_tab[fd] == (devsw *) NULL_DEVICE ) {
    return FALSE;
  }

  devsw *dev = proc->fd_tab[fd];
  int result = (dev->dev_ioctl)(proc, dev, command, varargs);

  // If BLOCK status is returned, block the process
  if (result == BLOCK ) return TRUE;

  proc->ret = result;
  return FALSE;
}

/*
 * Verifies buffer is in valid location and buflen is valid
 *
 * Arguments:
 *   pointer to the buffer
 *   length of buffer
 *
 * Returns:
 *   TRUE if both valid
 *   FALSE if not.
 */
Bool verify_buffer(void *buffer, int buflen) {

  // check if buffer is valid
  if ( buffer < 0 || ((char *) buffer) > maxaddr ) {
    return FALSE;
  }

  // check if buffer is in hole
  if ( ((unsigned long) buffer > HOLESTART) && ((unsigned long) buffer < HOLEEND) ) {
    return FALSE;
  }

  // check if buflen is valid
  if ( buflen <= 0 ) {
    return FALSE;
  }

  return TRUE;
}
