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

/**
 * This file implements client side functionality related to SAF.
 */

/*
 * Standard header files 
 */
#define __SERVER__
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCpmApi.h>
#include <clCpmErrors.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clAmsTypes.h>
#include <clHandleApi.h>
#include <clVersionApi.h>
#include <clAmsMgmtClientApi.h>
/*
 * Internal header files
 */
#include <clEoIpi.h>
#include <clHandleIpi.h>

/*
 * CPM internal header files 
 */
#include <clCpmClient.h>
#include <clCpmCommon.h>
#include <clCpmLog.h>
#include <clCpmInternal.h>
/*
 * XDR header files 
 */
#include "xdrClCpmCompInitSendT.h"
#include "xdrClCpmCompInitRecvT.h"
#include "xdrClCpmCompFinalizeSendT.h"
#include "xdrClCpmCompRegisterT.h"
#include "xdrClCpmCompCSISetT.h"
#include "xdrClCpmCompCSIRmvT.h"
#include "xdrClCpmResponseT.h"
#include "xdrClCpmPGTrackT.h"
#include "xdrClCpmPGResponseT.h"
#include "xdrClCpmPGTrackStopT.h"
#include "xdrClCpmHAStateGetSendT.h"
#include "xdrClCpmHAStateGetRecvT.h"
#include "xdrClCpmQuiescingCompleteT.h"
#include "xdrClCpmClientCompTerminateT.h"
#include "xdrClAmsCSIStateDescriptorT.h"
#include "xdrClCpmCSIDescriptorNameValueT.h"
#include "xdrClErrorReportT.h"
#include "xdrClAmsPGNotificationT.h"
#include "xdrClCpmCompHealthcheckT.h"

extern ClUint32T   clEoWithOutCpm;

ClCpmHandleT clCpmHandle = 0;

/**
 * SAF has the mechanism of responding to the callback by a different
 * call clCpmResponse. That call can be invoked from a different
 * thread of control. In case of component terminate, we have to do EO
 * cleanup, ioc unblock etc. for a SA-Aware component, but
 * clCpmResponse does not have any mechanism by which we can figure
 * out the component for which terminate request was invoked. Hence,
 * when we receive the terminate request for the SA-Aware callback, we
 * will make this flag CL_TRUE, which will be checked when the
 * clCpmResponse is called.
 */
ClBoolT componentTerminate = CL_FALSE;

/*
 * Flag to show if library is initialized
 */

static ClBoolT lib_initialized = CL_FALSE;

/*
 * Versions supported
 */

static ClVersionT versions_supported[] =
{
    {'B', 0x01, 0x01},
    {'B', 0x02, 0x00},
};

static ClVersionDatabaseT version_database =
{
    sizeof(versions_supported) / sizeof(ClVersionT),
    versions_supported
};

static ClCntHandleT gCpmCompCSIListHandle;
static ClOsalMutexIdT gCpmCompCSIListMutex;

/* 
 * Cached csi descriptors maintaining haState transitions
 */
typedef struct ClCpmCompCSIList
{
    ClNameT compName;
    ClCpmHandleT cpmHandle;
    ClCntHandleT csiList;
} ClCpmCompCSIListT;

typedef struct ClCpmCompCSIWalk
{
    ClAmsCSIDescriptorT *pCSIDescriptor;
    ClAmsHAStateT haState;
    ClBoolT haStateUpdated;
}ClCpmCompCSIWalkT;

/*
 * Context for a AMF "instance" - i.e. one user of AMS service
 */
typedef struct
{
    /*
     * User's callback functions
     */
    ClCpmCallbacksT callbacks;

    /*
     * Indicate that finalize has been called
     */
    ClBoolT finalize;
    ClOsalMutexIdT response_mutex;

    /*
     * write FD, this is the fd where write activity needs to be done
     */
    ClInt32T writeFd;

    /*
     * read FD, this is the fd where read activity needs to be done
     */
    ClInt32T readFd;

    /*
     * Queue on which the callback needs to be queued
     */
    ClQueueT cbQueue;

    /*
     * Protection of the cbQueue
     */
    ClOsalMutexIdT cbMutex;

    /*
     * Dispatch type being invoked by the application is stored in the context
     */
    ClDispatchFlagsT dispatchType;

    /*
     * The healthcheck key to be passed in the health check callback.
     */
    ClAmsCompHealthcheckKeyT healthcheckKey;
} ClCpmInstanceT;

typedef enum
{
    CL_CPM_FINALIZE             = 0,
    CL_CPM_CB_HEALTHCHECK       = 1,
    CL_CPM_CB_TERMINATE         = 2,
    CL_CPM_CB_CSI_SET           = 3,
    CL_CPM_CB_CSI_RMV           = 4,
    CL_CPM_CB_PROTECTION_TRACK  = 5,
    CL_CPM_CB_PROXIED_INST      = 6,
    CL_CPM_CB_PROXIED_CLEANUP   = 7,
} ClCpmCallbackTypeT;

typedef struct
{
    ClCpmCallbackTypeT cbType;
    void *params;
} ClCpmCallbackQueueDataT;

/*
 * SAF AMF related function callback
 */
static ClRcT VDECL(_clCpmClientCompHealthCheck)(ClEoDataT eoArg,
                                         ClBufferHandleT inMsgHandle,
                                         ClBufferHandleT outMsgHandle);

static ClRcT VDECL(_clCpmClientCompTerminate)(ClEoDataT eoArg,
                                       ClBufferHandleT inMsgHandle,
                                       ClBufferHandleT outMsgHandle);

static ClRcT VDECL(_clCpmClientCompCSISet)(ClEoDataT eoArg,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);
                                                                                                                        static ClRcT VDECL(_clCpmClientCompCSIRmv)(ClEoDataT eoArg,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

static ClRcT VDECL(_clCpmClientCompProxiedComponentInstantiate)(ClEoDataT eoArg,
                                            ClBufferHandleT inMsgHandle,
                                            ClBufferHandleT outMsgHandle);

static ClRcT VDECL(_clCpmClientCompProxiedComponentCleanup)(ClEoDataT eoArg,
                                        ClBufferHandleT inMsgHandle,
                                        ClBufferHandleT outMsgHandle);

static ClRcT VDECL(_clCpmClientCompProtectionGroupTrack)(ClEoDataT eoArg,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle);

#include <clCpmClientFuncList.h>

/*
 * Flag to indicate that the library is already initialized.
 */
static ClUint32T clClientInitialized = 0;

/*
 * Instance handle database
 */
static ClHandleDatabaseHandleT handle_database;

/*
 * Component-Handle Mapping Table
 */
typedef struct component_handle_table
{
    ClHandleT amfHandle;
    ClNameT componentName;
    struct component_handle_table *next;
} component_handle_table_t;

static component_handle_table_t *component_handle_mapping_db;

static ClRcT component_handle_mapping_create(CL_IN const ClNameT *compName,
                                             CL_IN ClHandleT amfHandle)
{
    ClRcT rc = CL_OK;
    component_handle_table_t *temp = NULL;
    component_handle_table_t *nextPtr = NULL;
    component_handle_table_t *currPtr = NULL;

    temp =
        (struct component_handle_table *)
        clHeapAllocate(sizeof(struct component_handle_table));
    if (temp == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    }

    /*
     * Populate the node in the list 
     */
    temp->amfHandle = amfHandle;
    memcpy(&(temp->componentName), compName, sizeof(ClNameT));
    temp->next = NULL;

    /*
     * If first element 
     */
    if (component_handle_mapping_db == NULL)
        component_handle_mapping_db = temp;
    else
    {
        /*
         * If not the first element 
         */
        currPtr = component_handle_mapping_db;
        nextPtr = component_handle_mapping_db->next;
        while (nextPtr != NULL)
        {
            /*
             * Compare the name 
             */
            if (compName->length == (currPtr->componentName).length &&
                !(strcmp(compName->value, (currPtr->componentName).value)))
            {
                rc = CL_CPM_RC(CL_CPM_ERR_EXIST);
                if (rc != CL_OK)
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG,
                               CL_CPM_CLIENT_LIB,
                               CL_CPM_LOG_1_LCM_MULT_REG_REQ_ERR, compName->value);
                }
                goto failure;
            }
            currPtr = nextPtr;
            nextPtr = currPtr->next;
        }
        /*
         * Add the new entry at the end 
         */
        currPtr->next = temp;
    }

    return rc;

  failure:
    if (temp != NULL)
        clHeapFree(temp);
    return rc;
}

static ClRcT component_handle_mapping_get(CL_IN ClNameT *compName,
                                          CL_OUT ClHandleT *amfHandle)
{

    ClRcT rc = CL_OK;
    component_handle_table_t *currPtr = NULL;
    ClUint32T found = 0;

    currPtr = component_handle_mapping_db;
    while (currPtr != NULL)
    {
        if (compName->length == (currPtr->componentName).length &&
            !(strcmp(compName->value, (currPtr->componentName).value)))
        {
            found = 1;
            break;
        }
        currPtr = currPtr->next;
    }

    if (found == 1 && currPtr != NULL)
    {
        *amfHandle = currPtr->amfHandle;
        rc = CL_OK;
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_EXIST);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_RESOURCE_NON_EXISTENT);
    }
    return rc;
}

static ClRcT component_handle_mapping_delete(CL_IN const ClNameT *compName,
                                             CL_IN ClHandleT amfHandle)
{
    ClRcT rc = CL_OK;
    component_handle_table_t *prevPtr = NULL;
    component_handle_table_t *currPtr = NULL;
    ClUint32T found = 0;

    prevPtr = currPtr = component_handle_mapping_db;

    while (currPtr != NULL)
    {
        if (compName->length == (currPtr->componentName).length &&
            !(strcmp(compName->value, (currPtr->componentName).value)))
        {
            if (memcmp(&(amfHandle), &(currPtr->amfHandle), sizeof(ClHandleT)))
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                           CL_CPM_LOG_2_HANDLE_INVALID, compName->value,
                           CL_CPM_RC(CL_ERR_INVALID_HANDLE));
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                                 ("passed handle was not for registering the given component %s",
                                  compName->value),
                                 CL_CPM_RC(CL_ERR_INVALID_HANDLE));
            }
            /*
             * Found the entry so delete it 
             */
            /*
             * If first entry 
             */
            if (component_handle_mapping_db == currPtr)
            {
                component_handle_mapping_db = currPtr->next;
                clHeapFree(currPtr);
                currPtr = NULL;
            }
            else
            {
                prevPtr->next = currPtr->next;
                clHeapFree(currPtr);
                currPtr = NULL;
            }

            found = 1;
            break;
        }
        prevPtr = currPtr;
        currPtr = currPtr->next;
    }

    if (found == 1)
        rc = CL_OK;
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_EXIST);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_RESOURCE_NON_EXISTENT);
    }

  failure:
    return rc;
}

static ClInt32T clCpmNameKeyCmp(ClNameT *pNameA,ClNameT *pNameB)
{
    ClInt32T cmp = pNameA->length - pNameB->length;
    if(cmp)
    {
        return cmp;
    }
    return strncmp(pNameA->value,pNameB->value,pNameA->length);

}

static ClInt32T cpmCompCSIListKeyCmp(ClCntKeyHandleT key1,
                                     ClCntKeyHandleT key2)
{
    return clCpmNameKeyCmp((ClNameT*)key1,(ClNameT*)key2);
}

static void cpmCompCSIListDeleteCallback(ClCntKeyHandleT key,
                                         ClCntDataHandleT data)
{
    ClCpmCompCSIListT *pCompCSIList = (ClCpmCompCSIListT*)data;
    clCntDelete(pCompCSIList->csiList);
    clHeapFree(pCompCSIList);
}

static ClInt32T cpmCompCSIKeyCmp(ClCntKeyHandleT key1,ClCntKeyHandleT key2)
{
    return clCpmNameKeyCmp((ClNameT*)key1, (ClNameT*)key2);
}

static void cpmCompCSIDeleteCallback(ClCntKeyHandleT key,
                                     ClCntDataHandleT data)
{
    ClCpmCompCSIT *pCSI = (ClCpmCompCSIT*)data;
    ClUint32T numAttributes = 0;
    ClAmsCSIAttributeT *pAttributes = NULL;
    if( (pAttributes = pCSI->csiDescriptor.csiAttributeList.attribute) )
    {
        ClInt32T i;
        numAttributes = pCSI->csiDescriptor.csiAttributeList.numAttributes;
        for(i = 0; i < numAttributes ; ++i)
        {
            if(pAttributes[i].attributeName)
                clHeapFree(pAttributes[i].attributeName);
            if(pAttributes[i].attributeValue)
                clHeapFree(pAttributes[i].attributeValue);
        }
        clHeapFree(pAttributes);
    }
    clHeapFree((ClCpmCompCSIT*)data);
}

static void amf_handle_instance_destructor(void *notused)
{
    /*
     * nothing to do
     */
}

/*
 *  Initialize the library, right now it does the handle database creation and 
 *  eo Client install
 */

static ClRcT clAmfLibInitialize(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *eo = NULL;

    /*
     * In library is not initialized then initialze the library
     */
    if (lib_initialized == CL_FALSE)
    {
        rc = clEoMyEoObjectGet(&eo);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                         ("Unable to get eo Object rc=%x\n", rc), rc);

        rc = clEoClientInstallTables(eo, CL_EO_SERVER_SYM_MOD(gAspFuncTable,
                                                              CPMMgmtClient));
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_EO_CLIENT_INST_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, (" Unable to initialize the \
    		AMF CB , rc = %x \n", rc), rc);

        rc = clCpmClientTableRegister(eo);
        if(rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_EO_CLIENT_INST_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to install CPM client table. rc [%#x]", rc), rc);

        rc = clHandleDatabaseCreate(amf_handle_instance_destructor,
                                    &handle_database);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_HANDLE_DB_CREATE_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                         ("Unable to create handle database rc=%x\n", rc), rc);

        lib_initialized = CL_TRUE;
    }

    return CL_OK;

  failure:
    return CL_CPM_RC(rc);
}


static ClRcT check_lib_init(void)
{
    ClRcT rc = CL_OK;

    if (lib_initialized == CL_FALSE)
    {
        rc = clAmfLibInitialize();
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                       CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "AMF", rc);
        }
    }
    return CL_CPM_RC(rc);
}

ClRcT clCpmComponentResourceCleanup(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;

    rc = clEoMyEoObjectGet(&pThis);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("MyEoObject Get failed rc=%x\n", rc), rc);
    if (pThis->clEoDeleteCallout != NULL)
    {
        rc = pThis->clEoDeleteCallout();
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_EO_DEL_CALL_OUT_ERR, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\n EO: Delete User Callout function failed \n"));
            return rc;
        }
    }

    /*
     * Moved clEoStateSet() to clEoCleanup() to avoid race
     * conditions. Instead we just set the flag
     * to false and Unblock the worker threads.
     */
    clEoUnblock(pThis);

  failure:
    return rc;
}

ClRcT cpmPostCallback(ClCpmCallbackTypeT cbType,
                      void *info,
                      ClCpmInstanceT *cpmInstance)
{
    ClCpmCallbackQueueDataT *queueData = NULL;
    ClRcT rc = CL_OK;
    char chr = 'c';
    ClInt32T count = 0;
        
    queueData = clHeapAllocate(sizeof(ClCpmCallbackQueueDataT));
    CL_ASSERT(queueData != NULL);
    queueData->cbType = cbType;
    queueData->params = info;

    clOsalMutexLock(cpmInstance->cbMutex);
    rc = clQueueNodeInsert(cpmInstance->cbQueue, (ClQueueDataT) queueData);
    if (rc != CL_OK)
    {
        clOsalMutexUnlock(cpmInstance->cbMutex); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Unable to delete Queue node %x\n", rc));
        goto failure;
    }
    if(cbType != CL_CPM_FINALIZE)
        count = write(cpmInstance->writeFd, (void *) &chr, 1);
    clOsalMutexUnlock(cpmInstance->cbMutex);
    
    return rc;

  failure:
    if(info) clHeapFree(info);
    if (queueData) clHeapFree(queueData);
    return rc;
}

/*
 * Wrapper for component Management functions 
 */

ClRcT handleComponentHealthcheck(ClCpmClientCompTerminateT *info,
                                 ClCpmInstanceT *cpmInstance,
                                 ClBufferHandleT outMsgHandle)
{
    if (cpmInstance->callbacks.appHealthCheck != NULL)
    {
        cpmInstance->callbacks.appHealthCheck(info->invocation,
                                              &(info->compName),
                                              &cpmInstance->healthcheckKey);
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "Healthcheck callback is NULL, so healthcheck "
                     "callback won't be invoked.");
    }
    if(info) clHeapFree(info);
    return CL_OK;
}

static ClRcT VDECL(_clCpmClientCompHealthCheck)(ClEoDataT eoArg,
                                         ClBufferHandleT  inMsgHandle,
                                         ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientCompTerminateT *info = NULL;
    ClNameT appName = {0};
    ClHandleT cpmHandle = 0;
    ClCpmInstanceT *cpmInstance = NULL;

    info = clHeapCalloc(1, sizeof(*info));
    CL_ASSERT(info != NULL);

    rc = VDECL_VER(clXdrUnmarshallClCpmClientCompTerminateT, 4, 0, 0)(inMsgHandle, (void *)info);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB", 
                   "Unable to unmarshall the buffer, error [%#x]",
                   rc);
        goto failure;
    }
        
    rc = clCpmComponentNameGet(0, &appName);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB",
                   "Unable to get component name, rc = [%#x]",
                   rc);
        goto failure;
    }
    
    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB",
                   "Unable to get handle for component [%.*s], "
                   "error [%#x]",
                   appName.length,
                   appName.value,
                   rc);
        goto failure;
    }
    
    rc = clHandleCheckout(handle_database, cpmHandle, (void *)&cpmInstance);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB",
                   "Unable to check out handle, error [%#x]",
                   rc);
        goto failure;
    }

    if (cpmInstance->readFd == 0)
    {
        rc = handleComponentHealthcheck(info, cpmInstance, outMsgHandle);
    }
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_HEALTHCHECK, (void *)info, cpmInstance);
        info = NULL;
    }
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB",
                   "Unable to check out handle, error [%#x]",
                   rc);
        goto failure;
    }

    return CL_OK;

failure:
    if(info) clHeapFree(info);
    return rc;
}

ClRcT handleComponentTerminate(ClCpmClientCompTerminateT *info,
                               ClCpmInstanceT *cpmInstance,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientCompTerminateT terminateInfo = {0};
    if(info && cpmInstance)
    {
        memcpy(&terminateInfo, info, sizeof(terminateInfo));
        clHeapFree(info);
        if (cpmInstance->callbacks.appTerminate != NULL)
        {
            cpmInstance->callbacks.appTerminate(terminateInfo.invocation,
                                                &terminateInfo.compName);
        }
        else
        {
            clLogWarning("AMF", "CLN",
                         "Terminate callback is NULL, will be treated as "
                         "termination failure due to timeout...");
        }
    }
    return rc;
}

ClRcT handleProxiedComponentInstantiate(ClCpmClientCompTerminateT *info,
                                        ClCpmInstanceT *cpmInstance,
                                        ClBufferHandleT outMsgHandle)
{
    ClRcT   rc = CL_OK;
    
    if ((cpmInstance->callbacks).appProxiedComponentInstantiate != NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_0_LCM_INVOKE_CALLBACK_INFO);
        cpmInstance->callbacks.appProxiedComponentInstantiate(info->invocation,
                                                              &(info->compName));
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "Proxied instantiate callback is NULL, will be treated as "
                     "proxied instantiation failure due to timeout...");
    }
    clHeapFree(info);
    return rc;
}

ClRcT handleProxiedComponentCleanup(ClCpmClientCompTerminateT *info,
                                    ClCpmInstanceT *cpmInstance,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT   rc = CL_OK;
    
    if (cpmInstance->callbacks.appProxiedComponentCleanup != NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_0_LCM_INVOKE_CALLBACK_INFO);

        cpmInstance->callbacks.appProxiedComponentCleanup(info->invocation, 
                &(info->compName));
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "Proxied cleanup callback is NULL, will be treated as "
                     "proxied cleanup failure due to timeout...");
    }
    clHeapFree(info);
    return rc;
}

/*
 * Called with lock held.
 */
static ClRcT cpmCompCSIListGet(const ClNameT *pCompName,
                               ClCpmCompCSIListT **ppCompCSIList,
                               ClCntNodeHandleT *pNodeHandle)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    ClCntNodeHandleT nodeHandle =0;
    
    if(ppCompCSIList==NULL && pNodeHandle ==NULL)
    {
        goto out;
    }

    if(pNodeHandle == NULL)
    {
        pNodeHandle = &nodeHandle;
    }

    rc = clCntNodeFind(gCpmCompCSIListHandle,(ClCntKeyHandleT)pCompName,
                       pNodeHandle);
    if(rc != CL_OK)
    {
        goto out;
    }
    if(ppCompCSIList)
    {
        rc = clCntNodeUserDataGet(gCpmCompCSIListHandle,*pNodeHandle,(ClCntDataHandleT*)ppCompCSIList);
    }
    out:
    return rc;
}

/*  
 * Called with lock held
 */
static ClRcT cpmCompCSIGet(ClNameT *pCSIName,
                           ClCpmCompCSIListT *pCompCSIList,
                           ClCpmCompCSIT **ppCSI,
                           ClCntNodeHandleT *pNodeHandle)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    ClCntNodeHandleT nodeHandle = 0;
    if(ppCSI == NULL && pNodeHandle == NULL)
    {
        goto out;
    }
    if(pNodeHandle == NULL)
    {
        pNodeHandle = &nodeHandle;
    }
    rc = clCntNodeFind(pCompCSIList->csiList,(ClCntKeyHandleT)pCSIName,
                       pNodeHandle);
    if(rc != CL_OK)
    {
        goto out;
    }
    if(ppCSI)
    {
        rc = clCntNodeUserDataGet(pCompCSIList->csiList,*pNodeHandle,
                                  (ClCntDataHandleT*)ppCSI);
    }
    out:
    return rc;
}

static ClRcT cpmCompCSIListDel(const ClNameT *pCompName)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle=0;

    clOsalMutexLock(gCpmCompCSIListMutex);
    rc = cpmCompCSIListGet(pCompName,NULL,&nodeHandle);
    if(rc != CL_OK)
    {
        goto out_unlock;
    }
    /*
     * Rip off the csi cache node
     */
    rc = clCntNodeDelete(gCpmCompCSIListHandle,nodeHandle);
    if(rc != CL_OK)
    {
        goto out_unlock;
    }
    out_unlock:
    clOsalMutexUnlock(gCpmCompCSIListMutex);
    return rc;
}

static ClRcT cpmCompCSIDel(ClCpmCompCSIRmvT *pInfo, ClCpmHandleT *pHandle)
{
    ClCpmCompCSIListT *pCompCSIList = NULL;
    ClCntNodeHandleT nodeHandle=0;
    ClRcT rc = CL_OK;

    *pHandle = 0;
    /*
     *Look up this comp:CSI pair
     */
    clOsalMutexLock(gCpmCompCSIListMutex);
    rc = cpmCompCSIListGet(&pInfo->compName,&pCompCSIList,NULL);
    if(rc != CL_OK)
    {
        /*
         * Its okay here to call apps. csi remove. 
         */
        rc = CL_OK;
        goto out_unlock;
    }
    *pHandle = pCompCSIList->cpmHandle;
    rc = cpmCompCSIGet(&pInfo->csiName,pCompCSIList,NULL,&nodeHandle);
    if(rc != CL_OK)
    {
        /*
         * Skip calling the apps. csi rmv.  
         */
        goto out_unlock;
    }
    rc = clCntNodeDelete(pCompCSIList->csiList,nodeHandle);
    if(rc != CL_OK)
    {
        /*
         * Not expected. So call apps. csi rmv.
         */
        rc = CL_OK;
        goto out_unlock;
    }
    out_unlock:
    clOsalMutexUnlock(gCpmCompCSIListMutex);
    return rc;
}

static ClRcT cpmCompCSIListAdd(const ClNameT *pCompName,ClCpmHandleT cpmHandle)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
    ClCpmCompCSIListT *pCompCSIList = NULL;
    ClCntHandleT csiList=0;

    pCompCSIList = clHeapCalloc(1,sizeof(*pCompCSIList));
    if(pCompCSIList == NULL)
    {
        goto out;
    }
    memcpy(&pCompCSIList->compName,pCompName,sizeof(pCompCSIList->compName));
    pCompCSIList->cpmHandle = cpmHandle;
    rc = clCntLlistCreate(cpmCompCSIKeyCmp,
                          cpmCompCSIDeleteCallback,
                          cpmCompCSIDeleteCallback,
                          CL_CNT_UNIQUE_KEY,
                          &csiList);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    pCompCSIList->csiList = csiList;
    clOsalMutexLock(gCpmCompCSIListMutex);
    rc = clCntNodeAdd(gCpmCompCSIListHandle,
                      (ClCntKeyHandleT)&pCompCSIList->compName,
                      (ClCntDataHandleT)pCompCSIList,
                      NULL);
    clOsalMutexUnlock(gCpmCompCSIListMutex);
    if(rc != CL_OK)
    {
        goto out_delete;
    }
    goto out;
    
    out_delete:
    clCntDelete(csiList);
    out_free:
    clHeapFree(pCompCSIList);
    out:
    return rc;
}

static ClRcT cpmCompCSIAdd(ClNameT *pCompName,
                           ClCpmCompCSIListT *pCompCSIList,
                           ClAmsCSIDescriptorT *pCSIDescriptor,
                           ClAmsHAStateT haState)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    ClCpmCompCSIT *pCSI = NULL;

    if(pCompName == NULL && pCompCSIList == NULL)
    {
        goto out;
    }

    /*Now add the entry*/
    clOsalMutexLock(gCpmCompCSIListMutex);
    if(pCompName && !pCompCSIList)
    {
        rc = cpmCompCSIListGet(pCompName,&pCompCSIList,NULL);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Unable to find component [%s] in comp csi cache. "
                            "rc = [0x%x]\n",
                            pCompName->value,rc));
            goto out_unlock;
        }
    }
    /*Now add the CSI to this compCSI cache*/
    pCSI = clHeapCalloc(1,sizeof(*pCSI));
    if(pCSI == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        goto out_unlock;
    }
    memcpy(&pCSI->csiDescriptor,pCSIDescriptor,sizeof(pCSI->csiDescriptor));
    memcpy(&pCSI->csiDescriptor.csiAttributeList, &pCSIDescriptor->csiAttributeList, 
           sizeof(pCSIDescriptor->csiAttributeList));
    if(!pCSIDescriptor->csiAttributeList.numAttributes)
        pCSI->csiDescriptor.csiAttributeList.attribute = NULL;
    pCSI->haState = haState;
    rc = clCntNodeAdd(pCompCSIList->csiList,
                      (ClCntKeyHandleT)&pCSI->csiDescriptor.csiName,
                      (ClCntDataHandleT)pCSI,
                      NULL);
    if(rc != CL_OK)
    {
        clHeapFree(pCSI);
        goto out_unlock;
    }
    out_unlock:
    clOsalMutexUnlock(gCpmCompCSIListMutex);

    out:
    return rc;
}

static ClRcT cpmCompCSIWalkCallback(ClCntKeyHandleT key,
                                    ClCntDataHandleT data,
                                    ClCntArgHandleT arg,
                                    ClUint32T dataLength)
{
    ClCpmCompCSIT *pCSI = (ClCpmCompCSIT*)data;
    ClCpmCompCSIWalkT *pCSIWalk = (ClCpmCompCSIWalkT*)arg;
    /*
     * Update the CSI and haState
     */
    if(pCSIWalk->haState != pCSI->haState)
    {
        pCSI->haState = pCSIWalk->haState;
        pCSIWalk->haStateUpdated = CL_TRUE;
    }
    memcpy(&pCSI->csiDescriptor.csiStateDescriptor,
           &pCSIWalk->pCSIDescriptor->csiStateDescriptor,
           sizeof(pCSI->csiDescriptor.csiStateDescriptor));
    pCSI->csiDescriptor.csiFlags = pCSIWalk->pCSIDescriptor->csiFlags;
    return CL_OK;
}

/*
 * Called with lock held.
 */
static ClRcT cpmCompCSIUpdateAll(ClCpmCompCSIListT *pCompCSIList,
                                 ClAmsCSIDescriptorT *pCSIDescriptor,
                                 ClAmsHAStateT haState,
                                 ClBoolT *pHAStateUpdated)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSIWalkT csiWalk = {0};

    if(!pHAStateUpdated) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    csiWalk.pCSIDescriptor = pCSIDescriptor;
    csiWalk.haState = haState;
    csiWalk.haStateUpdated = CL_FALSE;
    rc = clCntWalk(pCompCSIList->csiList,
                   cpmCompCSIWalkCallback,
                   (ClCntArgHandleT)&csiWalk,
                   sizeof(csiWalk));
    if(rc == CL_OK)
        *pHAStateUpdated = csiWalk.haStateUpdated;
    return rc;
}

static ClRcT _cpmCsiDescriptorUnpack(ClAmsCSIDescriptorT *csiDescriptor,
                                     ClAmsHAStateT haState,
                                     ClUint8T *buffer,
                                     ClUint32T bufferLength)
{
    ClRcT rc = CL_OK;
    ClUint32T numAttr = 0;
    ClUint32T totalAttr = 0;
    ClUint8T *attributeName = NULL;
    ClUint8T *attributeValue = NULL;
    ClUint32T attributeNameLength = 0;
    ClUint32T attributeValueLength = 0;
    ClAmsCSIAttributeT *attrList = NULL;
    ClCpmCSIDescriptorNameValueT attrType = CL_CPM_CSI_ATTR_NONE;
    ClAmsCSIDescriptorT tempCsi = {0};

    ClBufferHandleT message = 0;

    rc = clBufferCreate(&message);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create message \n"), rc);

    rc = clBufferNBytesWrite(message, buffer, bufferLength);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

    rc = clXdrUnmarshallClUint32T(message, (void *)&(tempCsi.csiFlags));
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

    if (CL_AMS_CSI_FLAG_TARGET_ALL != tempCsi.csiFlags)
    {
        rc = clXdrUnmarshallClNameT(message, (void *)&(tempCsi.csiName));
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
    }
    else
    {
        memset(&(tempCsi.csiName), '\0', sizeof(ClNameT));
    }

    if (CL_AMS_HA_STATE_ACTIVE == haState)
    {
        rc = VDECL_VER(clXdrUnmarshallClAmsCSIActiveDescriptorT, 4, 0, 0)(message,
                                                      (void *)
                                                      &(tempCsi.
                                                        csiStateDescriptor.
                                                        activeDescriptor));
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
    }
    else if (CL_AMS_HA_STATE_STANDBY == haState)
    {
        rc = VDECL_VER(clXdrUnmarshallClAmsCSIStandbyDescriptorT, 4, 0, 0)(message,
                                                       (void *)
                                                       &(tempCsi.
                                                         csiStateDescriptor.
                                                         standbyDescriptor));
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
    }

    if (CL_AMS_CSI_FLAG_ADD_ONE == tempCsi.csiFlags)
    {
        rc = clXdrUnmarshallClUint32T(message, 
                                      (void *)&(tempCsi.csiAttributeList.numAttributes));
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
                                                                                                  
        totalAttr = tempCsi.csiAttributeList.numAttributes;

        /*
         * Allocate Memory for the attrList
         */
        tempCsi.csiAttributeList.attribute = attrList =
        (ClAmsCSIAttributeT *) clHeapAllocate(totalAttr *
                                              sizeof(ClAmsCSIAttributeT));
        if (attrList == NULL)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("unable to allocate memory \n"),
                             CL_CPM_RC(CL_ERR_NO_MEMORY));
        }
        numAttr = 0;
        while (numAttr != totalAttr)
        {
            /*
             *  -   Get Type and Move the pointers
             *  -   Get attrName length and Move the pointers
             *  -   Allocate memory equal to length
             *  -   copy the received attrName to the attrName and move the pointers
             */
            rc = VDECL_VER(clXdrUnmarshallClCpmCSIDescriptorNameValueT, 4, 0, 0)(message,
                                                             (void *)&(attrType));
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
            if (attrType != CL_CPM_CSI_ATTR_NAME)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                           CL_LOG_MESSAGE_0_INVALID_BUFFER);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("invalide buffer received \n"),
                                 CL_CPM_RC(CL_ERR_INVALID_BUFFER));
            }

            rc = clXdrUnmarshallClUint32T(message, (void *)&(attributeNameLength));
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

            attributeName = (ClUint8T *) clHeapAllocate(attributeNameLength);
            if (attributeName == NULL)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("unable to allocate memory \n"),
                                 CL_CPM_RC(CL_ERR_NO_MEMORY));
            }
            rc = clXdrUnmarshallArrayClCharT(message, (void *) attributeName,
                                             attributeNameLength);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
            attrList[numAttr].attributeName = attributeName;

            /*
             *  -   Get Type and Move the pointers
             *  -   Get attrValue length and Move the pointers
             *  -   Allocate memory equal to length
             *  -   copy the received value to the attrValue and move the pointers
             */

            rc = VDECL_VER(clXdrUnmarshallClCpmCSIDescriptorNameValueT, 4, 0, 0)(message,
                                                             (void *)&(attrType));
            if (attrType != CL_CPM_CSI_ATTR_VALUE)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                           CL_LOG_MESSAGE_0_INVALID_BUFFER);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("invalide buffer received \n"),
                                 CL_CPM_RC(CL_ERR_INVALID_BUFFER));
            }

            rc = clXdrUnmarshallClUint32T(message, (void *)&(attributeValueLength));
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

            attributeValue = (ClUint8T *) clHeapAllocate(attributeValueLength);
            if (attributeValue == NULL)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("unable to allocate memory \n"),
                                 CL_CPM_RC(CL_ERR_NO_MEMORY));
            }
            rc = clXdrUnmarshallArrayClCharT(message, (void *) attributeValue,
                                             attributeValueLength);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
            attrList[numAttr].attributeValue = attributeValue;

            numAttr++;
        }
    }
    else
    {
        tempCsi.csiAttributeList.attribute = NULL;
        tempCsi.csiAttributeList.numAttributes = 0;
    }

    memcpy(csiDescriptor, &(tempCsi), sizeof(ClAmsCSIDescriptorT));

    clBufferDelete(&message);
    return rc;

failure:
     /*
      * Free the allocated memory
      */
    if(message != 0)
        clBufferDelete(&message);
    if (attrList != NULL)
    {
        numAttr = 0;
        while (numAttr != totalAttr)
        {
            if (attrList[numAttr].attributeName != NULL)
                clHeapFree(attrList[numAttr].attributeName);
            attrList[numAttr].attributeName = NULL;

            if (attrList[numAttr].attributeValue != NULL)
                clHeapFree(attrList[numAttr].attributeValue);
            attrList[numAttr].attributeValue = NULL;
            numAttr++;
        }
        clHeapFree(attrList);
        attrList = NULL;
    }
    return rc;
}

ClRcT handleCompCSIAssign(ClCpmCompCSISetT *info,
                          ClCpmInstanceT *cpmInstance)
{
    ClRcT   rc = CL_OK;
    ClAmsCSIDescriptorT csiDescriptor;
    ClAmsCSIAttributeT *attrList = NULL;
    ClUint32T numAttr = 0;
    ClUint32T totalAttr = 0;
    /*
     * Call unpack function Function here, which converts the buffer into the
     * csiDescriptor 
     */
    memset(&(csiDescriptor), '\0', sizeof(ClAmsCSIDescriptorT));
    rc = _cpmCsiDescriptorUnpack(&csiDescriptor,
                                 info->haState,
                                 info->buffer,
                                 info->bufferLength);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to unpack CSI descriptor %x\n", rc), rc);
    attrList = csiDescriptor.csiAttributeList.attribute;
    totalAttr = csiDescriptor.csiAttributeList.numAttributes;

    if ((cpmInstance->callbacks).appCSISet != NULL)
    {
        ClCpmCompCSIListT *pCompCSIList =  NULL;
        ClCpmCompCSIT *pCSI = NULL;
        ClCpmHandleT cpmHandle =0;
        clOsalMutexLock(gCpmCompCSIListMutex);
        rc = cpmCompCSIListGet(&info->compName,&pCompCSIList,NULL);
        if(rc != CL_OK)
        {
            rc = cpmCompCSIListGet(&info->proxyCompName, &pCompCSIList, NULL);
            clOsalMutexUnlock(gCpmCompCSIListMutex);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                               ("Unable to find component [%s] in cache. "
                                "rc = [0x%x]",
                                info->compName.value,
                                rc));
                goto csi_set;
            }
            /*
             * Now add this proxied to the csi cache.
             */
            rc = cpmCompCSIListAdd(&info->compName, pCompCSIList->cpmHandle);
            if(rc != CL_OK)
            {
                goto csi_set;
            }
            clOsalMutexLock(gCpmCompCSIListMutex);
            pCompCSIList = NULL;
            rc = cpmCompCSIListGet(&info->compName, &pCompCSIList, NULL);
            if(rc != CL_OK)
            {
                clOsalMutexUnlock(gCpmCompCSIListMutex);
                goto csi_set;
            }
        }
        /*
         * This is a TARGET_ALL or reassignCSIAll.
         * Update all CSI's of the component. Skip csi set
         * incase the ha state update is a duplicate.
         */
        if((csiDescriptor.csiFlags & CL_AMS_CSI_FLAG_TARGET_ALL))
        {
            ClBoolT haStateUpdated = CL_TRUE;
            cpmCompCSIUpdateAll(pCompCSIList,&csiDescriptor,info->haState, &haStateUpdated);
            if(haStateUpdated == CL_TRUE)
            {
                clOsalMutexUnlock(gCpmCompCSIListMutex);
                goto csi_set;
            }
            else
            {
                /*
                 * Skip the duplicate assignment.
                 */
                goto skip_csi_set;
            }
        }
        /*
         * Look up the CSI on the CSI name  
         */
        rc = cpmCompCSIGet(&csiDescriptor.csiName,pCompCSIList,&pCSI,NULL);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gCpmCompCSIListMutex);
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                           ("Unable to get CSI [%s] for Component [%s]. "
                            "rc = [0x%x]",
                            csiDescriptor.csiName.value,
                            info->compName.value,
                            rc));
            /*
             *Add this CSI to the compCSIList.
             */
            rc = cpmCompCSIAdd(NULL,pCompCSIList,&csiDescriptor,info->haState);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Unable to add CSI [%s] to Component [%s]. "
                                "rc = [0x%x]\n",
                                csiDescriptor.csiName.value,
                                info->compName.value,
                                rc));
            }
            else
            {
                if(totalAttr)
                {
                    /* 
                     * Prevent freeing as we have cached it
                     */
                    attrList = NULL;
                }
            }
            goto csi_set;
        }
        /*
         * Found the data.Check if this CSI is a duplicate assignment
         * or not.
         */
        if( pCSI->haState != info->haState)
        {
            pCSI->haState = info->haState;
            clOsalMutexUnlock(gCpmCompCSIListMutex);
            goto csi_set;
        }

        /*
         * Now check if its a standby assignment and the last one
         * was also a standby with a different rank.
         * If thats the case, allow it.
         */
        if(info->haState == CL_AMS_HA_STATE_STANDBY)
        {
            if(pCSI->csiDescriptor.csiStateDescriptor.
               standbyDescriptor.standbyRank !=
               csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank)
            {
                pCSI->csiDescriptor.csiStateDescriptor.
                    standbyDescriptor.standbyRank =
                    csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank;
                clOsalMutexUnlock(gCpmCompCSIListMutex);
                goto csi_set;
            }
        }

        /*
         * Okay . So its a duplicate csi assignment.
         * Skip this blighter.
         */
        skip_csi_set:
        cpmHandle = pCompCSIList->cpmHandle;
        clOsalMutexUnlock(gCpmCompCSIListMutex);
        clLogInfo("CSI", "SET",
                  "Skipping duplicate CSI Assignment for component "
                  "[%s], CSI [%s], haState [0x%x]",
                  info->compName.value,
                  csiDescriptor.csiName.length ? 
                  csiDescriptor.csiName.value : "TARGET_ALL",
                  info->haState);
        rc = clCpmResponse(cpmHandle,info->invocation,CL_OK);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("CPM Response returned error.rc=[0x%x]\n",rc));
        }
        goto failure;

        csi_set:
        cpmInstance->callbacks.appCSISet(info->invocation,
                                         &(info->compName),
                                         info->haState, csiDescriptor);
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "CSI set callback not provided during "
                     "AMF client initialization, will be treated "
                     "as CSI set failure due to timeout...");
    }

    failure:
    
    if(info->buffer != NULL)
        clHeapFree(info->buffer);
    clHeapFree(info);
    info = NULL;

    if (attrList != NULL)
    {
        numAttr = 0;
        while (numAttr != totalAttr)
        {
            if (attrList[numAttr].attributeName != NULL)
                clHeapFree(attrList[numAttr].attributeName);
            attrList[numAttr].attributeName = NULL;

            if (attrList[numAttr].attributeValue != NULL)
                clHeapFree(attrList[numAttr].attributeValue);
            attrList[numAttr].attributeValue = NULL;
            numAttr++;
        }
        clHeapFree(attrList);
        attrList = NULL;
    }
    return rc;
}

ClRcT handleCompCSIRmv(ClCpmCompCSIRmvT *info,
                       ClCpmInstanceT *cpmInstance)
{
    ClRcT   rc = CL_OK;

    /*
     * Call the callback provided while initialization 
     */
    if ((cpmInstance->callbacks).appCSIRmv != NULL)
    {
        ClCpmHandleT cpmHandle = 0;
        /*
         * Delete the CSI reference from the comp CSIList
         */
        rc = cpmCompCSIDel(info, &cpmHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                           ("Unable to delete CSI [%s] from component [%s] "
                            "in cache. rc = [0x%x]",
                            info->csiName.value,
                            info->compName.value,
                            rc));
            if(cpmHandle)
            {
                CL_DEBUG_PRINT(CL_DEBUG_INFO,("Skipping CSI remove for component "
                                              "[%s], CSI [%s]",
                                              info->compName.value,
                                              info->csiName.value));
                rc = clCpmResponse(cpmHandle, info->invocation, CL_OK);
                if(rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CPM response returned [%#x]", rc));
                }
                goto out_free;
            }
        }
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_0_LCM_INVOKE_CALLBACK_INFO);
        cpmInstance->callbacks.appCSIRmv(info->invocation,
                                         &(info->compName),
                                         &(info->csiName),
                                         info->csiFlags);
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "CSI remove callback not provided during "
                     "AMF client initialization, will be treated "
                     "as CSI remove failure due to timeout...");
    }
    
    out_free:
    clHeapFree(info);
    return rc;
}

ClRcT handlePGTrack(ClCpmPGResponseT *recvBuff,
                    ClCpmInstanceT *cpmInstance)
{
    ClRcT   rc = CL_OK;
    ClAmsPGNotificationBufferT *notificationBuffer = NULL;
    ClAmsPGNotificationT amsPGNotification;
    ClBufferHandleT message = 0;
    ClUint32T i = 0;

    /*
     * Call the callback provided while initialization 
     */
    if ((cpmInstance->callbacks).appProtectionGroupTrack != NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_0_LCM_INVOKE_CALLBACK_INFO);


        rc = clBufferCreate(&message);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create message \n"), rc);

        rc = clBufferNBytesWrite(message, recvBuff->buffer, 
                recvBuff->bufferLength);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

        notificationBuffer = (ClAmsPGNotificationBufferT *)
            clHeapAllocate(sizeof(ClAmsPGNotificationBufferT));
        if (notificationBuffer== NULL)
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                             CL_CPM_RC(CL_ERR_NO_MEMORY));

        rc = clXdrUnmarshallClUint32T(message, 
                (void *)&notificationBuffer->numItems);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

        notificationBuffer->notification = (ClAmsPGNotificationT *)
            clHeapAllocate(notificationBuffer->numItems * 
                    sizeof(ClAmsPGNotificationT));
        if (notificationBuffer->notification == NULL)
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                             CL_CPM_RC(CL_ERR_NO_MEMORY));
        
        for (i = 0; i < notificationBuffer->numItems; ++i) 
        {
            rc = VDECL_VER(clXdrUnmarshallClAmsPGNotificationT, 4, 0, 0)(message,
                    (void *)&amsPGNotification);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
            memcpy(&(notificationBuffer->notification[i]), 
                    &amsPGNotification, sizeof(ClAmsPGNotificationT)); 
        }

        cpmInstance->callbacks.appProtectionGroupTrack(
                &(recvBuff->csiName),
                (ClAmsPGNotificationBufferT*) notificationBuffer,
                recvBuff->numberOfMembers, 
                recvBuff->error);

        rc = clBufferDelete(&message);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to delete message \n"), rc);
    }
    else
    {
        clLogWarning("AMF", "CLN",
                     "Protection group track callback is NULL, so "
                     "protection group track callback won't be invoked.");
    }

    if(recvBuff)
    {
        if(recvBuff->buffer) clHeapFree(recvBuff->buffer);
        clHeapFree(recvBuff);
    }
    if(notificationBuffer)
    {
        if(notificationBuffer->notification) clHeapFree(notificationBuffer->notification);
        clHeapFree(notificationBuffer);
    }

    return rc;

failure:
    if(recvBuff)
    {
        if(recvBuff->buffer) clHeapFree(recvBuff->buffer);
        clHeapFree(recvBuff);
    }
    if(notificationBuffer)
    {
        if(notificationBuffer->notification) clHeapFree(notificationBuffer->notification);
        clHeapFree(notificationBuffer);
    }

    return rc;
}

static ClRcT VDECL(_clCpmClientCompTerminate)(ClEoDataT eoArg,
                                       ClBufferHandleT inMsgHandle,
                                       ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientCompTerminateT *info = { 0 };
    ClNameT appName = { 0 };
    ClHandleT cpmHandle;
    ClCpmInstanceT *cpmInstance = NULL;

    info =
        (ClCpmClientCompTerminateT *)
        clHeapAllocate(sizeof(ClCpmClientCompTerminateT));
    if (info == NULL)
        goto failure;

    rc = VDECL_VER(clXdrUnmarshallClCpmClientCompTerminateT, 4, 0, 0)(inMsgHandle, (void *) info);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_BUF_READ_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to Get message \n"), rc);

    /*  PROXIED Support: 
     *  Request may be for Proxy-Proxied case in which case compName will be
     *  of proxied. Thus using appName to get handle instead of compName as earlier
     *  and need not to distinguish between SA-aware or Proxied component.
     *
     *  This will work as long as multiple Components for same EO are not allowed
     */

    rc = clCpmComponentNameGet(0, &appName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Component name get failed rc=%x\n", rc),
            rc);
    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_2_HANDLE_GET_ERR, appName.value, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
            ("Unable to Get handle for component %s %x\n",
             appName.value, rc), rc);

    /* Don't think really need this now for new proxy-proxied interface
     * Put it for time being */  
    if (!strcmp(appName.value, info->compName.value))
        componentTerminate = CL_TRUE;

    /* End PROXIED change */

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    if(cpmInstance->readFd == 0)
        rc = handleComponentTerminate(info, cpmInstance, outMsgHandle);
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_TERMINATE, (void *)info, cpmInstance);
        info = NULL;
    }
    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;

failure:
    if(info != NULL)
        clHeapFree(info);
    return rc;
}

static ClRcT VDECL(_clCpmClientCompCSISet)(ClEoDataT eoArg,
                                           ClBufferHandleT inMsgHandle,
                                           ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSISetT *recvBuff=NULL;
    ClUint32T msgLength = 0;
    ClHandleT cpmHandle;
    ClCpmInstanceT *cpmInstance = NULL;
    ClUint32T cbType = 0;
    ClNameT appName = {0};

    rc = clBufferLengthGet(inMsgHandle, &msgLength);

    if(!msgLength || rc != CL_OK)
    {
        clLogNotice("CSI", "SET", "Buffer length [%d], csi set length [%d]",
                    msgLength, (ClUint32T)sizeof(ClCpmCompCSISetT));
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                         CL_CPM_RC(CL_ERR_INVALID_BUFFER));
    }

    if(msgLength < sizeof(ClCpmCompCSISetT))
        msgLength = sizeof(ClCpmCompCSISetT);

    recvBuff = (ClCpmCompCSISetT *) clHeapAllocate(msgLength);
    if (recvBuff == NULL)
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    rc = VDECL_VER(clXdrUnmarshallClCpmCompCSISetT, 4, 0, 0)(inMsgHandle, (void *)recvBuff);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

    CL_CPM_INVOCATION_CB_TYPE_GET(recvBuff->invocation, cbType);

    /*  PROXIED Support: 
     *  Request may be for Proxy-Proxied case in which case compName will be
     *  of proxied. Thus using appName to get handle instead of compName as earlier
     *  and need not to distinguish between SA-aware or Proxied component.
     *
     *  This will work as long as multiple Components for same EO are not allowed
     */
     
    rc = clCpmComponentNameGet(0, &appName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Component name get failed rc=%x\n", rc),
                     rc);
    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_GET_ERR, appName.value, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to Get handle for component %s %x\n",
                      appName.value, rc), rc);

    /* End PROXIED change */
    
    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    if(cpmInstance->readFd == 0)
        rc = handleCompCSIAssign(recvBuff, cpmInstance);
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_CSI_SET, (void *)recvBuff, cpmInstance);
        recvBuff = NULL;
    }
    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;
    
    failure:
    if (recvBuff != NULL)
    {
        if(recvBuff->buffer != NULL)
            clHeapFree(recvBuff->buffer);
        clHeapFree(recvBuff);
        recvBuff = NULL;
    }
    return rc;
}

static ClRcT VDECL(_clCpmClientCompCSIRmv)(ClEoDataT eoArg,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSIRmvT *recvBuff = NULL;
    ClUint32T msgLength = 0;
    ClHandleT cpmHandle;
    ClCpmInstanceT *cpmInstance = NULL;
    ClNameT appName = { 0 };

    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if(!msgLength || rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_INVALID_BUFFER);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                         CL_CPM_RC(CL_ERR_INVALID_BUFFER));
    }

    if (msgLength < sizeof(ClCpmCompCSIRmvT))
    {
        msgLength = sizeof(ClCpmCompCSIRmvT);
    }

    recvBuff = (ClCpmCompCSIRmvT *) clHeapAllocate(msgLength);
    if (recvBuff == NULL)
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    
    rc = VDECL_VER(clXdrUnmarshallClCpmCompCSIRmvT, 4, 0, 0)(inMsgHandle, recvBuff);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
               CL_CPM_LOG_2_LCM_CSI_RMV_INFO, recvBuff->compName.value,
               recvBuff->csiName.value);

    /*  PROXIED Support: 
     *  Request may be for Proxy-Proxied case in which case compName will be
     *  of proxied. Thus using appName to get handle instead of compName as earlier
     *  and need not to distinguish between SA-aware or Proxied component.
     *
     *  This will work as long as multiple Components for same EO are not allowed
     */

    rc = clCpmComponentNameGet(0, &appName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Component name get failed rc=%x\n", rc),
            rc);
    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_2_HANDLE_GET_ERR, appName.value, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
            ("Unable to Get handle for component %s %x\n",
             appName.value, rc), rc);

    /* End PROXIED change */

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    
    if(cpmInstance->readFd == 0)
        rc = handleCompCSIRmv(recvBuff, cpmInstance);
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_CSI_RMV, (void *)recvBuff, cpmInstance);
        recvBuff = NULL;
    }
    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;

  failure:
    if(recvBuff) clHeapFree(recvBuff);
    return rc;
}

ClRcT unmarshallPGResponse(ClBufferHandleT outMsgHandle, void* notification);

static ClRcT VDECL(_clCpmClientCompProtectionGroupTrack)(ClEoDataT eoArg,
                                                         ClBufferHandleT inMsgHandle,
                                                         ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmPGResponseT *recvBuff = NULL;
    ClUint32T msgLength = 0;
    ClCpmInstanceT *cpmInstance = NULL;

    rc = clBufferLengthGet(inMsgHandle, &msgLength);

    if(!msgLength || rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_INVALID_BUFFER);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                         CL_CPM_RC(CL_ERR_INVALID_BUFFER));
    }

    if (msgLength < sizeof(ClCpmPGResponseT))
    {
        msgLength = sizeof(ClCpmPGResponseT);
    }

    recvBuff = (ClCpmPGResponseT *) clHeapAllocate(msgLength);
    if (recvBuff == NULL)
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    rc = VDECL_VER(clXdrUnmarshallClCpmPGResponseT, 4, 0, 0)(inMsgHandle, (void *)recvBuff);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_BUF_READ_ERR, rc);
    }

    /*clOsalPrintf("Recv buffer %d total Length %d\n", recvBuff->bufferLength, msgLength);*/
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to unpack PG buffer descriptor %x\n", rc), rc);
    
    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, recvBuff->cpmHandle, 
                          (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);
    
#ifndef CL_CPM_SELECTION
    rc = handlePGTrack(recvBuff, cpmInstance);
#else
    rc = cpmPostCallback(CL_CPM_CB_PROTECTION_TRACK, (void *)recvBuff, cpmInstance);
#endif
    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, recvBuff->cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;

    failure:
    if(recvBuff)
    {
        if(recvBuff->buffer) clHeapFree(recvBuff->buffer);
        clHeapFree(recvBuff);
    }
    
    return rc;
}

static ClRcT
VDECL(_clCpmClientCompProxiedComponentInstantiate)(ClEoDataT eoArg,
                                            ClBufferHandleT inMsgHandle,
                                            ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientCompTerminateT *info = NULL;
    ClHandleT cpmHandle;
    ClCpmInstanceT *cpmInstance = NULL;
    ClNameT appName = { 0 };

    info =
        (ClCpmClientCompTerminateT *)
        clHeapAllocate(sizeof(ClCpmClientCompTerminateT));
    if (info == NULL)
        goto failure;
    
    rc = VDECL_VER(clXdrUnmarshallClCpmClientCompTerminateT, 4, 0, 0)(inMsgHandle, (void *) info);

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to Get message \n"), rc);

    /*  PROXIED Support: 
     *  Request may be for Proxy-Proxied case in which case compName will be
     *  of proxied. Thus using appName to get handle instead of compName as earlier
     *  and need not to distinguish between SA-aware or Proxied component.
     *
     *  This will work as long as multiple Components for same EO are not allowed
     */

    rc = clCpmComponentNameGet(0, &appName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Component name get failed rc=%x\n", rc),
            rc);

    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_2_HANDLE_GET_ERR, appName.value, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
            ("Unable to Get handle for component %s %x\n",
             appName.value, rc), rc);

    /* End PROXIED change */

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    if(cpmInstance->readFd == 0)
        rc = handleProxiedComponentInstantiate(info, cpmInstance, outMsgHandle);
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_PROXIED_INST, (void *)info, cpmInstance);
        info = NULL;
    }

    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }
    return rc;

failure:
    if(info != NULL)
        clHeapFree(info);
    return rc;
}

static ClRcT
VDECL(_clCpmClientCompProxiedComponentCleanup)(ClEoDataT eoArg,
                                        ClBufferHandleT inMsgHandle,
                                        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientCompTerminateT *info = NULL; 
    ClHandleT cpmHandle;
    ClCpmInstanceT *cpmInstance = NULL;
    ClNameT appName = { 0 };
    
    info =
        (ClCpmClientCompTerminateT *)
        clHeapAllocate(sizeof(ClCpmClientCompTerminateT));
    if (info == NULL)
        goto failure;
    
    rc = VDECL_VER(clXdrUnmarshallClCpmClientCompTerminateT, 4, 0, 0)(inMsgHandle, (void *) info);


    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to Get message \n"), rc);

    /*  PROXIED Support: 
     *  Request may be for Proxy-Proxied case in which case compName will be
     *  of proxied. Thus using appName to get handle instead of compName as earlier
     *  and need not to distinguish between SA-aware or Proxied component.
     *
     *  This will work as long as multiple Components for same EO are not allowed
     */

    rc = clCpmComponentNameGet(0, &appName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Component name get failed rc=%x\n", rc),
            rc);
    rc = component_handle_mapping_get(&appName, &cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_2_HANDLE_GET_ERR, appName.value, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
            ("Unable to Get handle for component %s %x\n",
             appName.value, rc), rc);

    /* End PROXIED change */

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    rc = cpmCompCSIListDel(&info->compName);
    if(rc != CL_OK)
        clLogError("PROXIED", "CLEANUP",
                   "Unable to delete Component [%s] from cache. "
                   "rc = [0x%x]",
                   info->compName.value,
                   rc);

    /* Bug 4674:
     * Changed the second parameter from NULL to cpmInstance.
     */
    if(cpmInstance->readFd == 0)
        rc = handleProxiedComponentCleanup(info, cpmInstance, outMsgHandle);
    else
    {
        rc = cpmPostCallback(CL_CPM_CB_PROXIED_CLEANUP, (void *)info, 
                cpmInstance);
        info = NULL;
    }
 
    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;

  failure:
    if(info != NULL)
        clHeapFree(info);
    return rc;
}

/******************************************************************************/
/*
 * START OF SAF RELATED CLIENT SIDE FUNCTIONS 
 */
/******************************************************************************/
/*
 * Dummy queue callback functions.
 */
static void userDequeueCallBack(ClQueueDataT userData)
{
    ;
}

static void userDestroyCallback(ClQueueDataT userData)
{
    ;
}

ClRcT clCpmClientInitialize(ClCpmHandleT *cpmHandle,
                            const ClCpmCallbacksT *callback,
                            ClVersionT *version)
{
    ClRcT rc = CL_OK;
    ClCpmCompInitSendT compInitSend = { 0 };
    ClCpmCompInitRecvT compInitRecv = { 0 };
    ClNameT compName = { 0 };
    ClUint32T outBufLen = sizeof(ClCpmCompInitRecvT);
    ClCpmInstanceT *cpmInstance = NULL;

    /*
     * Input Param check
     */
    if (cpmHandle == NULL || callback == NULL || version == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null pointer passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * FIXME: Right now only one initialization is allowed in a process 
     */
    if (clClientInitialized == 1)
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                         ("For this release only one initialize is supported"),
                         CL_CPM_RC(CL_ERR_INITIALIZED));
    
    /*
     * Initialize the library and handle database 
     */
    rc = check_lib_init();
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "AMF", rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to Initialize AMF library %x\n", rc), rc);

    /*
     * Verify the version information 
     */
    rc = clVersionVerify(&version_database, version);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_VERSION_MISMATCH);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Version verification failed %x\n", rc),
                     rc);

    rc = clOsalMutexCreate(&gCpmCompCSIListMutex);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create mutex.rc=[0x%x]\n", rc), rc);

    /*
     * Create the hash table for the component CSI Cache
     */
    rc = clCntLlistCreate(cpmCompCSIListKeyCmp,
                          cpmCompCSIListDeleteCallback,
                          cpmCompCSIListDeleteCallback,
                          CL_CNT_UNIQUE_KEY,
                          &gCpmCompCSIListHandle
                          );
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_1_CNT_CREATE_FAILED,rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("List container creation failed.rc=[%x]\n", rc),
                     rc);
    /*
     * Create the handle 
     */
    rc = clHandleCreate(handle_database, sizeof(ClCpmInstanceT), cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CREATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create handle %x\n", rc), rc);

    /* Initialize the global for use by other libraries */
    clCpmHandle = *cpmHandle;
    
    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, *cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    rc = clOsalMutexCreate(&cpmInstance->response_mutex);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create mutex %x\n", rc), rc);

    rc = clOsalMutexCreate(&cpmInstance->cbMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clQueueCreate(0, userDequeueCallBack, userDestroyCallback,
                       &cpmInstance->cbQueue);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_QUEUE_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    cpmInstance->readFd = 0;
    cpmInstance->writeFd = 0;
    cpmInstance->dispatchType = 0;
    cpmInstance->finalize = 0;
    /*
     * Build the send buffer and send it to the server [Local CPM] 
     */
    memcpy(&(compInitSend.cpmVersion), version, sizeof(ClVersionT));
    compInitSend.myPid = getpid();
    rc = clEoMyEoIocPortGet(&compInitSend.eoPort);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("MyEoIocPort Get failed rc=%x\n", rc),
                     rc);

    rc = clCpmComponentNameGet(0, &compName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to Get Component Name, rc = %x\n", rc), rc);
    strcpy((compInitSend.compName), compName.value);
    if(clEoWithOutCpm != CL_TRUE)
    {
        rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), 
                                   CPM_COMPONENT_INITIALIZE,
                                   (ClUint8T *) &compInitSend,
                                   sizeof(ClCpmCompInitSendT),
                                   (ClUint8T *) &compInitRecv,
                                   &outBufLen,
                                   CL_RMD_CALL_ATMOST_ONCE |
                                   CL_RMD_CALL_NEED_REPLY,
                                   0,
                                   0,
                                   0,
                                   MARSHALL_FN(ClCpmCompInitSendT, 4, 0, 0),
                                   UNMARSHALL_FN(ClCpmCompInitRecvT, 4, 0, 0));
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_INIT_ERR, rc);
        }
    }

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Failed to initialize , rc = %x \n", rc),
                     rc);

    /*
     * Initialize the callbacks in the instance , these functions are used for 
     * invoking client Callbacks 
     */
    memcpy(&(cpmInstance->callbacks), callback, sizeof(ClCpmCallbacksT));

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_CPM_CLIENT_LIB,
               CL_CPM_LOG_0_CLIENT_INIT_INFO);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("clCpmClientInitialize Sucess"));

    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, *cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

    return rc;

  failure:
    return rc;
}

ClRcT clCpmClientFinalize(ClCpmHandleT cpmHandle)
{
    ClRcT rc = CL_OK;
    ClNameT compName = { 0 };
    ClCpmCompFinalizeSendT compFinalize = { 0 };
    ClEoExecutionObjT *pEoObj = NULL;
    ClCpmInstanceT *cpmInstance = NULL;
    ClCharT chr = 'f';
    ClUint32T   count = 0;
    /*
     * Checkout the client handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    rc = clOsalMutexLock(cpmInstance->response_mutex);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to lock %x\n", rc), rc);

    /*
     * Another thread has already started finalizing 
     */
    if (cpmInstance->finalize)
    {
        rc = clOsalMutexUnlock(cpmInstance->response_mutex);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc), rc);

        rc = clHandleCheckin(handle_database, cpmHandle);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
        }
        rc = CL_CPM_RC(CL_ERR_INVALID_HANDLE);

        goto failure;
    }
    if(cpmInstance->writeFd != 0)
    {
        rc = cpmPostCallback(CL_CPM_FINALIZE, NULL, cpmInstance);
        count = write(cpmInstance->writeFd, (void *) &chr, 1);
    }
    cpmInstance->finalize = 1;
    /*if(cpmInstance->dispatchType == CL_DISPATCH_ONE || 
        cpmInstance->dispatchType == CL_DISPATCH_ALL) 
    {
        if(cpmInstance->readFd != 0)
            close(cpmInstance->readFd);
        if(cpmInstance->writeFd != 0)
            close(cpmInstance->writeFd);
    }*/
    rc = clOsalMutexUnlock(cpmInstance->response_mutex);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc), rc);
    
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }
    /*
     * Inform server about the finalize 
     */
    rc = clCpmComponentNameGet(0, &compName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to Get Component Name, rc = %x\n", rc), rc);

    strcpy((compFinalize.compName), compName.value);

    compFinalize.cpmHandle = cpmHandle;

    rc = clEoMyEoObjectGet(&pEoObj);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("MyEoObject Get failed rc=%x\n", rc), rc);

    rc = clCntDelete(gCpmCompCSIListHandle);

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,("Container delete failure.rc=[0x%x]\n",rc),rc);

    clOsalMutexDelete(gCpmCompCSIListMutex);

    if(clEoWithOutCpm != CL_TRUE)
    {
        rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), 
                CPM_COMPONENT_FINALIZE,
                (ClUint8T *) &compFinalize,
                sizeof(ClCpmCompFinalizeSendT), NULL, NULL,
                CL_RMD_CALL_ATMOST_ONCE, 0, 0, 0,
                MARSHALL_FN(ClCpmCompFinalizeSendT, 4, 0, 0)
                );

        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                    CL_CPM_LOG_1_CLIENT_FINALIZE_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed rc =%x\n", 
                    __FUNCTION__, rc), rc);
    }

    return rc;
  failure:
    return rc;
}

ClRcT clCpmSelectionObjectGet(ClCpmHandleT cpmHandle,
                              ClSelectionObjectT *pSelectionObject)
{
    ClRcT rc = CL_OK;
    ClInt32T fds[2];
    ClInt32T retCode = 0;
    ClCpmInstanceT *cpmInstance = NULL;

    if (pSelectionObject == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null pointer passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     *  Checkout the handle and put the write handle there in the handle 
     *  structure. return the read fd to the caller
     */
    /*
     * Checkout the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle,
            (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
            rc);

    if(cpmInstance->readFd == 0)
    {
        retCode = pipe(fds);
        if (retCode != 0)
        {
            rc = CL_CPM_RC(CL_ERR_NO_RESOURCE);
            goto failure;
        }

        cpmInstance->readFd = fds[0];
        cpmInstance->writeFd = fds[1];
        
        *pSelectionObject = fds[0]; /* Read FD */
    }
    else
    {
        *pSelectionObject = cpmInstance->readFd; /* Read FD */
    }
    /*
     * Checkin the handle 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }

  failure:
    return rc;
}

void cpmInvokeCallback(ClCpmCallbackTypeT cbType,
                       void *param,
                       ClCpmInstanceT * cpmInstance)
{
    ClRcT rc = CL_OK;
    ClCpmCallbackTypeT cbType1;

    switch (cbType)
    {
        case CL_CPM_CB_HEALTHCHECK:
        {
            ClCpmClientCompTerminateT *info =
            (ClCpmClientCompTerminateT *)param;
            rc = handleComponentHealthcheck(info, cpmInstance, 0);
            break;
        }
        case CL_CPM_CB_TERMINATE:
        {
            ClCpmClientCompTerminateT *info =
                (ClCpmClientCompTerminateT *) param;
            rc = handleComponentTerminate(info, cpmInstance, 0);
            break;
        }
        case CL_CPM_CB_CSI_SET:
        {
            ClCpmCompCSISetT *recvBuff = (ClCpmCompCSISetT *)param;

            CL_CPM_INVOCATION_CB_TYPE_GET(recvBuff->invocation, cbType1);
            rc = handleCompCSIAssign(recvBuff, cpmInstance);
            /* No need to free the recvBuff, as it is cleanup in the handleCompCSIAssign */
            break;
        }
        case CL_CPM_CB_CSI_RMV:
        {
            ClCpmCompCSIRmvT    *recvBuff = (ClCpmCompCSIRmvT *)param;
            
            CL_CPM_INVOCATION_CB_TYPE_GET(recvBuff->invocation, cbType1);

            rc = handleCompCSIRmv(recvBuff, cpmInstance);
            break;
        }
        case CL_CPM_CB_PROXIED_INST:
        {
            ClCpmClientCompTerminateT *info =
                (ClCpmClientCompTerminateT *) param;
            rc = handleProxiedComponentInstantiate(info, cpmInstance, 0);
            break;
        }
        case CL_CPM_CB_PROXIED_CLEANUP:
        {
            ClCpmClientCompTerminateT *info =
                (ClCpmClientCompTerminateT *) param;
            rc = handleProxiedComponentCleanup(info, cpmInstance, 0);
            break;
        }
        case CL_CPM_CB_PROTECTION_TRACK:
        {
            ClCpmPGResponseT *info =
                (ClCpmPGResponseT *) param;
            rc = handlePGTrack(info, cpmInstance);
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

ClRcT clCpmDispatch(CL_IN ClCpmHandleT cpmHandle,
                    CL_IN ClDispatchFlagsT dispatchFlags)
{
    ClRcT rc = CL_OK;
    ClCpmInstanceT *cpmInstance = NULL;
    ClCpmCallbackQueueDataT *queueData = NULL;
    ClCpmCallbackQueueDataT tempQueueData = {0};
    ClCharT chr1;
    ClUint32T queueSize = 0;
    ssize_t size = 0;

    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    if (dispatchFlags == CL_DISPATCH_ONE)
    {
        /*
         * Dequeue the first pending request
         */
        cpmInstance->dispatchType = CL_DISPATCH_ONE;
        rc = clQueueSizeGet(cpmInstance->cbQueue, &queueSize);
        if (rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Unable to get queue Size %x\n", rc));
        /* 
         *  If there is nothing in the queue return 
         */
        if(queueSize == 0)
            goto failure;
        /* 
         *  read the pipe and proceed with the callback 
         */
        while ( (size = read(cpmInstance->readFd, (void *) &chr1, 1) ) < 0
                && (errno == EINTR) );
        
        if(size <= 0 ) 
            goto failure;

        if(chr1 == 'f' || cpmInstance->finalize)
        {
            goto failure;
        }
            
        rc = clOsalMutexLock(cpmInstance->cbMutex);
        if (rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to lock %x\n", rc));

        rc = clQueueNodeDelete(cpmInstance->cbQueue,
                               (ClQueueDataT *) &queueData);
        if (rc != CL_OK)
        {
            clOsalMutexUnlock(cpmInstance->cbMutex);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Unable to delete Queue node %x\n", rc));
            goto failure;
        }

        rc = clOsalMutexUnlock(cpmInstance->cbMutex);
        /*
         * process the CB 
         */
        memcpy(&tempQueueData, queueData, sizeof(tempQueueData));
        clHeapFree(queueData);
        cpmInvokeCallback(tempQueueData.cbType, tempQueueData.params, cpmInstance);
    }
    else if (dispatchFlags == CL_DISPATCH_ALL)
    {
        /*
         * FIXME dequeue the first pending request, process it dequeue the next 
         * pending request, process it drain the whole queue 
         */
        while(1)
        {
            cpmInstance->dispatchType = CL_DISPATCH_ALL;
            queueSize = 0;
            rc = clQueueSizeGet(cpmInstance->cbQueue, &queueSize);
            if (rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Unable to get queue Size %x\n", rc));
            /* 
             *  If there is nothing in the queue return 
             */
            if(queueSize == 0)
                goto failure;

            while(queueSize > 0)
            {
                rc = clOsalMutexLock(cpmInstance->cbMutex);
                if (rc != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc));

                rc = clQueueNodeDelete(cpmInstance->cbQueue,
                        (ClQueueDataT *) &queueData);
                if (rc != CL_OK)
                {
                    clOsalMutexUnlock(cpmInstance->cbMutex); 
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("Unable to delete Queue node %x\n", rc));
                    goto failure;
                }

                rc = clOsalMutexUnlock(cpmInstance->cbMutex);
                
                while ( (size = read(cpmInstance->readFd, (void *) &chr1, 1) ) < 0
                        && (errno == EINTR) );
                
                if(size <= 0 )
                    goto failure;

                if(chr1 == 'f' || cpmInstance->finalize)
                {
                    goto failure;
                }
                /*
                 * process the CB 
                 */
                memcpy(&tempQueueData, queueData, sizeof(tempQueueData));
                clHeapFree(queueData);
                cpmInvokeCallback(tempQueueData.cbType, tempQueueData.params, cpmInstance);
                queueSize--;
            }
        }
    }
    else if(dispatchFlags == CL_DISPATCH_BLOCKING)
    {
        /*
         * FIXME Block here, and keep processing the request as it comes in 
         */
        while (1)
        {
            cpmInstance->dispatchType = CL_DISPATCH_BLOCKING;
            while ( (size = read(cpmInstance->readFd, (void *) &chr1, 1) ) < 0 
                    && (errno == EINTR) );
            
            if(size <= 0) 
                goto failure;

            if(chr1 == 'f' || cpmInstance->finalize)
            {
                if(cpmInstance->readFd != 0)
                    close(cpmInstance->readFd);
                if(cpmInstance->writeFd != 0)
                    close(cpmInstance->writeFd);
                goto failure;
            }
            rc = clOsalMutexLock(cpmInstance->cbMutex);
            if (rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc));

            rc = clQueueNodeDelete(cpmInstance->cbQueue,
                    (ClQueueDataT *) &queueData);
            if (rc != CL_OK)
            {
                clOsalMutexUnlock(cpmInstance->cbMutex); 
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Unable to delete Queue node %x\n", rc));
                goto failure;
            }

            rc = clOsalMutexUnlock(cpmInstance->cbMutex);
            /*
             * process the CB 
             */
            memcpy(&tempQueueData, queueData, sizeof(tempQueueData));
            clHeapFree(queueData);
            cpmInvokeCallback(tempQueueData.cbType, tempQueueData.params, cpmInstance);
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }
    return rc;

  failure:
    return rc;
}

ClRcT clCpmComponentRegister(ClCpmHandleT cpmHandle,
                             const ClNameT *compName,
                             const ClNameT *proxyCompName)
{
    ClRcT rc = CL_OK;
    ClCpmCompRegisterT compReg = {{0}};
    ClCpmInstanceT *cpmInstance = NULL;

    /*
     * Do the Client side validation 
     */
    if (compName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    rc = component_handle_mapping_create(compName, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_HANDLE_MAP_CREATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to create compoennt-to-handle mapping %x\n", rc),
                     rc);
    /*
     * Add this comp to the cache.
     */
    rc = cpmCompCSIListAdd((ClNameT*)compName,cpmHandle);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
    {
        if (proxyCompName)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                       "The component [%.*s] is already registered and is now "
                       "being registered as proxied component with [%.*s] as a "
                       "proxy. Please note that second parameter to the component "
                       "register API is the name of the proxied component and not "
                       "proxy component",
                       compName->length, compName->value,
                       proxyCompName->length, proxyCompName->value);
        }
        else
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                       "The component [%.*s] is already registered",
                       compName->length, compName->value);
        }
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }
    else
    {
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to add component [%s] to cache. "
                                          "error [0x%x]",
                                          compName->value,
                                          rc),
                         rc);
    }

    if ((cpmInstance->callbacks).appTerminate == NULL)
    {
        clLogError("AMF", "CLN",
                   "Terminate callback not provided during "
                   "AMF initialization.");
        rc = CL_CPM_RC(CL_CPM_ERR_INIT);
        goto failure;
    }
    else if ((proxyCompName != NULL) &&
             ((cpmInstance->callbacks).appProxiedComponentInstantiate == NULL ||
              (cpmInstance->callbacks).appProxiedComponentCleanup == NULL))
    {
        clLogError("AMF", "CLN",
                   "Proxied instantiate and/or proxied cleanup callback(s) "
                   "not provided during AMF initialization.");
        rc = CL_CPM_RC(CL_CPM_ERR_INIT);
        goto failure;
    }

    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkin handle %x\n", rc), rc);

    /*
     * communicate the registration to the server
     */

    /*
     * Build the Buffer, to be send to the server 
     */
    /*
     * Copy the component Name 
     */
    memcpy(&(compReg.compName), compName, sizeof(ClNameT));
    /*
     * Copy the proxy component Name if the registered component is proxied 
     */
    if (proxyCompName != NULL)
        memcpy(&(compReg.proxyCompName), proxyCompName, sizeof(ClNameT));
    else
        compReg.proxyCompName.length = 0;

    /*
     * Send the Port info also, because the callback has to lend on the eoPort 
     * using which the registration is happening 
     */
    rc = clEoMyEoIocPortGet(&compReg.eoPort);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR, rc);
    }

    if(clEoWithOutCpm != CL_TRUE)
    {
        static ClUint8T priority = CL_IOC_CPM_INSTANTIATE_PRIORITY;
        rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), 
                                   CPM_COMPONENT_REGISTER,
                                   (ClUint8T *) &compReg,
                                   sizeof(ClCpmCompRegisterT),
                                   NULL,
                                   NULL,
                                   CL_RMD_CALL_ATMOST_ONCE,
                                   0,
                                   0,
                                   priority,
                                   MARSHALL_FN(ClCpmCompRegisterT, 4, 0, 0),
                                   NULL);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                    CL_CPM_LOG_1_CLIENT_COMP_REG_ERR, rc);
        }
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed \n rc =%x", __FUNCTION__, rc),
                     rc);
  failure:
    return rc;
}

ClRcT clCpmComponentUnregister(ClCpmHandleT cpmHandle,
                               const ClNameT *compName,
                               const ClNameT *proxyCompName)
{
    ClRcT rc = CL_OK;
    ClCpmCompRegisterT compReg = { {0} };
    ClCpmInstanceT *cpmInstance = NULL;

    if (compName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * Check-Out the handle 
     */
    rc = clHandleCheckout(handle_database, cpmHandle, (void *) &cpmInstance);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkout handle %x\n", rc),
                     rc);

    rc = cpmCompCSIListDel((ClNameT*)compName);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to delete Component [%s] from cache. "
                      "rc = [0x%x]",
                      compName->value,
                      rc),
                     rc);

    rc = component_handle_mapping_delete(compName, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_HANDLE_MAP_DELETE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR,
                     ("Unable to delete the component-to-handle-mapping %x\n",
                      rc), rc);

    /*
     * Decrement handle use count and return 
     */
    rc = clHandleCheckin(handle_database, cpmHandle);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_HANDLE_CHECKIN_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to checkin handle %x\n", rc), rc);

    memcpy(&(compReg.compName), compName, sizeof(ClNameT));
    if (proxyCompName != NULL)
        memcpy(&(compReg.proxyCompName), proxyCompName, sizeof(ClNameT));
    else
        compReg.proxyCompName.length = 0;

    /*
     * Send the Port info also, 
     */
    rc = clEoMyEoIocPortGet(&compReg.eoPort);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR, rc);
    }
    if(clEoWithOutCpm != CL_TRUE)
    {
        static ClUint32T priority = CL_IOC_CPM_TERMINATE_PRIORITY;
        rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(),
                                    CPM_COMPONENT_UNREGISTER,
                                    (ClUint8T *) &compReg,
                                    sizeof(ClCpmCompRegisterT),
                                    NULL,
                                    NULL,
                                    0,
                                    0,
                                    0,
                                    priority,
                                    MARSHALL_FN(ClCpmCompRegisterT, 4, 0, 0));
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                    CL_CPM_LOG_1_CLIENT_COMP_UNREG_ERR, rc);
        }
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed rc =%x\n", 
                    __FUNCTION__, rc), rc);
    }

  failure:
    return rc;
}

ClRcT clCpmComponentDNNameGet(ClCpmHandleT cpmHandle,
                              ClNameT *compName,
                              ClNameT *pDNName)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};
    ClAmsMgmtHandleT mgmtHandle = 0;
    ClAmsCompConfigT *pCompConfig = NULL;
    ClAmsSUConfigT *pSUConfig = NULL;
    ClVersionT version = {'B', 0x1, 0x1 };

    if(!compName || !pDNName)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    rc = clCpmComponentNameGet(cpmHandle, compName);
    if(rc != CL_OK)
        return rc;

    entity.type = CL_AMS_ENTITY_TYPE_COMP;
    clNameCopy(&entity.name, compName);
    rc = clAmsMgmtInitialize(&mgmtHandle, NULL, &version);
    if(rc != CL_OK)
        return rc;

    rc = clAmsMgmtEntityGetConfig(mgmtHandle, &entity, 
                                  (ClAmsEntityConfigT**)&pCompConfig);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    
    rc = clAmsMgmtEntityGetConfig(mgmtHandle, &pCompConfig->parentSU.entity,
                               (ClAmsEntityConfigT**)&pSUConfig);
    if(rc != CL_OK)
    {
        goto out_free;
    }

    snprintf(pDNName->value, sizeof(pDNName->value), 
             "safComp=%s,safSu=%s,safSg=%s", 
             compName->value, pSUConfig->entity.name.value, pSUConfig->parentSG.entity.name.value);

    pDNName->length = strlen(pDNName->value);
    
    out_free:
    clAmsMgmtFinalize(mgmtHandle);

    if(pCompConfig)
        clHeapFree(pCompConfig);

    if(pSUConfig)
        clHeapFree(pSUConfig);

    return rc;
}

/*
 * Note that this is a local function, NO RMD involved 
 */
ClRcT clCpmComponentNameGet(ClCpmHandleT cpmHandle,
                            ClNameT *compName)
{
    ClRcT rc = CL_OK;
    ClCharT *cName = NULL;

    if (compName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /* FIXME: This validation should be enabled in
     * the future.
     */
#if 0
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, compName->value, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }
#endif
    
    /*
     * FIXME: Should ideally be not getting the env variable every time rather 
     * it should get once only and for rest of the calls just copy it in to
     * user provided buffer 
     */
    /*
     * Get the environment variable and return to the caller 
     */
    cName = getenv("ASP_COMPNAME");
    if (cName != NULL)
    {
        strcpy(compName->value, cName);
        compName->length = strlen(compName->value);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_ERR_DOESNT_EXIST, rc);
    }

failure:
    return rc;
}

ClRcT clCpmHealthcheckStart(ClCpmHandleT cpmHandle,
                            const ClNameT *compName,
                            const ClAmsCompHealthcheckKeyT *healthcheckKey,
                            ClAmsCompHealthcheckInvocationT invocationType,
                            ClAmsRecoveryT recommendedRecovery)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    ClCpmInstanceT *cpmInstance = NULL;
    ClNameT appName = {0};
    
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Invalid CPM handle, error [%#x]", rc);
        goto failure;
    }

    if (!compName)
    {
        rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    if ((CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED != invocationType) &&
        (CL_AMS_COMP_HEALTHCHECK_CLIENT_INVOKED != invocationType))
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clHandleCheckout(handle_database, cpmHandle, (void **)&cpmInstance);
    if (CL_OK != rc) 
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Unable to checkout handle, error [%#x]", rc);
        goto failure;
    }

    if ((CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED == invocationType) &&
        (cpmInstance->callbacks.appHealthCheck == NULL))
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Health check is AMF invoked, but health check callback "
                   "is not specified.");
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }
    memset(&cpmInstance->healthcheckKey, 0, sizeof(cpmInstance->healthcheckKey));
    if(healthcheckKey)
    {
        memcpy(&cpmInstance->healthcheckKey, healthcheckKey, sizeof(cpmInstance->healthcheckKey));
    }

    rc = clHandleCheckin(handle_database, cpmHandle);
    if (CL_OK != rc) 
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Unable to checkin handle, error [%#x]", rc);
        goto failure;
    }

    clNameCopy(&cpmCompHealthcheck.compName, compName);
    rc = clCpmComponentNameGet(0, &appName);
    if (CL_OK != rc)
    {
        clLogError("CPM", "HB",
                   "Unable to get component name, rc = [%#x]",
                   rc);
        goto failure;
    }
    clNameCopy(&cpmCompHealthcheck.proxyCompName, &appName);
    cpmCompHealthcheck.invocationType = invocationType;
    cpmCompHealthcheck.recommendedRecovery = recommendedRecovery;
    
    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(),
                                CPM_COMPONENT_HEALTHCHECK_START,
                                (ClUint8T *) &cpmCompHealthcheck,
                                sizeof(ClCpmCompHealthcheckT),
                                NULL,
                                NULL,
                                0,
                                0,
                                0,
                                0,
                                MARSHALL_FN(ClCpmCompHealthcheckT, 4, 0, 0));
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Unable to make an RMD, error [%#x]", rc);
        goto failure;
    }

    return CL_OK;

failure:
    return rc;
}

ClRcT clCpmHealthcheckStop(ClCpmHandleT cpmHandle,
                           const ClNameT *compName,
                           const ClAmsCompHealthcheckKeyT *compHealthCheck)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Invalid CPM handle, error [%#x]", rc);
        goto failure;
    }

    clNameCopy(&cpmCompHealthcheck.compName, compName);

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(),
                                CPM_COMPONENT_HEALTHCHECK_STOP,
                                (ClUint8T *) &cpmCompHealthcheck,
                                sizeof(ClCpmCompHealthcheckT),
                                NULL,
                                NULL,
                                0,
                                0,
                                0,
                                0,
                                MARSHALL_FN(ClCpmCompHealthcheckT, 4, 0, 0));
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Unable to make an RMD, error [%#x]", rc);
        goto failure;
    }

    return CL_OK;

failure:
    return rc;
}

ClRcT clCpmHealthcheckConfirm(ClCpmHandleT cpmHandle,
                              const ClNameT *compName,
                              const ClAmsCompHealthcheckKeyT *compHealthCheck,
                              ClRcT healthcheckResult)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Invalid CPM handle, error [%#x]", rc);
        goto failure;
    }
    
    clNameCopy(&cpmCompHealthcheck.compName, compName);
    cpmCompHealthcheck.healthcheckResult = healthcheckResult;
    
    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(),
                                CPM_COMPONENT_HEALTHCHECK_CONFIRM,
                                (ClUint8T *) &cpmCompHealthcheck,
                                sizeof(ClCpmCompHealthcheckT),
                                NULL,
                                NULL,
                                0,
                                0,
                                0,
                                0,
                                MARSHALL_FN(ClCpmCompHealthcheckT, 4, 0, 0));
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Unable to make an RMD, error [%#x]", rc);
        goto failure;
    }

failure:
    return rc;
}

ClRcT clCpmComponentFailureReport(ClCpmHandleT cpmHandle,
                                  const ClNameT *compName,
                                  ClTimeT errorDetectionTime,
                                  ClAmsLocalRecoveryT recommendedRecovery,
                                  ClUint32T alarmHandle)
{
    return clCpmComponentFailureReportWithCookie(cpmHandle, compName, 0,
                                                 errorDetectionTime, recommendedRecovery,
                                                 alarmHandle);
}

ClRcT clCpmComponentFailureClear(CL_IN ClCpmHandleT cpmHandle,
                                 CL_IN ClNameT *compName)
{
    ClRcT rc = CL_OK;
    ClErrorReportT *errorReport = NULL;
    ClUint32T tempLength = 0;

    if (compName == NULL )
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * This should be enabled in future.
     */
#if 0
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, compName->value, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }
#endif

    errorReport =
        (ClErrorReportT *) clHeapAllocate(sizeof(ClErrorReportT));
                                        
    if (errorReport == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    }
    memcpy(&(errorReport->compName), compName, sizeof(ClNameT));
    errorReport->instantiateCookie = 0;
    tempLength = sizeof(ClErrorReportT); 
    rc = clCpmClientRMDAsync(clIocLocalAddressGet(),
                             CPM_COMPONENT_FAILURE_CLEAR,
                             (ClUint8T *) errorReport, tempLength, NULL, NULL,
                             CL_RMD_CALL_ATMOST_ONCE, 0, 0, 0);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_FAILURE_CLEAR_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc),
                     rc);

    return rc;
  failure:
    if (errorReport == NULL)
    {
        clHeapFree(errorReport);
        errorReport = NULL;
    }
    return rc;
}

ClRcT clCpmHAStateGet(CL_IN ClCpmHandleT cpmHandle,
                      CL_IN ClNameT *compName,
                      CL_IN ClNameT *csiName,
                      CL_OUT ClAmsHAStateT *haState)
{
    ClCpmHAStateGetSendT sendBuff;
    ClCpmHAStateGetRecvT recvBuff;
    ClUint32T sendLength = sizeof(ClCpmHAStateGetSendT);
    ClUint32T recvLength = sizeof(ClCpmHAStateGetRecvT);
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterIocAddress = 0;

    if (compName == NULL || csiName == NULL || haState == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, compName->value, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }

    /*
     * Prepare the buffer 
     */
    memcpy(&(sendBuff.compName), compName, sizeof(ClNameT));
    memcpy(&(sendBuff.csiName), csiName, sizeof(ClNameT));
    sendBuff.cpmHandle = cpmHandle;

    /*
     * Direct ot to CPM\G, as in this case, it is not required to be directed
     * through the CPM\L 
     */
    rc = clCpmMasterAddressGet(&masterIocAddress);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                "Failed to get master CPM address. error code [0x%x].", rc);
        goto failure;
    }

    rc = clCpmClientRMDSyncNew(masterIocAddress,
                               CPM_COMPONENT_HA_STATE_GET,
                               (ClUint8T *) &sendBuff,
                               sendLength,
                               (ClUint8T *) &recvBuff,
                               &(recvLength),
                               CL_RMD_CALL_NEED_REPLY,
                               0,
                               0,
                               0,
                               MARSHALL_FN(ClCpmHAStateGetSendT, 4, 0, 0),
                               UNMARSHALL_FN(ClCpmHAStateGetRecvT, 4, 0, 0));
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_HA_STATE_GET_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc),
                     rc);

    *haState = recvBuff.haState;

  failure:
    return rc;
}

ClRcT clCpmResponse(CL_IN ClCpmHandleT cpmHandle,
                    CL_IN ClInvocationT invocation,
                    CL_IN ClRcT retCode)
{
    ClRcT               rc = CL_OK;
    ClCpmResponseT      sendBuff;
    ClUint32T           cbType = 0;
    ClUint32T priority = 0;

    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, "component", rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }

    sendBuff.cpmHandle = cpmHandle;
    sendBuff.invocation = invocation;
    sendBuff.retCode = retCode;
    
    CL_CPM_INVOCATION_CB_TYPE_GET(sendBuff.invocation, cbType);
    priority = CL_IOC_CPM_PRIORITY(cbType);
    
    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_CB_RESPONSE,
            (ClUint8T *) &sendBuff, sizeof(ClCpmResponseT),
            NULL, NULL, 0, 0, 0, priority,
            MARSHALL_FN(ClCpmResponseT, 4, 0, 0)
            );

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_CB_RESPONSE_ERR, rc);
    }

    if (cbType == CL_CPM_TERMINATE_CALLBACK && componentTerminate == CL_TRUE)
    {
        rc = clCpmComponentResourceCleanup();
    }

  failure:
    return rc;
}

ClRcT clCpmCSIQuiescingComplete(CL_IN ClCpmHandleT cpmHandle,
                                CL_IN ClInvocationT invocation,
                                CL_IN ClRcT retCode)
{
    ClRcT rc = CL_OK;
    ClCpmQuiescingCompleteT sendBuff;
    static ClUint32T priority = CL_IOC_CPM_CSIQUIESCING_PRIORITY;

    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, "component", rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }

    sendBuff.cpmHandle = cpmHandle;
    sendBuff.invocation = invocation;
    sendBuff.retCode = retCode;
    
    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(),
            CPM_COMPONENT_QUIESCING_COMPLETE,
            (ClUint8T *) &sendBuff,
            sizeof(ClCpmQuiescingCompleteT), NULL, NULL, 0,
            0, 0, priority, MARSHALL_FN(ClCpmQuiescingCompleteT, 4, 0, 0)
            );
    
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_QUIESCING_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc),
                     rc);

  failure:
    return rc;
}

ClRcT marshallPGRequest(void *data, ClBufferHandleT inMsgHdl, ClUint32T length)
{
    return clBufferNBytesWrite(inMsgHdl, data, sizeof(ClCpmPGTrackT));
}

ClRcT unmarshallPGResponse(ClBufferHandleT outMsgHandle, void* notification)
{
    ClUint32T   notificationSize = 0; 
    ClRcT   rc = CL_OK;
    ClAmsPGNotificationBufferT *notificationBuffer = 
        (ClAmsPGNotificationBufferT *)notification;

    /*
     * Assign the notification Buffer 
     */
    rc = clXdrUnmarshallClUint32T(outMsgHandle, 
            (void *)&notificationBuffer->numItems);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

    if (notificationBuffer->notification == NULL)
    {
        /*
         * Allocate memory for the notification, this MUST not be freed by
         * us, it is the application responsibility to free this memory 
         */
        notificationSize =
            (notificationBuffer->numItems) * sizeof(ClAmsPGNotificationT);;
        notificationBuffer->notification = clHeapAllocate(notificationSize);

        if (notificationBuffer->notification == NULL)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory"),
                    CL_CPM_RC(CL_ERR_NO_MEMORY));
        }
        else
        {
            ClUint32T i = 0;
            ClAmsPGNotificationT amsPGNotification;

            for (i = 0; i < notificationBuffer->numItems; ++i) 
            {
                rc = VDECL_VER(clXdrUnmarshallClAmsPGNotificationT, 4, 0, 0)(outMsgHandle,
                        (void *)&amsPGNotification);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
                memcpy(&(notificationBuffer->notification[i]), 
                        &amsPGNotification, sizeof(ClAmsPGNotificationT)); 
            }
        }
    }

failure:
    return rc;
}

ClRcT clCpmProtectionGroupTrack(CL_IN ClCpmHandleT cpmHandle,
                                CL_IN ClNameT *csiName,
                                CL_IN ClUint8T trackFlags,
                                CL_INOUT ClAmsPGNotificationBufferT
                                *notificationBuffer)
{
    ClRcT               rc = CL_OK;
    ClCpmInstanceT      *cpmInstance = NULL;
    ClCpmPGTrackT       sendBuff;
    ClUint32T           outPutLength = 0;
    ClIocNodeAddressT   masterIocAddress;

    if (csiName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, csiName->value, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }

    rc = clHandleCheckout(handle_database, cpmHandle, (void **)&cpmInstance);
    if (CL_OK != rc) 
    {
        clLogError("AMF", "CLN", "Unable to checkout handle, error [%#x]", rc);
        goto failure;
    }

    if (!(cpmInstance->callbacks.appProtectionGroupTrack))
    {
        clLogError("AMF", "CLN",
                   "Protection group track callback not provided during "
                   "AMF client initialization.");
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clHandleCheckin(handle_database, cpmHandle);
    if (CL_OK != rc) 
    {
        clLogError("AMF", "CLN", "Unable to checkin handle, error [%#x]", rc);
        goto failure;
    }

    sendBuff.cpmHandle = cpmHandle;
    memcpy(&(sendBuff.csiName), csiName, sizeof(ClNameT));
    sendBuff.trackFlags = trackFlags;
    
    sendBuff.iocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
    sendBuff.iocAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&sendBuff.iocAddress.clIocAddressIDLT.iocPhyAddress.portId);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB, "Failed to get my EO port. error code [0x%x].", rc);
        goto failure;
    }

    rc = clCpmMasterAddressGet(&masterIocAddress);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB, "Failed to get master CPM address. error code [0x%x].",rc);
        goto failure;
    }
    if ((trackFlags & CL_AMS_PG_TRACK_CURRENT) && notificationBuffer != NULL)
    {
        sendBuff.responseNeeded = CL_TRUE;
        rc = clCpmClientRMDSyncNew(masterIocAddress,
                                   CPM_COMPONENT_PGTRACK,
                                   (ClUint8T *) &sendBuff,
                                   sizeof(ClCpmPGTrackT),
                                   (ClUint8T *) notificationBuffer,
                                   &outPutLength, 
                                   CL_RMD_CALL_NEED_REPLY,
                                   0,
                                   0,
                                   0,
                                   MARSHALL_FN(ClCpmPGTrackT, 4, 0, 0), 
                                   unmarshallPGResponse);
    }
    else
    {
        sendBuff.responseNeeded = CL_FALSE;
        rc = clCpmClientRMDSyncNew(masterIocAddress,
                                   CPM_COMPONENT_PGTRACK,
                                   (ClUint8T *) &sendBuff,
                                   sizeof(ClCpmPGTrackT),
                                   NULL,
                                   NULL,
                                   0,
                                   0,
                                   0,
                                   0, 
                                   MARSHALL_FN(ClCpmPGTrackT, 4, 0, 0),
                                   NULL);
    }

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s : RMD to node [0x%x] failed. error code = [0x%x]\n", __FUNCTION__, masterIocAddress, rc),
                     rc);
  failure:
    return rc;
}


ClRcT clCpmProtectionGroupTrackStop(CL_IN ClCpmHandleT cpmHandle,
                                    CL_IN ClNameT *csiName)
{
    ClRcT rc = CL_OK;
    ClCpmPGTrackStopT sendBuff;
    ClIocNodeAddressT masterIocAddress;
    
    if (csiName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, csiName->value, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invalid CPM handle, rc = [%#x]", rc));
        goto failure;
    }

    sendBuff.cpmHandle = cpmHandle;
    memcpy(&(sendBuff.csiName), csiName, sizeof(ClNameT));

    sendBuff.iocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
    sendBuff.iocAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&sendBuff.iocAddress.clIocAddressIDLT.iocPhyAddress.portId);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB, "Failed to get my EO port. error code [0x%x].", rc);
        goto failure;
    }

    rc = clCpmMasterAddressGet(&masterIocAddress);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB, "Failed to get master CPM address. erro code [0x%x].",rc);
        goto failure;
    }
    rc = clCpmClientRMDAsyncNew(masterIocAddress,
                                CPM_COMPONENT_PGTRACK_STOP,
                                (ClUint8T *) &sendBuff,
                                sizeof(ClCpmPGTrackStopT),
                                NULL,
                                NULL,
                                0,
                                0,
                                0,
                                0,
                                MARSHALL_FN(ClCpmPGTrackStopT, 4, 0, 0));

    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("%s : RMD to node [0x%x] failed. error code = [0x%x].\n", 
                __FUNCTION__, masterIocAddress, rc), rc);
  failure:
    return rc;
}

static ClRcT cpmCompCSIListWalk(ClCntKeyHandleT key,
                                ClCntDataHandleT data,
                                ClCntArgHandleT arg,
                                ClUint32T argSize)
{
    ClCpmCompCSIT *pCSI = (ClCpmCompCSIT*) data;
    ClCpmCompCSIRefT *pCSIRef = (ClCpmCompCSIRefT*) arg;
    ClUint32T numCSI = pCSIRef->numCSIs;
    memcpy(&pCSIRef->pCSIList[numCSI], pCSI, sizeof(*pCSI));
    ++pCSIRef->numCSIs;
    return CL_OK;
}

ClRcT clCpmCompCSIList(const ClNameT *pCompName, ClCpmCompCSIRefT *pCSIRef)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    ClUint32T numCSIs = 0;
    ClCpmCompCSIListT *pCompCSIList = NULL;

    if(!pCompName || !pCSIRef || pCSIRef->pCSIList)
    {
        return rc;
    }

    clOsalMutexLock(gCpmCompCSIListMutex);
    rc = cpmCompCSIListGet((ClNameT*)pCompName,  &pCompCSIList, NULL);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Comp [%.*s] not found. rc [%#x]\n",
                                      pCompName->length, pCompName->value, rc),
                     rc);
    rc = clCntSizeGet(pCompCSIList->csiList, &numCSIs);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Comp CSI size get returned [%#x]\n", rc),
                     rc);
    pCSIRef->pCSIList = clHeapCalloc(numCSIs, sizeof(*pCSIRef->pCSIList));
    if(pCSIRef->pCSIList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No memory\n"));
        goto failure;
    }

    rc = clCntWalk(pCompCSIList->csiList, cpmCompCSIListWalk,
                   (ClCntArgHandleT) pCSIRef, 0);
    
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Comp CSI walk error. rc [%#x]\n", rc),
                     rc);

    failure:
    clOsalMutexUnlock(gCpmCompCSIListMutex);
    return rc;
}
