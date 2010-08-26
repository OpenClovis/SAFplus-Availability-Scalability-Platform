/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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


/************************************************************************
 * 
 * This file contains test cases for following APIs.
 *
 * saNtf<type>NotificationAllocate() 
 * saNtfNotificationFree()
 *
 * Following test cases are coded in this file.
 * 
 * 1. object_create_delete_ntf_filter_allocate_free
 * 2. attribute_change_ntf_filter_allocate_free
 * 3. state_change_ntf_filter_allocate_free
 * 4. alarm_ntf_filter_allocate_free
 * 5. security_alarm_ntf_filter_allocate_free
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static SaVersionT  correct_version = {'A',3,1};
static void notificationCallback (SaNtfSubscriptionIdT subscriptionId,
                           const SaNtfNotificationsT *notification)
{
    printf("Callback invoked");
}

static SaNtfCallbacksT_3 nonnull_callbacks = {
    .saNtfNotificationCallback      = notificationCallback,
    .saNtfNotificationDiscardedCallback  = NULL,
    .saNtfStaticSuppressionFilterSetCallback        = NULL
};

START_TEST_EXTERN(object_create_delete_ntf_filter_allocate_free)
{
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT   handle = 0;
    SaNtfObjectCreateDeleteNotificationFilterT filter = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfObjectCreateDeleteNotificationFilterAllocate(handle,
                                                           &filter,
                                                           4, //numEventTypes,
                                                           10, //numNotificationObjects
                                                           5,     //numNotifyingObjects
                                                           0,     //numNotificationClassIds
                                                           6);    //numSourceIndicators
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationFilterAllocate failed with rc 0x%x",rc);

    rc = saNtfNotificationFilterFree(filter.notificationFilterHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFilterFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(attribute_change_ntf_filter_allocate_free)
{
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT   handle = 0;
    SaNtfAttributeChangeNotificationFilterT filter = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfAttributeChangeNotificationFilterAllocate(handle,
                                                        &filter,
                                                        4, //numEventTypes,
                                                        10, //numNotificationObjects
                                                        5,     //numNotifyingObjects
                                                        0,     //numNotificationClassIds
                                                        6);    //numSourceIndicators
    fail_unless(rc == SA_AIS_OK, "saNtfAttributeChangeNotificationFilterAllocate failed with rc 0x%x",rc);

    rc = saNtfNotificationFilterFree(filter.notificationFilterHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFilterFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(state_change_ntf_filter_allocate_free)
{
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT   handle = 0;
    SaNtfStateChangeNotificationFilterT_2 filter = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfStateChangeNotificationFilterAllocate_2(handle,
                                                      &filter,
                                                      4, //numEventTypes,
                                                      10, //numNotificationObjects
                                                      5,     //numNotifyingObjects
                                                      0,     //numNotificationClassIds
                                                      6,    //numSourceIndicators
                                                      3);   //numChangedStates
    fail_unless(rc == SA_AIS_OK, "saNtfStateChangeNotificationFilterAllocate failed with rc 0x%x",rc);

    rc = saNtfNotificationFilterFree(filter.notificationFilterHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFilterFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(alarm_ntf_filter_allocate_free)
{
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT   handle = 0;
    SaNtfAlarmNotificationFilterT filter = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

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

    rc = saNtfNotificationFilterFree(filter.notificationFilterHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFilterFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(security_alarm_ntf_filter_allocate_free)
{
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT   handle = 0;
    SaNtfSecurityAlarmNotificationFilterT filter = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfSecurityAlarmNotificationFilterAllocate(handle,
                                                     &filter,
                                                     4, //numEventTypes,
                                                     10, //numNotificationObjects
                                                     5,     //numNotifyingObjects
                                                     0,     //numNotificationClassIds
                                                     3,   //numProbableCauses,
                                                     1,    //numPerceivedSeverities,
                                                     3,   //securityAlarmDetectors
                                                     2,   //numServiceUsers
                                                     1); //numServiceProviders,
    fail_unless(rc == SA_AIS_OK, "saNtfSecurityAlarmNotificationFilterAllocate failed with rc 0x%x",rc);

    rc = saNtfNotificationFilterFree(filter.notificationFilterHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFilterFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN
