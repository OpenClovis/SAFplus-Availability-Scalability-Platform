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
 * clGmsClusterMemberEject()
 *
 * Following test cases are coded in this file.
 * 
 * 1. eject_with_invalid_handle
 * 2. eject_with_non_existing_node_id
 * 3. eject_with_proper_values
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClBoolT     callback_invoked = CL_FALSE;
static ClNameT     nodeName = {
                .value = "TestNode",
                .length = 8
            };

static void clusterEjectCallbackFunction (CL_IN ClGmsMemberEjectReasonT   reasonCode);

static ClGmsClusterManageCallbacksT clusterManageCallbacks = {
    .clGmsMemberEjectCallback = clusterEjectCallbackFunction
};

static void clusterEjectCallbackFunction (CL_IN ClGmsMemberEjectReasonT   reasonCode)
{
    callback_invoked = CL_TRUE;
    printf("Inside eject callback function with eject reason %d\n", reasonCode);
}


START_TEST_EXTERN(eject_with_invalid_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterMemberEject(handle, 100, CL_GMS_MEMBER_EJECT_REASON_API_REQUEST);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

} END_TEST_EXTERN



START_TEST_EXTERN(eject_with_non_existing_node_id)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterMemberEject(handle, 100, CL_GMS_MEMBER_EJECT_REASON_API_REQUEST);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER,
            "ClusterJoin with NULL clusterManage callback failed with rc = 0x%x",rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN


START_TEST_EXTERN(eject_with_proper_values)
{
    ClRcT       rc = CL_OK;

    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);

    rc = clGmsClusterJoin(handle, &clusterManageCallbacks, 0, 0, 100, &nodeName);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterJoin with 0 handle failed with rc = 0x%x",rc);

    callback_invoked = CL_FALSE;

    rc = clGmsClusterMemberEject(handle, 100, CL_GMS_MEMBER_EJECT_REASON_API_REQUEST);
    fail_unless(rc == CL_OK,
            "Cluster Leave for nodeId 100 failed with rc 0x%x",rc);

    /* Wait for the callback to be invoked */
    sleep(5);
    fail_unless(callback_invoked == CL_TRUE,
            "Cluster Eject callback is not invoked");
    
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
 
} END_TEST_EXTERN

