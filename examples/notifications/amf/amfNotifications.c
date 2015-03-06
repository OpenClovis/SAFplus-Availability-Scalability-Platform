/*
 * Sample AMF notification example. 
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
 */

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsClientNotification.h>

#define CL_AMS_STRING_NTF(S)                                        \
(   ((S) == CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED)        ? "partially assigned" :          \
    ((S) == CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED)        ? "fully assigned" : \
    ((S) == CL_AMS_NOTIFICATION_SI_UNASSIGNED)        ? "unassigned" : \
                                                  "Unknown" )

static ClRcT amsNotificationCallback(ClAmsNotificationInfoT *notification)
{
    switch(notification->type)
    {
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

            break;
        }
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

    case CL_AMS_NOTIFICATION_COMP_ARRIVAL:
        {
            clLogNotice ("EVT", "NTF", "Component arrival for [%.*s]",
                       notification->amsCompNotification.compName.length,
                       notification->amsCompNotification.compName.value);
            clLogNotice("EVT", "NTF", "Comp node [%.*s]", 
                      notification->amsCompNotification.nodeName.length,
                      notification->amsCompNotification.nodeName.value);

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
}
