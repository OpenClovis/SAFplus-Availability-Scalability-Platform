/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : utils
 * File        : bit.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Bit manipulation routines.                               
 *************************************************************************/
/** @pkg cl.cbl */

/* FILES INCLUDED */
#include <clCommonErrors.h>
#include <clBitApi.h>

/*
 * FIXME: We should not need this as a global variable.  Endiannes
 * issues can be handled in compile time.
 */
ClUint8T clByteEndian;

/*
 * This library provides some low level routines to get/set/manipulate
 * bit fields in a bit array.
 * Most macros/function operate on a bit array. Bits in the bit array
 * are numbered starting from 0 to MAX_BITS-1. For example, in case of 64
 * bits array, first bit would be numbered 0 and the last one would be
 * numberred 63.
 */


/*
*  Function to detect the EndianNess.
* @return
*   CL_BIT_BIG_ENDIAN if it is Big Endian machine
*   CL_BIT_LITTLE_ENDIAN if it is a Little Endian machine
*/
ClUint8T clBitBlByteEndianGet(void)
{
   ClUint32T number = 0x11223344;
   ClUint8T byteArray[]= {0x11,0x22,0x33,0x44};
   if(number == *((ClUint32T *)byteArray))
   {
   clByteEndian = CL_BIT_BIG_ENDIAN;
   return (CL_BIT_BIG_ENDIAN);
   }else
    {
     clByteEndian = CL_BIT_LITTLE_ENDIAN; 
     return (CL_BIT_LITTLE_ENDIAN);
    }
}


/**
 *  Retrieve a bit field from the bit array.
 *
 *  This routine returns a bit field of specified size
 * from the given bit array. Currently there is a limit
 * on the maximum length of the field. This limit is
 * defines by CL_BIT_MAX_BIT_FIELD_SIZE.
 *
 *  @param bitArr   Input bit array.
 *  @param bitStart Retieve bits starting from this bit number.
 *  @param len      Specifies how many bits to retrieve.
 *  @param value    [OUT] the bit field is returned here.
 *
 *  @returns
 *    CL_OK on success. CL_ERR_NULL_POINTER or CL_ERR_INVALID_PARAM
 * on error.
 *
 */
ClRcT
clBitFieldGet (ClUint8T *bitArr,
               ClUint32T bitStart,
	       ClInt32T len,
	       ClUint64T *value)
{
    ClUint32T pre;
	ClUint8T *byteStart;

    /* input parameter checking */
    if ((bitArr == NULL) || (value == NULL))
	    return (CL_ERR_NULL_POINTER);

    if (len > CL_BIT_MAX_BIT_FIELD_SIZE)
	    return (CL_ERR_INVALID_PARAMETER);

    /* byte to start from */
    byteStart = bitArr + bitStart/CL_BIT_BITS_IN_BYTE;

    /* number of bits from the first byte */
    pre = CL_BIT_BITS_IN_BYTE - (bitStart % CL_BIT_BITS_IN_BYTE);

    /* get the bits from the first byte */
	*value = (*byteStart) & clBitMaskB(pre);
	len -= pre;

	/* move to the next byte */
	byteStart++;

    /*
	 * Until len > 0, get the bits from the remaining bytes.
	 * Note that we have to go byte-by-byte to avoid endianness AND
	 * alignment issues
	 */
	while (len > 0)
	{
	    *value <<= CL_BIT_BITS_IN_BYTE;
		*value |= *(byteStart);
		byteStart++;
		len -= CL_BIT_BITS_IN_BYTE;
	}


    /*
	 * Since we moved *value by CL_BIT_BITS_IN_BYTE, we may have got more than
	 * we should from the last byte. Readjust.
	 */
	*value >>= (-len) ;

	return (CL_OK);
}

/**
 *  Set a bit field in the bit array.
 *
 *  This routine sets value of a bit field of specified size
 * in the given bit array. Currently there is a limit
 * on the maximum length of the field. This limit is
 * defines by CL_BIT_MAX_BIT_FIELD_SIZE.
 *
 *  @param bitArr   Input bit array.
 *  @param bitStart Set bits starting from this bit number.
 *  @param len      Specifies how many bits to retrieve.
 *  @param value    bit field value.
 *
 *  @returns
 *    CL_OK on success. CL_ERR_NULL_POINTER or CL_ERR_INVALID_PARAM
 * on error.
 *
 */
ClRcT
clBitFieldSet (ClUint8T *bitArr,
                ClUint32T bitStart,
                ClUint32T len,
                ClUint64T value)
{
    ClUint64T tmp = 0;
    ClUint64T mask = (ClUint64T)0xFFFFFFFFFFFFFFFFLL;
    ClUint32T pre, post;
    ClUint32T byteOffset;
    ClUint32T numBits;
    ClUint32T numBytes;

    /* parameter checking */
    if (bitArr == NULL)
	    return (CL_ERR_NULL_POINTER);

    if (len >= CL_BIT_MAX_BIT_FIELD_SIZE)
	    return (CL_ERR_INVALID_PARAMETER);

    byteOffset = bitStart/CL_BIT_BITS_IN_BYTE;
    pre = bitStart % CL_BIT_BITS_IN_BYTE;
    post = CL_BIT_BITS_IN_DWORD - len - pre;

    /* position the value and mask based on the length and startBit */
    value <<= post;
    mask <<= post + pre;
    mask >>= pre;

	/* Make sure if write only the len bits */
    value &=mask;

    /*
	 * Since we are working on the whole word, need to take care of endianness.
	 */
    value = CL_BIT_H2N64(value);
    mask = CL_BIT_H2N64(mask); 

    /* The number of bytes to be copied. (to avoid boundary crossing) */
       numBits = (bitStart + len) - (byteOffset * CL_BIT_BITS_IN_BYTE);
       numBytes = numBits/CL_BIT_BITS_IN_BYTE;
       if(numBits % CL_BIT_BITS_IN_BYTE)
         ++numBytes;

    /* get the current value */
	memcpy (&tmp, &bitArr[byteOffset], numBytes);

    /* Write the bit field to tmp, avoid changing surrounding bits */
	tmp = (tmp & ~mask) | value;

    /*
	 * write tmp to bit array. Need to copy byte-by-byte to avoid
	 * any alignment issues.
	 */
	memcpy (&bitArr[byteOffset], &tmp, numBytes);

	return (CL_OK);
}

/**
 *  Right shift a bit array.
 *
 *  This routine right shift the bits in a bit array.
 *
 *  @param bitArr   Input bit array.
 *  @param size     Total size of the bit array in bytes.
 *  @param len      Right shift the bit array by these many bits.
 *
 *  @returns
 *    CL_OK on success. CL_ERR_NULL_POINTER on error.
 *
 */
ClRcT clBitFieldRShift (ClUint8T *bitArr, ClUint32T size, ClUint32T len)
{
    ClUint32T byteShift;
    ClUint32T bitShift;
	int i;

    /* parameter checking */
    if (bitArr == NULL)
	    return (CL_ERR_NULL_POINTER);

    if ((len == 0) || (size == 0))
	    return (CL_OK);

    /* byte portion */
    byteShift = len/CL_BIT_BITS_IN_BYTE; 
    /* bit portion */
    bitShift = len%CL_BIT_BITS_IN_BYTE; 

    /* shift the byte portion first */
	if (byteShift != 0)
	{
        for (i = size - 1; i >= (ClInt32T)byteShift; i--)
    	{
		    bitArr[i] = bitArr[i - byteShift];
    	}
		for (; i > 0; i--)
		    bitArr[i] = 0;
	}

    /* shift the bit portion */
    if (bitShift != 0)
	{
        for (i = size - 1; i > 0; i--)
    	{
    	   bitArr [i] = (bitArr [i] >> bitShift) | 
    	                (bitArr [i-1] << (CL_BIT_BITS_IN_BYTE - bitShift));
    	}
	    bitArr [0] >>= len;
	}

	return (CL_OK);
}

/**
 *  Left shift a bit array.
 *
 *  This routine left shift the bits in a bit array.
 *
 *  @param bitArr   Input bit array.
 *  @param size     Total size of the bit array in bytes.
 *  @param len      Left shift the bit array by these many bits.
 *
 *  @returns
 *    CL_OK on success. CL_ERR_NULL_POINTER on error.
 *
 */
ClRcT clBitFieldLShift (ClUint8T *bitArr, ClUint32T size, ClUint32T len)
{
    ClUint32T byteShift;
    ClUint32T bitShift;
	int i;

    /* parameter checking */
    if (bitArr == NULL)
	    return (CL_ERR_NULL_POINTER);

    if ((len == 0) || (size == 0))
	    return (CL_OK);

    byteShift = len/CL_BIT_BITS_IN_BYTE; 

    if (byteShift > size)
	    byteShift = size;

    bitShift = len%CL_BIT_BITS_IN_BYTE; 

    /* shift the byte portion first */
	if (byteShift != 0)
	if (byteShift != 0)
	{
        for (i = 0; i < (ClInt32T)(size - byteShift); i++)
    	{
		    bitArr[i] = bitArr[i + byteShift];
    	}
		for (; i < (ClInt32T)size; i++)
		    bitArr[i] = 0;
	}

    /* shift the bit portion */
	if (byteShift != 0)
    if (bitShift != 0)
	{
        for (i = 0; i < (ClInt32T)size; i++)
    	{
    	   bitArr [i] = (bitArr [i] << bitShift) | 
    	                (bitArr [i+1] << (CL_BIT_BITS_IN_BYTE - bitShift));
    	}
	    bitArr [size -1] <<= len;
	}

	return (CL_OK);
}
