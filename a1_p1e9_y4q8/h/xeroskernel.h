/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout Xinu */

typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes 
                              * addressable in this architecture.
                              */
#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */


/* Universal return constants */ 

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);
void           set_evec(unsigned int xnum, unsigned long handler);



/* Constant Definitions */
#define       PCB_ARR_SIZE    32       /* Size of PCB Table */
#define       SYS_CREATE      135      /* Create Syscall ID */
#define       SYS_YIELD       136      /* Yield Syscall ID */
#define       SYS_STOP        137      /* Stop Syscall ID */

/* 
 * Macro helper to turn IDT table entry from int to string literal 
 * Used for making IDT table entry a "single" variable instead of the same constant in two places
 */
#define       STRINGIFY2(X)   #X
#define       STRINGIFY(X)    STRINGIFY2(X)

/* IDT entries */
#define       SYS_CALL_ID     49       /* IDT index for syscall ISR */


/* Memory manager methods */
extern void    kmeminit( void );
extern void    *kmalloc( int size );
extern void    kfree( void *ptr );
extern Bool    test_mem_manager ( void );

/* Process structures */
/* Process States */
typedef enum {RUNNING, READY, BLOCKED, STOPPED} p_state_t;

/* Process Control Block */
struct pcb_t {
    int pid;
    p_state_t state;
    unsigned long esp;
    void *memory;
    int result_code;
    struct pcb_t *next_pcb;
};

/* Dispatch */
extern void    dispatch_init( void );
extern void    dispatch( void );
extern void    ready( struct pcb_t *new_pcb );
extern Bool    test_dispatcher ( void );


/* Context Switcher */
extern void   context_init( void );
extern int    contextswitch( struct pcb_t *p );

/* Process Creation */
extern void create_init( void );
extern int  create( void (*func)(void), int stack );
extern Bool test_create_process( void );

/* Context frame for initializing process stack */
struct context_frame {
  unsigned long   edi;
  unsigned long   esi;
  unsigned long   ebp;
  unsigned long   esp;
  unsigned long   ebx;
  unsigned long   edx;
  unsigned long   ecx;
  unsigned long   eax;
  unsigned long   iret_eip;
  unsigned long   iret_cs;
  unsigned long   eflags;
};

/* System Calls */
extern int     syscall( int call, ... );
extern unsigned int syscreate( void (*func)(void), int stack );
extern void    sysyield( void );
extern void    sysstop( void );

/* User Process */
extern void root( void );

/* Testing functions */
extern void run_unit_tests( void );
extern void run_kernel_tests( void );

/* Anything you add must be between the #define and this comment */
#endif
