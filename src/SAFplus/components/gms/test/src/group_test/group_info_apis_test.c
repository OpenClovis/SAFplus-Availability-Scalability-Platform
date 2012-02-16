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
 * clGmsClusterJoin() 
 * clGmsClusterJoinAsync()
 *
 * Following test cases are coded in this file.
 * 
 * 1. group_info_list_get_invalid_handle
 * 2. group_info_list_get_null_groups_param
 * 3. group_info_list_get_no_groups
 * 4. group_info_list_get_valid_values
 * 5. get_group_ingo_invalid_handle
 * 6. get_group_info_null_group_name_param
 * 7. get_group_info_null_group_info_param
 * 8. get_group_info_non_existing_group
 * 9. get_group_info_valid_params
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
static ClGmsGroupNameT groupName2 = {
                .value = "group2",
                .length = 6
};
static ClGmsGroupNameT groupName3 = {
                .value = "group3",
                .length = 6
};

static ClGmsGroupIdT    groupId1 = -1;
static ClGmsGroupIdT    groupId2 = -1;
static ClGmsGroupIdT    groupId3 = -1;
static ClGmsMemberNameT memberName1 = {
    .value = "member1",
    .length = 7
};
static ClGmsMemberIdT   memberId1 = 1234;
static ClGmsMemberNameT memberName2 = {
    .value = "member2",
    .length = 7
};
static ClGmsMemberIdT   memberId2 = 4444;



START_TEST_EXTERN(group_info_list_get_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsGroupsInfoListGet(handle, 0, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsGroupsInfoListGet with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_info_list_get_null_groups_param)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

#if 0
    rc = clGmsGroupsInfoListGet(handle, 0, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsGroupsInfoListGet with NULL groups param failed with rc = 0x%x\n",rc);
#endif
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_info_list_get_no_groups)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupInfoListT     groups = {0};

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupsInfoListGet(handle, 0, &groups);
    fail_unless(rc == CL_OK,
            "clGmsGroupsInfoListGet failed when there are no groups on the node rc = 0x%x\n",rc);

    fail_unless(groups.groupInfoList == NULL,
            "clGmsGroupsInfoListGet returned non-null groupInfoList pointer "
            "even when there are no groups on the node");

    fail_unless(groups.noOfGroups == 0,
            "clGmsGroupsInfoListGet returned noOfGroups = %d, even when there are no "
            "groups on the node");

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_info_list_get_valid_values)
{
    ClRcT                   rc = CL_OK;
    ClGmsGroupInfoListT     groups = {0};
    ClUint32T               i = 0;
    ClUint32T               old_no_of_groups = 0;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create 3 groups and verify that groupInfoListGet returns proper data */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId1);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate for groupName1 failed with rc 0x%x\n",rc);

    rc = clGmsGroupCreate(handle, &groupName2, NULL, NULL, &groupId2);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate for groupName2 failed with rc 0x%x\n",rc);

    rc = clGmsGroupCreate(handle, &groupName3, NULL, NULL, &groupId3);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate for groupName3 failed with rc 0x%x\n",rc);


    rc = clGmsGroupJoin(handle,groupId1, memberId1, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId1 failed with rc 0x%x\n",rc);

    rc = clGmsGroupJoin(handle,groupId2, memberId2, &memberName2, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId2 failed with rc 0x%x\n",rc);

    /* Now verify that groupInfoList get returns proper values */
    rc = clGmsGroupsInfoListGet(handle, 0 , &groups);
    fail_unless(rc == CL_OK,
            "clGmsGroupsInfoListGet failed with rc 0x%x\n",rc);

    fail_unless(groups.noOfGroups >= 3,
            "clGmsGroupsInfoListGet incorrectly returned number of groups = %d "
            "instead of 3 groups created here \n",groups.noOfGroups);

    fail_unless(groups.groupInfoList != NULL,
            "clGmsGroupsInfoListGet returned NULL groupsInfoPointer");

    /* Save the current number of groups values to verify after delete */
    old_no_of_groups = groups.noOfGroups;

    /* Now verify if groupInfoList contains all the 3 groups information */
    for (i = 0; i < old_no_of_groups; i++)
    {
        if (groups.groupInfoList[i].groupId == groupId1)
        {
                fail_unless(groups.groupInfoList[i].noOfMembers == 1,
                        "Values returned by groupInfoList are not proper");
                fail_unless(groups.groupInfoList[i].setForDelete == CL_FALSE,
                        "Values returned by groupInfoList are not proper");
        }
        else if (groups.groupInfoList[i].groupId == groupId2)
        {
                fail_unless(groups.groupInfoList[i].noOfMembers == 1,
                        "Values returned by groupInfoList are not proper");
                fail_unless(groups.groupInfoList[i].setForDelete == CL_FALSE,
                        "Values returned by groupInfoList are not proper");
        }
        else if (groups.groupInfoList[i].groupId == groupId3)
        {
                fail_unless(groups.groupInfoList[i].noOfMembers == 0,
                        "Values returned by groupInfoList are not proper");
                fail_unless(groups.groupInfoList[i].setForDelete == CL_FALSE,
                        "Values returned by groupInfoList are not proper");
        } 
        else 
        {
            fail_unless(1 == 2,
                    "Found a Group ID different from the one created in the script");
        }
    }

    /* Now leave member1 from group1 and destroy group1. But for group2,
     * dont leave the member but issue destroy. Dont do anything for
     * group3. Verify that the group parameters are changed accordingly
     */

    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId1, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave for memberId1 failed with rc 0x%x\n",rc);
    
    /* Destroy the group 1 & 2 */
    rc = clGmsGroupDestroy(handle, groupId1);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId1,rc);

    rc  = clGmsGroupDestroy(handle, groupId2);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId2,rc);

    rc = clGmsGroupsInfoListGet(handle, 0 , &groups);
    fail_unless(rc == CL_OK,
            "clGmsGroupsInfoListGet failed with rc 0x%x\n",rc);

    fail_unless(groups.noOfGroups == (old_no_of_groups - 1),
            "clGmsGroupsInfoListGet incorrectly returned number of groups = %d "
            "instead of 2 groups created here \n",groups.noOfGroups);

    fail_unless(groups.groupInfoList != NULL,
            "clGmsGroupsInfoListGet returned NULL groupsInfoPointer");

    /* Now verify if groupInfoList contains all the 3 groups information */
    for (i = 0; i < groups.noOfGroups; i++)
    {
        if (groups.groupInfoList[i].groupId == groupId1)
        {
                fail_unless(1 == 2,
                        "Got the groupId of the group which was already destroyed!");
        }
        else if (groups.groupInfoList[i].groupId == groupId2)
        {
                fail_unless(groups.groupInfoList[i].noOfMembers == 1,
                        "Values returned by groupInfoList are not proper");
                fail_unless(groups.groupInfoList[i].setForDelete == CL_TRUE,
                        "Values returned by groupInfoList are not proper");
        }
        else if (groups.groupInfoList[i].groupId == groupId3)
        {
                fail_unless(groups.groupInfoList[i].noOfMembers == 0,
                        "Values returned by groupInfoList are not proper");
                fail_unless(groups.groupInfoList[i].setForDelete == CL_FALSE,
                        "Values returned by groupInfoList are not proper");
        } 
        else 
        {
            fail_unless(1 == 2,
                    "Found a Group ID different from the one created in the script");
        }
    }

    /* Leave other members and delete the remaining groups */
    rc = clGmsGroupLeave(handle, groupId2, memberId2, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave for memberId2 failed with rc 0x%x\n",rc);

    /* Destroy the group 1 & 2 */
    rc = clGmsGroupDestroy(handle, groupId2);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId1,rc);

    rc = clGmsGroupDestroy(handle, groupId3);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId1,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(get_group_info_invalid_handle)
{
    ClRcT               rc = CL_OK;
    ClGmsGroupInfoT     groupInfo = {{0}};

    rc = clGmsGetGroupInfo(handle, &groupName1,0, &groupInfo);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsGetGroupInfo with 0 handle failed with rc 0x%x\n",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(get_group_info_null_group_name_param)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupInfoT     groupInfo = {{0}};

    rc = clGmsGetGroupInfo(handle, NULL, 0, &groupInfo);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsGetGroupInfo with NULL groupName failed with rc 0x%x\n",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(get_group_info_null_group_info_param)
{
    ClRcT       rc = CL_OK;

    rc = clGmsGetGroupInfo(handle, &groupName1, 0, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsGetGroupInfo with NULL groupInfo param failed with rc 0x%x\n",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(get_group_info_non_existing_group)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupInfoT     groupInfo = {{0}};
    ClGmsGroupNameT     dummyName = {
        .value = "asdfdfsdfdsfdffds",
        .length = 17
    };
   

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Issue groupInfo on a non-existing group */
    rc = clGmsGetGroupInfo(handle, &dummyName, 0, &groupInfo);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST,
            "clGmsGetGroupInfo with non-existing groupId failed with rc 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(get_group_info_valid_params)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupInfoT     groupInfo = {{0}};

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Create a group first before join */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId1);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Join the group */
    rc = clGmsGroupJoin(handle,groupId1, memberId1, &memberName1, 0,NULL,0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin failed with rc 0x%x\n",rc);

    /* Get the information of the group and see if it is ok */
    rc = clGmsGetGroupInfo(handle, &groupName1, 0, &groupInfo);
    fail_unless(rc == CL_OK,
            "clGmsGetGroupInfo with valid params failed with rc 0x%x\n",rc);

    /* Now verify the values */
    fail_unless(strncmp(groupInfo.groupName.value, groupName1.value, groupName1.length) == 0,
            "GroupName obtained in groupInfo is not proper");
    fail_unless(groupInfo.groupId == groupId1,
            "GroupId obtained in groupInfo does not match with groupId of the group");
    fail_unless(groupInfo.noOfMembers == 1,
            "NoOfMembers in the obtained groupInfo is %d",groupInfo.noOfMembers);
    fail_unless(groupInfo.setForDelete == CL_FALSE,
            "The group with ID %d obtained in groupInfo is incorrectly set for delete",groupInfo.setForDelete);
       
    /* Leave from the group */
    rc = clGmsGroupLeave(handle, groupId1, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x\n",rc);

    /* Destroy the group */
    rc = clGmsGroupDestroy(handle, groupId1);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId1,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN
