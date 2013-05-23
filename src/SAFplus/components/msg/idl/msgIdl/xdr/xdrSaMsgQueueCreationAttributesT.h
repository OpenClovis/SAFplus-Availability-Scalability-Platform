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


/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description : This file contains the declarations for marshall 
*               and unmarshall funtions of SaMsgQueueCreationAttributesT 
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/

#ifndef _XDR_SA_MSG_QUEUE_CREATION_ATTRIBUTES_T_H_
#define _XDR_SA_MSG_QUEUE_CREATION_ATTRIBUTES_T_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clXdrApi.h"

#include "clMsgCommon.h"
#include "clMsgCkptData.h"
#include "clIocApi.h"
#include "clCommon.h"
#include "saMsg.h"
#include "saAis.h"

struct _SaMsgQueueCreationAttributesT_4_0_0;


ClRcT  clXdrMarshallSaMsgQueueCreationAttributesT_4_0_0(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallSaMsgQueueCreationAttributesT_4_0_0(ClBufferHandleT, void *);

#define clXdrMarshallArraySaMsgQueueCreationAttributesT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0), (multiplicity), clXdrMarshallSaMsgQueueCreationAttributesT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallArraySaMsgQueueCreationAttributesT_4_0_0(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0), (multiplicity), clXdrUnmarshallSaMsgQueueCreationAttributesT_4_0_0)

#define clXdrMarshallPointerSaMsgQueueCreationAttributesT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0), (multiplicity), clXdrMarshallSaMsgQueueCreationAttributesT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPointerSaMsgQueueCreationAttributesT_4_0_0(msg,pointer) \
clXdrUnmarshallPointer((msg),(pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0), clXdrUnmarshallSaMsgQueueCreationAttributesT_4_0_0)

#define clXdrMarshallPtrSaMsgQueueCreationAttributesT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0), (multiplicity), clXdrMarshallSaMsgQueueCreationAttributesT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPtrSaMsgQueueCreationAttributesT_4_0_0(msg,pointer,multiplicity) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(SaMsgQueueCreationAttributesT_4_0_0),multiplicity, clXdrUnmarshallSaMsgQueueCreationAttributesT_4_0_0)


typedef SaMsgQueueCreationAttributesT SaMsgQueueCreationAttributesT_4_0_0;



#ifdef __cplusplus
}
#endif

#endif /*_XDR_SA_MSG_QUEUE_CREATION_ATTRIBUTES_T_H_*/