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
#include <clCpmApi.h>
#include <clNameCkptIpi.h>
#include <clCkptApi.h>

#include "clNameIpi.h"
#include "clNameCkptIpi.h"

extern ClCkptSvcHdlT     gNsCkptSvcHdl;
extern ClCntHandleT      gNSHashTable ;

#define NAME_LOG_AREA_NAME		"SVR"
#define NAME_LOG_AREA_CKPT		"CKP"
#define NAME_LOG_CTX_DESERIALIZER	"DESERIALIZER"

ClRcT
clNameContextCkptDeserializer(ClUint32T   dsId,
                              ClAddrT     pBuffer,
                              ClUint32T   size,
                              ClPtrT cookie)
{
    ClRcT           rc    = CL_OK;
    ClBufferHandleT inMsg = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    if( CL_NAME_CONTEXT_GBL_DSID != dsId )
    {        
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_DESERIALIZER, 
                   "dsId is not proper\n");
        return CL_NS_RC(CL_ERR_INVALID_PARAMETER);
    }    
    if( NULL == pBuffer )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_DESERIALIZER,
                   "Passed Value is NULL");
        return CL_NS_RC(CL_ERR_NULL_POINTER);
    }    
    rc = clBufferCreate(&inMsg);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_DESERIALIZER, 
                   "clBufferCreate() : rc[0x %x]", rc);
        return rc;
    }    
    rc = clBufferNBytesWrite(inMsg, (ClUint8T *)pBuffer, size); 
    if( CL_OK != rc )
    {
        clBufferDelete(&inMsg);
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_DESERIALIZER,
                   "clBufferNBytesWrite(): rc[0x %x]", rc);
        return rc;
    }    
    rc = clNameContextCkptNameUnpack(inMsg);
    if( CL_OK != rc )
    {
        clBufferDelete(&inMsg);
        return rc;
    }    
    if( CL_OK != (rc = clBufferDelete(&inMsg)) ) 
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_DESERIALIZER,
                   "clBufferDelete(): rc[0x %x]", rc);
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameContextCkptNameUnpack(ClBufferHandleT  inMsg)
{
    ClRcT             rc      = CL_OK;
    ClUint32T         key     = 0;
    ClUint32T         size    = 0;
    ClUint32T         count   = 0;
    ClIocNodeAddressT localAddr = 0;
    ClIocNodeAddressT masterAddr = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    rc = clXdrUnmarshallClUint32T(inMsg, &size);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_NAME,NAME_LOG_AREA_CKPT,
                   "clXdrUnmarshallClUint32T(): rc[0x %x]", rc);
        return rc;
    }    
    clLogDebug("SVR", "CKP", "Num of contexts: %d", size);
    for( count = 0; count < size; count++)
    {    
        rc = clXdrUnmarshallClUint32T(inMsg, &key);
        if( CL_OK != rc )
        {
            return rc;
        }    
        if( (key > CL_NS_DEFAULT_GLOBAL_CONTEXT) && 
            (key < CL_NS_BASE_NODELOCAL_CONTEXT) )
        {
            localAddr = clIocLocalAddressGet();
            rc = clCpmMasterAddressGet(&masterAddr);
            if (rc != CL_OK)
            {
                clLogError(NAME_LOG_AREA_NAME,NAME_LOG_AREA_CKPT,"clCpmMasterAddressGet failed with rc 0x%x",rc);
                return rc;
            }

            if( masterAddr == localAddr )
            {
                rc = clNameSvcCtxRecreate(key);
                if( CL_OK != rc )
                {
                    return rc;
                }    
            }    
        }    
        else
        {    
            rc = clNameSvcCtxRecreate(key);
            if( CL_OK != rc )
            {
                return rc;
            }    
        } 
    }
    
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    


ClRcT
clNameSvcBindingEntryRecreate(ClNameSvcContextInfoT  *pCtxData,
                              /* Suppressing coverity warning for pass by value with below comment */
                              // coverity[pass_by_value]
                              ClNsEntryPackT         nsEntryInfo)
{
    ClRcT              rc        = CL_OK;
    
    clLogDebug("SVR", "RESTART", "Binding entry type: %d", nsEntryInfo.type);
    switch( nsEntryInfo.type )
    {
        case CL_NAME_SVC_CTX_INFO:
            {
                rc = clNameSvcPerCtxInfoUpdate(pCtxData, &nsEntryInfo.nsInfo);
                break;
            }    
        case CL_NAME_SVC_ENTRY:
            {
              rc = clNameSvcBindingEntryCreate(pCtxData,
                     &nsEntryInfo.nsInfo);
              break;
            }    
        case CL_NAME_SVC_BINDING_DATA:
            {
              rc = clNameSvcBindingDetailEntryCreate(pCtxData,
                             &nsEntryInfo.nsInfo); 
              break;
            }                    
        case CL_NAME_SVC_COMP_INFO :
            {
                rc = clNameSvcCompInfoAdd(pCtxData, &nsEntryInfo.nsInfo);
                break;
            }                
        default:
            clLogError(NAME_LOG_AREA_CKPT,CL_LOG_CONTEXT_UNSPECIFIED,
                       "Invalid type of data");
            return CL_NS_RC(CL_ERR_INVALID_PARAMETER);
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}   

ClRcT
clNameSvcEntryDeserializer(ClUint32T  dsId,
                           ClAddrT    pBuffer,
                           ClUint32T  size,
                           ClPtrT cookie)
{
    ClRcT           rc            = CL_OK;
    ClNsEntryPackT  *pNsEntryInfo = (ClNsEntryPackT *)cookie;
    
    CL_NAME_DEBUG_TRACE(("Enter"));
    
    memcpy(&pNsEntryInfo->type, pBuffer, sizeof(pNsEntryInfo->type));
    pBuffer += sizeof(pNsEntryInfo->type);
    memcpy(&pNsEntryInfo->nsInfo, pBuffer, sizeof(ClNameSvcInfoIDLT));
    pBuffer += sizeof(ClNameSvcInfoIDLT);
    if( pNsEntryInfo->nsInfo.attrCount > 0 )
    {
       pNsEntryInfo->nsInfo.attr = clHeapCalloc(pNsEntryInfo->nsInfo.attrLen,
                                                sizeof(ClCharT));
       if( NULL == pNsEntryInfo->nsInfo.attr )
       {
            clLogError(NAME_LOG_AREA_NAME,NAME_LOG_CTX_DESERIALIZER,"clHeapCalloc(): ");        
            return CL_NS_RC(CL_ERR_NO_MEMORY);
       }    
       memcpy(pNsEntryInfo->nsInfo.attr, pBuffer,
               pNsEntryInfo->nsInfo.attrLen);
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcPerCtxDeserializer(ClUint32T  dsId,
                            ClAddrT    pBuffer,
                            ClUint32T  size,
                            ClPtrT cookie)
{
    ClRcT                 rc        = CL_OK;
    ClNameSvcContextInfoT *pCtxData = (ClNameSvcContextInfoT *)cookie;

    CL_NAME_DEBUG_TRACE(("Enter"));
    memcpy(pCtxData, pBuffer, size); 
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    
