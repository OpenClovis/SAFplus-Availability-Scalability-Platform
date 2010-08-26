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
 * clGmsGroupTrack() 
 * clGmsGroupTrackStop()
 *
 * Following test cases are coded in this file.
 * 
 * 1. group_track_with_non_existing_group
 * 2. group_track_with_callback_param_null
 * 3. group_track_with_null_callback_values
 * 4. group_track_with_wrong_flag
 * 5. group_track_with_invalid_params
 * 6. group_track_with_invalid_handle
 * 7. group_track_changes_and_changes_only_flag_test
 * 8. group_track_without_notify_buff
 * 9. group_track_with_notify_buff
 * 10. group_track_stop_without_track_registration
 * 11. group_track_stop_after_track_current
 * 12. group_track_stop_with_track_changes_flag
 * 13. group_track_stop_with_track_changes_only_flag
 * 14. group_double_track_stop
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClBoolT     callback_invoked = CL_FALSE;
static ClGmsGroupNameT  groupName1 = {
    .value = "group1",
    .length = 6
};
static  ClGmsGroupIdT  groupId = 0;
static  ClGmsMemberNameT memberName1 = {
    .value = "member1",
    .length = 7
};
static ClGmsMemberIdT   memberId1 = 1234;

static void clGmsGroupTrackCallbackFunction (
        CL_IN ClGmsGroupIdT         groupId,
        CL_IN const ClGmsGroupNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc);

static ClGmsCallbacksT callbacks_non_null = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = clGmsGroupTrackCallbackFunction,
    .clGmsGroupMemberGetCallback    = NULL
};

static void clGmsGroupTrackCallbackFunction (
        CL_IN ClGmsGroupIdT         groupId,
        CL_IN const ClGmsGroupNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc)
{
    callback_invoked = CL_TRUE;
    printf("Inside group track callback function. \n");
}


START_TEST_EXTERN(group_track_with_non_existing_group)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client with proper values */
    rc = clGmsInitialize(&handle, &callbacks_non_null, &correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with propre values failed with rc 0x%x\n",rc);

    /* Invoke group track on group id 10, assuming that group 10 doesnt exist */
    rc = clGmsGroupTrack(handle, 10, CL_GMS_TRACK_CHANGES, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsGroupTrack is successful on a non-existing group id 10. rc = 0x%x\n",rc);

    /* Finalize gms client */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_callback_param_null)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* 
     * Try cluster track with track_changes or track_changes_only flag. It
     * should fail with rc CL_ERR_NO_CALLBACK 
     */
    rc = clGmsGroupTrack(handle, groupId,CL_GMS_TRACK_CHANGES, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no callbacks are registered. rc = 0x%x",rc);

    /* Try same test with CL_GMS_TRACK_CHANGES_ONLY flag */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CHANGES_ONLY was successful even \
            when no callbacks are registered. rc = 0x%x",rc);

    /*
     * Try same test with CL_GMS_TRACK_CURRENT flag, but notificationBuffer
     * parameter is NULL, so callback should be provided in this case.
     */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CURRENT, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CURRENT was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_callback_value_null)
{
    /* This test case same as above one, except that instead of
     * passing the NULL parameter, we pass a non-null callbacks
     * parameter, but it doesnt have the function pointer properly
     * asigned. So the cluster track should fail here as well.
     */
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* 
     * Try cluster track with track_changes or track_changes_only flag. It
     * should fail with rc CL_ERR_NO_CALLBACK 
     */
    rc = clGmsGroupTrack(handle, groupId,CL_GMS_TRACK_CHANGES, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CHANGES was successful even \
            when no callbacks are registered. rc = 0x%x",rc);

    /* Try same test with CL_GMS_TRACK_CHANGES_ONLY flag */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CHANGES_ONLY was successful even \
            when no callbacks are registered. rc = 0x%x",rc);

    /*
     * Try same test with CL_GMS_TRACK_CURRENT flag, but notificationBuffer
     * parameter is NULL, so callback should be provided in this case.
     */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CURRENT, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsGroupTrack with CL_GMS_TRACK_CURRENT was successful even \
            when no trackflags are registered. rc = 0x%x",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_wrong_flag)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Try with track flag = 0. Note that the track flag verification is
     * done before verifying if the group exists. So ideally below test
     * should be ok */
    rc = clGmsGroupTrack(handle,1, 0, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "clGmsGroupTrack wrongly accepted 0 track flag value. rc = 0x%x",rc);

    /* Try with track flag other than 1/2/4 values */
    rc = clGmsGroupTrack(handle,1, 112, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_FLAG,
            "clGmsGroupTrack wrongly accepted invalid track flag value. rc = 0x%x",rc);

    /* Try with combination of CL_GMS_TRACK_CHANGES_ONLY 
     * and CL_GMS_TRACK_CHANGES flags */
    rc = clGmsGroupTrack(handle,1,  CL_GMS_TRACK_CHANGES|CL_GMS_TRACK_CHANGES_ONLY, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "clGmsGroupTrack wrongly accepted combination of CL_GMS_TRACK_CHANGES \
            & CL_GMS_TRACK_CHANGES_ONLY flag. rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_invalid_params)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupNotificationBufferT notifyBuff = {0};

    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsGroupNotificationT));

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /*
     * Here notification buffer is not null, but number of items is 0
     * so it should return CL_ERR_INVALID_PARAM
     */
    rc = clGmsGroupTrack(handle,1111, CL_GMS_TRACK_CURRENT, &notifyBuff);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "clGmsGroupTrack wrongly accepted invalid parameters. rc = 0x%x",rc);
    
    clHeapFree(notifyBuff.notification);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    rc = clGmsGroupTrack(0,1111, CL_GMS_TRACK_CURRENT,NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsGroupTrack is successful even with invalid handle. rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_track_changes_and_changes_only_flag_test)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    callback_invoked = CL_FALSE;

    /* Register for cluster track */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Group track callback is not invoked after groupjoin");

    callback_invoked = CL_FALSE;

    /* Register for TRACK_CHANGES_ONLY flag */
    rc = clGmsGroupTrack(handle, groupId, CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES_ONLY flag failed with rc = 0x%x",rc);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is invoked. */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
                        "Group track callback is not invoked after group leave");

    /* Track stop */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrackStop failed with rc 0x%x",rc);
    
    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_without_notify_buff)
{
    /*
     * In this test case, we pass the notificationBuffer
     * parameter but dont provide inner buffer for 
     * returning the cluster information. GMS will allocate
     * the buffer and returns. Verify whether the node information
     * is proper
     */

    ClRcT       rc = CL_OK;
    ClGmsGroupNotificationBufferT notifyBuff = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join a member to the group. */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);


    /* Register for cluster track */
    rc = clGmsGroupTrack(handle, groupId, CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    /* Verify that gms has allocated the notification buffer pointer and
     * the number of members parameter also */
    fail_unless(((notifyBuff.notification != NULL) && (notifyBuff.numberOfItems > 0)),
            "GMS has not allocated the notification buffer and not updated \
            numberOfItems value. NoOfItems=%d",notifyBuff.numberOfItems);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

     /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);


    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_with_notify_buff)
{
    /*
     * In this test case, we pass the notificationBuffer
     * along with inner notification buffer as well as
     * number of Items.
     */

    ClRcT       rc = CL_OK;
    ClGmsGroupNotificationBufferT notifyBuff = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join a member to the group. */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);


    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsGroupNotificationT));
    notifyBuff.numberOfItems = 1;

    /* cluster track */
    rc = clGmsGroupTrack(handle, groupId, CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }


    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_stop_without_track_registration)
{
    ClRcT       rc = CL_OK;

    /* 
     * This test case would verify the clGmsGroupTrack CLI
     * without invoking a prior clGmsGroupTrack. The return value should
     * be CL_ERR_DOESNT_EXIST
     */

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    rc = clGmsGroupTrackStop(handle,groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsGroupTrackStop was successful even without a prior \
            clGmsGroupTrack. rc = 0x%x",rc);

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_stop_after_track_current)
{
    /*
     * This test case would verify the clGmsGroupTrackStop CLI
     * by first invoking a clGmsGroupTrack with CL_GMS_TRACK_CURRENT
     * flag, and then invoking track stop. This verifies that TRACK_CURRENT
     * does not add a node to track node list. rc should be CL_ERR_DOESNT_EXIST
     */

    ClRcT       rc = CL_OK;
    ClGmsGroupNotificationBufferT notifyBuff = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join a member to the group. */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);


    notifyBuff.notification = clHeapAllocate(sizeof(ClGmsGroupNotificationT));
    if (notifyBuff.notification == NULL)
    {
        fail_unless(0,"Could not allocate memory");
        return;
    }
    notifyBuff.numberOfItems = 1;

    /* cluster track */
    rc = clGmsGroupTrack(handle, groupId, CL_GMS_TRACK_CURRENT,&notifyBuff);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CURRENT flag failed with rc = 0x%x",rc);

    if (notifyBuff.notification != NULL)
    {
        clHeapFree(notifyBuff.notification);
    }

    /* invoke track stop. */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsGroupTrackStop was successful even without a prior \
            clGmsGroupTrack. rc = 0x%x",rc);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_track_stop_with_track_changes_flag)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    callback_invoked = CL_FALSE;

    /* Register for cluster track */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Group track callback is not invoked after groupjoin");

    callback_invoked = CL_FALSE;

    /* Track stop */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrackStop failed with rc 0x%x",rc);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is not invoked. */
    sleep(5);
    fail_unless(callback_invoked == CL_FALSE,
                        "Group track callback is invoked even after group track stop");

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_track_stop_with_track_changes_only_flag)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    callback_invoked = CL_FALSE;

    /* Register for cluster track */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES_ONLY flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Group track callback is not invoked after groupjoin");

    callback_invoked = CL_FALSE;

    /* Track stop */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrackStop failed with rc 0x%x",rc);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is not invoked. */
    sleep(5);
    fail_unless(callback_invoked == CL_FALSE,
                        "Group track callback is invoked even after group track stop");

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(group_double_track_stop)
{
    ClRcT       rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    callback_invoked = CL_FALSE;

    /* Register for cluster track */
    rc = clGmsGroupTrack(handle,groupId, CL_GMS_TRACK_CHANGES,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES flag failed with rc = 0x%x",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,NULL, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    /* Wait and see if the callback is called */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Group track callback is not invoked after groupjoin");

    callback_invoked = CL_FALSE;

    /* Register for TRACK_CHANGES_ONLY flag */
    rc = clGmsGroupTrack(handle, groupId, CL_GMS_TRACK_CHANGES_ONLY,NULL);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrack with TRACK_CHANGES_ONLY flag failed with rc = 0x%x",rc);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Wait and see if the callback is invoked. */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
                        "Group track callback is not invoked after group leave");

    /* Track stop */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupTrackStop failed with rc 0x%x",rc);

    /* Invoke track stop again */
    rc = clGmsGroupTrackStop(handle, groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "double clGmsGroupTrackStop was successful with rc 0x%x",rc);

    
    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN

