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
 * ModuleName  : amf
 * File        : clAmsInvocation.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements AMS invocation related methods.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clAmsInvocation.h>
#include <clAmsDefaultConfig.h>
#include <clAmsDatabase.h>
#include <clAmsServerUtils.h>
#include <clAmsModify.h>
#include <clCntApi.h>
#include <string.h>

#include <clCpmAms.h>
#include <clCpmCommon.h>
#define INVOCATION

/******************************************************************************
 * Global data structures and macros
 *****************************************************************************/

#define CL_AMS_INSTANCE_DELETE_ONE 1
#define CL_AMS_INSTANCE_DELETE_ALL 2

typedef struct ClAmsInvocationCSIWalk
{
    ClAmsCompT *comp;
    ClAmsCSIT  *csi;
    ClUint16T  pendingOp;
}ClAmsInvocationCSIWalkT;

static ClOsalMutexT gClAmsInvocationListMutex;

/*-----------------------------------------------------------------------------
 * Interface functions exposed to rest of AMS for managing invocations
 *---------------------------------------------------------------------------*/

static void
clAmsInvocationListDestroyCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data )
{
    ClAmsInvocationT *invocationData = (ClAmsInvocationT*)data;
#ifdef INVOCATION
    cpmInvocationDeleteInvocation(invocationData->invocation);
#endif
    clHeapFree((void *)data);
}

/*
 * clAmsInocationListInstantiate
 * -----------------------------
 * Create a new invocation list. Typically, we keep one invocation list per
 * cluster.
 */

ClRcT
clAmsInvocationListInstantiate(
        CL_INOUT  ClCntHandleT  *invocationList )
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!invocationList);

    clOsalMutexInit(&gClAmsInvocationListMutex);

    AMS_CALL( clCntLlistCreate(
                clAmsEntityDbEntityKeyCompareCallback,
                clAmsEntityListDeleteCallback,
                clAmsInvocationListDestroyCallback,
                CL_CNT_NON_UNIQUE_KEY,
                invocationList) );

    return CL_OK;

}

ClRcT
clAmsInvocationListTerminate(ClCntHandleT *invocationList)
{
    AMS_CHECKPTR(!invocationList);
    clCntDelete(*invocationList);
    *invocationList = 0;
    clOsalMutexDestroy(&gClAmsInvocationListMutex);
    return CL_OK;
}


ClRcT
clAmsInvocationListAddAll(CL_IN ClAmsInvocationT *invocationData)
{
    AMS_FUNC_ENTER (("\n"));

#ifdef INVOCATION

    ClUint32T cbType = 0;

    switch ( invocationData->cmd )
    {
        case CL_AMS_TERMINATE_CALLBACK:
        {
            cbType = CL_CPM_TERMINATE_CALLBACK;
            break;
        }

        case CL_AMS_PROXIED_INSTANTIATE_CALLBACK:
        {
            cbType = CL_CPM_PROXIED_INSTANTIATE_CALLBACK;
            break;
        }

        case CL_AMS_PROXIED_CLEANUP_CALLBACK:
        {
            cbType = CL_CPM_PROXIED_CLEANUP_CALLBACK;
            break;
        }

        case CL_AMS_CSI_RMV_CALLBACK:
        {
            cbType = CL_CPM_CSI_RMV_CALLBACK;
            break;
        }

        case CL_AMS_CSI_SET_CALLBACK:
        {
            cbType = CL_CPM_CSI_SET_CALLBACK;
            break;
        }

        case CL_AMS_CSI_QUIESCING_CALLBACK:
        {
            cbType = CL_CPM_CSI_QUIESCING_CALLBACK;
            break;
        }

        default:
        {
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }

    }

    AMS_CALL ( cpmInvocationAddKey(cbType, invocationData, invocationData->invocation, 
                                   CL_CPM_INVOCATION_AMS | CL_CPM_INVOCATION_DATA_COPIED) );

#endif

    AMS_CALL ( clAmsInvocationListAdd(gAms.invocationList, invocationData) );

    return CL_OK;
}

/*
 * clAmsInvocationCreate
 * ---------------------
 * Create a new invocation entry and return back the invocation id to the 
 * caller. AMS has its own invocation counter which is used if CPM is
 * not present. If CPM is present, it also keeps an invocation list and
 * then the invocation id created by CPM is used. In either case, the CSIs
 * affected by the new invocation are marked as such.
 */

ClRcT
clAmsInvocationCreateExtended(
        CL_IN   ClAmsInvocationCmdT cmd,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClBoolT reassignCSI,
        CL_OUT  ClInvocationT *invocation)
{
    ClAmsInvocationT *invocationData = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECK_COMP ( comp );

    invocationData = (ClAmsInvocationT*) clHeapCalloc (1, sizeof (ClAmsInvocationT));

    AMS_CHECK_NO_MEMORY ( invocationData );

    memcpy(&invocationData->compName, &comp->config.entity.name, sizeof (SaNameT));

    invocationData->cmd = cmd;
    invocationData->csi = csi;
    invocationData->reassignCSI = reassignCSI;
    if(csi)
    {
        memcpy(&invocationData->csiName, &csi->config.entity.name, sizeof(invocationData->csiName));
    }
    invocationData->csiTargetOne = csi ? CL_TRUE : CL_FALSE;
    invocationData->invocation = *invocation = ++gAms.invocationCount;

#ifdef INVOCATION

    ClUint32T cbType = 0;

    switch ( cmd )
    {
        case CL_AMS_TERMINATE_CALLBACK:
        {
            cbType = CL_CPM_TERMINATE_CALLBACK;
            break;
        }

        case CL_AMS_PROXIED_INSTANTIATE_CALLBACK:
        {
            cbType = CL_CPM_PROXIED_INSTANTIATE_CALLBACK;
            break;
        }

        case CL_AMS_PROXIED_CLEANUP_CALLBACK:
        {
            cbType = CL_CPM_PROXIED_CLEANUP_CALLBACK;
            break;
        }

        case CL_AMS_CSI_RMV_CALLBACK:
        {
            cbType = CL_CPM_CSI_RMV_CALLBACK;
            break;
        }

        case CL_AMS_CSI_SET_CALLBACK:
        {
            cbType = CL_CPM_CSI_SET_CALLBACK;
            break;
        }

        case CL_AMS_CSI_QUIESCING_CALLBACK:
        {
            cbType = CL_CPM_CSI_QUIESCING_CALLBACK;
            break;
        }

        default:
        {
            clAmsFreeMemory(invocationData);

            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }

    }

    AMS_CALL ( cpmInvocationAdd(cbType, invocationData, invocation, CL_CPM_INVOCATION_AMS | CL_CPM_INVOCATION_DATA_COPIED) );

    invocationData->invocation = *invocation;

#endif

    AMS_CALL ( clAmsInvocationListAdd(gAms.invocationList, invocationData) );

    if ( (cmd == CL_AMS_CSI_SET_CALLBACK) || (cmd == CL_AMS_CSI_RMV_CALLBACK) 
         || (cmd == CL_AMS_CSI_QUIESCING_CALLBACK) )
    {
        AMS_CALL ( clAmsInvocationUpdateCSI(comp, csi, cmd) );
    }

    return CL_OK;
}


ClRcT
clAmsInvocationCreate(
        CL_IN   ClAmsInvocationCmdT cmd,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_OUT  ClInvocationT *invocation)
{
    return clAmsInvocationCreateExtended(cmd, comp, csi, CL_FALSE, invocation);
}

/*
 * clAmsInvocationGet
 * ------------------
 * Return the data associated with an invocation id and keep the entry in
 * AMS and CPM invocation lists as-is. 
 */

ClRcT
clAmsInvocationGet(
        CL_IN       ClInvocationT invocation,
        CL_INOUT    ClAmsInvocationT *data)
{
    ClRcT rc;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!data);

#ifdef INVOCATION

    ClUint32T  cbType = 0;

    ClAmsInvocationT  *invocationData = NULL;

    clOsalMutexLock(&gClAmsInvocationListMutex);
    clOsalMutexLock(gpClCpm->invocationMutex);
    if ( (rc = cpmInvocationGetWithLock(invocation, &cbType, (void **)&invocationData)) != CL_OK )
    {
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
        return rc;
    }

    if ( invocationData )
    {
        memcpy(data, invocationData, sizeof(ClAmsInvocationT));
        invocationData->invocation = invocation;
    }
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    clOsalMutexUnlock(&gClAmsInvocationListMutex);

#else

    if ( (rc = clAmsCntFindInvocationElement(
                         gAms.invocationList,
                         invocation,
                         data)) )
    {
        return rc;
    }

#endif

    return CL_OK;
}

/*
 * clAmsInvocationGetAndDelete
 * ---------------------------
 * Return the data associated with an invocation id and delete the entry from
 * AMS and CPM invocation lists. The CSIs affected by removing the invocation
 * are marked as such.
 */

ClRcT
clAmsInvocationGetAndDeleteExtended(
                                    CL_IN       ClInvocationT invocation,
                                    CL_INOUT    ClAmsInvocationT *data,
                                    ClBoolT     invocationUpdate)
{
    ClRcT rc;
    ClAmsInvocationT  *invocationData = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!data);

#ifdef INVOCATION

    ClUint32T  cbType = 0;

    clOsalMutexLock(&gClAmsInvocationListMutex);
    clOsalMutexLock(gpClCpm->invocationMutex);
    if ( (rc = cpmInvocationGetWithLock(invocation, &cbType, (void **)&invocationData)) != CL_OK )
    {
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
        return rc;
    }

    if ( invocationData )
    {
        memcpy(data, invocationData, sizeof(ClAmsInvocationT));
        invocationData->invocation = invocation;
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
        clAmsInvocationListDelete(gAms.invocationList, data);
        invocationData = data;
    }

#ifdef POST_RC2
    else
    {
        /*
         * See if the invocation is present in the invocation list of
         * previous system controller
         */
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
        if ( (rc = clAmsCntFindInvocationElement(
                                                 gAms.invocationList,
                                                 invocation,
                                                 data)) )
        {
            return rc;
        }
        clAmsInvocationListDelete(gAms.invocationList, data);
        invocationData = data;
    }

#else

    else
    {
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
    }

#endif

#else /* ! INVOCATION */

    if ( (rc = clAmsCntFindInvocationElement(
                                             gAms.invocationList,
                                             invocation,
                                             data)) )
    {
        return rc;
    }

    clAmsInvocationListDelete(gAms.invocationList, data);

    invocationData = data;

#endif

    /*
     * Update the invocation flags for all CSIs affected by this invocation.
     * Note, if invocation was for instantiation or termination, all CSIs
     * will be marked as invocation cleared.
     */

    if ( invocationData )
    {
        if(invocationUpdate)
        {
            ClAmsCompT *comp = NULL;
            ClAmsCSIT *csi = NULL;
            ClAmsEntityRefT compRef = { {CL_AMS_ENTITY_TYPE_ENTITY} };

            compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
            memcpy (&compRef.entity.name, &invocationData->compName, sizeof (SaNameT));
            AMS_CALL ( clAmsEntityDbFindEntity(
                                               &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                               &compRef) );

            comp = (ClAmsCompT *)compRef.ptr;
            csi = invocationData->csi;

            AMS_CALL ( clAmsInvocationUpdateCSI(comp, csi, 0) );
        }
#ifdef INVOCATION
        clOsalMutexLock(&gClAmsInvocationListMutex);
        cpmInvocationDeleteInvocation(invocation);
        clOsalMutexUnlock(&gClAmsInvocationListMutex);
#endif
    }

    return CL_OK;
}

ClRcT
clAmsInvocationGetAndDelete(
        CL_IN       ClInvocationT invocation,
        CL_INOUT    ClAmsInvocationT *data)
{
    return clAmsInvocationGetAndDeleteExtended(invocation, data, CL_TRUE);
}

/*
 * clAmsInvocationDeleteAll
 * ------------------------
 * All invocations for a particular component are deleted. This is typically
 * called when the component terminates or a fault is reported on a node and
 * we know that a response will not be received for pending invocations.
 */

ClRcT
clAmsInvocationDeleteAll(
        CL_IN  ClAmsCompT  *comp)
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECK_COMP (comp);

#ifdef INVOCATION

    clOsalMutexLock(&gClAmsInvocationListMutex);
    cpmInvocationClearCompInvocation(&comp->config.entity.name);
    clOsalMutexUnlock(&gClAmsInvocationListMutex);

#endif

    clAmsInvocationListDeleteAll(
            gAms.invocationList,
            (ClCharT*)comp->config.entity.name.value);

    AMS_CALL ( clAmsInvocationUpdateCSI(comp, NULL, 0) );

    return CL_OK;

}

/*
 * clAmsInvocationListWalk
 * -----------------------
 * Walk the entity list for a component and invoke the function with the
 * invocation and other parameters as arguments. This fn is currently
 * only used for invocations related to CSIs, so the logic is tailored
 * for this need. 
 *
 */

ClRcT
clAmsInvocationListWalk(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT (*fn)(ClAmsCompT *c, ClInvocationT i, ClRcT e, ClUint32T s),
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode,
        CL_IN   ClAmsInvocationCmdT invocationCmd)
{
    ClAmsInvocationT  *i;
    ClCntNodeHandleT  nodeHandle;
    ClCntHandleT  listHandle = gAms.invocationList;
    ClInvocationT *pendingInvocations  = NULL;
    ClUint32T count = 0;
    ClUint32T j = 0; 
    ClRcT rc = CL_OK;

    /*
     * Make a list of invocations for this component that match the requested
     * invocation command. We make this list first as invoking fn can delete 
     * and add more invocation entries into the list.
     */

    for (i = (ClAmsInvocationT *) clAmsCntGetFirst (&listHandle, &nodeHandle); 
         i != NULL;
         i = (ClAmsInvocationT *) clAmsCntGetNext(&listHandle, &nodeHandle))
    {
        if ( !memcmp (i->compName.value,
                      comp->config.entity.name.value,
                      i->compName.length) )
        {
            if(!(count & 7))
            {
                pendingInvocations = (ClInvocationT*) clHeapRealloc(pendingInvocations, sizeof(*pendingInvocations) * (count + 8));
                CL_ASSERT(pendingInvocations != NULL);
            }

            if ( (!invocationCmd) || (i->cmd == invocationCmd) )
            {
                pendingInvocations[count++] = i->invocation;
            }
        }
    }

    for (j = 0; j < count; j++)
    {
        if( ( rc = fn(comp, pendingInvocations[j], error, switchoverMode) ) != CL_OK)
        {
            break;
        }
    }

    if(pendingInvocations) clHeapFree(pendingInvocations);

    return rc;
}

/*-----------------------------------------------------------------------------
 * Interface functions to update CSIs and check status of pending operations
 *---------------------------------------------------------------------------*/

/*
 * clAmsInvocationUpdateCSI
 * ------------------------
 * Mark/Clear a CSI as having a pending invocation.
 */

ClRcT
clAmsInvocationUpdateCSI(
        ClAmsCompT *comp,
        ClAmsCSIT *csi,
        ClUint32T command)
{
    ClAmsCompCSIRefT *csiRef;

    AMS_CHECK_COMP ( comp );

    if ( csi )
    {
        ClRcT rc;

        rc = clAmsEntityListFindEntityRef2(
                        &comp->status.csiList, 
                        &csi->config.entity,
                        0, 
                        (ClAmsEntityRefT **)&csiRef);

        if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            return rc;
        }

        csiRef->pendingOp = command;
    }
    else
    {
        ClAmsEntityRefT *entityRef;

        for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                                 entityRef) )
        {
            csiRef = (ClAmsCompCSIRefT *) entityRef;

            csiRef->pendingOp = command;
        }
    }

    return CL_OK;
}

/*
 * Returns the number of pending invocations in the component of that type.
 */

ClUint32T
clAmsInvocationPendingForComp(
                              ClAmsCompT *comp,
                              ClAmsInvocationCmdT pendingOp)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T count = 0;
    for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                             entityRef) )
    {
        ClAmsCompCSIRefT *csiRef;
        csiRef = (ClAmsCompCSIRefT *) entityRef;
        if ( csiRef->pendingOp == pendingOp)  ++count;
    }
    return count;
}

ClUint32T
clAmsInvocationSetPendingForComp(ClAmsCompT *comp)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T count = 0;
    for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                             entityRef) )
    {
        ClAmsCompCSIRefT *csiRef;
        csiRef = (ClAmsCompCSIRefT *) entityRef;
        if ( csiRef->pendingOp == CL_AMS_CSI_SET_CALLBACK
             ||
             csiRef->pendingOp == CL_AMS_CSI_QUIESCING_CALLBACK)  ++count;
    }
    return count;
}

/*
 * clAmsInvocationsPendingForComp
 * ------------------------------
 * Returns a count of the number of pending invocations for a component.
 */

ClUint32T
clAmsInvocationsPendingForComp(
    ClAmsCompT *comp)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T count = 0;

    for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                             entityRef) )
    {
        ClAmsCompCSIRefT *csiRef;

        csiRef = (ClAmsCompCSIRefT *) entityRef;

        if ( csiRef->pendingOp )
            count++;
    }
    
    return count;
}

ClUint32T
clAmsInvocationPendingForSU(ClAmsSUT *su, ClAmsInvocationCmdT cmd)
{
    ClAmsEntityRefT *ref = NULL;
    ClUint32T count = 0;
    for(ref = clAmsEntityListGetFirst(&su->config.compList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&su->config.compList, ref))
    {
        ClAmsCompT *comp = (ClAmsCompT*)ref->ptr;
        ClUint32T invocationsPending = 0;
        invocationsPending = clAmsInvocationPendingForComp(comp, cmd);
        count += invocationsPending;
    }
    return count;
}

ClUint32T
clAmsInvocationSetPendingForSU(ClAmsSUT *su)
{
    ClAmsEntityRefT *ref = NULL;
    ClUint32T count = 0;
    for(ref = clAmsEntityListGetFirst(&su->config.compList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&su->config.compList, ref))
    {
        ClAmsCompT *comp = (ClAmsCompT*)ref->ptr;
        ClUint32T invocationsPending = 0;
        invocationsPending = clAmsInvocationSetPendingForComp(comp);
        count += invocationsPending;
    }
    return count;
}

ClBoolT
clAmsInvocationPendingForCompCSI(ClAmsCompT *comp, ClAmsCSIT *csi, ClAmsInvocationCmdT op)
{
    ClAmsEntityRefT *eRef = NULL;
    for(eRef = clAmsEntityListGetFirst(&comp->status.csiList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&comp->status.csiList, eRef))
    {
        ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT*)eRef;
        ClAmsCSIT *pendingCSI = (ClAmsCSIT*)csiRef->entityRef.ptr;
        if(pendingCSI == csi 
           &&
           csiRef->pendingOp == op)
            return CL_TRUE;
    }
    return CL_FALSE;
}

ClBoolT 
clAmsInvocationPendingForCSI(ClAmsSUT *su, ClAmsCompT *comp, ClAmsCSIT *csi, ClAmsInvocationCmdT op)
{
    if(su)
    {
        ClAmsEntityRefT *eRef = NULL;
        for(eRef = clAmsEntityListGetFirst(&su->config.compList);
            eRef != NULL;
            eRef = clAmsEntityListGetNext(&su->config.compList, eRef))
        {
            ClAmsCompT *compRef = (ClAmsCompT*)eRef->ptr;
            if(clAmsInvocationPendingForCompCSI(compRef, csi, op))
                return CL_TRUE;
        }
        return CL_FALSE;
    }
    return clAmsInvocationPendingForCompCSI(comp, csi, op);
}

/*
 * clAmsInvocationsPendingForSU
 * ----------------------------
 * Returns a count of the number of pending invocations for a SU.
 */

ClUint32T
clAmsInvocationsPendingForSU(
    ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T count = 0;

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
        ClUint32T invocationsPending = 0;
        invocationsPending = clAmsInvocationsPendingForComp(comp);
        count += invocationsPending;
    }
    
    return count;
}

ClUint32T
clAmsInvocationsPendingForSG(
                             ClAmsSGT *sg)
{
    ClUint32T count = 0;
    ClAmsEntityRefT *ref = NULL;
    if(sg->status.adjustTimer.count > 0 ) return 1;
    if(sg->status.adjustProbationTimer.count > 0) return 1;
    for(ref = clAmsEntityListGetFirst(&sg->config.suList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&sg->config.suList, ref))
    {
        ClAmsSUT *su = (ClAmsSUT*)ref->ptr;
        count += clAmsInvocationsPendingForSU(su);
    }
    return count;
}                

ClUint32T 
clAmsInvocationsPendingForNode(
                               ClAmsNodeT *node)
{
    ClUint32T count = 0;
    ClAmsEntityRefT *ref = NULL;
    for(ref = clAmsEntityListGetFirst(&node->config.suList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&node->config.suList, ref))
    {
        ClAmsSUT *su = (ClAmsSUT*)ref->ptr;
        count += clAmsInvocationsPendingForSU(su);
    }
    return count;
}

ClUint32T
clAmsInvocationPendingForSI(ClAmsSIT *si)
{
    ClAmsSGT *sg = NULL;
    AMS_CHECK_SG ( sg = (ClAmsSGT*)si->config.parentSG.ptr);
    return clAmsInvocationsPendingForSG(sg);
}

/*
 * clAmsInvocationsPendingForSI
 * ----------------------------
 * Return a count of the number of pending invocations for a SI assignment.
 */

ClUint32T
clAmsInvocationsPendingForSI(
        ClAmsSIT *si,
        ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef, *entityRef2;
    ClUint32T count = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECK_SU ( su );

    for ( entityRef2 = clAmsEntityListGetFirst(&su->config.compList);
          entityRef2 != (ClAmsEntityRefT *) NULL;
          entityRef2 = clAmsEntityListGetNext(&su->config.compList, entityRef2) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef2->ptr;

        AMS_CHECK_COMP ( comp );

        for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                                 entityRef) )
        {
            ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
            ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;

            if ( csi->config.parentSI.ptr != (ClAmsEntityT *) si )
                continue;

            if ( csiRef->pendingOp )
                count++;
        }
    }

    return count;
}

ClUint32T
clAmsInvocationsSetPendingForSI(
        ClAmsSIT *si,
        ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef, *entityRef2;
    ClUint32T count = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECK_SU ( su );

    for ( entityRef2 = clAmsEntityListGetFirst(&su->config.compList);
          entityRef2 != (ClAmsEntityRefT *) NULL;
          entityRef2 = clAmsEntityListGetNext(&su->config.compList, entityRef2) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef2->ptr;

        AMS_CHECK_COMP ( comp );

        for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&comp->status.csiList,
                                                 entityRef) )
        {
            ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
            ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;

            if ( csi->config.parentSI.ptr != (ClAmsEntityT *) si )
                continue;

            if ( csiRef->pendingOp == CL_AMS_CSI_SET_CALLBACK
                 ||
                 csiRef->pendingOp == CL_AMS_CSI_QUIESCING_CALLBACK)
                ++count;
        }
    }

    return count;
}

/*-----------------------------------------------------------------------------
 * Internal functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsInvocationListAdd
 * ----------------------
 * Add a new invocation entry to an invocation list. This is typically called
 * when a new entry is created.
 */

ClRcT   
clAmsInvocationListAdd(
        CL_IN  ClCntHandleT  invocationList,
        CL_IN  ClAmsInvocationT  *invocationData)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!invocationData);

    /*
     * Create a copy of the invocation data as cpmInvocationList delete will also 
     * delete the node so we need to keep two copies
     */

    ClAmsInvocationT  *data = (ClAmsInvocationT*) clHeapAllocate (sizeof (ClAmsInvocationT));

    AMS_CHECK_NO_MEMORY ( data );

    memcpy (data, invocationData, sizeof (ClAmsInvocationT));

    clOsalMutexLock(&gClAmsInvocationListMutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clCntNodeAddAndNodeGet(
                                                                invocationList, 
                                                                0,
                                                                (ClCntDataHandleT)data,
                                                                NULL,
                                                                &nodeHandle),
                                         &gClAmsInvocationListMutex );

    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    exitfn:
    return rc;
}

/*
 * clAmsInvocationListDeleteAll
 * ----------------------------
 * Delete all invocations for a component.
 */

ClRcT   
clAmsInvocationListDeleteAll(
        CL_IN  ClCntHandleT  invocationList,
        CL_IN  ClCharT  *compName)
{
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!compName); 
    
    clOsalMutexLock(&gClAmsInvocationListMutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsCntDeleteElement(
                                                               invocationList,
                                                               (ClCntDataHandleT )compName,
                                                               0,
                                                               clAmsCntMatchInvocationInstanceName,
                                                               CL_AMS_INSTANCE_DELETE_ALL),
                                         &gClAmsInvocationListMutex);
    
    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    exitfn:
    return rc;
}

/*
 * clAmsInvocationListDelete
 * -------------------------
 * Delete a particular invocation entry from the invocation list
 */

ClRcT   
clAmsInvocationListDelete(
        CL_IN  ClCntHandleT invocationList,
        CL_IN  ClAmsInvocationT  *invocationData)
{
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!invocationData);

    clOsalMutexLock(&gClAmsInvocationListMutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clAmsCntDeleteElement(
                                                              invocationList,
                                                              (ClCntDataHandleT )invocationData,
                                                              0,
                                                              clAmsCntMatchInvocationInstance,
                                                              CL_AMS_INSTANCE_DELETE_ONE),
                                        &gClAmsInvocationListMutex);

    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    exitfn:
    return rc;
}

/*
 * clAmsInvocationListDeleteAllInvocations
 * ---------------------------------------
 * Delete all the invocation entries in a list.
 */

ClRcT
clAmsInvocationListDeleteAllInvocations(
        ClCntHandleT  listHandle)
{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    ClAmsInvocationT  *invocationData = NULL;
    ClAmsInvocationT *nextInvocationData = NULL;
    ClCntNodeHandleT  nodeHandle =0;

    clOsalMutexLock(&gClAmsInvocationListMutex);

    for ( invocationData = (ClAmsInvocationT *)
            clAmsCntGetFirst( &listHandle,&nodeHandle); 
            invocationData!= NULL;
          invocationData = nextInvocationData )
    {

        nextInvocationData = (ClAmsInvocationT *) clAmsCntGetNext( &listHandle,
                                                                   &nodeHandle );
#ifdef INVOCATION

         rc = cpmInvocationClearCompInvocation(&invocationData->compName);

         if ((rc != CL_OK) && (CL_GET_ERROR_CODE (rc) != CL_ERR_NOT_EXIST))
         {
             clOsalMutexUnlock(&gClAmsInvocationListMutex);
             return CL_AMS_RC (rc);
         }

#endif

         rc = clAmsCntDeleteElement(
                        listHandle,
                        (ClCntDataHandleT )invocationData,
                        0,
                        clAmsCntMatchInvocationInstance,
                        CL_AMS_INSTANCE_DELETE_ONE);

         if ((rc != CL_OK) && (CL_GET_ERROR_CODE (rc) != CL_ERR_NOT_EXIST))
         {
             clOsalMutexUnlock(&gClAmsInvocationListMutex);
             return CL_AMS_RC (rc);
         }
    }

    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Internal container manipulation functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsCntDeleteElement
 * ---------------------
 * Delete a given element from the clovis container list
 *
 * listHandle          - handle to the clovis container list
 * keyHandle           - key of the element to be deleted
 * element             - element which needs to be deleted  
 * clCntDeleteCallback - callback function to compare the list element
 * flag                - should all matching elements should be deleted?
 */


ClRcT
clAmsCntDeleteElement(
        CL_IN  ClCntHandleT listHandle,
        CL_IN  ClCntDataHandleT element ,
        CL_IN  ClCntKeyHandleT keyHandle, 
        CL_IN  ClRcT (*clCntDeleteCallback) (
                        ClCntDataHandleT element, 
                        ClCntDataHandleT data, 
                        ClBoolT *match),
        CL_IN  ClUint32T flag )

{
    ClCntNodeHandleT  nodeHandle = 0;
    ClCntNodeHandleT  nextNodeHandle = 0;
    ClCntDataHandleT  data = 0;
    ClBoolT  match = CL_FALSE;
    ClBoolT  found = CL_FALSE;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!clCntDeleteCallback);

    /*
     * Find the actual element to be deleted from the list of all elements
     * with the same key. 
     */
    
    data = clAmsCntGetFirst(&listHandle,&nodeHandle);

    while ( data != 0 )
    {

        AMS_CALL ( (*clCntDeleteCallback)(element, data, &match) );

        nextNodeHandle = nodeHandle;

        data = clAmsCntGetNext(&listHandle, &nextNodeHandle);

        if ( match == CL_TRUE )
        { 
            
            AMS_CALL ( clCntNodeDelete(listHandle, nodeHandle) ); 
            
            found = CL_TRUE; 
            
            if ( flag == CL_AMS_INSTANCE_DELETE_ONE )
            {
                return CL_OK;
            } 
        } 
        
        nodeHandle = nextNodeHandle; 
    }

    if ( !found )
    {
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    return CL_OK;
}

/*
 * clAmsCntMatchInvocationInstance
 * -------------------------------
 * Compare whether the container invocation matches the given element
 *
 * element                     - element which needs to be compared  
 * dataElement                 - element against which it needs to be compared
 * match                       - whether the two element match
 */

ClRcT
clAmsCntMatchInvocationInstance(
        CL_IN  ClCntDataHandleT  element,
        CL_IN  ClCntDataHandleT  dataElement,
        CL_OUT  ClBoolT  *match)
{
    ClAmsInvocationT  *src = NULL;
    ClAmsInvocationT  *target  = NULL;

    AMS_FUNC_ENTER (("\n"));

    src =    (ClAmsInvocationT *) element;
    target = (ClAmsInvocationT *) dataElement;

    AMS_CHECKPTR ( !match||!src||!target );

    *match = CL_FALSE;

    if ( !memcmp (src, target, sizeof (ClAmsInvocationT)) )
    {
        *match = CL_TRUE;
    }

    return CL_OK;
}

/*
 * clAmsCntMatchInvocationInstanceName
 * -----------------------------------
 * Compare whether the container invocation name matches the give element
 *
 * element                     - element which needs to be compared  
 * dataElement                 - element against which it needs to be compared
 * match                       - whether the two element match
 */

ClRcT
clAmsCntMatchInvocationInstanceName(
        CL_IN  ClCntDataHandleT  element,
        CL_IN  ClCntDataHandleT  dataElement,
        CL_OUT  ClBoolT  *match)
{
    ClCharT  *srcName = NULL;
    ClAmsInvocationT  *target  = NULL;

    AMS_FUNC_ENTER (("\n"));

    srcName = (ClCharT *) element;
    target = (ClAmsInvocationT *) dataElement;

    AMS_CHECKPTR ( !srcName || !target || !match );

    *match = CL_FALSE;

    if ( !strcmp (srcName,(const ClCharT*)target->compName.value) )
    {
        *match = CL_TRUE;
    }

    return CL_OK;
}

/*
 * clAmsCntFindInvocationElement
 * -----------------------------
 * Find an element with given invocation id
 *
 * listHandle                  - cnt handle for invocation list
 * invocation                  - invocation id of the element to be found  
 * invocationData              - invocation data associated with the invocation
 */

ClRcT
clAmsCntFindInvocationElement(
        CL_IN  ClCntHandleT  listHandle,    
        CL_IN  ClInvocationT  invocation,
        CL_OUT  ClAmsInvocationT  *invocationData )
{ 
    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsInvocationT  *data = NULL;                

    AMS_FUNC_ENTER(("\n"));

    AMS_CHECKPTR (!invocationData);

    clOsalMutexLock(&gClAmsInvocationListMutex);

    for ( data = (ClAmsInvocationT *)
            clAmsCntGetFirst(&listHandle,&nodeHandle);
            data != NULL;
            data = (ClAmsInvocationT *)
            clAmsCntGetNext(&listHandle,&nodeHandle) )
    {
        AMS_CHECKPTR ( !data )

        if ( data->invocation == invocation )
        {
            memcpy(invocationData, data, sizeof(*invocationData));
            clOsalMutexUnlock(&gClAmsInvocationListMutex);
            return CL_OK;
        } 
    }

    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    return CL_AMS_RC (CL_ERR_NOT_EXIST);
}

/*
 * clAmsInvocationListWalkAll
 * -----------------------------
 * Traverse all the invocation list entries and invoke the callback function.
 *
 * pCallback-   Pointer to the callback function for each invocation entry
 */

ClRcT
clAmsInvocationListWalkAll(ClAmsInvocationCallbackT callback,ClPtrT arg,ClBoolT continueOnFailure)
{
    ClCntHandleT listHandle = gAms.invocationList;
    ClAmsInvocationT *invocationDataList = NULL;
    ClAmsInvocationT *invocationData = NULL;
    ClCntNodeHandleT nodeHandle = 0;
    ClUint32T numInvocations = 0;
    ClUint32T i;
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER(("\n"));

    AMS_CHECKPTR(!callback);

    clOsalMutexLock(&gClAmsInvocationListMutex);
    for(invocationData = 
            (ClAmsInvocationT*)clAmsCntGetFirst(&listHandle,&nodeHandle);
        invocationData != NULL;
        invocationData = (ClAmsInvocationT*)clAmsCntGetNext(&listHandle, &nodeHandle) )
    {
        if(!(numInvocations & 7))
        {
            invocationDataList = (ClAmsInvocationT*) clHeapRealloc(invocationDataList, sizeof(*invocationDataList) * (numInvocations + 8));
            CL_ASSERT(invocationDataList != NULL);
            memset(invocationDataList + numInvocations, 0, sizeof(*invocationDataList)*8);
        }
        memcpy(&invocationDataList[numInvocations], invocationData, sizeof(*invocationDataList));
        ++numInvocations;
    }
    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    for(i = 0; i < numInvocations; ++i)
    {
        /*
         * Its okay to have callback failures
         */
        if((rc = callback(&invocationDataList[i], arg)) != CL_OK)
        {
            clLogWarning("INVOCATION", "WALK", 
                         "Invocation Callback for invocation [%#llx] returned [%#x]",
                         invocationDataList[i].invocation, rc);
            if(continueOnFailure == CL_FALSE)
            {
                goto error;
            }
        }
    }

    rc = CL_OK;

    error:
    if(invocationDataList)
        clHeapFree(invocationDataList);

    return rc;
}

ClRcT
clAmsInvocationFindCSI(ClAmsCompT *comp,
                       ClAmsCSIT *csi,
                       ClUint16T pendingOp,
                       ClAmsInvocationT *invocationData
                       )
{
    ClCntHandleT list = gAms.invocationList;
    ClCntNodeHandleT nodeHandle=0;
    ClAmsInvocationT *data = NULL;

    AMS_FUNC_ENTER(("\n"));
    
    AMS_CHECK_COMP(comp);

    clOsalMutexLock(&gClAmsInvocationListMutex);

    for(data = (ClAmsInvocationT*)clAmsCntGetFirst(&list,&nodeHandle);
        data != NULL;
        data = (ClAmsInvocationT*)clAmsCntGetNext(&list,&nodeHandle))
    {
        if(!memcmp(data->compName.value,
                   comp->config.entity.name.value,
                   data->compName.length)
           &&
           csi == data->csi
           &&
           pendingOp == data->cmd
           )
        {
            if(NULL != invocationData)
            {
                memcpy(invocationData,data,sizeof(*invocationData));
            }
            clOsalMutexUnlock(&gClAmsInvocationListMutex);
            return CL_OK;
        }
    }
    
    clOsalMutexUnlock(&gClAmsInvocationListMutex);

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * Update all invocation CSI stale entries after a DB syncup in hot standby context.
 */
ClRcT
clAmsInvocationListUpdateCSIAll(ClBoolT updateCSI)
{
    ClAmsInvocationT *data = NULL;
    ClAmsInvocationT *next = NULL;
    ClCntHandleT list = gAms.invocationList;
    ClCntNodeHandleT nodeHandle = 0;
    ClRcT rc = CL_OK;

    clOsalMutexLock(&gClAmsInvocationListMutex);
    for(data = (ClAmsInvocationT*)clAmsCntGetFirst(&list, &nodeHandle);
        data != NULL;
        data = next)
    {
        ClAmsEntityRefT entityRef = {{CL_AMS_ENTITY_TYPE_ENTITY}};
        ClUint32T cbType = 0;
        ClAmsInvocationT *cpmInvocation = NULL;

        next = (ClAmsInvocationT*)clAmsCntGetNext(&list, &nodeHandle);

        if(data->cmd != CL_AMS_CSI_SET_CALLBACK &&
           data->cmd != CL_AMS_CSI_RMV_CALLBACK &&
           data->cmd != CL_AMS_CSI_QUIESCING_CALLBACK)
            continue;

        if(data->cmd == CL_AMS_CSI_SET_CALLBACK
           &&
           !data->csi)
        {
            /*
             * Ignore CSI reassign all or TARGET all invocations. for 2N
             */
            continue;
        }
        entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;
        memcpy(&entityRef.entity.name, &data->csiName,
               sizeof(entityRef.entity.name));
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                                     &entityRef);
        /*
         * Delete the stale invocation.
         */
        if(rc != CL_OK)
        {
            cpmInvocationDeleteInvocation(data->invocation);
            clOsalMutexUnlock(&gClAmsInvocationListMutex);
            clLogInfo("INVOCATION", "AUDIT",
                      "Deleting stale invocation [%#llx] from AMS for CSI [%s]",
                      data->invocation, entityRef.entity.name.length > 0 ? 
                                      (const ClCharT*)entityRef.entity.name.value : "");
            if(clAmsInvocationListDelete(gAms.invocationList,
                                         data) != CL_OK)
            {
                data->csi = NULL;
            }
            clOsalMutexLock(&gClAmsInvocationListMutex);
            continue;
        }
        
        if(updateCSI)
        {
            data->csi = (ClAmsCSIT*)entityRef.ptr;
            /*
             * Update in the CPM invocation list.
             */
            clOsalMutexLock(gpClCpm->invocationMutex);
            if( (rc = cpmInvocationGetWithLock(data->invocation, &cbType,  (void**)&cpmInvocation)) != CL_OK )
            {
                clOsalMutexUnlock(gpClCpm->invocationMutex);
                clOsalMutexUnlock(&gClAmsInvocationListMutex);
                return rc;
            }
            if(cpmInvocation)
                cpmInvocation->csi = (ClAmsCSIT*)entityRef.ptr;
            clOsalMutexUnlock(gpClCpm->invocationMutex);
        }
        
    }
    clOsalMutexUnlock(&gClAmsInvocationListMutex);
    return CL_OK;
}

void clAmsInvocationLock(void)
{
    clOsalMutexLock(&gClAmsInvocationListMutex);
}

void clAmsInvocationUnlock(void)
{
    clOsalMutexUnlock(&gClAmsInvocationListMutex);
}
