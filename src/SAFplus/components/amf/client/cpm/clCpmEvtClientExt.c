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
 * This file implements CPM client side functionality related to
 * event.
 */

/*
 * Standard header files 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * ASP header files 
 */
#include <clEoApi.h>
#include <clEventApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>

/*
 * CPM internal header files 
 */
#include <clCpmClient.h>
#include <clCpmCommon.h>
#include <clCpmLog.h>

/*
 * XDR header files 
 */
#include "xdrClCpmEventPayLoadT.h"
#include "xdrClCpmEventNodePayLoadT.h"

ClRcT clCpmEventPayLoadExtract(ClEventHandleT eventHandle,
                               ClSizeT eventDataSize, 
                               ClCpmEventTypeT cpmEventType,
                               void *payLoad)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT payLoadMsg = 0;
    void *eventData = NULL;

    eventData = clHeapAllocate(eventDataSize);
    if (eventData == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Unable to malloc \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    }
    rc = clEventDataGet (eventHandle, eventData,  &eventDataSize);
    if (rc != CL_OK)
    {
        clOsalPrintf("Event Data Get failed. rc=[0x%x]\n", rc);
        goto failure;
    }
        
    rc = clBufferCreate(&payLoadMsg);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_CREATE_ERR, rc, rc,
                   CL_LOG_SEV_DEBUG, CL_LOG_HANDLE_APP);
    rc = clBufferNBytesWrite(payLoadMsg, (ClUint8T *)eventData,
                                    eventDataSize);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_SEV_DEBUG, CL_LOG_HANDLE_APP);

    switch(cpmEventType)
    {
        case CL_CPM_COMP_EVENT:
            rc = VDECL_VER(clXdrUnmarshallClCpmEventPayLoadT, 4, 0, 0)(payLoadMsg, payLoad);
            CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                           CL_LOG_SEV_DEBUG, CL_LOG_HANDLE_APP);
            break;
        case CL_CPM_NODE_EVENT:
            rc = VDECL_VER(clXdrUnmarshallClCpmEventNodePayLoadT, 4, 0, 0)(payLoadMsg, payLoad);
            CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                           CL_LOG_SEV_DEBUG, CL_LOG_HANDLE_APP);
            break;
        default:
            clOsalPrintf("Invalid event type received.\n");
            goto failure;
            break;
    }

failure:
    clBufferDelete(&payLoadMsg);
    clHeapFree(eventData);
    return rc;
}
