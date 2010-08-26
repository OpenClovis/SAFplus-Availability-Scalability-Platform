/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */

/**
 * \file 
 * \brief Header file for ASP Test Lifecycle Control (TLC) APIs
 * \ingroup test_apis
 */

/**
 *  \addtogroup test_apis
 *  \{
 */


#ifndef TC_UTILS_API_H
#define TC_UTILS_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clAmsTypes.h>

/**
 * Default size of all strings used within the TLC infrastructure
 */
#define TC_MAX_STR_LENGTH	256

/**
 * Current paramater types supported by TLC.
 * 
 * Each test case can define the runtime configuration parameters in an
 * XML file, current limited to strings, integers and float types.
 */
typedef enum ClTcParamTypeE
{
	TC_PARAM_NONE,
	TC_PARAM_STRING,
	TC_PARAM_INT32,
	TC_PARAM_FLOAT
} ClTcParamTypeE;

/**
 * Union to store parsed runtime configuration paramater value, by TLC
 *
 * The XML file that defines the runtime configuration parameters
 * is parsed into the union and the type set to indicate what
 * type of parameter is stored.
 */
typedef union ClTcParamValU
{
	ClInt32T	int_val;
	double		flt_val;
	ClCharT		str_val[TC_MAX_STR_LENGTH];
} ClTcParamValU;

/**
 * Basic structure to store the parsed runtime configuration paramater, by TLC
 *
 * The XML file that defines the runtime configuration parameters
 * is parsed into this structure that defines the parameter name
 * its type and the value. The interger id field is used as a short cut
 * instead of strcmp within your code, you can use a switch() statement
 */
typedef struct ClTcParamT 
{
	/** parameter name
	 */
	ClCharT	 		name[TC_MAX_STR_LENGTH];
	/** paramter id; has the same meaning as paramater name, must be unique
	 */
	ClInt32T		id; /* short cut to avoid strcmps */
	/**  parameter type (see ClTcParamTypeE)
	 */
	ClTcParamTypeE	type;
	/** parameter value (see ClTcParamValU)
	 */
	ClTcParamValU	value;
} ClTcParamT;

/**
 * List to store all the parsed runtime configuration paramaters, by TLC
 * 
 * The parsing routines will allocate an array of type ClTcParamT and 
 * store the parsed values into it. 
 */
typedef struct ClTcParamListT
{
	ClAmsHAStateT 	ha_state;/* unsure if this is needed yet */
	ClInt32T		num_params;
	ClTcParamT		*params;
} ClTcParamListT;


/**
 * \brief Parse a file containing runtime paramaters for a test case
 *
 * \param file_path complete path to config file location on target
 * \param file_name config file name follows schema defined by clTcParamDef.xsd
 * \param subsystem_name test case qualifier used by parser to search 
 *        the configuration file to get to the appropriate subsystem 
 * \param test_case_name test case qualifier used by parser to search the 
 *        configuration file to get to the appropriate test case within the subsystem
 *
 * \return param_list pointer to a structure of type tc_param_listT in which all the parsed parameters are stored
 *
 * \retval CL_ERR_NULL_POINTER  file could not be opened by ezxml
 * \retval CL_ERR_NO_MEMORY    memory allocation failed
 * \retval CL_ERR_INVALID_PARAMETER  input argument(s) is/are invalid
 * \retval CL_ERR_DOESNT_EXIST  test case could not be found in config file
 * \retval CL_ERR_INVALID_BUFFER general parsing error
 *
 * \par Description
 * Parses a given XML file and the retrieves configuration parameters 
 * for a given test case and subsystem. This API assumes that the 
 * tc_param_listT struct is not allocated; so ensure that is the case 
 * before calling this API or else you will end up with a memory leak
 *
 * \par Library File:
 *   libTcClUtils.a
 *
 *  \sa bar()
 *
 */
int 
clTcParseConfigFile (
			ClCharT			*file_path,
			ClCharT 		*file_name,
			ClCharT			*subsystem_name,
			ClCharT			*test_case_name,
			ClTcParamListT 	*param_list );

/**
 *  \brief Wrapper function for clTcParsConfigFile, called with the AMF CSI descriptor
 *
 *  \param csi_desc Pointer to ClAmsCSIDescriptorT structure sent by AMF during a
 *         HA state change. This is expected to contain the configuration file name
 *         and location.
 * \param subsystem_name test case qualifier used by parser to search 
 *        the configuration file to get to the appropriate subsystem 
 * \param test_case_name test case qualifier used by parser to search the 
 *        configuration file to get to the appropriate test case within the subsystem
 * \return param_list pointer to a structure of type tc_param_listT in which all the  *         parsed parameters are stored
 *
 *  \retval CL_ERR_INVALID_PARAMETER if input arguments are invalid
 *  \retval see return codes for clTcParseConfigFile()
 *
 *  \par Description:
 *   A wrapper function for clTcParseConfigFile, but this API
 *   takes the CSI descriptor as input and returns the parameter list. See 
 *   clTcParseConfigFile() for more details.
 *
 *  \par Library File:
 *   libTcClUtils.a
 *
 *  \sa bar()
 *
 */
int 
clTcParseWorkLoad (
			ClAmsCSIDescriptorT *csi_desc,
			ClCharT				*subsystem_name,
			ClCharT				*test_case_name,
			ClTcParamListT 		*param_list );

/**
 *  \brief Initialize Test Lifecycle Control (TLC) infrastructure and register test case start function
 *
 *  \par Header File:
 *   clTcUtils.h
 *
 * \param subsystem_name test case qualifier used by parser to search 
 *        the configuration file to get to the appropriate subsystem 
 * \param test_case_name test case qualifier used by parser to search the 
 *        configuration file to get to the appropriate test case within the subsystem
 *  \param (*func_to_register)(ClTcParamListT *param_list) function pointer to user
           function that runs the test case
 *
 *  \return None
 * 
 *  \retval CL_OK if all succeeds
 *  \retval CL_ERR_INVALID_PARAMETER if function pointer is NULL
 *  \retval ASP return codes if OSAL API calls fail
 *
 *  \par Description:
 *   Initializes internal data structures so that a test can be started
 *   or stopped based on the locking and unlocking of a Service Group.
 *   The assumption here is that the "THREAD_FOR_APP" option was chosen
 *   during the modeling of this application. 
 *   
 *   The user is expected to call clTcRun() at the end of initialization
 *   phase in clCompAppInitialize() so that a forever loop is started.
 *
 *   When the state change to active is received, the user calls
 *   clTcActivate() which runs the test function registered with this
 *
 *  \par Library File:
 *   libClTcUtils.a
 *
 *  \sa bar()
 *
 */
int clTcInitialize ( 
		const ClCharT	*subsystem,
		const ClCharT	*test_name,
		int 	(*func_to_register)(ClTcParamListT *param_list) );

/**
 *  \brief Stop the Test Lifecyle Control (TLC) control loop.
 *
 *  \par Header File:
 *   clTcUtils.h
 *
 *  \param  None
 *
 *  \return None
 * 
 *  \retval None
 *
 *  \par Description:
 *   Set the exit_flag to true so that the tc_run() loop stops
 *   and returns control back to the user
 *
 *  \par Library File:
 *   libClTcUtils.a
 *
 *  \sa bar()
 *
 */
void clTcFinalize ( void );


/**
 *  \brief API called to start the test case registered by the user in clTcInitialze
 *
 *  \par Header File:
 *   clTcUtils.h
 *
 *  \param csi_desc Pointer to ClAmsCSIDescriptorT structure sent by AMF during a
 *         HA state change. This is expected to contain the configuration file name
 *         and location.
 *  \param ha_state Variable of type ClAmsHAStateT, used to store ha state, not needed
 *         by TLC but may be needed by application
 *
 *  \return None
 * 
 *  \retval CL_OK if successfull
 *  \retval return codes from clTcParseWorkLoad()
 *
 *  \par Description:
 *   When the user application receives a 'ha state' change to Active
 *   it is expected that clTcActivate() is invoked in order to run the
 *   test case. 
 *   The routine that actuall implements the test case is registered
 *   with clTcInitialize().
 *   The CSI descriptor contains two attributes that describe the config
 *   file name and location so that its contents can be parsed and the
 *   parameter list popualated with its contents. 
 *   If there is a failure in parsing the contents of the file then
 *   the user specified test routine will not be invoked. However
 *   if there is a failure in interpreting the CSI descriptor it is
 *   assumed the test runs without any arguments
 *
 *  \par Library File:
 *   libClTcUtils.a
 *
 *  \sa bar()
 *
 */
int clTcActivate ( ClAmsCSIDescriptorT *csi_desc, ClAmsHAStateT ha_state );

//
//Function   : clTcDeactivate
//
//Description: Set the tc_run_test_flag to false and frees up resources 
//allocated during activate
//Input      : none
//
//Returns    : none

void clTcDeactivate ( void );


/**
 *  \brief API called to activate Test Lifecycle Control (TLC) control loop
 *
 *  \par Header File:
 *   clTcUtils.h
 *
 *  \param None
 *
 *  \return None
 * 
 *  \retval None
 *
 *  \par Description:
 *   Runs a forever loop, and check for two conditions. If the 
 *   internal flag tc_run_test_flag is TRUE, then the user defined start routine is
 *   invoked with the configuration parameters read by clTcActivate
 *   The assumption here is that the "THREAD_FOR_APP" option was chosen
 *   during the modeling of this application. 
 *   When the user specified routine returns clTcDeactivate() is invoked
 *   to stop this run.
 *   The other condition tested for is the internal flag tc_exit_test_flag. 
 *   If this is true then the tc_finalize() routine has been invoked, which means
 *   the application is being terminated
 *
 *  \par Library File:
 *   libClTcUtils.a
 *
 *  \sa bar()
 *
 */

void clTcRun ( void );


/**
 *  \brief Print utility that prints the parsed runtime configuration parameters
 *
 *  \par Header File:
 *   clTcUtils.h
 *   clTestApi.h
 *
 *  \param None
 *
 *  \return None
 * 
 *  \retval None
 *
 *  \par Description:
 *   Utility function that prints all the parameters that are read
 *   in the configuration file; along with the subsystsem and test name
 *   Current this API calls the clTestPrint Macro as defined in clTestApi.h
 *
 *  \par Library File:
 *   libClTcUtils.a
 *
 *  \sa bar()
 *
 */
void clTcPrintParams ( void );

#ifdef __cplusplus
}
#endif

#endif /* TC_UTILS_API_H */

/** \} */
