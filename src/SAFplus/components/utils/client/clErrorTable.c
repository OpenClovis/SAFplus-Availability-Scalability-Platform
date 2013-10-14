#define _CL_ERROR_TABLE_
#include <clErrorApi.h>
#include <clLogApi.h>
#include "clErrorTable.h"

SaAisErrorT clErrorToSaf(ClRcT clError)
{
    switch (CL_GET_ERROR_CODE(clError))
    {
        case CL_OK:
            return SA_AIS_OK; /* 1 */

        case CL_ERR_LIBRARY:
            return SA_AIS_ERR_LIBRARY; /* 2 */

        case CL_ERR_VERSION_MISMATCH:
            return SA_AIS_ERR_VERSION; /* 3 */

        case CL_ERR_NOT_INITIALIZED:
        case CL_ERR_INITIALIZED:
        case CL_ERR_NO_CALLBACK:
            return SA_AIS_ERR_INIT; /* 4 */

        case CL_ERR_TIMEOUT:
            return SA_AIS_ERR_TIMEOUT; /* 5 */

        case CL_ERR_TRY_AGAIN:
            return SA_AIS_ERR_TRY_AGAIN; /* 6 */

        case CL_ERR_INVALID_PARAMETER:
        case CL_ERR_NULL_POINTER:      
            return SA_AIS_ERR_INVALID_PARAM; /* 7 */

        case CL_ERR_NO_MEMORY:
            return SA_AIS_ERR_NO_MEMORY; /* 8 */

        case CL_ERR_INVALID_HANDLE:
            return SA_AIS_ERR_BAD_HANDLE; /* 9 */

        case CL_ERR_INUSE:
            return SA_AIS_ERR_BUSY; /* 10 */

        case CL_ERR_OP_NOT_PERMITTED:
            return SA_AIS_ERR_ACCESS; /* 11 */

        case CL_ERR_NOT_EXIST:
        case CL_ERR_DOESNT_EXIST:
            return SA_AIS_ERR_NOT_EXIST; /* 12 */

        case CL_ERR_NAME_TOO_LONG:
            return SA_AIS_ERR_NAME_TOO_LONG; /* 13 */

        case CL_ERR_DUPLICATE:
        case CL_ERR_ALREADY_EXIST:
            return SA_AIS_ERR_EXIST; /* 14 */

        case CL_ERR_NO_SPACE:
            return SA_AIS_ERR_NO_SPACE; /* 15 */

        case CL_ERR_INTERRUPT:
            return SA_AIS_ERR_INTERRUPT; /* 16 */

        case CL_ERR_NAME_NOT_FOUND:
            return SA_AIS_ERR_NAME_NOT_FOUND; /* 17 */

        case CL_ERR_NO_RESOURCE:
            return SA_AIS_ERR_NO_RESOURCES; /* 18 */

        case CL_ERR_NOT_IMPLEMENTED:
            return SA_AIS_ERR_NOT_SUPPORTED; /* 19 */

        case CL_ERR_BAD_OPERATION:
            return SA_AIS_ERR_BAD_OPERATION; /* 20 */

        case CL_ERR_FAILED_OPERATION:
            return SA_AIS_ERR_FAILED_OPERATION; /* 21 */

        case CL_ERR_MESSAGE_ERROR: 
            return SA_AIS_ERR_MESSAGE_ERROR; /* 22 */

        case CL_ERR_BUFFER_OVERRUN :
            return SA_AIS_ERR_QUEUE_FULL; /* 23 */

        case CL_ERR_QUEUE_NOT_AVAILABLE:
            return SA_AIS_ERR_QUEUE_NOT_AVAILABLE; /* 24 */

        case CL_ERR_BAD_FLAG:
            return SA_AIS_ERR_BAD_FLAGS; /* 25 */

        case CL_ERR_TOO_BIG:
            return SA_AIS_ERR_TOO_BIG; /* 26 */

        case CL_ERR_NO_SECTIONS:
            return SA_AIS_ERR_NO_SECTIONS; /* 27 */

        default :
            clLogError("ASP", "ERR", "ASP error code [0x%x]. Returning [SA_AIS_ERR_FAILED_OPERATION].", clError);
            return SA_AIS_ERR_FAILED_OPERATION; /* 21 */
    }

    return SA_AIS_ERR_FAILED_OPERATION;
}

void clErrorCodeToString(ClRcT errorCode, ClCharT *buf, ClUint32T maxBufSize)
{
    ClUint32T compId;
    ClUint32T errNo;
    
    if(buf == NULL || !maxBufSize)
    {
        clLogError("UTL", "ERR", "Invalid buffer %s", maxBufSize ? "" :"size");
        return;
    }

    memset(buf, 0, maxBufSize);
    compId = CL_GET_CID(errorCode);
    errNo = CL_GET_ERROR_CODE(errorCode);

    if(compId == 0)
    {
        /* SAF errors. */
        if(errNo < sizeof(gpSafErrs)/sizeof(gpSafErrs[0]))
            strncpy(buf, gpSafErrs[errNo], maxBufSize-1);
        else
            clLogError("UTL", "ERR", "[0x%x] is not in SAF error-codes' range.", errNo);
    }
    else if(compId < sizeof(gClCompErrs)/sizeof(gClCompErrs[0]))
    {
        /* OpenClovis errors. */
        if(errNo < CL_COMP_SPECIFIC_ERROR_CODE_START)
        {
            /* Common errors. */
            if(errNo < sizeof(gpClCommonErrs)/sizeof(gpClCommonErrs[0]))
                snprintf(buf, maxBufSize, "%s(%s)", gClCompErrs[compId].pCompId, gpClCommonErrs[errNo]);
            else
                clLogError("UTL", "ERR", "Error code [0x%x] from error [0x%x] is not in common error-codes' range.", errNo, errorCode);
        }
        else
        {
            /* Component specific errors. */
            if(gClCompErrs[compId].ppCompErrs)
                snprintf(buf, maxBufSize, "%s(%s)", gClCompErrs[compId].pCompId, gClCompErrs[compId].ppCompErrs[errNo - CL_COMP_SPECIFIC_ERROR_CODE_START]);
            else
                clLogError("UTL", "ERR", "Dont have Error-to-String table yet.");
        }
    }
    else
    {
        clLogError("UTL", "ERR", "No component with ID [0x%x]. Please check the passed error-code(i.e. [0x%x]).", compId, errorCode);
    }

    return;
}

const ClCharT *clErrorToString(ClRcT errorCode)
{
    ClUint32T compId;
    ClUint32T errNo;

    compId = CL_GET_CID(errorCode);
    errNo = CL_GET_ERROR_CODE(errorCode);
    if(compId == 0)
    {
        /* SAF errors. */
        if(errNo < sizeof(gpSafErrs)/sizeof(gpSafErrs[0]))
        {
            return gpSafErrs[errNo];
        }
        else 
        {
            clLogError("UTL", "ERR", "[0x%x] is not in SAF error-codes' range.", errNo);
            return CL_INVALID_ERROR_CODE;
        }
    }
    else
    {
        if(errNo < CL_COMP_SPECIFIC_ERROR_CODE_START)
        {
            if(errNo < sizeof(gpClCommonErrs)/sizeof(gpClCommonErrs[0]))
            {
                return gpClCommonErrs[errNo];
            }
            else 
            {
                clLogError("UTL", "ERR", "Error code [0x%x] from error [0x%x] is not in common error-codes' range.", errNo, errorCode);
                return CL_INVALID_ERROR_CODE;
            }
        }
        else
        {
            clLogError("UTL", "ERR", "Dont have Error-to-String table yet.");
            return CL_INVALID_ERROR_CODE;
        }
    }
    return NULL;
}
