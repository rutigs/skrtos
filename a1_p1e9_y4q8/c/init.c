/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/

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
  int i; 

  kprintf( "\n\nCPSC 415, 2016W1 \n32 Bit Xeros 0.01 \nLocated at: %x to %x\n", 
       &entry, &end); 
  
  run_unit_tests();

  for (i = 0; i < 2000000; i++);

  /* Runs a kernel test that enters dispatch and creates test processes,
   * must be commented out for nornal kernel to run */
  /* run_kernel_test(); */


  /* Starting the kernel */
    
  /* Initialize the Memory Manager */
  kmeminit();

  /* Initialize the Dispatcher */
  dispatch_init();

  /* Initialize Context Switcher */
  context_init();
 
  /* Initalize Process creation */
  create_init();
  
  /* Create first user process, stack size set to 4096 */
  create(&root, 4096);

  for (i = 0; i < 4000000; i++);

  /* enter dispatch */
  dispatch( );

  for (i = 0; i < 2000000; i++);
  /* Add all of your code before this comment and after the previous comment */
  /* This code should never be reached after you are done */
  kprintf("\n\nWhen the kernel is working properly ");
  kprintf("this line should never be printed!\n");
  for(;;) ; /* loop forever */
}


