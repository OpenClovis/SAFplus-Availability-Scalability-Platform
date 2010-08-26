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
 * clGmsClusterJoin() 
 * clGmsClusterJoinAsync()
 *
 * Following test cases are coded in this file.
 * 
 * 1. group_join_with_invalid_handle
 * 2. group_join_with_non_existing_group_id
 * 3. group_join_with_null_member_name
 * 4. group_join_with_valid_parameters
 * 5. double_group_join
 * 6. group_leave_with_invalid_handle
 * 7. group_leave_with_non_existing_group_id
 * 8. group_leave_with_non_existing_member_id
 * 9. group_leave_with_valid_values
 * 10. group_join_after_destroy
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClGmsGroupNameT groupName1 = {
                .value = "group1",
                .length = 6
            };
static ClGmsGroupIdT    groupId = -1;
static ClGmsMemberNameT memberName1 = {
    .value = "member1",
    .length = 7
};
static ClGmsMemberIdT   memberId = 1234;
static ClGmsMemberNameT memberName2 = {
    .value = "member2",
    .length = 7
};
static ClGmsMemberIdT   memberId2 = 4444;



START_TEST_EXTERN(group_join_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsGroupJoin(handle, 111, 1234, &memberName1, 0,NULL, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "GroupJoin with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_join_with_non_existing_group_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupJoin(handle, 111, 1234, &memberName1, 0,NULL, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST,
            "clGmsGroupJoin with non-existing groupId is successful. rc = 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_join_with_null_member_name)
{
    /* Verify that NULL member name is accepted */
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join group with NULL memberName */
    rc = clGmsGroupJoin(handle,groupId, memberId, NULL, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin with NULL member name failed with rc 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_join_with_valid_parameters)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    rc = clGmsGroupJoin(handle,groupId, memberId, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin failed with rc 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);
    
    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(double_group_join)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    rc = clGmsGroupJoin(handle,groupId, memberId, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin failed with rc 0x%x\n",rc);

    /* Try joining the same node again */
    rc = clGmsGroupJoin(handle,groupId, memberId, &memberName1, 0,NULL,0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST,
            "Duplicate Group join is successful 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);
    
    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_leave_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "GroupJoin with 0 handle is successful with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_leave_with_non_existing_group_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Assuming that group id 10 doesnt exist, perform leave
     * on that group id. */
    rc = clGmsGroupLeave(handle, 10, memberId, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_GMS_ERR_GROUP_DOESNT_EXIST,
            "GroupLeave with non-existing groupID failed with rc 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_leave_with_non_existing_member_id)
{
    /* In this test case, create a group and without
     * joining any member do a leave. It should fail */

    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST,
            "clGmsGroupLeave with non-existing member failed with rc 0x%x\n",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_leave_with_valid_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join the group */
    rc = clGmsGroupJoin(handle,groupId, memberId, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin failed with rc 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_join_after_destroy)
{
    /* With a node still in the group, mark the group
     * for delete by calling groupDestroy. Then try to join again.
     * this should be denied */
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join the group */
    rc = clGmsGroupJoin(handle,groupId, memberId, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin failed with rc 0x%x\n",rc);

    /* Mark group for delete by calling destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Now try to join a new member to the group. With latest code this fail
     * due to failure in IOC multicast register*/
    rc = clGmsGroupJoin(handle,groupId, memberId2, &memberName2, 0,NULL, 0);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST,
            "GroupJoin after group destroy failed with rc 0x%x\n",rc);

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId, memberId, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN
