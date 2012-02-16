#ifndef _CL_AMS_CLIENT_NOTIFICATION_H_
#define _CL_AMS_CLIENT_NOTIFICATION_H_

#include <clCommon.h>
#include <clBufferApi.h>
#include <clEventApi.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clAmsUtils.h>
#include <clAmsErrors.h>
#include <clAmsEntities.h>
#include <saAmf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_AMS_EVENT_CHANNEL_NAME   "AMS_EVENT_CHANNEL"
#define CL_AMS_EVENT_PATTERN        "AMS_NOTIFICATION"
#define CL_AMS_EVENT_PUBLISHER_NAME "AMS_NOTIFICATION_MANAGER"
#define CL_AMS_EVENT_VERSION        "B.01.01"

typedef enum
{
    CL_AMS_NOTIFICATION_NONE,
    CL_AMS_NOTIFICATION_FAULT,
    CL_AMS_NOTIFICATION_SU_INSTANTIATION_FAILURE,
    CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE,
    CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED,
    CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED,
    CL_AMS_NOTIFICATION_SI_UNASSIGNED,
    CL_AMS_NOTIFICATION_COMP_ARRIVAL,
    CL_AMS_NOTIFICATION_COMP_DEPARTURE,
    CL_AMS_NOTIFICATION_NODE_ARRIVAL,
    CL_AMS_NOTIFICATION_NODE_DEPARTURE,
    CL_AMS_NOTIFICATION_ENTITY_CREATE,
    CL_AMS_NOTIFICATION_ENTITY_DELETE, 
    CL_AMS_NOTIFICATION_OPER_STATE_CHANGE,    
    CL_AMS_NOTIFICATION_ADMIN_STATE_CHANGE,
    CL_AMS_NOTIFICATION_MAX,
}ClAmsNotificationTypeT;

typedef struct
{
    ClAmsNotificationTypeT  type;
    ClAmsEntityTypeT  entityType;
    SaNameT  entityName;
    SaNameT  faultyCompName;
    SaNameT  siName;
    SaNameT  suName;
    SaAmfHAStateT lastHAState;
    SaAmfHAStateT newHAState;
    SaAmfRecommendedRecoveryT  recoveryActionTaken;
    ClBoolT  repairNecessary;
    ClAmsOperStateT lastOperState;
    ClAmsOperStateT newOperState;
    ClAmsAdminStateT lastAdminState;
    ClAmsAdminStateT newAdminState;
} ClAmsNotificationDescriptorT;

typedef struct ClAmsNotificationInfo
{
    ClAmsNotificationTypeT type;
    union
    {
        ClAmsNotificationDescriptorT amsStateInfo;
        ClCpmEventPayLoadT           amsCompInfo;
        ClCpmEventNodePayLoadT       amsNodeInfo;
    } amsNotificationInfo;

#define amsStateNotification amsNotificationInfo.amsStateInfo
#define amsCompNotification  amsNotificationInfo.amsCompInfo
#define amsNodeNotification  amsNotificationInfo.amsNodeInfo

}ClAmsNotificationInfoT;

typedef ClRcT (*ClAmsClientNotificationCallbackT)(ClAmsNotificationInfoT *);

ClRcT clAmsClientNotificationInitialize(ClAmsClientNotificationCallbackT callback);

ClRcT clAmsClientNotificationFinalize(void);

ClRcT clAmsNotificationEventPayloadExtract(ClEventHandleT eventHandle,
                                           ClSizeT eventDataSize,
                                           void *payLoad);

#ifdef __cplusplus
}
#endif

#endif
