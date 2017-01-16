/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/

static Bool word_equals(char *exp, char *act, int length);

int idle_pid;
int shell_pid;       /* pid we will alarm */
int alarm_ticks;     /* ticks after we will alarm */

/*------------------------------------------------------------------------
 *  The idle process
 *------------------------------------------------------------------------
 */
static void idleproc( void )
{
    for(;;);
}



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

  devices_init();
  kprintf("device drivers inited\n");


  // WARNING THE FIRST PROCESS CREATED MUST BE THE IDLE PROCESS.
  // See comments in create.c

  // Note that this idle process gets a regular time slice but
  // according to the A2 specs it should only get a time slice when
  // there are no other processes available to run. This approach
  // works, but will give the idle process a time slice when other
  // processes are available for execution and thereby needlessly waste
  // CPU resources that could be used by user processes. This is
  // somewhat migigated by the immediate call to sysyield()
  kprintf("Creating Idle Process\n");

  idle_pid = create(idleproc, PROC_STACK);

  //create( test_syssighandler, PROC_STACK );
  //create( test_syskill, PROC_STACK );
  //create( test_signal_priority, PROC_STACK );
  //create( test_syswait, PROC_STACK );
  //create( run_signal_tests, PROC_STACK );
  //create( run_device_tests, PROC_STACK );
  //create( shell, PROC_STACK );

  create( init, PROC_STACK );


  kprintf("create inited\n");

  dispatch();


  kprintf("Returned to init, you should never get here!\n");

  /* This code should never be reached after you are done */
  for(;;) ; /* loop forever */
}

/*
 * The init program
 *
 * Verifies a username and password and starts the shell program
 */
void init() {
  int fd;
  char username[20];
  char password[20];
  char *sys_user = "cs415";
  char *sys_pass = "EveryoneGetsAnA";

  while ( 1 ) {
    memset( &username, 0, 20 );
    memset( &password, 0, 20 );

    // 1. Print banner
    sysputs( "\n\nWelcome to SKRT OS.\nAn experimental OS.\n"  );

    // 2. Open the keyboard in no echo mode
    fd = sysopen(0);

    // 3. turns keyboard echoing on
    unsigned long echo_cmd_on = 56;
    sysioctl(fd, echo_cmd_on);

    // 4. prints Username:
    sysputs( "\nUsername: " );

    // 5. Reads the username: cs415
    int username_chars = sysread(fd, &username, 20);
    username[username_chars] = 0;

    // 6. Turns keyboard echoing off
    unsigned long echo_cmd_off = 55;
    sysioctl(fd, echo_cmd_off);

    // 7. prints Password:
    sysputs( "\nPassword: " );

    // 8. Read the password: EveryoneGetsAnA
    int password_chars = sysread(fd, &password, 20);
    password[password_chars] = 0;

    // 9. closes keyboard;
    sysclose(fd);

    // 10. verify username and password
    if ( strncmp( username, sys_user, 5 ) != 0 ) {
      sysputs("Username invalid\n");
      //kprintf( "\n%s != %s", username, sys_user );
      continue;
    }

    if ( strncmp( password, sys_pass, 15) != 0 ) {
      sysputs("Password invalid\n");
      //kprintf( "\n%s != %s", password, sys_pass );
      continue;
    }

    shell_pid = syscreate(shell, 1024);
    syswait(shell_pid);
  }
}

/*
 * Checks if the word in exp is the same as in act,
 * and the word ends with a space or newline.
 * 
 * Arguments
 *    the expected word 
 *    the actual input
 *    the length of the word
 * 
 * Returns
 *    TRUE if the words are the same
 *    FALSE if they are not
 */
Bool word_equals(char *exp, char *act, int length) {

  if( strncmp(exp, act, length) == 0 ) {
    if( act[length] == ' ' || act[length] == '\n' ) {
      return TRUE;
    }
  }
  return FALSE;
}


/*
 * The shell program.  Reads input and runs commands.
 * 
 * Possible commands are: 
 *
 *   ps
 *   ex - exists shell
 *   k [pid] - kills process with pid, if exists
 *   a [ticks] - sets an alarm or tick cpu quantums
 *   t - prints T evey 10 seconds
 *   m - prints Bloody Murder
 *   c [char] - Changes EOF to char until shell ends
 */
void shell( void ) {
  int fd = sysopen(1);
  char input[100];
  int ret, pid;
  int child_index = 0;
  int child_proc[50];
  Bool repeat = TRUE;

  while ( repeat ) {
    repeat = FALSE;

    sysputs("\n> ");
    ret = sysread(fd, &input, 100);

    if ( ret > 0 ) {

      input[ret] = 0;

      char *current = input;
      for( ; *current == ' '; current++);

      if( word_equals("ps", current, 2) ) {
        processStatuses psTab;
        int procs, j;
        char buff[500], status[30];
        procs = sysgetcputimes(&psTab);

        
        sysputs("\n PID      State          Time\n");
        for(j = 0; j <= procs; j++) {

          switch (psTab.status[j]) {
            case ( STATE_READY ):
              sprintf(status, "%s", "  READY");
              break;            

            case ( STATE_SLEEP ):
              sprintf(status, "%s", " ASLEEP");
              break;   

            case ( STATE_RUNNING ):
              sprintf(status, "%s", "RUNNING");
              break;            
            case ( STATE_WAIT ):
              sprintf(status, "%s", "WAITING");
              break;            
            case ( STATE_READ ):
              sprintf(status, "%s", "READING");
              break;
            default:
              sprintf(status, "%s", "UNKNOWN");
          }

          sprintf(buff, "%4d    %s    %10d\n", psTab.pid[j], status, 
           psTab.cpuTime[j]);
          kprintf(buff);
        }

      } else if( word_equals("ex", current, 2) ) {
        sysclose(fd);
        sysputs("Exiting Shell. Goodbye.\n");
        return;

      } else if( strncmp(current, "k ", 2) == 0 ) {
        current += 2;
        for( ; *current == ' '; current++);
        int pid = atoi(current);
        int kill_ret = syskillproc(pid);

        if( kill_ret == -1 ) {
          sysputs("No such process.\n");
        } else if ( kill_ret == -2 ) {
          sysputs("Illegal to kill self. Use ex instead.\n");
        }

      } else if( strncmp(current, "a ", 2) == 0 ) {
        current += 2;
        for( ; *current == ' '; current++);
        void (*oldhandler)(void *);
        syssighandler(15, alarm_handler, &oldhandler);
        alarm_ticks = atoi(current);
        pid = syscreate(alarm, 1024); 
        child_proc[child_index++] = pid;

      } else if( word_equals("t", current, 1) ) {
        pid = syscreate(t, 1024); 
        child_proc[child_index++] = pid;

      } else if( word_equals("m", current, 1) ) {
        sysputs("Bloody Murder!\n");

      } else if( strncmp(current, "c ", 2) == 0 ) {
        current += 2;
        for( ; *current == ' '; current++);

        int result = sysioctl(fd, 53, *current);
        char buff[500];

        if (result == 0) {
          sprintf(buff, "Successfully changed EOF to %c.\n", *current);
        } else {
          sprintf(buff, "Changing EOF to %c Failed.\n", *current);
        }

        sysputs(buff);

      }else {
        sysputs("Command not found\n");
        repeat = TRUE;
      }

      if ( input[ret - 2] == '&' ) {
        kprintf("hit amp\n");
        repeat = TRUE;
      }
    } else if ( ret == -362 ) {
      repeat = TRUE;
    }
  }

  while ( child_index >= 0 ) {
    ret = syswait(child_proc[child_index]);

    if ( ret == 0 || ret == -1 ) child_index--; 
  }

  sysclose(fd);
  sysputs("Exiting Shell. Goodbye.\n");
}


/*
 * Handler to wake from alarm.
 */
void alarm_handler( void *cntx ) {
  void (*oldhandler)(void *);
  sysputs("ALARM ALARM ALARM\n");
  syssighandler(15, NULL, &oldhandler);
}


/*
 * alarm - alarm the shell after a given time
 */
void alarm( void ) {
  syssleep( alarm_ticks * MILLISECONDS_TICK );
  syskill( shell_pid, 15 );
}

/*
 * t - t process prints T every 10 seconds or so
 */
void t( void ) {
  while ( 1 ) {
    syssleep(10000);
    sysputs("\nT\n");
  }
}
