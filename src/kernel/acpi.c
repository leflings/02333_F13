/*! \file acpi.c
 *  \brief
 *  This file contains code that parses ACPI tables. For more information on
 *  ACPI see http://www.acpi.info .
 */

#define MAX_NUMBER_OF_CPUS (16)

extern void
parse_acpi_tables(void);

extern void
halt_the_machine(void);

typedef struct
{
 char               signature[8];
 unsigned char      checksum;
 char               OEMID[6];
 char               revision;
 void*              rsdt_address;
 unsigned int       length;
 unsigned long long xsdt_address;
 unsigned char      extended_checksum;
 char               reserved[3];
} RSD;

typedef struct
{
 char               signature[4];
 unsigned int       length;
 char               revision;
 unsigned char      checksum;
 char               OEMID[6];
 char               OEM_table_id[8];
 unsigned int       OEM_revision;
 unsigned int       creator_id;
 unsigned int       creator_revision;
} DESCRIPTION_HEADER;

typedef struct
{
 DESCRIPTION_HEADER dheader;
 unsigned long long entry[0];
} XSDT;

typedef struct
{
 DESCRIPTION_HEADER  dheader;
 DESCRIPTION_HEADER* entry[0];
} RSDT;

typedef struct
{
 unsigned char       type;
 unsigned char       length;
} APIC_structure;

typedef struct
{
 DESCRIPTION_HEADER  dheader;
 unsigned int        local_APIC_address;
 unsigned int        flags;
 char                structures[0];
} MADT;

typedef struct
{
 APIC_structure      header;
 unsigned char       ACPI_processor_id;
 unsigned char       APIC_id;
 unsigned int        flags;
} processor_local_APIC_structure;

typedef struct
{
 APIC_structure      header;
 unsigned char       IO_APIC_id;
 unsigned char       reserved;
 unsigned int        IO_APIC_address;
 unsigned int        global_system_interrupt_base;
} IO_APIC_structure;

typedef struct
{
 APIC_structure      header;
 unsigned char       bus;
 unsigned char       source;
 unsigned int        global_system_interrupt;
 unsigned short      flags;
} ISO_structure;

extern unsigned int
number_of_available_CPUs;

unsigned int
number_of_available_CPUs = 0;

extern unsigned long long
APIC_id_bit_field;

unsigned long long
APIC_id_bit_field = 1;

extern unsigned long long
pic_interrupt_bitfield;

unsigned long long
pic_interrupt_bitfield;

static unsigned int
pic_interrupt_map[16];

static int
strncmp(register const char* buffer, register const char* string,
        register unsigned int length)
{
 for(; length>0; length--, buffer++, string++)
 {
  if (*buffer != *string)
  {
   return 0;
  }
 }
 return 1;
}

static RSD*
search_for_RSD(register const char* area, register unsigned int length)
{
 for(;length>0; length-=16, area+=16)
 {
  /* Check the checksum. */
  register char checksum = 0;
  register int  i;

  for (i=0; i<20; i++)
  {
   checksum+=area[i];
  }

  /* Check if the checksum is correct. */
  if (0 != checksum)
   continue;

  if (0 == strncmp(area, "RSD PTR ", 8))
   continue;

  return (RSD*) area;
 }
 return (RSD*) 0;
}

static int
parse_description_header(const DESCRIPTION_HEADER* const table)
{
 register unsigned int i;
 register char         checksum = 0;
 register unsigned int curr_length;
 register unsigned int ioapic_index=0;
 register int          return_value=1;

 if (0 == strncmp(table->signature, "APIC", 4))
  return 0;

 /* Check checksum. */
 for(i=0; i<table->length; i++)
 {
  checksum += ((char*) table)[i];
 }

 if (0 != checksum)
  return 0;

 /* Checksum is correct and we have found an MADT. Store the physical address
    of the APIC. */
 register unsigned int const APIC_address =
  ((MADT*) table) -> local_APIC_address;

 /* Sanity check the address. */
 if (0 != (APIC_address & 0xfff))
  return 0;

 if (0xfec00000 !=  (APIC_address & 0xffc00000))
  return 0;

 /* Now we need to parse all the entries in the table. */
 for(curr_length=44; curr_length < table->length;)
 {
  register const APIC_structure* const structure =
   ((APIC_structure*) &((MADT*) table) -> structures[curr_length-44]);

  curr_length += structure -> length;

  switch (structure -> type)
  {
   case 0: /* Processor local APIC */
   {
    register const processor_local_APIC_structure* const local_APIC_structure =
     (processor_local_APIC_structure*) structure;

    /* Sanity check the size of the structure. */
    if (8 != structure->length)
    {
     return_value=0;
     break;
    }

    /* Check if processor is disabled. */
    if (((local_APIC_structure->flags)&1) == 0)
     break;

    /* Check base address. */
    if (0xfee00000UL != APIC_address)
     break;

    /* Cannot support ids larger than 63. */
    if (local_APIC_structure->APIC_id > 63)
     break;

    /* Check if the kernel cannot support more CPUs. */
    if (MAX_NUMBER_OF_CPUS <= number_of_available_CPUs)
     break;

    /* Set bit field. */
    APIC_id_bit_field |= 1 << (local_APIC_structure->APIC_id);

    number_of_available_CPUs++;

    break;
   }

   case 1: /* I/O APIC */
   {
    register const IO_APIC_structure* const IO_APIC_structure_ptr =
     (IO_APIC_structure*) structure;

    /* Sanity check the size of the structure. */
    if (12 != structure->length)
    {
     return_value=0;
     break;
    }

    /* We can only handle one IO APIC. The IO APIC must handle interrupts
       starting from Global System Interrupt 0. */
    if ((0 != ioapic_index) ||
        (0 != IO_APIC_structure_ptr->global_system_interrupt_base))
    {
     return_value=0;
     break;
    }

    /* Sanity check the address. */
    if (0xfec00000 != IO_APIC_structure_ptr->IO_APIC_address)
    {
     return_value=0;
     break;
    }

    ioapic_index++;

    break;
   }

   case 2:
   {
    /* Interrupt Source Override. */
    register const ISO_structure* const iso_structure_ptr =
     (ISO_structure*) structure;

    /* Sanity check. */
    if ((10 != structure->length) ||
        (0 != iso_structure_ptr->bus) ||
        (15 < iso_structure_ptr->source) ||
        (32 <= iso_structure_ptr->global_system_interrupt))
    {
     return_value=0;
     break;
    }

    pic_interrupt_map[iso_structure_ptr->source]=
     iso_structure_ptr->global_system_interrupt;
    break;
   }

   case 3: /* NMI  */
   case 4: /* Local APIC NMI Structure. */
   {
    /* We just don't care about these. */
    break;
   }

   default:
    halt_the_machine();
  }
 }

 /* Did we actually find any useful information? */
 if ((0 == ioapic_index) || (0 == number_of_available_CPUs))
  return_value = 0;

 return return_value;
}

static void
compress_pic_interrupt_map(void)
{
 register int i;
 /* Create a compressed version of the pic interrupt map. */
 pic_interrupt_bitfield = 0;

 for(i=0; i<12; i++)
 {
  pic_interrupt_bitfield |= ((unsigned long long)(pic_interrupt_map[i] & 31)) << (i*5);
 }
}

/*! Parse ACPI tables and extract the information needed to boot the system. */
void
parse_acpi_tables(void)
{
 register char* EDBA = (char*)
                       (((unsigned long)(*((unsigned short*) 0x40e)))<<4);

 /* Search the first 1k of EDBA for the RSD. */
 register RSD* RSDP = search_for_RSD(EDBA, 1024);
 register int  done = 0;

 /* Check if we actually found the RSD. */
 if (0 == RSDP)
 {
  /* No we didn't. Search the BIOS. */
  RSDP = search_for_RSD((char*) 0xe0000, 0x20000);
 }

 if (0 == RSDP)
 {
  /* Couldn't find RSDP. We have to halt. */
  halt_the_machine();
 }

 /* Initialize the data structures. */
 {
  unsigned int index;
  for (index=0; index<16; index++)
  {
   pic_interrupt_map[index]=index;
  }
 }

 /* We have found the RSDP. Now we need to figure out if we should use the
    RSDT or the XSDT. We always use XSDT if we can find it and it is
    located within the first 4GB. */
 {
  /* Check the extended checksum. */
  register char checksum = 0;
  register int  i;

  for (i=0; i<36; i++)
  {
   checksum+=((char*) RSDP)[i];
  }

  /* Check if the checksum is correct and the XSDT is within the . */
  if ((0 == checksum) && (RSDP->xsdt_address < (0x100000000ULL-sizeof(XSDT))))
  {
   register const XSDT* const xsdt = (XSDT*) ((long)(RSDP->xsdt_address&
                                                     0xffffffff));

   /* Check if the XSDT is valid. */
   if ((0 != strncmp(xsdt->dheader.signature, "XSDT", 4)) &&
       ((xsdt->dheader.length&3) == 0))
   {
    /* Check checksum. */
    register unsigned int i;
    register char         checksum = 0;

    for(i=0; i<xsdt->dheader.length; i++)
    {
     checksum += ((char*) xsdt)[i];
    }

    if (0 == checksum)
    {
     /* Calculate the number of entries. */
     register unsigned int entries = (xsdt->dheader.length-36) >> 3;
     /* Go through the DESCRIPTION_HEADERS looking for ACPI tables. */
     for(i=0; i<entries; i++)
     {
      if (xsdt->entry[i] >0x100000000ULL)
       continue;

      done |= parse_description_header(
       (DESCRIPTION_HEADER*) ((unsigned int)(xsdt->entry[i] & 0xffffffff)));
     }

     /* Return if we could parse the table. */
     if (0 != done)
     {
      compress_pic_interrupt_map();
      return;
     }
    }
   }
  }
 }

 /* Couldn't use the XSDT. Fall back to the RSDT. The RSDT only contains 32-bit
    physical addresses so we do not have to check if they are accessible. */
 {
  register const RSDT* const rsdt = (RSDT*) (RSDP->rsdt_address);

  /* Check if the RSDT is valid. */
  if ((0 != strncmp(rsdt->dheader.signature, "RSDT", 4)) &&
      ((rsdt->dheader.length&3) == 0))
  {
   /* Check checksum. */
   register unsigned int i;
   register char         checksum = 0;

   for(i=0; i<rsdt->dheader.length; i++)
   {
    checksum += ((char*) rsdt)[i];
   }

   if (0 == checksum)
   {
    /* Calculate the number of entries. */
    register unsigned int entries = (rsdt->dheader.length-36) >> 2;

    /* Go through the DESCRIPTION_HEADERS looking for ACPI tables. */
    for(i=0; i<entries; i++)
    {
     done |= parse_description_header(rsdt->entry[i]);
    }

    /* Return if we could parse the table. */
    if (0 != done)
    {
     compress_pic_interrupt_map();
     return;
    }
   }
  }
 }

 halt_the_machine();
}
