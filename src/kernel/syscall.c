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
	 *       input to the scheduler to indicate if scheduling is necessary. */
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

	case SYSCALL_VERSION:
	{
		SYSCALL_ARGUMENTS.rax = KERNEL_VERSION;
		break;
	}

	case SYSCALL_CREATEPROCESS:
	{
		int process_number, thread_number;
		long int executable_number = SYSCALL_ARGUMENTS.rdi;
		struct prepare_process_return_value prepare_process_ret_val;

		/* Search process_table for a slot */
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
			break;
		}

		prepare_process_ret_val = prepare_process(
				executable_table[executable_number].elf_image,
				process_number,
				executable_table[executable_number].memory_footprint_size);

		if(0 == prepare_process_ret_val.first_instruction_address)
		{
			kprints("Error starting image\n");
		}

		process_table[process_number].parent
		= thread_table[cpu_private_data.thread_index].data.owner;

		/* Allocate default port 0 */
		if(-1 == allocate_port(0,process_number)) {
			SYSCALL_ARGUMENTS.rax = ERROR;
			kprints("Ran out of ports or port is already allocated\n");
			break;
		}

		thread_number = allocate_thread();

		thread_table[thread_number].data.owner = process_number;
		thread_table[thread_number].data.registers.integer_registers.rflags = 0x200;
		thread_table[thread_number].data.registers.integer_registers.rip =
				prepare_process_ret_val.first_instruction_address;

		process_table[process_number].threads += 1;

		SYSCALL_ARGUMENTS.rax = ALL_OK;

		thread_queue_enqueue(&ready_queue, thread_number);
		break;
	}

	case SYSCALL_TERMINATE:
	{
		int i;
		int owner_process = thread_table[cpu_private_data.thread_index].data.owner;
		int parent_process = process_table[owner_process].parent;
		int tmp_thread;

		/* Terminate thread */
		if (process_table[owner_process].threads == 1){
			thread_table[cpu_private_data.thread_index].data.owner = -1;

		}

		/*Decrement thread count */
		process_table[owner_process].threads -= 1;

		if(process_table[owner_process].threads < 1)
		{
			cleanup_process(owner_process);
		}

		/* Cleanup associated ports */
		for(i = 0; i < MAX_NUMBER_OF_PORTS; i++) {
			if(port_table[i].owner == owner_process) {
				port_table[i].owner = -1;
				/* If it has waiting threads, release them, set rax to ERROR
				 * and put them back in the ready queue
				 */
				while(!thread_queue_is_empty(&port_table[i].sender_queue)) {
					tmp_thread = thread_queue_dequeue(&port_table[i].sender_queue);
					thread_table[tmp_thread].data.registers.integer_registers.rax = ERROR;
					thread_queue_enqueue(&ready_queue,tmp_thread);
				}
			}
		}

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
		rcv_thread = port_table[port].receiver;

		/* Check if receiving thread is ready to receive */
		if(rcv_thread == -1) {
			/* If no thread is ready to receive, we queue sending thread on the port
			 * belonging to the receiving thread */
			thread_queue_enqueue(&port_table[port].sender_queue,
					cpu_private_data.thread_index);

			/* We also save the message pointer on it's execution context, so that
			 * the receiving thread can pick it up later */
			thread_table[cpu_private_data.thread_index]
			             .data.registers.integer_registers.rbx = msg_ptr;

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
			thread_queue_enqueue(&ready_queue, rcv_thread);

			/* Update port to indicate to waiting receiver */
			port_table[port].receiver = -1;
			SYSCALL_ARGUMENTS.rax = ALL_OK;
		}

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
		if(thread_queue_is_empty(&port_table[port].sender_queue)) {
			port_table[port].receiver = cpu_private_data.thread_index;
			thread_table[cpu_private_data.thread_index]
			             .data.registers.integer_registers.rbx = msg_ptr;
			schedule = 1;
		} else {
			send_thread = thread_queue_dequeue(&port_table[port].sender_queue);

			thread_table[send_thread].data.registers.integer_registers.rax = ALL_OK;
			*msg_ptr = *(struct message *) thread_table[send_thread].data.registers.integer_registers.rbx;

			thread_queue_enqueue(&ready_queue, send_thread);

			SYSCALL_ARGUMENTS.rdi = thread_table[send_thread].data.owner;
			SYSCALL_ARGUMENTS.rax = ALL_OK;
		}
		break;
	}
	case SYSCALL_CREATETHREAD:
	{
		int thread_number;
		int process_number;
		thread_number = allocate_thread();
		if (thread_number == -1){
			SYSCALL_ARGUMENTS.rax = ERROR;
			kprints("Max number of threads reached\n");
			break;
		}
		process_number = thread_table[cpu_private_data.thread_index].data.owner;

		thread_table[thread_number].data.owner = process_number;
		thread_table[thread_number].data.registers.integer_registers.rflags = 0x200;
		thread_table[thread_number].data.registers.integer_registers.rip = SYSCALL_ARGUMENTS.rdi;
		thread_table[thread_number].data.registers.integer_registers.rsp = SYSCALL_ARGUMENTS.rsi;

		process_table[process_number].threads += 1;

		SYSCALL_ARGUMENTS.rax = ALL_OK;

		thread_queue_enqueue(&ready_queue, thread_number);
		break;
	}

	case SYSCALL_CREATESEMAPHORE:
	{
		int current_thread = cpu_private_data.thread_index;
		int current_thread_owner = thread_table[current_thread].data.owner;


		//Check rdi register
		if (SYSCALL_ARGUMENTS.rdi < 0){
			kprints("semaphore count can't be negative");
			SYSCALL_ARGUMENTS.rax = ERROR;
			break;
		}
		//Find a free entry in the semaphore table
		int i;
		for (i = 0;; i++) {
		  if(i == MAX_NUMBER_OF_SEMAPHORES) {
		    break;
		  }
		  else if (semaphore_table[i].owner < 0)
		  {
				semaphore_table[i].count = SYSCALL_ARGUMENTS.rdi;
				//Set the owner of the semaphore
				semaphore_table[i].owner = current_thread_owner;
				//Handle to semaphore
				SYSCALL_ARGUMENTS.rax = i;


				break;
			}

		}
		//No free entry in semaphore table
		if (i == MAX_NUMBER_OF_SEMAPHORES){
			kprints("No free entry in semaphore table");
			SYSCALL_ARGUMENTS.rax = ERROR;
		}
		break;
	}

	case SYSCALL_SEMAPHOREDOWN:
	{

		int current_thread = cpu_private_data.thread_index;
		int current_thread_owner = thread_table[current_thread].data.owner;
		struct semaphore* s = &semaphore_table[SYSCALL_ARGUMENTS.rdi];

		if (s->owner != current_thread_owner){
			kprints("ERROR! Process is not owner of semaphore");
			SYSCALL_ARGUMENTS.rax = ERROR;
			break;
		}
		if(current_thread > 0) {

		}
		schedule = semaphoredown(s);


		SYSCALL_ARGUMENTS.rax = ALL_OK;
		break;
	}

	case SYSCALL_SEMAPHOREUP:
	{
		int current_thread = cpu_private_data.thread_index;
		int current_thread_owner = thread_table[current_thread].data.owner;
		struct semaphore* s = &semaphore_table[SYSCALL_ARGUMENTS.rdi];



		if (s->owner != current_thread_owner){
			kprints("ERROR! Process is not owner of semaphore");
			SYSCALL_ARGUMENTS.rax = ERROR;
			break;
		}


		semaphoreup(s);
		SYSCALL_ARGUMENTS.rax = ALL_OK;
		break;
	}


	/* Do not touch any lines above or including this line. */

	/* Add the implementation of more system calls here. */


	/* Do not touch any lines below or including this line. */
	default:
	{
		/* No system call defined. */
		SYSCALL_ARGUMENTS.rax = ERROR_ILLEGAL_SYSCALL;
	}
	}

	return schedule;
}
