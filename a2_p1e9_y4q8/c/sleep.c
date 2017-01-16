/* sleep.c : sleep device
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>


/* Your code goes here */
extern pcb      *sleep_head;  /* Head of the sleeping blocked list */
extern pcb      *sleep_tail;  /* Tail of the sleeping blocked list */

/*
 * Sleep the process for at least the specified amount of miliseconds.
 * 
 * Arguments:
 *  sleeping_pcb - the process to sleep
 *
 * Convert the milliseconds to sleep into number of time slices.
 * Add the process to the sleeping blocked list, and set the pcb properties
 * to sleep.
 */
void sleep( pcb *sleeping_pcb ) {

  va_list ap = (va_list)sleeping_pcb->args;
  unsigned int milliseconds = va_arg( ap, unsigned int );

  // Calculate the number of timeslices needed to acheive the minimum millisecond sleep
  unsigned long delta_ticks = ( milliseconds / TIME_SLICE ) ;
  if ( milliseconds % TIME_SLICE ) {
    delta_ticks++;
  }


  // Decrement delta_ticks and insert the pcb on the sleeping delta list
  pcb *node;
  for (node = sleep_head; node && node->buffer < delta_ticks; node = node->next) {
    delta_ticks -= node->buffer;
  }

  // Set the buffer and the state of p
  sleeping_pcb->state = STATE_BLOCKED_SLEEP;
  sleeping_pcb->buffer = delta_ticks;

  if ( !sleep_head ) {
    // Insert as head of list if list is empty
    insertAfter(&sleep_head, &sleep_tail, NULL, sleeping_pcb);
  } else if ( node ) {
    // Insert in middle of list
    insertAfter(&sleep_head, &sleep_tail, node->prev, sleeping_pcb);
  } else {
    // Insert after tail if we ran off end of list
    insertAfter(&sleep_head, &sleep_tail, sleep_tail, sleeping_pcb);
  }

  // Decrement the interval of the rest of the list
  for ( ; node; node = node->next) {
    node->buffer -= delta_ticks;
  }
}

/*
 * Advances the countdown on sleeping processes.
 *
 * If sleeping processes exists, decrement the countdown by one.
 * If the countdown reaches 0, ready the sleeping process.
 */
void tick( void ) {

  if (!sleep_head) {
    return;
  }

  sleep_head->buffer--;

  pcb *p;
  for(p = sleep_head; p && p->buffer <= 0; p = p->next) {
    p->ret = 0;
    p->buffer = NULL;
    dequeue(&sleep_head, &sleep_tail);
    ready(p);
  }
}
