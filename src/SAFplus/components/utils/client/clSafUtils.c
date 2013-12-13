#include <clSafUtils.h>
#include <clCpmErrors.h>

SaAisErrorT clClovisToSafError(ClRcT clError)
{
    SaAisErrorT safError = SA_AIS_ERR_LIBRARY;

    /* First check component specific error translations */
    switch(clError)
    {
        case CL_CPM_RC(CL_CPM_ERR_OPERATION_IN_PROGRESS):
            safError = SA_AIS_ERR_BUSY;
            break;
    }

    if (safError != SA_AIS_ERR_LIBRARY) return safError;
    
    /* Next check to see if the error can be translated generically */    
    switch(CL_GET_ERROR_CODE(clError))
    {
        case CL_OK:
        {
            safError = SA_AIS_OK;
            break;
        }
        case CL_ERR_VERSION_MISMATCH:
        {
            safError = SA_AIS_ERR_VERSION;
            break;
        }
        case CL_ERR_TIMEOUT:
        {
            safError = SA_AIS_ERR_TIMEOUT;
            break;
        }
        case CL_ERR_TRY_AGAIN:
        {
            safError = SA_AIS_ERR_TRY_AGAIN;
            break;
        }
        case CL_ERR_INVALID_PARAMETER:
        {
            safError = SA_AIS_ERR_INVALID_PARAM;
            break;
        }
        case CL_ERR_NO_MEMORY:
        {
            safError = SA_AIS_ERR_NO_MEMORY;
            break;
        }
        case CL_ERR_INVALID_HANDLE:
        {
            safError = SA_AIS_ERR_BAD_HANDLE;
            break;
        }
        case CL_ERR_OP_NOT_PERMITTED:
        {
            safError = SA_AIS_ERR_ACCESS;
            break;
        }
        case CL_ERR_DOESNT_EXIST:
        {
            safError = SA_AIS_ERR_NOT_EXIST;
            break;
        }
        case CL_ERR_ALREADY_EXIST:
        {
            safError = SA_AIS_ERR_EXIST;
            break;
        }
        case CL_ERR_NO_SPACE:
        {
            safError = SA_AIS_ERR_NO_SPACE;
            break;
        }
        case CL_ERR_NO_RESOURCE:
        {
            safError = SA_AIS_ERR_NO_RESOURCES;
            break;
        }
        case CL_ERR_NOT_IMPLEMENTED:
        {
            safError = SA_AIS_ERR_NOT_SUPPORTED;
            break;
        }
        case CL_ERR_BAD_OPERATION:
        {
            safError = SA_AIS_ERR_BAD_OPERATION;
            break;
        }
        case CL_ERR_BAD_FLAG:
        {
            safError = SA_AIS_ERR_BAD_FLAGS;
            break;
        }
        case CL_ERR_NULL_POINTER:
        {
            safError = SA_AIS_ERR_INVALID_PARAM;
            break;
        }
        default:
        {
            clLogWarning("AMF", "SAF",
                         "Clovis error code [%#x] is not mapped to a valid "
                         "SAF error code, returning default [%#x]",
                         CL_GET_ERROR_CODE(clError),
                         (ClUint32T)safError);
            break;
        }
    }

    return safError;
}

ClRcT clSafToClovisError(SaAisErrorT safError)
{
    ClRcT clError = CL_ERR_UNSPECIFIED;
    
    switch (safError)
    {
        case SA_AIS_OK:
        {
            clError = CL_OK;
            break;
        }
        case SA_AIS_ERR_LIBRARY:
        {
            clError = CL_ERR_INITIALIZED;
            break;
        }
        case SA_AIS_ERR_VERSION:
        {
            clError = CL_ERR_VERSION_MISMATCH;
            break;
        }
        case SA_AIS_ERR_INIT:
        {
            clError = CL_ERR_NOT_INITIALIZED;
            break;
        }
        case SA_AIS_ERR_TIMEOUT:
        {
            clError = CL_ERR_TIMEOUT;
            break;
        }
        case SA_AIS_ERR_TRY_AGAIN:
        {
            clError = CL_ERR_TRY_AGAIN;
            break;
        }
        case SA_AIS_ERR_INVALID_PARAM:
        {
            clError = CL_ERR_INVALID_PARAMETER;
            break;
        }
        case SA_AIS_ERR_NO_MEMORY:
        {
            clError = CL_ERR_NO_MEMORY;
            break;
        }
        case SA_AIS_ERR_BAD_HANDLE:
        {
            clError = CL_ERR_INVALID_HANDLE;
            break;
        }
        case SA_AIS_ERR_BUSY:
        {
            clError = CL_ERR_TRY_AGAIN;
            break;
        }
        case SA_AIS_ERR_ACCESS:
        {
            clError = CL_ERR_OP_NOT_PERMITTED;
            break;
        }
        case SA_AIS_ERR_NOT_EXIST:
        {
            clError = CL_ERR_DOESNT_EXIST;
            break;
        }
        case SA_AIS_ERR_NAME_TOO_LONG:
        {
            clError = CL_ERR_OUT_OF_RANGE;
            break;
        }
        case SA_AIS_ERR_EXIST:
        {
            clError = CL_ERR_ALREADY_EXIST;
            break;
        }
        case SA_AIS_ERR_NO_SPACE:
        {
            clError = CL_ERR_NO_SPACE;
            break;
        }
        case SA_AIS_ERR_INTERRUPT:
        {
            clError = CL_ERR_UNSPECIFIED;
            break;
        }
        case SA_AIS_ERR_NAME_NOT_FOUND:
        {
            clError = CL_ERR_DOESNT_EXIST;
            break;
        }
        case SA_AIS_ERR_NO_RESOURCES:
        {
            clError = CL_ERR_NO_RESOURCE;
            break;
        }
        case SA_AIS_ERR_NOT_SUPPORTED:
        {
            clError = CL_ERR_NOT_IMPLEMENTED;
            break;
        }
        case SA_AIS_ERR_BAD_OPERATION:
        {
            clError = CL_ERR_BAD_OPERATION;
            break;
        }
        case SA_AIS_ERR_FAILED_OPERATION:
        {
            clError = CL_ERR_BAD_OPERATION;
            break;
        }
        case SA_AIS_ERR_BAD_FLAGS:
        {
            clError = CL_ERR_BAD_FLAG;
            break;
        }
        case SA_AIS_ERR_TOO_BIG:
        {
            clError = CL_ERR_OUT_OF_RANGE;
            break;
        }
        default:
        {
            clLogWarning("AMF", "SAF",
                         "SAF error code [%#x] is not mapped to a valid "
                         "Clovis error code, returning default [%#x]",
                         (ClUint32T)safError,
                         clError);
            break;
        }
    }

    return clError;
}
