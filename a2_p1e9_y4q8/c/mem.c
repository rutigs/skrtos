/*
 * Note: This is the memory manager taken from our (p1e9 and y4q8) Assignment 1. It has
 *       kfree with coalesceing implemented.
 *
 * mem.c : memory manager
 *
 * This file is responsible for the memory management of the kernel including allocation
 * and freeing of available memory
 *
 * Public functions:
 * - void kmeminit();
 *     This method is responsible for initializing two memory headers, one at the start
 *     of available memory with size up to the hole, and another from the end of the hole 
 *     to the end of memory. Both of these headers are 16 byte aligned. The method also
 *     puts free memory footers of each slot to help manage memory coalescing. The first
 *     header next will point to the second header, the second header prev will point to
 *     the first header.
 *
 * - void *kmalloc(int size);
 *     kmalloc scans the free list for a slot of memory that is of given size + 16 and is
 *     16 byte aligned and returns a pointer to the requested size of memory after putting
 *     a header in place. kmalloc will adjust the free list and create new mem headers to
 *     keep track of the free memory after this allocation. Kmalloc will also give each
 *     allocated memory slot a unique sanity check as well as each free memory slot a 
 *     unique sanity check for error checking.
 *
 *     Returns:
 *     -> NULL if there is not enough memory available or the request is <= 0
 *     -> void pointer to the start of the requested memory
 *
 * - void kfree( void *ptr );
 *     kfree takes a given pointer to a chunk of memory, looks behind it at its mem header
 *     and adds it back to the free list of memory. It will verify the slot the is valid 
 *     using the sanity checks and their functions and may potentially coalesce memory if
 *     the boundaries of free slots are adjacent.
 */


#include <xeroskernel.h>
#include <i386.h>

/* Your code goes here */


/* internal data structure for managing memory */
struct mem_header_t {
  unsigned long size;         
  struct mem_header_t *prev;  
  struct mem_header_t *next;
  unsigned long sanity_check;   /* Also a free/allocated flag.  */
  unsigned char dataStart[0];
};

/* internal data structure for coalesceing memory. 16 bytes */
struct free_mem_footer_t {
  unsigned long size;         
  unsigned long placeholder[2];  
  unsigned long sanity_check;
};

/* Free List start*/
struct mem_header_t *mem_head;

/* Need beginning and end addresses for initialization */
extern long freemem;
extern char *maxaddr;

/* Internal helpers */
unsigned long amem_sanity_hash( void *ptr, unsigned long size );
unsigned long fmem_sanity_hash( void *ptr, unsigned long size );
void          put_free_mem_footer ( struct mem_header_t* free_mem_header );
void          coalesce( struct mem_header_t* first_mem, struct mem_header_t* last_mem );


/* 
 * Initialize free memory list.
 */
void kmeminit( void ) 
{
  /* 16 byte align start of memory */
  unsigned long start_of_mem = 16 * (freemem/16 + ((freemem%16)?1:0));

  /* Initialize first mem header at start_of_mem to start of the hole*/
  mem_head = (struct mem_header_t*) start_of_mem;
  mem_head->size = ((long) HOLESTART) - start_of_mem - sizeof(struct mem_header_t);
  mem_head->sanity_check = fmem_sanity_hash(mem_head, mem_head->size);

  /* Put free_mem_footer at end of free block before hole */
  put_free_mem_footer(mem_head);

  /* Initialize second mem header from end of the hole to end of mem */
  struct mem_header_t *mem_slot = (struct mem_header_t*) HOLEEND;
  mem_slot->size = ((long) maxaddr) - ((long) HOLEEND) - sizeof(struct mem_header_t);
  mem_slot->sanity_check = fmem_sanity_hash(mem_slot, mem_slot->size);

  /* Put free_mem_footer at end of free block after hole */
  put_free_mem_footer(mem_slot);


  /* Point the 2 initial nodes to each other */
  mem_head->next = mem_slot;
  mem_head->prev = NULL;

  mem_slot->next = NULL;
  mem_slot->prev = mem_head;
}

/*
 * Allocates a block of 16 byte aligned memory of requested size.
 * 
 * Arguments:
 *  size - the mininum size of the memory block to allocate
 *
 * Returns:
 *  pointer to memory block of at least requested size.
 *  NULL on error 
 */
void *kmalloc( size_t size )
{
  if (size <= 0) {
    kprintf("\nError at kmalloc: Invalid Size: %d", size);
    return NULL;
  }

  unsigned long amnt = size/16 + ((size%16)?1:0);
  amnt = amnt*16 + sizeof(struct mem_header_t);

  struct mem_header_t *curr_mem = mem_head;
  while (curr_mem->size < amnt) {

    if (curr_mem->next == NULL) {
      kprintf("Error in kmalloc: Out of memory - Requested size too large. Requested size: %d", size);
      return NULL;
    }
    curr_mem = curr_mem->next;
  }

  /* Put new mem_header at start of next node. */
  struct mem_header_t *next_mem = (void *) curr_mem + amnt;

  next_mem->size = curr_mem->size - amnt;
  next_mem->sanity_check = fmem_sanity_hash(next_mem, next_mem->size);

  /* Put free_mem_footer at end of free block */
  if (next_mem->size >= sizeof(struct free_mem_footer_t) ) {
    put_free_mem_footer(next_mem);
  }

  /* Add new mem_header to free list. */
  next_mem->prev = curr_mem->prev;
  if (curr_mem->prev != NULL) {
    curr_mem->prev->next = next_mem;
  } else {
    /* If curr_mem is start of list, update mem_head (head of free list). */
    mem_head = next_mem;
  }

  next_mem->next = curr_mem->next;
  if (curr_mem->next != NULL) {
    curr_mem->next->prev = next_mem;
  }

  /* Make curr_mem the allocated memory. */
  curr_mem->size = amnt - sizeof(struct mem_header_t);
  curr_mem->sanity_check = amem_sanity_hash(curr_mem, curr_mem->size);

  return (void *) curr_mem->dataStart;
}

/*
 * Free memory block starting at ptr.
 *
 * Arguments:
 *  ptr - pointer to the block of memory to be freed.
 * 
 * Returns:
 *  0 on success
 *  -1 on error
 */
int kfree(void * ptr)
{
  struct mem_header_t *curr_mem = ptr - sizeof(struct mem_header_t);

  if (curr_mem->sanity_check != amem_sanity_hash(curr_mem, curr_mem->size)) {
    kprintf("Error in kfree: sanity check failed for memory at: %x", ptr);
    return -1;
  }

  /* NULL pointers for coalesce */
  curr_mem->prev = NULL;
  curr_mem->next = NULL;

 
  struct mem_header_t *next_mem_block = ptr + curr_mem->size;
  if (next_mem_block->sanity_check == fmem_sanity_hash(next_mem_block, next_mem_block->size)) {
    /* Memory block immediately after is a free block. Coalesce. */
    coalesce(curr_mem, next_mem_block);
  }

  struct free_mem_footer_t *prev_mem_footer = (void *) curr_mem - sizeof(struct free_mem_footer_t);
  if (prev_mem_footer->sanity_check == fmem_sanity_hash(prev_mem_footer, prev_mem_footer->size)) {
    /* Memory block immediately before is a free block. Coalesce and update mem_header. */
    struct mem_header_t *prev_mem_header = (void *) prev_mem_footer - prev_mem_footer->size;
    coalesce(prev_mem_header, curr_mem);
  } else {
    /* Memory block cannot be coalesed with one before it, so add block to front of free list */
    curr_mem->prev = NULL;
    curr_mem->next = mem_head;
    mem_head->prev = curr_mem;
    curr_mem->sanity_check = fmem_sanity_hash(curr_mem, curr_mem->size);
    mem_head = curr_mem;
    put_free_mem_footer(mem_head);
  }
  return 0;
}

/*
 * Sanity check creation function for allocated memory.
 *
 * Based on "Knuth's Multiplicative Method" 
 */
unsigned long amem_sanity_hash( void *ptr, unsigned long size )
{
    return ((unsigned long) ptr) * ((unsigned long)2654435761) + size;
}

/*
 * Sanity check creation function for free memory.
 *
 * Based on "Knuth's Multiplicative Method"
 */
unsigned long fmem_sanity_hash( void *ptr, unsigned long size )
{
    return ((unsigned long) ptr) * ((unsigned long)265435761) + size; 
}

/*
 * Adds a free_mem_header_t at the end of the free memory block, for coalecsing
 *
 * ptr must be a pointer to a valid mem_header_t
 */
void put_free_mem_footer( struct mem_header_t* free_mem_header ) 
{
  struct free_mem_footer_t *free_mem_footer = (void *) free_mem_header + free_mem_header->size;
  free_mem_footer->size = free_mem_header->size;
  free_mem_footer->sanity_check = fmem_sanity_hash(free_mem_footer, free_mem_footer->size);
}

/*
 * Coalesce memory.
 *
 * Merges the adjacent memory blocks at first_mem and next_me
 */
void coalesce( struct mem_header_t* first_mem, struct mem_header_t* next_mem ) 
{
  /* Remove next_mem from free list */
  if (next_mem->prev != NULL) {
    next_mem->prev->next = next_mem->next;
  } else if (next_mem == mem_head) {
    mem_head = next_mem->next;
  } 
  if (next_mem->next != NULL) {
    next_mem->next->prev = next_mem->prev;
  } 

  /* update first_mem size and sanity check */
  first_mem->size =  first_mem->size + next_mem->size + sizeof(struct mem_header_t);
  first_mem->sanity_check = fmem_sanity_hash(first_mem, first_mem->size);

  /* update free mem footer */
  put_free_mem_footer(first_mem);
}

