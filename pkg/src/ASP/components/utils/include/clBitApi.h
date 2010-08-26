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
 * File        : clBitApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Clovis bit manipulation library header file.
 *
 *
 *****************************************************************************/

#ifndef _CL_BIT_API_H_
#define _CL_BIT_API_H_
                                                                                                       
#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.cbl */

/* FILES INCLUDED */
#ifndef  __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <clCommon.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/*
 * Most macros/function operate on a bit array. Bits in the bit array
 * are numbered starting from 0 to MAX_BITS-1. For example, in case of 64
 * bits array, first bit would be numbered 0 and the last one would be
 * numberred 63.
 */

#define CL_BIT_BITS_IN_BYTE     8
#define CL_BIT_BITS_IN_SHORT    16
#define CL_BIT_BITS_IN_WORD     32
#define CL_BIT_BITS_IN_DWORD    64

#define CL_BIT_BITS_IN_BYTE_SHIFT     3  /* 2^^3 */
#define CL_BIT_BITS_IN_SHORT_SHIFT    4  /* 2^^4 */
#define CL_BIT_BITS_IN_WORD_SHIFT     5  /* 2^^5 */

#define CL_BIT_WORD_ALL_ZERO    (0)
#define CL_BIT_WORD_ALL_ONE     (0xFFFFFFFF)

#define CL_BIT_BYTE_ALL_ZERO    (0)
#define CL_BIT_BYTE_ALL_ONE     (0xFF)

#define CL_BIT_LITTLE_ENDIAN    0
#define CL_BIT_BIG_ENDIAN       1

#define CL_BIT_SWAP64(x)  ((((x)>>56)&0xFF)|\
                         (((x)>>40)&0xFF00)|\
                         (((x)>>24)&0xFF0000)|\
                         (((x)>>8)&0xFF000000)|\
                         (((x)&0xFF000000)<<8)|\
                         (((x)&0xFF0000)<<24)|\
                         (((x)&0xFF00)<<40)|\
                         (((x)&0xFF)<<56))

#define CL_BIT_WORD_SWAP(x)  ((((x)>>32)&0xFFFFFFFF)|\
                              (((x)<<32)))

#define CL_BIT_SWAP32(x)  ((((x)>>24)&0xFF)|\
                         (((x)>>8)&0xFF00)|\
                         (((x)&0xFF00)<<8)|\
                         (((x)&0xFF)<<24))

#define CL_BIT_SWAP16(x)  ((((x)>>8)&0x00FF)|\
                         (((x)&0xFF)<<8))
/** Host to Network **/
#define CL_BIT_H2N64(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP64(x))
#define CL_BIT_H2N32(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP32(x))
#define CL_BIT_H2N16(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP16(x))

/** Network to Host **/
#define CL_BIT_N2H64(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP64(x))
#define CL_BIT_N2H32(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP32(x))
#define CL_BIT_N2H16(x)   ((clByteEndian==CL_BIT_BIG_ENDIAN)?x:CL_BIT_SWAP16(x))


/* Macro returns appropriate start byte address(for char,short), given the 
*  pointer to 32bit int.(takes Endianness into consideration.)
*  Mostly helpfull when used before downcasting.
*/
#define CL_BIT_GET_START_BYTE(ptr, sz)  ((clByteEndian==CL_BIT_BIG_ENDIAN)?((void *)ptr+(sizeof(ClInt32T)-sz)):ptr)

/* 
 * Defining the max bit field size to 25. This makes implementation of
 * set and get much more efficient and simple. Why 25? Because with 
 * 25 bits field we can gaurantee that it does not span more than
 * 4 bytes.
 */
#define CL_BIT_MAX_BIT_FIELD_SIZE (CL_BIT_BITS_IN_DWORD - CL_BIT_BITS_IN_BYTE + 1)

#define clBitOneW(bitNum)    (0x80000000 >> ((bitNum)))
#define clBitMaskW(size)     (CL_BIT_WORD_ALL_ONE >> (CL_BIT_BITS_IN_WORD - (size)))

#define clBitOneB(bitNum)    (0x80 >> ((bitNum)))
#define clBitMaskB(size)     (CL_BIT_BYTE_ALL_ONE >> (CL_BIT_BITS_IN_BYTE - (size)))

#define clBitGet(bitArr, bitNum)  ( \
     ((*(ClUint8T *)((bitArr) + ((bitNum)/CL_BIT_BITS_IN_BYTE)))\
	  >> (CL_BIT_BITS_IN_BYTE -1 - (bitNum & (CL_BIT_BITS_IN_BYTE -1)))) & 1 )

#define clBitSet(bitArr, bitNum)  ( \
                  (*((ClUint8T *)(bitArr) + ((bitNum)/CL_BIT_BITS_IN_BYTE))) |= \
				  (clBitOneB((bitNum) & (CL_BIT_BITS_IN_BYTE -1))) )

/* Global
*  clByteEndian - Variable to store the byte Endianness.
*               It has been defined here so that users do not have to explicitly
*		'extern' it. clByteEndian variable should NOT be written by any function other than
*		'clBitBlByteEndianGet'. It can only be READ to determine code logic according
*		to endianness.
*/

extern ClUint8T clByteEndian;  /*By Default, it is LITTLE_ENDIAN*/

/*****************************************************************************
 *  Functions
 *****************************************************************************/

extern ClUint8T clBitBlByteEndianGet(void);

extern ClRcT
clBitFieldGet (ClUint8T *bitArr,
                ClUint32T bitStart,
                ClInt32T len,
                ClUint64T *value);
extern ClRcT
clBitFieldSet (ClUint8T *bitArr,
                ClUint32T bitStart,
                ClUint32T len,
                ClUint64T value);
extern ClRcT 
clBitFieldRShift (ClUint8T *bitArr,
                   ClUint32T totalSize,
                   ClUint32T shiftBy);
extern ClRcT 
clBitFieldLShift (ClUint8T *bitArr,
                   ClUint32T totalSize,
                   ClUint32T shiftBy);

#ifdef __cplusplus
}
#endif
                                                                                                       
#endif  /* _CL_BIT_API_H_ */

