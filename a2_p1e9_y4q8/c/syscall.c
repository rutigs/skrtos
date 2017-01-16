/*
 * syscall.c :
 *
 * This file is responsible for the syscalls into the kernel
 *
 * Public functions:
 * - int syscall(int call, ...);
 *      This method takes the given syscall id, moves the request id
 *      and the arguments into their correct registers and executes an interrupt
 *
 * - unsigned int syscreate( void (*func)(void), int stack );
 *      External (for users) wrapper around syscall() for creating a process
 *
 * - void sysyield( void );
 *      External wrapper around syscall() for yielding a process
 *
 * - void sysstop( void );
 *      External wrapper around syscall() for stopping a process
 *
 * - int sysgetpid( void );
 *      External wrapper around syscall() that returns the process' pid
 *
 * - void sysputs( char *str );
 *      External wrapper around syscall() that lets a process print a null
 *      terminated string to screen
 *
 * - int syskill( int pid );
 *      External wrapper around syscall() that lets a process kill another
 *      process with given pid
 *
 * - int syssend( int dest_pid, unsigned long num );
 *      External wrapper around syscall() that lets a process send a number
 *      to another process with given pid
 *
 * - int sysrecv( int *from_pid, unsigned long *num );
 *      External wrapper around syscall() that lets a process recv a number
 *      from another process (or any process)
 *
 * - unsigned int syssleep( unsigned int milliseconds );
 *      External wrapper around syscall() that lets a process sleep for a given
 *      number of milliseconds
 */


#include <xeroskernel.h>
#include <stdarg.h>

/*
 * Syscall method for a given call and its variadic arguments
 * executes an interrupt into the kernel that will begin its
 * processing of the syscall in the context switcher
 */
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

/*
 * syscall wrapper for creating a process
 *
 * Returns
 *   1 on success
 *   -1 on failure
 */
int syscreate( funcptr fp, size_t stack ) {
/*********************************************/

  return( syscall( SYS_CREATE, fp, stack ) );
}

/*
 * syscall wrapper for yielding a process
 */
void sysyield( void ) {
/***************************/
  syscall( SYS_YIELD );
}

/*
 * syscall wrapper for stopping a process
 */
void sysstop( void ) {
/**************************/

  syscall( SYS_STOP );
}

/*
 * syscall wrapper for getting the pid of the process from the kernel
 *
 * Returns the process ID
 */
int sysgetpid( void )
{
  return syscall( SYS_GETPID );
}

/*
 * syscall wrapper for a process to print something on the screen
 *
 * Arguments:
 *  str - pointer to a null terminated string to print
 */
void sysputs( char *str )
{
  syscall( SYS_PUTS, str );
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
int syskill( int pid )
{
  return( syscall( SYS_KILL, pid ) );
}


/*
 * syscall wrapper to send an unsigned long to another process
 *
 * Arguments:
 *  dest_pid -  the ID of the process to sent the message to
 *  num - the unsigned long to send
 *
 * Returns:
 *   0 on success
 *  -1 if the recieving process does not exist or has terminated
 *  -2 if dest_pid points to self
 *  -3 on other error
 */
int syssend( int dest_pid, unsigned long num ) {
  return( syscall( SYS_SEND, dest_pid, num ) );
}


/*
 * syscall wrapper to recieve an unsigned long from another process
 *
 * Arguments:
 *  from_pid - pointer to the ID of the process to recieve from
 *             Can be set to 0 to recieve from any process.
 *             Contains the ID of the process that sent the message on return.
 *  num -  address of the integer to store the received value into on return
 *
 * Returns:
 *   0 on success
 *  -1 if the process to recieve from does not exist or has terminated
 *  -2 if *from_pid points to self
 *  -3 on other error
 */
int sysrecv( int *from_pid, unsigned long *num ) {
  return( syscall( SYS_RECV, from_pid, num ) );
}

/*
 * syscall wrapper to sleep the process for at least the specified
 * amount of miliseconds
 *
 * Arguments:
 *  milliseconds - the mininum amount of milliseconds to sleep
 *
 * Returns:
 *  0  on success
 *  the milliseconds the process slept for on failure
 */
unsigned int syssleep( unsigned int milliseconds ) {
  return ( syscall( SYS_SLEEP, milliseconds ) );
}
