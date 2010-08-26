/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 3.0.1
 */

#include <clLogApi.h>
#include <clHandleApi.h>

static ClLogHandleT  sInitHandle = CL_HANDLE_INVALID_VALUE; 
const ClCharT  *FILELOCATION = ".:var/log";
const ClCharT  *FILESUFFIX   = "Log";
const ClCharT  *APP_PREFIX   = "stream_";

#define  FILESUFFIX_LEN  strlen(FILESUFFIX)

#define CL_LOG_CLNT_VERSION   {'B', 0x01, 0x01}

static void
clEvalAppStreamAttrCopy(ClCharT                 *pAppName, 
                        ClLogStreamAttributesT  *pStreamAttr)
{
    ClUint32T  length = 0;
    
    length = FILESUFFIX_LEN + strlen(pAppName) + 1;
    pStreamAttr->fileName = clHeapAllocate(length);
    if( pStreamAttr->fileName )
    {
        snprintf(pStreamAttr->fileName, length, "%s%s", pAppName, FILESUFFIX);
    }
    length = strlen(FILELOCATION) + 1;
    pStreamAttr->fileLocation = clHeapAllocate(length);
    if( pStreamAttr->fileLocation )
    {
        snprintf(pStreamAttr->fileLocation, length, "%s", FILELOCATION);
    }
    pStreamAttr->fileUnitSize = 150000000L; /*150 MB */
    pStreamAttr->recordSize   = 300;
    pStreamAttr->fileFullAction = CL_LOG_FILE_FULL_ACTION_ROTATE;
    pStreamAttr->maxFilesRotated = 3;
    pStreamAttr->flushFreq       = 64;
    pStreamAttr->flushInterval   = 100000; 

    return ;
}

ClRcT
clEvalAppLogStreamOpen(ClCharT             *pAppName,
                       ClLogStreamHandleT  *pLogAppStream)
{
    ClRcT       rc          = CL_OK;
    ClVersionT  version     = CL_LOG_CLNT_VERSION; 
    ClNameT     streamName  = {0};
    ClLogStreamAttributesT  streamAttributes = {0};
   
   /*
    * Initialize the log service 
    */
    rc = clLogInitialize(&sInitHandle, NULL, &version);
    if( CL_OK != rc )
    {
        return rc;
    }

    streamName.length = snprintf(streamName.value, CL_MAX_NAME_LENGTH, "%s%s", APP_PREFIX, pAppName);

    /*
     * copy the attributes & open the stream  
     */
    clEvalAppStreamAttrCopy(pAppName, &streamAttributes);

    rc = clLogStreamOpen(sInitHandle, streamName, CL_LOG_STREAM_LOCAL, 
                         &streamAttributes, CL_LOG_STREAM_CREATE, 0, pLogAppStream);
    if( CL_OK != rc )
    {
        clLogFinalize(sInitHandle);
        sInitHandle = CL_HANDLE_INVALID_VALUE;
        return rc;
    }
    return rc;
}

ClRcT 
clEvalAppLogStreamClose(ClLogStreamHandleT  appStream)
{
    clLogStreamClose(appStream);
    clLogFinalize(sInitHandle);
    return CL_OK;
}
