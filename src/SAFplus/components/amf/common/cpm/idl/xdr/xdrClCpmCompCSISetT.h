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
*               and unmarshall funtions of ClCpmCompCSISetT 
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/

#ifndef _XDR_CL_CPM_COMP_CSI_SET_T_H_
#define _XDR_CL_CPM_COMP_CSI_SET_T_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clXdrApi.h"

#include "clCpmApi.h"
#include "clCpmIpi.h"
#include "clIocApi.h"
#include "clCpmExtApi.h"
#include "clEoConfigApi.h"
#include "clAmsTypes.h"
#include "xdrClAmsHAStateT.h"

struct _ClCpmCompCSISetT_4_0_0;

typedef struct _ClCpmCompCSISetT_4_0_0 {
    ClUint64T    invocation;
    SaNameT    compName;
    SaNameT    proxyCompName;
    SaNameT    nodeName;
    ClAmsHAStateT_4_0_0    haState;
    ClUint32T    bufferLength;
    ClUint8T*    buffer;

}ClCpmCompCSISetT_4_0_0;


ClRcT  clXdrMarshallClCpmCompCSISetT_4_0_0(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallClCpmCompCSISetT_4_0_0(ClBufferHandleT, void *);

#define clXdrMarshallArrayClCpmCompCSISetT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClCpmCompCSISetT_4_0_0), (multiplicity), clXdrMarshallClCpmCompCSISetT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallArrayClCpmCompCSISetT_4_0_0(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClCpmCompCSISetT_4_0_0), (multiplicity), clXdrUnmarshallClCpmCompCSISetT_4_0_0)

#define clXdrMarshallPointerClCpmCompCSISetT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(ClCpmCompCSISetT_4_0_0), (multiplicity), clXdrMarshallClCpmCompCSISetT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPointerClCpmCompCSISetT_4_0_0(msg,pointer) \
clXdrUnmarshallPointer((msg),(pointer), sizeof(ClCpmCompCSISetT_4_0_0), clXdrUnmarshallClCpmCompCSISetT_4_0_0)

#define clXdrMarshallPtrClCpmCompCSISetT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClCpmCompCSISetT_4_0_0), (multiplicity), clXdrMarshallClCpmCompCSISetT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPtrClCpmCompCSISetT_4_0_0(msg,pointer,multiplicity) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClCpmCompCSISetT_4_0_0),multiplicity, clXdrUnmarshallClCpmCompCSISetT_4_0_0)


typedef ClCpmCompCSISetT_4_0_0 ClCpmCompCSISetT;



#ifdef __cplusplus
}
#endif

#endif /*_XDR_CL_CPM_COMP_CSI_SET_T_H_*/
