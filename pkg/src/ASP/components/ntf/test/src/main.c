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
    return CL_ERR_NOT_IMPLEMENTED;
}


static void run_all_test()
{
    int n_failures = 0;

    Suite *s = suite_create("NTF Client API");

    /* Test Cases for saNtfInitialize_3 and saNtfFinalize APIs */
    TCase *tc_init_and_finalize = tcase_create("Initialize/Finalize");
    suite_add_tcase(s, tc_init_and_finalize);

    tcase_add_test(tc_init_and_finalize, simple_init_finalize_cycle);
    tcase_add_test(tc_init_and_finalize, init_with_null_callback);
    tcase_add_test(tc_init_and_finalize, init_with_null_handle);
    tcase_add_test(tc_init_and_finalize, init_with_null_version);
#if 0
    tcase_add_test(tc_init_and_finalize, init_version_older_code);
    tcase_add_test(tc_init_and_finalize, init_version_newer_code);
    tcase_add_test(tc_init_and_finalize, init_version_lower_major);
    tcase_add_test(tc_init_and_finalize, init_version_higher_major);
    tcase_add_test(tc_init_and_finalize, init_version_lower_minor);
    tcase_add_test(tc_init_and_finalize, init_version_higher_minor);
#endif
    tcase_add_test(tc_init_and_finalize, finalize_with_bad_handle);
    tcase_add_test(tc_init_and_finalize, double_finalize);
    tcase_add_test(tc_init_and_finalize, multiple_init_finalize);
    tcase_add_test(tc_init_and_finalize, selection_object_get);
    tcase_add_test(tc_init_and_finalize, dispatch_test);


    /* Test cases for saNtf<type>NotificationAllocate() and saNtfNotificationFree() APIs */
    TCase *ntf_allocate_free = tcase_create("NotificationAllocate/Free");
    suite_add_tcase(s, ntf_allocate_free);

    tcase_add_test(ntf_allocate_free, object_create_delete_ntf_allocate_free);
    tcase_add_test(ntf_allocate_free, attribute_change_ntf_allocate_free);
    tcase_add_test(ntf_allocate_free, state_change_ntf_allocate_free);
    tcase_add_test(ntf_allocate_free, alarm_ntf_allocate_free);
    //tcase_add_test(ntf_allocate_free, security_alarm_ntf_allocate_free);

    /* Test cases for saNtf<type>NotificationFilterAllocate() and saNtfNotificationFilterFree() APIs */
    TCase *ntf_filter_allocate_free = tcase_create("NotificationFilterAllocate/Free");
    suite_add_tcase(s, ntf_filter_allocate_free);

    tcase_add_test(ntf_filter_allocate_free, object_create_delete_ntf_filter_allocate_free);
    tcase_add_test(ntf_filter_allocate_free, attribute_change_ntf_filter_allocate_free);
    tcase_add_test(ntf_filter_allocate_free, state_change_ntf_filter_allocate_free);
    tcase_add_test(ntf_filter_allocate_free, alarm_ntf_filter_allocate_free);
    tcase_add_test(ntf_filter_allocate_free, security_alarm_ntf_filter_allocate_free);

    /* Test cases for saNtfArrayValAllocate() and saNtfPtrValAllocate() APIs */
    TCase *obj_create_delete_ptr_val = tcase_create("PtrValGetWithObjCreatDel");
    suite_add_tcase(s, obj_create_delete_ptr_val);

    tcase_add_test(obj_create_delete_ptr_val, ptr_full_size);
    tcase_add_test(obj_create_delete_ptr_val, array_full_size);

    TCase *ntf_send = tcase_create("NotificationSend");
    suite_add_tcase(s, ntf_send);

    tcase_add_test(ntf_send, object_create_delete_ntf_send);
    tcase_add_test(ntf_send, attribute_change_ntf_send);
    tcase_add_test(ntf_send, state_change_ntf_send);
    tcase_add_test(ntf_send, alarm_ntf_send);
    tcase_add_test(ntf_send, object_create_delete_ntf_send_with_ptr_val);

    /* Now run the tests */
    SRunner *sr = srunner_create(s);
    /* do not fork; forking seg-faults EO (why?) */
    srunner_set_fork_status(sr, CK_NOFORK);

    srunner_run_all(sr, CK_NORMAL); /* Can also use CK_VERBOSE */

    srunner_set_log(sr, "./ntf_unit_tests.log");

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
#if 0
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
#endif
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
    if (!strcmp(tcase_name,"selection_object_get"))
    {
        tcase_add_test(tc, selection_object_get);
        return;
    }

    if (!strcmp(tcase_name,"dispatch_test"))
    {
        tcase_add_test(tc, dispatch_test);
        return;
    }

    if (!strcmp(tcase_name,"object_create_delete_ntf_allocate_free"))
    {
        tcase_add_test(tc, object_create_delete_ntf_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"attribute_change_ntf_allocate_free"))
    {
        tcase_add_test(tc, attribute_change_ntf_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"state_change_ntf_allocate_free"))
    {
        tcase_add_test(tc, state_change_ntf_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"alarm_ntf_allocate_free"))
    {
        tcase_add_test(tc, alarm_ntf_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"security_alarm_ntf_allocate_free"))
    {
        tcase_add_test(tc, security_alarm_ntf_allocate_free);
        return;
    }

    if (!strcmp(tcase_name,"object_create_delete_ntf_send"))
    {
        tcase_add_test(tc, object_create_delete_ntf_send);
        return;
    }
    if (!strcmp(tcase_name,"attribute_change_ntf_send"))
    {
        tcase_add_test(tc, attribute_change_ntf_send);
        return;
    }
    if (!strcmp(tcase_name,"state_change_ntf_send"))
    {
        tcase_add_test(tc, state_change_ntf_send);
        return;
    }
    if (!strcmp(tcase_name,"alarm_ntf_send"))
    {
        tcase_add_test(tc, alarm_ntf_send);
        return;
    }
    if (!strcmp(tcase_name,"object_create_delete_ntf_send_with_ptr_val"))
    {
        tcase_add_test(tc, object_create_delete_ntf_send_with_ptr_val);
        return;
    }
    if (!strcmp(tcase_name,"object_create_delete_ntf_filter_allocate_free"))
    {
        tcase_add_test(tc, object_create_delete_ntf_filter_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"attribute_change_ntf_filter_allocate_free"))
    {
        tcase_add_test(tc, attribute_change_ntf_filter_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"alarm_ntf_filter_allocate_free"))
    {
        tcase_add_test(tc, alarm_ntf_filter_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"security_alarm_ntf_filter_allocate_free"))
    {
        tcase_add_test(tc, security_alarm_ntf_filter_allocate_free);
        return;
    }
    if (!strcmp(tcase_name,"ptr_full_size"))
    {
        tcase_add_test(tc, ptr_full_size);
        return;
    }
    if (!strcmp(tcase_name,"array_full_size"))
    {
        tcase_add_test(tc, array_full_size);
        return;
    }

}

static void run_from_file(char *filename)
{
    FILE *tc_file = NULL;
    int n_failures = 0;
    char tc[100] = "";
    int num_tcs = 0;
    int len=0;

    Suite *s = suite_create("NTF Client API");


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
