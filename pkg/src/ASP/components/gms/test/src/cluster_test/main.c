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

#include "main.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

static ClRcT   app_main(ClUint32T argc, ClCharT *argv[]);
static ClRcT   dummy_finalize();
static ClRcT   dummy_state_change(ClEoStateT eoState);
static ClRcT   dummy_health_check(ClEoSchedFeedBackT* schFeedback);


ClEoConfigT clEoConfig =
{
    "test_main",          /* EO Name*/
    1,                          /* Thread Priority */
    1,                          /* No of EO thread */
    0x20,                     /* ReqIocPort */
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


static ClRcT dummy_finalize(void) 
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


static void run_all_test()
{
    int n_failures = 0;

    Suite *s = suite_create("GMS Client API");

    /* Test Cases for clGmsInitialize and clGmsFinalize APIs */
    TCase *tc_init_and_finalize = tcase_create("Initialize/Finalize");
    suite_add_tcase(s, tc_init_and_finalize);

    tcase_add_test(tc_init_and_finalize, simple_init_finalize_cycle);
    tcase_add_test(tc_init_and_finalize, init_with_null_callback);
    tcase_add_test(tc_init_and_finalize, init_with_null_handle);
    tcase_add_test(tc_init_and_finalize, init_with_null_version);
    tcase_add_test(tc_init_and_finalize, init_version_older_code);
    tcase_add_test(tc_init_and_finalize, init_version_newer_code);
    tcase_add_test(tc_init_and_finalize, init_version_lower_major);
    tcase_add_test(tc_init_and_finalize, init_version_higher_major);
    tcase_add_test(tc_init_and_finalize, init_version_lower_minor);
    tcase_add_test(tc_init_and_finalize, init_version_higher_minor);
    tcase_add_test(tc_init_and_finalize, finalize_with_bad_handle);
    tcase_add_test(tc_init_and_finalize, double_finalize);
    tcase_add_test(tc_init_and_finalize, multiple_init_finalize);

    /* Test Cases for clGmsClusterMemberGet and clGmsClusterMemberGetAsync APIs */
    TCase *tc_cluster_member_get = tcase_create("MemberGet Sync and Async");
    suite_add_tcase(s, tc_cluster_member_get);

    tcase_add_test(tc_cluster_member_get, member_get_invalid_handle);
    tcase_add_test(tc_cluster_member_get, member_get_for_local_node);
    tcase_add_test(tc_cluster_member_get, member_get_with_null_param);
    tcase_add_test(tc_cluster_member_get, member_get_for_non_local_node_id);
    tcase_add_test(tc_cluster_member_get, member_get_async_with_no_callback);
    tcase_add_test(tc_cluster_member_get, member_get_async_with_invalid_handle);
    tcase_add_test(tc_cluster_member_get, member_get_async_proper_params);

    
    /* Test Cases for clGmsClusterTrack and clGmsClusterTrackStop APIs */
    TCase *tc_track_and_trackstop = tcase_create("Cluster Track and Track Stop");
    suite_add_tcase(s, tc_track_and_trackstop);
    tcase_add_test(tc_track_and_trackstop, track_with_callback_param_null);
    tcase_add_test(tc_track_and_trackstop, track_with_null_callback_values);
    tcase_add_test(tc_track_and_trackstop, track_with_wrong_flag);
    tcase_add_test(tc_track_and_trackstop, track_with_invalid_params);
    tcase_add_test(tc_track_and_trackstop, track_with_invalid_handle);
    tcase_add_test(tc_track_and_trackstop, track_changes_and_changes_only_flag_test);
    tcase_add_test(tc_track_and_trackstop, track_without_notify_buff);
    tcase_add_test(tc_track_and_trackstop, track_with_notify_buff);
    tcase_add_test(tc_track_and_trackstop, track_stop_without_track_registration);
    tcase_add_test(tc_track_and_trackstop, track_stop_after_track_current);
    tcase_add_test(tc_track_and_trackstop, track_stop_with_track_changes_flag);
    tcase_add_test(tc_track_and_trackstop, track_stop_with_track_changes_only_flag);
    tcase_add_test(tc_track_and_trackstop, double_track_stop);
//    tcase_add_test(tc_track_and_trackstop, verify_track_checkpointing);
//    tcase_add_test(tc_track_and_trackstop, track_stop_after_gms_kill);


    
    /* Test Cases for clGmsClusterJoin, clGmsClusterJoinAsync,
     * clGmsClusterLeave and clGmsClusterLeaveAsync APIs.
     */
    TCase *tc_join_and_leave = tcase_create("ClusterJoin and ClusterLeave - Sync and Async");
    suite_add_tcase(s, tc_join_and_leave);

    tcase_add_test(tc_join_and_leave, join_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, join_with_null_manage_callbacks);
    tcase_add_test(tc_join_and_leave, join_with_manage_callbackvalues_null);
    tcase_add_test(tc_join_and_leave, join_with_null_node_name);
    tcase_add_test(tc_join_and_leave, join_with_proper_values);
    tcase_add_test(tc_join_and_leave, joinasync_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, joinasync_with_null_manage_callbacks);
    tcase_add_test(tc_join_and_leave, joinasync_with_manage_callbackvalues_null);
    tcase_add_test(tc_join_and_leave, joinasync_with_null_node_name);
    tcase_add_test(tc_join_and_leave, joinasync_with_proper_values);
    tcase_add_test(tc_join_and_leave, leave_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, leave_with_invalid_node_id);
    tcase_add_test(tc_join_and_leave, leave_with_proper_values);
    tcase_add_test(tc_join_and_leave, leaveasync_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, leaveasync_with_invalid_node_id);
    tcase_add_test(tc_join_and_leave, leaveasync_with_proper_values);



    /* Test Cases for clGmsClusterLeaderElect API */
    TCase *tc_cluster_leader_elect = tcase_create("Cluster Leader Elect Request");
    suite_add_tcase(s, tc_cluster_leader_elect);

    tcase_add_test(tc_cluster_leader_elect, leader_elect_with_invalid_handle);
    tcase_add_test(tc_cluster_leader_elect, leader_elect_with_proper_handle);

    /* Test Cases for clGmsClusterMemberEject API */
    TCase *tc_cluster_member_eject = tcase_create("Cluster Member Eject Request");
    suite_add_tcase(s, tc_cluster_member_eject);

    tcase_add_test(tc_cluster_member_eject, eject_with_invalid_handle);
    tcase_add_test(tc_cluster_member_eject, eject_with_non_existing_node_id);
    tcase_add_test(tc_cluster_member_eject, eject_with_proper_values);

    /* Now run the tests */
    SRunner *sr = srunner_create(s);
    /* do not fork; forking seg-faults EO (why?) */
    srunner_set_fork_status(sr, CK_NOFORK);

    srunner_run_all(sr, CK_NORMAL); /* Can also use CK_VERBOSE */

    srunner_set_log(sr, "./gms_unit_test_logs.log");

    n_failures = srunner_ntests_failed(sr);

    srunner_free(sr);
}

static void tc_add(TCase *tc,char *tcase_name)
{
    if (!strcmp(tcase_name,"simple_init_finalize_cycle"))
    {
        tcase_add_test(tc, simple_init_finalize_cycle);
        return;
    }
    if (!strcmp(tcase_name,"init_with_null_callback"))
    {
        tcase_add_test(tc, init_with_null_callback);
        return;
    }
    if (!strcmp(tcase_name,"init_with_null_handle"))
    {
        tcase_add_test(tc, init_with_null_handle);
        return;
    }
    if (!strcmp(tcase_name,"init_with_null_version"))
    {
        tcase_add_test(tc, init_with_null_version);
        return;
    }
    if (!strcmp(tcase_name,"init_version_older_code"))
    {
        tcase_add_test(tc, init_version_older_code);
        return;
    }
    if (!strcmp(tcase_name,"init_version_newer_code"))
    {
        tcase_add_test(tc, init_version_newer_code);
        return;
    }
    if (!strcmp(tcase_name,"init_version_lower_major"))
    {
        tcase_add_test(tc, init_version_lower_major);
        return;
    }
    if (!strcmp(tcase_name,"init_version_higher_major"))
    {
        tcase_add_test(tc, init_version_higher_major);
        return;
    }
    if (!strcmp(tcase_name,"init_version_lower_minor"))
    {
        tcase_add_test(tc, init_version_lower_minor);
        return;
    }
    if (!strcmp(tcase_name,"init_version_higher_minor"))
    {
        tcase_add_test(tc, init_version_higher_minor);
        return;
    }
    if (!strcmp(tcase_name,"finalize_with_bad_handle"))
    {
        tcase_add_test(tc, finalize_with_bad_handle);
        return;
    }
    if (!strcmp(tcase_name,"double_finalize"))
    {
        tcase_add_test(tc, double_finalize);
        return;
    }
    if (!strcmp(tcase_name,"multiple_init_finalize"))
    {
        tcase_add_test(tc, multiple_init_finalize);
        return;
    }

    if (!strcmp(tcase_name,"member_get_invalid_handle"))
    {
        tcase_add_test(tc, member_get_invalid_handle);
        return;
    }
    if (!strcmp(tcase_name,"member_get_for_local_node"))
    {
        tcase_add_test(tc, member_get_for_local_node);
        return;
    }
    if (!strcmp(tcase_name,"member_get_with_null_param"))
    {
        tcase_add_test(tc, member_get_with_null_param);
        return;
    }
    if (!strcmp(tcase_name,"member_get_for_non_local_node_id"))
    {
        tcase_add_test(tc, member_get_for_non_local_node_id);
        return;
    }
    if (!strcmp(tcase_name,"member_get_async_with_no_callback"))
    {
        tcase_add_test(tc, member_get_async_with_no_callback);
        return;
    }
    if (!strcmp(tcase_name,"member_get_async_with_invalid_handle"))
    {
        tcase_add_test(tc, member_get_async_with_invalid_handle);
        return;
    }
    if (!strcmp(tcase_name,"member_get_async_proper_params"))
    {
        tcase_add_test(tc, member_get_async_proper_params);
        return;
    }
    
    if (!strcmp(tcase_name,"verify_track_checkpointing"))
    {
        tcase_add_test(tc, verify_track_checkpointing);
        return;
    }
    if (!strcmp(tcase_name,"track_stop_after_gms_kill"))
    {
        tcase_add_test(tc, track_stop_after_gms_kill);
        return;
    }
#if 0
    tcase_add_test(tc_track_and_trackstop, track_with_callback_param_null);
    tcase_add_test(tc_track_and_trackstop, track_with_null_callback_values);
    tcase_add_test(tc_track_and_trackstop, track_with_wrong_flag);
    tcase_add_test(tc_track_and_trackstop, track_with_invalid_params);
    tcase_add_test(tc_track_and_trackstop, track_with_invalid_handle);
    tcase_add_test(tc_track_and_trackstop, track_changes_and_changes_only_flag_test);
    tcase_add_test(tc_track_and_trackstop, track_without_notify_buff);
    tcase_add_test(tc_track_and_trackstop, track_with_notify_buff);
    tcase_add_test(tc_track_and_trackstop, track_stop_without_track_registration);
    tcase_add_test(tc_track_and_trackstop, track_stop_after_track_current);
    tcase_add_test(tc_track_and_trackstop, track_stop_with_track_changes_flag);
    tcase_add_test(tc_track_and_trackstop, track_stop_with_track_changes_only_flag);
    tcase_add_test(tc_track_and_trackstop, double_track_stop);


    
    tcase_add_test(tc_join_and_leave, join_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, join_with_null_manage_callbacks);
    tcase_add_test(tc_join_and_leave, join_with_manage_callbackvalues_null);
    tcase_add_test(tc_join_and_leave, join_with_null_node_name);
    tcase_add_test(tc_join_and_leave, join_with_proper_values);
    tcase_add_test(tc_join_and_leave, joinasync_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, joinasync_with_null_manage_callbacks);
    tcase_add_test(tc_join_and_leave, joinasync_with_manage_callbackvalues_null);
    tcase_add_test(tc_join_and_leave, joinasync_with_null_node_name);
    tcase_add_test(tc_join_and_leave, joinasync_with_proper_values);
    tcase_add_test(tc_join_and_leave, leave_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, leave_with_invalid_node_id);
    tcase_add_test(tc_join_and_leave, leave_with_proper_values);
    tcase_add_test(tc_join_and_leave, leaveasync_with_invalid_handle);
    tcase_add_test(tc_join_and_leave, leaveasync_with_invalid_node_id);
    tcase_add_test(tc_join_and_leave, leaveasync_with_proper_values);




    tcase_add_test(tc_cluster_leader_elect, leader_elect_with_invalid_handle);
    tcase_add_test(tc_cluster_leader_elect, leader_elect_with_proper_handle);

    tcase_add_test(tc_cluster_member_eject, eject_with_invalid_handle);
    tcase_add_test(tc_cluster_member_eject, eject_with_non_existing_node_id);
    tcase_add_test(tc_cluster_member_eject, eject_with_proper_values);

#endif
}

static void run_from_file(char *filename)
{
    FILE *tc_file = NULL;
    int n_failures = 0;
    char tc[100] = "";
    int num_tcs = 0;
    int len=0;

    Suite *s = suite_create("GMS Client API");


    /* Test Cases for clGmsInitialize and clGmsFinalize APIs */
    TCase *sel_tcs = tcase_create("Selected Test");
    suite_add_tcase(s, sel_tcs);

    tc_file = fopen(filename, "r");
    if (tc_file == NULL)
    {
        perror("Open Failed:");
        exit(-1);
    }

    while (1)
    {
        if (fgets (tc, 100,tc_file) != NULL)
        {
            if (tc[0] != '#')
            {
                num_tcs++;
                len=strlen(tc);
                tc[len-1] = '\0';
                tc_add(sel_tcs,tc);
            }
        } else {
            break;
        }
    }

    if (num_tcs == 0)
    {
        printf("NO Test cases are specified in the given file\n");
        exit(-1);
    } else {
        printf("%d test cases are provided\n",num_tcs);
    }


    /* Now run the tests */
    SRunner *sr = srunner_create(s);
    /* do not fork; forking seg-faults EO (why?) */
    srunner_set_fork_status(sr, CK_NOFORK);

    srunner_run_all(sr, CK_NORMAL); /* Can also use CK_VERBOSE */

    srunner_set_log(sr, "./gms_unit_test_logs.log");

    n_failures = srunner_ntests_failed(sr);

    srunner_free(sr);
}

ClRcT
app_main(ClUint32T argc, ClCharT **argv)
{
    if (argc == 2)
    {
        run_from_file(argv[1]);
    } else {
        run_all_test();
    }
    return CL_OK;
}
