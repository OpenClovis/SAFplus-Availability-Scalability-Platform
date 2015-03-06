/*******************************************************************************
 * ModuleName  : include
 * $File: //depot/dev/main/Andromeda/Yamuna/ASP/components/timer/newTimerPrototype/clCommonErrors.h $
 * $Author: karthick $
 * $Date: 2007/01/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains common error codes shared across
 * multiple Clovis SISP components, and macros to be used
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

#ifndef _CL_COMMON_ERRORS_H_
#define _CL_COMMON_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clTimerTestCommon.h"

/******************************************************************************
 * COMMON ERROR CODES
 *****************************************************************************/
#define CL_OK                    0x00  /* Every thing is OK */
#define CL_ERR_NO_MEMORY         0x01  /* Memory is not available */
#define CL_ERR_INVALID_PARAMETER 0x02  /* Input parameters are invalid */
#define CL_ERR_NULL_POINTER      0x03  /* Input parameter is a NULL pointer */
#define CL_ERR_NOT_EXIST         0x04  /* Requested resource does not exist */
#define CL_ERR_INVALID_HANDLE    0x05  /* The handle passed is invalid */
#define CL_ERR_INVALID_BUFFER    0x06  /* The buffer passed in is invalid */
#define CL_ERR_NOT_IMPLEMENTED   0x07  /* The function not yet implemented */
#define CL_ERR_DUPLICATE         0x08  /* Duplicate entry */
#define CL_ERR_OUT_OF_RANGE      0x0a  /* Out of range paramenters */
#define CL_ERR_NO_RESOURCE       0x0b  /* No resources */
#define CL_ERR_INITIALIZED       0x0c  /* Already initialized */
#define CL_ERR_BUFFER_OVERRUN    0x0d  /* Buffer over run */
#define CL_ERR_NOT_INITIALIZED   0x0e  /* Component not initialized */
#define CL_ERR_VERSION_MISMATCH  0x0f  /* Version mismatch */
#define CL_ERR_ALREADY_EXIST     0x10  /* An entry is already existing */
#define CL_ERR_UNSPECIFIED       0x11  /* Unknown/Unspecified error */
#define CL_ERR_INVALID_STATE     0x12  /* Invalid State */
#define CL_ERR_DOESNT_EXIST      0x13  /* An entry does not exist */
#define CL_ERR_TIMEOUT           0x14  /* Timeout */
#define CL_ERR_INUSE             0x15  /* Resource is in use */
#define CL_ERR_TRY_AGAIN         0x16  /* Component is busy , Try again */
#define CL_ERR_NO_CALLBACK       0x17  /* No callback available for request */
#define CL_ERR_MUTEX_ERROR       0x18  /* Thread mutex error */
#define CL_ERR_NO_OP             0x19  /* Null operation */
#define CL_ERR_OP_NOT_PERMITTED  0x20  /* Requested operation is not permitted*/
#define CL_ERR_NO_SPACE          0x21  /* Sapce limitation */
#define CL_ERR_BAD_FLAG          0x22  /* The passed flag is invalid */
#define CL_ERR_BAD_OPERATION     0x23  /* The requested operation is invalid */
#define CL_ERR_LIBRARY           0x24  /* System call invocation failed and returned an error */
#define CL_ERR_COMMON_MAX        0xff  /* 2^8-1, the max for common errors */

/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/
/*
 * The following macros assist in constructing return codes,
 * and in extracting the CID or the error code from the return code.
 */
#define CL_CID_OFFSET           16      /* 16 bit positions */
#define CL_ERROR_CODE_MASK      0xffff  /* 16 bits */

#define CL_RC(CID, ERROR_CODE)  (((ERROR_CODE) == CL_OK) ? \
                                (ERROR_CODE) : \
                                ((ClUint32T) (((CID) << CL_CID_OFFSET) | \
                                ((ERROR_CODE) & CL_ERROR_CODE_MASK))))

#define CL_GET_ERROR_CODE(RC)   ((ClUint32T) ((RC) & CL_ERROR_CODE_MASK))
#define CL_GET_CID(RC)          ((ClUint32T) ((RC) >> CL_CID_OFFSET))

#ifdef __cplusplus
}
#endif

#endif /* _CL_COMMON_ERRORS_H_ */
