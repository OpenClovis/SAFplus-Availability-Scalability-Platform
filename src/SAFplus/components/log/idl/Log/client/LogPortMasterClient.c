
/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description :ClientSide Stub routines
*
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*
*********************************************************************/

#include <netinet/in.h>
#include <string.h>
#include <clBufferApi.h>
#include <clRmdApi.h>
#include <clIdlApi.h>
#include <clEoApi.h>
#include <clXdrApi.h>
#include <clHandleApi.h>
#include "LogPortMasterClient.h"
extern ClIdlClntT gIdlClnt;



static void clLogMasterAttrVerifyNGetAsyncCallback_4_0_0(ClRcT rc, void *pIdlCookie, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlCookieT* pCookie = (ClIdlCookieT*)pIdlCookie;
    ClRcT retVal = CL_OK;
    ClLogStreamAttrIDLT_4_0_0  pStreamAttr;
    SaNameT  pStreamName;
    ClLogStreamScopeT  pStreamScope;
    SaNameT  pStreamScopeNode;
    ClUint16T  pStreamId;
    ClUint64T  pStreamMcastAddr;

    memset(&(pStreamAttr), 0, sizeof(ClLogStreamAttrIDLT_4_0_0));
    memset(&(pStreamName), 0, sizeof(SaNameT));
    memset(&(pStreamScope), 0, sizeof(ClLogStreamScopeT));
    memset(&(pStreamScopeNode), 0, sizeof(SaNameT));
    memset(&(pStreamId), 0, sizeof(ClUint16T));
    memset(&(pStreamMcastAddr), 0, sizeof(ClUint64T));


    retVal = clXdrUnmarshallClLogStreamAttrIDLT_4_0_0(inMsgHdl, &(pStreamAttr));
    if (CL_OK != retVal)
    {
        goto L0;
    }

    retVal = clXdrUnmarshallSaNameT(inMsgHdl, &(pStreamName));
    if (CL_OK != retVal)
    {
        goto L1;
    }

    retVal = clXdrUnmarshallClLogStreamScopeT_4_0_0(inMsgHdl, &(pStreamScope));
    if (CL_OK != retVal)
    {
        goto L2;
    }

    retVal = clXdrUnmarshallSaNameT(inMsgHdl, &(pStreamScopeNode));
    if (CL_OK != retVal)
    {
        goto L3;
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint16T(outMsgHdl, &(pStreamId));
        if (CL_OK != retVal)
        {
            goto L4;
        }
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint64T(outMsgHdl, &(pStreamMcastAddr));
        if (CL_OK != retVal)
        {
            goto L5;
        }
    }

    if (rc != CL_OK)
    {
        retVal = rc;
    }

    ((LogClLogMasterAttrVerifyNGetAsyncCallbackT_4_0_0)(pCookie->actualCallback))(pCookie->handle, &(pStreamAttr), &(pStreamName), &(pStreamScope), &(pStreamScopeNode), &(pStreamId), &(pStreamMcastAddr), retVal, pCookie->pCookie);
    goto L6;

L6: 
L5: 
L4: 
L3: 
L2: 
L1: 

L0:  clHeapFree(pCookie);
     clBufferDelete(&outMsgHdl);
     return;
}


ClRcT clLogMasterAttrVerifyNGetClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClLogStreamAttrIDLT_4_0_0* pStreamAttr, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT* pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_INOUT ClUint16T* pStreamId, CL_OUT ClUint64T* pStreamMcastAddr,CL_IN LogClLogMasterAttrVerifyNGetAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClRmdAsyncOptionsT asyncOptions;
    ClUint32T tempFlags = 0;
    ClIdlCookieT* pCookie = NULL;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if(rc != CL_OK)
    {
        return rc;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            goto L;
        }
    }
    else
    {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        goto L;
    }

    rc = clBufferCreate(&inMsgHdl);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClLogStreamAttrIDLT_4_0_0(pStreamAttr, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallSaNameT(pStreamName, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClLogStreamScopeT_4_0_0(pStreamScope, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallSaNameT(pStreamScopeNode, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClUint16T(pStreamId, inMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L;
    }

    if(fpAsyncCallback != NULL)
    {
        

        pCookie = (ClIdlCookieT*) clHeapAllocate(sizeof(ClIdlCookieT));
        if (NULL == pCookie)
        {
            return CL_IDL_RC(CL_ERR_NO_MEMORY);
        }
        
        asyncOptions.pCookie = NULL;
        asyncOptions.fpCallback = NULL;
        
        rc = clBufferCreate(&outMsgHdl);
        if (CL_OK != rc)
        {
            goto L2;
        }

        tempFlags |= pHandleObj->flags | (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
        
        pCookie->pCookie = cookie;
        pCookie->actualCallback = (void(*)())fpAsyncCallback;
        pCookie->handle = handle;
        asyncOptions.pCookie = pCookie;
        asyncOptions.fpCallback = clLogMasterAttrVerifyNGetAsyncCallback_4_0_0;

        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), &asyncOptions);
        if (CL_OK != rc)
        {
            goto LL;
         }
    }
    else
    {
        tempFlags |= pHandleObj->flags | (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT);
        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, 0, tempFlags, &(pHandleObj->options),NULL);
        if(CL_OK != rc)
        {
               goto L;
        }
    }
    
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;

LL: clBufferDelete(&outMsgHdl);
L2:  clHeapFree(pCookie);
L:
     clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}



static void clLogMasterStreamCloseNotifyAsyncCallback_4_0_0(ClRcT rc, void *pIdlCookie, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlCookieT* pCookie = (ClIdlCookieT*)pIdlCookie;
    ClRcT retVal = CL_OK;
    ClStringT  pFileName;
    ClStringT  pFileLocation;
    SaNameT  pStreamName;
    ClLogStreamScopeT  pStreamScope;
    SaNameT  pStreamScopeNode;

    memset(&(pFileName), 0, sizeof(ClStringT));
    memset(&(pFileLocation), 0, sizeof(ClStringT));
    memset(&(pStreamName), 0, sizeof(SaNameT));
    memset(&(pStreamScope), 0, sizeof(ClLogStreamScopeT));
    memset(&(pStreamScopeNode), 0, sizeof(SaNameT));


    retVal = clXdrUnmarshallClStringT(inMsgHdl, &(pFileName));
    if (CL_OK != retVal)
    {
        goto L0;
    }

    retVal = clXdrUnmarshallClStringT(inMsgHdl, &(pFileLocation));
    if (CL_OK != retVal)
    {
        goto L1;
    }

    retVal = clXdrUnmarshallSaNameT(inMsgHdl, &(pStreamName));
    if (CL_OK != retVal)
    {
        goto L2;
    }

    retVal = clXdrUnmarshallClLogStreamScopeT_4_0_0(inMsgHdl, &(pStreamScope));
    if (CL_OK != retVal)
    {
        goto L3;
    }

    retVal = clXdrUnmarshallSaNameT(inMsgHdl, &(pStreamScopeNode));
    if (CL_OK != retVal)
    {
        goto L4;
    }

    if (rc != CL_OK)
    {
        retVal = rc;
    }

    ((LogClLogMasterStreamCloseNotifyAsyncCallbackT_4_0_0)(pCookie->actualCallback))(pCookie->handle, &(pFileName), &(pFileLocation), &(pStreamName), pStreamScope, &(pStreamScopeNode), retVal, pCookie->pCookie);
    goto L5;

L5: 
L4: 
L3: 
L2: 
L1: 

L0:  clHeapFree(pCookie);
     clBufferDelete(&outMsgHdl);
     return;
}


ClRcT clLogMasterStreamCloseNotifyClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClStringT* pFileName, CL_IN ClStringT* pFileLocation, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode,CL_IN LogClLogMasterStreamCloseNotifyAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClRmdAsyncOptionsT asyncOptions;
    ClUint32T tempFlags = 0;
    ClIdlCookieT* pCookie = NULL;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if(rc != CL_OK)
    {
        return rc;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            goto L;
        }
    }
    else
    {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        goto L;
    }

    rc = clBufferCreate(&inMsgHdl);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClStringT(pFileName, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClStringT(pFileLocation, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallSaNameT(pStreamName, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallSaNameT(pStreamScopeNode, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    if(fpAsyncCallback != NULL)
    {
        

        pCookie = (ClIdlCookieT*) clHeapAllocate(sizeof(ClIdlCookieT));
        if (NULL == pCookie)
        {
            return CL_IDL_RC(CL_ERR_NO_MEMORY);
        }
        
        asyncOptions.pCookie = NULL;
        asyncOptions.fpCallback = NULL;
        
        rc = clBufferCreate(&outMsgHdl);
        if (CL_OK != rc)
        {
            goto L2;
        }

        tempFlags |= pHandleObj->flags | (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
        
        pCookie->pCookie = cookie;
        pCookie->actualCallback = (void(*)())fpAsyncCallback;
        pCookie->handle = handle;
        asyncOptions.pCookie = pCookie;
        asyncOptions.fpCallback = clLogMasterStreamCloseNotifyAsyncCallback_4_0_0;

        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), &asyncOptions);
        if (CL_OK != rc)
        {
            goto LL;
         }
    }
    else
    {
        tempFlags |= pHandleObj->flags | (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT);
        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, 0, tempFlags, &(pHandleObj->options),NULL);
        if(CL_OK != rc)
        {
               goto L;
        }
    }
    
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;

LL: clBufferDelete(&outMsgHdl);
L2:  clHeapFree(pCookie);
L:
     clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}


ClRcT clLogMasterStreamListGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_OUT ClUint32T* pNumStreams, CL_OUT ClUint32T* pBuffLen, CL_OUT ClUint8T** pBuffer)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClUint32T tempFlags = 0;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if( rc != CL_OK )
    {
        return rc ;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }


    rc = clBufferCreate(&outMsgHdl);
    if (CL_OK != rc)
    {
        return rc;
    }

    tempFlags |= pHandleObj->flags |
                 (CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
    tempFlags &= ~CL_RMD_CALL_ASYNC;

    rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), NULL);
    if(CL_OK != rc)
    {
        clBufferDelete(&outMsgHdl);
    return rc;
    }


    rc = clXdrUnmarshallClUint32T( outMsgHdl, pNumStreams);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T( outMsgHdl, pBuffLen);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallPtrClUint8T( outMsgHdl, (void **)pBuffer, *pBuffLen);
    if (CL_OK != rc)
    {
        return rc;
    }

    clBufferDelete(&outMsgHdl);
    
    rc = clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}


static void clLogMasterStreamListGetAsyncCallback_4_0_0(ClRcT rc, void *pIdlCookie, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlCookieT* pCookie = (ClIdlCookieT*)pIdlCookie;
    ClRcT retVal = CL_OK;
    ClUint32T  pNumStreams;
    ClUint32T  pBuffLen;
    ClUint8T* pBuffer;

    memset(&(pNumStreams), 0, sizeof(ClUint32T));
    memset(&(pBuffLen), 0, sizeof(ClUint32T));
    memset(&(pBuffer), 0, sizeof(ClUint8T*));


    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint32T(outMsgHdl, &(pNumStreams));
        if (CL_OK != retVal)
        {
            goto L0;
        }
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint32T(outMsgHdl, &(pBuffLen));
        if (CL_OK != retVal)
        {
            goto L1;
        }
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallPtrClUint8T(outMsgHdl, (void**)&(pBuffer), pBuffLen);
        if (CL_OK != retVal)
        {
            goto L2;
        }
    }

    if (rc != CL_OK)
    {
        retVal = rc;
    }

    ((LogClLogMasterStreamListGetAsyncCallbackT_4_0_0)(pCookie->actualCallback))(pCookie->handle, &(pNumStreams), &(pBuffLen), &pBuffer, retVal, pCookie->pCookie);
    goto L3;

L3: 
L2: 
L1: 

L0:  clHeapFree(pCookie);
     clBufferDelete(&outMsgHdl);
     return;
}


ClRcT clLogMasterStreamListGetClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_OUT ClUint32T* pNumStreams, CL_OUT ClUint32T* pBuffLen, CL_OUT ClUint8T** pBuffer,CL_IN LogClLogMasterStreamListGetAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClRmdAsyncOptionsT asyncOptions;
    ClUint32T tempFlags = 0;
    ClIdlCookieT* pCookie = NULL;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if(rc != CL_OK)
    {
        return rc;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            goto L;
        }
    }
    else
    {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        goto L;
    }

    if(fpAsyncCallback != NULL)
    {
        

        pCookie = (ClIdlCookieT*) clHeapAllocate(sizeof(ClIdlCookieT));
        if (NULL == pCookie)
        {
            return CL_IDL_RC(CL_ERR_NO_MEMORY);
        }
        
        asyncOptions.pCookie = NULL;
        asyncOptions.fpCallback = NULL;
        
        rc = clBufferCreate(&outMsgHdl);
        if (CL_OK != rc)
        {
            goto L2;
        }

        tempFlags |= pHandleObj->flags |
                     (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
        
        pCookie->pCookie = cookie;
        pCookie->actualCallback = (void(*)())fpAsyncCallback;
        pCookie->handle = handle;
        asyncOptions.pCookie = pCookie;
        asyncOptions.fpCallback = clLogMasterStreamListGetAsyncCallback_4_0_0;

        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), &asyncOptions);
        if (CL_OK != rc)
        {
            goto LL;
         }
    }
    else
    {
        tempFlags |= pHandleObj->flags |
                         (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT);
        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, 0, tempFlags, &(pHandleObj->options),NULL);
        if(CL_OK != rc)
        {
               goto L;
        }
    }
    
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;

LL: clBufferDelete(&outMsgHdl);
L2:  clHeapFree(pCookie);
L:
     clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}


ClRcT clLogMasterCompIdChkNGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pCompName, CL_INOUT ClUint32T* pClientId)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClUint32T tempFlags = 0;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if( rc != CL_OK )
    {
        return rc ;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clBufferCreate(&inMsgHdl);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallSaNameT(pCompName, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClUint32T(pClientId, inMsgHdl, 1);
    if (CL_OK != rc)
    {
        return rc;
    }


    rc = clBufferCreate(&outMsgHdl);
    if (CL_OK != rc)
    {
        return rc;
    }

    tempFlags |= pHandleObj->flags |
                 (CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
    tempFlags &= ~CL_RMD_CALL_ASYNC;

    rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), NULL);
    if(CL_OK != rc)
    {
        clBufferDelete(&outMsgHdl);
    return rc;
    }


    rc = clXdrUnmarshallClUint32T( outMsgHdl, pClientId);
    if (CL_OK != rc)
    {
        return rc;
    }

    clBufferDelete(&outMsgHdl);
    
    rc = clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}

ClRcT clLogMasterCompListGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_OUT ClUint32T* pNumStreams, CL_OUT ClUint32T* pBuffLen, CL_OUT ClUint8T** pBuffer)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClUint32T tempFlags = 0;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if( rc != CL_OK )
    {
        return rc ;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }


    rc = clBufferCreate(&outMsgHdl);
    if (CL_OK != rc)
    {
        return rc;
    }

    tempFlags |= pHandleObj->flags |
                 (CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
    tempFlags &= ~CL_RMD_CALL_ASYNC;

    rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), NULL);
    if(CL_OK != rc)
    {
        clBufferDelete(&outMsgHdl);
    return rc;
    }


    rc = clXdrUnmarshallClUint32T( outMsgHdl, pNumStreams);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T( outMsgHdl, pBuffLen);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallPtrClUint8T( outMsgHdl, (void **)pBuffer, *pBuffLen);
    if (CL_OK != rc)
    {
        return rc;
    }

    clBufferDelete(&outMsgHdl);
    
    rc = clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}


static void clLogMasterCompListGetAsyncCallback_4_0_0(ClRcT rc, void *pIdlCookie, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlCookieT* pCookie = (ClIdlCookieT*)pIdlCookie;
    ClRcT retVal = CL_OK;
    ClUint32T  pNumStreams;
    ClUint32T  pBuffLen;
    ClUint8T* pBuffer;

    memset(&(pNumStreams), 0, sizeof(ClUint32T));
    memset(&(pBuffLen), 0, sizeof(ClUint32T));
    memset(&(pBuffer), 0, sizeof(ClUint8T*));


    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint32T(outMsgHdl, &(pNumStreams));
        if (CL_OK != retVal)
        {
            goto L0;
        }
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallClUint32T(outMsgHdl, &(pBuffLen));
        if (CL_OK != retVal)
        {
            goto L1;
        }
    }

    if (CL_OK == rc)
    {
        retVal = clXdrUnmarshallPtrClUint8T(outMsgHdl, (void**)&(pBuffer), pBuffLen);
        if (CL_OK != retVal)
        {
            goto L2;
        }
    }

    if (rc != CL_OK)
    {
        retVal = rc;
    }

    ((LogClLogMasterCompListGetAsyncCallbackT_4_0_0)(pCookie->actualCallback))(pCookie->handle, &(pNumStreams), &(pBuffLen), &pBuffer, retVal, pCookie->pCookie);
    goto L3;

L3: 
L2: 
L1: 

L0:  clHeapFree(pCookie);
     clBufferDelete(&outMsgHdl);
     return;
}


ClRcT clLogMasterCompListGetClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_OUT ClUint32T* pNumStreams, CL_OUT ClUint32T* pBuffLen, CL_OUT ClUint8T** pBuffer,CL_IN LogClLogMasterCompListGetAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClRmdAsyncOptionsT asyncOptions;
    ClUint32T tempFlags = 0;
    ClIdlCookieT* pCookie = NULL;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if(rc != CL_OK)
    {
        return rc;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            goto L;
        }
    }
    else
    {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        goto L;
    }

    if(fpAsyncCallback != NULL)
    {
        

        pCookie = (ClIdlCookieT*) clHeapAllocate(sizeof(ClIdlCookieT));
        if (NULL == pCookie)
        {
            return CL_IDL_RC(CL_ERR_NO_MEMORY);
        }
        
        asyncOptions.pCookie = NULL;
        asyncOptions.fpCallback = NULL;
        
        rc = clBufferCreate(&outMsgHdl);
        if (CL_OK != rc)
        {
            goto L2;
        }

        tempFlags |= pHandleObj->flags |
                     (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
        
        pCookie->pCookie = cookie;
        pCookie->actualCallback = (void(*)())fpAsyncCallback;
        pCookie->handle = handle;
        asyncOptions.pCookie = pCookie;
        asyncOptions.fpCallback = clLogMasterCompListGetAsyncCallback_4_0_0;

        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), &asyncOptions);
        if (CL_OK != rc)
        {
            goto LL;
         }
    }
    else
    {
        tempFlags |= pHandleObj->flags |
                         (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT);
        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, 0, tempFlags, &(pHandleObj->options),NULL);
        if(CL_OK != rc)
        {
               goto L;
        }
    }
    
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;

LL: clBufferDelete(&outMsgHdl);
L2:  clHeapFree(pCookie);
L:
     clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}



static void clLogMasterCompListNotifyAsyncCallback_4_0_0(ClRcT rc, void *pIdlCookie, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlCookieT* pCookie = (ClIdlCookieT*)pIdlCookie;
    ClRcT retVal = CL_OK;
    ClUint32T  pNumEntries;
    ClLogCompDataT_4_0_0* pCompData;

    memset(&(pNumEntries), 0, sizeof(ClUint32T));
    memset(&(pCompData), 0, sizeof(ClLogCompDataT_4_0_0*));


    retVal = clXdrUnmarshallClUint32T(inMsgHdl, &(pNumEntries));
    if (CL_OK != retVal)
    {
        goto L0;
    }

    retVal = clXdrUnmarshallPtrClLogCompDataT_4_0_0(inMsgHdl, (void**)&(pCompData), pNumEntries);
    if (CL_OK != retVal)
    {
        goto L1;
    }

    if (rc != CL_OK)
    {
        retVal = rc;
    }

    ((LogClLogMasterCompListNotifyAsyncCallbackT_4_0_0)(pCookie->actualCallback))(pCookie->handle, pNumEntries, pCompData, retVal, pCookie->pCookie);
    goto L2;

L2: 
L1: 

L0:  clHeapFree(pCookie);
     clBufferDelete(&outMsgHdl);
     return;
}


ClRcT clLogMasterCompListNotifyClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClUint32T  pNumEntries, CL_IN ClLogCompDataT_4_0_0* pCompData,CL_IN LogClLogMasterCompListNotifyAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT funcVer = {4, 0, 0};
    ClUint32T funcNo = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 5);
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClIocAddressT address = {{0}};
    ClIdlHandleObjT* pHandleObj = NULL;
    ClRmdAsyncOptionsT asyncOptions;
    ClUint32T tempFlags = 0;
    ClIdlCookieT* pCookie = NULL;

    rc = clHandleCheckout(gIdlClnt.idlDbHdl,handle,(void **)&pHandleObj);
    if(rc != CL_OK)
    {
        return rc;
    }
    if (CL_IDL_ADDRESSTYPE_IOC == pHandleObj->address.addressType)
    {
        address = pHandleObj->address.address.iocAddress;
    }
    else if (CL_IDL_ADDRESSTYPE_NAME == pHandleObj->address.addressType)
    {
        rc = clNameToObjectReferenceGet(&(pHandleObj->address.address.nameAddress.name),
                                        pHandleObj->address.address.nameAddress.attrCount,
                                        pHandleObj->address.address.nameAddress.attr,
                                        pHandleObj->address.address.nameAddress.contextCookie,
                                        (ClUint64T*)&address);
        if (CL_OK != rc)
        {
            goto L;
        }
    }
    else
    {
        rc = CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
        goto L;
    }

    rc = clBufferCreate(&inMsgHdl);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallClUint32T(&(pNumEntries), inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    rc = clXdrMarshallArrayClLogCompDataT_4_0_0(pCompData, pNumEntries, inMsgHdl, 0);
    if (CL_OK != rc)
    {
        goto L;
    }

    if(fpAsyncCallback != NULL)
    {
        

        pCookie = (ClIdlCookieT*) clHeapAllocate(sizeof(ClIdlCookieT));
        if (NULL == pCookie)
        {
            return CL_IDL_RC(CL_ERR_NO_MEMORY);
        }
        
        asyncOptions.pCookie = NULL;
        asyncOptions.fpCallback = NULL;
        
        rc = clBufferCreate(&outMsgHdl);
        if (CL_OK != rc)
        {
            goto L2;
        }

        tempFlags |= pHandleObj->flags |
                     (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_NEED_REPLY);
        
        pCookie->pCookie = cookie;
        pCookie->actualCallback = (void(*)())fpAsyncCallback;
        pCookie->handle = handle;
        asyncOptions.pCookie = pCookie;
        asyncOptions.fpCallback = clLogMasterCompListNotifyAsyncCallback_4_0_0;

        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, outMsgHdl, tempFlags, &(pHandleObj->options), &asyncOptions);
        if (CL_OK != rc)
        {
            goto LL;
         }
    }
    else
    {
        tempFlags |= pHandleObj->flags |
                         (CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT);
        rc = clRmdWithMsgVer(address, &funcVer, funcNo, inMsgHdl, 0, tempFlags, &(pHandleObj->options),NULL);
        if(CL_OK != rc)
        {
               goto L;
        }
    }
    
    
    clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;

LL: clBufferDelete(&outMsgHdl);
L2:  clHeapFree(pCookie);
L:
     clHandleCheckin(gIdlClnt.idlDbHdl,handle);
    return rc;
}


