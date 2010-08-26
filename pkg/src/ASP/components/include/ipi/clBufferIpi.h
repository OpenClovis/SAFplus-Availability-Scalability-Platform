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
/*This file can only be included from ioc kernel
  or buffer.c.
*/
#if !defined(_CL_BUFFER_C_) && !defined(__KERNEL__)
#error "clBufferIpi.h can be only included from client/clBuffer.c or IOC kernel"
#endif

#ifndef _CL_BUFFER_IPI_H_
#define _CL_BUFFER_IPI_H_

#define CL_BUFFER_RC(rc)  CL_RC(CL_CID_BUFFER,rc)

#define PREPEND_SPACE (512)

#define BUFFER_VALIDITY    0xB055

# define MARKER_VALIDITY     0xBABA

# define MARKER_INVALID_FOR_RESTORE 0x001

# define BM_TRUE 0x1

# define BM_FALSE 0x0

#define CL_POOL_ONE_KB_BUFFER               (1*1024)

#define CL_POOL_TWO_KB_BUFFER               (2*1024)

#define CL_POOL_FOUR_KB_BUFFER              (4*1024)

#define CL_POOL_EIGHT_KB_BUFFER             (8*1024)

#define BUFFER_VALIDITY_CHECK(X)   do {                   \
    if( !(X) ) return CL_BUFFER_RC(CL_ERR_NULL_POINTER);    \
    if( (X)->bufferValidity != BUFFER_VALIDITY) {         \
        return CL_BUFFER_RC(CL_ERR_INVALID_HANDLE);       \
    }                                                     \
}while(0)

#define NULL_CHECK(X)  do {                         \
    if(NULL == (X)) {                               \
        return CL_BUFFER_RC(CL_ERR_NULL_POINTER);   \
    }                                               \
}while(0)                                           \


# define MARKER_VALIDITY_CHECK(X)  do {                 \
    if( (X)->markerValidity != MARKER_VALIDITY)         \
    {                                                   \
        return CL_BUFFER_RC(CL_ERR_INVALID_HANDLE);     \
    }                                                   \
}while(0)

/*
  This is the header thats associated with each buffer
 */
typedef struct ClBufferHeader {
    ClUint32T startOffset; /* data start */
    ClUint32T dataLength; /* data end */
    ClUint32T readOffset; /* Offset to read from */
    ClUint32T writeOffset; /* offset to begin writing data */
    ClUint32T actualLength; /* size of entire buffer including the Buffer header and prepend space */
    ClUint32T chunkSize; /*the size of the chunk given out*/
    ClPoolT pool; /*the pool to which it belongs*/
    void *pCookie; /*metadata for freeing*/
    struct ClBufferHeader* pNextBufferHeader;
    struct ClBufferHeader* pPreviousBufferHeader;
}ClBufferHeaderT;

/*
  This is the ctrl header for the buffer thats hosted by the first buffer.
*/
typedef struct ClBufferCtrlHeader {
    ClUint32T bufferValidity;
    ClUint32T length; /*length of the message*/
    ClBufferHeaderT *pCurrentReadBuffer;
    ClBufferHeaderT *pCurrentWriteBuffer;
    ClUint32T currentReadOffset;
    ClUint32T currentWriteOffset;
} ClBufferCtrlHeaderT;

#endif
