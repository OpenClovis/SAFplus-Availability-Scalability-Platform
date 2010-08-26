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

#ifndef TC_UTILS_H
#define TC_UTILS_H

#include <clOsalApi.h>
#include <clTcUtilsApi.h>

typedef struct ClTcControlT
{
	/* keeps track of whether init
	 * was successfully completed
	 */
	ClBoolT			init_complete;

	/* Exit flag to instruct test app to 
	 * exit from the infinite loop
	 * tied to terminate request from AMS
	 */
	ClOsalMutexIdT  exit_flag_mutex;
	ClBoolT		    exit_flag;

	/* run flag to instruct test app that
	 * HA state has turned active, so that 
	 * the desired test can be started
	 */
	ClOsalMutexIdT  run_test_mutex;
	ClBoolT		    run_test_flag;

	/* If the user is sticking with the (Service Group
	 * Unlock == Run Test) paradigm, then the tc_activate()
	 * will specify the CSI descriptor that will in turn
	 * give the config file name and location. This 
	 * config file will be parsed and the user specified
	 * test routine called with this parameter list
	 */
	ClTcParamListT  param_list;

	/* Test API to run when service group is unlocked. Note
	 * this API will be invoked with the parsed parameter
	 * list; If the CSI descriptor does not contain a config
	 * file, it is assumed that the test case runs without
	 * any configurable parameters
	 */
	int   			(*test_function)(ClTcParamListT *param_list);

	/* Subsystem being registered
	 */
	char			subsystem[TC_MAX_STR_LENGTH];	

	/* test case name being registered
	 */
	char			test_name[TC_MAX_STR_LENGTH];	
} ClTcControlT;

#endif /* TC_UTILS_H */
