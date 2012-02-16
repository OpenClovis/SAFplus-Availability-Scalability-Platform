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

#ifndef _XDR_CLMD5T_H_
#define _XDR_CLMD5T_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clXdrApi.h"

ClRcT clXdrMarshallClMD5T(void* pGenVar, ClBufferHandleT msg, ClUint32T isDelete);

ClRcT clXdrUnmarshallClMD5T(ClBufferHandleT msg , void* pGenVar);

#define clXdrMarshallPtrClMD5T(pointer, multiplicity, msg, isDelete)    \
clXdrMarshallPtr((pointer), sizeof(ClMD5T), \
                 (multiplicity), clXdrMarshallClMD5T, \
                 (msg), (isDelete))

#define clXdrUnmarshallPtrClMD5T(msg, pointer, multiplicity)  \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClMD5T), \
                   multiplicity, clXdrUnmarshallClMD5T)

#define clXdrMarshallArrayClMD5T(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClMD5T), (multiplicity), clXdrMarshallClMD5T, (msg), (isDelete))

#define clXdrUnmarshallArrayClMD5T(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClMD5T), (multiplicity), clXdrUnmarshallClMD5T)

#ifdef __cplusplus
}
#endif

#endif /*_XDR_CLMD5T_H_*/

