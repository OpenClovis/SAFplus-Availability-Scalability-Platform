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
 * ModuleName  : utils
 * File        : clOampRtErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header contains the various OAMP error codes.
 *
 *
 *****************************************************************************/
#ifndef _CL_OAMP_RT_ERRORS_H_
#define _CL_OAMP_RT_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* OAMP RT ERROR codes */
#define CL_OAMP_RT_RC(ERR_ID)                           (CL_RC(CL_CID_OAMP_RT, ERR_ID))

/**
 *  An internal error occurred in this library.  See the logs for more information.
 */
#define CL_OAMP_RT_ERR_INTERNAL                    0x100

/**
 * The file does not exist
 */
#define CL_OAMP_RT_ERR_FILE_NOT_EXIST              0x101


/** 
 *  Returned when both wildcard and object creation is specified.
 */
#define CL_OAMP_RT_ERR_INVALID_CONFIG		   0x103

#ifdef __cplusplus
}
#endif

#endif /* _CL_OAMP_RT_ERRORS_H_ */


