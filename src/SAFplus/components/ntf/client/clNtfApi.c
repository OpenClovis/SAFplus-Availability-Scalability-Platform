#include <clNtfApi.h>
#include <clNtfUtil.h>
#include <clNtfXdr.h>

/******************************************************************************
 * LOCAL TYPES AND VARIABLES
 ******************************************************************************/
/* instance handle database */
ClHandleDatabaseHandleT handleDbHandle;

/* notification handle database */
ClHandleDatabaseHandleT notificationHandleDbHandle;

/* notification filter handle database */
ClHandleDatabaseHandleT ntfFilterHandleDbHandle;

/* Event service handle */
/* Event needs to be initialized only once per EO, and hence we
 * dont do it in ntfInitialize */
ClEventHandleT     evtHandle;

/* Event publish channel handle */
ClEventChannelHandleT  evtPublishChannelHdl;

/* Event publish channel handle */
ClEventChannelHandleT  evtSubscribeChannelHdl;

/* Publisher name */
ClNameT            eventPublisher = {sizeof(CL_NTF_EVENT_PUBLISHER_NAME)-1, CL_NTF_EVENT_PUBLISHER_NAME};
/* File descriptor variable used to store fd value for the 
 * notification ID counter file fd */
ClFdT   ntfIdFileFd;

/* IOC address of this node */
ClIocNodeAddressT   thisNodeId;

/* Flag to show if library is initialized */
static ClBoolT lib_initialized = CL_FALSE;

/*
 * Versions supported
 */
static ClVersionT versions_supported[] = {
	{ 'A', 3, 1 }
};

static ClVersionDatabaseT version_database = {
	sizeof (versions_supported) / sizeof (ClVersionT),
	versions_supported
};

/* Macro to check whether library has been initilized or not */
#define CHECK_LIB_INIT  do {            \
    if (lib_initialized == FALSE)       \
    {                                   \
        return CL_ERR_NOT_INITIALIZED;  \
    }                                   \
}while (0)

/******************************************************************************
 * FUNCTION DECLARATIONS
 ******************************************************************************/
/* Callback function used to register to event service. We are using event service
 * as the transport to implement the NTF service */
static void 
clNtfEventDeliveryCallback(
        ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT         eventHandle,
        ClSizeT                eventDataSize);

static void
_ntfFreeNotificationHeaderAndVarSizeDataElements(
        ClNtfNotificationHeaderBufferT *ntfHeader,
        ClPtrT                          variableDataPtr);

static void 
_ntfFreeFilterNtfHeaderBufferElements(ClNtfNotificationFilterHeaderBufferT *filterHdrBuf);

/******************************************************************************
 * STATIC FUNCTION DEFINITIONS
 ******************************************************************************/
/* Destructor to be called when deleting NTF svc handles */
void ntf_handle_instance_destructor(void *unused)
{
    /* NO OP */
    return;
}

/* Destructor to be called when deleting notification handles */
void notification_handle_instance_destructor(void *unused)
{
    /* NO OP */
    return;
}

/* Destructor to be called when deleting notification filter handles */
void notification_filter_handle_instance_destructor(void *unused)
{
    /* NO OP */
    return;
}

/* Library initialization function. It creates the handle databases for
 * svc handles, notification and notification filter handles. It also
 * initializes event and opens the event channels for notification
 * publish and subscribe */
ClRcT 
clNtfLibInitialize(void)
{
    ClRcT               rc = CL_OK;
    ClVersionT          evtVersion = {'B', 0x01, 0x01};
    ClEventCallbacksT   eventCallbacks = {
                        NULL,
                        clNtfEventDeliveryCallback };
    ClNameT             evtChannelName = { CL_NTF_EVENT_CHANNEL_NAME_SIZE,
                                           CL_NTF_EVENT_CHANNEL_NAME };
    
    clLogDebug("NTF","INIT","Initializing the NTF library");
    if (lib_initialized)
    {
        return CL_OK;
    } else {
        /* Create the handle database used for NTF SVC handles */
        rc = clHandleDatabaseCreate(ntf_handle_instance_destructor,
                                    &handleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","INIT","Handle database creation failed with rc 0x%x",rc);
            return rc;
        }
        
        /* Create handle database used for notification handles */
        rc = clHandleDatabaseCreate(notification_handle_instance_destructor,
                                    &notificationHandleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","INIT","Handle database creation failed with rc 0x%x",rc);
            return rc;
        }

        /* Create handle database used for notification filter handles */
        rc = clHandleDatabaseCreate(notification_filter_handle_instance_destructor,
                                    &ntfFilterHandleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","INIT","Handle database creation failed with rc 0x%x",rc);
            return rc;
        }
        /* No need to install any RMD tables as we dont make any direct RMD calls
         * here. Instead we will use event and alarm client libraries
         */

        /* Initialize the event library and open the Global channel for
         * event publisher and subscriber.
         */
        rc = clEventInitialize(&evtHandle, &eventCallbacks, &evtVersion);
        if (rc != CL_OK)
        {
            clLogError("NTF","INIT","Event initialize failed with rc 0x%x",rc);
            goto error_exit;
        }

        /* Open the global event channel for publish */
        rc = clEventChannelOpen(evtHandle,
                                &evtChannelName, 
                                CL_EVENT_GLOBAL_CHANNEL|CL_EVENT_CHANNEL_PUBLISHER|CL_EVENT_CHANNEL_CREATE,
                                (ClTimeT)-1,
                                &evtPublishChannelHdl);
        if (rc != CL_OK)
        {
            clLogError("NTF","INIT","Event channel open failed with rc 0x%x",rc);
            goto error_exit;
        }

        /* Open the global event channel for subscribe */
        rc = clEventChannelOpen(evtHandle,
                                &evtChannelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL,
                                (ClTimeT) -1,
                                &evtSubscribeChannelHdl);

        /* Open the file used to generate the notificationID */
        ntfIdFileFd = clNtfNotificationIdFileOpen();
        if (ntfIdFileFd == -1)
        {
            clLogError("NTF","INIT","Notification ID counter file open failed");
            rc = CL_ERR_LIBRARY;
            goto error_exit;
        }

        /* Get the node address */
        thisNodeId = clIocLocalAddressGet();
        
        /* This node ID should never be 0 */
        CL_ASSERT(thisNodeId != 0);

        lib_initialized = CL_TRUE;
        return rc;
    }
        
error_exit:
    clHandleDatabaseDestroy(handleDbHandle);
    return rc;
}

/* Library finalize function. */
ClRcT 
clNtfLibFinalize(void)
{
    ClRcT rc = CL_OK;
    
    clLogDebug("NTF","FIN","Finalizing the NTF library");
    if (lib_initialized == CL_TRUE)
    {
        lib_initialized = CL_FALSE;
        /* Destroy SVC handle database */
        rc = clHandleDatabaseDestroy(handleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","SVC Handle database destroy failed with rc 0x%x",rc);
            /* We dont want return on error, as it is finalize path. Just continue
             * to finalize */
        }

        /* Destroy notification handle database */
        rc = clHandleDatabaseDestroy(notificationHandleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","Notification Handle database destroy failed with rc 0x%x",rc);
            /* We dont want return on error, as it is finalize path. Just
             * attempt to finalize everything even on error
             */
        }

        /* Destroy notification filter handle database */
        rc = clHandleDatabaseDestroy(ntfFilterHandleDbHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","Notification Filter Handle database destroy failed with rc 0x%x",rc);
            /* We dont want return on error, as it is finalize path. Just
             * attempt to finalize everything even on error
             */
        }

        /* Close the publisher event channel */
        rc = clEventChannelClose(evtPublishChannelHdl);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","Publisher Event channel close failed with rc 0x%x",rc);
        }

        /* Close the subscriber event channel */
        rc = clEventChannelClose(evtSubscribeChannelHdl);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","Subscriber Event channel close failed with rc 0x%x",rc);
        }

        /* Finalize event library */
        rc = clEventFinalize(evtHandle);
        if (rc != CL_OK)
        {
            clLogError("NTF","FIN","Event finalize failed with rc 0x%x",rc);
        }

        /* Close the notification ID file descriptor */
        clNtfNotificationIdFileClose(ntfIdFileFd);
    }

    return rc;
}

/* Wrapper callback to work with dispatch library */
static void 
dispatchWrapperCallback (
        ClHandleT  svcHandle,
        ClUint32T  callbackType,
        void*      callbackData)
{
    /* Checkout the svc handle */
    ClRcT rc = CL_OK;
    ClNtfLibInstanceT   *ntfInstancePtr = NULL;

    clLogDebug("NTF","DIS","dispatch wrapper callback is invoked for %s",
               (callbackType ==  CL_NTF_NOTIFICATION_CALLBACK) ? "NOTIFICATION_CALLBACK": 
               (callbackType == CL_NTF_NOTIFICATION_DISCARDED_CALLBACK) ? "DISCARDED_CALLBACK" :
               (callbackType == CL_NTF_SUPPRESSION_FILTER_CALLBACK) ?"SUPPRESSION_FILTER_CALLBACK" : "UNKNOWN");

    rc = clHandleCheckout(handleDbHandle, svcHandle, (void *)&ntfInstancePtr);
    if (rc != CL_OK)
    {
        clLogError("NTF","DIS","Failed to checkout the NTF svc handle. May be its already finalized"
                            " Callback cannot be invoked");
        goto out_free;
    }

    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","DIS","handle pointer is found to be NULL after handle checkout");
        goto out_free;
    }

    /* Based on the callback type, invoke
     * the particular callback */
    switch (callbackType)
    {
        case CL_NTF_NOTIFICATION_CALLBACK:
            {
                clLogDebug("NTF","DIS","Invoking the notification callback");
                ntfInstancePtr->callbacks.saNtfNotificationCallback(
                        ((ClNtfNotificationCallbackDataT*)callbackData)->subscriptionId,
                        ((ClNtfNotificationCallbackDataT*)callbackData)->notificationPtr);
            }
            break;
        case CL_NTF_NOTIFICATION_DISCARDED_CALLBACK:
            /* Not implemented yet */
            break;
        case CL_NTF_SUPPRESSION_FILTER_CALLBACK:
            /* Not implemented yet */
            break;
        default:
            clLogError("NTF","","Invalid callback type received in the dispatch callback");
    }

    /* Checkin the handle and return */
    clHandleCheckin(handleDbHandle, svcHandle);

out_free:
    if(callbackData)
        clHeapFree(callbackData);
    return;
}

/* Callback called when the dispatch queue is destroyed */
static void  dispatchQDestroyCallback (ClUint32T callbackType,
                                       void*     callbackData)
{
    /* If finalize is invoked while there are pending callbacks
     * we need to free the callback */
    if (callbackData)
        clHeapFree(callbackData);
    return;
}

/******************************************************************************
 * NTF LIFE CYCLE APIs
 *
 * * saNtfInitialize_3()
 * * saNtfSelectionObjectGet()
 * * saNtfDispatch()
 * * saNtfFinalize()
 *
 ******************************************************************************/

SaAisErrorT 
saNtfInitialize_3(
    SaNtfHandleT *ntfHandle,
    const SaNtfCallbacksT_3 *ntfCallbacks,
    SaVersionT *version)
{
    ClRcT                rc = CL_OK;
    ClNtfLibInstanceT   *ntfInstancePtr = NULL;
    ClHandleT            dispatchHandle = CL_HANDLE_INVALID_VALUE;


    /* Check for the input values */
    if ((ntfHandle == NULL) || (version == NULL))
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /* Check if the library is already initialized */
    if (lib_initialized == CL_FALSE)
    {
        rc = clNtfLibInitialize();
        if (rc != CL_OK)
        {
            return SA_AIS_ERR_LIBRARY;
        }
    }

    /* Verify the version compatability */
    rc = clVersionVerify (&version_database, (ClVersionT*)version);
    if (rc != CL_OK)
    {
        return SA_AIS_ERR_VERSION;
    }

    /* Create the handle */
    rc = clHandleCreate(handleDbHandle, sizeof(ClNtfLibInstanceT), ntfHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","INIT","Handle create failed during initialization");
        goto error_no_destroy;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, *ntfHandle, (void *)&ntfInstancePtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen as we just created the handle */

    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","INIT","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    if (ntfCallbacks != NULL)
    {
        memcpy(&ntfInstancePtr->callbacks, ntfCallbacks, sizeof(SaNtfCallbacksT_3));
    } else {
        memset(&ntfInstancePtr->callbacks, 0, sizeof(SaNtfCallbacksT_3));
    }

    /* Register with dispatch client */
    rc = clDispatchRegister(&dispatchHandle,
                             *ntfHandle,
                             dispatchWrapperCallback,
                             dispatchQDestroyCallback);
    if (rc != CL_OK)
    {
        clLogError("NTF","INIT","Dispatch register failed with rc 0x%x",rc);
        clHandleCheckin(handleDbHandle, *ntfHandle);
        clHandleDestroy(handleDbHandle, *ntfHandle);
        return _aspErrToAisError(rc);
    }

    ntfInstancePtr->dispatchHandle = dispatchHandle;

    clHandleCheckin(handleDbHandle, *ntfHandle);

error_no_destroy:
    /* No need to destroy the handle, just return */
    return _aspErrToAisError(rc);
}

SaAisErrorT
saNtfSelectionObjectGet(
    SaNtfHandleT ntfHandle,
    SaSelectionObjectT *selectionObject)
{
    ClRcT                rc = CL_OK;
    ClNtfLibInstanceT   *ntfInstancePtr = NULL;

    /* Check for valid input parameters */
    if (selectionObject == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","SEL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    rc = clDispatchSelectionObjectGet(ntfInstancePtr->dispatchHandle,
                                      selectionObject);
    if (rc != CL_OK)
    {
        clLogError("NTF","SEL","clDispatchSelectionObjectGet failed with rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }

    clHandleCheckin(handleDbHandle, ntfHandle);

    return SA_AIS_OK;
}

SaAisErrorT 
saNtfDispatch(
    SaNtfHandleT ntfHandle,
    SaDispatchFlagsT dispatchFlags)
{
    ClRcT                rc = CL_OK;
    ClNtfLibInstanceT   *ntfInstancePtr = NULL;

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","DIS","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    rc = clDispatchCbDispatch(ntfInstancePtr->dispatchHandle,
                              dispatchFlags);
    if (rc != CL_OK)
    {
        clLogError("NTF","LC","clDispatchCbDispatch failed with rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }

    clHandleCheckin(handleDbHandle, ntfHandle);
    return SA_AIS_OK;
}


SaAisErrorT 
saNtfFinalize(
    SaNtfHandleT ntfHandle)
{
    ClRcT                rc = CL_OK;
    ClNtfLibInstanceT   *ntfInstancePtr = NULL;

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }
    
    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","FIN","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    ntfInstancePtr->finalized = CL_TRUE;

    /* Deregister with dispatch client */
    rc = clDispatchDeregister(ntfInstancePtr->dispatchHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","FIN","Dispatch deregister failed with rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }

    clHandleCheckin(handleDbHandle, ntfHandle);

    /* Destroy the handle */
    rc = clHandleDestroy(handleDbHandle, ntfHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","FIN","Handle destroy failed during finalize");
        return _aspErrToAisError(rc);
    }
    return _aspErrToAisError(rc);
}


/******************************************************************************
 * NOTIFICATION ALLOCATE APIs
 *
 * * saNtfObjectCreateDeleteNotificationAllocate()
 * * saNtfAttributeChangeNotificationAllocate()
 * * saNtfStateChangeNotificationAllocate()
 * * saNtfAlarmNotificationAllocate()
 * * saNtfSecurityAlarmNotificationAllocate()
 *
 ******************************************************************************/

static ClRcT 
_ntfCreateNotificationHandleAndHeaderBuffer(SaNtfHandleT                ntfHandle,
                                            SaNtfNotificationTypeT      notifType,
                                            ClInt32T                    handleSize,
                                            SaNtfNotificationHandleT   *notificationHandle,
                                            SaUint16T                   numCorrelatedNotifications,
                                            SaUint16T                   lengthAdditionalText,
                                            SaUint16T                   numAdditionalInfo,
                                            SaInt16T                    variableDataSize,
                                            SaNtfNotificationHeaderT    *notificationHeader)
{
    ClRcT                                     rc = CL_OK;
    ClNtfLibInstanceT                        *ntfInstancePtr = NULL;
    void                                     *notificationPtr = NULL;
    ClNtfNotificationHeaderBufferT           *ntfHeader = NULL;
    SaInt16T                                 *variableDataSizePtr = NULL;
    SaUint16T                                *allocatedVariableDataSize = NULL;
    ClPtrT                                   *variableDataPtr = NULL;
    SaUint16T                                *usedVariableDataSize = NULL;


    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return rc;
    }

    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        return CL_ERR_LIBRARY;
    }

    /* Create a handle with give handleSize */
    rc = clHandleCreate(notificationHandleDbHandle, handleSize, notificationHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","NAL","Handle create failed while allocating notification");
        goto error_no_destroy;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, *notificationHandle, &notificationPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        rc=CL_ERR_LIBRARY;
        goto destroy_no_free;
    }

    /* memset the notificationPtr */
    memset(notificationPtr, 0, handleSize);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            ((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            ((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfAttributeChangeNotificationBufferT*)notificationPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            ((ClNtfStateChangeNotificationBufferT*)notificationPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfStateChangeNotificationBufferT*)notificationPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfStateChangeNotificationBufferT*)notificationPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfStateChangeNotificationBufferT*)notificationPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfStateChangeNotificationBufferT*)notificationPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfStateChangeNotificationBufferT*)notificationPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_ALARM:
            ((ClNtfAlarmNotificationBufferT*)notificationPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfAlarmNotificationBufferT*)notificationPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfAlarmNotificationBufferT*)notificationPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfAlarmNotificationBufferT*)notificationPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfAlarmNotificationBufferT*)notificationPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfAlarmNotificationBufferT*)notificationPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            ((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfSecurityAlarmNotificationBufferT*)notificationPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            clLogError("NTF","NAL","SA_NTF_TYPE_MISCELLANEOUS notifications are not supported");
            rc = SA_AIS_ERR_NOT_SUPPORTED;
            goto destroy_no_free;
        default:
            clLogError("NTF","NAL","Invalid notification type passed to allocate function");
            rc = SA_AIS_ERR_INVALID_PARAM;
            goto destroy_no_free;
    }


    /* Initialize the ntfHeader attributes */
    ntfHeader->notificationHandle = *notificationHandle;
    ntfHeader->numCorrelatedNotifications = numCorrelatedNotifications;
    ntfHeader->lengthAdditionalText = lengthAdditionalText;
    ntfHeader->numAdditionalInfo = numAdditionalInfo;

    /* Initialize variable data values */
    *variableDataSizePtr = variableDataSize;
    if (*variableDataSizePtr == SA_NTF_ALLOC_SYSTEM_LIMIT)
    {
        *allocatedVariableDataSize = CL_NTF_SYSTEM_LIMIT_VAR_DATA_SIZE;
    } else {
        *allocatedVariableDataSize = variableDataSize;
    }

    /* Allocate the memory */
    ntfHeader->correlatedNotifications = clHeapCalloc(1,(sizeof(SaNtfIdentifierT) * numCorrelatedNotifications));
    ntfHeader->additionalText = clHeapCalloc(1, lengthAdditionalText);
    ntfHeader->additionalInfo = clHeapCalloc(1, (sizeof(SaNtfAdditionalInfoT) * numAdditionalInfo));
    *variableDataPtr = clHeapCalloc(1,(*allocatedVariableDataSize));

    if ((ntfHeader->correlatedNotifications == NULL) ||
            (ntfHeader->additionalText == NULL) ||
            (ntfHeader->additionalInfo == NULL) ||
            (*variableDataPtr == NULL))
    {
        clLogError("NTF","NAL","Failed to allocate memory for notification header attributes");
        *allocatedVariableDataSize = 0;
        rc = CL_ERR_NO_MEMORY;
        goto destroy_handle;
    }

    *usedVariableDataSize = 0;

    /* Now assign input notification header pointer with allocated values */
    notificationHeader->eventType = &(ntfHeader->eventType);
    notificationHeader->notificationObject = &(ntfHeader->notificationObject);
    notificationHeader->notifyingObject = &(ntfHeader->notifyingObject);
    notificationHeader->notificationClassId = &(ntfHeader->notificationClassId);
    notificationHeader->eventTime = &(ntfHeader->eventTime);
    notificationHeader->numCorrelatedNotifications = ntfHeader->numCorrelatedNotifications;
    notificationHeader->lengthAdditionalText = ntfHeader->lengthAdditionalText;
    notificationHeader->numAdditionalInfo = ntfHeader->numAdditionalInfo;
    notificationHeader->notificationId = &(ntfHeader->notificationId);
    notificationHeader->correlatedNotifications = ntfHeader->correlatedNotifications;
    notificationHeader->additionalText = ntfHeader->additionalText;
    notificationHeader->additionalInfo = ntfHeader->additionalInfo;

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, *notificationHandle);
    goto error_no_destroy;

destroy_handle:
    _ntfFreeNotificationHeaderAndVarSizeDataElements(ntfHeader, *variableDataPtr);
destroy_no_free:
    clHandleDestroy(notificationHandleDbHandle, *notificationHandle);

error_no_destroy:

    clHandleCheckin(handleDbHandle, ntfHandle);

    /* Return OK */
    return rc;
}

static void
_ntfFreeNotificationHeaderAndVarSizeDataElements(ClNtfNotificationHeaderBufferT *ntfHeader,
                                                 ClPtrT                          variableDataPtr)
{
    if (ntfHeader->correlatedNotifications) 
    {
        clHeapFree(ntfHeader->correlatedNotifications);
        ntfHeader->correlatedNotifications = NULL;
    }
    if (ntfHeader->additionalText)
    {
        clHeapFree(ntfHeader->additionalText);
        ntfHeader->additionalText = NULL;
    }
    if (ntfHeader->additionalInfo)
    {
        clHeapFree(ntfHeader->additionalInfo);
        ntfHeader->additionalInfo = NULL;
    }
    if (variableDataPtr)
    {
        clHeapFree(variableDataPtr);
        variableDataPtr = NULL;
    }
}

SaAisErrorT 
saNtfObjectCreateDeleteNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfObjectCreateDeleteNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numAttributes,
    SaInt16T variableDataSize)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                       rc = CL_OK;
    ClHandleT                                   notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfObjectCreateDeleteNotificationBufferT *notificationPtr = NULL;


    /* Check the input values */
    if (notification == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if ((variableDataSize < 0) && (variableDataSize != SA_NTF_ALLOC_SYSTEM_LIMIT))
    {
        clLogError("NTF","NAL","Invalid value specified for variableDataSize parameter "
                               "This value should be either SA_NTF_ALLOC_SYSTEM_LIMIT or any value "
                               "less than or equal to 32767 (Max +ve value that can be stored in int16)");


        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNotificationHandleAndHeaderBuffer(ntfHandle,
                                                     SA_NTF_TYPE_OBJECT_CREATE_DELETE,
                                                     sizeof(ClNtfObjectCreateDeleteNotificationBufferT),
                                                     &notificationHandle,
                                                     numCorrelatedNotifications,
                                                     lengthAdditionalText,
                                                     numAdditionalInfo,
                                                     variableDataSize,
                                                     &notification->notificationHeader);
    if (rc != CL_OK)
    {
        clLogError("NTF","NAL","Allocating notification header buffer failed with rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }

    /* Checkout the created handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&notificationPtr);

    if (rc != CL_OK)
    {
        /* This should never happen. So libray is unusable anymore */
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        /* This failure cannot be handled gracefully and there will be memory leak here */
        return SA_AIS_ERR_LIBRARY;
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        goto destroy_handle;
    }

    /* Now initialize notification specific attributes */
    notificationPtr->numAttributes = numAttributes;

    notificationPtr->objectAttributes = 
        clHeapCalloc(1,(sizeof(SaNtfAttributeT) * numAttributes));
    if (notificationPtr->objectAttributes == NULL)
    {
        clLogError("NTF","NAL","Failed to allocate memory for objectAttributes attribute");
        rc = CL_ERR_NO_MEMORY;
        goto free_and_destroy_handle;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notification->notificationHandle = notificationHandle;

    /* Assign notification specific attributes */
    notification->numAttributes = notificationPtr->numAttributes;
    notification->sourceIndicator = &(notificationPtr->sourceIndicator);
    notification->objectAttributes = notificationPtr->objectAttributes;

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    goto error_no_destroy;

free_and_destroy_handle:
    _ntfFreeNotificationHeaderAndVarSizeDataElements(&notificationPtr->ntfHeader,
                                                     notificationPtr->variableDataPtr);
destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);
error_no_destroy:

    /* Return OK */
    return _aspErrToAisError(rc);
}

SaAisErrorT 
saNtfAttributeChangeNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAttributeChangeNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numAttributes,
    SaInt16T variableDataSize)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                     rc = CL_OK;
    ClHandleT                                 notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfAttributeChangeNotificationBufferT  *notificationPtr = NULL;


    /* Check the input values */
    if (notification == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if ((variableDataSize < 0) && (variableDataSize != SA_NTF_ALLOC_SYSTEM_LIMIT))
    {
        clLogError("NTF","NAL","Invalid value specified for variableDataSize parameter "
                               "This value should be either SA_NTF_ALLOC_SYSTEM_LIMIT or any value "
                               "less than or equal to 32767 (Max +ve value that can be stored in int16)");


        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNotificationHandleAndHeaderBuffer(ntfHandle,
                                                     SA_NTF_TYPE_ATTRIBUTE_CHANGE,
                                                     sizeof(ClNtfAttributeChangeNotificationBufferT),
                                                     &notificationHandle,
                                                     numCorrelatedNotifications,
                                                     lengthAdditionalText,
                                                     numAdditionalInfo,
                                                     variableDataSize,
                                                     &notification->notificationHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the created handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&notificationPtr);

    if (rc != CL_OK)
    {
        /* This should never happen. So libray is unusable anymore */
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        /* This failure cannot be handled gracefully and there will be memory leak here */
        return SA_AIS_ERR_LIBRARY;
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        goto destroy_handle;
    }

    /* Initialize notification specific attributes */
    notificationPtr->numAttributes = numAttributes;

    notificationPtr->changedAttributes = 
        clHeapCalloc(1,(sizeof(SaNtfAttributeChangeT) * numAttributes));
    if (notificationPtr->changedAttributes == NULL)
    {
        clLogError("NTF","NAL","Failed to allocate memory for changedAttributes");
        rc = CL_ERR_NO_MEMORY;
        goto free_and_destroy_handle;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notification->notificationHandle = notificationHandle;

    /* Assign notification specific attributes */
    notification->numAttributes = notificationPtr->numAttributes;
    notification->sourceIndicator = &(notificationPtr->sourceIndicator);
    notification->changedAttributes = notificationPtr->changedAttributes;

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    goto error_no_destroy;

free_and_destroy_handle:
    _ntfFreeNotificationHeaderAndVarSizeDataElements(&notificationPtr->ntfHeader,
                                                     notificationPtr->variableDataPtr);
destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);
error_no_destroy:

    /* Return OK */
    return _aspErrToAisError(rc);
}

SaAisErrorT 
saNtfStateChangeNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfStateChangeNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numStateChanges,
    SaInt16T variableDataSize)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                     rc = CL_OK;
    ClHandleT                                 notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfStateChangeNotificationBufferT      *notificationPtr = NULL;


    /* Check the input values */
    if (notification == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if ((variableDataSize < 0) && (variableDataSize != SA_NTF_ALLOC_SYSTEM_LIMIT))
    {
        clLogError("NTF","NAL","Invalid value specified for variableDataSize parameter "
                               "This value should be either SA_NTF_ALLOC_SYSTEM_LIMIT or any value "
                               "less than or equal to 32767 (Max +ve value that can be stored in int16)");
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNotificationHandleAndHeaderBuffer(ntfHandle,
                                                     SA_NTF_TYPE_STATE_CHANGE,
                                                     sizeof(ClNtfStateChangeNotificationBufferT),
                                                     &notificationHandle,
                                                     numCorrelatedNotifications,
                                                     lengthAdditionalText,
                                                     numAdditionalInfo,
                                                     variableDataSize,
                                                     &notification->notificationHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the created handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&notificationPtr);

    if (rc != CL_OK)
    {
        /* This should never happen. So libray is unusable anymore */
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        /* This failure cannot be handled gracefully and there will be memory leak here */
        return SA_AIS_ERR_LIBRARY;
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        goto destroy_handle;
    }

    /* Initialize notification specific attributes */
    notificationPtr->numStateChanges = numStateChanges;

    notificationPtr->changedStates = 
        clHeapCalloc(1,(sizeof(SaNtfStateChangeT) * numStateChanges));
    if (notificationPtr->changedStates == NULL)
    {
        clLogError("NTF","NAL","Failed to allocate memory for changedStates attribute");
        rc = CL_ERR_NO_MEMORY;
        goto free_and_destroy_handle;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notification->notificationHandle = notificationHandle;

    /* Assign notification specific attributes */
    notification->numStateChanges = notificationPtr->numStateChanges;
    notification->sourceIndicator = &(notificationPtr->sourceIndicator);
    notification->changedStates = notificationPtr->changedStates;

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    goto error_no_destroy;

free_and_destroy_handle:
    _ntfFreeNotificationHeaderAndVarSizeDataElements(&notificationPtr->ntfHeader,
                                                     notificationPtr->variableDataPtr);
destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);
error_no_destroy:

    /* Return OK */
    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfAlarmNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAlarmNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numSpecificProblems,
    SaUint16T numMonitoredAttributes,
    SaUint16T numProposedRepairActions,
    SaInt16T variableDataSize)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                           rc = CL_OK;
    ClHandleT                       notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfAlarmNotificationBufferT  *notificationPtr = NULL;


    /* Check the input values */
    if (notification == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if ((variableDataSize < 0) && (variableDataSize != SA_NTF_ALLOC_SYSTEM_LIMIT))
    {
        clLogError("NTF","NAL","Invalid value specified for variableDataSize parameter "
                               "This value should be either SA_NTF_ALLOC_SYSTEM_LIMIT or any value "
                               "less than or equal to 32767 (Max +ve value that can be stored in int16)");


        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNotificationHandleAndHeaderBuffer(ntfHandle,
                                                     SA_NTF_TYPE_ALARM,
                                                     sizeof(ClNtfAlarmNotificationBufferT),
                                                     &notificationHandle,
                                                     numCorrelatedNotifications,
                                                     lengthAdditionalText,
                                                     numAdditionalInfo,
                                                     variableDataSize,
                                                     &notification->notificationHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the created handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&notificationPtr);

    if (rc != CL_OK)
    {
        /* This should never happen. So libray is unusable anymore */
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        /* This failure cannot be handled gracefully and there will be memory leak here */
        return SA_AIS_ERR_LIBRARY;
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        goto destroy_handle;
    }

    /* Initialize notification specific attributes */
    notificationPtr->numSpecificProblems = numSpecificProblems;
    notificationPtr->numMonitoredAttributes = numMonitoredAttributes;
    notificationPtr->numProposedRepairActions = numProposedRepairActions;

    notificationPtr->specificProblems = clHeapCalloc(1, (sizeof(SaNtfSpecificProblemT) * numSpecificProblems));
    notificationPtr->monitoredAttributes = clHeapCalloc(1, (sizeof(SaNtfAttributeT) * numMonitoredAttributes));
    notificationPtr->proposedRepairActions = clHeapCalloc(1, (sizeof(SaNtfProposedRepairActionT) * numProposedRepairActions));

    if ((notificationPtr->specificProblems == NULL) ||
            (notificationPtr->monitoredAttributes == NULL) ||
            (notificationPtr->proposedRepairActions == NULL))
    {
        clLogError("NTF","NAL","Failed to allocate memory for alarm attribute");
        rc = CL_ERR_NO_MEMORY;
        goto free_and_destroy_handle;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notification->notificationHandle = notificationHandle;

    /* Assign notification specific attributes */
    notification->numSpecificProblems = notificationPtr->numSpecificProblems;
    notification->numMonitoredAttributes = notificationPtr->numMonitoredAttributes;
    notification->numProposedRepairActions = notificationPtr->numProposedRepairActions;
    notification->probableCause = &(notificationPtr->probableCause);
    notification->specificProblems = notificationPtr->specificProblems;
    notification->perceivedSeverity = &(notificationPtr->perceivedSeverity);
    notification->trend = &(notificationPtr->trend);
    notification->thresholdInformation = &(notificationPtr->thresholdInformation);
    notification->monitoredAttributes = notificationPtr->monitoredAttributes;
    notification->proposedRepairActions = notificationPtr->proposedRepairActions;

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    goto error_no_destroy;

free_and_destroy_handle:
    /* Free alarm attribute pointers if allocated */
    if (notificationPtr->specificProblems)
    {
        clHeapFree(notificationPtr->specificProblems);
        notificationPtr->specificProblems = NULL;
    }

    if (notificationPtr->monitoredAttributes)
    {
        clHeapFree(notificationPtr->monitoredAttributes);
        notificationPtr->monitoredAttributes = NULL;
    }
    
    if (notificationPtr->proposedRepairActions)
    {
        clHeapFree(notificationPtr->proposedRepairActions);
        notificationPtr->proposedRepairActions = NULL;
    }

    _ntfFreeNotificationHeaderAndVarSizeDataElements(&notificationPtr->ntfHeader,
                                                     notificationPtr->variableDataPtr);
destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);
error_no_destroy:

    /* Return */
    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfSecurityAlarmNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfSecurityAlarmNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaInt16T variableDataSize)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                           rc = CL_OK;
    ClHandleT                       notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfSecurityAlarmNotificationBufferT  *notificationPtr = NULL;


    /* Check the input values */
    if (notification == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if ((variableDataSize < 0) && (variableDataSize != SA_NTF_ALLOC_SYSTEM_LIMIT))
    {
        clLogError("NTF","NAL","Invalid value specified for variableDataSize parameter "
                               "This value should be either SA_NTF_ALLOC_SYSTEM_LIMIT or any value "
                               "less than or equal to 32767 (Max +ve value that can be stored in int16)");


        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNotificationHandleAndHeaderBuffer(ntfHandle,
                                                     SA_NTF_TYPE_SECURITY_ALARM,
                                                     sizeof(ClNtfSecurityAlarmNotificationBufferT),
                                                     &notificationHandle,
                                                     numCorrelatedNotifications,
                                                     lengthAdditionalText,
                                                     numAdditionalInfo,
                                                     variableDataSize,
                                                     &notification->notificationHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the created handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&notificationPtr);

    if (rc != CL_OK)
    {
        /* This should never happen. So libray is unusable anymore */
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        /* This failure cannot be handled gracefully and there will be memory leak here */
        return SA_AIS_ERR_LIBRARY;
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NAL","handle pointer is found to be NULL after handle checkout");
        goto destroy_handle;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notification->notificationHandle = notificationHandle;

    /* Assign notification specific attributes */
    notification->probableCause = &(notificationPtr->probableCause);
    notification->severity = &(notificationPtr->severity);
    notification->securityAlarmDetector = &(notificationPtr->securityAlarmDetector);
    notification->serviceUser = &(notificationPtr->serviceUser);
    notification->serviceProvider = &(notificationPtr->serviceProvider);

    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    goto error_no_destroy;

destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);
error_no_destroy:

    /* Return */
    return _aspErrToAisError(rc);

}

/******************************************************************************
 * NOTIFICATION FREE API
 *
 * * saNtfNotificationFree()
 *
 ******************************************************************************/
SaAisErrorT 
saNtfNotificationFree(
    SaNtfNotificationHandleT notificationHandle)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","NFR","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Get the notification type from the notificationPtr */
    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
                _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader,
                                                                  ntfPtr->variableDataPtr);
                clHeapFree(ntfPtr->objectAttributes);
                ntfPtr->objectAttributes=NULL;
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader,
                                                                  ntfPtr->variableDataPtr);
                clHeapFree(ntfPtr->changedAttributes);
                ntfPtr->changedAttributes=NULL;
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader,
                                                                  ntfPtr->variableDataPtr);
                clHeapFree(ntfPtr->changedStates);
                ntfPtr->changedStates=NULL;
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader,
                                                                  ntfPtr->variableDataPtr);
                clHeapFree(ntfPtr->monitoredAttributes);
                ntfPtr->monitoredAttributes=NULL;
                clHeapFree(ntfPtr->specificProblems);
                ntfPtr->specificProblems=NULL;
                clHeapFree(ntfPtr->proposedRepairActions);
                ntfPtr->proposedRepairActions=NULL;
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader,
                                                                  ntfPtr->variableDataPtr);
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            /* No OP for now */
            break;
        default:
            clLogError("NTF","NFR","Invalid notification type found in the allocated notification data");
    }

    /* Checkin and destroy the handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    rc = clHandleDestroy(notificationHandleDbHandle, notificationHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","NFR","handle destroy failed during notificationFree, rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }
    
    return _aspErrToAisError(rc);
}

/******************************************************************************
 * VARIABLE LENGTH DATA ACCESS APIs
 *
 * * saNtfPtrValAllocate()
 * * saNtfArrayValAllocate()
 * * saNtfPtrValGet()
 * * saNtfArrayValGet()
 * * saNtfVariableDataSizeGet()
 *
 ******************************************************************************/
SaAisErrorT 
saNtfPtrValAllocate(
    SaNtfNotificationHandleT notificationHandle,
    SaUint16T dataSize,
    void **dataPtr,
    SaNtfValueT *value)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    if ((dataPtr == NULL) || (value == NULL))
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        /* Invalid handle */
        return _aspErrToAisError(rc);
    }

    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
                if (dataSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    clLogError("NTF","","Requested data size if greater than available data size");
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *dataPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->ptrVal.dataOffset = ntfPtr->usedVariableDataSize;
                value->ptrVal.dataSize = dataSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + dataSize;
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
                if (dataSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *dataPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->ptrVal.dataOffset = ntfPtr->usedVariableDataSize;
                value->ptrVal.dataSize = dataSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + dataSize;
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
                if (dataSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *dataPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->ptrVal.dataOffset = ntfPtr->usedVariableDataSize;
                value->ptrVal.dataSize = dataSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + dataSize;
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
                if (dataSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *dataPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->ptrVal.dataOffset = ntfPtr->usedVariableDataSize;
                value->ptrVal.dataSize = dataSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + dataSize;
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
                if (dataSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *dataPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->ptrVal.dataOffset = ntfPtr->usedVariableDataSize;
                value->ptrVal.dataSize = dataSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + dataSize;
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","PAL","Invalid notification type found in the allocated notification data");
            rc = CL_ERR_LIBRARY;
            break;
    }
    
checkin_return:
    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    return _aspErrToAisError(rc);
}

SaAisErrorT 
saNtfArrayValAllocate(
    SaNtfNotificationHandleT notificationHandle,
    SaUint16T numElements,
    SaUint16T elementSize,
    void **arrayPtr,
    SaNtfValueT *value)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;
    ClUint32T               totalSize = numElements * elementSize;

    if ((arrayPtr == NULL) || (value == NULL))
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }
    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        /* Invalid handle */
        return _aspErrToAisError(rc);
    }

    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
                if (totalSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *arrayPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->arrayVal.arrayOffset = ntfPtr->usedVariableDataSize;
                value->arrayVal.numElements = numElements;
                value->arrayVal.elementSize = elementSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + totalSize;
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
                if (totalSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *arrayPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->arrayVal.arrayOffset = ntfPtr->usedVariableDataSize;
                value->arrayVal.numElements = numElements;
                value->arrayVal.elementSize = elementSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + totalSize;
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
                if (totalSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *arrayPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->arrayVal.arrayOffset = ntfPtr->usedVariableDataSize;
                value->arrayVal.numElements = numElements;
                value->arrayVal.elementSize = elementSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + totalSize;
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
                if (totalSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *arrayPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->arrayVal.arrayOffset = ntfPtr->usedVariableDataSize;
                value->arrayVal.numElements = numElements;
                value->arrayVal.elementSize = elementSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + totalSize;
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
                if (totalSize > (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize))
                {
                    rc = CL_ERR_NO_SPACE;
                    goto checkin_return;
                }
                
                *arrayPtr=((ClUint8T*)ntfPtr->variableDataPtr + ntfPtr->usedVariableDataSize);
                value->arrayVal.arrayOffset = ntfPtr->usedVariableDataSize;
                value->arrayVal.numElements = numElements;
                value->arrayVal.elementSize = elementSize;
                ntfPtr->usedVariableDataSize = ntfPtr->usedVariableDataSize + totalSize;
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","AAL","Invalid notification type found in the allocated notification data");
            rc = CL_ERR_LIBRARY;
            break;
    }
    
checkin_return:
    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    return _aspErrToAisError(rc);
}

SaAisErrorT saNtfPtrValGet(
    SaNtfNotificationHandleT notificationHandle,
    SaNtfValueT *value,
    void **dataPtr,
    SaUint16T *dataSize)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    if ((dataPtr == NULL) || (value == NULL) || (dataSize == NULL))
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        /* Invalid handle */
        return _aspErrToAisError(rc);
    }

    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
                
                *dataPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->ptrVal.dataOffset);
                *dataSize = value->ptrVal.dataSize; 
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
                
                *dataPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->ptrVal.dataOffset);
                *dataSize = value->ptrVal.dataSize; 
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
                
                *dataPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->ptrVal.dataOffset);
                *dataSize = value->ptrVal.dataSize; 
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
                
                *dataPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->ptrVal.dataOffset);
                *dataSize = value->ptrVal.dataSize; 
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
                
                *dataPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->ptrVal.dataOffset);
                *dataSize = value->ptrVal.dataSize; 
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","PGT","Invalid notification type found in the allocated notification data");
            rc = CL_ERR_LIBRARY;
            break;
    }
    
    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    return _aspErrToAisError(rc);
}

SaAisErrorT 
saNtfArrayValGet(
    SaNtfNotificationHandleT notificationHandle,
    SaNtfValueT *value,
    void **arrayPtr,
    SaUint16T *numElements,
    SaUint16T *elementSize)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    if ((arrayPtr == NULL) || (value == NULL) || 
            (numElements == NULL) || (elementSize == NULL))
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        /* Invalid handle */
        return _aspErrToAisError(rc);
    }

    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
                
                *arrayPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->arrayVal.arrayOffset);
                *numElements = value->arrayVal.numElements;
                *elementSize = value->arrayVal.elementSize;
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
                
                *arrayPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->arrayVal.arrayOffset);
                *numElements = value->arrayVal.numElements;
                *elementSize = value->arrayVal.elementSize;
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
                
                *arrayPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->arrayVal.arrayOffset);
                *numElements = value->arrayVal.numElements;
                *elementSize = value->arrayVal.elementSize;
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
                
                *arrayPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->arrayVal.arrayOffset);
                *numElements = value->arrayVal.numElements;
                *elementSize = value->arrayVal.elementSize;
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
                
                *arrayPtr = ((ClUint8T*)ntfPtr->variableDataPtr + value->arrayVal.arrayOffset);
                *numElements = value->arrayVal.numElements;
                *elementSize = value->arrayVal.elementSize;
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","","Invalid notification type found in the allocated notification data");
            rc = CL_ERR_LIBRARY;
            break;
    }
    
    /* Check-in notification handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    return _aspErrToAisError(rc);
}

SaAisErrorT
saNtfVariableDataSizeGet(
                         SaNtfNotificationHandleT notificationHandle,
                         SaUint16T *variableDataSpaceAvailable)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","VSG","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Get the notification type from the notificationPtr */
    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
    case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
        {
            ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            *variableDataSpaceAvailable = (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize);
        }
        break;
    case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
        {
            ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;
            *variableDataSpaceAvailable = (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize);
        }
        break;
    case SA_NTF_TYPE_STATE_CHANGE:
        {
            ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;
            *variableDataSpaceAvailable = (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize);
        }
        break;
    case SA_NTF_TYPE_ALARM:
        {
            ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;
            *variableDataSpaceAvailable = (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize);
        }
        break;
    case SA_NTF_TYPE_SECURITY_ALARM:
        {
            ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;
            *variableDataSpaceAvailable = (ntfPtr->allocatedVariableDataSize - ntfPtr->usedVariableDataSize);
        }
        break;
    case SA_NTF_TYPE_MISCELLANEOUS:
        //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
        break;
    default:
        clLogError("NTF","VSG","Invalid notification type found in the allocated notification data");
        rc = CL_ERR_LIBRARY;
    }

    /* Checkin and destroy the handle */
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);

    return SA_AIS_OK;
}

/******************************************************************************
 * NOTIFICATION SEND/SUBSCRIBE APIs
 *
 * * saNtfNotificationSend()
 *
 ******************************************************************************/
SaAisErrorT 
saNtfNotificationSend(
    SaNtfNotificationHandleT notificationHandle)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;
    SaNtfIdentifierT        notificationId;
    ClBufferHandleT         payLoadMsg = 0;
    ClUint8T               *payLoadBuffer = NULL;
    ClUint32T               msgLength = 0;
    ClUint32T               index = 0;
    ClEventIdT              eventId = 0;
    ClEventHandleT          eventHandle = CL_HANDLE_INVALID_VALUE;

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","SND","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Create a buffer to marshall the notification data */
    rc = clBufferCreate(&payLoadMsg);
    if (rc != CL_OK)
    {
        clLogError("NTF","SND","buffer creation failed while sending the notification. rc 0x%x",rc);
        goto checkin_return;
    }

    /* Generate a notification ID to be assigned to this notification */
    notificationId = clNtfNotificationIdGenerate(ntfIdFileFd,thisNodeId);
    if (notificationId == SA_NTF_IDENTIFIER_UNUSED)
    {
        clLogError("NTF","","Error while allocating the notification");
        rc = CL_ERR_LIBRARY;
        goto checkin_return;
    }

    /* Get the notification type from the notificationPtr */
    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    /* First marshall the notificationType and the header fileds which are 
     * common to all kinds of notifications. Since all type of notification
     * buffers have ntfHeader as the second filed, we can extract this by
     * typecasting the notificationPtr to any of the notificationBuffer types.
     * So we use ClNtfObjectCreateDeleteNotificationBufferT type
     * to extract the header fileds and marshall them into the buffer */
    {
        ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
        ntfPtr->ntfHeader.notificationId = notificationId;

        /* Marshall notification Type first */
        clXdrMarshallClUint32T(&notifType, payLoadMsg, 0);
        rc = marshallNotificationHeaderBuffer(payLoadMsg, &ntfPtr->ntfHeader);
        if (rc != CL_OK)
        {
            clLogError("NTF","SND","marshallNotificationHeaderBuffer failed with rc 0x%x",rc);
            goto reset_notification_id;
        }
    }


    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;

                /* Marshall variable data soon after the header so that it is easy while
                 * unmarshalling */
                clXdrMarshallClUint16T(&ntfPtr->variableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallArrayClCharT(ntfPtr->variableDataPtr, ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->usedVariableDataSize, payLoadMsg, 0);

                /* Marshall this notification specific attributes */
                clXdrMarshallClUint16T(&ntfPtr->numAttributes, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->sourceIndicator, payLoadMsg, 0);
                for (index = 0; index < ntfPtr->numAttributes; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->objectAttributes[index].attributeId, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->objectAttributes[index].attributeType, payLoadMsg, 0);
                    marshallNtfValueTypeT(ntfPtr->objectAttributes[index].attributeType,
                                          &ntfPtr->objectAttributes[index].attributeValue, payLoadMsg);
                }

            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationBufferT*)notificationPtr;

                /* Marshall variable data soon after the header so that it is easy while
                 * unmarshalling */
                clXdrMarshallClUint16T(&ntfPtr->variableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallArrayClCharT(ntfPtr->variableDataPtr, ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->usedVariableDataSize, payLoadMsg, 0);

                /* Marshall this notification specific attributes */
                clXdrMarshallClUint16T(&ntfPtr->numAttributes, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->sourceIndicator, payLoadMsg, 0);

                for (index = 0; index < ntfPtr->numAttributes; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->changedAttributes[index].attributeId, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->changedAttributes[index].attributeType, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->changedAttributes[index].oldAttributePresent, payLoadMsg, 0);
                    marshallNtfValueTypeT(ntfPtr->changedAttributes[index].attributeType,
                                          &ntfPtr->changedAttributes[index].oldAttributeValue, payLoadMsg);
                    marshallNtfValueTypeT(ntfPtr->changedAttributes[index].attributeType,
                                          &ntfPtr->changedAttributes[index].newAttributeValue, payLoadMsg);
                }

            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationBufferT  *ntfPtr = (ClNtfStateChangeNotificationBufferT*)notificationPtr;

                /* Marshall variable data soon after the header so that it is easy while
                 * unmarshalling */
                clXdrMarshallClUint16T(&ntfPtr->variableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallArrayClCharT(ntfPtr->variableDataPtr, ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->usedVariableDataSize, payLoadMsg, 0);

                /* Marshall this notification specific attributes */
                clXdrMarshallClUint16T(&ntfPtr->numStateChanges, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->sourceIndicator, payLoadMsg, 0);

                for (index = 0; index < ntfPtr->numStateChanges; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->changedStates[index].stateId, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->changedStates[index].oldStatePresent, payLoadMsg, 0);
                    clXdrMarshallClUint16T(&ntfPtr->changedStates[index].oldState, payLoadMsg, 0);
                    clXdrMarshallClUint16T(&ntfPtr->changedStates[index].newState, payLoadMsg, 0);
                }
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationBufferT  *ntfPtr = (ClNtfAlarmNotificationBufferT*)notificationPtr;

                /* Marshall variable data soon after the header so that it is easy while
                 * unmarshalling */
                clXdrMarshallClUint16T(&ntfPtr->variableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallArrayClCharT(ntfPtr->variableDataPtr, ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->usedVariableDataSize, payLoadMsg, 0);

                /* Marshall this notification specific attributes */
                clXdrMarshallClUint16T(&ntfPtr->numSpecificProblems, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->numMonitoredAttributes, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->numProposedRepairActions, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->probableCause, payLoadMsg, 0);
                for (index = 0; index < ntfPtr->numSpecificProblems; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->specificProblems[index].problemId, payLoadMsg, 0);
                    marshallNtfClassIdT(&ntfPtr->specificProblems[index].problemClassId, payLoadMsg);
                    clXdrMarshallClUint32T(&ntfPtr->specificProblems[index].problemType, payLoadMsg, 0);
                    marshallNtfValueTypeT(ntfPtr->specificProblems[index].problemType,
                                          &ntfPtr->specificProblems[index].problemValue, payLoadMsg);
                }
                clXdrMarshallClUint32T(&ntfPtr->perceivedSeverity, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->trend, payLoadMsg, 0);
                
                /* marshall thresholdInformation */
                clXdrMarshallClUint16T(&ntfPtr->thresholdInformation.thresholdId, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->thresholdInformation.thresholdValueType, payLoadMsg, 0);
                marshallNtfValueTypeT(ntfPtr->thresholdInformation.thresholdValueType,
                                      &ntfPtr->thresholdInformation.thresholdValue, payLoadMsg);
                marshallNtfValueTypeT(ntfPtr->thresholdInformation.thresholdValueType,
                                      &ntfPtr->thresholdInformation.thresholdHysteresis, payLoadMsg);
                marshallNtfValueTypeT(ntfPtr->thresholdInformation.thresholdValueType,
                                      &ntfPtr->thresholdInformation.observedValue, payLoadMsg);
                clXdrMarshallClUint64T(&ntfPtr->thresholdInformation.armTime, payLoadMsg, 0);

                /* marshall monitoredAttributes array */
                for (index = 0; index < ntfPtr->numMonitoredAttributes; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->monitoredAttributes[index].attributeId, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->monitoredAttributes[index].attributeType, payLoadMsg, 0);
                    marshallNtfValueTypeT(ntfPtr->monitoredAttributes[index].attributeType,
                                          &ntfPtr->monitoredAttributes[index].attributeValue, payLoadMsg);
                }

                /* marshall proposed repair actions */
                for (index = 0; index < ntfPtr->numProposedRepairActions; index++)
                {
                    clXdrMarshallClUint16T(&ntfPtr->proposedRepairActions[index].actionId, payLoadMsg, 0);
                    clXdrMarshallClUint32T(&ntfPtr->proposedRepairActions[index].actionValueType, payLoadMsg, 0);
                    marshallNtfValueTypeT(ntfPtr->proposedRepairActions[index].actionValueType,
                                          &ntfPtr->proposedRepairActions[index].actionValue, payLoadMsg);
                }
            
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationBufferT*)notificationPtr;

                /* Marshall variable data soon after the header so that it is easy while
                 * unmarshalling */
                clXdrMarshallClUint16T(&ntfPtr->variableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallArrayClCharT(ntfPtr->variableDataPtr, ntfPtr->allocatedVariableDataSize, payLoadMsg, 0);
                clXdrMarshallClUint16T(&ntfPtr->usedVariableDataSize, payLoadMsg, 0);

                /* Marshall notification specific fields */
                clXdrMarshallClUint32T(&ntfPtr->probableCause, payLoadMsg, 0);
                clXdrMarshallClUint32T(&ntfPtr->severity, payLoadMsg, 0);

                clXdrMarshallClUint32T(&ntfPtr->securityAlarmDetector.valueType, payLoadMsg, 0);
                marshallNtfValueTypeT(ntfPtr->securityAlarmDetector.valueType,
                                      &ntfPtr->securityAlarmDetector.value, payLoadMsg);

                clXdrMarshallClUint32T(&ntfPtr->serviceUser.valueType, payLoadMsg, 0);
                marshallNtfValueTypeT(ntfPtr->serviceUser.valueType,
                                      &ntfPtr->serviceUser.value, payLoadMsg);

                clXdrMarshallClUint32T(&ntfPtr->serviceProvider, payLoadMsg, 0);
                marshallNtfValueTypeT(ntfPtr->serviceProvider.valueType,
                                      &ntfPtr->serviceProvider.value, payLoadMsg);
            }

            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","","Invalid notification type found in the allocated notification data");
            goto reset_notification_id;
    }

    /* Flatten the buffer */
    rc = clBufferLengthGet(payLoadMsg, &msgLength);
    if (CL_OK != rc)
    {
        clLogError("NTF","SND","Failed to get message length. rc 0x%x",rc);
        goto reset_notification_id;
    }

    rc = clBufferFlatten(payLoadMsg, &payLoadBuffer);
    if (rc != CL_OK)
    {
        clLogError("NTF","SND","Failed to get message length. rc 0x%x",rc);
        goto reset_notification_id;
    }

    /* Allocate the event which need to carry this notification */
    rc = clEventAllocate(evtPublishChannelHdl,
                         &eventHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","SND","Event allocate failed with rc 0x%x",rc);
        goto reset_notification_id;
    }
    
    rc = clEventAttributesSet(eventHandle, NULL, CL_EVENT_HIGHEST_PRIORITY, 0, &eventPublisher);
    if (rc != CL_OK)
    {
        clLogError("NTF","","Failed to set event attributes. rc 0x%x",rc);
        goto free_event;
    }

    rc = clEventPublish(eventHandle,
                        payLoadBuffer,
                        msgLength,
                        &eventId);
    if (rc != CL_OK)
    {
        clLogError("NTF", "SEND", "clEventPublish failed while sending the notification. rc 0x%x",rc);
        goto free_event;
    }

    /* Send successful. So dont reset notificationID */
    clEventFree(eventHandle);

    goto buffer_delete_return;

free_event:
    clEventFree(eventHandle);

reset_notification_id:
    ((ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr)->ntfHeader.notificationId = SA_NTF_IDENTIFIER_UNUSED;

buffer_delete_return:
    rc = clBufferDelete(&payLoadMsg);
    if (rc != CL_OK)
    {
        clLogError("NTF","","Buffer delete failed with rc 0x%x",rc);
    }

checkin_return:
    /* Checkin the handle */
    rc = clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","","handleCheckin failed with error 0x%x",rc);
    }
    return _aspErrToAisError(rc);
}




/******************************************************************************
 * NOTIFICATION FILTER ALLOCATE/FREE APIs
 *
 * * saNtfObjectCreateDeleteNotificationFilterAllocate()
 * * saNtfAttributeChangeNotificationFilterAllocate()
 * * saNtfStateChangeNotificationFilterAllocate()
 * * saNtfAlarmNotificationFilterAllocate()
 * * saNtfSecurityAlarmNotificationFilterAllocate()
 * * saNtfNotificationFilterFree()
 *
 ******************************************************************************/

ClRcT
_ntfCreateNtfFilterHandleAndHeaderBuffer(SaNtfHandleT                     ntfHandle,
                                         SaNtfNotificationTypeT           notifType,
                                         ClInt32T                         handleSize,
                                         SaNtfNotificationFilterHandleT  *filterHandle,
                                         SaUint16T                        numEventTypes,
                                         SaUint16T                        numNotificationObjects,
                                         SaUint16T                        numNotifyingObjects,
                                         SaUint16T                        numNotificationClassIds,
                                         SaNtfNotificationFilterHeaderT  *ntfFilterHeader)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                    rc = CL_OK;
    ClNtfLibInstanceT                       *ntfInstancePtr = NULL;
    void                                    *ntfFilterPtr = NULL;
    ClNtfNotificationFilterHeaderBufferT    *filterHdrBuf = NULL;

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return rc;
    }
    
    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return CL_ERR_LIBRARY;
    }

    /* Create a handle with given handleSize */
    rc = clHandleCreate(ntfFilterHandleDbHandle, handleSize, filterHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","FAL","Handle create failed while allocating notification filter");
        goto checkin_return;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, *filterHandle, &ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","INIT","handle pointer is found to be NULL after handle checkout");
        rc = CL_ERR_LIBRARY;
        goto destroy_no_free;
    }

    /* memset the allocated handle memory */
    memset(ntfFilterPtr, 0, handleSize);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ((ClNtfObjectCreateDeleteNotificationFilterBufferT *)ntfFilterPtr)->notificationType = notifType;
                ((ClNtfObjectCreateDeleteNotificationFilterBufferT *)ntfFilterPtr)->svcHandle = ntfHandle;
                filterHdrBuf = &(((ClNtfObjectCreateDeleteNotificationFilterBufferT *)ntfFilterPtr)->ntfFilterHeader);
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ((ClNtfAttributeChangeNotificationFilterBufferT *)ntfFilterPtr)->notificationType = notifType;
                ((ClNtfAttributeChangeNotificationFilterBufferT *)ntfFilterPtr)->svcHandle = ntfHandle;
                filterHdrBuf = &(((ClNtfAttributeChangeNotificationFilterBufferT *)ntfFilterPtr)->ntfFilterHeader);
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ((ClNtfStateChangeNotificationFilterBufferT *)ntfFilterPtr)->notificationType = notifType;
                ((ClNtfStateChangeNotificationFilterBufferT *)ntfFilterPtr)->svcHandle = ntfHandle;
                filterHdrBuf = &(((ClNtfStateChangeNotificationFilterBufferT *)ntfFilterPtr)->ntfFilterHeader);
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ((ClNtfAlarmNotificationFilterBufferT *)ntfFilterPtr)->notificationType = notifType;
                ((ClNtfAlarmNotificationFilterBufferT *)ntfFilterPtr)->svcHandle = ntfHandle;
                filterHdrBuf = &(((ClNtfAlarmNotificationFilterBufferT *)ntfFilterPtr)->ntfFilterHeader);
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ((ClNtfSecurityAlarmNotificationFilterBufferT *)ntfFilterPtr)->notificationType = notifType;
                ((ClNtfSecurityAlarmNotificationFilterBufferT *)ntfFilterPtr)->svcHandle = ntfHandle;
                filterHdrBuf = &(((ClNtfSecurityAlarmNotificationFilterBufferT *)ntfFilterPtr)->ntfFilterHeader);
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            clLogError("NTF","FAL","SA_NTF_TYPE_MISCELLANEOUS notifications are not supported");
            rc = CL_ERR_NOT_SUPPORTED;
            goto destroy_no_free;
        default:
            clLogError("NTF","FAL","Invalid notification type");
            rc = CL_ERR_INVALID_PARAMETER;
            goto destroy_no_free;
    }

    /* Initialize the ntfFilterHeader attributes */
    filterHdrBuf->notificationFilterHandle = *filterHandle;

    filterHdrBuf->numEventTypes = numEventTypes;
    filterHdrBuf->numNotificationObjects = numNotificationObjects;
    filterHdrBuf->numNotifyingObjects = numNotifyingObjects;
    filterHdrBuf->numNotificationClassIds = numNotificationClassIds;

    filterHdrBuf->eventTypes = clHeapCalloc(1,(sizeof(SaNtfEventTypeT)*numEventTypes));
    filterHdrBuf->notificationObjects = clHeapCalloc(1,(sizeof(SaNameT) * numNotificationObjects));
    filterHdrBuf->notifyingObjects = clHeapCalloc(1,(sizeof(SaNameT) * numNotifyingObjects));
    filterHdrBuf->notificationClassIds = clHeapCalloc(1, (sizeof(SaNtfClassIdT) * numNotificationClassIds));
    if ((filterHdrBuf->eventTypes == NULL) ||
            (filterHdrBuf->notificationObjects == NULL) ||
            (filterHdrBuf->notifyingObjects == NULL) ||
            (filterHdrBuf->notificationClassIds == NULL))
    {
        clLogError("NTF","FAL","Failed to allocate memory for eventTypes field");
        rc = CL_ERR_NO_MEMORY;
        goto destroy_handle_return;
    }

    /* Now initialize the input notificationFilterHeader structure with the allocated
     * values */
    ntfFilterHeader->numEventTypes = filterHdrBuf->numEventTypes;
    ntfFilterHeader->eventTypes = filterHdrBuf->eventTypes;
    ntfFilterHeader->numNotificationObjects = filterHdrBuf->numNotificationObjects;
    ntfFilterHeader->notificationObjects = filterHdrBuf->notificationObjects;
    ntfFilterHeader->numNotifyingObjects = filterHdrBuf->numNotifyingObjects;
    ntfFilterHeader->notifyingObjects = filterHdrBuf->notifyingObjects;
    ntfFilterHeader->numNotificationClassIds = filterHdrBuf->numNotificationClassIds;
    ntfFilterHeader->notificationClassIds = filterHdrBuf->notificationClassIds;

    clHandleCheckin(ntfFilterHandleDbHandle, *filterHandle);
    goto checkin_return;

destroy_handle_return:
    /* Free the memory allocated for any attributes */
    _ntfFreeFilterNtfHeaderBufferElements(filterHdrBuf);
destroy_no_free:
    clHandleCheckin(ntfFilterHandleDbHandle, *filterHandle);
    clHandleDestroy(ntfFilterHandleDbHandle, *filterHandle);

checkin_return:
    clHandleCheckin(handleDbHandle, ntfHandle);

    /* Return OK */
    return rc;
}

static void 
_ntfFreeFilterNtfHeaderBufferElements(ClNtfNotificationFilterHeaderBufferT *filterHdrBuf)
{
    if (filterHdrBuf->eventTypes)
    {
         clHeapFree(filterHdrBuf->eventTypes);
         filterHdrBuf->eventTypes = NULL;
    }
    if (filterHdrBuf->notificationObjects)
    {
        clHeapFree(filterHdrBuf->notificationObjects);
        filterHdrBuf->notificationObjects = NULL;
    }

    if (filterHdrBuf->notifyingObjects)
    {
        clHeapFree(filterHdrBuf->notifyingObjects);
        filterHdrBuf->notifyingObjects = NULL;
    }
    if (filterHdrBuf->notificationClassIds)
    {
        clHeapFree(filterHdrBuf->notificationClassIds);
        filterHdrBuf->notificationClassIds=NULL;
    }
}

SaAisErrorT 
saNtfObjectCreateDeleteNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint16T numSourceIndicators)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                              rc = CL_OK;
    ClHandleT                                          ntfFilterHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfObjectCreateDeleteNotificationFilterBufferT  *ntfFilterPtr = NULL;

    /* Check the input values */
    if (notificationFilter == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNtfFilterHandleAndHeaderBuffer(ntfHandle,
                                             SA_NTF_TYPE_OBJECT_CREATE_DELETE,
                                             sizeof(ClNtfObjectCreateDeleteNotificationFilterBufferT),
                                             &ntfFilterHandle,
                                             numEventTypes,
                                             numNotificationObjects,
                                             numNotifyingObjects,
                                             numNotificationClassIds,
                                             &notificationFilter->notificationFilterHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the handle to the notification filter created just now */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, ntfFilterHandle, (void *)&ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just created the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Initialize this API specific attributes */
    ntfFilterPtr->numSourceIndicators = numSourceIndicators;
    ntfFilterPtr->sourceIndicators = 
        clHeapCalloc(1, (sizeof(SaNtfSourceIndicatorT) * numSourceIndicators));
    if (ntfFilterPtr->sourceIndicators == NULL)
    {
        clLogError("NTF","FAL","Failed to allocate memory for sourceIndicators field");
        _ntfFreeFilterNtfHeaderBufferElements(&ntfFilterPtr->ntfFilterHeader);
        clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
        clHandleDestroy(ntfFilterHandleDbHandle, ntfFilterHandle);
        rc = CL_ERR_NO_MEMORY;
        goto checkin_return;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notificationFilter->notificationFilterHandle = ntfFilterPtr->ntfFilterHeader.notificationFilterHandle;

    /* Assign API specific attributes */
    notificationFilter->numSourceIndicators = ntfFilterPtr->numSourceIndicators;
    notificationFilter->sourceIndicators = ntfFilterPtr->sourceIndicators;

checkin_return:
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);

    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfAttributeChangeNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAttributeChangeNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numSourceIndicators)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                           rc = CL_OK;
    ClHandleT                                       ntfFilterHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfAttributeChangeNotificationFilterBufferT  *ntfFilterPtr = NULL;

    /* Check the input values */
    if (notificationFilter == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNtfFilterHandleAndHeaderBuffer(ntfHandle,
                                             SA_NTF_TYPE_ATTRIBUTE_CHANGE,
                                             sizeof(ClNtfAttributeChangeNotificationFilterBufferT),
                                             &ntfFilterHandle,
                                             numEventTypes,
                                             numNotificationObjects,
                                             numNotifyingObjects,
                                             numNotificationClassIds,
                                             &notificationFilter->notificationFilterHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the handle to the notification filter created just now */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, ntfFilterHandle, (void *)&ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just created the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Initialize this API specific attributes */
    ntfFilterPtr->numSourceIndicators = numSourceIndicators;
    ntfFilterPtr->sourceIndicators = 
        clHeapCalloc(1, (sizeof(SaNtfSourceIndicatorT) * numSourceIndicators));
    if (ntfFilterPtr->sourceIndicators == NULL)
    {
        clLogError("NTF","FAL","Failed to allocate memory for sourceIndicators field");
        _ntfFreeFilterNtfHeaderBufferElements(&ntfFilterPtr->ntfFilterHeader);
        clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
        clHandleDestroy(ntfFilterHandleDbHandle, ntfFilterHandle);
        rc = CL_ERR_NO_MEMORY;
        goto checkin_return;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notificationFilter->notificationFilterHandle = ntfFilterPtr->ntfFilterHeader.notificationFilterHandle;

    /* Assign API specific attributes */
    notificationFilter->numSourceIndicators = ntfFilterPtr->numSourceIndicators;
    notificationFilter->sourceIndicators = ntfFilterPtr->sourceIndicators;

checkin_return:
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);

    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfStateChangeNotificationFilterAllocate_2( 
    SaNtfHandleT ntfHandle,
    SaNtfStateChangeNotificationFilterT_2 *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numSourceIndicators,
    SaUint32T numChangedStates)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                       rc = CL_OK;
    ClHandleT                                   ntfFilterHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfStateChangeNotificationFilterBufferT  *ntfFilterPtr = NULL;

    /* Check the input values */
    if (notificationFilter == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNtfFilterHandleAndHeaderBuffer(ntfHandle,
                                             SA_NTF_TYPE_STATE_CHANGE,
                                             sizeof(ClNtfStateChangeNotificationFilterBufferT),
                                             &ntfFilterHandle,
                                             numEventTypes,
                                             numNotificationObjects,
                                             numNotifyingObjects,
                                             numNotificationClassIds,
                                             &notificationFilter->notificationFilterHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the handle to the notification filter created just now */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, ntfFilterHandle, (void *)&ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just created the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Initialize this API specific attributes */
    ntfFilterPtr->numSourceIndicators = numSourceIndicators;
    ntfFilterPtr->numStateChanges = numChangedStates;
    ntfFilterPtr->sourceIndicators = clHeapCalloc(1, (sizeof(SaNtfSourceIndicatorT) * numSourceIndicators));
    ntfFilterPtr->stateId = clHeapCalloc(1, (sizeof(SaNtfElementIdT) * numChangedStates));

    if ((ntfFilterPtr->sourceIndicators == NULL) ||
            (ntfFilterPtr->stateId == NULL))
    {
        clLogError("NTF","FAL","Failed to allocate memory for state change filter attributes");
        rc = CL_ERR_NO_MEMORY;
        goto destroy_handle_return;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notificationFilter->notificationFilterHandle = ntfFilterPtr->ntfFilterHeader.notificationFilterHandle;

    /* Assign API specific attributes */
    notificationFilter->numSourceIndicators = ntfFilterPtr->numSourceIndicators;
    notificationFilter->sourceIndicators = ntfFilterPtr->sourceIndicators;
    notificationFilter->numStateChanges = ntfFilterPtr->numStateChanges;
    notificationFilter->stateId = ntfFilterPtr->stateId;

    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);

    return _aspErrToAisError(rc);

destroy_handle_return:
    if (ntfFilterPtr->sourceIndicators)
        clHeapFree(ntfFilterPtr->sourceIndicators);
    if (ntfFilterPtr->stateId)
        clHeapFree(ntfFilterPtr->stateId);

    _ntfFreeFilterNtfHeaderBufferElements(&ntfFilterPtr->ntfFilterHeader);
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
    clHandleDestroy(ntfFilterHandleDbHandle, ntfFilterHandle);
    return _aspErrToAisError(rc);

}

SaAisErrorT 
saNtfAlarmNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAlarmNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numProbableCauses,
    SaUint32T numPerceivedSeverities,
    SaUint32T numTrends)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                 rc = CL_OK;
    ClHandleT                             ntfFilterHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfAlarmNotificationFilterBufferT  *ntfFilterPtr = NULL;

    /* Check the input values */
    if (notificationFilter == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNtfFilterHandleAndHeaderBuffer(ntfHandle,
                                             SA_NTF_TYPE_ALARM,
                                             sizeof(ClNtfAlarmNotificationFilterBufferT),
                                             &ntfFilterHandle,
                                             numEventTypes,
                                             numNotificationObjects,
                                             numNotifyingObjects,
                                             numNotificationClassIds,
                                             &notificationFilter->notificationFilterHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the handle to the notification filter created just now */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, ntfFilterHandle, (void *)&ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just created the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Initialize this API specific attributes */
    ntfFilterPtr->numProbableCauses = numProbableCauses;
    ntfFilterPtr->numPerceivedSeverities = numPerceivedSeverities;
    ntfFilterPtr->numTrends = numTrends;

    ntfFilterPtr->probableCauses = clHeapCalloc(1, (sizeof(SaNtfProbableCauseT) * numProbableCauses));
    ntfFilterPtr->perceivedSeverities = clHeapCalloc(1, (sizeof(SaNtfSeverityT) * numPerceivedSeverities));
    ntfFilterPtr->trends = clHeapCalloc(1, (sizeof(SaNtfSeverityTrendT) * numTrends));
    if ((ntfFilterPtr->probableCauses == NULL) ||
            (ntfFilterPtr->perceivedSeverities == NULL) ||
            (ntfFilterPtr->trends == NULL))
    {
        clLogError("NTF","FAL","Failed to allocate memory for probableCauses field");
        rc = CL_ERR_NO_MEMORY;
        goto destroy_handle_return;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notificationFilter->notificationFilterHandle = ntfFilterPtr->ntfFilterHeader.notificationFilterHandle;

    /* Assign API specific attributes */
    notificationFilter->numProbableCauses = ntfFilterPtr->numProbableCauses;
    notificationFilter->numPerceivedSeverities = ntfFilterPtr->numPerceivedSeverities;
    notificationFilter->numTrends = ntfFilterPtr->numTrends;
    notificationFilter->probableCauses = ntfFilterPtr->probableCauses;
    notificationFilter->perceivedSeverities = ntfFilterPtr->perceivedSeverities;
    notificationFilter->trends = ntfFilterPtr->trends;

    goto checkin_return;

destroy_handle_return:
    _ntfFreeFilterNtfHeaderBufferElements(&ntfFilterPtr->ntfFilterHeader);
    if (ntfFilterPtr->probableCauses)
        clHeapFree(ntfFilterPtr->probableCauses);
    if (ntfFilterPtr->perceivedSeverities)
        clHeapFree(ntfFilterPtr->perceivedSeverities);
    if (ntfFilterPtr->trends)
        clHeapFree(ntfFilterPtr->trends);

    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
    clHandleDestroy(ntfFilterHandleDbHandle, ntfFilterHandle);
    return _aspErrToAisError(rc);

checkin_return:
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfSecurityAlarmNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numProbableCauses,
    SaUint32T numSeverities,
    SaUint32T numSecurityAlarmDetectors,
    SaUint32T numServiceUsers,
    SaUint32T numServiceProviders)
{
    /* This function will create an handle for the given notification
     * type by specifying the total size of the memory to be allocated
     * for this handle. Later on the user can use this */
    ClRcT                                 rc = CL_OK;
    ClHandleT                             ntfFilterHandle = CL_HANDLE_INVALID_VALUE;
    ClNtfSecurityAlarmNotificationFilterBufferT  *ntfFilterPtr = NULL;

    /* Check the input values */
    if (notificationFilter == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = _ntfCreateNtfFilterHandleAndHeaderBuffer(ntfHandle,
                                             SA_NTF_TYPE_SECURITY_ALARM,
                                             sizeof(ClNtfSecurityAlarmNotificationFilterBufferT),
                                             &ntfFilterHandle,
                                             numEventTypes,
                                             numNotificationObjects,
                                             numNotifyingObjects,
                                             numNotificationClassIds,
                                             &notificationFilter->notificationFilterHeader);
    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    /* Checkout the handle to the notification filter created just now */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, ntfFilterHandle, (void *)&ntfFilterPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just created the handle */

    if (ntfFilterPtr == NULL)
    {
        clLogError("NTF","FAL","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Initialize this API specific attributes */
    ntfFilterPtr->numProbableCauses = numProbableCauses;
    ntfFilterPtr->numSeverities = numSeverities;
    ntfFilterPtr->numSecurityAlarmDetectors = numSecurityAlarmDetectors;
    ntfFilterPtr->numServiceUsers = numServiceUsers;
    ntfFilterPtr->numServiceProviders = numServiceProviders;

    ntfFilterPtr->probableCauses = clHeapCalloc(1, (sizeof(SaNtfProbableCauseT) * numProbableCauses));
    ntfFilterPtr->severities = clHeapCalloc(1, (sizeof(SaNtfSeverityT) * numSeverities));
    ntfFilterPtr->securityAlarmDetectors = clHeapCalloc(1, (sizeof(SaNtfSecurityAlarmDetectorT) * numSecurityAlarmDetectors));
    ntfFilterPtr->serviceUsers = clHeapCalloc(1, (sizeof(SaNtfServiceUserT) * numServiceUsers));
    ntfFilterPtr->serviceProviders = clHeapCalloc(1, (sizeof(SaNtfServiceUserT) * numServiceProviders));

    if ((ntfFilterPtr->probableCauses == NULL) ||
            (ntfFilterPtr->severities == NULL) ||
            (ntfFilterPtr->securityAlarmDetectors == NULL) ||
            (ntfFilterPtr->serviceUsers == NULL) ||
            (ntfFilterPtr->serviceProviders == NULL))
    {
        clLogError("NTF","FAL","Failed to allocate memory for security alarm filter attributes");
        rc = CL_ERR_NO_MEMORY;
        goto destroy_handle_return;
    }

    /* Now initialize notification pointer (input pointer) with the address
     * of fields of newly allocated structure. */
    notificationFilter->notificationFilterHandle = ntfFilterPtr->ntfFilterHeader.notificationFilterHandle;

    /* Assign API specific attributes */
    notificationFilter->numProbableCauses = ntfFilterPtr->numProbableCauses;
    notificationFilter->numSeverities = ntfFilterPtr->numSeverities;
    notificationFilter->numSecurityAlarmDetectors = ntfFilterPtr->numSecurityAlarmDetectors;
    notificationFilter->numServiceUsers = ntfFilterPtr->numServiceUsers;
    notificationFilter->numServiceProviders = ntfFilterPtr->numServiceProviders;
    notificationFilter->probableCauses = ntfFilterPtr->probableCauses;
    notificationFilter->severities = ntfFilterPtr->severities;
    notificationFilter->securityAlarmDetectors = ntfFilterPtr->securityAlarmDetectors;
    notificationFilter->serviceUsers = ntfFilterPtr->serviceUsers;
    notificationFilter->serviceProviders = ntfFilterPtr->serviceProviders;

    goto checkin_return;

destroy_handle_return:
    if (ntfFilterPtr->probableCauses)
        clHeapFree(ntfFilterPtr->probableCauses);
    if (ntfFilterPtr->severities) 
        clHeapFree(ntfFilterPtr->severities);
    if (ntfFilterPtr->securityAlarmDetectors)
        clHeapFree(ntfFilterPtr->securityAlarmDetectors);
    if (ntfFilterPtr->serviceUsers)
        clHeapFree(ntfFilterPtr->serviceUsers);
    if (ntfFilterPtr->serviceProviders)
        clHeapFree(ntfFilterPtr->serviceProviders);
    _ntfFreeFilterNtfHeaderBufferElements(&ntfFilterPtr->ntfFilterHeader);
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
    clHandleDestroy(ntfFilterHandleDbHandle, ntfFilterHandle);
    return _aspErrToAisError(rc);

checkin_return:
    clHandleCheckin(ntfFilterHandleDbHandle, ntfFilterHandle);
    return _aspErrToAisError(rc);
}


SaAisErrorT 
saNtfNotificationFilterFree(
    SaNtfNotificationFilterHandleT notificationFilterHandle)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfNotificationTypeT  notifType;

    /* Checkout the handle */
    rc = clHandleCheckout(ntfFilterHandleDbHandle, notificationFilterHandle, &notificationPtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }

    if (notificationPtr == NULL)
    {
        clLogError("NTF","FFR","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Get the notification type from the notificationPtr */
    notifType = *((SaNtfNotificationTypeT*)notificationPtr);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                ClNtfObjectCreateDeleteNotificationFilterBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationFilterBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                clHeapFree(ntfPtr->sourceIndicators);
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                ClNtfAttributeChangeNotificationFilterBufferT  *ntfPtr = (ClNtfAttributeChangeNotificationFilterBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                clHeapFree(ntfPtr->sourceIndicators);
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                ClNtfStateChangeNotificationFilterBufferT  *ntfPtr = (ClNtfStateChangeNotificationFilterBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                clHeapFree(ntfPtr->sourceIndicators);
                clHeapFree(ntfPtr->stateId);
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                ClNtfAlarmNotificationFilterBufferT  *ntfPtr = (ClNtfAlarmNotificationFilterBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                clHeapFree(ntfPtr->probableCauses);
                clHeapFree(ntfPtr->perceivedSeverities);
                clHeapFree(ntfPtr->trends);
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                ClNtfSecurityAlarmNotificationFilterBufferT  *ntfPtr = (ClNtfSecurityAlarmNotificationFilterBufferT*)notificationPtr;
                /* Free all the internal allocated values */
                clHeapFree(ntfPtr->probableCauses);
                clHeapFree(ntfPtr->severities);
                clHeapFree(ntfPtr->securityAlarmDetectors);
                clHeapFree(ntfPtr->serviceUsers);
                clHeapFree(ntfPtr->serviceProviders);
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            //ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationBufferT*)notificationPtr;
            break;
        default:
            clLogError("NTF","FFR","Invalid notification type found in the allocated notification data");
            /* Checkin and destroy the handle */
            clHandleCheckin(ntfFilterHandleDbHandle, notificationFilterHandle);
            return SA_AIS_ERR_LIBRARY;
    }

    /* Free the pointers allocated for filterHeader attributes */
    {
        ClNtfObjectCreateDeleteNotificationFilterBufferT  *ntfPtr = (ClNtfObjectCreateDeleteNotificationFilterBufferT*)notificationPtr;
        _ntfFreeFilterNtfHeaderBufferElements(&ntfPtr->ntfFilterHeader);
    } 

    /* Checkin and destroy the handle */
    clHandleCheckin(ntfFilterHandleDbHandle, notificationFilterHandle);

    rc = clHandleDestroy(ntfFilterHandleDbHandle, notificationFilterHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","FFR","handle destroy failed during notificationFree, rc 0x%x",rc);
    }
    
    return _aspErrToAisError(rc);
}

/******************************************************************************
 * NOTIFICATION SUBSCRIBE/UNSUBSCRIBE APIs
 *
 * * saNtfNotificationSubscribe_3()
 * * saNtfNotificationUnsubscribe_2()
 *
 ******************************************************************************/
SaAisErrorT 
saNtfNotificationSubscribe_3(
    const SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles,
    SaNtfSubscriptionIdT subscriptionId)
{
    ClRcT                   rc = CL_OK;
    void                   *notificationPtr = NULL;
    SaNtfHandleT            svcHandle = CL_HANDLE_INVALID_VALUE;
    SaNtfHandleT           *svcHandleCookie = NULL;

    if (notificationFilterHandles == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if (notificationFilterHandles->objectCreateDeleteFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    {
        /* Checkout the handle */
        rc = clHandleCheckout(ntfFilterHandleDbHandle, 
                              notificationFilterHandles->objectCreateDeleteFilterHandle,
                              &notificationPtr);
        if (rc != CL_OK)
        {
            return _aspErrToAisError(rc);
        }

        if (notificationPtr == NULL)
        {
            clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
            return SA_AIS_ERR_LIBRARY;
        }

        svcHandle = ((ClNtfObjectCreateDeleteNotificationFilterBufferT*)notificationPtr)->svcHandle;

        clHandleCheckin(ntfFilterHandleDbHandle,notificationFilterHandles->objectCreateDeleteFilterHandle);
    }
    else if (notificationFilterHandles->attributeChangeFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    {
        /* Checkout the handle */
        rc = clHandleCheckout(ntfFilterHandleDbHandle, 
                              notificationFilterHandles->attributeChangeFilterHandle,
                              &notificationPtr);
        if (rc != CL_OK)
        {
            return _aspErrToAisError(rc);
        }

        if (notificationPtr == NULL)
        {
            clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
            return SA_AIS_ERR_LIBRARY;
        }

        svcHandle = ((ClNtfAttributeChangeNotificationFilterBufferT*)notificationPtr)->svcHandle;

        clHandleCheckin(ntfFilterHandleDbHandle,notificationFilterHandles->attributeChangeFilterHandle);
    }
    else if (notificationFilterHandles->stateChangeFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    {
        /* Checkout the handle */
        rc = clHandleCheckout(ntfFilterHandleDbHandle, 
                              notificationFilterHandles->stateChangeFilterHandle,
                              &notificationPtr);
        if (rc != CL_OK)
        {
            return _aspErrToAisError(rc);
        }

        if (notificationPtr == NULL)
        {
            clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
            return SA_AIS_ERR_LIBRARY;
        }

        svcHandle = ((ClNtfStateChangeNotificationFilterBufferT*)notificationPtr)->svcHandle;

        clHandleCheckin(ntfFilterHandleDbHandle,notificationFilterHandles->stateChangeFilterHandle);
    }
    else if (notificationFilterHandles->alarmFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    {
        /* Checkout the handle */
        rc = clHandleCheckout(ntfFilterHandleDbHandle, 
                              notificationFilterHandles->alarmFilterHandle,
                              &notificationPtr);
        if (rc != CL_OK)
        {
            return _aspErrToAisError(rc);
        }

        if (notificationPtr == NULL)
        {
            clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
            return SA_AIS_ERR_LIBRARY;
        }

        svcHandle = ((ClNtfAlarmNotificationFilterBufferT*)notificationPtr)->svcHandle;

        clHandleCheckin(ntfFilterHandleDbHandle,notificationFilterHandles->alarmFilterHandle);
    }
    else if (notificationFilterHandles->securityAlarmFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    {
        /* Checkout the handle */
        rc = clHandleCheckout(ntfFilterHandleDbHandle, 
                              notificationFilterHandles->securityAlarmFilterHandle,
                              &notificationPtr);
        if (rc != CL_OK)
        {
            return _aspErrToAisError(rc);
        }

        if (notificationPtr == NULL)
        {
            clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
            return SA_AIS_ERR_LIBRARY;
        }

        svcHandle = ((ClNtfSecurityAlarmNotificationFilterBufferT*)notificationPtr)->svcHandle;

        clHandleCheckin(ntfFilterHandleDbHandle,notificationFilterHandles->securityAlarmFilterHandle);
    }
    /*
    else if (notificationFilterHandles->miscellaneousFilterHandle != SA_NTF_FILTER_HANDLE_NULL)
    { 
            Not implemented yet
    } */
    else {
        clLogError("NTF","SUB","Notification Filter should be supplied for atleast one of the notification types");
        return SA_AIS_ERR_INVALID_PARAM;
    }

    svcHandleCookie = clHeapAllocate(sizeof(SaNtfHandleT));
    if (svcHandleCookie == NULL)
    {
        return SA_AIS_ERR_NO_MEMORY;
    }

    *svcHandleCookie = svcHandle;

    /* Subscribe to event */
    rc = clEventSubscribe(evtSubscribeChannelHdl,
                          NULL, //No filters for now
                          subscriptionId,
                          (ClPtrT)svcHandleCookie);
    if (rc != CL_OK)
    {
        clLogError("NTF","SUB","Event subscription on global channel failed with rc 0x%x",rc);
        clHeapFree(svcHandleCookie);
        return _aspErrToAisError(rc);
    }

    return SA_AIS_OK;
}


SaAisErrorT 
saNtfNotificationUnsubscribe_2(
    SaNtfHandleT ntfHandle,
    SaNtfSubscriptionIdT subscriptionId)
{
    ClRcT                   rc = CL_OK;
    ClNtfLibInstanceT      *ntfInstancePtr = NULL;

    /* Checkout the handle */
    rc = clHandleCheckout(handleDbHandle, ntfHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        return _aspErrToAisError(rc);
    }
    
    if (ntfInstancePtr == NULL)
    {
        clLogError("NTF","SUB","handle pointer is found to be NULL after handle checkout");
        return SA_AIS_ERR_LIBRARY;
    }

    /* Unsubscribe to event */
    rc = clEventUnsubscribe(evtSubscribeChannelHdl,
                            subscriptionId);
    if (rc != CL_OK)
    {
        clLogError("NTF","SUB","Event desubscription on global channel failed with rc 0x%x",rc);
        return _aspErrToAisError(rc);
    }

    clHandleCheckin(handleDbHandle, ntfHandle);

    return SA_AIS_OK;
}

/******************************************************************************
 * EVENT CALLBACK FUNCTION
 *
 * This callback function gets invoked from event client on a delivery of an
 * event. This function processes the event and extracts the notification from 
 * the same.
 *
 ******************************************************************************/
static ClRcT 
_unmarshallAndCreateNotificationHandleFromEventData(ClBufferHandleT payLoadMsg,
                                                    SaNtfNotificationHandleT *notificationHandle,
                                                    ClUint32T                 handleSize,
                                                    SaNtfNotificationTypeT    notifType,
                                                    SaNtfNotificationHeaderT *notificationHeader)
{
    /* Create a handle of and unmarshall the values into
     * the handle memory */
    ClRcT                           rc = CL_OK;
    void                           *ntfPtr = NULL;
    ClNtfNotificationHeaderBufferT *ntfHeader = NULL;
    SaInt16T                       *variableDataSizePtr = NULL;
    SaUint16T                      *allocatedVariableDataSize = NULL;
    ClPtrT                         *variableDataPtr = NULL;
    SaUint16T                      *usedVariableDataSize = NULL;


    rc = clHandleCreate(notificationHandleDbHandle, handleSize, notificationHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Handle create failed while allocating notification");
        return rc;
    }

    /* Checkout the handle */
    rc = clHandleCheckout(notificationHandleDbHandle, *notificationHandle, &ntfPtr);

    CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

    if (ntfPtr == NULL)
    {
        clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
        rc=CL_ERR_LIBRARY;
        goto destroy_handle;
    }

    /* memset the notificationPtr */
    memset(ntfPtr, 0, handleSize);

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            ((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfObjectCreateDeleteNotificationBufferT*)ntfPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            ((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfAttributeChangeNotificationBufferT*)ntfPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            ((ClNtfStateChangeNotificationBufferT*)ntfPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfStateChangeNotificationBufferT*)ntfPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfStateChangeNotificationBufferT*)ntfPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfStateChangeNotificationBufferT*)ntfPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfStateChangeNotificationBufferT*)ntfPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfStateChangeNotificationBufferT*)ntfPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_ALARM:
            ((ClNtfAlarmNotificationBufferT*)ntfPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfAlarmNotificationBufferT*)ntfPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfAlarmNotificationBufferT*)ntfPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfAlarmNotificationBufferT*)ntfPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfAlarmNotificationBufferT*)ntfPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfAlarmNotificationBufferT*)ntfPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            ((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->notificationType = notifType;
            ntfHeader = &(((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->ntfHeader);
            variableDataSizePtr = &(((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->variableDataSize);
            allocatedVariableDataSize = &(((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->allocatedVariableDataSize);
            variableDataPtr = &(((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->variableDataPtr);
            usedVariableDataSize = &(((ClNtfSecurityAlarmNotificationBufferT*)ntfPtr)->usedVariableDataSize);
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            clLogError("NTF","NAL","SA_NTF_TYPE_MISCELLANEOUS notifications are not supported");
            rc = SA_AIS_ERR_NOT_SUPPORTED;
            goto destroy_handle;
        default:
            clLogError("NTF","NAL","Invalid notification type passed to allocate function");
            rc = SA_AIS_ERR_INVALID_PARAM;
            goto destroy_handle;
    }

    /* Now unmarshall all the values from the buffer and store them back
     * into the handle */
    rc = unmarshallNotificationHeaderBuffer(payLoadMsg, ntfHeader);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Failed to unmarshal the notification data. rc=0x%x",rc);
        goto error_return;
    }

    /* UnMarshall variable data */
    clXdrUnmarshallClUint16T(payLoadMsg, variableDataSizePtr);
    clXdrUnmarshallClUint16T(payLoadMsg, allocatedVariableDataSize);
    *variableDataPtr = clHeapCalloc(1,(*allocatedVariableDataSize));
    if (*variableDataPtr == NULL)
    {
        clLogError("NTF","EVT","Failed to allocate memory while unmarshalling the notification");
        rc = CL_ERR_NO_MEMORY;
        goto error_return;
    }
    clXdrUnmarshallArrayClCharT(payLoadMsg, *variableDataPtr, *allocatedVariableDataSize);
    clXdrUnmarshallClUint16T(payLoadMsg, usedVariableDataSize);


    /* Assign handle value */
    notificationHandle = notificationHandle;

    /* Assign notificationHeader values */
    notificationHeader->eventType                  = &(ntfHeader->eventType);
    notificationHeader->notificationObject         = &(ntfHeader->notificationObject);
    notificationHeader->notifyingObject            = &(ntfHeader->notifyingObject);
    notificationHeader->notificationClassId        = &(ntfHeader->notificationClassId);
    notificationHeader->eventTime                  = &(ntfHeader->eventTime);
    notificationHeader->numCorrelatedNotifications = ntfHeader->numCorrelatedNotifications;
    notificationHeader->lengthAdditionalText       = ntfHeader->lengthAdditionalText;
    notificationHeader->numAdditionalInfo          = ntfHeader->numAdditionalInfo;
    notificationHeader->notificationId             = &(ntfHeader->notificationId);
    notificationHeader->correlatedNotifications    = ntfHeader->correlatedNotifications;
    notificationHeader->additionalText             = ntfHeader->additionalText;
    notificationHeader->additionalInfo             = ntfHeader->additionalInfo;

    clHandleCheckin(notificationHandleDbHandle, *notificationHandle);
    return CL_OK;

error_return:
    _ntfFreeNotificationHeaderAndVarSizeDataElements(ntfHeader,*variableDataPtr);
destroy_handle:
    clHandleCheckin(notificationHandleDbHandle, *notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, *notificationHandle);
    return rc;
}


void clNtfEventDeliveryCallback(
        ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT         eventHandle,
        ClSizeT                eventDataSize)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT         payLoadMsg = 0;
    void                    *eventData = NULL;
    SaNtfNotificationTypeT  notifType = 0;
    SaNtfHandleT            *svcHandle = NULL;
    ClNtfLibInstanceT       *ntfInstancePtr = NULL;
    SaNtfNotificationsT     *notification = NULL;
    ClNtfNotificationCallbackDataT  *callbackData = NULL;
    ClHandleT               notificationHandle = CL_HANDLE_INVALID_VALUE;
    ClUint32T               index = 0;
    ClBoolT                 free_notification = CL_TRUE;

    clLogDebug("NTF","EVT","An NTF notification event has been received on event channel");
    
    /* First get the cookie and find out if the initialization is still valid */
    rc = clEventCookieGet(eventHandle, (void**)&svcHandle);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Failed to get cookie data from event handle. rc 0x%x",rc);
        goto free_return;
    }
    
    /* Checkout the svc instance handle */
    rc = clHandleCheckout(handleDbHandle, *svcHandle, (void *)&ntfInstancePtr);

    if (rc != CL_OK)
    {
        /* The instance might have been already finalized */
        clLogWarning("NTF","EVT","Failed to checkout the handle received in the callback");
        goto free_return;
    }

    /* Allocate callback data */
    eventData = clHeapAllocate(eventDataSize);
    notification = clHeapCalloc(1, (sizeof(SaNtfNotificationsT)));
    callbackData = clHeapCalloc(1, (sizeof(ClNtfNotificationCallbackDataT)));
    if ((notification == NULL) ||
            (callbackData == NULL) ||
            (eventData == NULL))
    {
        clLogError("NTF","EVT","Failed to allocate memory while invoking notification callback");
        goto free_notification_return;
    }

    rc = clEventDataGet (eventHandle, eventData,  &eventDataSize);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Failed to get event data. rc 0x%x",rc);
        goto free_notification_return;
    }

    /* Create a buffer */
    rc = clBufferCreate(&payLoadMsg);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Buffer creation failed while processing event. rc 0x%x",rc);
        goto free_notification_return;
    }

    rc = clBufferNBytesWrite(payLoadMsg, (ClUint8T *)eventData, eventDataSize);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Buffer write from event payload failed with rc 0x%x",rc);
        goto buffer_delete_return;
    }

    /* Unmarshall the notification type */
    rc = clXdrUnmarshallClUint32T(payLoadMsg, &notifType);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Failed to unmarshall notification type from the event data. rc 0x%x",rc);
        goto buffer_delete_return;
    }

    notification->notificationType = notifType;

    switch (notifType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            {
                /* Create a handle of and unmarshall the values into
                 * the handle memory */
                ClNtfObjectCreateDeleteNotificationBufferT  *ntfPtr = NULL;

                rc = _unmarshallAndCreateNotificationHandleFromEventData(payLoadMsg,
                            &notificationHandle,
                            sizeof(ClNtfObjectCreateDeleteNotificationBufferT),
                            notifType,
                            &(notification->notification.objectCreateDeleteNotification.notificationHeader));
                if (rc != CL_OK)
                {
                    clLogError("NTF","EVT","Failed to unmarshall the notification data");
                    goto buffer_delete_return;
                }

                /* Checkout the handle */
                rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&ntfPtr);

                CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

                if (ntfPtr == NULL)
                {
                    clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
                    rc = CL_ERR_LIBRARY;
                    goto destroy_handle_return;
                }

                /* Unmarshal notification specific data */
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numAttributes);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->sourceIndicator);
                ntfPtr->objectAttributes = clHeapCalloc(1,(sizeof(SaNtfAttributeT) * ntfPtr->numAttributes));
                if (ntfPtr->objectAttributes == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for object attributes");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }
                for (index = 0; index < ntfPtr->numAttributes; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->objectAttributes[index].attributeId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->objectAttributes[index].attributeType);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->objectAttributes[index].attributeType,
                            &ntfPtr->objectAttributes[index].attributeValue);
                }

                /* Assign handle value */
                notification->notification.objectCreateDeleteNotification.notificationHandle = notificationHandle;

                /* Assign notification specific attributes */
                notification->notification.objectCreateDeleteNotification.numAttributes = ntfPtr->numAttributes;
                notification->notification.objectCreateDeleteNotification.sourceIndicator = &(ntfPtr->sourceIndicator);
                notification->notification.objectCreateDeleteNotification.objectAttributes = ntfPtr->objectAttributes;
                
                clHandleCheckin(notificationHandleDbHandle, notificationHandle);
            }
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            {
                /* Create a handle of and unmarshall the values into
                 * the handle memory */
                ClNtfAttributeChangeNotificationBufferT  *ntfPtr = NULL;

                rc = _unmarshallAndCreateNotificationHandleFromEventData(payLoadMsg,
                            &notificationHandle,
                            sizeof(ClNtfAttributeChangeNotificationBufferT),
                            notifType,
                            &(notification->notification.attributeChangeNotification.notificationHeader));
                if (rc != CL_OK)
                {
                    clLogError("NTF","EVT","Failed to unmarshall the notification data");
                    goto buffer_delete_return;
                }

                /* Checkout the handle */
                rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&ntfPtr);

                CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

                if (ntfPtr == NULL)
                {
                    clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
                    goto buffer_delete_return;
                }

                /* Unmarshall notification specific data */
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numAttributes);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->sourceIndicator);
                ntfPtr->changedAttributes = clHeapCalloc(1,(sizeof(SaNtfAttributeChangeT) * ntfPtr->numAttributes));
                if (ntfPtr->changedAttributes == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for changed attributes");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }

                for (index = 0; index < ntfPtr->numAttributes; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->changedAttributes[index].attributeId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->changedAttributes[index].attributeType);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->changedAttributes[index].oldAttributePresent);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->changedAttributes[index].attributeType,
                            &ntfPtr->changedAttributes[index].oldAttributeValue);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->changedAttributes[index].attributeType,
                            &ntfPtr->changedAttributes[index].newAttributeValue);
                }

                /* Assign handle value */
                notification->notification.attributeChangeNotification.notificationHandle = notificationHandle;

                /* Assign notification specific attributes */
                notification->notification.attributeChangeNotification.numAttributes = ntfPtr->numAttributes;
                notification->notification.attributeChangeNotification.sourceIndicator = &(ntfPtr->sourceIndicator);
                notification->notification.attributeChangeNotification.changedAttributes = ntfPtr->changedAttributes;
                
                clHandleCheckin(notificationHandleDbHandle, notificationHandle);
            }
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            {
                /* Create a handle of and unmarshall the values into
                 * the handle memory */
                ClNtfStateChangeNotificationBufferT  *ntfPtr = NULL;

                rc = _unmarshallAndCreateNotificationHandleFromEventData(payLoadMsg,
                            &notificationHandle,
                            sizeof(ClNtfStateChangeNotificationBufferT),
                            notifType,
                            &(notification->notification.stateChangeNotification.notificationHeader));
                if (rc != CL_OK)
                {
                    clLogError("NTF","EVT","Failed to unmarshall the notification data");
                    goto buffer_delete_return;
                }

                /* Checkout the handle */
                rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&ntfPtr);

                CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

                if (ntfPtr == NULL)
                {
                    clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
                    goto buffer_delete_return;
                }

                /* Unmarshall notification specific values */
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numStateChanges);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->sourceIndicator);
                ntfPtr->changedStates = clHeapCalloc(1,(sizeof(SaNtfStateChangeT) * ntfPtr->numStateChanges));
                if (ntfPtr->changedStates == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for changed states");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }
                for (index = 0; index < ntfPtr->numStateChanges; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->changedStates[index].stateId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->changedStates[index].oldStatePresent);
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->changedStates[index].oldState);
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->changedStates[index].newState);
                }

                /* Assign handle value */
                notification->notification.stateChangeNotification.notificationHandle = notificationHandle;

                /* Assign notification specific attributes */
                notification->notification.stateChangeNotification.numStateChanges = ntfPtr->numStateChanges;
                notification->notification.stateChangeNotification.sourceIndicator = &(ntfPtr->sourceIndicator);
                notification->notification.stateChangeNotification.changedStates = ntfPtr->changedStates;

                clHandleCheckin(notificationHandleDbHandle, notificationHandle);
            }
            break;
        case SA_NTF_TYPE_ALARM:
            {
                /* Create a handle of and unmarshall the values into
                 * the handle memory */
                ClNtfAlarmNotificationBufferT  *ntfPtr = NULL;

                rc = _unmarshallAndCreateNotificationHandleFromEventData(payLoadMsg,
                            &notificationHandle,
                            sizeof(ClNtfAlarmNotificationBufferT),
                            notifType,
                            &(notification->notification.alarmNotification.notificationHeader));
                if (rc != CL_OK)
                {
                    clLogError("NTF","EVT","Failed to unmarshall the notification data");
                    goto buffer_delete_return;
                }

                /* Checkout the handle */
                rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&ntfPtr);

                CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

                if (ntfPtr == NULL)
                {
                    clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
                    goto buffer_delete_return;
                }

                /* Unmarshall notification specific data */
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numSpecificProblems);
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numMonitoredAttributes);
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->numProposedRepairActions);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->probableCause);
                ntfPtr->specificProblems = clHeapCalloc(1,(sizeof(SaNtfSpecificProblemT) * ntfPtr->numSpecificProblems));
                if (ntfPtr->specificProblems == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for alarm notification attributes");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }
                for (index = 0; index < ntfPtr->numSpecificProblems; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->specificProblems[index].problemId);
                    unmarshallNtfClassIdT(payLoadMsg, &ntfPtr->specificProblems[index].problemClassId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->specificProblems[index].problemType);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->specificProblems[index].problemType, 
                                                &ntfPtr->specificProblems[index].problemValue);
                }
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->perceivedSeverity);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->trend);
                clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->thresholdInformation.thresholdId);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->thresholdInformation.thresholdValueType);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->thresholdInformation.thresholdValueType,
                                                &ntfPtr->thresholdInformation.thresholdValue);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->thresholdInformation.thresholdValueType,
                                                &ntfPtr->thresholdInformation.thresholdHysteresis);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->thresholdInformation.thresholdValueType,
                                                &ntfPtr->thresholdInformation.observedValue);
                clXdrUnmarshallClInt64T(payLoadMsg, &ntfPtr->thresholdInformation.armTime);

                ntfPtr->monitoredAttributes = clHeapCalloc(1,(sizeof(SaNtfAttributeT) * ntfPtr->numMonitoredAttributes));
                if (ntfPtr->monitoredAttributes == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for alarm notification attributes");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }
                for (index = 0; index < ntfPtr->numMonitoredAttributes; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->monitoredAttributes[index].attributeId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->monitoredAttributes[index].attributeType);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->monitoredAttributes[index].attributeType, 
                                                &ntfPtr->monitoredAttributes[index].attributeValue);
                }
                ntfPtr->proposedRepairActions = clHeapCalloc(1,(sizeof(SaNtfProposedRepairActionT) * ntfPtr->numProposedRepairActions));
                if (ntfPtr->proposedRepairActions == NULL)
                {
                    clLogError("NTF","EVT","Failed to allocate memory for alarm notification attributes");
                    rc = CL_ERR_NO_MEMORY;
                    _ntfFreeNotificationHeaderAndVarSizeDataElements(&ntfPtr->ntfHeader, &ntfPtr->variableDataPtr);
                    goto destroy_handle_return;
                }
                for (index = 0; index < ntfPtr->numProposedRepairActions; ++index)
                {
                    clXdrUnmarshallClUint16T(payLoadMsg, &ntfPtr->proposedRepairActions[index].actionId);
                    clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->proposedRepairActions[index].actionValueType);
                    unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->proposedRepairActions[index].actionValueType, 
                                                &ntfPtr->proposedRepairActions[index].actionValue);
                }

                /* Assign handle value */
                notification->notification.alarmNotification.notificationHandle = notificationHandle;


                /* Assign notification specific attributes */
                notification->notification.alarmNotification.numSpecificProblems = ntfPtr->numSpecificProblems;
                notification->notification.alarmNotification.numMonitoredAttributes = ntfPtr->numMonitoredAttributes;
                notification->notification.alarmNotification.numProposedRepairActions = ntfPtr->numProposedRepairActions;
                notification->notification.alarmNotification.probableCause = &(ntfPtr->probableCause);
                notification->notification.alarmNotification.specificProblems = ntfPtr->specificProblems;
                notification->notification.alarmNotification.perceivedSeverity = &(ntfPtr->perceivedSeverity);
                notification->notification.alarmNotification.trend = &(ntfPtr->trend);
                notification->notification.alarmNotification.thresholdInformation = &(ntfPtr->thresholdInformation);
                notification->notification.alarmNotification.monitoredAttributes = ntfPtr->monitoredAttributes;
                notification->notification.alarmNotification.proposedRepairActions = ntfPtr->proposedRepairActions;

                clHandleCheckin(notificationHandleDbHandle, notificationHandle);
            }
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            {
                /* Create a handle of and unmarshall the values into
                 * the handle memory */
                ClNtfSecurityAlarmNotificationBufferT  *ntfPtr = NULL;

                rc = _unmarshallAndCreateNotificationHandleFromEventData(payLoadMsg,
                            &notificationHandle,
                            sizeof(ClNtfSecurityAlarmNotificationBufferT),
                            notifType,
                            &(notification->notification.securityAlarmNotification.notificationHeader));
                if (rc != CL_OK)
                {
                    clLogError("NTF","EVT","Failed to unmarshall the notification data");
                    goto buffer_delete_return;
                }

                /* Checkout the handle */
                rc = clHandleCheckout(notificationHandleDbHandle, notificationHandle, (void *)&ntfPtr);

                CL_ASSERT(rc == CL_OK); /* Should never happen because we have just checked out the handle */

                if (ntfPtr == NULL)
                {
                    clLogError("NTF","EVT","handle pointer is found to be NULL after handle checkout");
                    goto buffer_delete_return;
                }

                /* Unmarshall notification specific parameters */
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->probableCause);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->severity);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->securityAlarmDetector.valueType);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->securityAlarmDetector.valueType,
                                                &ntfPtr->securityAlarmDetector.value);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->serviceUser.valueType);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->serviceUser.valueType,
                                                &ntfPtr->serviceUser.value);
                clXdrUnmarshallClUint32T(payLoadMsg, &ntfPtr->serviceProvider.valueType);
                unmarshallNtfValueTypeT(payLoadMsg, ntfPtr->serviceProvider.valueType,
                                                &ntfPtr->serviceProvider.value);

                /* Assign handle value */
                notification->notification.alarmNotification.notificationHandle = notificationHandle;

                /* Assign notification specific attributes */
                notification->notification.securityAlarmNotification.probableCause = &(ntfPtr->probableCause);
                notification->notification.securityAlarmNotification.severity = &(ntfPtr->severity);
                notification->notification.securityAlarmNotification.securityAlarmDetector = &(ntfPtr->securityAlarmDetector);
                notification->notification.securityAlarmNotification.serviceUser = &(ntfPtr->serviceUser);
                notification->notification.securityAlarmNotification.serviceProvider = &(ntfPtr->serviceProvider);

                clHandleCheckin(notificationHandleDbHandle, notificationHandle);
            }
            break;
        case SA_NTF_TYPE_MISCELLANEOUS:
            break;
        default:
            clLogError("NTF","EVT","Invalid value of notification type received");
            goto buffer_delete_return;
    }

    /* Allocate the callback data */
    callbackData->svcHandle = *svcHandle;
    callbackData->subscriptionId = subscriptionId;
    callbackData->notificationPtr = notification;

    /* Now enqueue the callback */
    rc = clDispatchCbEnqueue(ntfInstancePtr->dispatchHandle,
                             CL_NTF_NOTIFICATION_CALLBACK,
                             (void*)callbackData);
    if (rc != CL_OK)
    {
        clLogError("NTF","EVT","Failed to enqueue the callback into the dispatch queue. rc 0x%x",rc);
        clHandleDestroy(notificationHandleDbHandle, notificationHandle);
        goto buffer_delete_return;
    }

    /* Everything went fine, so dont free the notification */
    free_notification = CL_FALSE;
    goto buffer_delete_return;

destroy_handle_return:
    clHandleCheckin(notificationHandleDbHandle, notificationHandle);
    clHandleDestroy(notificationHandleDbHandle, notificationHandle);

buffer_delete_return:
    clBufferDelete(&payLoadMsg);

free_notification_return:
    if (free_notification)
    {
        if (callbackData) clHeapFree(callbackData);
        if (notification) clHeapFree(notification);
    }
    clHandleCheckin(handleDbHandle, *svcHandle);

free_return:
    if(eventData) clHeapFree(eventData);
    clEventFree(eventHandle);
    return;
}

/******************************************************************************
 * NOTIFICATION SUBSCRIBE/UNSUBSCRIBE APIs
 *
 * * saNtfNotificationSubscribe_3()
 * * saNtfNotificationUnsubscribe_2()
 *
 ******************************************************************************/

SaAisErrorT 
saNtfNotificationReadNext(
    SaNtfReadHandleT readHandle,
    SaNtfSearchDirectionT searchDirection,
    SaNtfNotificationsT *notification)
{
    return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT 
saNtfNotificationReadInitialize_3(
   const SaNtfSearchCriteriaT *searchCriteria,
   const SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles,
   SaNtfReadHandleT *readHandle)
{
    return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT 
saNtfLocalizedMessageGet(
    SaNtfNotificationHandleT notificationHandle,
    SaStringT *message)
{
    return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT 
saNtfLocalizedMessageFree_2(
    SaNtfHandleT ntfHandle,
    SaStringT message)
{
    return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT saNtfNotificationReadFinalize(
    SaNtfReadHandleT readHandle)
{
    return SA_AIS_ERR_NOT_SUPPORTED;
}

