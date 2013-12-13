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
#include <clNameCkptIpi.h>

// extern ClCkptSvcHdlT   gNsCkptSvcHdl ;
extern ClCntHandleT    gNSHashTable;

#define NAME_LOG_AREA_CKPT	"CKPT"
#define NAME_LOG_AREA_NAME	"SER"
#define NAME_LOG_CTX_SERIALIZER	"SERIALIZER"
#define NAME_LOG_CTX_PACK	"PACK"

ClRcT
clNameContextCkptSerializer(ClUint32T  dsId,
                            ClAddrT    *pBuffer,
                            ClUint32T  *pSize,
                            ClPtrT cookie)
{
    ClRcT           rc     = CL_OK;
    ClBufferHandleT inMsg  = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    if( CL_NAME_CONTEXT_GBL_DSID != dsId )
    {        
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER, 
                   "dsId is not proper\n");
        return CL_NS_RC(CL_ERR_INVALID_PARAMETER);
    }    
    if( (NULL == pBuffer) || (NULL == pSize) )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "Passed Value is NULL");
        return CL_NS_RC(CL_ERR_NULL_POINTER);
    }    
    rc = clBufferCreate(&inMsg);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clBufferCreate() : rc[0x %x]", rc);
        return rc;
    }    
    rc = clNameContextCkptNamePack(inMsg);
    if( CL_OK != rc )
    {
        clBufferDelete(&inMsg);
        return rc;
    }    
    rc = clBufferLengthGet(inMsg, pSize);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clBufferLengthGet(); rc[0x %x]", rc);
        clBufferDelete(&inMsg);
        return rc;
    }    
    *pBuffer = clHeapCalloc(*pSize, sizeof(ClUint8T));
    if( NULL == *pBuffer )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clHeapCalloc()");
        clBufferDelete(&inMsg);
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }    
    rc = clBufferNBytesRead(inMsg, (ClUint8T *)*pBuffer, pSize);
    if( CL_OK != rc )
    {
        clHeapFree(*pBuffer);
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clBufferNBytesRead");
        clBufferDelete(&inMsg);
        return rc ;
    }    
    if( CL_OK != (rc = clBufferDelete(&inMsg)) ) 
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clBufferDelete(): rc[0x %x]", rc);
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameNSTableWalk(ClCntKeyHandleT   key,
                  ClCntDataHandleT  data,
                  ClCntArgHandleT   arg,
                  ClUint32T         dataSize)
{
    ClRcT           rc     = CL_OK;
    ClBufferHandleT inMsg  = (ClBufferHandleT)arg;

    CL_NAME_DEBUG_TRACE(("Enter"));
    rc = clXdrMarshallClUint32T(&key, inMsg, 0); 
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clXdrMarshallClUint32T(): rc[0x %x]", rc);
        return rc;
    }    
    CL_NAME_DEBUG_TRACE(("Exit"));

    return rc;
}    


ClRcT
clNameContextCkptNamePack(ClBufferHandleT  inMsg)
{
    ClRcT     rc   = CL_OK;
    ClUint32T size = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    rc = clCntSizeGet(gNSHashTable, &size);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_PACK,
                   "clCntSizeGet(): rc[0x %x]", rc);
        return rc;
    }    
    rc = clXdrMarshallClUint32T(&size, inMsg, 0);
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_PACK,
                   "clXdrMarshallClUint32T(): rc[0x %x]", rc);
        return rc;
    }    
    rc = clCntWalk(gNSHashTable, clNameNSTableWalk,
                   (ClCntArgHandleT)inMsg, sizeof(inMsg));

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcEntrySerializer(ClUint32T  dsId,
                         ClAddrT    *pBuffer,
                         ClUint32T  *pSize,
                         ClPtrT cookie)
{
    ClRcT            rc        = CL_OK;
    ClNsEntryPackT  *pNsEntry  = (ClNsEntryPackT *)cookie; 
    ClAddrT          pTemp     = NULL;

    CL_NAME_DEBUG_TRACE(("Enter"));
    
    *pSize   = sizeof(ClNsEntryPackT) + pNsEntry->nsInfo.attrLen;
    *pBuffer = clHeapCalloc(1, sizeof(ClNsEntryPackT)+ pNsEntry->nsInfo.attrLen);
    if( NULL == *pBuffer )
    {
        clLogError(NAME_LOG_AREA_NAME,NAME_LOG_CTX_SERIALIZER,
                   "clHeapCalloc(); rc[0x %x]", rc);
        return CL_NS_RC(CL_ERR_NULL_POINTER);
    }    
    pTemp = *pBuffer;
    memcpy(*pBuffer, &pNsEntry->type, sizeof(pNsEntry->type));
    *pBuffer += sizeof(pNsEntry->type);
    memcpy(*pBuffer, &pNsEntry->nsInfo, sizeof(ClNameSvcInfoIDLT));
    if( pNsEntry->nsInfo.attrCount > 0 )
    {    
        *pBuffer = *pBuffer + sizeof(ClNameSvcInfoIDLT);
        memcpy(*pBuffer, pNsEntry->nsInfo.attr, pNsEntry->nsInfo.attrLen);
    }   
    *pBuffer = pTemp;

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcPerCtxSerializer(ClUint32T  dsId,
                          ClAddrT    *pBuffer,
                          ClUint32T  *pSize,
                          ClPtrT cookie)
{
    ClRcT                   rc       = CL_OK;
    ClNameSvcContextInfoT  *pCtxData = (ClNameSvcContextInfoT *)cookie;

    CL_NAME_DEBUG_TRACE(("Enter"));
    *pSize = sizeof(ClNameSvcContextInfoT);  
    *pBuffer = clHeapCalloc(1, sizeof(ClNameSvcContextInfoT));
    if( NULL == *pBuffer )
    {
        clLogError(NAME_LOG_AREA_CKPT,NAME_LOG_CTX_SERIALIZER,
                   "clHeapCalloc()");
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }    
    memcpy(*pBuffer, pCtxData, sizeof(ClNameSvcContextInfoT));
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}


