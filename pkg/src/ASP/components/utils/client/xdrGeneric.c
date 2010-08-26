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
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : utils
 * File        : xdrGeneric.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <netinet/in.h>
#include <clOsalApi.h>
#include <clBufferApi.h>
#include <clHandleApi.h>
#include <ipi/clRmdIpi.h>
#include "clXdrApi.h"

ClRcT clXdrMarshallArray(void* array, ClUint32T typeSize, ClUint32T multiplicity, ClXdrMarshallFuncT func, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    CL_XDR_ENTER()

    if (0 == multiplicity)
    {
        CL_XDR_EXIT()
        return rc;
    }

    if (NULL == array)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    for (i = 0; i < multiplicity; i++)
    {
        rc = func(array, msg, isDelete);
        if (CL_OK != rc)
        {
            CL_XDR_EXIT()
            return rc;
        }

        array = ((ClUint8T *)array + typeSize);
    }

    CL_XDR_EXIT()
    return rc;
}


ClRcT clXdrUnmarshallArray( ClBufferHandleT msg,void* array, ClUint32T typeSize, ClUint32T multiplicity, ClXdrUnmarshallFuncT func)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    CL_XDR_ENTER()

    if (0 == multiplicity)
    {
        CL_XDR_EXIT()
        return rc;
    }

    if (NULL == array)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    for (i = 0; i < multiplicity; i++)
    {
        rc = func(msg,array);
        if (CL_OK != rc)
        {
            CL_XDR_EXIT()
            return rc;
        }

        array = ((ClUint8T *)array + typeSize);
    }

    CL_XDR_EXIT()
    return rc;
}


ClRcT clXdrError(void *pPyld, ...)
{
    CL_XDR_ENTER()
    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT clIdlSyncPrivateInfoSet(ClUint32T idlSyncKey, void *  pIdlCtxInfo)
{
   return clOsalTaskDataSet(idlSyncKey,(ClOsalTaskDataT)pIdlCtxInfo);
}

ClRcT clIdlSyncPrivateInfoGet(ClUint32T idlSyncKey, void **  data)
{
   return clOsalTaskDataGet(idlSyncKey,(ClOsalTaskDataT *)data);
}

ClRcT clIdlSyncDefer(ClHandleDatabaseHandleT idlDatabaseHdl,
		     ClUint32T               idlSyncKey,
		     ClIdlHandleT            *pIdlHdl)
{  
    ClIdlContextInfoT  *pIdlContextInfo   = NULL; 
    ClIdlSyncInfoT     *pIdlSyncDeferInfo = NULL;
    ClRcT               rc                = CL_OK;

    rc = clIdlSyncPrivateInfoGet(idlSyncKey, (void **)&pIdlContextInfo);
    if(rc != CL_OK)
    {
       return rc;
    }
    rc = clHandleCreate(idlDatabaseHdl,sizeof(ClIdlSyncInfoT),pIdlHdl);
    if(rc != CL_OK)
    { 
       return rc;
    }
    rc = clHandleCheckout(idlDatabaseHdl,*pIdlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
       return rc;
    }
    rc = clRmdResponseDefer((ClRmdResponseContextHandleT *)&pIdlSyncDeferInfo->idlRmdDeferHdl); 
    if(rc != CL_OK)
    { 
       clHandleCheckin(idlDatabaseHdl,*pIdlHdl);
       return rc;
    }
    pIdlSyncDeferInfo->idlRmdDeferMsg = pIdlContextInfo->idlDeferMsg;
    
    pIdlContextInfo->inProgress = CL_TRUE;
    rc = clHandleCheckin(idlDatabaseHdl, *pIdlHdl);
    return rc;
}

ClRcT clIdlSyncResponseSend(ClIdlHandleT idlRmdHdl,ClBufferHandleT rmdMsgHdl,ClRcT retCode)
{
  return clRmdSyncResponseSend(idlRmdHdl, rmdMsgHdl, retCode);
}
