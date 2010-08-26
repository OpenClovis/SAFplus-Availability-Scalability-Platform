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
                                                                                                                             
#include <clGmsErrors.h>
#include <clTmsApi.h>
#include <clHandleApi.h>
#include <clHandleErrors.h>

extern void simple_init_finalize_cycle(void);
extern void init_with_null_callback(void);
extern void init_with_null_handle(void);
extern void init_with_null_version(void);
extern void init_version_older_code(void);
extern void init_version_newer_code(void);
extern void init_version_lower_major(void);
extern void init_version_higher_major(void);
extern void init_version_lower_minor(void);
extern void init_version_higher_minor(void);
extern void finalize_with_bad_handle(void);
extern void double_finalize(void);
extern void multiple_init_finalize(void);


extern void member_get_invalid_handle(void);
extern void member_get_for_local_node(void);
extern void member_get_with_null_param(void);
extern void member_get_for_non_local_node_id(void);
extern void member_get_async_with_no_callback(void);
extern void member_get_async_with_invalid_handle(void);
extern void member_get_async_proper_params(void);

extern void track_with_callback_param_null(void);
extern void track_with_null_callback_values(void);
extern void track_with_wrong_flag(void);
extern void track_with_invalid_params(void);
extern void track_with_invalid_handle(void);
extern void track_changes_and_changes_only_flag_test(void);
extern void track_without_notify_buff(void);
extern void track_with_notify_buff(void);
extern void track_stop_without_track_registration(void);
extern void track_stop_after_track_current(void);
extern void track_stop_with_track_changes_flag(void);
extern void track_stop_with_track_changes_only_flag(void);
extern void double_track_stop(void);
extern void verify_track_checkpointing(void);
extern void track_stop_after_gms_kill(void);

extern void join_with_invalid_handle(void);
extern void join_with_null_manage_callbacks(void);
extern void join_with_manage_callbackvalues_null(void);
extern void join_with_null_node_name(void);
extern void join_with_proper_values(void);
extern void joinasync_with_invalid_handle(void);
extern void joinasync_with_null_manage_callbacks(void);
extern void joinasync_with_manage_callbackvalues_null(void);
extern void joinasync_with_null_node_name(void);
extern void joinasync_with_proper_values(void);
extern void leave_with_invalid_handle(void);
extern void leave_with_invalid_node_id(void);
extern void leave_with_proper_values(void);
extern void leaveasync_with_invalid_handle(void);
extern void leaveasync_with_invalid_node_id(void);
extern void leaveasync_with_proper_values(void);

extern void leader_elect_with_invalid_handle(void);
extern void leader_elect_with_proper_handle(void);


extern void eject_with_invalid_handle(void);
extern void eject_with_non_existing_node_id(void);
extern void eject_with_proper_values(void);

 
#define START_TEST_EXTERN(__testname)\
void __testname (void)\
{\
  tcase_fn_start (""# __testname, __FILE__, __LINE__);
                                                                                                                             
/* End a unit test */
#define END_TEST_EXTERN }

