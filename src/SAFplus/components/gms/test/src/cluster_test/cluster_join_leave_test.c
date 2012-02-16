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
 * 1. join_with_invalid_handle
 * 2. join_with_null_manage_callbacks
 * 3. join_with_manage_callbackvalues_null
 * 4. join_with_null_node_name
 * 5. join_with_proper_values
 * 6. joinasync_with_invalid_handle
 * 7. joinasync_with_null_manage_callbacks
 * 8. joinasync_with_manage_callbackvalues_null
 * 9. joinasync_with_null_node_name
 * 10. joinasync_with_proper_values
 * 11. leave_with_invalid_handle
 * 12. leave_with_invalid_node_id
 * 13. leave_with_proper_values
 * 14. leaveasync_with_invalid_handle
 * 15. leaveasync_with_invalid_node_id
 * 16. leaveasync_with_proper_values
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClNameT     nodeName = {
                .value = "TestNode",
                .length = 8
            };

static ClGmsClusterManageCallbacksT clusterManageCallbacksNull = {
    .clGmsMemberEjectCallback = NULL
};


static void clusterEjectCallbackFunction (CL_IN ClGmsMemberEjectReasonT   reasonCode);

static ClGmsClusterManageCallbacksT clusterManageCallbacks = {
    .clGmsMemberEjectCallback = clusterEjectCallbackFunction
};

static void clusterEjectCallbackFunction (CL_IN ClGmsMemberEjectReasonT   reasonCode)
{
    printf("Inside eject callback function\n");
}


START_TEST_EXTERN(join_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(join_with_null_manage_callbacks)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoin(handle, NULL, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with NULL clusterManage callback failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(join_with_manage_callbackvalues_null)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoin(handle, &clusterManageCallbacksNull, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with clusterManage callback function being null, failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(join_with_null_node_name)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(join_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    rc = clGmsClusterLeave(handle, 0, 100);
    fail_unless(rc == CL_OK,
            "Cluster Leave for nodeId 100 failed with rc 0x%x",rc);
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(joinasync_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterJoinAsync(handle, &clusterManageCallbacks, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(joinasync_with_null_manage_callbacks)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoinAsync(handle, NULL, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with NULL clusterManage callback failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(joinasync_with_manage_callbackvalues_null)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoinAsync(handle, &clusterManageCallbacksNull, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with clusterManage callback function being null, failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(joinasync_with_null_node_name)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoinAsync(handle, &clusterManageCallbacks, 0, 100, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(joinasync_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoinAsync(handle, &clusterManageCallbacks, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    /* Sleep for 5 seconds for the join request to complete */
    sleep(5);

    rc = clGmsClusterLeave(handle, 0, 100);
    fail_unless(rc == CL_OK,
            "Cluster Leave for nodeId 100 failed with rc 0x%x",rc);
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(leave_with_invalid_handle)
{
    ClRcT   rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterLeave(handle, 0, 100);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterLeave with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(leave_with_invalid_node_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Call cluster leave without clusterjoin */
    rc = clGmsClusterLeave(handle, 0, 100);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "ClusterLeave with non-existing nodeid failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN



START_TEST_EXTERN(leave_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Join with 100 as node ID */
    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    /* Perform leave */
    rc = clGmsClusterLeave(handle, 0, 100);
    fail_unless(rc == CL_OK,
            "Cluster Leave for nodeId 100 failed with rc 0x%x",rc);
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(leaveasync_with_invalid_handle)
{
    ClRcT   rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterLeaveAsync(handle, 100);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterLeave with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(leaveasync_with_invalid_node_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Call cluster leave without clusterjoin */
    rc = clGmsClusterLeaveAsync(handle, 100);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "ClusterLeave with non-existing nodeid failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(leaveasync_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    /* Join with 100 as node ID */
    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    /* Perform leave */
    rc = clGmsClusterLeaveAsync(handle, 100);
    fail_unless(rc == CL_OK,
            "Cluster Leave for nodeId 100 failed with rc 0x%x",rc);
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN

