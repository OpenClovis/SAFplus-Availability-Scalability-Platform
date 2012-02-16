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

/**
 * \file 
 * \brief Header file of Log Service defined Errors
 * \ingroup log_apis
 */

/**
 * \addtogroup log_apis
 * \{
 */

#ifndef _CL_LOG_ERRORS_H_
#define _CL_LOG_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * INCLUDES 
     */
#include <clCommon.h>
#include <clCommonErrors.h>

#define CL_LOG_RC(err_code) (CL_RC(CL_CID_LOG, err_code))

/**
  * If file full action is CL_LOG_FILE_FULL_ACTION_HALT, then file becomes
  * full, this error will be returned to the user.
  */
#define CL_LOG_ERR_FILE_FULL          0x101
  
/**
 * If the configuration clLog.xml is not proper, some of the expected fields are
 * not proper, then this error will be returned.
 */
#define CL_LOG_ERR_INVALID_XML_FILE   0x102

 
#ifdef __cplusplus
}
#endif

#endif  /* _CL_LOG_ERRORS_H_ */

/** 
 * \} 
 */
