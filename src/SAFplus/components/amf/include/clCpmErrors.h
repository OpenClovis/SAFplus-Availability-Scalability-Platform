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
 *  \file
 *  \brief Header file of error Messages that are CPM specific.
 *  \ingroup cpm_apis
 */

/**
 *  \addtogroup cpm_apis
 *  \{
 */

#ifndef _CL_CPM_ERRORS_H_
#define _CL_CPM_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/**
 * Requested operation is not allowed.
 */
#define CL_CPM_ERR_OPERATION_NOT_ALLOWED    0x100

/**
 * EO is not reachable.
 */
#define CL_CPM_ERR_EO_UNREACHABLE           0x101

/**
 * One of the passed argument is invalid.
 */
#define CL_CPM_ERR_INVALID_ARGUMENTS        0x102

/**
 * Requested operation could not be performed by Component Manager.
 */
#define CL_CPM_ERR_OPERATION_FAILED         0x103

/**
 * Component Manager is not able to forward the request to the
 * required node.
 */
#define CL_CPM_ERR_FORWARDING_FAILED        0x104

/**
 * Component Manager cannot handle this request at this point, as it
 * is currently doing similar operation.
 */
#define CL_CPM_ERR_OPERATION_IN_PROGRESS    0x105

/**
 * Component Manager returns this error if the initialization was not
 * in a proper way and other functions are being accessed.
 */
#define CL_CPM_ERR_INIT                     0x106

/**
 * Component Manager returns this error when it receives an invalid
 * request.
 */
#define CL_CPM_ERR_BAD_OPERATION            0x107

/**
 * Component Manager returns this error when the same registration
 * request is performed multiple times.
 */
#define CL_CPM_ERR_EXIST                    0x108

/**
 * Bug 3610: Added this message.
 * Component Manager returns this error when processing of the request
 * was abandoned due to higher priority request.
 */
#define CL_CPM_ERR_OPER_ABANDONED           0x109

/**
 * Error macro definitions for CPM.
 */
#define CL_CPM_RC(ERROR_CODE)  CL_RC(CL_CID_CPM, (ERROR_CODE))

#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CPM_ERRORS_H_ */

/** \} */
