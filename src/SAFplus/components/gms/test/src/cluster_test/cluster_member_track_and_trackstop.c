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
 * clGmsClusterTrack() 
 * clGmsClusterTrackStop()
 *
 * Following test cases are coded in this file.
 * 
 * 1. track_with_callback_param_null
 * 2. track_with_null_callback_values
 * 3. track_with_wrong_flag
 * 4. track_with_invalid_params
 * 5. track_with_invalid_handle
 * 6. track_changes_and_changes_only_flag_test
 * 7. track_without_notify_buff
 * 8. track_with_notify_buff
 * 9. track_stop_without_track_registration
 * 10. track_stop_after_track_current
 * 11. track_stop_with_track_changes_flag
 * 12. track_stop_with_track_changes_only_flag
 * 13. double_track_stop
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClBoolT     callback_invoked = CL_FALSE;
static ClGmsNodeIdT dummyNodeId = 1234;
static SaNameT      dummyNodeName = {
    .value = "Dummy Node",
    .length = 10
};

static ClGmsCallbacksT callbacks_all_null = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};

static void clGmsClusterTrackCallbackFuntion (
        CL_IN const ClGmsClusterNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc);

static ClGmsCallbacksT callbacks_non_null = {
    .clGmsClusterTrackCallback      = clGmsClusterTrackCallbackFuntion,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};

static void  clGmsClusterMemberEjectCallbackFunction (
        CL_IN ClGmsMemberEjectReasonT   reasonCode)
{
    printf("ClusterMemberEjection invoked with reason code %d\n",reasonCode);
}

static ClGmsClusterManageCallbacksT        clusterManageCallback = {
    .clGmsMemberEjectCallback = clGmsClusterMemberEjectCallbackFunction
};

static void clGmsClusterTrackCallbackFuntion (
        CL_IN const ClGmsClusterNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc)
{
    callback_invoked = CL_TRUE;
    printf("Inside cluster track callback function. \n");
}


START_TEST_EXTERN(track_with_callback_param_null)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* 
     * Try cluster track with track_changes or track_changes_only flag. It
     * should fail with rc CL_ERR_NO_CALLBACK 
     */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CHANGES, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /* Try same test with CL_GMS_TRACK_CHANGES_ONLY flag */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /*
     * Try same test with CL_GMS_TRACK_CURRENT flag, but notificationBuffer
     * parameter is NULL, so callback should be provided in this case.
     */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CURRENT, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no trackflags are registered. rc = 0x%x",rc);


    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_with_null_callback_values)
{
    ClRcT       rc = CL_OK;
    /*
     * This test is slightly different from the above one,
     * in the sense that, in the privious test, the callbacks
     * parameter itself was null. But here, we provide the 
     * callbacks structure, but the structure members are NULL
     */
    rc = clGmsInitialize(&handle,&callbacks_all_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /*
     * Try cluster track with track_changes or track_changes_only flag. It
     * should fail with rc CL_ERR_NO_CALLBACK
     */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CHANGES, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /* Try same test with CL_GMS_TRACK_CHANGES_ONLY flag */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_with_wrong_flag)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Try with track flag = 0 */
    rc = clGmsClusterTrack(handle, 0, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_FLAG,
            "clGmsClusterTrack wrongly accepted 0 track flag value. rc = 0x%x",rc);

    /* Try with track flag other than 1/2/4 values */
    rc = clGmsClusterTrack(handle, 112, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_FLAG,
            "clGmsClusterTrack wrongly accepted invalid track flag value. rc = 0x%x",rc);

    /* Try with combination of CL_GMS_TRACK_CHANGES_ONLY 
     * and CL_GMS_TRACK_CHANGES flags */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CHANGES|CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_FLAG,
            "clGmsClusterTrack wrongly accepted combination of CL_GMS_TRACK_CHANGES \
            & CL_GMS_TRACK_CHANGES_ONLY flag. rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(track_with_invalid_params)
{
    ClRcT       rc = CL_OK;
    ClGmsClusterNotificationBufferT notifyBuff = {0};

    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsClusterNotificationT));

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /*
     * Here notification buffer is not null, but number of items is 0
     * so it should return CL_ERR_INVALID_PARAM
     */
    rc = clGmsClusterTrack(handle, CL_GMS_TRACK_CURRENT, &notifyBuff);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "clGmsClusterTrack wrongly accepted invalid parameters. rc = 0x%x",rc);
    
    clHeapFree(notifyBuff.notification);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(track_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    rc = clGmsClusterTrack(0,CL_GMS_TRACK_CURRENT,NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterTrack is successful even with invalid handle. rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(track_changes_and_changes_only_flag_test)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    printf("!!!!!!!!CAlling join here..\n");
    /* Do a node join */
    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);

    /* Register for TRACK_CHANGES_ONLY flag */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES_ONLY flag failed with rc = 0x%x",rc);

    dummyNodeId = 1234;
    /* Do node leave */
    rc = clGmsClusterLeave(handle,
                           0,
                           dummyNodeId);
    fail_unless(rc == CL_OK,
            "clGmsClusterLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is invoked. */
    sleep(5);

    printf("Calling track stop...\n");
    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);
    
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_without_notify_buff)
{
    /*
     * In this test case, we pass the notificationBuffer
     * parameter but dont provide inner buffer for 
     * returning the cluster information. GMS will allocate
     * the buffer and returns. Verify whether the node information
     * is proper
     */

    ClRcT       rc = CL_OK;
    ClGmsClusterNotificationBufferT notifyBuff = {0};

    printf("Invoking track with notify buffer ...!!!!!!!!!\n");
    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    /* Verify that gms has allocated the notification buffer pointer and
     * the number of members parameter also */
    fail_unless(((notifyBuff.notification != NULL) && (notifyBuff.numberOfItems > 0)),
            "GMS has not allocated the notification buffer and not updated \
            numberOfItems value. NoOfItems=%d",notifyBuff.numberOfItems);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_with_notify_buff)
{
    /*
     * In this test case, we pass the notificationBuffer
     * along with inner notification buffer as well as
     * number of Items.
     */

    ClRcT       rc = CL_OK;
    ClGmsClusterNotificationBufferT notifyBuff = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsClusterNotificationT));
    notifyBuff.numberOfItems = 1;

    /* cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_stop_without_track_registration)
{
    ClRcT       rc = CL_OK;

    /* 
     * This test case would verify the clGmsClusterTrackStop CLI
     * without invoking a prior clGmsClusterTrack. The return value should
     * be CL_ERR_DOESNT_EXIST
     */

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    rc = clGmsClusterTrackStop(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsClusterTrackStop was successful even without a prior \
            clGmsClusterTrack. rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_stop_after_track_current)
{
    /* 
     * This test case would verify the clGmsClusterTrackStop CLI
     * by first invoking a clGmsClusterTrack with CL_GMS_TRACK_CURRENT
     * flag, and then invoking track stop. This verifies that TRACK_CURRENT
     * does not add a node to track node list. rc should be CL_ERR_DOESNT_EXIST
     */

    ClRcT       rc = CL_OK;
    ClGmsClusterNotificationBufferT notifyBuff = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsClusterNotificationT));
    notifyBuff.numberOfItems = 1;

    /* cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    /* invoke track stop. */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsClusterTrackStop was successful even without a prior \
            clGmsClusterTrack. rc = 0x%x",rc);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(track_stop_with_track_changes_flag)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    printf("Inside track_stop_with_track_changes_flag\n");
    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);
    printf("Calling track stop\n");

    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);

    /* Do node leave */
    printf("Calling node leave\n");
    rc = clGmsClusterLeave(handle,
                           0,
                           dummyNodeId);
    fail_unless(rc == CL_OK,
            "clGmsClusterLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is not invoked. */
    sleep(5);


    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN



START_TEST_EXTERN(track_stop_with_track_changes_only_flag)
{
    ClRcT       rc = CL_OK;

    printf("Inside track_stop_with_track_changes_only_flag\n");
    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    /* Do a node join */
    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);

    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);

    /* Do node leave */
    rc = clGmsClusterLeave(handle,
                           0,
                           dummyNodeId);
    fail_unless(rc == CL_OK,
            "clGmsClusterLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is not invoked. */
    sleep(5);


    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(double_track_stop)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);

    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);

    /* Do node leave */
    rc = clGmsClusterLeave(handle,
                           0,
                           dummyNodeId);
    fail_unless(rc == CL_OK,
            "clGmsClusterLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is not invoked. */
    sleep(5);

    /* Do another trackStop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "double invokation of clGmsClusterTrackStop was successful \
            with rc 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN

START_TEST_EXTERN(verify_track_checkpointing)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Now kill gms server */
    sleep(2);
    system("killall -9 safplus_gms");
    sleep(10);

    callback_invoked = CL_FALSE;
    /* Do a node join */
    dummyNodeId = 1234;
    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Track callback is not invoked after killing the gms process");

    /* Register for TRACK_CHANGES_ONLY flag */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES_ONLY flag failed with rc = 0x%x",rc);

    /* Now kill gms server */
    sleep(2);
    system("killall -9 safplus_gms");
    sleep(10);

    /* Above mentioned dummy node join will not exist after the restart.
     * So rejoin and leave to verify the callback invokation*/
    callback_invoked = CL_FALSE;

    rc = clGmsClusterJoin(handle,                   /* GMS Handle */
                          &clusterManageCallback,   /* Cluster manage callbacks */
                          0,                        /* Credentials */
                          0,                        /* Timeout */
                          dummyNodeId,              /* Node Id */
                          &dummyNodeName);          /* Node Name */

    fail_unless(rc == CL_OK,
            "clGmsClusterJoin failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Track callback is not invoked after killing the gms process");

    callback_invoked = CL_FALSE;
    /* Do node leave */
    rc = clGmsClusterLeave(handle,
                           0,
                           dummyNodeId);
    fail_unless(rc == CL_OK,
            "clGmsClusterLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Track callback is not invoked after killing the gms process");

    printf("Calling track stop...\n");
    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);
    
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(track_stop_after_gms_kill)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Register for cluster track */
    rc = clGmsClusterTrack(handle,CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Now kill gms server */
    sleep(5);
    system("killall -9 safplus_gms");
    sleep(15);

    /* Track stop */
    rc = clGmsClusterTrackStop(handle);
    fail_unless(rc == CL_OK,
            "clGmsClusterTrackStop failed with rc 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN
