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
 * clGmsClusterMemberGet() 
 * clGmsClusterMemberGetAsync()
 *
 * Following test cases are coded in this file.
 * 
 * 1. member_get_invalid_handle
 * 2. member_get_for_local_node
 * 3. member_get_with_null_param
 * 4. member_get_for_non_local_node_id
 * 5. member_get_async_with_no_callback
 * 6. member_get_async_with_invalid_handle
 * 7. member_get_async_proper_params
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClBoolT     callback_invoked = CL_FALSE;
static ClInvocationT invocationId   = 0;

#if 0
static ClGmsCallbacksT callbacks_all_null = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};
#endif

static void clGmsClusterMemberGetCallbackFunction (
        CL_IN ClInvocationT         invocation,
        CL_IN const ClGmsClusterMemberT *clusterMember,
        CL_IN ClRcT                 rc);

static ClGmsCallbacksT callbacks_non_null = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = clGmsClusterMemberGetCallbackFunction,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};

static void clGmsClusterMemberGetCallbackFunction (
        CL_IN ClInvocationT         invocation,
        CL_IN const ClGmsClusterMemberT *clusterMember,
        CL_IN ClRcT                 rc)
{
    callback_invoked = CL_TRUE;
    printf("Inside cluster track callback function. \n");
}


START_TEST_EXTERN(member_get_invalid_handle)
{
    ClRcT                   rc = CL_OK;
    ClGmsClusterMemberT     clusterMember = {0};

    /* Test with 0 handle value */
    rc = clGmsClusterMemberGet(0,CL_GMS_LOCAL_NODE_ID,0,&clusterMember);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGet was successful even with invalid handle 0 \
            rc = 0x%x",rc);

    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

    /* Now do memberGet with handle value */
    rc = clGmsClusterMemberGet(handle,CL_GMS_LOCAL_NODE_ID,0,&clusterMember);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGet was successful even with invalid handle 0 \
            rc = 0x%x",rc);


    /* Test with a garbage value of handle */
    rc = clGmsClusterMemberGet(1111,CL_GMS_LOCAL_NODE_ID,0,&clusterMember);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGet was successful even with invalid handle 0 \
            rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_for_local_node)
{
    ClRcT                   rc = CL_OK;
    ClGmsClusterMemberT     clusterMember = {0};

    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterMemberGet(handle,CL_GMS_LOCAL_NODE_ID,0,&clusterMember);
    fail_unless(rc == CL_OK,
            "clGmsClusterMemberGet failed for CL_GMS_LOCAL_NODE_ID rc = 0x%x",rc);

    fail_unless(clusterMember.nodeId == clIocLocalAddressGet(),
            "Ioc local address doesnt match with the Local Node info \
            received through clGmsClusterMemberGet");
    printf("Got node detailed for nodeID %d\n",clusterMember.nodeId);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_with_null_param)
{
    ClRcT                   rc = CL_OK;
    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterMemberGet(handle,CL_GMS_LOCAL_NODE_ID,0,NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsClusterMemberGet was successful even with NULL param \
            rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_for_non_local_node_id)
{
    ClRcT                   rc = CL_OK;
    ClGmsClusterMemberT     clusterMember = {0};
    ClIocNodeAddressT       nodeId = 0;

    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    nodeId = clIocLocalAddressGet();
    rc = clGmsClusterMemberGet(handle,nodeId,0,&clusterMember);
    fail_unless(rc == CL_OK,
            "clGmsClusterMemberGet was successful even with invalid handle 0 \
            rc = 0x%x",rc);

    fail_unless(clusterMember.nodeId == nodeId,
            "Ioc local address doesnt match with the Local Node info \
            received through clGmsClusterMemberGet");

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_async_with_no_callback)
{
    ClRcT                   rc = CL_OK;

    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    invocationId = 1111;
    rc = clGmsClusterMemberGetAsync(handle,invocationId,CL_GMS_LOCAL_NODE_ID);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_CALLBACK,
            "clGmsClusterMemberGetAsync was successful even with no \
            callbacks registered. rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_async_with_invalid_handle)
{
    ClRcT                   rc = CL_OK;

    /* Test with 0 handle value */
    rc = clGmsClusterMemberGetAsync(0,1111,CL_GMS_LOCAL_NODE_ID);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGetAsync was successful even with invalid handle 0 \
            rc = 0x%x",rc);

    /* Test with finalized handle value */
    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

    /* Now do memberGet with handle value */
    rc = clGmsClusterMemberGetAsync(handle,1111,CL_GMS_LOCAL_NODE_ID);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGetAsync was successful even with finalized handle \
            rc = 0x%x",rc);


    /* Test with a garbage value of handle */
    rc = clGmsClusterMemberGetAsync(1111,1234,CL_GMS_LOCAL_NODE_ID);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "clGmsClusterMemberGetAsync was successful even with invalid handle 1111 \
            rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(member_get_async_proper_params)
{
    ClRcT                   rc = CL_OK;

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    invocationId = 1234;
    rc = clGmsClusterMemberGetAsync(handle,invocationId,CL_GMS_LOCAL_NODE_ID);
    fail_unless(rc == CL_OK,
            "clGmsClusterMemberGetAsync failed with rc = 0x%x",rc);

    /* Wait for the callback to be invoked */
    sleep(5);

    invocationId = 2222;
    /* Test the same with differrent nodeId */
    rc = clGmsClusterMemberGetAsync(handle, invocationId,clIocLocalAddressGet());
    fail_unless(rc == CL_OK,
            "clGmsClusterMemberGetAsync failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN

