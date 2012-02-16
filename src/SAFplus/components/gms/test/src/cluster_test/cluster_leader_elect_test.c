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
 * clGmsClusterLeaderElect() 
 *
 * Following test cases are coded in this file.
 * 
 * 1. leader_elect_with_invalid_handle
 * 2. leader_elect_with_proper_handle
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};


START_TEST_EXTERN(leader_elect_with_invalid_handle)
{
#if 0
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Call cluster join without gms initialize */
    rc = clGmsClusterLeaderElect(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
            "ClusterLeaderElect with 0 handle failed with rc = 0x%x",rc);
#endif
} END_TEST_EXTERN


START_TEST_EXTERN(leader_elect_with_proper_handle)
{
    ClRcT       rc = CL_OK;

    handle = 0;
    /* Initialize gms client without any callbacks being registered. */
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with NULL callbacks failed with rc 0x%x",rc);
#if 0
    /* Call cluster join without gms initialize */
    rc = clGmsClusterLeaderElect(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_OK,
            "ClusterLeaderElect with proper handle failed with rc = 0x%x",rc);
#endif
    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);

} END_TEST_EXTERN

