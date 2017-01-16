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

#define CREATE_FAILURE -1  /* Process creation failed     */



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


/* Some constants involved with process creation and managment */

   /* Maximum number of processes */
#define MAX_PROC        64
   /* Kernel trap number          */
#define KERNEL_INT      80
   /* Timer Interrupt trap number          */
#define TIMER_INT       32
   /* Minimum size of a stack when a process is created */
#define PROC_STACK      (4096 * 4)

   /* Time Slice Length */
#define TIME_SLICE      100


/* Constants to track states that a process is in */
#define STATE_STOPPED       0
#define STATE_READY         1
#define STATE_BLOCKED_SEND  2
#define STATE_BLOCKED_RECV  3
#define STATE_BLOCKED_SLEEP 4


/* System call identifiers */
#define SYS_STOP        10
#define SYS_YIELD       11
#define SYS_CREATE      22
#define SYS_TIMER       33
#define SYS_GETPID      44
#define SYS_PUTS        55
#define SYS_KILL        66
#define SYS_SEND        77
#define SYS_RECV        88
#define SYS_SLEEP       89
#define TIMER_INT_SYS   99


/* Structure to track the information associated with a single process */

typedef struct struct_pcb pcb;
struct struct_pcb {
  unsigned long  *esp;       /* Pointer to top of saved stack                */
  pcb            *next;      /* Next process in the list, if applicable      */
  pcb            *prev;      /* Previous process in the list, if applicable  */
  pcb            *send_head; /* Head of queue of incoming sends to process   */
  pcb            *send_tail; /* Tail of queue of incoming sends to process   */
  pcb            *recv_head; /* Head of queue of incoming recvs to process   */
  pcb            *recv_tail; /* Tail of queue of incoming recvs to process   */
  int             state;     /* State the process is in, see above           */
  int             pid;       /* The process's ID                             */
  int             ret;       /* Return value of system call                  */
                             /* if process interrupted because of system     */
                             /* call                                         */
  long            args;      /* pointer to system call arguments             */
  void           *mem;       /* Pointer to allocated memory                  */
  unsigned long   buffer;    /* buffer for send's message or recv's from_pid */
  unsigned long   *mem_locn; /* location for the receiver to get the message */
  int             dest_pid; /* pointer to pid of process to send or recv    */
};


/* The actual space is set aside in create.c */
extern pcb     proctab[MAX_PROC];

#pragma pack(1)

/* What the set of pushed registers looks like on the stack */
typedef struct context_frame {
  unsigned long        edi;
  unsigned long        esi;
  unsigned long        ebp;
  unsigned long        esp;
  unsigned long        ebx;
  unsigned long        edx;
  unsigned long        ecx;
  unsigned long        eax;
  unsigned long        iret_eip;
  unsigned long        iret_cs;
  unsigned long        eflags;
  unsigned long        stackSlots[];
} context_frame;


/* Memory mangement system functions, it is OK for user level   */
/* processes to call these.                                     */

int      kfree(void *ptr);
void     kmeminit( void );
void     *kmalloc( size_t );


/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);


/* Internal functions for the kernel, applications must never  */
/* call these.                                                 */
void          dispatch( void );
void          dispatchinit( void );
void          ready( pcb *p );
pcb           *next( void );
void          contextinit( void );
int           contextswitch( pcb *p );
void          createinit( void );
int           create( funcptr fp, size_t stack );
void          set_evec(unsigned int xnum, unsigned long handler);
void          printCF (void * stack);  /* print the call frame */
int           syscall(int call, ...);  /* Used in the system call stub */
void          send( pcb *sending_pcb );
void          recv( pcb *receiving_pcb );
void          sleep( pcb *sleeping_pcb );
void          tick( void );
int           kill_process( int pid, int proc_pid );
int           cleanup( pcb *p );
void          notify_blocked_proc( pcb *p );
pcb           *get_pcb_by_id( int pid );
int           is_running( int pid );
void          enqueue(pcb **head, pcb **tail, pcb *node);
pcb           *dequeue(pcb **head, pcb **tail);
void          remove(pcb **head, pcb **tail, pcb *node);
void          insertAfter(pcb **head, pcb **tail, pcb *prev_node, pcb *node_to_add);


/* Function prototypes for system calls as called by the application */
int          syscreate( funcptr fp, size_t stack );
void         sysyield( void );
void         sysstop( void );
int          sysgetpid( void );
void         sysputs( char *str );
int          syskill( int pid );
int          syssend( int dest_pid, unsigned long num );
int          sysrecv( int *from_pid, unsigned long *num );
unsigned int syssleep( unsigned int milliseconds );

/* The initial process that the system creates and schedules */
void        root( void );

void        set_evec(unsigned int xnum, unsigned long handler);

void        idleproc( void );

/* Test functions */
void        test_sysgetpid_and_return( void );
void        test_sysgetpid_helper( void );
void        test_sysputs( void );
void        test_sysputs_helper( void );
void        test_syskill( void );
void        test_syskill_helper( void );
void        test_syssend( void );
void        test_sysrecv( void );
void        test_send_recv_kill( void );
void        test_syssend_helper( void );
void        test_sysrecv_helper( void );
void        test_sysrecv_any_helper( void );
void        test_child_killer_helper( void );
void        test_stop_helper( void );
void        test_syssleep( void );
void        test_syssleep_helper( void );

/* Anything you add must be between the #define and this comment */
#endif

