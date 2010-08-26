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
 * ModuleName  : idl
 * File        : clIdlGen.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *     This file contains IDL APIs implementation
 *******************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clOsalApi.h>
#include <clIdlApi.h>
#include <clDebugApi.h>

/*Supporetd version*/
static ClVersionT clVersionSupported[]={
{'B',0x01 , 0x01}
};
/*Version Database*/
static ClVersionDatabaseT versionDatabase={
sizeof(clVersionSupported)/sizeof(ClVersionT),
clVersionSupported
}; 

char* clObjId_ClIdlHandleObjT = "ClIdlHandleObjT";


static ClOsalMutexT  idlMutex;
static ClBoolT       idlInit = CL_FALSE;

ClIdlClntT     gIdlClnt = {0};/*global variable for idl client*/
ClRcT clIdlHandleInitialize(
        ClIdlHandleObjT*        pIdlObj,    /*address of the remote object*/
        ClIdlHandleT*           pHandle)     /*handle to the IDL object*/
{

    ClIdlHandleObjT* pObj = NULL;
    ClRcT            rc   = CL_OK;

    if ((NULL == pIdlObj) || (NULL == pHandle))
    {
        return CL_IDL_RC(CL_ERR_NULL_POINTER);
    }
    if( CL_FALSE == idlInit )
    {
        rc = clOsalMutexInit(&idlMutex);
        if( CL_OK != rc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clOsalMutexCreate(): rc[0x %x]", rc));
            return rc;
        }    
        idlInit = CL_TRUE;
    }    
    rc = clOsalMutexLock(&idlMutex);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clOsalMutexLock(&): rc[0x %x]", rc));
        return rc;
    }        
    if( 0 == gIdlClnt.idlDbHdl)
    {
          rc = clHandleDatabaseCreate(NULL,&gIdlClnt.idlDbHdl);
          if(rc != CL_OK)
          {
               clOsalMutexUnlock(&idlMutex);
               return CL_IDL_RC(rc);
          }
          gIdlClnt.handleCount = 0;
    }
    rc = clHandleCreate(gIdlClnt.idlDbHdl,sizeof(ClIdlHandleObjT),(ClHandleT *)pHandle);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&idlMutex);
        return CL_IDL_RC(rc);
    }
    gIdlClnt.handleCount++;
    rc = clHandleCheckout(gIdlClnt.idlDbHdl,*pHandle,(void **)&pObj);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&idlMutex);
        return CL_IDL_RC(rc);
    }
    memset(pObj,'\0',sizeof(*pObj));
    pObj->objId    = clObjId_ClIdlHandleObjT;
    pObj->address  = pIdlObj->address;
    pObj->flags = pIdlObj->flags;
    pObj->options.timeout  = pIdlObj->options.timeout;
    pObj->options.retries  = pIdlObj->options.retries;
    pObj->options.priority = pIdlObj->options.priority;
    
    clHandleCheckin(gIdlClnt.idlDbHdl,*pHandle);
    clOsalMutexUnlock(&idlMutex);
    return CL_OK;
}

ClRcT clIdlVersionCheck(ClVersionT *pVersion)
{
     ClRcT  rc = CL_OK;
     if(pVersion == NULL)
     {
        rc = CL_ERR_NULL_POINTER;
        return CL_IDL_RC(rc);
     }
     rc = clVersionVerify(&versionDatabase,pVersion);
     if(rc != CL_OK)
     {
          return CL_IDL_RC(rc);
     }
     return rc;
}

ClRcT clIdlHandleCheckout(ClIdlHandleT    handle, ClIdlHandleObjT** out)
{
  *out = NULL;
  ClRcT            rc   = CL_OK;

  rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void**) out);
  if(rc == CL_OK)
    {
      if ( (*out)->objId != clObjId_ClIdlHandleObjT)
        {
          rc = CL_IDL_RC(CL_ERR_INVALID_HANDLE);
          clDbgCodeError(rc,("Handle is not of type %s",clObjId_ClIdlHandleObjT));
        }    
    }

  return rc;
}

ClRcT clIdlHandleCheckin(ClIdlHandleT    handle,ClIdlHandleObjT** out)
{
  ClRcT            rc   = CL_OK;
  if (out != NULL) *out = NULL;
  rc = clHandleCheckin(gIdlClnt.idlDbHdl,handle);

  return rc;
}


ClRcT clIdlHandleUpdate(ClIdlHandleT    handle,
                        ClIdlHandleObjT *pIdlObj)
{
    ClIdlHandleObjT* pObj = NULL;
    ClRcT            rc   = CL_OK;
    
    if ( NULL == pIdlObj)
    {
        return CL_IDL_RC(CL_ERR_NULL_POINTER);
    }
    if ( pIdlObj->objId != clObjId_ClIdlHandleObjT)
      {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(rc, ("Passed object is not of type %s",clObjId_ClIdlHandleObjT));
        return rc;
      }

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pObj);
    if(rc != CL_OK)
    {
        return CL_IDL_RC(rc);
    }

    if ( pObj->objId != clObjId_ClIdlHandleObjT)
      {
        rc = CL_IDL_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(rc, ("Handle is not of type %s",clObjId_ClIdlHandleObjT));
        return rc;
      }    

    pObj->address  = pIdlObj->address;
    pObj->flags = pIdlObj->flags;
    pObj->options.timeout  = pIdlObj->options.timeout;
    pObj->options.retries  = pIdlObj->options.retries;
    pObj->options.priority = pIdlObj->options.priority;
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    
    return CL_OK;
    
}
ClRcT clIdlHandleFinalize(ClIdlHandleT handle)
{
   ClRcT  rc = CL_OK;

   rc = clOsalMutexLock(&idlMutex);
   if( CL_OK != rc )
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clOsalMutexLock(&): rc[0x %x]", rc));
        return rc;
   }        
   rc = clHandleDestroy(gIdlClnt.idlDbHdl,handle);
   if(rc != CL_OK)
   {    
       CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleDestroy(): rc[0x %x]", rc));
       clOsalMutexUnlock(&idlMutex);
       return CL_IDL_RC(rc);
   }    
   if( --gIdlClnt.handleCount == 0 )
   {
      clHandleDatabaseDestroy(gIdlClnt.idlDbHdl);
      gIdlClnt.idlDbHdl = 0;
   }
   clOsalMutexUnlock(&idlMutex);
   return CL_OK;
}
