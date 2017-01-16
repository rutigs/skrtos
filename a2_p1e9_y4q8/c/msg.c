/*
 * msg.c : messaging system
 *
 * this file is responsible for messaging between processes
 *
 * Public function:
 *
 * void send( pcb *sending_pcb );
 *    this method takes the pcb that is trying to send a message to another
 *    process and handles the message passing. It will check the parameters,
 *    move pcbs into different queues and adjust process state depending
 *    on what needs to be done
 *
 * void recv( pcb *receiving_pcb );
 *    this method takes the pcb that trying to receive a message from
 *    another process ( or any ) and handles the message passing. It will
 *    check parameters, move pcbs into queues and adjust the process state
 *    depending on what needs to be done.
 */

#include <xeroskernel.h>
#include <stdarg.h>

/* Your code goes here */

extern pcb      *recv_any_head;   /* Head of the recv any blocked list */
extern pcb      *recv_any_tail;   /* Tail of the recv any blocked list */

/*
 *   Takes the given pcb, checks its arguments, and tries to complete the
 *   message passing based on the following criteria
 *
 *   if trying to send to a non-existent process place on ready queue with
 *   return code -1
 *
 *   if trying to send to itself place on ready queue with return code -2
 *
 *   if the arguments are invalid place on ready queue with return code -3
 *
 *   if there is a corresponding recv on the blocked queue
 *   -> get the receiver's blocked_pcb
 *   -> put the msg in the mem_locn for the receiver
 *   -> put receiver on ready queue with return code 0
 *   -> put sender on the ready queue with return code 0
 *   -> free blocked_pcb
 *
 *   if there is NOT a corresponding recv on the blocked
 *   -> place the sender pcb on the blocked queue with buffer set to num
 */
void send( pcb *sending_pcb ) {

  va_list ap = (va_list)sending_pcb->args;
  int dest_pid = va_arg( ap, int );
  unsigned long num_to_send = va_arg( ap, unsigned long );

  // check for non-null destination before we do anything
  if ( !dest_pid ) {
    sending_pcb->ret = -3;
    ready( sending_pcb );
    return;
  }

  // kprintf( "pid %d is trying to send to pid %d\n", sending_pcb->pid, dest_pid );

  if ( sending_pcb->pid == dest_pid ) {
    sending_pcb->ret = -2;
    ready( sending_pcb );
    return;
  }

  pcb *dest = get_pcb_by_id(dest_pid);

  // If there is no valid process to send to
  if (!dest) {
    // kprintf( "there is no valid receiving process!\n" );
    sending_pcb->ret = -1;
    ready( sending_pcb );
    return;
  }

  // Check the destination process is blocked for receive from this , or any, process
  if ( dest->state == STATE_BLOCKED_RECV &&
       ( dest->dest_pid == sending_pcb->pid || dest->dest_pid == 0 ) ) {

    // Remove blocked reciever from blocked queue
    if( dest->dest_pid != 0 ) {
      // Remove from process's blocked queue
      remove(&sending_pcb->recv_head, &sending_pcb->recv_tail, dest);
    } else {
      //Remove from receive any's blocked queue
      remove(&recv_any_head, &recv_any_tail, dest);
    }

    // kprintf( "blocked receiver found!\n" );

    // Put the sending number at the receiver's address
    unsigned long *mem_locn = dest->mem_locn;
    *mem_locn = num_to_send;

    // Set the return code and add the receiver back to the ready queue
    // for recv, the buffer is the address of pid to set
    *(int *) dest->buffer = sending_pcb->pid;
    dest->ret = 0;
    ready( dest );

    // Set the return code and add the sender back to the ready queue
    sending_pcb->ret = 0;
    ready( sending_pcb );

  } else {
    // The destination process exitsts but is not waiting to recieve from sending process

    // kprintf( "receiver is still running!\n" );
    // add the pcb to the dest's sending blocked queue
    enqueue(&dest->send_head, &dest->send_tail, sending_pcb);

    // Store the arguments on the pcb
    sending_pcb->buffer = num_to_send;
    sending_pcb->dest_pid = dest_pid;
    sending_pcb->state = STATE_BLOCKED_SEND;
  }
}

/*
 *   Takes the given pcb, checks its arguments, and tries to complete the
 *   message passing based on the following criteria
 *
 *   if pid to receive from is invalid place it on the ready queue with return
 *   code -1
 *
 *   if trying to receive from itself place it on the ready queue with return
 *   code -2
 *
 *   if the arguments are invalid place it on the ready queue with return code -3
 *
 *   if there is a corresponding send on the blocked queue
 *   -> get the msg from the buffer and copy it to location num
 *   -> mark the blocked pcb's return code to success 0 and move it to the ready queue
 *   -> put receiver on ready queue with return 0
 *   -> free blocked_pcb
 *
 *   if there is NOT a corresponding send on the blocked queue
 *   -> put the receiver pcb on the blocked queue
 */
void recv( pcb *receiving_pcb ) {

  va_list ap = (va_list)receiving_pcb->args;
  int *from_pid = va_arg( ap, unsigned int* );
  unsigned long *mem_locn = va_arg( ap, unsigned long* );

  // kprintf( "pid %d is trying to receive from pid %d\n", receiving_pcb->pid, *from_pid );

  if ( !from_pid || !mem_locn ) {
    receiving_pcb->ret = -3;
    ready( receiving_pcb );
    return;
  }

  if ( *from_pid == receiving_pcb->pid ) {
    receiving_pcb->ret = -2;
    ready( receiving_pcb );
    return;
  }

  pcb *from = get_pcb_by_id(*from_pid);

  // For receiving from any process, get the first process blocked on send.
  if (*from_pid == 0) {
    from = dequeue( &receiving_pcb->send_head, &receiving_pcb->send_tail );
  }

  // If there is no valid process to receive from
  if ( !from && *from_pid != 0 ) {
    // kprintf( "there is no valid receiving process!\n" );
    receiving_pcb->ret = -1;
    ready( receiving_pcb );
    return;
  }

  // Check the destination process is blocked waiting to send to this process
  if ( from && from->state == STATE_BLOCKED_SEND && from->dest_pid == receiving_pcb->pid ) {

    // Remove blocked sender from blocked queue
    remove( &receiving_pcb->send_head, &receiving_pcb->send_tail, from );

    // kprintf("blocked sender found!\n");

    // Get the number from the buffer and set pid
    unsigned long num_to_send = from->buffer;
    *mem_locn = num_to_send;
    *from_pid = from->pid;

    // Set the return code and add the sender back to the ready queue
    from->ret = 0;
    ready( from );

    // Set the return code and add the receiver back to the ready queue
    receiving_pcb->ret = 0;
    ready( receiving_pcb );

  } else {
    // The destination process exitsts but is not waiting to send from receiving process

    // kprintf( "sender is still running!\n" );
    if (from) {
      // add the pcb to the from's receiving blocked queue if from exits
      enqueue(&from->recv_head, &from->recv_tail, receiving_pcb);
    } else {
      // this is a receive from any call. Add pcb to recv_any blocked queue
      enqueue(&recv_any_head, &recv_any_tail, receiving_pcb);
    }

    // Store the arguments on the pcb
    receiving_pcb->mem_locn = mem_locn;
    receiving_pcb->dest_pid = *from_pid;
    receiving_pcb->state = STATE_BLOCKED_RECV;
    receiving_pcb->buffer = (unsigned long) from_pid;
  }
}

