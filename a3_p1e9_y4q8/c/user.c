/* user.c : User processes
 */

#include <xeroskernel.h>
#include <xeroslib.h>


/*
 * ==============================================================================
 *                                    Testing code
 * ==============================================================================
*/

Bool verbose = FALSE;
static int test_counter, dest_pid, to_signal;


/*
 * Prints error if exp not equal act
 *
 * Returns 1 if exp equals act
 *         0 otherwise
 */
int assert_equal( int exp, int act, const char* func, int test_case, char* message ) {
  char *str[500];

  if (exp != act) {
    sprintf( (char *)str, "%s Case %d Error: %s, Expected %d, Actual %d\n",
             func, test_case, message, exp, act );
    sysputs( (char *)str );
    return 0;
  }

  if ( verbose ) {
   sprintf( (char *)str, "%s Case %d Success\n", func, test_case);
   sysputs( (char *)str );
  }

  return 1;
}


/*
 * Signal handler that calls sysstop
 */
void stop_handler( void *cntx ) {
  sysstop();
}

/*
 * Signal handler to increment test_counter;
 */
void increment_handler( void *cntx ) {

  test_counter++;
}

/*
 * Signal handler to triple test_counter;
 */
void triple_handler( void *cntx ) {
  test_counter = 3 * test_counter;
}

/*
 * Signal handler to syskill dest_pid with to_signal
 */
void reraise_handler( void *cntx ) {
  syskill(dest_pid, to_signal);

}


/*
 * Process with signal handlers registered.
 */
void test_handlers_helper( void ) {
  void (*oldhandler)(void *);

  syssighandler(31, increment_handler, &oldhandler);
  syssighandler(25, triple_handler, &oldhandler);
  syssighandler(18, stop_handler, &oldhandler);
  syssighandler(11, reraise_handler, &oldhandler);

  for( ; ; ) sysyield();
}


/*
 * Test syssighandler.
 */
void test_syssighandler( void ) {
  int test_result = 1;
  char *str[500];

  int ret;
  void (*oldhandler)(void *);

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );

  //Test Case 1: signal invalid
  ret = syssighandler(35, stop_handler, &oldhandler);
  test_result &= assert_equal(-1, ret, __func__, 1, "invalid signal not caught");

  //Test Case 2: handler in hole
  ret = syssighandler(0, (void (*)(void *)) (641 * 1024), &oldhandler);
  test_result &= assert_equal(-2, ret, __func__, 3, "invalid handler not caught");

  //Test Case 3: handler installed
  ret = syssighandler(0, stop_handler, &oldhandler);
  test_result &= assert_equal(0, ret, __func__, 3, "handler not installed");

  //Test Case 4: handler replaced
  ret = syssighandler(0, increment_handler, &oldhandler);
  test_result &= assert_equal(0, ret, __func__, 4, "handler not installed");
  test_result &= assert_equal((int) stop_handler, (int) oldhandler, __func__, 4, "handler not replaced");


  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}


/*
 * Test syskill
 */
void test_syskill( void ) {
  int test_result = 1;
  char *str[500];

  int ret, pid;
  void (*oldhandler)(void *);

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );

  pid = sysgetpid();
  test_counter = 0;

  //Test Case 1: process does not exist
  ret = syskill(12345, 0);
  test_result &= assert_equal(-712, ret, __func__, 1, "process should not exist");

  //Test Case 2: signal is invalid
  ret = syskill(pid, 34);
  test_result &= assert_equal(-651, ret, __func__, 2, "signal should be invalid");

  //Test Case 3: no handler signal ignored
  ret = syskill(pid, 0);
  test_result &= assert_equal(0, ret, __func__, 3, "syskill failed");

  //Test Case 4: handler triggered.
  ret = syssighandler(0, increment_handler, &oldhandler);
  test_result &= assert_equal(0, ret, __func__, 4, "handler not installed");
  ret = syskill(pid, 0);
  test_result &= assert_equal(0, ret, __func__, 4, "syskill failed");
  test_result &= assert_equal(1, test_counter, __func__, 4, "handler not called");

  //Test Case 5: different signal, no handler
  ret = syskill(pid, 1);
  test_result &= assert_equal(0, ret, __func__, 5, "syskill failed");
  test_result &= assert_equal(1, test_counter, __func__, 4, "handler should not be called ");

  //Test Case 6: Kill Sleeping process
  int helper_pid = syscreate(test_handlers_helper, 1024); // start a new helper
  sysyield(); // For handlers to register 
  dest_pid = pid;
  to_signal = 0;
  syskill(helper_pid, 11); // To signal helper to signal this with no. 4
  ret = syssleep(4000);
  if (ret <= 0) {
    sprintf( (char *)str, "%s Case 6 Error: Syssleep return wrong, ret code: %d \n",
            __func__, ret);
    sysputs( (char *)str );
    test_result = 0;
  }


  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}


/*
 * Test signal priority and masking. Higher numbered signals have higher prority.
 */
void test_signal_priority( void ) {
  int test_result = 1;
  char *str[500];

  int ret, pid;

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );

  pid = syscreate(test_handlers_helper, 1024);
  test_counter = 0;

  //Test Case 1: no handler
  ret = syskill(pid, 31);
  test_result &= assert_equal(0, ret, __func__, 1, "syskill failed");

  //Test Case 2: handler triggered.
  sysyield(); // For handlers to register
  ret = syskill(pid, 31);
  test_result &= assert_equal(0, ret, __func__, 2, "syskill failed");
  sysyield(); // For signal to be delivered
  test_result &= assert_equal(1, test_counter, __func__, 2, "handler not called");

  //Test Case 3: 2 signals, higher has priority
  test_counter = 1;
  ret = syskill(pid, 31);  // increments test_counter to 2
  ret = syskill(pid, 25);  // triples test_counter to 6
  sysyield(); // For signal to be delivered
  test_result &= assert_equal(6, test_counter, __func__, 3, "handler not in prority");

  //Test Case 4: handler triggers another signal 
  test_counter = 6;
  dest_pid = pid;
  to_signal = 25;
  ret = syskill(pid, 11);
  sysyield(); // For signal to be delivered
  test_result &= assert_equal(18, test_counter, __func__, 4, "handler did not run");

  //Test Case 5: handler triggers already run signal 
  test_counter = 0;
  dest_pid = pid;
  to_signal = 31;
  ret = syskill(pid, 11);
  ret = syskill(pid, 31);
  sysyield(); // For signal to be delivered
  test_result &= assert_equal(2, test_counter, __func__, 5, "handler did not run again");

  //Test Case 6: priority hander stops process
  test_counter = 0;
  dest_pid = pid;
  to_signal = 31;
  ret = syskill(pid, 11);
  ret = syskill(pid, 18);
  sysyield(); // For signal to be delivered
  test_result &= assert_equal(0, test_counter, __func__, 6, "handler not run in prority order");


  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}



/*
 * Test syswait
 */
void test_syswait( void ) {
  int test_result = 1;
  char *str[500];

  int ret, pid, helper_pid;
  void (*oldhandler)(void *);

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );

  pid = sysgetpid();
  syssighandler(4, increment_handler, &oldhandler);
  helper_pid = syscreate(test_handlers_helper, 1024);
  sysyield(); // For handlers to register 

  //Test Case 1: invalid pid
  ret = syswait(5);
  test_result &= assert_equal(-1, ret, __func__, 1, "pid should be invalid");

  //Test Case 2: wait sucess.
  syskill(helper_pid, 18); // Signal helper to sysstop
  ret = syswait(helper_pid); // Wait for helper to terminate
  test_result &= assert_equal(0, ret, __func__, 2, "syswait failed");

  //Test Case 3: handler triggers another signal 
  helper_pid = syscreate(test_handlers_helper, 1024); // start a new helper
  sysyield(); // For handlers to register 

  dest_pid = pid;
  to_signal = 4;
  syskill(helper_pid, 11); // To signal helper to signal this with no. 4
  ret = syswait(helper_pid);
  test_result &= assert_equal(-2, ret, __func__, 3, "syswait wrong return code on signaled");


  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}

/*
 * Run all signal tests
 */
void run_signal_tests( void ) {
  int pid;

  pid = syscreate(test_syssighandler, 1024);
  syswait(pid);

  pid = syscreate(test_syskill, 1024);
  syswait(pid);

  pid = syscreate(test_signal_priority, 1024);
  syswait(pid);

  pid = syscreate(test_syswait, 1024);
  syswait(pid);
}



/*
 * Test sysopen and sysclose with valid and invalid args
 */
void test_sysopen_sysclose( void ) {

  int test_result = 1;
  char *str[500];

  int ret, fd;

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );  

  //Test Case 1: invalid device number
  ret = sysopen(5);
  test_result &= assert_equal(-1, ret, __func__, 1, "device number should be invalid");

  //Test Case 2: open success
  fd = sysopen(0);
  test_result &= assert_equal(0, fd, __func__, 2, "sysopen failed");

  //Test Case 3: open again fails
  ret = sysopen(0);
  test_result &= assert_equal(-1, ret, __func__, 3, "device should have been open already");

  //Test Case 4: close succedes
  ret = sysclose(fd);
  test_result &= assert_equal(0, ret, __func__, 4, "sysclose failed");

  //Test Case 5: close alrady closed descripter fails
  ret = sysclose(fd);
  test_result &= assert_equal(-1, ret, __func__, 4, "device should have been closed");

  //Test Case 4: close invalid fd
  ret = sysclose(5);
  test_result &= assert_equal(-1, ret, __func__, 1, "fd should be invalid");

  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}


/*
 * Test syswrite 
 */
void test_syswrite( void ) {

  int test_result = 1;
  char *str[500];
  char input[100];


  int ret, fd;

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );  

  fd = sysopen(0);

  //Test Case 1: invalid file descriptor
  ret = syswrite(5, &input, 10);
  test_result &= assert_equal(-1, ret, __func__, 1, "file descriptor should be invalid");

  //Test Case 2: invalid file descriptor again
  ret = syswrite(fd + 1, &input, 10);
  test_result &= assert_equal(-1, ret, __func__, 2, "file descriptor should be invalid");

  //Test Case 3: invalid buffer
  ret = syswrite(fd, (void *) -100, 10);
  test_result &= assert_equal(-1, ret, __func__, 3, "buffer should be invalid");

  //Test Case 4: invalid buflen
  ret = syswrite(fd, &input, 0);
  test_result &= assert_equal(-1, ret, __func__, 4, "buflen should be invalid");

  //Test Case 5: unsupported write
  ret = syswrite(fd, &input, 10);
  test_result &= assert_equal(-1, ret, __func__, 5, "write should be unsupported on keyboard");

  sysclose(fd);
  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}

/*
 * Test sysioctl with keyboard 
 */
void test_sysioctl( void ) {

  int test_result = 1;
  char *str[500];

  int ret, fd;
  unsigned long command;

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );  

  fd = sysopen(0);

  //Test Case 1: invalid command
  command = 11;
  ret = sysioctl(fd, command);
  test_result &= assert_equal(-1, ret, __func__, 1, "command should be invalid");

  //Test Case 2: turn echo on 
  command = 56;
  ret = sysioctl(fd, command);
  test_result &= assert_equal(0, ret, __func__, 2, "command should be valid");

  //Test Case 3: turn echo on 
  command = 55;
  ret = sysioctl(fd, command);
  test_result &= assert_equal(0, ret, __func__, 3, "command should be valid");

  //Test Case 4: change EOF
  command = 53;
  ret = sysioctl(fd, command, 87);
  test_result &= assert_equal(0, ret, __func__, 4, "command should be valid");


  sysclose(fd);
  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}


/*
 * Test sysread with keyboard 
 */
void test_sysread( void ) {

  int test_result = 1;
  char *str[500];
  unsigned int input[100];


  int ret, fd, i;

  sprintf( (char *)str, "\nRunning Tests: %s \n", __func__ );
  sysputs( (char *)str );  

  fd = sysopen(1);


  //Test Case 1: invalid file discriptor
  ret = sysread(5, &input, 10);
  test_result &= assert_equal(-1, ret, __func__, 1, "file descriptor should be invalid");

   //Test Case 2: invalid buffer
  ret = sysread(fd, (void *) -100, 10);
  test_result &= assert_equal(-1, ret, __func__, 2, "buffer should be invalid");

  //Test Case 3: invalid buflen
  ret = sysread(fd, &input, 0);
  test_result &= assert_equal(-1, ret, __func__, 3, "buflen should be invalid");

  //Test Case 4: read from device
  sysputs("Type 4 characters\n");
  ret = sysread(fd, &input, 4);
  test_result &= assert_equal(4, ret, __func__, 4, "wrong number of characters read.");


  //Test Case 5: read from kernel buffer
  sysputs("Type 5 characters\n");
  syssleep(10000);
  sysputs("Going to read 4 chars: ");
  ret = sysread(fd, &input, 4);
  for(i = 0; i < ret; i++) {
    sprintf( (char *)str, "%c", input[i]);
    sysputs( (char *)str );
  }  
  sysputs("\n");

  test_result &= assert_equal(4, ret, __func__, 5, "wrong number of characters read.");

  //Test Case 6: read from kernel buffer, twice
  sysputs("Type 4 characters\n");
  syssleep(10000);
  sysputs("Going to read 1 char: ");
  ret = sysread(fd, &input, 1);
  for(i = 0; i < ret; i++) {
    sprintf( (char *)str, "%c", input[i]);
    sysputs( (char *)str );
  }  
  sysputs("\n");

  test_result &= assert_equal(1, ret, __func__, 6, "wrong number of characters read.");

  sysputs("Type some char. Going to read 5 char: ");
  ret = sysread(fd, &input, 5);
  for(i = 0; i < ret; i++) {
    sprintf( (char *)str, "%c", input[i]);
    sysputs( (char *)str );
  }  
  sysputs("\n");
  test_result &= assert_equal(5, ret, __func__, 6, "wrong number of characters read."); 

  
  sysclose(fd);
  sprintf( (char *)str, "%s %s\n", __func__, (test_result? "TEST PASSED" : "TEST FAILED"));
  sysputs( (char *)str );
}


/*
 * Run all device tests
 */
void run_device_tests( void ) {

  test_sysopen_sysclose();
  test_syswrite();
  test_sysioctl();
  test_sysread();
}


/* ================================================================ */
/*                         Original Tests                           */
/* ================================================================ */


void sig_handler( void *arg ) {
  sysstop();
}

void busy( void ) {
  int myPid;
  char buff[100];
  int i;
  int count = 0;

  void (*oldhandler)(void *);
  syssighandler(0, sig_handler, &oldhandler);

  myPid = sysgetpid();
  
  for (i = 0; i < 10; i++) {
    sprintf(buff, "My pid is %d\n", myPid);
    sysputs(buff);
    if (myPid == 2 && count == 1) syskill(3, 0);
    count++;
    sysyield();
  }
}



void sleep1( void ) {
  int myPid;
  char buff[100];
  void (*oldhandler)(void *);
  syssighandler(0, sig_handler, &oldhandler);

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 1000 is %d\n", myPid);
  sysputs(buff);
  syssleep(1000);
  sprintf(buff, "Awoke 1000 from my nap %d\n", myPid);
  sysputs(buff);
}



void sleep2( void ) {
  int myPid;
  char buff[100];
  void (*oldhandler)(void *);
  syssighandler(0, sig_handler, &oldhandler);

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 2000 pid is %d\n", myPid);
  sysputs(buff);
  syssleep(2000);
  sprintf(buff, "Awoke 2000 from my nap %d\n", myPid);
  sysputs(buff);
}



void sleep3( void ) {
  int myPid;
  char buff[100];
  void (*oldhandler)(void *);
  syssighandler(0, sig_handler, &oldhandler);

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 3000 pid is %d\n", myPid);
  sysputs(buff);
  syssleep(3000);
  sprintf(buff, "Awoke 3000 from my nap %d\n", myPid);
  sysputs(buff);
}








void producer( void ) {
/****************************/

    int         i;
    char        buff[100];
    void (*oldhandler)(void *);
    syssighandler(0, sig_handler, &oldhandler);


    // Sping to get some cpu time
    for(i = 0; i < 100000; i++);

    syssleep(3000);
    for( i = 0; i < 20; i++ ) {
      
      sprintf(buff, "Producer %x and in hex %x %d\n", i+1, i, i+1);
      sysputs(buff);
      syssleep(1500);

    }
    for (i = 0; i < 15; i++) {
      sysputs("P");
      syssleep(1500);
    }
    sprintf(buff, "Producer finished\n");
    sysputs( buff );
    sysstop();
}

void consumer( void ) {
/****************************/

    int         i;
    char        buff[100];
    void (*oldhandler)(void *);
    syssighandler(0, sig_handler, &oldhandler);

    for(i = 0; i < 50000; i++);
    syssleep(3000);
    for( i = 0; i < 10; i++ ) {
      sprintf(buff, "Consumer %d\n", i);
      sysputs( buff );
      syssleep(1500);
      sysyield();
    }

    for (i = 0; i < 40; i++) {
      sysputs("C");
      syssleep(700);
    }

    sprintf(buff, "Consumer finished\n");
    sysputs( buff );
    sysstop();
}

void     root( void ) {
/****************************/

    char  buff[100];
    int pids[5];
    int proc_pid, con_pid;
    int i;

    sysputs("Root has been called\n");


    // Test for ready queue removal. 
   
    proc_pid = syscreate(&busy, 1024);
    con_pid = syscreate(&busy, 1024);
    sysyield();
    syskill(proc_pid, 0);
    sysyield();
    syskill(con_pid, 0);

    
    for(i = 0; i < 5; i++) {
      pids[i] = syscreate(&busy, 1024);
    }

    sysyield();
    
    syskill(pids[3], 0);
    sysyield();
    syskill(pids[2], 0);
    syskill(pids[4], 0);
    sysyield();
    syskill(pids[0], 0);
    sysyield();
    syskill(pids[1], 0);
    sysyield();

    syssleep(8000);;



    kprintf("***********Sleeping no kills *****\n");
    // Now test for sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    sysyield();
    syssleep(8000);;



    kprintf("***********Sleeping kill 2000 *****\n");
    // Now test for removing middle sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    syssleep(110);
    syskill(pids[1], 0);
    syssleep(8000);;

    kprintf("***********Sleeping kill last 3000 *****\n");
    // Now test for removing last sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    sysyield();
    syskill(pids[2], 0);
    syssleep(8000);;

    kprintf("***********Sleeping kill first process 1000*****\n");
    // Now test for first sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    syssleep(100);
    syskill(pids[0], 0);
    syssleep(8000);;

    // Now test for 1 process


    kprintf("***********One sleeping process, killed***\n");
    pids[0] = syscreate(&sleep2, 1024);

    sysyield();
    syskill(pids[0], 0);
    syssleep(8000);;

    kprintf("***********One sleeping process, not killed***\n");
    pids[0] = syscreate(&sleep2, 1024);

    sysyield();
    syssleep(8000);;



    kprintf("***********Three sleeping processes***\n");    // 
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);


    // Producer and consumer started too
    proc_pid = syscreate( &producer, 4096 );
    con_pid = syscreate( &consumer, 4096 );
    sprintf(buff, "Proc pid = %d Con pid = %d\n", proc_pid, con_pid);
    sysputs( buff );


    processStatuses psTab;
    int procs;
    



    syssleep(500);
    procs = sysgetcputimes(&psTab);

    for(int j = 0; j <= procs; j++) {
      sprintf(buff, "%4d    %4d    %10d\n", psTab.pid[j], psTab.status[j], 
        psTab.cpuTime[j]);
      kprintf(buff);
    }


    syssleep(10000);
    procs = sysgetcputimes(&psTab);

    for(int j = 0; j <= procs; j++) {
      sprintf(buff, "%4d    %4d    %10d\n", psTab.pid[j], psTab.status[j], 
        psTab.cpuTime[j]);
      kprintf(buff);
    }

    sprintf(buff, "Root finished\n");
    sysputs( buff );
    sysstop();
    
    for( ;; ) {
     sysyield();
    }
    
}


