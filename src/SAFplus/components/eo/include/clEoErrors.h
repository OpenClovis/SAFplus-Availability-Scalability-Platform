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
 * ModuleName  : eo
 * File        : clEoErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains all the error messages related to EO.
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of EO related Error Messages
 *  \ingroup eo_apis
 */

/**
 *  \addtogroup eo_apis
 *  \{
 */


#ifndef _CL_EO_ERRORS_H_
#define _CL_EO_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
    

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/

/**
 * EO related error codes.
 */
/*#define CL_EO_ERR_FUNC_NOT_IMPLEMENTED      0x100*/

/**
 * EO is currently in suspended state, so will be be able to provide service
 */
#define CL_EO_ERR_EO_SUSPENDED              0x101

/**
 * Called EO RMD function is not currently registered
 */
#define CL_EO_ERR_FUNC_NOT_REGISTERED       0x102

/**
 * Invalid client ID is passed, as one of the arguments
 */
#define CL_EO_ERR_INVALID_CLIENTID          0x103

/**
 * Invalid service ID is passed, as one of the arguments
 */
#define CL_EO_ERR_INVALID_SERVICEID         0x104

/**
 * The Library ID specified is invalid
 */
#define CL_EO_ERR_LIB_ID_INVALID            0x105

/**
 * The Water Mark ID specified is invalid
 */
#define CL_EO_ERR_WATER_MARK_ID_INVALID     0x106

/**
 * EO state can not be set to the provided value
 */
#define CL_EO_ERR_IMPROPER_STATE            0x107

/**
 * The Action Queue has overflown
 */
#define CL_EO_ERR_QUEUE_OVERFLOW            0x108

/**
 * Enqueue this message, even though it was originally targeted as non-blocking
 */
#define CL_EO_ERR_ENQUEUE_MSG               0x109


/******************************************************************************
 * ERROR/RETURN CODE HANDLING MACROS
 *****************************************************************************/


#define CL_EO_RC(ERROR_CODE)  CL_RC(CL_CID_EO, (ERROR_CODE))
             
#ifdef __cplusplus

}
#endif

#endif /* _CL_EO_ERRORS_H_ */

/** \} */
