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
File       : clTcParser.c

Description: Provides APIs to parse a XML configuration file 
             that specifies the parameters to run the test with

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHeapApi.h>
#include <clParserApi.h>
#include <clDebugApi.h>


#include <clTcUtils.h>
#include <clTcParserTags.h>

#if 0 /* used for debugging */
#if defined(CL_DEBUG_LEVEL_THRESHOLD)
#undef  CL_DEBUG_LEVEL_THRESHOLD
#endif
#define CL_DEBUG_LEVEL_THRESHOLD CL_DEBUG_INFO
#endif
/*******************************************************************************
Function : clTcParseWorkLoad

Description: Mostly a wrapper function for clTcParseConfigFile, but this API
             takes the CSI descriptor as input and returns the parameter list

Input    : 1. AMS CSI descriptor
		   2. subsystem name for test case of interest
		   3. name of test case of interest

Output   : ClTcParamListT with all the parameters read

Returns  : CL_ERR_INVALID_PARAMETER (if CSI parsing fails)
           error codes returned by clTcParseConfigFile

Note     : This API assumes that the ClTcParamListT struct is not allocated; so
           ensure that is the case before calling this API or else you will
		   end up with a memory leak
*******************************************************************************/
int 
clTcParseWorkLoad (
			ClAmsCSIDescriptorT *csi_desc,
			ClCharT				*subsystem_name,
			ClCharT				*test_case_name,
			ClTcParamListT 		*param_list )
{
	int 	iter;
	ClCharT *attr_name;
	ClCharT	config_file[TC_MAX_STR_LENGTH];
	ClCharT	file_path[TC_MAX_STR_LENGTH];

	if (csi_desc->csiAttributeList.numAttributes < TC_CSI_WORK_LOAD_NUM_ARGS) 
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseWorkLoad: expected:%d params got %d\n", 
			   							TC_CSI_WORK_LOAD_NUM_ARGS,
			   							csi_desc->csiAttributeList.numAttributes));
		return CL_ERR_INVALID_PARAMETER;
	}

	/* iterate through attribute list
	 */
	for (iter = 0; iter < csi_desc->csiAttributeList.numAttributes; iter++)
	{
		attr_name = 
		(ClCharT*)csi_desc->csiAttributeList.attribute[iter].attributeName;

		if (strcmp(attr_name, TC_CONFIG_FILE) == 0)
		{
			strcpy(config_file, (ClCharT*)
				   csi_desc->csiAttributeList.attribute[iter].attributeValue);
		}
		else if (strcmp(attr_name, TC_CONFIG_FILE_PATH) == 0)
		{
			strcpy(file_path, (ClCharT*)
				   csi_desc->csiAttributeList.attribute[iter].attributeValue);
		}
		else
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseWorkLoad: received invalid attribute:%s\n", 
				   							attr_name));
			return CL_ERR_INVALID_PARAMETER;
		}
	}

	return (clTcParseConfigFile(file_path, config_file, 
							    subsystem_name, test_case_name,
							    param_list));
}

/*******************************************************************************
Function : clTcParseConfigFile

Input    : 1. path to config file
           2. config file name
		   3. subsystem name for test case of interest
		   4. name of test case of interest

Output   : ClTcParamListT with all the parameters read

Returns  : CL_ERR_NULL_POINTER (if file could not be opened by ezxml)
		   CL_ERR_NO_MEMORY    (if memeory allocation fails)
		   CL_ERR_NULL_POINTER (if input arguments are invalid)
		   CL_ERR_DOESNT_EXIST (if test case could not be found)
		   CL_ERR_INVALID_BUFFER (general parsing error)

Note     : This API assumes that the ClTcParamListT struct is not allocated; so
           ensure that is the case before calling this API or else you will
		   end up with a memory leak
*******************************************************************************/
int 
clTcParseConfigFile (
			ClCharT			*file_path,
			ClCharT 		*file_name,
			ClCharT			*subsystem_name,
			ClCharT			*test_case_name,
			ClTcParamListT 	*param_list )
{

    ClParserPtrT  	file_ptr   = NULL;

    ClParserPtrT  	test_cases = NULL;
    ClParserPtrT  	test_case  = NULL;
	const char*		sub_name   = NULL;
	const char*		test_name  = NULL;

	ClParserPtrT	params     = NULL;
	int			 	num_params = 0;
	ClParserPtrT	param_name = NULL;
	ClParserPtrT	param_id   = NULL;
	ClParserPtrT	param_values= NULL;
	ClParserPtrT	param_value= NULL;


	/* prints error messaeg if test case specified
	 * is not found in the confgiuration file
	 */
	ClBoolT			found_test_case = CL_FALSE;

	ClRcT			ret_code = CL_OK;

    if (file_path == NULL || file_name == NULL || subsystem_name == NULL  || 
	    test_case_name == NULL || param_list == NULL)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseConfigFile: NULL pointer input argument \
		       							path=%s name=%s list=%p\n subsyetm=%s test_case=%s\n",
			   							file_path, file_name, (ClPtrT)param_list, 
			   							subsystem_name, test_case_name));
        
        return CL_ERR_NULL_POINTER;
    }


    file_ptr = clParserOpenFile(file_path, file_name);
    if (file_ptr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseConfigFile: Error opening config file \
			   							(path=%s name=%s). "\
                                        "Trying ASP_CONFIG path\n",
			   							file_path, file_name));
        file_path = getenv("ASP_CONFIG");
        if(!file_path)
        {
            return CL_ERR_NULL_POINTER;
        }
        else
        {
            file_ptr = clParserOpenFile(file_path, file_name);
            if(!file_ptr)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseConfigFile: Error "\
                                                "opening file "\
                                                "(path=%s name=%s).\n",
                                                file_path, file_name));
                return CL_ERR_NULL_POINTER;
            }
        }
    } 


	/* locate the test_cases paragraph
	 */
	test_cases = clParserChild(file_ptr, TC_TAG_TEST_CASES); 
	if (test_cases == NULL)
	{
       	CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
						("clTcParseConfigFile: could not find tag: %s \
						(path=%s name=%s)\n", 
						TC_TAG_TEST_CASES, file_path, file_name));
		ret_code = CL_ERR_INVALID_BUFFER;
       	goto finish_parsing_file;
	}
	
	
	/* iterate through all test cases to get the subsystem and test case of
	 * interest 
	 */
	for (test_case = clParserChild(test_cases, TC_TAG_TEST_CASE); test_case; 
		 test_case = CL_PARSER_NEXT(test_case))
	{
		sub_name = clParserAttr(test_case, TC_ATTR_SUBSYSTEM_NAME);
		if (sub_name == NULL) 
		{
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
							("clTcParseConfigFile: subsystem attribute missing \
							(path=%s name=%s)\n", file_path, file_name));
			ret_code = CL_ERR_INVALID_BUFFER;
        	goto finish_parsing_file;
		}

		test_name = clParserAttr(test_case, TC_ATTR_TEST_CASE_NAME);
		if ( test_name == NULL)
		{
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
						   ("clTcParseConfigFile: test_case name attribute missing \
				   			(path=%s name=%s)\n", file_path, file_name));
			ret_code = CL_ERR_INVALID_BUFFER;
        	goto finish_parsing_file;
		}

		CL_DEBUG_PRINT(CL_DEBUG_INFO,
		               ("clTcParseConfigFile: sub system=%s; test case=%s\n",
					    sub_name, test_name));

		if ((strcasecmp(test_name, test_case_name) == 0) &&
			(strcasecmp(sub_name, subsystem_name) == 0))
		{
			found_test_case = CL_TRUE;

			/* get number of parameters 
		  	 */
			for(params = clParserChild(test_case, TC_TAG_PARAM); params;
				params = CL_PARSER_NEXT(params))
			{
				num_params++;
			}

			param_list->num_params = num_params;

			CL_DEBUG_PRINT(CL_DEBUG_INFO, 
						   ("clTcParseConfigFile: sub-system:%s; \
						    test-case=%s; num params=%d\n",
						    subsystem_name, test_case_name, num_params));

			/* Allocate memory for paramaters
			 */
			param_list->params = clHeapAllocate(num_params * sizeof(ClTcParamT));
			if (param_list->params == NULL)
			{
        		CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
				               ("clTcParseConfigFile: failed allocation for params \
					   			(path=%s name=%s)\n", file_path, file_name));
				ret_code = CL_ERR_NO_MEMORY;
        		goto finish_parsing_file;
			}

			for (params = clParserChild(test_case, TC_TAG_PARAM), num_params=0; 
				 params; params = CL_PARSER_NEXT(params), num_params++)
			{
				/* parameter name */
				param_name  = clParserChild(params, TC_TAG_PARAM_NAME);

				/* validate for correct parsing */
				if (!param_name)
				{
					CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
								   ("clTcParseConfigFile: error parsing name; parameter # %d\n", 
								   	num_params));
					ret_code = CL_ERR_INVALID_BUFFER;
        			goto error_parsing_params;
				}

				strcpy(param_list->params[num_params].name, param_name->txt);

				/* parameter id */
				param_id  = clParserChild(params, TC_TAG_PARAM_ID);
				/* validate for correct parsing */
				if (!param_id)
				{
					CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
								   ("clTcParseConfigFile: error parsing id; parameter # %d\n", 
								   	num_params));
					ret_code = CL_ERR_INVALID_BUFFER;
        			goto error_parsing_params;
				}

				param_list->params[num_params].id = atoi(param_id->txt);

				/* parameer values is a choice of three;
				 * needs one more level of probing
				 */
				param_values = clParserChild(params, TC_TAG_PARAM_VALUE);

				/* value can be one of string, int or float first try string
				 */
				param_value = clParserChild(param_values, TC_TAG_PARAM_VAL_STR);
				if (param_value == NULL) /* try int next */
				{
					param_value = clParserChild(param_values, 
											    TC_TAG_PARAM_VAL_INT);
					if (param_value == NULL) /* try float next */
					{
						param_value = clParserChild(param_values, 
													TC_TAG_PARAM_VAL_FLT);
						if (param_value == NULL) /* error */
						{
							CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
							               ("clTcParseConfigFile:param value type: has \
							       		    be one of %s, %s, %s(path=%s name=%s)\n",
								   			TC_TAG_PARAM_VAL_STR,TC_TAG_PARAM_VAL_INT,
								   			TC_TAG_PARAM_VAL_FLT, 
											file_path, file_name)); 
							ret_code = CL_ERR_INVALID_BUFFER;
        					goto error_parsing_params;
						}
						else /* value is of type float */
						{
							param_list->params[num_params].type = TC_PARAM_FLOAT;
							param_list->params[num_params].value.int_val = 
														atof(param_value->txt);
						}
					}
					else /* value is of type int */
					{
						param_list->params[num_params].type = TC_PARAM_INT32;
						param_list->params[num_params].value.int_val = 
														atoi(param_value->txt);
					}
				}
				else /* value is of type string */
				{
					param_list->params[num_params].type = TC_PARAM_STRING;
					strcpy(param_list->params[num_params].value.str_val, 
						   param_value->txt);
				}
			}
			break;
		}
	}
	
	/* check to see if the test case was found
	 */
	if (found_test_case != CL_TRUE)
	{
   		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clTcParseConfigFile: could not find test_case=%s for \
			   							subsystem=%s (path=%s name=%s)\n", 
			   							test_case_name, subsystem_name, file_path, file_name));
		ret_code = CL_ERR_DOESNT_EXIST; 
	}

	finish_parsing_file:
		ezxml_free(file_ptr);

	return ret_code;

	error_parsing_params:
		/* free up parameter list */
		clHeapFree(param_list->params);
		param_list->num_params = 0;

		ezxml_free(file_ptr);

	return ret_code;
}
