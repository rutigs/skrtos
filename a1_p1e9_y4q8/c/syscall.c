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
 *      This is a wrapper around the syscall function for creating a process
 *
 * - void sysyield( void );
 *      This is a wrapper around the syscall function for yielding a process
 *
 * - void sysstop( void );
 *      This is a wrapper around the syscall function for stopping a process
 */

#include <xeroskernel.h>
#include <stdarg.h>
/* Your code goes here */

/*
 * syscall method for a given call and variadic arguments list
 * executes an interrupt based on the given syscall
 * that will then be process by the context switcher
 */
int syscall(int call, ...) 
{
  __asm __volatile( "                 \
    movl   8(%%ebp), %%eax          \n\
    movl   %%ebp, %%edx             \n\
    addl   $12, %%edx               \n\
    int    $"STRINGIFY(SYS_CALL_ID)"\n\
      "
  :
  : 
  : "%eax"
  );
}

/*
 * syscall wrapper for creating a process
 *
 * Return 1 on success and -1 on failure.
 */
unsigned int syscreate( void (*func)(void), int stack ) 
{
  return syscall(SYS_CREATE, func, stack);
}

/*
 * syscall wrapper for yielding a process
 */
void sysyield( void ) 
{
  syscall(SYS_YIELD);
}


/*
 * syscall wrapper for stopping a process
 */
void sysstop( void ) 
{
  syscall(SYS_STOP);
}

