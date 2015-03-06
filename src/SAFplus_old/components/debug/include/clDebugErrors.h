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
* ModuleName  : debug                                                         
* File        : clDebugErrors.h
*******************************************************************************/

/******************************************************************************
* Description :
*   This module contains common DEBUG Error definitions
******************************************************************************/

/**
 *  \file
 *  \brief Header file of Debug Error Codes
 *  \ingroup debug_apis 
 */
       
/**
 ************************************
 *  \addtogroup debug_apis
 *  \{
 */


#ifndef _CL_DEBUG_ERRORS_H_
#define _CL_DEBUG_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * DEBUG Specific Error Codes.
 */
#define CL_DEBUG_RC(ERROR_CODE)             CL_RC(CL_CID_DEBUG, (ERROR_CODE))

/**
 * Debug Library error.
 */
#define CL_DBG_ERR_CMD_NOT_FOUND            0x100

/**
 * Debug unrecognized command 
 */
#define CL_DBG_ERR_UNRECOGNIZED_CMD         0x101

/**
 * Debug command ignored 
 */
#define CL_DBG_ERR_INVALID_CTX              0x102

/**
 * Invalid time out
 */
#define CL_DBG_ERR_INVALID_PARAM            0x103

/**
 * Loglevel set failed 
 */
#define CL_DBG_ERR_COMMON_ERROR             0x104


#ifdef __cplusplus
}
#endif

#endif /* _CL_DEBUG_ERRORS_H_ */

/**
 *  \}
 */
