/*! \file 
    \brief Holds the implementation of the synchronization sub-system. */

#include "kernel.h"
#include "sync.h"

/* The function interfaces are documented in sync.h */

struct port
port_table[MAX_NUMBER_OF_PORTS];

volatile unsigned int
port_table_lock = 0;

void
initialize_ports(void)
{
 register int i;

 /* Loop over all ports in the table and set the owner to -1 indicating a 
    free port. */
 for(i=0; i<MAX_NUMBER_OF_PORTS; i++)
 {
  port_table[i].owner=-1;
  port_table[i].lock=0;
 }
}

int
allocate_port(const unsigned long id, const int new_owner)
{
 register int i;
 register int first_available=-1;
 int return_value;

 /* Loop over all ports to see if the port has already been allocated. */
 grab_lock_rw(&port_table_lock);
 for(i=0; i<MAX_NUMBER_OF_PORTS; i++)
 {
  /* We keep track of the first available port. This way we do not
     have to scan the table twice. */
  if ((-1 == port_table[i].owner) &&
      (-1 == first_available))
  {
   first_available=i;
  }

  /* Check if the port entry match the one we want to create. */
  if ((new_owner == port_table[i].owner) &&
      (id == port_table[i].id))
  {
   return_value = -1;
  }
 }

 if (-1 != first_available)
 {
  /* Set the new owner and new identity. */
  port_table[first_available].owner=new_owner;
  port_table[first_available].id=id;

  /* Initially, the sender queue is  empty and there are no receiving thread. */
  port_table[first_available].receiver=-1;
  thread_queue_init(&port_table[first_available].sender_queue);

  return_value = first_available;
 }
 release_lock(&port_table_lock);

 /* Return -1 if we ran out of ports. */
 return return_value;
}

int
find_port(const unsigned long id, const int owner)
{
 register int i;
 int return_value = -1;

 grab_lock_r(&port_table_lock);
 /* Loop over all ports in the table. */
 for(i=0; i<MAX_NUMBER_OF_PORTS; i++)
 {
  /* Return as soon as we find a match. */
  if ((owner == port_table[i].owner) &&
      (id == port_table[i].id))
  {
   return_value = i;
   break;
  }
 }
 release_lock(&port_table_lock);

 return return_value;
}

void
initialize_thread_synchronization(void)
{
 /* In task 6, add code here. */
}

/* Put any code you need to add to implement tasks B5, A5, B6 or A6 here. */
