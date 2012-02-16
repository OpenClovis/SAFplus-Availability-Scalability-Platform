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
 * clGmsGroupCreate() 
 * clGmsGroupDestroy()
 *
 * Following test cases are coded in this file.
 * 
 * 1. group_create_invalid_handle
 * 2. group_create_null_group_name
 * 3. group_create_null_group_id
 * 4. group_create_twice_with_same_name
 * 5. group_create_with_proper_values
 * 6. group_destroy_with_invalid_handle
 * 7. group_destroy_with_non_existing_group_id
 * 8. group_destroy_with_proper_values
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClGmsGroupNameT     group1_name = {
    .value = "group1",
    .length = 6
};
static ClGmsGroupIdT groupId;


START_TEST_EXTERN(group_create_invalid_handle)
{
    ClRcT       rc = CL_OK;

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, &groupId);

    fail_unless (CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsGroupCreate with invalid handle failed with rc 0x%x\n", rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_create_null_group_name)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupCreate(handle, NULL, NULL, NULL, &groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsGroupCreate with NULL group name failed with rc = 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_create_null_group_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsGroupCreate with NULL groupId failed with rc 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_create_twice_with_same_name)
{
    ClRcT       rc = CL_OK;
    ClGmsGroupIdT groupId2 = 0;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc = 0x%x\n",rc);

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, &groupId2);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST,
            "clGmsGroupCreate failed with rc = 0x%x\n",rc);

    /* Destroy the created group */
    rc = clGmsGroupDestroy(handle, groupId); 
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy failed with rc = 0x%x\n",rc);
    
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_create_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc = 0x%x\n",rc);

    /* Destroy the created group */
    rc = clGmsGroupDestroy(handle, groupId); 
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy failed with rc = 0x%x\n",rc);
    
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_destroy_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "GroupCreate with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(group_destroy_with_non_existing_group_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    groupId = 22;
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST,
            "GroupCreate with non existing groupId failed with rc = 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(group_destroy_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsGroupCreate(handle, &group1_name, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc = 0x%x\n",rc);

    /* Destroy the created group */
    rc = clGmsGroupDestroy(handle, groupId); 
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy failed with rc = 0x%x\n",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN
