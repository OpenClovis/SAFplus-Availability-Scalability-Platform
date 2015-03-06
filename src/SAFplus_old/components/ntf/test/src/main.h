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
                                                                                                                             
#include <saAis.h>
#include <saNtf.h>

extern void simple_init_finalize_cycle(void);
extern void init_with_null_callback(void);
extern void init_with_null_handle(void);
extern void init_with_null_version(void);
//extern void init_version_older_code(void);
//extern void init_version_newer_code(void);
//extern void init_version_lower_major(void);
//extern void init_version_higher_major(void);
//extern void init_version_lower_minor(void);
//extern void init_version_higher_minor(void);
extern void finalize_with_bad_handle(void);
extern void double_finalize(void);
extern void multiple_init_finalize(void);
extern void selection_object_get(void);
extern void dispatch_test(void);

extern void object_create_delete_ntf_allocate_free(void);
extern void attribute_change_ntf_allocate_free(void);
extern void state_change_ntf_allocate_free(void);
extern void alarm_ntf_allocate_free(void);
extern void security_alarm_ntf_allocate_free(void);

extern void object_create_delete_ntf_filter_allocate_free(void);
extern void attribute_change_ntf_filter_allocate_free(void);
extern void state_change_ntf_filter_allocate_free(void);
extern void alarm_ntf_filter_allocate_free(void);
extern void security_alarm_ntf_filter_allocate_free(void);

extern void ptr_full_size(void);
extern void array_full_size(void);

extern void object_create_delete_ntf_send(void);
extern void attribute_change_ntf_send(void);
extern void state_change_ntf_send(void);
extern void alarm_ntf_send(void);

extern void object_create_delete_ntf_send_with_ptr_val(void);
#define START_TEST_EXTERN(__testname)\
void __testname (void)\
{\
  tcase_fn_start (""# __testname, __FILE__, __LINE__);
                                                                                                                             
/* End a unit test */
#define END_TEST_EXTERN }

