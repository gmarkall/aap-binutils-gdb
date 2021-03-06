/* AAP-specific support for 32-bit ELF
   Copyright 1994, 1995, 1997, 1999, 2001, 2002, 2005, 2007
   Free Software Foundation, Inc.
   Contributed by Doug Evans (dje@cygnus.com).

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "libiberty.h"
#include "elf/aap.h"
#include "bfd_stdint.h"

#define BASEADDR(SEC)	((SEC)->output_section->vma + (SEC)->output_offset)

/* The top 5 bits contain the address space id, we need to mask this off
   to get the actual memory address */
#define AAP_GET_MEM_SPACE(ADDR) (((ADDR) >> (32 - 5)) & 0x1f)
#define AAP_GET_ADDR_LOCATION(ADDR) ((ADDR) & 0x7ffffff)
#define AAP_BUILD_ADDRESS(ID, ADDR) (((ID & 0x1f) << (32 - 5)) | (ADDR & 0x7ffffff))

static reloc_howto_type elf_aap_howto_table[] =
{
  HOWTO (R_AAP_NONE,            /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         16,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_bitfield,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_NONE",          /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0,                     /* Src_mask.  */
         0,                     /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_8,               /* Type.  */
         0,                     /* Rightshift.  */
         0,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         8,                     /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_bitfield,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_8",             /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0xff,                  /* Src_mask.  */
         0xff,                  /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_16,              /* Type.  */
         0,                     /* Rightshift.  */
         1,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         16,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_bitfield,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_16",            /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0xffff,                /* Src_mask.  */
         0xffff,                /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_32,              /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         32,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_bitfield,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_32",            /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0xffffffff,            /* Src_mask.  */
         0xffffffff,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_64,              /* Type.  */
         0,                     /* Rightshift.  */
         4,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         64,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_bitfield,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_64",            /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0xffffffffffffffff,    /* Src_mask.  */
         0xffffffffffffffff,    /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BR16,            /* Type.  */
         0,                     /* Rightshift.  */
         1,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         9,                     /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BR16",          /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x01ff,                /* Src_mask.  */
         0x01ff,                /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BR32,            /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         22,                    /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BR32",          /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1fff01ff,            /* Src_mask.  */
         0x1fff01ff,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BRCC16,          /* Type.  */
         0,                     /* Rightshift.  */
         1,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         3,                     /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         6,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BRCC16",        /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x01c0,                /* Src_mask.  */
         0x01c0,                /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BRCC32,          /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         10,                    /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         6,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BRCC32",        /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1fc001c0,            /* Src_mask.  */
         0x1fc001c0,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BAL16,           /* Type.  */
         0,                     /* Rightshift.  */
         1,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         6,                     /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         3,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BAL16",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x01f8,                /* Src_mask.  */
         0x01f8,                /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_BAL32,           /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         16,                    /* Bitsize.  */
         TRUE,                  /* PC_relative.  */
         3,                     /* Bitpos. */
         complain_overflow_signed,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_BAL32",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1ff801f8,            /* Src_mask.  */
         0x1ff801f8,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_ABS6,            /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         6,                     /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_ABS6",          /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x00070007,            /* Src_mask.  */
         0x00070007,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_ABS9,            /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         9,                     /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_ABS9",          /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1c070007,            /* Src_mask.  */
         0x1c070007,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_ABS10,           /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         10,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_ABS10",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1e070007,            /* Src_mask.  */
         0x1e070007,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_ABS12,           /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         12,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_ABS12",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x003f003f,            /* Src_mask.  */
         0x003f003f,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_ABS16,           /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         16,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_ABS16",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1e3f003f,            /* Src_mask.  */
         0x1e3f003f,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_SHIFT6,          /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         6,                     /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_SHIFT6",        /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x00070007,            /* Src_mask.  */
         0x00070007,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
  HOWTO (R_AAP_OFF10,           /* Type.  */
         0,                     /* Rightshift.  */
         2,                     /* Size (0 = byte, 1 = short, 2 = long).  */
         10,                    /* Bitsize.  */
         FALSE,                 /* PC_relative.  */
         0,                     /* Bitpos. */
         complain_overflow_unsigned,/* Complain_on_overflow.  */
         bfd_elf_generic_reloc, /* Special_function.  */
         "R_AAP_OFF10",         /* Name.  */
         TRUE,                  /* Partial_inplace.  */
         0x1e070007,            /* Src_mask.  */
         0x1e070007,            /* Dst_mask.  */
         FALSE),                /* PCrel_offset.  */
};

/* Map BFD reloc types to AAP ELF reloc types.  */

struct aap_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char elf_reloc_val;
};

static const struct aap_reloc_map aap_reloc_map[] =
{
  { BFD_RELOC_NONE, R_AAP_NONE },
  { BFD_RELOC_8,    R_AAP_8 },
  { BFD_RELOC_16,   R_AAP_16 },
  { BFD_RELOC_32,   R_AAP_32 }
};

static reloc_howto_type *
bfd_elf32_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = ARRAY_SIZE (aap_reloc_map); i--;)
    if (aap_reloc_map[i].bfd_reloc_val == code)
      return elf_aap_howto_table + aap_reloc_map[i].elf_reloc_val;

  return NULL;
}

static reloc_howto_type *
bfd_elf32_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (elf_aap_howto_table) / sizeof (elf_aap_howto_table[0]);
       i++)
    if (elf_aap_howto_table[i].name != NULL
	&& strcasecmp (elf_aap_howto_table[i].name, r_name) == 0)
      return &elf_aap_howto_table[i];

  return NULL;
}

/* Set the howto pointer for an AAP ELF reloc.  */

static void
aap_info_to_howto_rel (bfd *abfd ATTRIBUTE_UNUSED,
		       arelent *cache_ptr,
		       Elf_Internal_Rela *dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  BFD_ASSERT (r_type < (unsigned int) R_AAP_max);
  cache_ptr->howto = &elf_aap_howto_table[r_type];
}

/* Set the right machine number for an AAP ELF file.  */
static bfd_boolean
aap_elf_object_p (bfd *abfd)
{
  // Set the machine to 0. (we have no officially assigned machine number.)
  unsigned int mach = 0;
  return bfd_default_set_arch_mach (abfd, bfd_arch_aap, mach);
}

/* Perform a single relocation.
   The SYMBOL_NAME is passed only for a debugging aid.  */

bfd_reloc_status_type
aap_relocate_contents (reloc_howto_type *howto,
		       bfd *input_bfd,
		       bfd_vma relocation,
		       bfd_byte *location);

static bfd_reloc_status_type
aap_final_link_relocate (reloc_howto_type *  howto,
			  bfd *               input_bfd,
			  asection *          input_section,
			  bfd_byte *          contents,
			  Elf_Internal_Rela * rel,
			  bfd_vma             relocation,
			  asection *          symbol_section,
			  const char *        symbol_name ATTRIBUTE_UNUSED,
                          struct elf_link_hash_entry * h ATTRIBUTE_UNUSED)
{
  bfd_vma offset = rel->r_offset;
  bfd_vma addend = rel->r_addend;
  bfd_vma address = (input_section->output_section->vma
		     + input_section->output_offset
		     + offset);
  bfd_vma reloc_mem_space;

  /* Sanity check the address */
  if (offset > bfd_get_section_limit_octets (input_bfd, input_section))
    return bfd_reloc_outofrange;

  relocation += addend;

  /* memory space that this relocation points to */
  reloc_mem_space = AAP_GET_MEM_SPACE(relocation);

  /* Check that the memory space of the relocation is the same as that of
     its address. If not, then treat it as being out of range
     FIXME: This needs a more helpful error message */
  if (howto->pc_relative
      && reloc_mem_space != 0
      && reloc_mem_space != AAP_GET_MEM_SPACE(address))
    return bfd_reloc_outofrange;

  /* actual location of the relocation and address sans memory space */
  relocation = AAP_GET_ADDR_LOCATION(relocation);
  bfd_vma address_location = AAP_GET_ADDR_LOCATION(address);

  /* if pc-relative, we want RELOCATION to be the distance between the symbol
   * and the location we are relocating */
  if (howto->pc_relative)
    relocation -= address_location;

  /* If the relocation is to a code address and is not located inside debug
     information then we must scale the value to make it word addressed */
  if (symbol_section
      && ((symbol_section->flags & SEC_CODE) != 0)
      && ((input_section->flags & SEC_DEBUGGING) == 0))
    {
      /* FIXME: handle the case where lsb erroneously set */
      if (relocation & 1)
	return bfd_reloc_outofrange;
      if (address_location & 1)
	return bfd_reloc_outofrange;

      /* Scale the byte addresses into word addresses */
      relocation = ((bfd_signed_vma)relocation) >> 1;
      address_location = ((bfd_signed_vma)address_location) >> 1;
    }

  /* If the input section is a debug section, then we need to merge the
     address space bits back into the relocation */
  if ((input_section->flags & SEC_DEBUGGING) != 0
      && !howto->pc_relative
      && (howto->bitsize == 32 || howto->bitsize == 64))
    relocation = AAP_BUILD_ADDRESS (reloc_mem_space, relocation);

  /* Call a custom function to handle actually emplacing the relocation. This
     is necessary as most of our relocations are not contiguous and require
     special handling */
  return aap_relocate_contents (howto, input_bfd, relocation,
				contents + offset);
}


bfd_reloc_status_type
aap_relocate_contents (reloc_howto_type *howto,
		       bfd *input_bfd,
		       bfd_vma relocation,
		       bfd_byte *location)
{
  if (howto->type == R_AAP_BR32)
    {
      /* check for signed overflow, top bit must match upper bits */
      bfd_signed_vma sign = ((bfd_signed_vma)relocation) >> 21;
      if ((sign != 0) && (sign != -1))
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation &  0x1ff) <<  0;  relocation >>= 9;
      x |= (relocation & 0x1fff) << 16;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_BRCC32)
    {
      /* signed overflow check, top bit must match upper bits  */
      bfd_signed_vma sign = ((bfd_signed_vma)relocation) >> 9;
      if ((sign != 0) && (sign != -1))
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation &  0x7) <<  6;  relocation >>= 3;
      x |= (relocation & 0x7f) << 22;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_BAL32)
    {
      /* signed overflow check, top bit must match upper bits  */
      bfd_signed_vma sign = ((bfd_signed_vma)relocation) >> 17;
      if ((sign != 0) && (sign != -1))
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation &  0x3f) << 3;  relocation >>= 6;
      x |= (relocation & 0x3ff) << 19;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_ABS6)
    {
      if (relocation >> 6 != 0)
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x7) <<  0;  relocation >>= 3;
      x |= (relocation & 0x7) << 16;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_ABS9)
    {
      if (relocation >> 9 != 0)
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x7) <<  0;  relocation >>= 3;
      x |= (relocation & 0x7) << 16;  relocation >>= 3;
      x |= (relocation & 0x7) << 26;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_ABS10)
    {
      if (relocation >> 10 != 0)
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x7) <<  0;  relocation >>= 3;
      x |= (relocation & 0x7) << 16;  relocation >>= 3;
      x |= (relocation & 0xf) << 25;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_ABS12)
    {
      if (relocation >> 12 != 0)
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x3f) <<  0;  relocation >>= 6;
      x |= (relocation & 0x3f) << 16;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_ABS16)
    {
      /* relocation must be in the range [-32768, 65535] */
      bfd_signed_vma reloc_signed = (bfd_signed_vma)relocation;
      if ((reloc_signed < -32768) || (reloc_signed > 65535))
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x3f) <<  0;  relocation >>= 6;
      x |= (relocation & 0x3f) << 16;  relocation >>= 6;
      x |= (relocation &  0xf) << 25;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_SHIFT6)
    {
      if (relocation == 0 || relocation > 64)
	return bfd_reloc_outofrange;

      /* map from the range [1, 64], to [0, 63] for encoding */
      relocation -= 1;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x7) <<  0;  relocation >>= 3;
      x |= (relocation & 0x7) << 16;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else if (howto->type == R_AAP_OFF10)
    {
      /* signed overflow check, top bit must match upper bits  */
      bfd_signed_vma sign = ((bfd_signed_vma)relocation) >> 10;
      if ((sign != 0) && (sign != -1))
	return bfd_reloc_outofrange;

      bfd_vma x = bfd_get_32(input_bfd, location);
      x &= ~howto->dst_mask;

      x |= (relocation & 0x7) <<  0;  relocation >>= 3;
      x |= (relocation & 0x7) << 16;  relocation >>= 3;
      x |= (relocation & 0xf) << 25;

      bfd_put_32 (input_bfd, x, location);
      return bfd_reloc_ok;
    }
  else
    {
      return _bfd_relocate_contents (howto, input_bfd, relocation, location);
    }
}

/* Relocate an AAP ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjusting the section contents as
   necessary, and (if using Rela relocs and generating a relocatable
   output file) adjusting the reloc addend as necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocatable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

static bfd_boolean
aap_elf_relocate_section (bfd *output_bfd ATTRIBUTE_UNUSED,
			       struct bfd_link_info *info,
			       bfd *input_bfd,
			       asection *input_section,
			       bfd_byte *contents,
			       Elf_Internal_Rela *relocs,
			       Elf_Internal_Sym *local_syms,
			       asection **local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *           howto;
      unsigned long                r_symndx;
      Elf_Internal_Sym *           sym;
      asection *                   sec;
      struct elf_link_hash_entry * h;
      bfd_vma                      relocation;
      bfd_reloc_status_type        r;
      const char *                 name = NULL;
      int                          r_type ATTRIBUTE_UNUSED;

      r_type = ELF32_R_TYPE (rel->r_info);
      r_symndx = ELF32_R_SYM (rel->r_info);
      howto  = elf_aap_howto_table + ELF32_R_TYPE (rel->r_info);
      h      = NULL;
      sym    = NULL;
      sec    = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  asection *osec;

	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  osec = sec;

	  if ((sec->flags & SEC_MERGE)
	      && ELF_ST_TYPE (sym->st_info) == STT_SECTION)
	    {
	      /* This relocation is relative to a section symbol that is going
		 to be merged.  Change it so that it is relative to the merged
		 section symbol.  */
	      rel->r_addend = _bfd_elf_rel_local_sym (output_bfd, sym, &sec,
						      rel->r_addend);
	    }

	  relocation = BASEADDR (sec) + sym->st_value;

	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = (name == NULL) ? bfd_section_name (input_bfd, osec) : name;
	}
      else
	{
	  bfd_boolean warned ATTRIBUTE_UNUSED;
	  bfd_boolean unresolved_reloc ATTRIBUTE_UNUSED;
	  bfd_boolean ignored ATTRIBUTE_UNUSED;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);

	  name = h->root.root.string;
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (info->relocatable)
	continue;

      /* Finally, the sole AAP-specific part.  */
      r = aap_final_link_relocate (howto, input_bfd, input_section,
                                    contents, rel, relocation, sec, name, h);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      r = info->callbacks->reloc_overflow
		(info, (h ? &h->root : NULL), name, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      break;

	    case bfd_reloc_undefined:
	      r = info->callbacks->undefined_symbol
		(info, name, input_bfd, input_section, rel->r_offset, TRUE);
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	      /* This is how aap_final_link_relocate tells us of a
		 non-kosher reference between insn & data address spaces.  */
	    case bfd_reloc_notsupported:
	      if (sym != NULL) /* Only if it's not an unresolved symbol.  */
		 msg = _("unsupported relocation between data/insn address spaces");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    r = info->callbacks->warning
	      (info, msg, name, input_bfd, input_section, rel->r_offset);

	  if (! r)
	    return FALSE;
	}
    }

  return TRUE;
}

#define TARGET_LITTLE_SYM   aap_elf32_vec
#define TARGET_LITTLE_NAME  "elf32-aap"
/*#define TARGET_BIG_SYM      bfd_elf32_aap_vec*/
/*#define TARGET_BIG_NAME     "elf32-aap"*/

#define ELF_ARCH            bfd_arch_aap
#define ELF_MACHINE_CODE    EM_AAP
#define ELF_MAXPAGESIZE     0x1000

#define elf_info_to_howto                   0
#define elf_info_to_howto_rel               aap_info_to_howto_rel
#define elf_backend_object_p                aap_elf_object_p
#define elf_backend_relocate_section		aap_elf_relocate_section

#include "elf32-target.h"
