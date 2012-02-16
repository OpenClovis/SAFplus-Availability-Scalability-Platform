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

/************************************************************************
 * 
 * This file contains test cases for following APIs.
 *
 * saNtf<type>NotificationAllocate() 
 * saNtfNotificationFree()
 *
 * Following test cases are coded in this file.
 * 
 * 1. object_create_delete_ntf_send
 * 2. attribute_change_ntf_send
 * 3. state_change_ntf_send
 * 4. alarm_ntf_send
 * 5. security_alarm_ntf_send
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static SaBoolT        callbackReceived = SA_FALSE;
static SaNtfHandleT   handle = 0;
static SaVersionT  correct_version = {'A',3,1};
static void notificationCallback (SaNtfSubscriptionIdT subscriptionId,
                           const SaNtfNotificationsT *notification)
{
    SaNtfIdentifierT    notificationId = 0;
    printf("Notification Received for subscription ID %d\n", subscriptionId);
    switch (notification->notificationType)
    {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            notificationId = *(notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
            printf("Received ObjectCreateDelete notification with ID %lld\n", notificationId);
            break;
        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            notificationId = *(notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
            printf("Received ObjectCreateDelete notification with ID %lld\n", notificationId);
            break;
        case SA_NTF_TYPE_STATE_CHANGE:
            notificationId = *(notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
            printf("Received ObjectCreateDelete notification with ID %lld\n", notificationId);
            break;
        case SA_NTF_TYPE_ALARM:
            notificationId = *(notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
            printf("Received ObjectCreateDelete notification with ID %lld\n", notificationId);
            break;
        case SA_NTF_TYPE_SECURITY_ALARM:
            notificationId = *(notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
            printf("Received ObjectCreateDelete notification with ID %lld\n", notificationId);
            break;
        default:
            break;
    }

    printf("This notification was sent from node %d, counter value %d\n", (ClUint32T)(notificationId  >> 32),
            (ClUint32T)(notificationId & 0x00000000FFFFFFFF));
    callbackReceived = SA_TRUE;
}

static void notificationCallback2 (SaNtfSubscriptionIdT subscriptionId,
                           const SaNtfNotificationsT *notification)
{
    SaAisErrorT     rc = SA_AIS_OK;
    void            *ptr=NULL;
    ClUint16T       dataSize = 0;
    printf("Notification Received for subscription ID %d\n", subscriptionId);
    callbackReceived = SA_TRUE;

    rc = saNtfPtrValGet(notification->notification.objectCreateDeleteNotification.notificationHandle,
            &notification->notification.objectCreateDeleteNotification.objectAttributes[0].attributeValue,
            &ptr,
            &dataSize);
    fail_unless(rc == SA_AIS_OK, "Couldnt retrive ptr val");
    if (rc != SA_AIS_OK)
        return;
    printf("Received data size %d\n",dataSize);
    printf("Received data = %s\n",(char*)ptr);

    printf("notification.objectAttributes[1].attributeType %d, val = %d\n",
            notification->notification.objectCreateDeleteNotification.objectAttributes[1].attributeType,
            notification->notification.objectCreateDeleteNotification.objectAttributes[1].attributeValue.uint32Val);

    rc = saNtfPtrValGet(notification->notification.objectCreateDeleteNotification.notificationHandle,
            &notification->notification.objectCreateDeleteNotification.objectAttributes[2].attributeValue,
            &ptr,
            &dataSize);
    fail_unless(rc == SA_AIS_OK, "Couldnt retrive ptr val");
    if (rc != SA_AIS_OK)
        return;
    printf("dataSize = %d ptr.length = %d , ptr.value= %s", dataSize, ((ClNameT*)ptr)->length, ((ClNameT*)ptr)->value);
}

static SaNtfCallbacksT_3 nonnull_callbacks = {
    .saNtfNotificationCallback      = notificationCallback,
    .saNtfNotificationDiscardedCallback  = NULL,
    .saNtfStaticSuppressionFilterSetCallback        = NULL
};

static SaNtfCallbacksT_3 callbacks = {
    .saNtfNotificationCallback      = notificationCallback2,
    .saNtfNotificationDiscardedCallback  = NULL,
    .saNtfStaticSuppressionFilterSetCallback        = NULL
};

START_TEST_EXTERN(object_create_delete_ntf_send)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfObjectCreateDeleteNotificationT notification = {0};
    SaNtfObjectCreateDeleteNotificationFilterT filter = {0};
    SaNtfNotificationTypeFilterHandlesT_3 notificationFilterHandles = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    /* First allocate a filter and then subscribe for the notification */
    rc = saNtfObjectCreateDeleteNotificationFilterAllocate(handle,
                                                           &filter,
                                                           4, //numEventTypes,
                                                           10, //numNotificationObjects
                                                           5,     //numNotifyingObjects
                                                           0,     //numNotificationClassIds
                                                           6);    //numSourceIndicators
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationFilterAllocate failed with rc 0x%x",rc);

    notificationFilterHandles.objectCreateDeleteFilterHandle = filter.notificationFilterHandle;
    notificationFilterHandles.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.alarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.miscellaneousFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe_3(&notificationFilterHandles, 10);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSubscribe_3 failed with rc 0x%x",rc);

    rc = saNtfObjectCreateDeleteNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     2,     //numAttributes
                                                     SA_NTF_ALLOC_SYSTEM_LIMIT);
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationAllocate failed with rc 0x%x",rc);

    callbackReceived = SA_FALSE;
    rc = saNtfNotificationSend(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSend failed with rc 0x%x",rc);

    sleep(2);

    rc = saNtfDispatch(handle, SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch failed with rc 0x%x",rc);

    sleep(2);

    fail_unless(callbackReceived == SA_TRUE, "ObjectCreateDelete notification is not received");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfNotificationUnsubscribe_2(handle, 10);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationUnsubscribe_2 failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(attribute_change_ntf_send)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfAttributeChangeNotificationT notification = {0};
    SaNtfAttributeChangeNotificationFilterT filter = {0};
    SaNtfNotificationTypeFilterHandlesT_3 notificationFilterHandles = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    /* First allocate a filter and then subscribe for the notification */
    rc = saNtfAttributeChangeNotificationFilterAllocate(handle,
                                                           &filter,
                                                           4, //numEventTypes,
                                                           10, //numNotificationObjects
                                                           5,     //numNotifyingObjects
                                                           0,     //numNotificationClassIds
                                                           6);    //numSourceIndicators
    fail_unless(rc == SA_AIS_OK, "saNtfAttributeChangeNotificationFilterAllocate failed with rc 0x%x",rc);

    notificationFilterHandles.objectCreateDeleteFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.attributeChangeFilterHandle = filter.notificationFilterHandle;
    notificationFilterHandles.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.alarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.miscellaneousFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe_3(&notificationFilterHandles, 11);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSubscribe_3 failed with rc 0x%x",rc);

    rc = saNtfAttributeChangeNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     2,     //numAttributes
                                                     SA_NTF_ALLOC_SYSTEM_LIMIT);
    fail_unless(rc == SA_AIS_OK, "saNtfAttributeChangeNotificationAllocate failed with rc 0x%x",rc);

    callbackReceived = SA_FALSE;
    rc = saNtfNotificationSend(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSend failed with rc 0x%x",rc);

    sleep(2);

    rc = saNtfDispatch(handle, SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch failed with rc 0x%x",rc);

    sleep(2);

    fail_unless(callbackReceived == SA_TRUE, "AttributeChange notification is not received");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfNotificationUnsubscribe_2(handle, 11);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationUnsubscribe_2 failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(state_change_ntf_send)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfStateChangeNotificationT notification = {0};
    SaNtfStateChangeNotificationFilterT_2 filter = {0};
    SaNtfNotificationTypeFilterHandlesT_3 notificationFilterHandles = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    /* First allocate a filter and then subscribe for the notification */
    rc = saNtfStateChangeNotificationFilterAllocate_2(handle,
                                                    &filter,
                                                    4, //numEventTypes,
                                                    10, //numNotificationObjects
                                                    5,     //numNotifyingObjects
                                                    0,     //numNotificationClassIds
                                                    6,    //numSourceIndicators
                                                    3);   //numChangedStates

    fail_unless(rc == SA_AIS_OK, "saNtfStateChangeNotificationFilterAllocate failed with rc 0x%x",rc);

    notificationFilterHandles.objectCreateDeleteFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.stateChangeFilterHandle = filter.notificationFilterHandle;
    notificationFilterHandles.alarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.miscellaneousFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe_3(&notificationFilterHandles, 12);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSubscribe_3 failed with rc 0x%x",rc);

    rc = saNtfStateChangeNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     2,     //numAttributes
                                                     SA_NTF_ALLOC_SYSTEM_LIMIT);
    fail_unless(rc == SA_AIS_OK, "saNtfStateChangeNotificationAllocate failed with rc 0x%x",rc);

    callbackReceived = SA_FALSE;
    rc = saNtfNotificationSend(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSend failed with rc 0x%x",rc);

    sleep(2);

    rc = saNtfDispatch(handle, SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch failed with rc 0x%x",rc);

    sleep(2);

    fail_unless(callbackReceived == SA_TRUE, "StateChange notification is not received");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfNotificationUnsubscribe_2(handle, 12);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationUnsubscribe_2 failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(alarm_ntf_send)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfAlarmNotificationT notification = {0};
    SaNtfAlarmNotificationFilterT filter = {0};
    SaNtfNotificationTypeFilterHandlesT_3 notificationFilterHandles = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    /* First allocate a filter and then subscribe for the notification */
    rc = saNtfAlarmNotificationFilterAllocate(handle,
                                              &filter,
                                              4, //numEventTypes,
                                              10, //numNotificationObjects
                                              5,     //numNotifyingObjects
                                              0,     //numNotificationClassIds
                                              3,   //numProbableCauses,
                                              1,    //numPerceivedSeverities,
                                              3);   //numTrends,

    fail_unless(rc == SA_AIS_OK, "saNtfAlarmNotificationFilterAllocate failed with rc 0x%x",rc);

    notificationFilterHandles.objectCreateDeleteFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.alarmFilterHandle = filter.notificationFilterHandle;
    notificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.miscellaneousFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe_3(&notificationFilterHandles, 13);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSubscribe_3 failed with rc 0x%x",rc);

    rc = saNtfAlarmNotificationAllocate(handle,
                                        &notification,
                                        4, //numCorelatedNotifications
                                        100, //length additional text
                                        2,     //numAdditionalInfo
                                        2,     //numSpecificProblems
                                        0,     //numMonitoredAttributes
                                        10,     //numProposedRepairActions
                                        SA_NTF_ALLOC_SYSTEM_LIMIT);

    fail_unless(rc == SA_AIS_OK, "saNtfAlarmNotificationAllocate failed with rc 0x%x",rc);

    callbackReceived = SA_FALSE;
    rc = saNtfNotificationSend(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSend failed with rc 0x%x",rc);

    sleep(2);

    rc = saNtfDispatch(handle, SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch failed with rc 0x%x",rc);

    sleep(2);

    fail_unless(callbackReceived == SA_TRUE, "Alarm notification is not received");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfNotificationUnsubscribe_2(handle, 13);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationUnsubscribe_2 failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(security_alarm_ntf_send)
{
    //Has some known issue -- not being tested for now.
} END_TEST_EXTERN

START_TEST_EXTERN(object_create_delete_ntf_send_with_ptr_val)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfObjectCreateDeleteNotificationT notification = {0};
    SaNtfObjectCreateDeleteNotificationFilterT filter = {0};
    SaNtfNotificationTypeFilterHandlesT_3 notificationFilterHandles = {0};
    void *ptr=NULL;
    ClNameT     *ptr3 = NULL;
    ClUint16T   size = 0;

    rc = saNtfInitialize_3(&handle, &callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    /* First allocate a filter and then subscribe for the notification */
    rc = saNtfObjectCreateDeleteNotificationFilterAllocate(handle,
                                                           &filter,
                                                           4, //numEventTypes,
                                                           10, //numNotificationObjects
                                                           5,     //numNotifyingObjects
                                                           0,     //numNotificationClassIds
                                                           6);    //numSourceIndicators
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationFilterAllocate failed with rc 0x%x",rc);

    notificationFilterHandles.objectCreateDeleteFilterHandle = filter.notificationFilterHandle;
    notificationFilterHandles.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.alarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    notificationFilterHandles.miscellaneousFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe_3(&notificationFilterHandles, 10);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSubscribe_3 failed with rc 0x%x",rc);

    rc = saNtfObjectCreateDeleteNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     5,     //numAttributes
                                                     800);
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationAllocate failed with rc 0x%x",rc);

    // Allocate a string object
    notification.objectAttributes[0].attributeType = SA_NTF_VALUE_STRING;
    rc = saNtfPtrValAllocate(notification.notificationHandle,
                             100,
                             &ptr,
                             &notification.objectAttributes[0].attributeValue);
    fail_unless(rc == SA_AIS_OK, "saNtfPtrValAllocate for 100 bytes failed with rc 0x%x");

    rc = saNtfVariableDataSizeGet(notification.notificationHandle,
                                  &size);
    fail_unless(((rc == SA_AIS_OK) && (size == 700)), "Available data size of %d is not proper",size);
    printf("Available data is %d\n",size);

    strncpy((ClCharT*)ptr,"Hello How are you man",200);

    // allocate an ClNameT object 
    notification.objectAttributes[1].attributeType = SA_NTF_VALUE_UINT32;
    notification.objectAttributes[1].attributeValue.uint32Val = 1000;

    // allocate an Uint32 object 
    printf("Sizeof ClNameT %d\n",sizeof(ClNameT));
    notification.objectAttributes[2].attributeType = SA_NTF_VALUE_LDAP_NAME;
    rc = saNtfPtrValAllocate(notification.notificationHandle,
                             sizeof(ClNameT),
                             (void**)&ptr3,
                             &notification.objectAttributes[2].attributeValue);

    fail_unless(rc==SA_AIS_OK, "saNtfPtrValAllocate for ClNameT failed with rc 0x%x",rc);

    if (rc == SA_AIS_OK)
    {
        ptr3->length= 17;
        strncpy(ptr3->value, "Shridhar Sahukar", 17);
    }

    callbackReceived = SA_FALSE;
    rc = saNtfNotificationSend(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationSend failed with rc 0x%x",rc);

    sleep(2);

    rc = saNtfDispatch(handle, SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch failed with rc 0x%x",rc);

    sleep(2);

    fail_unless(callbackReceived == SA_TRUE, "ObjectCreateDelete notification is not received");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfNotificationUnsubscribe_2(handle, 10);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationUnsubscribe_2 failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

