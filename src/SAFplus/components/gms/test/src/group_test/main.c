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
#include "main.h"

static ClRcT   app_main(ClUint32T argc, ClCharT *argv[]);
static ClRcT   dummy_finalize();
static ClRcT   dummy_state_change(ClEoStateT eoState);
static ClRcT   dummy_health_check(ClEoSchedFeedBackT* schFeedback);


ClEoConfigT clEoConfig =
{
    "test_main",          /* EO Name*/
    1,                          /* Thread Priority */
    1,                          /* No of EO thread */
    0x21,                     /* ReqIocPort */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_APP,   /* test application needs to run main thread */
    app_main,
    dummy_finalize,
    dummy_state_change,
    dummy_health_check,
};
                                                                                                                             
/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,             /* osal */
    CL_TRUE,             /* timer */
    CL_TRUE,             /* buffer */
    CL_TRUE,             /* ioc */
    CL_TRUE,             /* rmd */
    CL_TRUE,             /* eo */
    CL_FALSE,            /* om */
    CL_FALSE,            /* hal */
    CL_FALSE,            /* dbal */
};
                                                                                                                             
ClUint8T clEoClientLibs[] = {
    CL_FALSE,            /* cor */
    CL_FALSE,            /* cm */
    CL_FALSE,            /* name */
    CL_FALSE,            /* log */
    CL_FALSE,            /* trace */
    CL_FALSE,            /* diag */
    CL_FALSE,            /* txn */
    CL_FALSE,            /* hpi */
    CL_FALSE,            /* cli */
    CL_FALSE,            /* alarm */
    CL_FALSE,            /* debug */
    CL_FALSE             /* gms */
};


static ClRcT dummy_finalize() 
{
    return CL_OK; 
}
                                                                                                                             
static ClRcT dummy_state_change(ClEoStateT eoState) 
{
    return CL_OK; 
}
                                                                                                                             
static ClRcT dummy_health_check(ClEoSchedFeedBackT* schFeedback)
{
    return CL_GMS_RC(CL_ERR_NOT_IMPLEMENTED);
}


ClRcT
app_main(ClUint32T argc, ClCharT **argv)
{
    int n_failures = 0;

    Suite *s = suite_create("GMS Client API");

#if 0
    /* Test Cases for clGmsInitialize and clGmsFinalize APIs */
    TCase *tc_group_create_destroy = tcase_create("Group Create and Group Destroy");
    suite_add_tcase(s, tc_group_create_destroy);

    tcase_add_test(tc_group_create_destroy, group_create_invalid_handle);
    tcase_add_test(tc_group_create_destroy, group_create_null_group_name);
    tcase_add_test(tc_group_create_destroy, group_create_null_group_id);
    tcase_add_test(tc_group_create_destroy, group_create_twice_with_same_name);
    tcase_add_test(tc_group_create_destroy, group_create_with_proper_values);
    tcase_add_test(tc_group_create_destroy, group_destroy_with_invalid_handle);
    tcase_add_test(tc_group_create_destroy, group_destroy_with_non_existing_group_id);
    tcase_add_test(tc_group_create_destroy, group_destroy_with_proper_values);

    TCase *tc_group_join_leave = tcase_create("Group Join and Group Leave");
    suite_add_tcase(s, tc_group_join_leave);

    tcase_add_test(tc_group_join_leave, group_join_with_invalid_handle);
    tcase_add_test(tc_group_join_leave, group_join_with_non_existing_group_id);
    tcase_add_test(tc_group_join_leave, group_join_with_null_member_name);
    tcase_add_test(tc_group_join_leave, group_join_with_valid_parameters);
    tcase_add_test(tc_group_join_leave, double_group_join);
    tcase_add_test(tc_group_join_leave, group_leave_with_invalid_handle);
    tcase_add_test(tc_group_join_leave, group_leave_with_non_existing_group_id);
    tcase_add_test(tc_group_join_leave, group_leave_with_non_existing_member_id);
    tcase_add_test(tc_group_join_leave, group_leave_with_valid_values);
    tcase_add_test(tc_group_join_leave, group_join_after_destroy);

    TCase *tc_group_track = tcase_create("Group Track and Track stop");
    suite_add_tcase(s, tc_group_track);

    tcase_add_test(tc_group_track, group_track_with_non_existing_group);
    tcase_add_test(tc_group_track, group_track_with_callback_param_null);
    tcase_add_test(tc_group_track, group_track_with_callback_value_null);
    tcase_add_test(tc_group_track, group_track_with_wrong_flag);
    tcase_add_test(tc_group_track, group_track_with_invalid_params);
    tcase_add_test(tc_group_track, group_track_with_invalid_handle);
    tcase_add_test(tc_group_track, group_track_changes_and_changes_only_flag_test);
    tcase_add_test(tc_group_track, group_track_without_notify_buff);
    tcase_add_test(tc_group_track, group_track_with_notify_buff);
    tcase_add_test(tc_group_track, group_track_stop_without_track_registration);
    tcase_add_test(tc_group_track, group_track_stop_after_track_current);
    tcase_add_test(tc_group_track, group_track_stop_with_track_changes_flag);
    tcase_add_test(tc_group_track, group_track_stop_with_track_changes_only_flag);
    tcase_add_test(tc_group_track, group_double_track_stop);

    TCase *tc_group_info = tcase_create("GroupInfoListGet and GetGroupInfo");
    suite_add_tcase(s, tc_group_info);

    tcase_add_test(tc_group_info, group_info_list_get_invalid_handle);
    tcase_add_test(tc_group_info, group_info_list_get_null_groups_param);
    tcase_add_test(tc_group_info, group_info_list_get_no_groups);
    tcase_add_test(tc_group_info, group_info_list_get_valid_values);
    tcase_add_test(tc_group_info, get_group_info_invalid_handle);
    tcase_add_test(tc_group_info, get_group_info_null_group_name_param);
    tcase_add_test(tc_group_info, get_group_info_null_group_info_param);
    tcase_add_test(tc_group_info, get_group_info_non_existing_group);
    tcase_add_test(tc_group_info, get_group_info_valid_params);
#endif
    TCase *tc_mcast = tcase_create("Group Mcast send");
    suite_add_tcase(s, tc_mcast);

    tcase_add_test(tc_mcast, mcast_send_char_array);
    tcase_add_test(tc_mcast, mcast_send_struct_msg);

    /* Now run the tests */
    SRunner *sr = srunner_create(s);
    /* do not fork; forking seg-faults EO (why?) */
    srunner_set_fork_status(sr, CK_NOFORK);

    srunner_run_all(sr, CK_NORMAL); /* Can also use CK_VERBOSE */

    srunner_set_log(sr, "./gms_unit_test_logs.log");

    n_failures = srunner_ntests_failed(sr);

    srunner_free(sr);
    
    return CL_OK;
}
