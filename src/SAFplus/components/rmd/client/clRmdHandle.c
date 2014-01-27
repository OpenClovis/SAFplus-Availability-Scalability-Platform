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
 * ModuleName  : rmd
 * File        : clRmdHandle.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module implements RMD.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clIocApi.h>
#include "clRmdApi.h"
#include "clEoApi.h"
#include "clRmdMain.h"
#include "clDebugApi.h"

#ifdef DMALLOC
# include "dmalloc.h"
#endif

/**
 *
 * NAME:clRmdInvoke
 *                     
 * DESCRIPTION:       
 *                     
 * Calls the registered function 
 *                                                              
 * ARGUMENTS: 
 *     pRequest is the pointer to rmdMsg whereas
 *     pReply  is the pointer to data part of the repl rmd msg. 
 *                                                            
 * RETURNS:                                                 
 *                                                         
 **/
ClRcT clRmdInvoke(ClEoPayloadWithReplyCallbackT func, ClEoDataT eoArg, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{

    ClRmdHdrT inHdr = {{0}};
    ClRcT rc = CL_OK;
    ClUint8T hdrBuffer[sizeof(ClRmdHdrT)*2] = {0};
    ClUint32T hdrLen = 0;
#if 0
    ClBufferHandleT tempInMsgHdl = 0;
    ClUint32T msgLength = 0;
#endif

    CL_FUNC_ENTER();

    rc = clBufferReadOffsetSet(inMsgHdl, 0, CL_BUFFER_SEEK_SET);
    if (CL_OK != rc)
    {
        goto L0;
    }
    
    clRmdUnmarshallRmdHdr(inMsgHdl, &inHdr, &hdrLen);

    rc = clBufferHeaderTrim(inMsgHdl, hdrLen);
    if (CL_OK != rc)
    {
        goto L0;
    }

    rc = clBufferReadOffsetSet(inMsgHdl, 0, CL_BUFFER_SEEK_SET);
    if (CL_OK != rc)
    {
        goto L1;
    }

    /*
     * The following does not hold any longer so removing the duplication to 
     * improve the performance.
     */
#if 0
    /*
     * The inMsgHdl could be passed to multiple functions each of which
     * could be doing a buffer flatten inside. Since buffers cannot be
     * flattened more than once(as per restriction specified in clBufferApi.h), 
     * the message is deplicated before passing it down to the user. Final
     * resolution has to be a removal of the restriction in clBufferApi.h. This 
     * is a temporary fix to allow users to flatten buffers. A better temporary 
     * fix would be to have the clEoWalk check if functions are chained and
     * only in that case, make duplicates. Very few functions are chained
     */
    rc = clBufferLengthGet(inMsgHdl, &msgLength);
    if (CL_OK != rc)
    {
        goto L1;
    }

    if (msgLength)
    {
        rc = clBufferDuplicate(inMsgHdl, &tempInMsgHdl);
        if (CL_OK != rc)
        {
            goto L1;
        }
    }
    tempInMsgHdl = inMsgHdl;
#endif

    if ((inHdr.flags & CL_RMD_CALL_NEED_REPLY))
    {

        RMD_DBG3((" RMD Reply needed Params\n"));
        rc = clBufferWriteOffsetSet(outMsgHdl, 0, CL_BUFFER_SEEK_SET);
        if (CL_OK != rc)
        {
            goto L1;
        }

        rc = func(eoArg, inMsgHdl, outMsgHdl);
        if (CL_OK != rc)
        {
            goto L1;
        }
    }
    else
    {

        RMD_DBG3((" RMD Reply not needed \n"));
        rc = func(eoArg, inMsgHdl, 0);
        if (CL_OK != rc)
        {
            goto L1;
        }
    }

#if 0
    if (msgLength)
    {
        rc = clBufferDelete(&tempInMsgHdl);
        if (CL_OK != rc)
        {
            goto L1;
        }
    }
#endif

    rc = clBufferReadOffsetSet(inMsgHdl, 0, CL_BUFFER_SEEK_SET);
    if (CL_OK != rc)
    {
        goto L0;
    }

    hdrLen = 0;
    clRmdMarshallRmdHdr(&inHdr, hdrBuffer, &hdrLen);
    rc = clBufferDataPrepend(inMsgHdl, hdrBuffer, hdrLen);
    if (CL_OK != rc)
    {
        goto L0;
    }

    CL_FUNC_EXIT();
    return rc;

#if 0
    L2:if (tempInMsgHdl)
    {
        clBufferDelete(&tempInMsgHdl);
    }
#endif
    L1:clBufferReadOffsetSet(inMsgHdl, 0, CL_BUFFER_SEEK_SET);
    hdrLen = 0;
    clRmdMarshallRmdHdr(&inHdr, hdrBuffer, &hdrLen);
    clBufferDataPrepend(inMsgHdl, hdrBuffer, hdrLen);
    L0:CL_FUNC_EXIT();
    return rc;
}
