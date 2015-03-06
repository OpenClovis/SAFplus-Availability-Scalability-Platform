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
 * ModuleName  : fault
 * File        : clFaultErrorId.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains fault service related error codes
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Fault Service related Error Codes
 *  \ingroup fault_apis
 */

/**
 *  \addtogroup fault_apis
 *  \{
 */

#ifndef _CL_FAULT_ERROR_ID_H_
#define _CL_FAULT_ERROR_ID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/
/* fault service related error codes */

/**
 * The fault occured is duplicate. It is a carbon copy
 * of a fault that has occured earlier.
 */
#define CL_FAULT_ERR_DUPLICATE                      0x100

/**
 * The fault is not found.
 */
#define CL_FAULT_ERR_FAULT_NOT_FOUND                0x101

/**
 * The fault is invalid.
 */
#define CL_FAULT_ERR_INVLD_VAL                      0x102

/**
 * The fault has exceeded the severity level.
 */
#define CL_FAULT_ERR_SEVERITY_EXCEED                0x103

/**
 * The fault is not being added to the history.
 */
#define CL_FAULT_ERR_HISTORY_ADD_ERROR              0x104

/**
 * The fault is not being created in the history.
 */
#define CL_FAULT_ERR_HISTORY_CREATE_ERROR           0x105

/**
 * A violation of shared data occurs when a fault is being created in the
 * history.
 */
#define CL_FAULT_ERR_HISTORY_MUTEX_CREATE_ERROR     0x106

/**
 * The fault is not being queried from the history.
 */
#define CL_FAULT_ERR_HISTORY_QUERY_ERROR            0x107

/**
 * The fault is not created.
 */
#define CL_FAULT_ERR_CREATE_FAILED                  0x108

/**
 * The fault is not added.
 */
#define CL_FAULT_ERR_ADD_FAILED                     0x109

/**
 * The fault handler is not found.
 */
#define CL_FAULT_ERR_FAULT_HANDLER_NOT_FOUND        0x10a

/**
 * The fault sequence table is NULL.
 */
#define CL_FAULT_ERR_REPAIR_SEQ_TBL_NULL            0x10b

/**
 * Fault Internal Error.
 */
#define CL_FAULT_ERR_INTERNAL                       0x10c

/**
 * The fault category is invalid.
 */
#define CL_FAULT_ERR_INVALID_CATEGORY               0x10d

/**
 * The fault severity is invalid.
 */
#define CL_FAULT_ERR_INVALID_SEVERITY               0x10e

/**
 * MOID of the fault is NULL.
 */
#define CL_FAULT_ERR_MOID_NULL                      0x10f

/**
 * Fault client library initialize failure.
 */
#define CL_FAULT_ERR_CLIENT_INIT_FAILED             0x110

/**
 * Fault client library finalize failure.
 */
#define CL_FAULT_ERR_CLIENT_FINALIZE_FAILED         0x111

/**
 * Component name passed to fault is NULL.
 */
#define CL_FAULT_ERR_COMPNAME_NULL                  0x112

/**
 *  On version mismatch.
 */
#define CL_FAULT_ERR_VERSION_UNSUPPORTED            0x113

/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/
/**
 *  This is useful while comparing error codes returned by two or more components.
 *  The individual component IDs can be extracted if the error returned by them
 *  is the same.
 */
#define CL_FAULT_RC(ERROR_CODE)  CL_RC(CL_CID_FAULTS, (ERROR_CODE))

#ifdef __cplusplus
}
#endif

#endif  /*   _CL_FAULT_ERROR_ID_H_ */


/** \} */

