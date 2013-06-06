/*! \file syscall.c 

 This files holds the implementations of all system calls.

 */

#include "kernel.h"
#include "sync.h"

int
system_call_implementation(void)
{
 register int schedule=0;
 /*!< System calls may set this variable to 1. The variable is used as
      input to the scheduler to indicate if scheduling is necessary. */
 switch(SYSCALL_ARGUMENTS.rax)
 {
  case SYSCALL_PRINTS:
  {
   kprints((char*) (SYSCALL_ARGUMENTS.rdi));
   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_PRINTHEX:
  {
   kprinthex(SYSCALL_ARGUMENTS.rdi);
   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_DEBUGGER:
  {
   /* Enable the bochs iodevice and force a return to the debugger. */
   outw(0x8a00, 0x8a00);
   outw(0x8a00, 0x8ae0);

   SYSCALL_ARGUMENTS.rax = ALL_OK;
   break;
  }

  case SYSCALL_CREATEPROCESS:
  {
    int process_number;
    int thread_number = -1;
    struct prepare_process_return_value prepare_process_ret_val;

    long int executable_number = SYSCALL_ARGUMENTS.rdi;

    /* Search process_table for a slot */
    grab_lock_rw(&process_table_lock);
    for(process_number = 0;; process_number++)
    {
      if(process_number < MAX_NUMBER_OF_PROCESSES
                      && process_table[process_number].threads > 0)
      {
        continue;
      }
      break;
    }

    if(process_number == MAX_NUMBER_OF_PROCESSES)
    {
      SYSCALL_ARGUMENTS.rax = ERROR;
      kprints("Max number of processes reached\n");
    }
    else
    {
      prepare_process_ret_val = prepare_process(
                      executable_table[executable_number].elf_image,
                      process_number,
                      executable_table[executable_number].memory_footprint_size);

      if(0 == prepare_process_ret_val.first_instruction_address)
      {
              kprints("Error starting image\n");
      }

      grab_lock_rw(&thread_table_lock);


      /* Allocate default port 0 */
      if(-1 == allocate_port(0,process_number)) {
              SYSCALL_ARGUMENTS.rax = ERROR;
              kprints("Ran out of ports or port is already allocated\n");
              break;
      }

      // FIXME: Add sanity check
      thread_number = allocate_thread();

      if(thread_number == -1) {
        kprints("Error allocating thread\n");
        SYSCALL_ARGUMENTS.rax = ERROR;
      } else {
        process_table[process_number].parent = thread_table[get_current_thread()].data.owner;
        process_table[process_number].threads = 1;
        thread_table[thread_number].data.owner = process_number;


        SYSCALL_ARGUMENTS.rax = ALL_OK;
      }

      release_lock(&thread_table_lock);

    }
    release_lock(&process_table_lock);
    if(thread_number != -1) {

      process_table[process_number].page_table_root = prepare_process_ret_val.page_table_address;
        thread_table[thread_number].data.registers.integer_registers.rflags = 0x200;
        thread_table[thread_number].data.registers.integer_registers.rip =
                        prepare_process_ret_val.first_instruction_address;

      grab_lock_rw(&ready_queue_lock);
      thread_queue_enqueue(&ready_queue, thread_number);
      release_lock(&ready_queue_lock);
    }
    break;
  }

  case SYSCALL_TERMINATE:
  {
    grab_lock_rw(&process_table_lock);
    grab_lock_rw(&thread_table_lock);
    int i;
    int owner_process = thread_table[get_current_thread()].data.owner;
    int parent_process = process_table[owner_process].parent;
    int tmp_thread;

    thread_table[get_current_thread()].data.owner = -1;
    if(--process_table[owner_process].threads < 1)
    {
      cleanup_process(owner_process);
    }

    /* Cleanup associated ports */
    for(i = 0; i < MAX_NUMBER_OF_PORTS; i++) {
        if(port_table[i].owner == owner_process) {
          /* since owner cannot change, we grab the port lock, instead
           * of port talbe lock
           */
            grab_lock_rw(&port_table[i].lock);
            port_table[i].owner = -1;
            /* If it has waiting threads, release them, set rax to ERROR
             * and put them back in the ready queue
             */
            grab_lock_rw(&ready_queue_lock);
            while(!thread_queue_is_empty(&port_table[i].sender_queue)) {
                tmp_thread = thread_queue_dequeue(&port_table[i].sender_queue);
                thread_table[tmp_thread].data.registers.integer_registers.rax = ERROR;
                thread_queue_enqueue(&ready_queue,tmp_thread);
            }
            release_lock(&ready_queue_lock);
            release_lock(&port_table[i].lock);
        }
    }


    release_lock(&thread_table_lock);
    release_lock(&process_table_lock);
    schedule = 1;

    break;

  }

  case SYSCALL_SEND:
    {
      short port = SYSCALL_ARGUMENTS.rdi;
      struct message *msg_ptr = (struct message *) SYSCALL_ARGUMENTS.rbx;
      int rcv_thread;
      if(SYSCALL_ARGUMENTS.rsi != SYSCALL_MSG_SHORT) {
        SYSCALL_ARGUMENTS.rax = ERROR;
        break;
      }

      grab_lock_rw(&port_table[port].lock);
      rcv_thread = port_table[port].receiver;

      /* Check if receiving thread is ready to receive */
      if(rcv_thread == -1) {
        /* If no thread is ready to receive, we queue sending thread on the port
         * belonging to the receiving thread */
        thread_queue_enqueue(&port_table[port].sender_queue, get_current_thread());

        /* We also save the message pointer on it's execution context, so that
         * the receiving thread can pick it up later */
        thread_table[get_current_thread()].data.registers.integer_registers.rbx = msg_ptr;

        /* Sending thread is now blocked, so we must schedule */
        schedule = 1;
      } else {
        /* Receiving thread is already in a blocked state, waiting for a message */


        /* We copy the message from sender to receiver */
        *(struct message *)thread_table[rcv_thread].data.registers.integer_registers.rbx = *msg_ptr;

        /* Set the appropriate registers */
        thread_table[rcv_thread].data.registers.integer_registers.rax = ALL_OK;
        thread_table[rcv_thread].data.registers.integer_registers.rsi = SYSCALL_ARGUMENTS.rsi;

        /* Receiving thread is no longer blocked, so we put it in the ready queue */
        grab_lock_rw(&ready_queue_lock);
        thread_queue_enqueue(&ready_queue, rcv_thread);
        release_lock(&ready_queue_lock);

        /* Update port to indicate to waiting receiver */
        port_table[port].receiver = -1;
        SYSCALL_ARGUMENTS.rax = ALL_OK;
      }
      release_lock(&port_table[port].lock);

      break;
    }

    case SYSCALL_RECEIVE:
    {
      short port = SYSCALL_ARGUMENTS.rdi;
      struct message *msg_ptr = (struct message *) SYSCALL_ARGUMENTS.rbx;
      int send_thread;
      if(SYSCALL_ARGUMENTS.rsi != SYSCALL_MSG_SHORT) {
        SYSCALL_ARGUMENTS.rax = ERROR;
        break;
      }

      /* This is pretty much the same as for the send system call, just sort of
       * reversed
       */
      grab_lock_rw(&port_table[port].lock);
      if(thread_queue_is_empty(&port_table[port].sender_queue)) {
        port_table[port].receiver = get_current_thread();
        thread_table[get_current_thread()].data.registers.integer_registers.rbx = msg_ptr;
        schedule = 1;
      } else {
        send_thread = thread_queue_dequeue(&port_table[port].sender_queue);

        thread_table[send_thread].data.registers.integer_registers.rax = ALL_OK;
        *msg_ptr = *(struct message *) thread_table[send_thread].data.registers.integer_registers.rbx;

        SYSCALL_ARGUMENTS.rdi = thread_table[send_thread].data.owner;
        SYSCALL_ARGUMENTS.rax = ALL_OK;

        grab_lock_rw(&ready_queue_lock);
        thread_queue_enqueue(&ready_queue, send_thread);
        release_lock(&ready_queue_lock);
      }
      release_lock(&port_table[port].lock);
      break;
    }


  /* Do not touch any lines below or including this line. */
  default:
  {
   /* No system call defined. */
   SYSCALL_ARGUMENTS.rax = ERROR_ILLEGAL_SYSCALL;
  }
 }

 return schedule;
}
