/*!
 \file network.h
 This file holds macros and declarations for the network system and the ne2k
 driver.
 */

#ifndef _NETWORK_H_
#define _NETWORK_H_

/*! Base IO address for the ne2k card. */
#define NE2K_BASE_IO_ADDRESS (0x300)

/* MAC address for the ne2k card. */
#define MAC_ADDRESS_0 (0xfe)
#define MAC_ADDRESS_1 (0xfd)
#define MAC_ADDRESS_2 (0xde)
#define MAC_ADDRESS_3 (0xad)
#define MAC_ADDRESS_4 (0xbe)
#define MAC_ADDRESS_5 (0xef)

/* IP address for the ne2k card. */
#define IP_ADDRESS_0 (192)
#define IP_ADDRESS_1 (168)
#define IP_ADDRESS_2 (1)
#define IP_ADDRESS_3 (2)

/*! Initializes the ne2k driver. */
extern void
initialize_ne2k(void);

/*! The ne2k interrupt routine. */
extern void
ne2k_interrupt_handler(void);

/*! Sends one Ethernet frame. The source MAC address will be filled in by
    the function. The function blocks until enough space is available in the
    send buffer. */
extern void
ne2k_send_frame(register unsigned char* const frame
                /*!< Pointer to the Ethernet frame to be sent. */,
                register const unsigned int length
                /*!< Length of the frame to be sent. */);

#endif
