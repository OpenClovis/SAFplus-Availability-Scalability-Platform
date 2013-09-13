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

#ifndef _XDR_SANAMET_H_
#define _XDR_SANAMET_H_

#ifdef __cplusplus
extern "C" {
#endif

struct _SaNameT;
#include "clXdrApi.h"

#include "clIocApi.h"
#include "clAmsTypes.h"
#include "clCpmApi.h"
#include "clEoConfigApi.h"



ClRcT  clXdrMarshallSaNameT(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallSaNameT(ClBufferHandleT, void *);

#define clXdrMarshallArraySaNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(SaNameT), (multiplicity), clXdrMarshallSaNameT, (msg), (isDelete))

#define clXdrUnmarshallArraySaNameT(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(SaNameT), (multiplicity), clXdrUnmarshallSaNameT)

#define clXdrMarshallPtrSaNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(SaNameT), (multiplicity), clXdrMarshallSaNameT, (msg), (isDelete))

#define clXdrUnmarshallPtrSaNameT(msg,pointer) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(SaNameT), clXdrUnmarshallSaNameT)

#ifdef __cplusplus
}
#endif

#endif /*_XDR_SANAMET_H_*/

