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
#ifndef TC_XML_TAGS_H
#define TC_XML_TAGS_H

/* not XML tags; per se but they serve the same
 * purpose as they are part of amfDefintions.xml
 */
#define TC_CSI_WORK_LOAD_NUM_ARGS 2

#define TC_CONFIG_FILE			"config_file"
#define TC_CONFIG_FILE_PATH		"file_path"

/* XM TAGS  for all test cases in the config file*/
#define TC_TAG_TEST_CASES	    "test_cases"

/* XML Attribute tags to go to the subsystem of interest */
#define TC_ATTR_SUBSYSTEM_NAME	"subsystem"
#define TC_ATTR_NUM_TEST_CASES	"num_tests"

/* XM TAGS for an individual test case in the config file*/
#define TC_TAG_TEST_CASE	    "test_case"

/* XML Attribute tags to go to the test case of interest */
#define TC_ATTR_TEST_CASE_NAME	"test_name"
#define TC_ATTR_TEST_CASE_ID	"test_id"

/* XML tags to define paramaters for each test case */
#define TC_TAG_PARAM     		"tc_params"
#define TC_TAG_PARAM_NAME  		"param_name"
#define TC_TAG_PARAM_ID  		"param_id"
#define TC_TAG_PARAM_VALUE 		"param_value"
#define TC_TAG_PARAM_VAL_STR	"str"
#define TC_TAG_PARAM_VAL_INT	"int"
#define TC_TAG_PARAM_VAL_FLT	"flt"

#endif /* TC_XML_TAGS_H */
