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
/*******************************************************************************
 * ModuleName  : med
 * File        : clMedLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_MED_LOG_H_
# define _CL_MED_LOG_H_

# ifdef __cplusplus
extern "C"
{
# endif

extern ClCharT * clLogMedMsg[];

#define CL_MED_LOG_1_PRE_CREATED_OBJ_NOTIFY_FAILED  clLogMedMsg[0]
#define CL_MED_LOG_1_ID_XLN_IS_NOT_PRESENT  clLogMedMsg[1]
#define CL_MED_LOG_1_OP_CODE_IS_NOT_PRESENT  clLogMedMsg[2]
#define CL_MED_LOG_1_ERROR_CODE_IS_NOT_PRESENT  clLogMedMsg[3]
#define CL_MED_LOG_1_WATCH_ATTR_IS_NOT_PRESENT  clLogMedMsg[4]
#define CL_MED_LOG_1_INSTANCE_XLN_CALLBACK_FAILED  clLogMedMsg[5]
#define CL_MED_LOG_1_ID_XLN_INSERT_FAILED  clLogMedMsg[6]
#define  CL_MED_LOG_1_ID_XLN_DELETE_FAILED clLogMedMsg[7]
#define  CL_MED_LOG_0_ID_XLN_INDEX_NULL  clLogMedMsg[8]

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_MED_LOG_H_ */
