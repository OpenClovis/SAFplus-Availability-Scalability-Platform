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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/common/emTestPatterns.h $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_EM_TEST_PATTERNS_H_
# define _CL_EM_TEST_PATTERNS_H_

# ifdef __cplusplus
extern "C"
{
# endif

    /*
     * Indices to Pattern & Filter Database 
     */
    typedef enum ClEvtPatternIndex
    {

        CL_EVT_DEFAULT_PATTERN = 0,
        CL_EVT_STR_PATTERN = 1,
        CL_EVT_STRUCT_PATTERN = 2,
        CL_EVT_MIXED_PATTERN = 3,
        CL_EVT_NORMAL_PATTERN = 4,
        CL_EVT_SUPERSET_PATTERN = 5,
        CL_EVT_SUBSET_PATTERN = 6,
        CL_EVT_DEFAULT_PASS_ALL_PATTERN = 7,
        CL_EVT_NORMAL_PASS_ALL_PATTERN = 8,
        CL_EVT_SUBSET_PASS_ALL_PATTERN = 9,

    } ClEvtPatternIndexT;

    extern ClPtrT gEmTestAppFilterDb[];
    extern ClSizeT gEmTestAppFilterDbSize;
    extern ClPtrT gEmTestAppPatternDb[];
    extern ClSizeT gEmTestAppPatternDbSize;

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_EM_TEST_PATTERNS_H_ */
