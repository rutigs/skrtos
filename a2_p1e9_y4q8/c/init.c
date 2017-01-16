/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern  char	*maxaddr;	/* max memory address (set in i386.c)	*/

int idle_pid;

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc( void )				/* The beginning */
{
  kprintf( "\n\nCPSC 415, 2016W1 \n32 Bit Xeros 0.01 \nLocated at: %x to %x\n",
	   &entry, &end);

  /* Your code goes here */

  kprintf("Max addr is %d %x\n", maxaddr, maxaddr);

  kmeminit();
  kprintf("memory inited\n");

  dispatchinit();
  kprintf("dispatcher inited\n");

  contextinit();
  kprintf("context inited\n");

  createinit();
  kprintf("create inited\n");

  create( root, PROC_STACK );

  // We need to know the idle_pid for knowing when to schedule it.
  // (aka when no other processes are doing anything)
  idle_pid = create( idleproc, 2048 );

  // === Test Section ===
  //create( test_sysgetpid_and_return, PROC_STACK );
  //create( test_sysputs, PROC_STACK );
  //create( test_syskill, PROC_STACK );
  //create( test_syssend, PROC_STACK );
  //create( test_sysrecv, PROC_STACK );
  //create( test_send_recv_kill, PROC_STACK );
  //create( test_syssleep, PROC_STACK );

  dispatch();


  kprintf("Returned to init, you should never get here!\n");

  /* This code should never be reached after you are done */
  for(;;) ; /* loop forever */

}

/*
 * The idle process that does nothing
 */
void idleproc( void ) {
  for ( ;; );
}
