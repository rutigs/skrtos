/*
 * syscall.c :
 *
 * This file is responsible for the syscalls into the kernel
 *
 * This document is for functions we have added/edited.
 *
 * All of these functions are wrappers around kernel function that are exposed
 * to "user" processes
 *
 * - int syskill( int pid, int signalNumber );
 *     lets a process signal another process with given pid and signal number
 *
 * - int syssighandler(int signal, void (*newHandler)(void *), void (** oldHandler)(void *));
 *      lets a process register a signal handler for a given signal, and copy the
 *      older handler to a variable
 *
 * - void syssigreturn(void *old_sp);
 *      kernel only when return from signal handling code
 *
 * - int syswait(int PID);
 *      allows a process to wait for another process to terminate before
 *      continuing to run
 *
 * - int sysopen(int device_no);
 *      returns a fd for a device and opens it for that process to use
 *
 * - int sysclose(int fd);
 *      closes a device from a given fd
 *
 * - int syswrite(int fd, void *buf, int buflen);
 *      allows a process to write data to a device on a file descriptor from a
 *      given buffer up to a given length
 *
 * - int sysread(int fd, void *buf, int buflen);
 *      lets a process read data of a given length from a device into a buffer
 *
 * - int sysioctl(int fd, unsigned long command, ...);
 *      allows a process to perform out of band interaction with a device
 *      by with specific commands and their variadic args.
 *
 */

#include <xeroskernel.h>
#include <stdarg.h>


int syscall( int req, ... ) {
/**********************************/

    va_list     ap;
    int         rc;

    va_start( ap, req );

    __asm __volatile( " \
        movl %1, %%eax \n\
        movl %2, %%edx \n\
        int  %3 \n\
        movl %%eax, %0 \n\
        "
        : "=g" (rc)
        : "g" (req), "g" (ap), "i" (KERNEL_INT)
        : "%eax"
    );

    va_end( ap );

    return( rc );
}

int syscreate( funcptr fp, size_t stack ) {
/*********************************************/

    return( syscall( SYS_CREATE, fp, stack ) );
}

void sysyield( void ) {
/***************************/
  syscall( SYS_YIELD );
}

 void sysstop( void ) {
/**************************/

   syscall( SYS_STOP );
}

unsigned int sysgetpid( void ) {
/****************************/

    return( syscall( SYS_GETPID ) );
}

/*
 * syscall wrapper to kill the process with pid
 *
 * Arguments:
 *  pid - the ID of the process to kill
 *
 * Returns:
 *   0 on success.
 *  -1 if the target process does not exist
 *  -2 if pid points to self
 */
int syskillproc(int pid) {
  return syscall(SYS_KILL_PROC, pid);
}

void sysputs( char *str ) {
/********************************/

    syscall( SYS_PUTS, str );
}

unsigned int syssleep( unsigned int t ) {
/*****************************/

    return syscall( SYS_SLEEP, t );
}

/*
 * syscall wrapper to kill a process with a signal
 *
 * Arguments:
 *   pid to deliver the signal to
 *   signal number to send to
 *
 * Returns:
 *    0 on success
 *   -712 if the process does not exist
 *   -651 if the signal is invalid
 */
int syskill(int pid, int signalNumber) {
  return syscall(SYS_KILL, pid, signalNumber);
}

/*
 * syscall wrapper to get the cpu times
 *
 * Arguments:
 *   each element in this structure corresponds to a process in the system
 *   for This function fills in the table starting at element 0
 *
 * Returns
 *   -1 if the address is in the hole
 *   -2 if the structure being pointed to goes beyond main mem.
 *    0 on success
 */
int sysgetcputimes(processStatuses *ps) {
  return syscall(SYS_CPUTIMES, ps);
}

/*
 * syscall wrapper to register a signal handler for the indicated signal
 *
 * Arguments:
 *   Signal number you want to register the handler for
 *   Function pointer to the new signal handler
 *   Pointer to a variable that points to the old handler.
 *
 * Returns
 *   -1 if signal is invalid
 *   -2 if the handler resides at an invalid address
 *    0 on success
 */
int syssighandler(int signal, void (*newHandler)(void *), void (** oldHandler)(void *)) {
  return syscall(SYS_SIGHANDLER, signal, newHandler, oldHandler);
}

/*
 * syscall wrapper, only used by the signal trampoline code.
 *
 * Arguments:
 *   the location in the application stack, of the context frame to switch this process to
 */
void syssigreturn(void *old_sp) {
  syscall(SYS_SIGRETURN, old_sp);
}

/*
 * syscall wrapper that causes the calling process to wait for the process with PID to terminate
 *
 * Arguments:
 *   process PID to wait for
 *
 * Return:
 *    0 on sucess
 *   -1 if the target process does not exist
 *   -2 if a signal is targetted at the process calling this
 */
int syswait(int PID) {
  return syscall(SYS_WAIT, PID);
}

/*
 * syscall wrapper that opens a specified device
 *
 * Arguments:
 *   major device number we want to open
 *
 * Return:
 *   -1 if it fails
 *   file descriptor in the range of 0-3
 */
int sysopen(int device_no) {
  return syscall(SYS_OPEN, device_no);
}

/*
 * syscall wrapper that closes a device on a given fd from a previous open
 *
 * Arguments:
 *   file descriptor of device we want to close
 *
 * Return:
 *   0 on success
 *   -1 on failure
 */
int sysclose(int fd) {
  return syscall(SYS_CLOSE, fd);
}

/*
 * syscall wrapper to write to a device
 *
 * Arguments:
 *   file descriptor of device
 *   buffer of data we want to write
 *   length of buffer to write
 *
 * Return:
 *   -1 on error
 *   # of bytes written
 */
int syswrite(int fd, void *buf, int buflen) {
  return syscall(SYS_WRITE, fd, buf, buflen);
}

/*
 * syscall wrapper to read from a device
 *
 * Arguments:
 *   file descriptor of device
 *   buffer to read into
 *   maximum number of bytes to read
 *
 * Return:
 *   -1 on error
 *   0 on EOF
 *   # of bytes reads
 */
int sysread(int fd, void *buf, int buflen) {
  return syscall(SYS_READ, fd, buf, buflen);
}

/*
 * syscall wrapper for device specific control command
 *
 * Arguments:
 *   controller command
 *   variadic arguments depending on device/command
 *
 * Return:
 *   -1 on error
 *   0 otherwise
 */
int sysioctl(int fd, unsigned long command, ...) {
  va_list ap;
  va_start( ap, command );
  int result = syscall(SYS_IOCTL, fd, command, ap);
  va_end( ap );
  return result;
}

