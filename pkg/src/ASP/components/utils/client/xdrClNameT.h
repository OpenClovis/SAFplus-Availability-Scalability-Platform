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

#ifndef _XDR_CLNAMET_H_
#define _XDR_CLNAMET_H_

#ifdef __cplusplus
extern "C" {
#endif

struct _ClNameT;
#include "clXdrApi.h"

#include "clIocApi.h"
#include "clAmsTypes.h"
#include "clCpmApi.h"
#include "clEoConfigApi.h"



ClRcT  clXdrMarshallClNameT(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallClNameT(ClBufferHandleT, void *);

#define clXdrMarshallArrayClNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClNameT), (multiplicity), clXdrMarshallClNameT, (msg), (isDelete))

#define clXdrUnmarshallArrayClNameT(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClNameT), (multiplicity), clXdrUnmarshallClNameT)

#define clXdrMarshallPtrClNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClNameT), (multiplicity), clXdrMarshallClNameT, (msg), (isDelete))

#define clXdrUnmarshallPtrClNameT(msg,pointer) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClNameT), clXdrUnmarshallClNameT)

#ifdef __cplusplus
}
#endif

#endif /*_XDR_CLNAMET_H_*/

