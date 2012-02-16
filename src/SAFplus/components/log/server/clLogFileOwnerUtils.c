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
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <clOsalApi.h>
#include <clLogFileOwnerUtils.h>
#include <clLogOsal.h>
#include <clLogSvrCommon.h>

#define  CL_LOG_NAME_LENGTH    2 * CL_MAX_NAME_LENGTH
static ClRcT clLogFileCreationError(ClRcT errorCode, ClCharT *hFileName);

ClRcT
clLogFileNameForStreamAttrGet(ClLogStreamAttrIDLT  *pStreamAttr,
                              ClCharT              **pFileName)
{
    ClRcT  rc = CL_OK;
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileNameGet(&pStreamAttr->fileName,
            &pStreamAttr->fileLocation, pFileName);

    CL_LOG_DEBUG_TRACE(("Exit: fileName: %s", *pFileName));
    return rc;
}

ClRcT
clLogFileOwnerFileNameGet(ClStringT  *fileName,
                           ClStringT  *fileLocation,
                           ClCharT    **pFileName)
{
    ClRcT      rc                          = CL_OK;
    ClUint32T  fileNameLen                 = 0;
    ClCharT    nodeStr[CL_LOG_NAME_LENGTH] = {0};
    ClCharT    filePath[CL_LOG_NAME_LENGTH] = {0};
    ClCharT    logfilepath[CL_LOG_NAME_LENGTH] = {0};
    ClCharT    *aspdir                     = NULL;
    ClUint32T  filePathlen                 = 0;

    sscanf(fileLocation->pValue,"%[^:]:%s", nodeStr, filePath); 
    if( filePath[0] != '/' ) /* path from ASP_INSTALLDIR */
    {
        if ( NULL != (aspdir = getenv("ASP_DIR")) )
        {
            filePathlen = snprintf(logfilepath, CL_LOG_NAME_LENGTH, "%s/%s", aspdir, filePath);
        }
        else 
        {
            ClCharT  cwd[CL_LOG_NAME_LENGTH] = {0};
            clLogWarning(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT,
                    "ASP_DIR is not set, creating files in the current directory");
            if( NULL == getcwd(cwd, CL_LOG_NAME_LENGTH) )
            {
                clLogWarning(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
                   "getting current working directory failed");
            }
            filePathlen = snprintf(logfilepath, CL_LOG_NAME_LENGTH, "%s/%s", cwd, "log"); 
        }
        rc = mkdir(logfilepath, 0755);
        if ((-1 == rc) && (EEXIST != errno))
        {
            clLogCritical(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT,
                    "Unable to create directory [%s] : [%s]", logfilepath,
                    strerror(errno));
            clDbgPause();
            return rc; 
        }
        fileNameLen = fileName->length + filePathlen + 2;
        *pFileName = clHeapCalloc(fileNameLen, sizeof(ClCharT));
        if( NULL == *pFileName )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        sprintf(*pFileName, "%s/%s", logfilepath, fileName->pValue);
        return CL_OK;
    }
    fileNameLen = fileLocation->length + fileName->length + 1; 
    *pFileName = clHeapCalloc(fileNameLen, sizeof(ClCharT));
    if( NULL == *pFileName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    sprintf(*pFileName, "%s/%s", filePath, fileName->pValue);
    return CL_OK;
}

ClRcT
clLogFileOwnerTimeGet(ClCharT   *pStrTime, 
                       ClInt32T  delta)
{
    ClRcT            rc         = CL_OK;
    ClTimerTimeOutT  time       = {0};
    time_t           timeInSec  = delta;
    struct tm        brokenTime = {0};

	CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalTimeOfDayGet(&time);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalTimeOfDayGet(): rc[0x %x]", rc));
        return rc;
    }    
    timeInSec  += time.tsSec + time.tsMilliSec/CL_LOG_MILLISEC_IN_SEC;
    localtime_r(&timeInSec, &brokenTime);
#ifndef VXWORKS_BUILD
    if( 0 == strftime(pStrTime, CL_MAX_NAME_LENGTH , "%Y-%m-%dT%H:%M:%S",
                &brokenTime) )
#else
    if( 0 == strftime(pStrTime, CL_MAX_NAME_LENGTH , "%Y-%m-%dT%H_%M_%S",
                &brokenTime) )
#endif
    {
         CL_LOG_DEBUG_ERROR(("strftime() failed"));
         return CL_OK;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogFileOwnerSoftLinkDelete(ClLogFileKeyT  *pFileKey, 
                              ClUint32T      fileMaxCnt)
{
    ClRcT      rc                                    = CL_OK;
    ClCharT    *pSoftLinkName                        = NULL;
    ClCharT    *pSoftLinkNameLatest                  = NULL;
    ClCharT    *fileName                             = NULL;
    ClUint32T  count                                 = 0;
    ClUint32T  fileCountLen                          = 0;
    ClUint32T  softLinkLen                           = 0;
    ClUint8T   tempVar                               = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileNameGet(&pFileKey->fileName,
                                   &pFileKey->fileLocation, &fileName);
    if( CL_OK != rc )
    {
        return rc;
    }

    fileCountLen = snprintf((ClCharT*)&tempVar, 1, "%u", fileMaxCnt);
    softLinkLen = strlen(fileName) + 1 + fileCountLen + 1;
    
    pSoftLinkName = (ClCharT*) clHeapAllocate(softLinkLen);

    if( NULL == pSoftLinkName )
    {
        clHeapFree(fileName);
        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(); rc[0x %x]", rc));
        return rc;
    }

    snprintf(pSoftLinkName, softLinkLen, "%s.cfg", fileName);

    CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);

    do
    {
        snprintf(pSoftLinkName, softLinkLen, "%s.%u", fileName, count);
        (void)clLogFileUnlink(pSoftLinkName);
    }while(++count < fileMaxCnt);

    pSoftLinkNameLatest = clHeapCalloc(1, strlen(fileName) + strlen(".latest") + 1);
    CL_ASSERT(pSoftLinkNameLatest != NULL);
    sprintf(pSoftLinkNameLatest, "%s.latest", fileName);
    
    CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkNameLatest), CL_OK);

    clHeapFree(fileName);
    clHeapFree(pSoftLinkName);
    clHeapFree(pSoftLinkNameLatest);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerSoftLinkCreate(ClCharT    *fileName,
                             ClCharT    *softLinkName, 
                             ClCharT    *pTimeStr,
                             ClUint32T  fileCount)
{
    ClRcT          rc                                 = CL_OK;
    ClLogFilePtrT  fp                                 = NULL;
    ClCharT        *hFileName                         = NULL;
    ClUint32T       hFileNameLen                      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    hFileNameLen = strlen(fileName) + 1 + strlen(pTimeStr) + 5;
    hFileName = (ClCharT*) clHeapAllocate(hFileNameLen);
    if( NULL == hFileName )
    {
        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(); rc[0x %x]", rc));
        return rc;
    }

    
    if( CL_LOG_FILE_CNT_MAX == fileCount )
    {    
        snprintf(hFileName, hFileNameLen, "%s_%s", fileName, pTimeStr);
        
        rc = clLogFileCreat(hFileName, &fp);
        if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST )
        {
            clHeapFree(hFileName); 
            return rc;
        }
        if( CL_OK != rc )
        {
            clLogFileCreationError(rc, hFileName);
            clHeapFree(hFileName);
            return rc;
        }    
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_CLEANUP(clLogFileUnlink(hFileName), CL_OK);
        snprintf(hFileName, hFileNameLen, "%s_%s.cfg", fileName, pTimeStr);
    }    
    else
    {
        snprintf(hFileName, hFileNameLen, "%s_%s", fileName, pTimeStr);
    }    
    
    rc = clLogFileCreat(hFileName, &fp);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST )
    {
        clHeapFree(hFileName);
        return rc;
    }
    if( CL_OK != rc )
    {
        clLogFileCreationError(rc, hFileName);
        clHeapFree(hFileName);
        return rc;
    }    
    clLogDebug(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
              "New file has been created [%s]", hFileName);

    rc = clLogFileUnlink(softLinkName);
    if( (rc != CL_OK) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)))
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_DEBUG_ERROR(("clLogUnlinkFile(): rc[0x %x]", rc));
        clHeapFree(hFileName);
        return rc;
    }    
    rc = clLogSymLink(hFileName, softLinkName);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_DEBUG_ERROR(("Creating softlink failed"));
        clHeapFree(hFileName);
        return rc;
    }   
    clLogDebug(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
              "Symlink [%s] is created for [%s]", softLinkName, hFileName);

    CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);

    clHeapFree(hFileName);
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT clLogFileCreationError(ClRcT errorCode, ClCharT *hFileName)
{
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    if(CL_ERR_OP_NOT_PERMITTED == CL_GET_ERROR_CODE(errorCode))
    {
        CL_LOG_DEBUG_ERROR(("Failed to create log file: %s"
                    " - Write access to the specified"
                    " log file is not allowed. Please check"
                    " the directory permission. Continuing...",
                    hFileName));
    }
    else if(CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(errorCode))
    {
        CL_LOG_DEBUG_ERROR(("Failed to create log file: %s"
                    " - Directory component in log"
                    " file pathname does not exist. Please create"
                    " the required directories. Continuing...",
                    hFileName));
    }
    else
    {
        CL_LOG_DEBUG_ERROR(("clLogFileOpen(): rc[0x %x]", errorCode));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));

    return CL_OK;
}
