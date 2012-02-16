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
 * 1. ptr_full_size
 * 2. array_full_size
 * 3. state_change_ntf_allocate_free
 * 4. alarm_ntf_allocate_free
 * 5. security_alarm_ntf_allocate_free
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

START_TEST_EXTERN(ptr_full_size)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT  handle = 0;
    void          *ptr = NULL;
    SaNtfObjectCreateDeleteNotificationT notification = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfObjectCreateDeleteNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     2,     //numAttributes
                                                     200); //Allocate 200 bytes
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationAllocate failed with rc 0x%x",rc);

    // Try to Allocate ptr value for 200
    rc = saNtfPtrValAllocate(notification.notificationHandle,
                             200,
                             &ptr,
                             &notification.objectAttributes[0].attributeValue);
    fail_unless(rc == SA_AIS_OK, "saNtfPtrValAllocate failed with rc 0x%x",rc);

    fail_unless(((notification.objectAttributes[0].attributeValue.ptrVal.dataSize == 200) && 
            (notification.objectAttributes[0].attributeValue.ptrVal.dataOffset == 0)),
            "Allocated ptr value paramters are not proper");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(array_full_size)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaNtfHandleT  handle = 0;
    void          *ptr = NULL;
    SaNtfObjectCreateDeleteNotificationT notification = {0};

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfObjectCreateDeleteNotificationAllocate(handle,
                                                     &notification,
                                                     4, //numCorelatedNotifications
                                                     100, //length additional text
                                                     2,     //numAdditionalInfo
                                                     2,     //numAttributes
                                                     200); //Allocate 200 bytes
    fail_unless(rc == SA_AIS_OK, "saNtfObjectCreateDeleteNotificationAllocate failed with rc 0x%x",rc);

    // Try to Allocate ptr value for 200
    rc = saNtfArrayValAllocate(notification.notificationHandle,
                               5, //num elements
                               40,  //element size
                               &ptr,
                               &notification.objectAttributes[0].attributeValue);
    fail_unless(rc == SA_AIS_OK, "saNtfPtrValAllocate failed with rc 0x%x",rc);

    fail_unless(((notification.objectAttributes[0].attributeValue.arrayVal.arrayOffset == 0) && 
            (notification.objectAttributes[0].attributeValue.arrayVal.numElements == 5) &&
            (notification.objectAttributes[0].attributeValue.arrayVal.elementSize == 40)),
            "Allocated array value paramters are not proper");

    rc = saNtfNotificationFree(notification.notificationHandle);
    fail_unless(rc == SA_AIS_OK, "saNtfNotificationFree failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN
