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
/******************************************************************************
 * Description :
 * This file populates the log message structure with the
 * message and makes an rmd call to the log server
 *****************************************************************************/
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdarg.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clHandleApi.h>
#include <clCpmExtApi.h>
#include <clParserApi.h>

#include <clLogApi.h>
#include <clLogErrors.h>
#include <clLogCommon.h>
#include <clLogClient.h>
#include <AppServer.h>
#include <AppClient.h>

#define CL_LOG_DEFAULT_SERVICE_ID         10
#define CL_HANDLE_MAX_VALUE               0xFFFFFFFF
#define CL_LOG_MSG_LEN                    1024
#define CL_LOG_FILE_FLUSH_FREQ            10

static ClHandleT  logHandle         = CL_HANDLE_MAX_VALUE;

ClCharT	 *clLogCommonMsg[] =
{
    "%s server fully up", /* 0 */
    "%s server stopped", /* 1 */
    "ASP_CONFIG environment variable is not set", /* 2 */
    "Invalid parameter passed in function [%s]",/* 3 */
    "Index [%d] out of range in function [%s]",/* 4 */
    "Invalid log handle [%llx]", /* 5 */
    "NULL argument passed", /* 6 */
    "Function [%s] is obsolete",/* 7 */
    "XML file [%s] not valid", /* 8 */
    "Requested resource does not exist",/* 9 */
    "The buffer passed is invalid",/* 10 */
    "Duplicate entry",/* 11 */
    "Paramenter passed is out of range",/* 12 */
    "No resources available",/* 13 */
    "Component already initialized",/* 14 */
    "Buffer over run",/* 15 */
    "Component not initialized",/* 16 */
    "Version mismatch",/* 17 */
    "An entry is already existing",/* 18 */
    "Invalid State",/* 19 */
    "Resource is in use",/* 20 */
    "Component [%s] is busy, try again",/* 21 */
    "No callback available for request",/* 22 */
    "Failed to allocate memory", /* 23 */
    "Failed to communicate with [%s], rc=[0x%x]", /* 24 */
    "Host [%s] unreachable, rc=[0x%x]", /* 25 */
    "Service [%s] could not be started, rc=[0x%x]", /* 26 */
    "Library [%s] initialization failed, rc=[0x%x]", /* 27 */
    "Reading checkpoint data failed, rc=[0x%x]", /* 28 */
    "Unable to write Checkpoint dataset, rc=[0x%x]", /* 29 */
    "Container creation failed, rc=[0x%x]", /* 30 */
    "Container data get failed, rc=[0x%x]", /* 31 */
    "Container data addition failed, rc=[0x%x]", /* 32 */
    "Container data delete failed, rc=[0x%x]", /* 33 */
    "Unable to get eo object, rc=[0x%x]", /* 34 */
    "Opening event channel [%s] failed, rc=[0x%x]", /* 35 */
    "Event subsription failed, rc=[0x%x]", /* 36 */
    "ASP_BINDIR environment variable not set", /* 37 */
    "Container node find failed, rc=[0x%x]", /* 38 */
    "Container node get failed, rc=[0x%x]", /* 39 */
    "Container node add failed, rc=[0x%x]", /* 40 */
    "Container walk failed, rc=[0x%x]", /* 42 */
    "Container delete failed, rc=[0x%x]", /* 43 */
    "Cor event subscribe failed, rc=[0x%x]", /* 41 */
    "Cor object handle get failed, rc=[0x%x]", /* 44 */
    "Cor attribute type get failed, rc=[0x%x]", /* 45 */
    "Cor MOID to class get failed, rc=[0x%x]", /* 46 */
    "Cor notify event to moid get failed, rc=[0x%x]", /* 47 */
    "Event cookie get failed, rc=[0x%x]", /* 48 */
    "Cor event to containment attribute path get failed, rc=[0x%x]", /* 49 */
    "Cor object attribute get failed, rc=[0x%x]", /* 50 */
    "Container node delete failed, rc=[0x%x]",/* 51 */
    "Cor object handle to moid get failed, rc=[0x%x]", /* 52 */
    "Cor transaction commit failed, rc=[0x%x]", /* 53 */
    "Function [%s] not implemented", /* 54 */
    "Timeout", /* 55 */
    "Handle creation failed, rc=[0x%x]", /* 56 */
    "Handle database creation failed, rc=[0x%x]", /* 57 */
    "Handle database destroy failed, rc=[0x%x]", /* 58 */
    "Failed to get handle for component [%s], rc=[0x%x]", /* 59 */
    "Handle checkin failed, rc=[0x%x]", /* 60 */
    "Handle checkout failed, rc=[0x%x]", /* 61 */
    "Message creation failed, rc=[0x%x]", /* 62 */
    "Message read failed, rc=[0x%x]", /* 63 */
    "Message write failed, rc=[0x%x]", /* 64 */
    "Message deletion failed, rc=[0x%x]", /* 65 */
    "Checkpoint creation failed, rc=[0x%x]", /* 66 */
    "Checkpoint dataset creation failed, rc=[0x%x]", /* 67 */
    "Mutex creation failed, rc=[0x%x]", /* 68 */
    "Mutex could not be locked, rc=[0x%x]", /* 69 */
    "Mutex could not be unlocked, rc=[0x%x]", /* 70 */
    "Rmd call failed, rc=[0x%x]", /* 71 */
    "Installation of function table for client failed, rc=[0x%x]", /* 72 */
    "User callout function deletion failed, rc=[0x%x]", /* 73 */
    "Registration with debug failed, rc=[0x%x]", /* 74 */
    "%s NACK received from Node - Supported Version {'%c', 0x%x, 0x%x}",
    NULL };

ClRcT
clLogLibInitialize(void)
{
    ClRcT       rc      = CL_OK;
    ClVersionT  version = {0};

#if defined(CL_DEBUG) 
    clLogDebugLevelSet();
#endif

#ifdef VXWORKS_BUILD
    return CL_OK;
#endif

    version.releaseCode  ='B';
    version.majorVersion = 1;
    version.minorVersion = 1;
    if( CL_HANDLE_MAX_VALUE == logHandle )
    {
        rc = clLogInitialize(&logHandle, NULL, &version);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogInitialize(): rc[0x %x]", rc));
            return rc;
        }

        CL_LOG_HANDLE_APP = stdStreamList[0].hStream;
        CL_LOG_HANDLE_SYS = stdStreamList[1].hStream;
    }
    return CL_OK;
}

ClRcT
clLogLibFinalize(void)
{
    ClRcT  rc = CL_OK;

#ifdef VXWORKS_BUILD
    return rc;
#endif

    if( CL_HANDLE_MAX_VALUE == logHandle )
    {
        CL_LOG_DEBUG_ERROR(("Library has not been initialized"));
        return CL_LOG_RC(CL_ERR_NOT_INITIALIZED);
    }

    //CL_LOG_CLEANUP(clLogFinalize(logHandle), CL_OK);

    logHandle = CL_HANDLE_MAX_VALUE;

    return rc;
}

ClRcT
clLogOpen(ClLogStreamHandleT      *pStreamHandle,
          // coverity[pass_by_value]
          ClLogFormatFileConfigT  fileConfig)
{
    CL_LOG_DEBUG_ERROR(("This API is not Supported"));
    return CL_LOG_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT
clLogClose(ClLogStreamHdlT  handle)
{
    CL_LOG_DEBUG_ERROR(("This API is not Supported"));
    return CL_LOG_RC(CL_ERR_NOT_IMPLEMENTED);
}
/**
 *Sets the severity level for default log streams -
 * app and sys streams
 */
ClRcT
clLogLevelSet(ClLogSeverityT  severity)
{
    ClRcT         rc     = CL_OK;
    ClLogFilterT  filter = {0, 
                            0, NULL, 0, NULL };
    ClLogSeverityFilterT sevIdx = 0;
    
    for(sevIdx = 0; sevIdx < severity; sevIdx++)
    {
        filter.severityFilter |= 1 << sevIdx;
    }

    rc = clLogFilterSet(CL_LOG_HANDLE_APP,      
                        CL_LOG_FILTER_ASSIGN,  
                        filter);              
    if(CL_OK != rc)                          
    {                                       
        return rc;                         
    }                                     

    rc = clLogFilterSet(CL_LOG_HANDLE_SYS,
                        CL_LOG_FILTER_ASSIGN,
                        filter);            
    if(CL_OK != rc)                        
    {                                     
        return rc;                       
    }                                   
    return rc;
}

ClRcT
clLogLevelGet(ClLogSeverityT  *severity)
{
    ClRcT   rc   = CL_OK;
    ClLogSeverityFilterT sevFilter = 0;
    ClLogSeverityFilterT sevIdx = 0;
    
    if( NULL == severity ) 
    {
        clLogError("LOG", CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Passed variable [severity] is NULL");
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }
    rc = clLogSeverityFilterGet(CL_LOG_HANDLE_SYS, &sevFilter);
    if(CL_OK != rc)                        
    {                                     
        return rc;                       
    }
    
    for(sevIdx = 0; sevIdx < CL_LOG_SEV_TRACE; sevIdx++)
    {
        if(!(sevFilter & (1 << sevIdx)))
            break;
    }

    *severity = (ClLogSeverityT)(sevIdx);
    
    return rc;
}

ClRcT clLogSeverityFilterToValueGet(ClLogSeverityFilterT filter, ClLogSeverityT* pSeverity)
{
    ClLogSeverityT severity = 0;
    if(!pSeverity) 
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    severity = (ClLogSeverityT)clBinaryPower((ClUint32T)filter);
    if(severity > CL_LOG_SEV_TRACE) 
        severity = CL_LOG_SEV_TRACE;
    *pSeverity = severity;
    return CL_OK;
}

ClRcT clLogSeverityValueToFilterGet(ClLogSeverityT severity, ClLogSeverityFilterT* pFilter)
{
    if (!pFilter)
    {
        clLogError("", "", "NULL value passed in pFilter.");
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }

    *pFilter = (1<<severity)-1;

    return CL_OK;
}

ClRcT
clLogSystemLevelSet(ClLogSeverityT	 severity,
		    ClIocNodeAddressT	 nodeAddress,
		    ClLogFilterPatternT	 pattern)
{
    CL_LOG_DEBUG_ERROR(("This API is not Supported"));
    return CL_LOG_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT
clLogVersionVerify(ClVersionT  *pVersion)
{
    CL_LOG_DEBUG_ERROR(("This API is not Supported"));
    return CL_LOG_RC(CL_ERR_NOT_IMPLEMENTED);
}
