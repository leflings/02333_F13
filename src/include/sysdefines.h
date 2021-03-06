/*! \file sysdefines.h
 *  This file defines constants used by system calls.
 *
 */

#ifndef _SYSDEFINES_H_
#define _SYSDEFINES_H_

/* Constant declarations. */

/*! Return code when system call returns normally. */
#define ALL_OK                  (0)
/*! Return code when system call returns with an error. */
#define ERROR                   (-1)
/*! Return code when system call is unknown. */
#define ERROR_ILLEGAL_SYSCALL   (-2)
/*! Return code when a message is too long. */
#define ERROR_MESSAGE_TOO_LONG  (-3)
/*! Max number of columns in VGA buffer. */
#define MAX_COLS                (80)
/*! Max number of columns in VGA buffer. */
#define MAX_ROWS                (25)

/*! System call that returns the version
 *  of the kernel. */
#define SYSCALL_VERSION         (0)
/*! System call that prints a string. */
#define SYSCALL_PRINTS          (1)
/*! System call that prints a hexadecimal
 *  value. */
#define SYSCALL_PRINTHEX        (2)
/*! System call that pauses execution
 *  by invoking the bochs debugger. */
#define SYSCALL_DEBUGGER        (3)

/*! System call that terminates the currently running
 *  thread. It takes no parameters. Terminates the process
 *  when there are no threads left. */
#define SYSCALL_TERMINATE       (4)

/*! System call that creates a new process with one single
 *  thread. It takes an index into the executable table in
 *  rdi. The program used is the executable whose index is
 *  passed in rdi. */
#define SYSCALL_CREATEPROCESS   (5)

/*! System call that blocks the calling thread a number of clocks ticks. The
   number of clock ticks is passed in rdi. There are 200 clock
   ticks per second.*/
#define SYSCALL_PAUSE           (6)

/*! System call that returns the current system time. The system time is the 
    number of clock ticks since system start. There are 200 clock ticks per 
    second. */
#define SYSCALL_TIME            (7)

/*! Allocates a memory block. The length of the requested memory block.
   is passed in rdi. Allocation flags for the block is passed in rsi.
   The system call returns the address or an error code. */
#define SYSCALL_ALLOCATE        (8)

/*! The allocated block is to be readonly. */
#define ALLOCATE_FLAG_READONLY  (1)

/*! The allocated block can contain executable instructions. */
#define ALLOCATE_FLAG_EX        (2)


/*! Frees a memory block allocated through the allocate system call. The
   address of the memory block is passed in rdi. The system call returns 
   ALL_OK if successful or an error code if unsuccessful. */
#define SYSCALL_FREE            (9)

/*! System call that allocates a port. The identity number of the port is 
    passed in rdi. The system call returns a handle to the port or an error 
    code if unsuccessful. 
 */
#define SYSCALL_ALLOCATEPORT    (10)

/*! System call that finds a port. The identity number of the port is passed 
    in rdi. The owning process identity is passed in rsi. The system call 
    returns a handle to the port or an error code if unsuccessful. 
 */
#define SYSCALL_FINDPORT        (11)

/*! System call that sends a message. The handle of the destination port is 
    passed in rdi. The message type is passed in rsi. A pointer to the 
    message is passed in rbx. The system call returns ALL_OK if successful 
    or an error code if unsuccessful. 
   
    There are three defined message types: short messages, long messages and 
    notifications. Short messages are passed in registers. Notifications are 
    single bit messages that will never block. Long messages are partially 
    located in memory.

    NOTE: You only need to implement short messages functionality in task 5B.
    Notification and long messages functionality is implemented in task 5A. 
    The functionality to be implemented in task 5A has a slightly different
    interface. This interface is described in the instructions.
   */
#define SYSCALL_SEND            (12)
#define SYSCALL_MSG_SHORT        (0)
#define SYSCALL_MSG_LONG         (1)
#define SYSCALL_MSG_NOTIFICATION (2)

/*! System call that receives a message. The handle of the port on which to 
    receive is passed in rdi. 1 is passed in register rsi if long messages 
    are permitted. Register rbx holds a pointer to a buffer that will hold
    the received message.

    The system call returns ALL_OK if successful or an error code if 
    unsuccessful. Register rsi holds the received message type. 
    Register rdi holds the identity of process from which the message was 
    received.

    NOTE: You only need to implement short messages functionality in task 5B.
    Notification and long messages functionality is implemented in task 5A. 
    The functionality to be implemented in task 5A has a slightly different
    interface. This interface is described in the instructions.
   */
#define SYSCALL_RECEIVE         (14)

/*! Returns the process identity of the calling thread in rax. */
#define SYSCALL_GETPID          (15)

/*! Creates a new thread in the process of the calling thread. The value of 
    rip register of the new thread is passed in the rdi registe. The value of 
    the rsp register of the new thread is passed in rsi. rflags of the new 
    thread is set to 0x200. The other registers are undefinied.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_CREATETHREAD    (16)

/*! Creates a new semaphore and returns if successful the handle of the
    semaphore in the rax register. The initial value of the semaphore count
    value is passed in the rdi register.
   
    If unsuccessful the system call returns an error code in rax. */
#define SYSCALL_CREATESEMAPHORE (17)

/*! Performs a semaphore down operation. The handle of the semaphore is passed
    in the rdi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_SEMAPHOREDOWN   (18)

/*! Performs an semaphore up operation. The handle of the semaphore is passed
    in the rdi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_SEMAPHOREUP     (19)

/*! Creates a new mutex and returns if successful the handle of the mutex in 
    the rax register. If unsuccessful the system call returns  an error code 
    in rax. */
#define SYSCALL_CREATEMUTEX     (20)

/*! Creates a new condition variable and returns if successful the handle of 
    the condition variable in the rax register. If unsuccessful the system 
    call returns  an error code in rax. */
#define SYSCALL_CREATECONDITIONVARIABLE (21)

/*! Performs a mutex lock operation. The handle of the mutex is passed
    in the rdi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_MUTEXLOCK       (22)

/*! Performs a mutex unlock operation. The handle of the mutex is passed
    in the rdi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_MUTEXUNLOCK     (23)

/*! Performs a wait operation on a condition variable. The handle of the 
    condition variable is passed in the rdi register. The handle of the 
    associated mutex is passed in the rsi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_CONDITIONVARIABLEWAIT (24)

/*! Performs a signal operation on a condition variable. The handle of the 
    condition variable is passed in the rdi register.

    The system call returns in rax ALL_OK if successful or an error code if 
    unsuccessful.
   */
#define SYSCALL_CONDITIONVARIABLESIGNAL (25)

/*! Returns the next scan code from the keyboard in the rax register. The
    system call blocks until data is available.

    The system call returns a keyboard scan code in rax if successful or an 
    error code if unsuccessful.
 */
#define SYSCALL_GETSCANCODE     (27)


/* Type declarations. */

/*! Describes a message. */
struct message
{
 unsigned long quad_0;
 unsigned long quad_1;
 unsigned long quad_2;
 unsigned long quad_3;
 unsigned long quad_4;
 unsigned long quad_5;
 unsigned long quad_6;
 unsigned long quad_7;
};

#endif
