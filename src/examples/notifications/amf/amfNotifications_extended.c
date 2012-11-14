/*
 * Sample AMF notification example. 
 * This is an extension of amfNotifications.c example that tries to get active comps
 * 
 * The notification types are in clAmsClientNotification.h with ClAmsNotificationInfoT
 * for payload
 * The below examples shows the notifications for the important notifications that
 * our customers use to handle notifications. 
 * Some of them use it with their mgmt/confd wrapper
 *
 * clAmsNotificationInitialize sets up the notification.
 * Can be called by the comp. after saAmfInitialize for eg:
 *
 * clAmsNotificationFinalize just finalizes it.
 * Can be called by the comp. in app terminate before saAmfResponse for eg:
 *
 * The getComps routine below tries to obtain the active component 
 * from either SI partial ha state notifications or SU ha state notifications 
 * (any one though eg: below fires it from both on haState active for si/su notification)
 * The partial SI notifications with SU would come when the SI becomes active
 * or moves from fully assigned (redundant) to partially assigned on standby removal
 *
 * Another version: getCompsAsync tries to do the same but by fetching the components
 * through a task pool or job queue without blocking the amf notification callback context.
 *
 */

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsClientNotification.h>
#include <clJobQueue.h>

#define CL_AMS_STRING_NTF(S)                                        \
(   ((S) == CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED)        ? "partially assigned" :          \
    ((S) == CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED)        ? "fully assigned" : \
    ((S) == CL_AMS_NOTIFICATION_SI_UNASSIGNED)        ? "unassigned" : \
                                                  "Unknown" )

static ClAmsMgmtHandleT mgmtHandle;
static ClJobQueueT notifJobPool;
/*
 * If SI/SU ha state notifications indicate active ha state for si/su,
 * get SU comp list. 
 * Optionally, just fetch comp csi list as an example below
 */
static ClRcT getComps(const SaNameT *si, const SaNameT *su)
{
    ClAmsEntityT targetEntity = {0};
    ClAmsEntityBufferT compBuffer = {0};
    ClUint32T i;
    ClRcT rc = CL_OK;

#if 0
    ClAmsSUConfigT *suConfig = NULL;
    ClAmsSIConfigT *siConfig = NULL;

    targetEntity.type = CL_AMS_ENTITY_TYPE_SU;
    clNameCopy(&targetEntity.name, (ClNameT*)su);

    /*
     * If you want, get su or si config to check parent SG 
     */
    rc = clAmsMgmtEntityGetConfig(mgmtHandle, &targetEntity, (ClAmsEntityConfigT**)&suConfig);
    if(rc != CL_OK)
    {
        clLogError("GET", "COMP", "Get SU [%s] config returned with [%#x]",
                   targetEntity.name.value, rc);
        goto out;
    }
    clLogNotice("GET", "COMP", "SU [%s] parent SG [%s]",
                targetEntity.name.value, suConfig->parentSG.entity.name.value);
    clHeapFree(suConfig);

    /*
     * Or you can fetch SG from si config as well
     */
    targetEntity.type = CL_AMS_ENTITY_TYPE_SI;
    clNameCopy(&targetEntity.name, (ClNameT*)si);

    rc = clAmsMgmtEntityGetConfig(mgmtHandle, &targetEntity, (ClAmsEntityConfigT**)&siConfig);
    if(rc != CL_OK)
    {
        clLogError("GET", "COMP", "GET SI [%s] config returned with [%#x]",
                   targetEntity.name.value, rc);
        goto out;
    }
    clLogNotice("GET", "COMP", "SI [%s] parent SG [%s]", 
                targetEntity.name.value, siConfig->parentSG.entity.name.value);
    clHeapFree(siConfig);
#endif

    targetEntity.type = CL_AMS_ENTITY_TYPE_SU;
    clNameCopy(&targetEntity.name, (ClNameT*)su);

    /*  
     * Get SU comp list for the active components since we fetch this on 
     * active ha state notifications.
     * Additionally, just fetch the comp csi list as an extended example
     * just in case ...
     */
    rc = clAmsMgmtGetSUCompList(mgmtHandle, &targetEntity, &compBuffer);
    if(rc != CL_OK)
    {
        clLogNotice("GET", "COMP", "SU [%s] comp list returned with [%#x]",
                    targetEntity.name.value, rc);
        goto out;
    }
    ClAmsCompCSIRefBufferT csiBuffer = {0};
    for(i = 0; i < compBuffer.count; ++i)
    {
        ClAmsEntityT *comp = compBuffer.entity + i;
        clLogNotice("GET", "COMP", "SU [%s], comp [%s]",
                    targetEntity.name.value, comp->name.value);
        /*
         * Optional step since the above function would be invoked
         * anyway on active ha state transitions in which case, above comp
         * would be active,
         */
        rc = clAmsMgmtGetCompCSIList(mgmtHandle, comp, &csiBuffer);
        if(rc != CL_OK)
        {
            clLogError("GET", "COMP", "Comp csi list returned with [%#x]", rc);
            goto out;
        }
        for(ClUint32T j = 0; j < csiBuffer.count; ++j )
        {
            ClAmsCompCSIRefT *csiRef = csiBuffer.entityRef + j;
            if(csiRef->haState == CL_AMS_HA_STATE_ACTIVE
               &&
               csiRef->activeComp)
            {
                clLogNotice("GET", "COMP", "CSI [%s], Active comp [%s]",
                            csiRef->entityRef.entity.name.value,
                            csiRef->activeComp->name.value);
                        
            }
        }
        clAmsMgmtFreeCompCSIRefBuffer(&csiBuffer);
        csiBuffer.count = 0;
    }

    out:
    if(csiBuffer.entityRef)
        clAmsMgmtFreeCompCSIRefBuffer(&csiBuffer);
    if(compBuffer.entity)
        clHeapFree(compBuffer.entity);

    return rc;
}

/*
 * Async version through task pool thread to just unblock the amf client notification
 * callback
 */
__attribute__((unused)) static ClRcT getCompsAsyncJobRequest(ClPtrT arg)
{
    SaNameT **arr = arg;
    ClRcT rc = CL_OK;
    CL_ASSERT(arr != NULL);
    CL_ASSERT(arr[0] && arr[1]);
    rc = getComps(arr[0], arr[1]);
    clHeapFree(arr[0]);
    clHeapFree(arr[1]);
    clHeapFree(arr);
    return rc;
}

/*
 * In case you want to unblock the AMF notification callback context,
 * you can force this through a task pool thread
 */
__attribute__((unused)) static ClRcT getCompsAsync(const SaNameT *si, const SaNameT *su)
{
    ClRcT rc = CL_OK;
    SaNameT *cloneSI = clHeapCalloc(1, sizeof(*cloneSI));
    SaNameT *cloneSU = clHeapCalloc(1, sizeof(*cloneSU));
    SaNameT **arr = clHeapCalloc(2, sizeof(*arr));
    CL_ASSERT(cloneSI && cloneSU && arr);

    memcpy(cloneSI, si, sizeof(*cloneSI));
    memcpy(cloneSU, su, sizeof(*cloneSU));
    arr[0] = cloneSI;
    arr[1] = cloneSU;
    if(!notifJobPool.flags)
    {
        rc = clJobQueueInit(&notifJobPool, 0, 1);
        if(rc != CL_OK)
        {
            clLogError("GET", "COMP", "Job queue init returned with [%#x]", rc);
            goto out;
        }
    }
    rc = clJobQueuePush(&notifJobPool, getCompsAsyncJobRequest, (ClPtrT)arr);
    out:
    return rc;
}

static ClRcT amsNotificationCallback(ClAmsNotificationInfoT *notification)
{
    switch(notification->type)
    {
        /*
         * Partially assigned when SI is assigned active with no standby 
         * or standby removed to transition from fully assigned
         */
    case CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED:
        {
            clLogNotice("EVT", "NTF", "Received SI [%s] event", CL_AMS_STRING_NTF(notification->type));
            clLogNotice("EVT", "NTF", "SI name : [%.*s]",
                      notification->amsStateNotification.siName.length,
                      notification->amsStateNotification.siName.value);
            clLogNotice("EVT", "NTF", "SU name : [%.*s]",
                      notification->amsStateNotification.suName.length,
                      notification->amsStateNotification.suName.value);
            clLogNotice("EVT", "NTF", "Last HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.lastHAState));
            clLogNotice("EVT", "NTF", "New HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.newHAState));
            
            /*
             * Get comps for the ha state
             */
            if(notification->amsStateNotification.newHAState == SA_AMF_HA_ACTIVE)
            {
                getComps(&notification->amsStateNotification.siName,
                         &notification->amsStateNotification.suName);
            }
            break;
        }
        /*
         * Fully assigned SI notification when SI is fully assigned
         * or when standby is assigned to SI with active assignments
         * making it redundant or available.
         */
    case CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED:
        {
            clLogNotice("EVT", "NTF", "Received SI [%s] event", CL_AMS_STRING_NTF(notification->type));
            clLogNotice("EVT", "NTF", "SI name : [%.*s]",
                      notification->amsStateNotification.siName.length,
                      notification->amsStateNotification.siName.value);
            clLogNotice("EVT", "NTF", "SU name : [%.*s]",
                      notification->amsStateNotification.suName.length,
                      notification->amsStateNotification.suName.value);
            clLogNotice("EVT", "NTF", "Last HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.lastHAState));
            clLogNotice("EVT", "NTF", "New HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.newHAState));

            break;
        }
        /*
         * Removal of SI assignment
         */
    case CL_AMS_NOTIFICATION_SI_UNASSIGNED:
        {
            clLogNotice("EVT", "NTF", "Received SI [%s] event", CL_AMS_STRING_NTF(notification->type));
            clLogNotice("EVT", "NTF", "SI name : [%.*s]",
                      notification->amsStateNotification.siName.length,
                      notification->amsStateNotification.siName.value);
            clLogNotice("EVT", "NTF", "SU name : [%.*s]",
                      notification->amsStateNotification.suName.length,
                      notification->amsStateNotification.suName.value);
            clLogNotice("EVT", "NTF", "Last HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.lastHAState));
            clLogNotice("EVT", "NTF", "New HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.newHAState));

            break;
        }
        /*
         * SU ha state change notifications with SI
         */
    case CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE:
        {
            clLogNotice("EVT", "NTF", "Received SU HA state change event");
            clLogNotice("EVT", "NTF", "SU name : [%.*s]",
                      notification->amsStateNotification.suName.length,
                      notification->amsStateNotification.suName.value);
            clLogNotice("EVT", "NTF", "SI name : [%.*s]",
                      notification->amsStateNotification.siName.length,
                      notification->amsStateNotification.siName.value);
            clLogNotice("EVT", "NTF", "Last HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.lastHAState));
            clLogNotice("EVT", "NTF", "New HA State : [%s]",
                      CL_AMS_STRING_H_STATE(notification->amsStateNotification.newHAState));
            if(notification->amsStateNotification.newHAState == SA_AMF_HA_ACTIVE)
            {
                getComps(&notification->amsStateNotification.siName,
                         &notification->amsStateNotification.suName);
            }
            break;
        }

    case CL_AMS_NOTIFICATION_OPER_STATE_CHANGE:
        {
            clLogNotice("EVT", "NTF", "Received operational state [%s - %s] notification for type [%s] "
                      "entity [%.*s]", CL_AMS_STRING_O_STATE(notification->amsStateNotification.lastOperState),
                      CL_AMS_STRING_O_STATE(notification->amsStateNotification.newOperState),
                      CL_AMS_STRING_ENTITY_TYPE(notification->amsStateNotification.entityType),
                      notification->amsStateNotification.entityName.length,
                      notification->amsStateNotification.entityName.value);
        }
        break;

    case CL_AMS_NOTIFICATION_ADMIN_STATE_CHANGE:
        {
            clLogNotice("EVT", "NTF", "Received admin state [%s - %s] notification for type [%s] "
                      "entity [%.*s]", 
                      CL_AMS_STRING_A_STATE(notification->amsStateNotification.lastAdminState),
                      CL_AMS_STRING_A_STATE(notification->amsStateNotification.newAdminState),
                      CL_AMS_STRING_ENTITY_TYPE(notification->amsStateNotification.entityType),
                      notification->amsStateNotification.entityName.length,
                      notification->amsStateNotification.entityName.value);
        }
        break;

    case CL_AMS_NOTIFICATION_COMP_DEPARTURE:
        {
            clLogNotice ("EVT", "NTF", "Component [%s] for [%.*s]",
                       notification->amsCompNotification.operation == CL_CPM_COMP_DEATH ?
                       "death" : "departure",
                       notification->amsCompNotification.compName.length,
                       notification->amsCompNotification.compName.value);
            clLogNotice("EVT", "NTF", "Comp node [%.*s]", 
                      notification->amsCompNotification.nodeName.length,
                      notification->amsCompNotification.nodeName.value);

        }
        break;
        
    case CL_AMS_NOTIFICATION_NODE_DEPARTURE:
        {
            if(notification->amsNodeNotification.operation == CL_CPM_NODE_DEPARTURE
               ||
               notification->amsNodeNotification.operation == CL_CPM_NODE_DEATH)
            {
                clLogNotice("EVT", "NTF", "Node [%s] for [%.*s], address [%d]",
                          notification->amsNodeNotification.operation == CL_CPM_NODE_DEPARTURE ?
                          "departure" : "death",
                          notification->amsNodeNotification.nodeName.length,
                          notification->amsNodeNotification.nodeName.value,
                          notification->amsNodeNotification.nodeIocAddress);
            }
        }
        break;

    default:
        {
            break;
        }
    }
    
    return CL_OK;
}

/*
 * clAmsNotificationInitialize sets up the notification.
 * Can be called by the comp. after saAmfInitialize for eg:
 */
ClRcT clAmsNotificationInitialize(void)
{
    ClRcT rc = CL_OK;
    static ClVersionT version = { 'B', 1, 1 };
    rc = clAmsMgmtInitialize(&mgmtHandle, NULL, &version);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", "Mgmt initialize returned with [%#x]", rc);
        return rc;
    }
    if((rc = clAmsClientNotificationInitialize(amsNotificationCallback) ) != CL_OK)
    {
        clLogWarning("EVT", "NTF", "AMF notification initialize returned with [%#x]", rc);
    }
    return rc;
}

/*
 * clAmsNotificationFinalize just finalizes it.
 * Can be called by the comp. in app terminate before saAmfResponse for eg:
 */
void clAmsNotificationFinalize(void)
{
    clAmsClientNotificationFinalize();
    if(mgmtHandle)
    {
        clAmsMgmtFinalize(mgmtHandle);
        mgmtHandle = 0;
    }
    if(notifJobPool.flags)
        clJobQueueDelete(&notifJobPool);
}
