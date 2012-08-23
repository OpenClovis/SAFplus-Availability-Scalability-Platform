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
 * ModuleName  : include
 * File        : clCommonErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains common error codes shared across
 * multiple Clovis ASP components, and macros to be used
 * do handle error codes.
 *
 * Retrun codes are forumated combining a 16-bit component ID (CID) and a
 * 16-bit error code.  Common codes are listed above; component-specific
 * error codes are defined in the respective components Errors.h file.
 * The first 256 error codes are reserved for common errors, the remaining
 * range (256-65535) are available for each component.  The CIDs are defined
 * in clCommon.h.
 *
 *
 *****************************************************************************/

/**
 * \file
 * \brief Common Error Codes shared across multiple Clovis ASP Components
 * \ingroup common_apis
 *
 */

/**
 * \addtogroup common_apis
 * \{
 */

#ifndef _CL_COMMON_ERRORS_H_
#define _CL_COMMON_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/******************************************************************************
 * COMMON ERROR CODES
 *****************************************************************************/

/** 
 * Every thing is OK
 */
#define CL_OK                    0x00  

/** 
 * Memory is not available
 */
#define CL_ERR_NO_MEMORY         0x01  

/** 
 * Input parameters are invalid
 */
#define CL_ERR_INVALID_PARAMETER 0x02  

/** 
 * Input parameter is a NULL pointer
 */
#define CL_ERR_NULL_POINTER      0x03 

/** 
 * Requested resource does not exist
 */
#define CL_ERR_NOT_EXIST         0x04 

/** 
 * The handle passed is invalid
 */
#define CL_ERR_INVALID_HANDLE    0x05 

/** 
 * The buffer passed in is invalid
 */
#define CL_ERR_INVALID_BUFFER    0x06 

/** 
 * The function not yet implemented
 */
#define CL_ERR_NOT_IMPLEMENTED   0x07 

/** 
 * Duplicate entry
 */
#define CL_ERR_DUPLICATE         0x08 

/**
 * The destination queue is not available.
 */
#define CL_ERR_QUEUE_NOT_AVAILABLE 0x09

/** 
 * Out of range paramenters
 */
#define CL_ERR_OUT_OF_RANGE      0x0a  

/** 
 * No resources
 */
#define CL_ERR_NO_RESOURCE       0x0b 

/** 
 * Already initialized
 */
#define CL_ERR_INITIALIZED       0x0c  

/**
 * Buffer over run
 */
#define CL_ERR_BUFFER_OVERRUN    0x0d  

/** 
 * Component not initialized
 */
#define CL_ERR_NOT_INITIALIZED   0x0e 

/** 
 * Version mismatch
 */
#define CL_ERR_VERSION_MISMATCH  0x0f  

/** 
 * An entry is already existing
 */
#define CL_ERR_ALREADY_EXIST     0x10  

/** 
 * Unknown/Unspecified error 
 */
#define CL_ERR_UNSPECIFIED       0x11  

/** 
 * Invalid State
 */
#define CL_ERR_INVALID_STATE     0x12  

/** 
 * An entry does not exist
 */
#define CL_ERR_DOESNT_EXIST      0x13  

/** 
 * Timeout 
 */
#define CL_ERR_TIMEOUT           0x14  

/** 
 * Resource is in use
 */
#define CL_ERR_INUSE             0x15  

/** 
 * Component is busy , Try again 
 */
#define CL_ERR_TRY_AGAIN         0x16 

/** 
 * No callback available for request
 */
#define CL_ERR_NO_CALLBACK       0x17 

/** 
 * Thread mutex error
 */
#define CL_ERR_MUTEX_ERROR       0x18 

/** 
 * Null operation
 */
#define CL_ERR_NO_OP             0x19  

/**
 * Name exceeds maximum allowed length.
 */
#define CL_ERR_NAME_TOO_LONG     0x1a

/**
 * Name doesnt exist or cannot be found.
 */
#define CL_ERR_NAME_NOT_FOUND    0x1b

/**
 * A communication error occurred.
 */
#define CL_ERR_MESSAGE_ERROR     0x1c

/**
 * A value is larger than the maximum value permitted.
 */
#define CL_ERR_TOO_BIG           0x1d

/**
 * There are no or no more sections matching the specified sections.
 */
#define CL_ERR_NO_SECTIONS       0x1e

/**
 * The requested operation Failed.
 */
#define CL_ERR_FAILED_OPERATION  0x1f

/** 
 * Requested operation is not permitted
 */
#define CL_ERR_OP_NOT_PERMITTED  0x20  

/** 
 * Space limitation
 */
#define CL_ERR_NO_SPACE          0x21  

/**  
 * The passed flag is invalid 
 */
#define CL_ERR_BAD_FLAG          0x22  

/** 
 * The requested operation is invalid
 */
#define CL_ERR_BAD_OPERATION     0x23  

/** 
 * System call invocation failed and returned an error 
 */
#define CL_ERR_LIBRARY           0x24  

/** 
 * Requested feature is not supported
 */
#define CL_ERR_NOT_SUPPORTED     0x25  

/**
 * The operation interrupted by application/user.
 */
#define CL_ERR_INTERRUPT         0x26

#define CL_ERR_CONTINUE          0x27

/** 
 * 2^8-1, the max for common errors
 */
#define CL_ERR_COMMON_MAX        0xff 

/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/
/*
 * The following macros assist in constructing return codes,
 * and in extracting the CID or the error code from the return code.
 */

/** 
 * Component identifier offset
 */
#define CL_CID_OFFSET           16      /* 16 bit positions */

/** 
 * Error code mask 
 */
#define CL_ERROR_CODE_MASK      0xffff  /* 16 bits */

/** 
 * This macro constructs the return code from the component identifier 
 * and error code
 */
#define CL_RC(CID, ERROR_CODE)  ((ClUint32T) \
                                (((ERROR_CODE) == CL_OK) ? \
                                (ERROR_CODE) : \
                                ((ClUint32T) (((CID) << CL_CID_OFFSET) | \
                                ((ERROR_CODE) & CL_ERROR_CODE_MASK)))))
/** 
 * This macro extracts the error code from the return code
 */
#define CL_GET_ERROR_CODE(RC)   ((ClUint32T) ((RC) & CL_ERROR_CODE_MASK))

/** 
 * This macro extracts the component identifier from the return code
 */
#define CL_GET_CID(RC)          ((ClUint32T) ((RC) >> CL_CID_OFFSET))

#ifdef __cplusplus
}
#endif

/**
 * \}
 */

#endif /* _CL_COMMON_ERRORS_H_ */
