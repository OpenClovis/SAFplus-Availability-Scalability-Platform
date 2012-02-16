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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
                                                                                                                             
#include <check.h>
                                                                                                                             
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clEoApi.h>
                                                                                                                             
#include <clTmsApi.h>
#include <clGmsErrors.h>
#include <clHandleApi.h>
#include <clHandleErrors.h>


extern void group_create_invalid_handle(void);
extern void group_create_null_group_name(void);
extern void group_create_null_group_id(void);
extern void group_create_twice_with_same_name(void);
extern void group_create_with_proper_values(void);
extern void group_destroy_with_invalid_handle(void);
extern void group_destroy_with_non_existing_group_id(void);
extern void group_destroy_with_proper_values(void);

extern void group_join_with_invalid_handle(void);
extern void group_join_with_non_existing_group_id(void);
extern void group_join_with_null_member_name(void);
extern void group_join_with_valid_parameters(void);
extern void double_group_join(void);
extern void group_leave_with_invalid_handle(void);
extern void group_leave_with_non_existing_group_id(void);
extern void group_leave_with_non_existing_member_id(void);
extern void group_leave_with_valid_values(void);
extern void group_join_after_destroy(void);

extern void group_track_with_non_existing_group(void);
extern void group_track_with_callback_param_null(void);
extern void group_track_with_callback_value_null(void);
extern void group_track_with_wrong_flag(void);
extern void group_track_with_invalid_params(void);
extern void group_track_with_invalid_handle(void);
extern void group_track_changes_and_changes_only_flag_test(void);
extern void group_track_without_notify_buff(void);
extern void group_track_with_notify_buff(void);
extern void group_track_stop_without_track_registration(void);
extern void group_track_stop_after_track_current(void);
extern void group_track_stop_with_track_changes_flag(void);
extern void group_track_stop_with_track_changes_only_flag(void);
extern void group_double_track_stop(void);
 
extern void group_info_list_get_invalid_handle(void);
extern void group_info_list_get_null_groups_param(void);
extern void group_info_list_get_no_groups(void);
extern void group_info_list_get_valid_values(void);
extern void get_group_info_invalid_handle(void);
extern void get_group_info_null_group_name_param(void);
extern void get_group_info_null_group_info_param(void);
extern void get_group_info_non_existing_group(void);
extern void get_group_info_valid_params(void);

extern void mcast_send_char_array(void);
extern void mcast_send_struct_msg(void);

#define START_TEST_EXTERN(__testname)\
void __testname (void)\
{\
  tcase_fn_start (""# __testname, __FILE__, __LINE__);
                                                                                                                             
/* End a unit test */
#define END_TEST_EXTERN }

