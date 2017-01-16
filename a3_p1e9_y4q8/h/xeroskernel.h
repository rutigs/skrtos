/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

#include <stdarg.h>

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
#define BLOCKERR     -5         /* non-blocking op would block  */
#define BLOCK        -6         /* Block process */

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

/* Flags for setting Eflags  */
#define STARTING_EFLAGS         0x00003000
#define ARM_INTERRUPTS          0x00000200

/* Some constants involved with process creation and managment */

#define MAX_SIGNALS     32
   /* Maximum number of processes */
#define MAX_PROC        64
   /* Kernel trap number          */
#define KERNEL_INT      80
   /* Interrupt number for the timer */
#define TIMER_INT      (TIMER_IRQ + 32)
   /* IRQ of the keyboard */
#define KEYBOARD_IRQ    1
   /* Interrupt number for the keyboard */
#define KEYBOARD_INT      (KEYBOARD_IRQ + 32)
   /* Minimum size of a stack when a process is created */
#define PROC_STACK      (4096 * 4)
   /* Number of milliseconds in a tick */
#define MILLISECONDS_TICK 10

/* Constants to track states that a process is in */
#define STATE_STOPPED   0
#define STATE_READY     1
#define STATE_SLEEP     22
#define STATE_RUNNING   23
#define STATE_WAIT      24
#define STATE_READ      25

/* System call identifiers */
#define SYS_STOP        10
#define SYS_YIELD       11
#define SYS_CREATE      22
#define SYS_TIMER       33
#define SYS_GETPID      144
#define SYS_PUTS        155
#define SYS_SLEEP       166
#define SYS_KILL        176
#define SYS_KILL_PROC   177
#define SYS_CPUTIMES    178
#define SYS_SIGHANDLER  179
#define SYS_SIGRETURN   180
#define SYS_WAIT        181
#define SYS_OPEN        182
#define SYS_CLOSE       183
#define SYS_WRITE       184
#define SYS_READ        185
#define SYS_IOCTL       186
#define SYS_KEYBD       187

/* Device stuff */
#define MAX_PROC_DEVICES 4
#define MAX_KERN_DEVICES 2
#define NULL_DEVICE     -1

typedef struct struct_pcb pcb;
typedef struct struct_devsw devsw;

struct struct_devsw {
  char        *dev_name;
  int          dev_num;
  int        (*dev_open)( pcb *proc, devsw * dev );
  int        (*dev_close)( pcb *proc,  devsw * dev );
  int        (*dev_read)( pcb *proc, devsw * dev, void *buf, int buflen );
  int        (*dev_write)( pcb *proc, devsw * dev, void *buf, int buflen );
  int        (*dev_ioctl)( pcb *proc, devsw * dev, unsigned long command, va_list varargs );
  void        *dev_state;
};

/* Structure to track the information associated with a single process */
struct struct_pcb {
  void        *esp;    /* Pointer to top of saved stack           */
  pcb         *next;   /* Next process in the list, if applicable */
  pcb         *prev;   /* Previous proccess in list, if applicable*/
  int          state;  /* State the process is in, see above      */
  unsigned int pid;    /* The process's ID                        */
  int          ret;    /* Return value of system call             */
                       /* if process interrupted because of system*/
                       /* call                                    */
  long         args;
  void        *buffer;                    /* Buffer for syscalls              */
  int          bufferlen;                 /* Length of buffer                 */
  int          sleepdiff;
  long         cpuTime;                   /* CPU time  consumed               */
  void        *sig_handlers[MAX_SIGNALS]; /* Table containing signal handlers */
  unsigned int signals;                   /* A bit flag for the 32 signals    */
  int          processing;                /* A flag to indicate currently processing a signal */
  pcb         *waiting_proc;              /* The process this is waiting on   */
  pcb         *wait_head;                 /* Head of waiting queue            */
  pcb         *wait_tail;                 /* Tail of waiting queue            */
  devsw       *fd_tab[MAX_PROC_DEVICES];  /* Device fd table for process      */
};

typedef struct struct_ps processStatuses;
struct struct_ps {
  int  pid[MAX_PROC];      // The process ID
  int  status[MAX_PROC];   // The process status
  long  cpuTime[MAX_PROC]; // CPU time used in milliseconds
};

/* Kernel device table */
devsw dev_tab[MAX_KERN_DEVICES];

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

void     kfree(void *ptr);
void     kmeminit( void );
void     *kmalloc( size_t );


/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);


/* Internal functions for the kernel, applications must never  */
/* call these.                                                 */
void     dispatch( void );
void     dispatchinit( void );
void     ready( pcb *p );
pcb     *next( void );
void     enqueue(pcb **head, pcb **tail, pcb *node);
pcb     *dequeue(pcb **head, pcb **tail);
void     remove(pcb **head, pcb **tail, pcb *node);
void     contextinit( void );
int      contextswitch( pcb *p );
int      create( funcptr fp, size_t stack );
void     set_evec(unsigned int xnum, unsigned long handler);
void     printCF (void * stack);  /* print the call frame */
int      syscall(int call, ...);  /* Used in the system call stub */
void     sleep(pcb *, unsigned int);
int      removeFromSleep(pcb * p);
void     stop(pcb * p);
void     tick( void );
int      getCPUtimes(pcb * p, processStatuses *ps);
pcb     *findPCB( int pid );
void     keyboard_int_handler( void );

/* Function prototypes for system calls as called by the application */
int          syscreate( funcptr fp, size_t stack );
void         sysyield( void );
void         sysstop( void );
unsigned int sysgetpid( void );
unsigned int syssleep(unsigned int);
void         sysputs(char *str);
int          syskill(int pid, int signalNumber);
int          syskillproc(int pid);
int          sysgetcputimes(processStatuses *ps);
int          syssighandler(int signal, void (*newHandler)(void *), void (** oldHandler)(void *));
void         syssigreturn(void *old_sp);
int          syswait(int PID);
int          sysopen(int device_no);
int          sysclose(int fd);
int          syswrite(int fd, void *buf, int buflen);
int          sysread(int fd, void *buf, int buflen);
int          sysioctl(int fd, unsigned long command, ...);

/* signal.c functions */
int          signal(int pid, int sig_no);
int          sighandler(pcb *proc, int signal, void (*newHandler)(void *), void (** oldHandler)(void *));
void         sigtramp(void (* handler)(void *), void *cntx);
void         sigreturn(pcb *proc, void *old_sp);
void         setup_sigtramp(pcb *proc);

/* The initial process that the system creates and schedules */
void         root( void );

void         set_evec(unsigned int xnum, unsigned long handler);

/* Init program */
void         init( void);

/* command programs */
void         shell( void );
void         alarm( void );
void         alarm_handler( void *cntx );
void         t( void );


/* di_calls */
Bool         di_open(pcb *proc, int device_no);
Bool         di_close(pcb *proc, int fd);
Bool         di_read(pcb *proc, int fd, void *buf, int buflen);
Bool         di_write(pcb *proc, int fd, void *buf, int buflen);
Bool         di_ioctl(pcb *proc, int fd, unsigned long command, va_list varargs);
void         devices_init( void );


/* Tests */
void         test_syssighandler( void );
void         test_syskill( void );
void         test_signal_priority( void );
void         test_syswait( void );
void         run_signal_tests( void );
void         run_device_tests( void );


void           set_evec(unsigned int xnum, unsigned long handler);

/* keyboard functions */
void         keyboards_init( void );
void         unblock_proc( void );


/* Anything you add must be between the #define and this comment */
#endif

