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


#ifndef _CL_GMS_SAF_API_H_
#define _CL_GMS_SAF_API_H_
                                                                                                                             
# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

#define isInitDone() ((0 == saGmsInitCount)? CL_FALSE : CL_TRUE)
#define isLastFinalize() ((0 == saGmsInitCount)? CL_TRUE : CL_FALSE)
#define SA_GMS_INIT_COUNT_INC() saGmsInitCount++
#define SA_GMS_INIT_COUNT_DEC() saGmsInitCount--
#define SA_GMS_CHECK_INIT_COUNT()\
    do{\
        if(0 == saGmsInitCount)\
        {\
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CLM Initialization not done \r\n"));\
                return SA_AIS_ERR_BAD_HANDLE;              \
        }\
    }while(0);

extern   ClUint32T   clGmsPrivateDataKey;
/* Static Funciton Declarations */
static SaAisErrorT _aspErrToAisErr(ClRcT clError);

static void saClmHandleInstanceDestructor(void* cbArgs);

static void clGmsClusterMemberGetCallbackWrapper (
        CL_IN const ClInvocationT         invocation,
        CL_IN const ClGmsClusterMemberT* const clusterMember,
        CL_IN const ClRcT                 rc);

static void clGmsClusterTrackCallbackWrapper (
        CL_IN const ClGmsClusterNotificationBufferT* const notificationBuffer,
        CL_IN const ClUint32T             numberOfMembers,
        CL_IN const ClRcT                 rc);

static void dispatchWrapperCallback (
        ClHandleT  localHandle,
        ClUint32T  callbackType,
        void*      callbackData);

static void  dispatchQDestroyCallback (
        ClUint32T callbackType,
        void*     callbackData);

/* Data structure declarations */

typedef struct SaClmInstance {
    SaClmCallbacksT     callbacks;
    ClHandleT           dispatchHandle;
} SaClmInstanceT;

typedef struct SaClmClusterNodeGetData {
    SaInvocationT invocation;
    SaClmClusterNodeT *clusterNode;
    SaAisErrorT rc;
} SaClmClusterNodeGetDataT;

typedef struct SaClmClusterTrackData {
    SaClmClusterNotificationBufferT *notificationBuffer;
    SaUint32T numberOfMembers;
    SaAisErrorT rc;
} SaClmClusterTrackDataT;


# ifdef __cplusplus
}
# endif /* __cplusplus */

#endif  /* ifndef _CL_GMS_SAF_API_H_ */
