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
 * clGmsInitialize() 
 * clGmsFinalize()
 *
 * Following test cases are coded in this file.
 * 
 * 1. simple_init_finalize_cycle
 * 2. init_with_null_callback
 * 3. init_with_null_handle
 * 4. init_with_null_version
 * 5. init_version_older_code
 * 6. init_version_newer_code
 * 7. init_version_lower_major
 * 8. init_version_higher_major
 * 9. init_version_lower_minor
 * 10. init_version_higher_minor
 * 11. finalize_with_bad_handle
 * 12. double_finalize
 * 13. multiple_init_finalize
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClGmsCallbacksT callbacks = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};


START_TEST_EXTERN(simple_init_finalize_cycle)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    ClRcT   rc = CL_OK;
    rc = clGmsInitialize(&handle, &callbacks, &correct_version);
    fail_unless(rc == CL_OK, "clGmsInitialize() failed with rc 0x%x",rc);

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK, "clGmsFinalize failed");
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_callback)
{
    /*
     * Verify that clGmsInitialize works with callbacks argument
     * being NULL, followed by clGmsFinalize
     */

    ClRcT   rc = CL_OK;
    rc = clGmsInitialize(&handle,NULL,&correct_version);
    fail_unless(rc == CL_OK, "clGmsInitialize() with NULL callbacks failed");

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK, "clGmsFinalize failed");
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_handle)
{
    /*
     * Verify that clGmsInitialize with handle parameter being NULL
     * fails with error code CL_ERR_NULL_POINTER
     */
    ClRcT   rc = CL_OK;

    rc = clGmsInitialize(NULL, &callbacks, &correct_version);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER,
            "clGmsInitialize() with NULL handle failed with rc 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_version)
{
    /*
     * Verify that clGmsInitialize with version parameter being NULL
     * fails with error code CL_ERR_NULL_POINTER
     */
    ClRcT   rc = CL_OK;

    rc = clGmsInitialize(&handle, &callbacks, NULL);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER, 
            "clGmsInitialize() with NULL version failed with rc 0x%x",rc);
} END_TEST_EXTERN


/* Below test cases are test for the versioning support
 * Currently since only one version is supported, we check the
 * same in the return value. In future this needs to be modified
 * to see that it returns the proper version code being supported.
 */
START_TEST_EXTERN(init_version_older_code)
{
    /*
     * Verify that clGmsInitialize with the version value lower
     * than the currently supported version fails with error code
     * CL_ERR_VERSION_MISMATCH and returns the actual version being
     * supported.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'A', 1, 1 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH,
            "clGmsInitialize() wrongly accepted old version code");

    fail_unless(version.releaseCode == 'B' &&
                version.majorVersion == 1 &&
                version.minorVersion == 1);
} END_TEST_EXTERN

START_TEST_EXTERN(init_version_newer_code)
{
    /*
     * Verify that clGmsInitialize with the version value lower
     * than the currently supported version fails with error code
     * CL_ERR_VERSION_MISMATCH and returns the actual version being
     * supported.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'C', 1, 1 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH,
            "clGmsInitialize() wrongly accepted newer version code");

    fail_unless(version.releaseCode == 'B' &&
                version.majorVersion == 1 &&
                version.minorVersion == 1);
} END_TEST_EXTERN


START_TEST_EXTERN(init_version_lower_major)
{
    /*
     * Verify that clGmsInitialize with the lower value of major version 
     * than the currently supported version succeeds and returns CL_OK
     * However the version parameter is changed to the currently supported
     * version value.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'B', 0, 1 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize() wrongly rejected lower major number");

    fail_unless(version.releaseCode == 'B' &&
                version.majorVersion == 1 &&
                version.minorVersion == 1);

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK);
} END_TEST_EXTERN


START_TEST_EXTERN(init_version_higher_major)
{
    /*
     * Verify that clGmsInitialize with the higher value of major version
     * than the currently supported version fails with error code
     * CL_ERR_VERSION_MISMATCH and returns the actual version being
     * supported.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'B', 123, 1 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH,
            "clGmsInitialize() wrongly accepted higher major number");

    fail_unless(version.releaseCode == 'B' &&
            version.majorVersion == 1 &&
            version.minorVersion == 1);
} END_TEST_EXTERN


START_TEST_EXTERN(init_version_lower_minor)
{
    /*
     * Verify that clGmsInitialize with the lower value of minor version
     * than the currently supported version succeeds with return value 
     * CL_OK and returns the actual version being supported.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'B', 1, 0 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize() wrongly rejected lower minor number");

    fail_unless(version.releaseCode == 'B' &&
                version.majorVersion == 1 &&
                version.minorVersion == 1);

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK);
} END_TEST_EXTERN


START_TEST_EXTERN(init_version_higher_minor)
{
    /*
     * Verify that clGmsInitialize with the higher value of minor version
     * than the currently supported version succeeds with return value 
     * CL_OK and returns the actual version being supported.
     */
    ClRcT   rc = CL_OK;
    ClVersionT version = { 'B', 1, 122 };
    rc = clGmsInitialize(&handle, &callbacks, &version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize() wrongly rejected higher minor number");

    fail_unless(version.releaseCode == 'B' &&
                version.majorVersion == 1 &&
                version.minorVersion == 1);
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK);
} END_TEST_EXTERN


START_TEST_EXTERN(finalize_with_bad_handle)
{
    /*
     * Verify that clGmsFinalize with bad handle fails with
     * return value CL_ERR_INVALID_HANDLE
     */
    ClRcT   rc = CL_OK;
    rc = clGmsFinalize(0); /* Please note that 0 is an invalid handle */
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE,
        "clGmsFinalize() was successful with invalid handle value");
} END_TEST_EXTERN


START_TEST_EXTERN(double_finalize)
{
    /*
     * Verify that double finalize on the same handle
     * value fails with error code CL_ERR_INVALID_HANDLE
     */
    ClRcT   rc = CL_OK;
    rc = clGmsInitialize(&handle, &callbacks, &correct_version);
    fail_unless(rc == CL_OK, "clGmsInitialize() failed");

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK, "clGmsFinalize failed");

    rc = clGmsFinalize(handle);
    fail_unless(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE, 
            "clGmsFinalize was succesful on already finalized hanlde");
} END_TEST_EXTERN


START_TEST_EXTERN(multiple_init_finalize)
{
    /*
     * Verify that multiple calls to clGmsInitialize is successful
     * with return code CL_OK, and following clGmsFinalizes on respective
     * handles is also successful.
     */
    ClRcT       rc = CL_OK;
    ClHandleT   handle2 = 0;
    ClHandleT   handle3 = 0;

    rc = clGmsInitialize(&handle, &callbacks, &correct_version);
    fail_unless(rc == CL_OK, "clGmsInitialize() failed");

    rc = clGmsInitialize(&handle2, &callbacks, &correct_version);
    fail_unless(rc == CL_OK, "Multiple clGmsInitialize() failed");

    rc = clGmsInitialize(&handle3, &callbacks, &correct_version);
    fail_unless(rc == CL_OK, "Multiple clGmsInitialize() failed");

    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK, "clGmsFinalize failed for handle1 with rc 0x%x",rc);

    rc = clGmsFinalize(handle2);
    fail_unless(rc == CL_OK, "clGmsFinalize failed for handle2 with rc 0x%x",rc);

    rc = clGmsFinalize(handle3);
    fail_unless(rc == CL_OK, "clGmsFinalize failed for handle3 with rc 0x%x",rc);
} END_TEST_EXTERN
