/*
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
 *
 * - Bool test_mem_manager();
 *      Entry point to testing function test_mem_manager
 *
 *      Returns:
 *      -> False if any of the tests fail
 *      -> True  iff all of the tests pass
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
 * Returns pointer to memory block of at least requested size.
 * Returns NULL on error 
 */
void *kmalloc( int size )
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
 */
void kfree( void *ptr )
{
  struct mem_header_t *curr_mem = ptr - sizeof(struct mem_header_t);

  if (curr_mem->sanity_check != amem_sanity_hash(curr_mem, curr_mem->size)) {
    kprintf("Error in kfree: sanity check failed for memory at: %x", ptr);
    return;
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

/* ========================================================================= */
/*                               Internal Tests                              */
/* ========================================================================= */

/* Test methods declaration */
Bool test_kmeminit( void );
Bool test_kmalloc( void );
Bool test_kfree( void );
Bool test_free_mem_footer( struct mem_header_t* free_mem_header );

/*
 * All the memory manager tests.
 *
 * Return true if all tests pass.
 */
Bool test_mem_manager() {
  kprintf("\nTesting: Running mem.c tests. \n");

  Bool result = TRUE;

  result &= test_kmeminit();
  result &= test_kmalloc();
  result &= test_kfree();

  if (result) {
    kprintf("\nTesting: mem.c tests passed. \n");
  }

  return result;
} 

/*
 * Test kmeminit.
 *
 * Return true if tests pass.
 */
Bool test_kmeminit( void ){
  kmeminit();

  if (((unsigned long) mem_head) % 16) {
    kprintf("kmeminit test failed at mem.c: mem_head not 16 byte aligned. ");
    kprintf("mem_head is: %x", mem_head);
    return FALSE;
  }

  if (mem_head > ((long) HOLESTART)) {
    kprintf("kmeminit test failed at mem.c: mem_head larger than HOLESTART. ");
    kprintf("mem_head is: %x", mem_head);
    return FALSE;
  }

  if (mem_head->size > ((long) HOLESTART) - freemem - 16 ) { /* 16 is sizeof(struct mem_header_t) */
    kprintf("kmeminit test failed at mem.c: mem_head->size too large. ");
    kprintf("mem_head->size is: %x", mem_head->size);
    return FALSE;
  } else if (mem_head->size < ((long) HOLESTART) - freemem - 16 - 15) { /* at most waste 15 bytes for 16 byte alignment */
    kprintf("kmeminit test failed at mem.c: mem_head->size too small. ");
    kprintf("mem_head->size is: %x", mem_head->size);
    return FALSE;
  } 

  if (mem_head->sanity_check != fmem_sanity_hash(mem_head, mem_head->size)) {
    kprintf("kmeminit test failed at mem.c: mem_head->sanity_check error. ");
    kprintf("Expected: %x, Actual: %x", fmem_sanity_hash(mem_head, mem_head->size), mem_head->sanity_check);
    return FALSE;
  }

  /* test free_mem_footer for mem_head*/
  if (!test_free_mem_footer(mem_head)){
    return FALSE;
  }

  struct mem_header_t *mem_slot = mem_head->next;

  if (((unsigned long) mem_slot) % 16) {
    kprintf("kmeminit test failed at mem.c: mem_slot not 16 byte aligned. ");
    kprintf("mem_slot is: %x", mem_slot);
    return FALSE;
  }

  if (mem_slot != ((unsigned long) HOLEEND)) {
    kprintf("kmeminit test failed at mem.c: mem_slot not equal HOLEEND. ");
    kprintf("mem_slot is: %x, HOLEEND is: %x", mem_slot, HOLEEND);
    return FALSE;
  }

  if (mem_slot->size != (((long) maxaddr) - ((long) HOLEEND) - 16)) {  /* 16 is sizeof(struct mem_header_t) */
    kprintf("kmeminit test failed at mem.c: mem_slot->size incorrect. ");
    kprintf("mem_slot->size is: %x", mem_slot->size);
    return FALSE;
  }

  if (mem_slot->sanity_check != fmem_sanity_hash(mem_slot, mem_slot->size)) {
    kprintf("kmeminit test failed at mem.c: mem_slot->sanity_check error. ");
    kprintf("Expected: %x, Actual: %x", fmem_sanity_hash(mem_slot, mem_slot->size), mem_slot->sanity_check);
    return FALSE;
  }

  /* test free_mem_footer for mem_head*/
  if (!test_free_mem_footer(mem_head)){
    return FALSE;
  }

  if (mem_head->prev != NULL) {
    kprintf("kmeminit test failed at mem.c: mem_head->prev not NULL ");
    kprintf("mem_head->prev is: %x", mem_head->prev);
    return FALSE;
  }

  if (mem_slot->next != NULL) {
    kprintf("kmeminit test failed at mem.c: mem_slot->next not NULL ");
    kprintf("mem_slot->next is: %x", mem_slot->next);
    return FALSE;
  }

  if (mem_slot->prev != mem_head) {
    kprintf("kmeminit test failed at mem.c: mem_slot->prev does not point to mem_head ");
    kprintf("mem_slot->prev is: %x", mem_slot->prev);
    return FALSE;
  }

  /* test free_mem_footer for mem_head*/
  if (!test_free_mem_footer(mem_slot)){
    return FALSE;
  }

  return TRUE;
}

/*
 * Test kmalloc.
 *
 * Return true if tests pass.
 */
Bool test_kmalloc( void ){
  kmeminit();

  if (kmalloc(0) != NULL) {
    kprintf("kmalloc test failed at mem.c: Invalid Size not thrown on request size of 0");
    return FALSE;
  }

  if (kmalloc(-1) != NULL) {
    kprintf("kmalloc test failed at mem.c: Invalid Size not thrown on negative request size");
    return FALSE;
  }

  if (kmalloc(1000000000) != NULL) {
    kprintf("kmalloc test failed at mem.c: Out of memory error not thrown.");
    return FALSE;
  }

  /* Test kmalloc on aligned memory */
  unsigned long test_base = kmalloc(16);
  if (test_base != (void *) mem_head - 16) {
    kprintf("kmalloc test failed at mem.c: test_base address not as expected. ");
    kprintf("Expected: %x, Actual: %x", (void *) mem_head - 16, test_base);
    return FALSE;
  }
  struct mem_header_t *test_base_header = test_base - sizeof(struct mem_header_t);
  if (test_base_header->sanity_check != amem_sanity_hash(test_base_header, 16)) {
    kprintf("kmalloc test failed at mem.c: test_base sanity_check not as expected.");
    return FALSE;
  }

  /* Test kmalloc on non-aligned memory */
  unsigned long test_nonaligned = kmalloc(17);  
  if (test_nonaligned != (void *) mem_head - 32) {
    kprintf("kmalloc test failed at mem.c: test_nonaligned address not as expected. ");
    kprintf("Expected: %x, Actual: %x", (void *) mem_head - 32, test_nonaligned);
    return FALSE;
  }
  struct mem_header_t *test_nonaligned_header = test_nonaligned - sizeof(struct mem_header_t);
  if (test_nonaligned_header->sanity_check != amem_sanity_hash(test_nonaligned_header, 32)) {
    kprintf("kmalloc test failed at mem.c: test_nonaligned sanity_check not as expected.");
    return FALSE;
  }

  /* Test kmalloc with request larger than first free block. */  
  unsigned long test_big_size = kmalloc(624361);  
  if (test_big_size != (void *) HOLEEND + 16) {
    kprintf("kmalloc test failed at mem.c: test_big_size address not as expected. ");
    kprintf("Expected: %x, Actual: %x", HOLEEND + 16, test_big_size);
    return FALSE;
  }
  struct mem_header_t *test_big_header = test_big_size - sizeof(struct mem_header_t);
  if (test_big_header->sanity_check != amem_sanity_hash(test_big_header, 624368)) {
    kprintf("kmalloc test failed at mem.c: test_big_size sanity_check not as expected.");
    return FALSE;
  }

  /* Test kmalloc on memory before hole. */
  unsigned long test_before = kmalloc(456);  
  if (test_before != (void *) mem_head - 464) {
    kprintf("kmalloc test failed at mem.c: test_before address not as expected. ");
    kprintf("Expected: %x, Actual: %x", (void *) mem_head - 464, test_before);
    return FALSE;
  }
  struct mem_header_t *test_before_header = test_before - sizeof(struct mem_header_t);
  if (test_before_header->sanity_check != amem_sanity_hash(test_before_header, 464)) {
    kprintf("kmalloc test failed at mem.c: test_before sanity_check not as expected.");
    return FALSE;
  }

  if (mem_head->next->next != NULL) {
    kprintf("kmalloc test failed at mem.c: free list structure error.");
    return FALSE;
  }

  return TRUE;
}


/*
 * Test kfree.
 *
 * Return true if tests pass.
 */
Bool test_kfree ( void ) {
  kmeminit();
  unsigned long original_mem_head_sanity_check = mem_head->sanity_check;
  unsigned long before_hole_size = mem_head->size + sizeof(struct mem_header_t);

  /* Test 1: Test free with A(16)FHF (Allocated(size 16), Free block, Hole, Free block) Coalesce behind */
  unsigned long test_1 = kmalloc(16); 
  kfree(test_1); /* Back to original state */
  if (mem_head->sanity_check != original_mem_head_sanity_check) {
    kprintf("kfree test failed at mem.c: Test1: free memory not back to original state.");
    return FALSE;
  }
  /* test free_mem_footer inserted correctly */
  if (!test_free_mem_footer(mem_head)){
    return FALSE;
  }
  if (mem_head->next == NULL) {
    kprintf("kfree test failed at mem.c: Test 1: free list-broken.");
    return FALSE;
  }
  if (mem_head->next->next != NULL) {
    kprintf("kfree test failed at mem.c: Test 1: free list too long");
    return FALSE;
  }


  /* Test2: Test kfree with Memory layout: A(16)A(32)A(48)FHF */
  test_1 = kmalloc(14); 
  unsigned long test_2 = kmalloc(32); 
  unsigned long test_3 = kmalloc(45); 


  /* Test free middle block. No coalescing */
  kfree(test_2);
  if (mem_head != (void *) test_2 - 16) {
    kprintf("kfree test failed at mem.c: Test 2 free non-coalesce case free block not added back to list.");
    return FALSE;
  }
  if (mem_head->next->next->next != NULL) {
    kprintf("kfree test failed at mem.c: Test 2 free list size wrong: Expected 3.");
    return FALSE;
  }


  /* Test 3: Test kfree with Memory layout: A(16)FA(48)FHF. Coalesce on both sides */
  kfree(test_3);
  if (mem_head->sanity_check != fmem_sanity_hash(mem_head, before_hole_size - 48)) {
    kprintf("kfree test failed at mem.c: Test 3 free memory before hole size wrong.");
    kprintf("Expected: %lu, Actual: %lu", before_hole_size - 48, mem_head->size);
    return FALSE;
  }
  /* test free_mem_footer inserted correctly */
  if (!test_free_mem_footer(mem_head)){
    return FALSE;
  }
  if (mem_head->next == NULL) {
    kprintf("kfree test failed at mem.c: Test 3 free list-broken.");
    return FALSE;
  }  
  if (mem_head->next->next != NULL) {
    kprintf("kfree test failed at mem.c: Test 3: free list too long");
    kprintf("mem_head: %x, mem_head->next: %x, mem_head->next->next: %x", mem_head, mem_head->next, mem_head->next->next);
    return FALSE;
  }


  /* Test 4: Test kfree with Memory layout: A(16)FHF */
  kfree(test_1);
  if (mem_head->sanity_check != fmem_sanity_hash(mem_head, before_hole_size - sizeof(struct mem_header_t))) {
    kprintf("kfree test failed at mem.c: Test 4 free memory before hole size wrong.");
    kprintf("Expected: %lu, Actual: %lu", before_hole_size - sizeof(struct mem_header_t), mem_head->size);
    return FALSE;
  }
  if (mem_head->sanity_check != original_mem_head_sanity_check) {
    kprintf("kfree test failed at mem.c: Test 4 free memory not back to original state.");
    return FALSE;
  }
  /* test free_mem_footer inserted correctly */
  if (!test_free_mem_footer(mem_head)){
    return FALSE;
  }
  if (mem_head->next == NULL) {
    kprintf("kfree test failed at mem.c: Test 4 free list-broken.");
    return FALSE;
  }

  return TRUE;
}


/*
 * Test free_mem_footer has correct fields.
 *
 * Return true if tests pass.
 */
Bool test_free_mem_footer(struct mem_header_t* free_mem_header ) {
  struct free_mem_footer_t *free_mem_footer = (void *) free_mem_header + free_mem_header->size;

  if (free_mem_footer->sanity_check != fmem_sanity_hash(free_mem_footer, free_mem_footer->size)){
    kprintf("free_mem_footer test failed at mem.c: sanity check failed. ");
    kprintf("free_mem_header: %x, Expected sanity_check is: %x, Actual sanity_check is %x.",
     free_mem_header, fmem_sanity_hash(free_mem_footer, free_mem_footer->size), free_mem_footer->sanity_check);
    return FALSE;
  }

  if (free_mem_footer->size != free_mem_header->size){
    kprintf("free_mem_footer test failed at mem.c: size mismatch. ");
    kprintf("free_mem_header: %x, Expected size is: %x, Actual size is %x.",
     free_mem_header, free_mem_header->size, free_mem_footer->size);
    return FALSE;
  }

  return TRUE;
}





