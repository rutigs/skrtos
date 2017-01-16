/*
 * user.c : User processes
 *
 * This file is responsible for code that would take place in user processes that is
 * multitasked by the kernel
 *
 * Public functions:
 * - void root( void );
 *      This method starts the root 'process' in which creates child processes to demo
 *      sysput, sysgetpid, syssend, sysrecv, and syssleep.
 */


#include <xeroskernel.h>
#include <xeroslib.h>


/*
 * Child process that will declare self is alive, 
 * recv a message, sleep, and return.
 */
void child( void ) {

  char *str[500];
  int pid = sysgetpid();
  sprintf( (char *)str, "Child is alive with pid: %d \n", pid );
  sysputs( (char *)str );

  int from_pid = 0;
  unsigned long num;
  int result = sysrecv(&from_pid, &num);

  sprintf( (char *)str, "Messaged recieved with status: %d, Process %d will sleep for %d milliseconds.\n",
           result, pid, num );
  sysputs( (char *)str );

  result = syssleep(num);
  sprintf( (char *)str, "Process %d woke from sleep with result %d. Bye\n", pid, result );
  sysputs( (char *)str );
}



/*
 * Root that creates 4 children and tells them to sleep.
 */
void root( void ) {

  int child_proc, child_proc2, child_proc3, child_proc4, result;
  char *str[500];
  
  sysputs("Root has been called and is alive.\n");

  // Creates child processes
  child_proc = syscreate( &child, 4096 );
  sprintf( (char *)str, "Root created first child with pid: %d \n", child_proc );
  sysputs( (char *)str );

  child_proc2 = syscreate( &child, 4096 );
  sprintf( (char *)str, "Root created second child with pid: %d \n", child_proc2 );
  sysputs( (char *)str );

  child_proc3 = syscreate( &child, 4096 );
  sprintf( (char *)str, "Root created third child with pid: %d \n", child_proc3 );
  sysputs( (char *)str );

  child_proc4 = syscreate( &child, 4096 );
  sprintf( (char *)str, "Root created fourth child with pid: %d \n", child_proc4 );
  sysputs( (char *)str );

  // Sleep for 4 seconds
  sysputs( "Root is going to sleep for 4 seconds.\n" );
  result = syssleep( 4000 );
  sysputs( "Root woke from sleep.\n" );

  // Sends messages to children to tell them to sleep
  sprintf( (char *)str, "Root is telling process: %d to sleep for 10 seconds.\n", child_proc3 );
  sysputs( (char *)str );
  result = syssend(child_proc3, 10000 );

  sprintf( (char *)str, "Root is telling process: %d to sleep for 7 seconds.\n", child_proc2);
  sysputs( (char *)str );
  result = syssend(child_proc2, 7000 );

  sprintf( (char *)str, "Root is telling process: %d to sleep for 20 seconds.\n", child_proc);
  sysputs( (char *)str );
  result = syssend(child_proc, 20000 );

  sprintf( (char *)str, "Root is telling process: %d to sleep for 27 seconds.\n", child_proc4);
  sysputs( (char *)str );
  result = syssend(child_proc4, 27000 );

  // Attempt to recv from child_proc4
  int from_pid = child_proc4;
  unsigned long num;
  result = sysrecv(&from_pid, &num);
  sprintf( (char *)str, "Root tried to recv message from process: %d, result is: %d.\n",
           child_proc4, result);
  sysputs( (char *)str );

  // Attempt to send to child_proc3
  result = syssend(child_proc3, 12345 );
  sprintf( (char *)str, "Root tried to send message to process: %d, result is: %d.\n",
           child_proc3, result );
  sysputs( (char *)str );

  sysputs("End of Root\n");
  
  sysstop();
}




/*
 * ==============================================================================
 *                                    Testing code
 * ==============================================================================
 */


/*
 * Tests sysgetpid for this process and child processes.
 * Tests Return from process.
 */
void test_sysgetpid_and_return( void ) {
  int expected_pid, pid, child_proc, child_proc2;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__ );
  sysputs( (char *)str );

  expected_pid = 2;
  pid = sysgetpid();

  sprintf( (char *)str, "%s: expected: %d, pid: %d\n", __func__, expected_pid, pid );
  sysputs( (char *)str );

  child_proc = syscreate( &test_sysgetpid_helper, 4096 );
  child_proc2 = syscreate( &test_sysgetpid_helper, 4096 );
  sprintf( (char *)str, "%s: expecting child pid's: %d and %d\n", __func__, child_proc, child_proc2 );
  sysputs( (char *)str );

  sysyield();
  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
  sysstop();
}

/*
 * Helper for testing sysgetpid.
 */
void test_sysgetpid_helper( void ) {
  char *str[500];
  int pid = sysgetpid();
  sprintf( (char *)str, "%s has pid: %d\n", __func__, pid );
  sysputs( (char *)str );
  return;
}

/*
 * Tests sysput for this process and child processes.
 */
void test_sysputs( void ) {
  int child_proc, child_proc2;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__ );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - sysput. vs test: skrrrt\n\0 this should not be printed\n", __func__ );
  sysputs( (char *)str );

  child_proc = syscreate( &test_sysputs_helper, 4096 );
  child_proc2 = syscreate( &test_sysputs_helper, 4096 );
  sprintf( (char *)str, "%s - sysputs expecting pids: %d and %d \n", __func__, child_proc, child_proc2 );
  sysputs( (char *)str );

  sysyield();
  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
  sysstop();
}

/*
 * Helper to test sysput.
 */
void test_sysputs_helper( void ) {
  int pid = sysgetpid();
  char *str[20];
  sprintf( (char *)str, "%s - sysputs: has pid: %d\n", __func__, pid );
  sysputs( (char *)str );
  sysstop();
}

/*
 * Test syskill.
 */
void test_syskill( void ) {
  int i, pid, child_proc, result;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__  );
  sysputs( (char *)str );

  // Try to kill itself
  pid = sysgetpid();
  result = syskill( pid );
  sprintf( (char *)str, "%s has tried to kill itself with result: %d\n", __func__, result );
  sysputs( (char *)str );

  // Try to kill a process that doesn't exist
  result = syskill( 1337 );
  sprintf( (char *)str, "%s has tried to the process that doesn't exist  with result: %d\n", __func__, result );
  sysputs( (char *)str );

  child_proc = syscreate( &test_syskill_helper, 4096 );

  // Yield to let the child process try to kill itself
  for ( i = 0; i < 4; i++ ) {
    sysyield();
  }

  result = syskill(child_proc);
  sprintf( (char *)str, "%s - child process kill result: %d\n", __func__, result );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
  return;
}

/*
 * Helper to test syskill.
 */
void test_syskill_helper( void ) {
  char *str[500];
  int i, pid, result;

  // Try to kill itself
  pid = sysgetpid();
  result = syskill( pid );

  sprintf( (char *)str, "%s has tried to kill itself with result: %d\n", __func__, result );
  sysputs( (char *)str );

  for ( i = 0; i < 10; i++ ) {
    sprintf( (char *)str, "%s is waiting to be killed... \n", __func__ );
    sysputs( (char *)str );
    sysyield();
  }
}

// This is used for the helper processes;
int send_recv_parent_pid;
int child_to_kill_pid;
unsigned long send_num = 1337;


/*
 * Test the syscalls for send.
 */
void test_syssend( void ) {
  char *str[500];
  int child_pid, result;

  sprintf( (char *)str, "%s has been called\n", __func__  );
  sysputs( (char *)str );

  send_recv_parent_pid = sysgetpid();

  // Send Tests:

  // Test 1. Send, recv is not blocked, recv is valid
  child_pid = syscreate( &test_sysrecv_helper, 4096 );
  result = syssend( child_pid, send_num );
  sprintf( (char *)str, "%s - Send Test 1: expected = 0, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 2. Send, recv is blocked, recv is valid
  child_pid = syscreate( &test_sysrecv_helper, 4096 );
  sysyield(); // yield so the child recv can block
  result = syssend( child_pid, send_num );
  sprintf( (char *)str, "%s - Send Test 2: expected = 0, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 3. Send, no valid recv process
  result = syssend( child_pid, send_num );
  sprintf( (char *)str, "%s - Send Test 3: expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 4. Send to itself
  result = syssend( send_recv_parent_pid, send_num );
  sprintf( (char *)str, "%s - Send Test 4: expected = -2, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 5. Send to a process that terminates
  child_pid = syscreate( &test_stop_helper, 4096 ); // will stop immediately when run
  result = syssend( child_pid, send_num ); //send blocks, child_pid is killed
  sprintf( (char *)str, "%s - Send Test 5: expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 6. Send with invalid parameters 1. invalid child_pid
  result = syssend( NULL, send_num );
  sprintf( (char *)str, "%s - Send Test 6: expected = -3, result = %d\n", __func__, result );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
}


/*
 * Test the syscalls for recv.
 */
void test_sysrecv( void ) {
  int child_pid, result;
  unsigned long recv_num;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__  );
  sysputs( (char *)str );

  send_recv_parent_pid = sysgetpid();

  // Test 1. Recv, send is not blocked, send is valid.
  child_pid = syscreate( &test_syssend_helper, 4096 );
  result = sysrecv( &child_pid, &recv_num );
  sprintf( (char *)str, "%s - Recv Test 1: expected = 0, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 2. Recv, send is blocked, send is valid
  child_pid = syscreate( &test_syssend_helper, 4096 );
  sysyield(); // yield so child send can block
  result = sysrecv( &child_pid, &recv_num );
  sprintf( (char *)str, "%s - Recv Test 2: expected = 0, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 3. Recv, no valid recv process
  result = sysrecv( &child_pid, &recv_num );
  sprintf( (char *)str, "%s - Recv Test 3: expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 4. Recv to itself
  result = sysrecv( &send_recv_parent_pid, &recv_num );
  sprintf( (char *)str, "%s - Recv Test 4: expected = -2, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 5. Recv from any, send is blocked, send is valid
  child_pid = syscreate( &test_syssend_helper, 4096 );
  int any_pid = 0;
  sysyield(); // yield so child send can block
  result = sysrecv( &any_pid, &recv_num );
  sprintf( (char *)str, "%s - Recv Test 5: expected = 0, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 6. Recv from a process that terminates
  child_pid = syscreate( &test_stop_helper, 4096 ); // will stop immediately when run
  result = sysrecv( &child_pid, &recv_num ); //recieve blocks, child_pid is killed
  sprintf( (char *)str, "%s - Recv Test 6: expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 7. Recv from any, other process terminates
  child_pid = syscreate( &test_stop_helper, 4096 ); // will stop immediately when run
  any_pid = 0;
  result = sysrecv( &any_pid, &recv_num ); //recieve blocks, only other process is killed
  sprintf( (char *)str, "%s - Recv Test 7: expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 8. Invalid parameters 1. from_pid
  result = sysrecv( NULL, &recv_num ); // The pid itself can be NULL (0) but the address of it cannot be
  sprintf( (char *)str, "%s - Recv Test 8: expected = -3, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 9. Invalid parameters 2. mem_locn
  result = sysrecv( &any_pid, NULL );
  sprintf( (char *)str, "%s - Recv Test 9: expected = -3, result = %d\n", __func__, result );
  sysputs( (char *)str );

  // Test 10. Invalid parameters 3. from_pid and mem_locn
  result = sysrecv( NULL, NULL );
  sprintf( (char *)str, "%s - Recv Test 10: expected = -3, result = %d\n", __func__, result );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
}


/*
 * Test the syscalls for send and recv when blocked and killed.
 * Note: sometimes these fail because of pre-emption
 */
void test_send_recv_kill( void ) {
  int result;
  unsigned long recv_num;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__  );
  sysputs( (char *)str );

  send_recv_parent_pid = sysgetpid();


  //Test 1. Send, recv is originally valid, recv dies
  child_to_kill_pid = syscreate( &test_sysrecv_helper, 4096 );
  sysyield(); // Allow recv to block
  syscreate( &test_child_killer_helper, 4096 );
  sysyield(); // kills process blocked on recv
  sysyield();
  result = syssend( child_to_kill_pid, send_num ); //this is the only process that exits
  sprintf( (char *)str, "%s - Misc Test 1. expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  //Test 2. Recv, send is originally valid, send dies
  syscreate( &test_child_killer_helper, 4096 ); // killer on ready queue
  child_to_kill_pid = syscreate( &test_syssend_helper, 4096 ); // sender on ready queue
  sprintf( (char *)str, "%s created child to kill with  pid %d\n", __func__, child_to_kill_pid );
  sysputs( (char *)str );

  //recv blocks, sending process killed, recv should return error
  result = sysrecv( &child_to_kill_pid, &recv_num );
  sprintf( (char *)str, "%s - Misc Test 2. expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  //Test 3. Kill process blocked on recv any
  child_to_kill_pid = syscreate( &test_sysrecv_any_helper, 4096 );
  sysyield(); // Allow recv to block
  syscreate( &test_child_killer_helper, 4096 );
  sysyield(); // kills process blocked on recv any
  result = syssend( child_to_kill_pid, send_num ); //this is the only process that exits
  sprintf( (char *)str, "%s - Misc Test 3. expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  //Test 4. Kill process blocked on send
  child_to_kill_pid = syscreate( &test_syssend_helper, 4096 );
  sysyield(); // Allow send to block
  syscreate( &test_child_killer_helper, 4096 );
  sysyield(); // kills process blocked on send
  sysyield();
  result = sysrecv( &child_to_kill_pid, &recv_num ); //this is the only process that exits
  sprintf( (char *)str, "%s - Misc Test 4. expected = -1, result = %d\n", __func__, result );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
}


/*
 * Helper for testing syssend. Tries to send to send_recv_parent_pid.
 */
void test_syssend_helper( void ) {
  char *str[500];
  int result = syssend( send_recv_parent_pid, send_num );
  if ( result == 0 ) {
    sprintf( (char *)str, "%s has sent %lu\n", __func__, send_num );
    sysputs( (char *)str );
  }

  sysstop();
}

/*
 * Helper for testing sysrecv. Tries to recieve from send_recv_parent_pid.
 */
void test_sysrecv_helper( void ) {
  char *str[500];
  int result;
  unsigned long recv_num;
  int from_pid = send_recv_parent_pid;
  result = sysrecv( &from_pid, &recv_num );
  if ( result == 0 && recv_num != send_num) {
    sprintf( (char *)str, "Test Failed: %s has received %lu\n", __func__, recv_num );
    sysputs( (char *)str );
  }

  sysstop();
}

/*
 * Helper for testing sysrecv. Tries to recieve from any process.
 */
void test_sysrecv_any_helper( void ) {
  char *str[500];
  int result;
  unsigned long recv_num;
  int from_pid = 0;
  result = sysrecv( &from_pid, &recv_num );
  if ( result == 0 && recv_num != send_num ) {
    sprintf( (char *)str, "Test Failed: %s has received %lu\n", __func__, recv_num );
    sysputs( (char *)str );
  }

  sysstop();
}

/*
 * Helper for testing terminating send/recv when the destination process dies.
 * Kills process with pid child_to_kill_pid.
 */
void test_child_killer_helper( void ) {
  char *str[500];

  sprintf( (char *)str, "%s is killing child pid %d\n", __func__, child_to_kill_pid );
  sysputs( (char *)str );
  syskill( child_to_kill_pid );
  sysstop();
}

/*
 * Helper for testing terminating send/recv when the destination process dies.
 * Stops immediatly when run
 */
void test_stop_helper( void ) {
  sysstop();
}


/*
 * Test syssleep.
 */
void test_syssleep( void ) {
  int child_proc, result;
  char *str[500];

  sprintf( (char *)str, "%s has been called\n", __func__  );
  sysputs( (char *)str );

  // Try sleep for 1 sec (1000 milliseconds)
  sprintf( (char *)str, "%s is going to sleep for 1000 milliseconds\n", __func__ );
  sysputs( (char *)str );
  result = syssleep( 1000 );
  sprintf( (char *)str, "%s Woke from sleep. expected = 0, result = %d \n\n", __func__, result );
  sysputs( (char *)str );

  // Two processes sleep. Helper wakes first
  child_proc = syscreate( &test_syssleep_helper, 4096 );
  sysyield();

  sprintf( (char *)str, "%s is going to sleep for 1304 milliseconds.\n", __func__ );
  sysputs( (char *)str );
  result = syssleep( 1304 );
  sprintf( (char *)str, "%s Woke from sleep. expected = 0, result = %d \n\n", __func__, result );
  sysputs( (char *)str );

  // Helper sleeps first; root sleeps shorter. 
  child_proc = syscreate( &test_syssleep_helper, 4096 );
  sysyield();
  sprintf( (char *)str, "%s is going to sleep for 200 milliseconds\n", __func__ );
  sysputs( (char *)str );
  result = syssleep( 200 );
  sprintf( (char *)str, "%s Woke from sleep. expected = 0, result = %d \n\n", __func__, result );
  sysputs( (char *)str );
  sprintf( (char *)str, "%s is going to sleep for 1000 milliseconds\n", __func__ );
  sysputs( (char *)str );
  result = syssleep( 1000 ); //Sleep again to finish this test case
  sprintf( (char *)str, "%s Woke from sleep. expected = 0, result = %d \n\n", __func__, result );
  sysputs( (char *)str );

  // Kill process in sleep
  child_proc = syscreate( &test_syssleep_helper, 4096 );
  sysyield();
  result = syskill( child_proc );
  sprintf( (char *)str, "%s Killed sleeping child. expected = 0, result = %d \n", __func__, result );
  sysputs( (char *)str );

  sprintf( (char *)str, "%s - TESTS COMPLETE\n", __func__ );
  sysputs( (char *)str );
}

/*
 * Helper to test sysleep.
 */
void test_syssleep_helper( void ) {
  int result;
  char *str[500];

  sprintf( (char *)str, "%s is going to sleep for 520 milliseconds\n", __func__ );
  sysputs( (char *)str );
  result = syssleep( 520 );

  sprintf( (char *)str, "%s Woke from sleep. expected = 0, result = %d \n", __func__, result );
  sysputs( (char *)str );
  return;
}
