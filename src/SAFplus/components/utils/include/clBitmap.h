/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : utils
File        : clBitmap.h
 *******************************************************************************/

/******************************************************************************
 * Description :
 *
 * OpenClovis Bitmap library header file.
 *
 *****************************************************************************/

#ifndef _CL_BITMAP_H_
#define _CL_BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <clOsalApi.h>

#define CL_OSAL_ERR_MUTEX_EBUSY     -1

#define CL_BM_BITS_IN_BYTE          8

#define CL_BM_BITS_IN_BYTE_SHIFT    (0x3)
#define CL_BM_BITS_IN_BYTE_MASK     ( (1 << CL_BM_BITS_IN_BYTE_SHIFT) - 1 )
#define CL_BM_BIT_UNDEF -1
#define CL_BM_BIT_CLEAR  0
#define CL_BM_BIT_SET    1

typedef struct
{
  ClUint32T       nBits;
  ClUint32T       nBytes;
  ClUint32T       nBitsSet;
  ClOsalMutexIdT  bitmapLock;
  ClUint8T        *pBitmap;
} ClBitmapInfoT;

typedef ClBitmapInfoT  *ClBitmapHandleT;
#define CL_BM_INVALID_BITMAP_HANDLE NULL

#ifdef __cplusplus
}
#endif

#endif  /* _CL_BITMAP_H_ */
