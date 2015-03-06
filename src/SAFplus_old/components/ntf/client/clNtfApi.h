#include <saNtf.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHandleApi.h>
#include <clEventApi.h>
#include <clVersionApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clDispatchApi.h>
#include <clErrorApi.h>
#include <clXdrApi.h>

#ifndef _CL_NTF_API_H_
#define _CL_NTF_API_H_

# ifdef __cplusplus
extern "C"
{
# endif

/* Name for the event channel used by Notification Service */
#define CL_NTF_EVENT_CHANNEL_NAME        "NTF_EVENT_CHANNEL"
#define CL_NTF_EVENT_CHANNEL_NAME_SIZE   17
#define CL_NTF_EVENT_PUBLISHER_NAME      "NTF_EVENT_PUBLISHER"
#define CL_NTF_EVENT_PUBLISHER_NAME_SIZE 19
#define MAX_FILE_NAME                    1024

/* System limit for maximum variable data size */
#define CL_NTF_SYSTEM_LIMIT_VAR_DATA_SIZE   (SaUint16T)(~((SaUint16T)0)) //Maximum SaUint16T value.

#define _aspErrToAisError(rc)            clErrorToSaf((rc))

/* Notification SVC handle instance structure that keeps
 * data related to notification service instance */
typedef struct ntf_instance {
    SaNtfCallbacksT_3  callbacks;
    ClHandleT          dispatchHandle;
    ClBoolT            finalized;
} ClNtfLibInstanceT;

/* Typedefinition for the type of callback queued into
 * the dispatch queue */
typedef enum {
    CL_NTF_NOTIFICATION_CALLBACK,
    CL_NTF_NOTIFICATION_DISCARDED_CALLBACK,
    CL_NTF_SUPPRESSION_FILTER_CALLBACK
} ClNtfCallbackTypeT;

/* Notification callback data */
typedef struct ntfCallbackData {
    SaNtfHandleT             svcHandle;
    SaNtfSubscriptionIdT     subscriptionId;
    SaNtfNotificationsT     *notificationPtr;
} ClNtfNotificationCallbackDataT;

/* Flat buffer representing notification header */
typedef struct notificationHeaderBuf {
    SaNtfNotificationHandleT notificationHandle;
    /* Fields of SaNtfNotificationHeaderT */
    SaNtfEventTypeT          eventType;
    SaNameT                  notificationObject;
    SaNameT                  notifyingObject;
    SaNtfClassIdT            notificationClassId;
    SaTimeT                  eventTime;
    SaUint16T                numCorrelatedNotifications;
    SaUint16T                lengthAdditionalText;
    SaUint16T                numAdditionalInfo;
    SaNtfIdentifierT         notificationId;
    SaNtfIdentifierT        *correlatedNotifications; //pointer to the array of notifiction ids
    SaStringT                additionalText; //character pointer
    SaNtfAdditionalInfoT    *additionalInfo; //pointer to array of additionalInfo structures
} ClNtfNotificationHeaderBufferT;

/* A flat buffer that indicates an instance of objectCreateDelete
 * notification. Used when notificationAllocate() function is 
 * invoked */
typedef struct objectCreateDeleteNtfBuf {
    /* Type of notification which can be used while freeing the 
     * allocated notification buffer */
    SaNtfNotificationTypeT          notificationType;
    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationHeaderBufferT  ntfHeader;

    /* Other fields from SaNtfObjectCreateDeleteNotificationT */
    SaUint16T                       numAttributes;
    SaNtfSourceIndicatorT           sourceIndicator;
    SaNtfAttributeT                *objectAttributes; //pointer to array of attributes

    /* Variable data */
    SaInt16T                        variableDataSize;
    SaUint16T                       allocatedVariableDataSize;
    ClPtrT                          variableDataPtr;
    SaUint16T                       usedVariableDataSize;

} ClNtfObjectCreateDeleteNotificationBufferT;


/* A flat buffer that indicates an instance of attributeChange
 * notification. Used when notificationAllocate() function is 
 * invoked */
typedef struct attributeChangeNotificationBuf {
    /* Type of notification which can be used while freeing the 
     * allocated notification buffer */
    SaNtfNotificationTypeT          notificationType;
    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationHeaderBufferT  ntfHeader;
    
    /* Other fields from
     */
    SaUint16T                       numAttributes;
    SaNtfSourceIndicatorT           sourceIndicator;
    SaNtfAttributeChangeT          *changedAttributes; //pointer to array of SaNtfAttributeChangeT types

    /* Variable data */
    SaInt16T                        variableDataSize;
    SaUint16T                       allocatedVariableDataSize;
    ClPtrT                          variableDataPtr;
    SaUint16T                       usedVariableDataSize;
} ClNtfAttributeChangeNotificationBufferT;


/* A flat buffer that indicates an instance of stateChange
 * notification. Used when notificationAllocate() function is 
 * invoked */
typedef struct stateChangeNotificationBuf {
    /* Type of notification which can be used while freeing the 
     * allocated notification buffer */
    SaNtfNotificationTypeT          notificationType;
    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationHeaderBufferT  ntfHeader;
    
    /* Other fields from
     */
    SaUint16T                       numStateChanges;
    SaNtfSourceIndicatorT           sourceIndicator;
    SaNtfStateChangeT              *changedStates; //pointer to array of SaNtfStateChangeT types

    /* Variable data */
    SaInt16T                        variableDataSize;
    SaUint16T                       allocatedVariableDataSize;
    ClPtrT                          variableDataPtr;
    SaUint16T                       usedVariableDataSize;
} ClNtfStateChangeNotificationBufferT;


/* A flat buffer that indicates an instance of alarm
 * notification. Used when notificationAllocate() function is 
 * invoked */
typedef struct alarmNotificationBuf {
    /* Type of notification which can be used while freeing the 
     * allocated notification buffer */
    SaNtfNotificationTypeT          notificationType;
    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationHeaderBufferT  ntfHeader;
    
    /* Other fields from
     */
    SaUint16T                       numSpecificProblems;
    SaUint16T                       numMonitoredAttributes;
    SaUint16T                       numProposedRepairActions;
    SaNtfProbableCauseT             probableCause;
    SaNtfSpecificProblemT          *specificProblems;
    SaNtfSeverityT                  perceivedSeverity;
    SaNtfSeverityTrendT             trend;
    SaNtfThresholdInformationT      thresholdInformation;
    SaNtfAttributeT                *monitoredAttributes;
    SaNtfProposedRepairActionT     *proposedRepairActions;

    /* Variable data */
    SaInt16T                        variableDataSize;
    SaUint16T                       allocatedVariableDataSize;
    ClPtrT                          variableDataPtr;
    SaUint16T                       usedVariableDataSize;
} ClNtfAlarmNotificationBufferT;


/* A flat buffer that indicates an instance of security alarm
 * notification. Used when notificationAllocate() function is 
 * invoked */
typedef struct securtyAlarmNotififcationBuf {
    /* Type of notification which can be used while freeing the 
     * allocated notification buffer */
    SaNtfNotificationTypeT          notificationType;
    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationHeaderBufferT  ntfHeader;
    
    /* Other fields from
     */
    SaNtfProbableCauseT             probableCause;
    SaNtfSeverityT                  severity;
    SaNtfSecurityAlarmDetectorT     securityAlarmDetector;
    SaNtfServiceUserT               serviceUser;
    SaNtfServiceUserT               serviceProvider;

    /* Variable data */
    SaInt16T                        variableDataSize;
    SaUint16T                       allocatedVariableDataSize;
    ClPtrT                          variableDataPtr;
    SaUint16T                       usedVariableDataSize;
} ClNtfSecurityAlarmNotificationBufferT;

/* Below are the notification filter buffers used to allocate
 * the notification filters */
typedef struct notificationFilterHeaderBuf {
    /* Filter handle */
    SaNtfNotificationFilterHandleT notificationFilterHandle;

    /* Fields of notificationFilterHeaderT */
    SaUint16T                      numEventTypes;
    SaNtfEventTypeT               *eventTypes;
    SaUint16T                      numNotificationObjects;
    SaNameT                       *notificationObjects;
    SaUint16T                      numNotifyingObjects;
    SaNameT                       *notifyingObjects;
    SaUint16T                      numNotificationClassIds;
    SaNtfClassIdT                 *notificationClassIds;
} ClNtfNotificationFilterHeaderBufferT;

/* Structure used to allocate the total memory used for an object createDelete
 * notificationFilter. This strucutre is used to store the data on the
 * handle database */
typedef struct objectCreateDeleteNotificationFilterBuffer {
    /* Type of notification which can be used while freeing the
     * allocation notification filter on the handle database */
    SaNtfNotificationTypeT         notificationType;

    /* Notification svc instance handle - this is required 
     * used during subscription */
    SaNtfHandleT                   svcHandle;

    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationFilterHeaderBufferT  ntfFilterHeader;

    /* Other attributes part of FilterT structure */
    SaUint16T                      numSourceIndicators;
    SaNtfSourceIndicatorT         *sourceIndicators;
} ClNtfObjectCreateDeleteNotificationFilterBufferT;

/* Structure used to allocate the total memory used for an AttributeChange
 * notificationFilter. This strucutre is used to store the data on the
 * handle database */
typedef struct attributeChangeNotificationFilterBuffer {
    /* Type of notification which can be used while freeing the
     * allocation notification filter on the handle database */
    SaNtfNotificationTypeT         notificationType;

    /* Notification svc instance handle - this is required to be
     * used during subscription */

    SaNtfHandleT                  svcHandle;

    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationFilterHeaderBufferT  ntfFilterHeader;

    SaUint16T                      numSourceIndicators;
    SaNtfSourceIndicatorT         *sourceIndicators;
} ClNtfAttributeChangeNotificationFilterBufferT;

/* Structure used to allocate the total memory used for an stateChange
 * notificationFilter. This strucutre is used to store the data on the
 * handle database */
typedef struct stateChangeNotificationFilterBuffer {

    /* Type of notification which can be used while freeing the
     * allocation notification filter on the handle database */
    SaNtfNotificationTypeT         notificationType;

    /* Notification svc instance handle - this is required to be
     * used during subscription */

    SaNtfHandleT                  svcHandle;

    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationFilterHeaderBufferT  ntfFilterHeader;

    SaUint16T                      numSourceIndicators;
    SaNtfSourceIndicatorT         *sourceIndicators;
    SaUint16T                      numStateChanges;
    SaNtfElementIdT               *stateId;
} ClNtfStateChangeNotificationFilterBufferT;


/* Structure used to allocate the total memory used for an alarm
 * notificationFilter. This strucutre is used to store the data on the
 * handle database */
typedef struct alarmNotificationFilterBuffer {
    /* Type of notification which can be used while freeing the
     * allocation notification filter on the handle database */
    SaNtfNotificationTypeT         notificationType;

    /* Notification svc instance handle - this is required to be
     * used during subscription */

    SaNtfHandleT                  svcHandle;

    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationFilterHeaderBufferT  ntfFilterHeader;

    SaUint16T                      numProbableCauses;
    SaUint16T                      numPerceivedSeverities;
    SaUint16T                      numTrends;
    SaNtfProbableCauseT           *probableCauses;
    SaNtfSeverityT                *perceivedSeverities;
    SaNtfSeverityTrendT           *trends;
} ClNtfAlarmNotificationFilterBufferT;


/* Structure used to allocate the total memory used for a security alarm
 * notificationFilter. This strucutre is used to store the data on the
 * handle database */
typedef struct securityAlarmNotificationFilterBuffer {
    /* Type of notification which can be used while freeing the
     * allocation notification filter on the handle database */
    SaNtfNotificationTypeT         notificationType;

    /* Notification svc instance handle - this is required to be
     * used during subscription */

    SaNtfHandleT                  svcHandle;

    /* Notification header buffer that contains notification
     * handle and other fields
     */
    ClNtfNotificationFilterHeaderBufferT  ntfFilterHeader;

    SaUint16T                      numProbableCauses;
    SaUint16T                      numSeverities;
    SaUint16T                      numSecurityAlarmDetectors;
    SaUint16T                      numServiceUsers;
    SaUint16T                      numServiceProviders;
    SaNtfProbableCauseT           *probableCauses;
    SaNtfSeverityT                *severities;
    SaNtfSecurityAlarmDetectorT   *securityAlarmDetectors;
    SaNtfServiceUserT             *serviceUsers;
    SaNtfServiceUserT             *serviceProviders;
} ClNtfSecurityAlarmNotificationFilterBufferT;


# ifdef __cplusplus
}
# endif

#endif 
