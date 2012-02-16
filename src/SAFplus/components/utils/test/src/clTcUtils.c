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
/*******************************************************************************
File       : tc_utils.c

Description: Provides APIs to start and stop tests from within
			 a test application

*******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clTcUtils.h>
#include <clTestApi.h>

static ClTcControlT tc_control;

/*******************************************************************************
Function   : clTcInitialize

Description: Initializes internal data structures so that a test can be started
             or stopped based on the locking and unlocking of a Service Group.
			 The assumption here is that the "THREAD_FOR_APP" option was chosen
			 during the modeling of this application. 

			 The user is expected to call clTcRun() at the end of initialization
			 phase in clCompAppInitialize() so that a forever loop is started.
			 
			 When the state change to active is received, the user calls
			 tc_activate() which runs the test function registered with this
			 function.

			 At the end of the run clTcDeactivate() is invoked to stop the run.

Input      :  1. Subsystem name
			  2. Test case name
			  3. function pointer to api that runs the test case.

Returns    : CL_OK if all succeeds
             CL_ERR_INVALID_PARAMETER if function pointer is NULL
			 error code returned from clOsalMutexCreate()

*******************************************************************************/
int clTcInitialize ( 
		const ClCharT	*subsystem,
		const ClCharT	*test_name,
		int 	(*func_to_register)(ClTcParamListT *param_list) )
{
	ClRcT ret_code = CL_OK;

	/* Initialize the control structure associated with this
	 * test case. This can be generecized further by having
	 * multiple invokes of clTcInitialize() made each returning
	 * a dynamically allocated control structure
	 */
	memset(&tc_control, 0, sizeof(ClTcControlT));

	/* Set to indicate we are not initialized
	 */
	tc_control.init_complete = CL_FALSE;

	/* Initialize the paramter list 
	 */
	tc_control.param_list.num_params = 0;
	tc_control.param_list.params     = NULL;

	if (func_to_register != NULL)
	{
		tc_control.test_function = func_to_register;
	}
	else
	{
	    printf("clTcInitialize: no test start routine registered\n");
		return CL_ERR_INVALID_PARAMETER;
	}

	if (subsystem != NULL)
	{
		strncpy(tc_control.subsystem, subsystem, TC_MAX_STR_LENGTH-1);
	}
	else
	{
	    printf("clTcInitialize: no subsystem registered\n");
		return CL_ERR_INVALID_PARAMETER;
	}

	if (test_name != NULL)
	{
		strncpy(tc_control.test_name, test_name, TC_MAX_STR_LENGTH-1);
	}
	else
	{
	    printf("clTcInitialize: no test case name registered\n");
		return CL_ERR_INVALID_PARAMETER;
	}
	ret_code = clOsalMutexCreate(&tc_control.run_test_mutex);

	if (ret_code != CL_OK)
	{
	    printf("clTcInitialize: failed create tc_run_test_mutex: 0x%x\n", 
			   ret_code);
		return ret_code;
    }

	ret_code = clOsalMutexCreate(&tc_control.exit_flag_mutex);
	if (ret_code != CL_OK)
	{
	    printf("clTcInitialize: failed create tc_exit_flag_mutex: 0x%x\n", 
			   ret_code);
		goto error_initialize;
    }

	/* Set to indicate we are successfuly initialized
	 */
	if (ret_code == CL_OK)
	{
		tc_control.init_complete = CL_TRUE;
	}
	return ret_code;

	error_initialize:
		clOsalMutexDelete(tc_control.run_test_mutex);

	return ret_code;
}

/*******************************************************************************
Function   : clTcDeactivate

Description: Set the tc_run_test_flag to false and frees up resources 
			 allocated during activate
Input      : none

Returns    : none
			 
*******************************************************************************/
void clTcDeactivate ( void )
{
	printf("clTcDeactivate: stopping test\n");
	clOsalMutexLock(tc_control.run_test_mutex);
	tc_control.run_test_flag = CL_FALSE;
	clOsalMutexUnlock(tc_control.run_test_mutex);

	if (tc_control.param_list.num_params != 0 &&
		tc_control.param_list.params != NULL)
	{
		clHeapFree(tc_control.param_list.params);
	}

	tc_control.param_list.num_params = 0;
	tc_control.param_list.params = NULL;
}

/*******************************************************************************
Function   : clTcActivate

Description: When the user application receives a 'ha state' change to Active
             it is expected that clTcActivate() is invoked in order to run the
			 test case. 

			 The routine that actuall implements the test case is registered
			 with clTcInitialize().

			 The CSI descriptor contains two attributes that describe the config
			 file name and location so that its contents can be parsed and the
			 parameter list popualated with its contents. 

			 Note: If there is a failure in parsing the contents of the file then
			       the user specified test routine will not be invoked. However
				   if there is a failure in interpreting the CSI descriptor it is
				   assumed the test runs without any arguments

Input      : 1. AMS CSI descriptor  
             2. AMS HA State. This is simply stored by TLC in case it is needed for
			    later processing.

Returns    : CL_OK if successfull
			 error code from tc_parse-work_load if not
*******************************************************************************/
int clTcActivate ( 
		ClAmsCSIDescriptorT *csi_desc,
		ClAmsHAStateT		ha_state )
{

	ClRcT ret_code = CL_OK;
	if (tc_control.init_complete != CL_TRUE)
	{
		printf("clTcActivate: cannot run test [init not complete]\n");
		return CL_ERR_INVALID_STATE;
	}

	ret_code = clTcParseWorkLoad(csi_desc, tc_control.subsystem,
							     tc_control.test_name, &tc_control.param_list);	

	if (ret_code == CL_OK || ret_code == CL_ERR_INVALID_PARAMETER)
	{
		if (ret_code == CL_ERR_INVALID_PARAMETER)
		{
			printf("clTcActivate: ignoring error related to invalid CSI descriptor\n");
			ret_code = CL_OK;
		}

		tc_control.param_list.ha_state = ha_state;
	
		printf("clTcActivate: running test\n");
		clOsalMutexLock(tc_control.run_test_mutex);
		tc_control.run_test_flag = CL_TRUE;
		clOsalMutexUnlock(tc_control.run_test_mutex);
	}
	else
	{
		printf("clTcActivate: error parsing config file; test will not run\n");
	}
	
	return ret_code;
}

/*******************************************************************************
Function   : clTcFinalize

Description: Set the exit_flag to true so that the clTcRun() loop stops
			 and returns control back to the user

Input      : none

Returns    : none
			 
*******************************************************************************/
void clTcFinalize ( void )
{
	printf("clTcFinalize: quitting run loop\n");
	clOsalMutexLock(tc_control.exit_flag_mutex);
	tc_control.exit_flag = CL_TRUE;
	clOsalMutexUnlock(tc_control.exit_flag_mutex);
}

/*******************************************************************************
Function   : clTcPrintParams

Description: Utility function that prints all the parameters that are read
             in the configuration file; along with the subsystsem and test name
			 Current this API calls the clTestPrint Macro as defined in 
			 clTestApi.h
			
Input      : none

Returns    : none
*******************************************************************************/
void clTcPrintParams ( void )
{
	int i;
	/* ensure that we successfully completed initialization
	 * we cannot run the test
	 */
	if (tc_control.init_complete != CL_TRUE)
	{
		printf("clTcPrintParams: (fatal) test not initialized\n");
		return;
	}

	clTestPrint(("Component: '%s'; Test Name: '%s'\nTest Parameters:\n", 
				tc_control.subsystem, tc_control.test_name));
	for (i = 0; i < tc_control.param_list.num_params; i++) 
	{
		switch(tc_control.param_list.params[i].type)
		{
			case TC_PARAM_STRING:
				clTestPrint(("%s : %s\n", 
							tc_control.param_list.params[i].name,
							tc_control.param_list.params[i].value.str_val));
				break;
			case TC_PARAM_INT32:
				clTestPrint(("%s : %d\n", 
							tc_control.param_list.params[i].name,
							tc_control.param_list.params[i].value.int_val));
				break;
			case TC_PARAM_FLOAT:
				clTestPrint(("%s : %f\n", 
							tc_control.param_list.params[i].name,
							tc_control.param_list.params[i].value.flt_val));
				break;
			default:
				break;
		}
	}
}

/*******************************************************************************
Function   : clTcRun

Description: Runs a forever loop, and check for two conditions. If the 
			 tc_run_test_flag is TRUE, then the user defined start routine is
			 invoked with the configuration parameters read by clTcActivate
			 The assumption here is that the "THREAD_FOR_APP" option was chosen
			 during the modeling of this application. 

			 When the user specified routine returns clTcDeactivate() is invoked
			 to stop this run.

			 The other condition tested for is tc_exit_test_flag. If this is 
			 true then the clTcFinalize() routine has been invoked, which means
			 the application is being terminated
			 
Input      :  none

Returns    :  none
*******************************************************************************/
void clTcRun ( void )
{
	/* ensure that we successfully completed initialization
	 * we cannot run the test
	 */
	if (tc_control.init_complete != CL_TRUE)
	{
		printf("clTcRun: (fatal) test not initialized \n");
		return;
	}

	for ( ;; )
	{
		/* did the component get a terminate
		 * request
		 */
	    clOsalMutexLock(tc_control.exit_flag_mutex);
		if (tc_control.exit_flag == CL_TRUE)
		{
	    	clOsalMutexUnlock(tc_control.exit_flag_mutex);
			break;
		}
	    clOsalMutexUnlock(tc_control.exit_flag_mutex);

		/* Should we start another run of the
		 * test
		 */
		clOsalMutexLock(tc_control.run_test_mutex);
		if (tc_control.run_test_flag == CL_TRUE)
		{
			clOsalMutexUnlock(tc_control.run_test_mutex);

			/* call user register test function
			 */
			(tc_control.test_function)(&tc_control.param_list);

			/* free up parameter list resources and
		 	 * go back to waiting
		 	 */
			clTcDeactivate();
		}
		else
		{
			clOsalMutexUnlock(tc_control.run_test_mutex);
		}

		/* For now this app sleeps in order not to hug CPU
		 */
		sleep(1);
	}

	/* Free up internal resources and return
	 * control back to calling function
	 */
	clOsalMutexDelete(tc_control.exit_flag_mutex);

	clOsalMutexDelete(tc_control.run_test_mutex);

	if (tc_control.param_list.num_params != 0 &&
	    tc_control.param_list.params != NULL)
	{
	    clHeapFree(tc_control.param_list.params);

		tc_control.param_list.num_params = 0;
	    tc_control.param_list.params = NULL;
	}

	tc_control.init_complete = CL_FALSE;
}
