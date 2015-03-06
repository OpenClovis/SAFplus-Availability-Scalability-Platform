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
 * saNtfInitialize() 
 * saNtfFinalize()
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
 * 14. selection_object_get
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static SaNtfHandleT   handle = 0;
static SaVersionT  correct_version = {'A',3,1};
static SaNtfCallbacksT_3 null_callbacks = {
    .saNtfNotificationCallback      = NULL,
    .saNtfNotificationDiscardedCallback  = NULL,
    .saNtfStaticSuppressionFilterSetCallback        = NULL
};

static void notificationCallback (SaNtfSubscriptionIdT subscriptionId,
                           const SaNtfNotificationsT *notification)
{
    printf("Callback invoked");
}

static SaNtfCallbacksT_3 nonnull_callbacks = {
    .saNtfNotificationCallback      = notificationCallback,
    .saNtfNotificationDiscardedCallback  = NULL,
    .saNtfStaticSuppressionFilterSetCallback        = NULL
};

START_TEST_EXTERN(simple_init_finalize_cycle)
{
    /*
     * This test tests a simple initialize/finalize cycle.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_values_in_callback)
{
    /*
     * Verify that saNtfInitialize works with callbacks parameter
     * being non NULL, but the internal values of the callbacks being NULL
     * followed by saNtfFinalize
     */
    SaAisErrorT   rc = SA_AIS_OK;
    rc = saNtfInitialize_3(&handle,&null_callbacks,&correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() with NULL callbacks failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(init_with_null_callback)
{
    /*
     * Verify that saNtfInitialize works with callbacks argument
     * being NULL, followed by saNtfFinalize
     */
    SaAisErrorT   rc = SA_AIS_OK;
    rc = saNtfInitialize_3(&handle,NULL,&correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize_3() with NULL callbacks failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize failed with rc 0x%x",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_handle)
{
    /*
     * Verify that saNtfInitialze with handle parameter being NULL
     * fails with error code SA_AIS_ERR_INVALID_PARAM
     */
    SaAisErrorT   rc = SA_AIS_OK;

    rc = saNtfInitialize_3(NULL, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_ERR_INVALID_PARAM,
            "saNtfInialize() with NULL handle returned rc 0x%x, expected SA_AIS_ERR_INVALID_PARAM",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(init_with_null_version)
{
    /*
     * Verify that saNtfInitialze with version parameter being NULL
     * fails with error code SA_AIS_ERR_INVALID_PARAM
     */
    SaAisErrorT   rc = SA_AIS_OK;

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, NULL);
    fail_unless(rc == SA_AIS_ERR_INVALID_PARAM,
            "saNtfInialize() with NULL version returned rc 0x%x, expected SA_AIS_ERR_INVALID_PARAM",rc);
} END_TEST_EXTERN


/* Below test cases are test for the versioning support
 * Currently since only one version is supported, we check the
 * same in the return value. In future this needs to be modified
 * to see that it returns the proper version code being supported.
 */
#if 0 //Version related tests not yet ready
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

#endif

START_TEST_EXTERN(finalize_with_bad_handle)
{
    /*
     * Verify that saNtfFinalize with bad handle fails with
     * return value CL_ERR_INVALID_HANDLE
     */
    SaAisErrorT   rc = SA_AIS_OK;
    rc = saNtfFinalize(0); /* Please note that 0 is an invalid handle */
    fail_unless(rc == SA_AIS_ERR_BAD_HANDLE,
        "saNtfFinalize() with handle 0, returned 0x%x, expected SA_AIS_ERR_BAD_HANDLE",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(double_finalize)
{
    /*
     * Verify that double finalize on the same handle
     * value fails with error code SA_AIS_ERR_BAD_HANDLE
     */
    SaAisErrorT   rc = SA_AIS_OK;
    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_ERR_BAD_HANDLE, 
            "saNtfFinalize on already finalized hanlde returned 0x%x, expected SA_AIS_ERR_BAD_HANDLE",rc);
} END_TEST_EXTERN


START_TEST_EXTERN(multiple_init_finalize)
{
    /*
     * Verify that multiple calls to saNtfInitialize is successful
     * with return code SA_AIS_OK, and following saNtfFinalize on respective
     * handles is also successful.
     */
    SaAisErrorT   rc = SA_AIS_OK;
    ClHandleT   handle2 = 0;
    ClHandleT   handle3 = 0;

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() failed with rc 0x%x",rc);

    rc = saNtfInitialize_3(&handle2, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() second time failed with rc 0x%x",rc);

    rc = saNtfInitialize_3(&handle3, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() third time failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle2);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() second time failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle3);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() third time failed with rc 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(selection_object_get)
{
    /*
     * Verify invoking selectionObjectGet function
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaSelectionObjectT  selectionObj;

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() failed with rc 0x%x",rc);

    rc = saNtfSelectionObjectGet(handle, &selectionObj);
    fail_unless(rc == SA_AIS_OK, "saNtfSelectionObjectGet() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() failed with rc 0x%x",rc);

} END_TEST_EXTERN

START_TEST_EXTERN(dispatch_test)
{
    /*
     * Verify invoking selectionObjectGet function
     */
    SaAisErrorT   rc = SA_AIS_OK;
    SaSelectionObjectT  selectionObj;

    rc = saNtfInitialize_3(&handle, &nonnull_callbacks, &correct_version);
    fail_unless(rc == SA_AIS_OK, "saNtfInitialize() failed with rc 0x%x",rc);

    rc = saNtfSelectionObjectGet(handle, &selectionObj);
    fail_unless(rc == SA_AIS_OK, "saNtfSelectionObjectGet() failed with rc 0x%x",rc);

    rc = saNtfDispatch(handle,SA_DISPATCH_ONE);
    fail_unless(rc == SA_AIS_OK, "saNtfDispatch() failed with rc 0x%x",rc);

    rc = saNtfFinalize(handle);
    fail_unless(rc == SA_AIS_OK, "saNtfFinalize() failed with rc 0x%x",rc);

} END_TEST_EXTERN
