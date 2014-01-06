/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description :Server Stub routines
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/
#include <netinet/in.h>
#include <clBufferApi.h>
#include <clRmdApi.h>
#include <clEoApi.h>
#include <ipi/clRmdIpi.h>
#include <string.h>
#include "LogPortMasterServer.h"
#include "LogServer.h"

extern ClUint32T  LogidlSyncKey;
extern ClHandleDatabaseHandleT  LogidlDatabaseHdl;



ClRcT clLogMasterAttrVerifyNGetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
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


    rc = clXdrUnmarshallClLogStreamAttrIDLT_4_0_0( inMsgHdl,&(pStreamAttr));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallSaNameT( inMsgHdl,&(pStreamName));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClLogStreamScopeT_4_0_0( inMsgHdl,&(pStreamScope));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallSaNameT( inMsgHdl,&(pStreamScopeNode));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    rc = clXdrUnmarshallClUint16T( inMsgHdl,&(pStreamId));
    if (CL_OK != rc)
    {
        goto LL4;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterAttrVerifyNGet_4_0_0(&(pStreamAttr), &(pStreamName), &(pStreamScope), &(pStreamScopeNode), &(pStreamId), &(pStreamMcastAddr));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClLogStreamAttrIDLT_4_0_0(&(pStreamAttr), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallSaNameT(&(pStreamName), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClUint16T(&(pStreamId), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

    rc = clXdrMarshallClUint64T(&(pStreamMcastAddr), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L6;
    }

L6:    return rc;

LL4:  clXdrMarshallClUint16T(&(pStreamId), 0, 1);
LL3:  clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);
LL2:  clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
LL1:  clXdrMarshallSaNameT(&(pStreamName), 0, 1);
LL0:  clXdrMarshallClLogStreamAttrIDLT_4_0_0(&(pStreamAttr), 0, 1);

    return rc;

L0:  clXdrMarshallClLogStreamAttrIDLT_4_0_0(&(pStreamAttr), 0, 1);

L1:  clXdrMarshallSaNameT(&(pStreamName), 0, 1);
L2:  clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
L3:  clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);
L4:  clXdrMarshallClUint16T(&(pStreamId), 0, 1);
L5:  clXdrMarshallClUint64T(&(pStreamMcastAddr), 0, 1);

    return rc;
}

ClRcT clLogMasterAttrVerifyNGetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClUint16T  pStreamId,CL_OUT  ClUint64T  pStreamMcastAddr)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClUint16T(&(pStreamId), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

    rc = clXdrMarshallClUint64T(&(pStreamMcastAddr), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L6;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    goto Label1; 
L5:  clXdrMarshallClUint16T(&(pStreamId), 0, 1);
L6:  clXdrMarshallClUint64T(&(pStreamMcastAddr), 0, 1);

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
    return rc;
Label1:
    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clLogMasterStreamCloseNotifyServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
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


    rc = clXdrUnmarshallClStringT( inMsgHdl,&(pFileName));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClStringT( inMsgHdl,&(pFileLocation));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallSaNameT( inMsgHdl,&(pStreamName));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallClLogStreamScopeT_4_0_0( inMsgHdl,&(pStreamScope));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    rc = clXdrUnmarshallSaNameT( inMsgHdl,&(pStreamScopeNode));
    if (CL_OK != rc)
    {
        goto LL4;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterStreamCloseNotify_4_0_0(&(pFileName), &(pFileLocation), &(pStreamName), pStreamScope, &(pStreamScopeNode));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClStringT(&(pFileName), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClStringT(&(pFileLocation), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallSaNameT(&(pStreamName), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    rc = clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
L5:    return rc;

LL4:  clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);
LL3:  clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
LL2:  clXdrMarshallSaNameT(&(pStreamName), 0, 1);
LL1:  clXdrMarshallClStringT(&(pFileLocation), 0, 1);
LL0:  clXdrMarshallClStringT(&(pFileName), 0, 1);

    return rc;

L0:  clXdrMarshallClStringT(&(pFileName), 0, 1);
L1:  clXdrMarshallClStringT(&(pFileLocation), 0, 1);
L2:  clXdrMarshallSaNameT(&(pStreamName), 0, 1);
L3:  clXdrMarshallClLogStreamScopeT_4_0_0(&(pStreamScope), 0, 1);
L4:  clXdrMarshallSaNameT(&(pStreamScopeNode), 0, 1);


    return rc;
}

ClRcT clLogMasterStreamCloseNotifyResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    

    

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clLogMasterStreamListGetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClUint32T  pNumStreams;
    ClUint32T  pBuffLen;
    ClUint8T*  pBuffer;

    memset(&(pNumStreams), 0, sizeof(ClUint32T));
    memset(&(pBuffLen), 0, sizeof(ClUint32T));
    memset(&(pBuffer), 0, sizeof(ClUint8T*));



    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterStreamListGet_4_0_0(&(pNumStreams), &(pBuffLen), &pBuffer);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClUint32T(&(pNumStreams), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(pBuffLen), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

L3:    return rc;


    return rc;


L0:  clXdrMarshallClUint32T(&(pNumStreams), 0, 1);
L1:  clXdrMarshallClUint32T(&(pBuffLen), 0, 1);
L2:  clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, 0, 1);

    return rc;
}

ClRcT clLogMasterStreamListGetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_OUT  ClUint32T  pNumStreams,CL_OUT  ClUint32T  pBuffLen,ClUint8T*  pBuffer)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClUint32T(&(pNumStreams), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(pBuffLen), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    goto Label1; 
L1:  clXdrMarshallClUint32T(&(pNumStreams), 0, 1);
L2:  clXdrMarshallClUint32T(&(pBuffLen), 0, 1);
L3:  clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, 0, 1);

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
    return rc;
Label1:
    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clLogMasterCompIdChkNGetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    SaNameT  pCompName;
    ClUint32T  pClientId;

    memset(&(pCompName), 0, sizeof(SaNameT));
    memset(&(pClientId), 0, sizeof(ClUint32T));


    rc = clXdrUnmarshallSaNameT( inMsgHdl,&(pCompName));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(pClientId));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterCompIdChkNGet_4_0_0(&(pCompName), &(pClientId));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallSaNameT(&(pCompName), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClUint32T(&(pClientId), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

L2:    return rc;

LL1:  clXdrMarshallClUint32T(&(pClientId), 0, 1);
LL0:  clXdrMarshallSaNameT(&(pCompName), 0, 1);

    return rc;

L0:  clXdrMarshallSaNameT(&(pCompName), 0, 1);

L1:  clXdrMarshallClUint32T(&(pClientId), 0, 1);

    return rc;
}

ClRcT clLogMasterCompIdChkNGetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClUint32T  pClientId)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClUint32T(&(pClientId), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L2:  clXdrMarshallClUint32T(&(pClientId), 0, 1);

    

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clLogMasterCompListGetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClUint32T  pNumStreams;
    ClUint32T  pBuffLen;
    ClUint8T*  pBuffer;

    memset(&(pNumStreams), 0, sizeof(ClUint32T));
    memset(&(pBuffLen), 0, sizeof(ClUint32T));
    memset(&(pBuffer), 0, sizeof(ClUint8T*));



    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterCompListGet_4_0_0(&(pNumStreams), &(pBuffLen), &pBuffer);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClUint32T(&(pNumStreams), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(pBuffLen), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

L3:    return rc;


    return rc;


L0:  clXdrMarshallClUint32T(&(pNumStreams), 0, 1);
L1:  clXdrMarshallClUint32T(&(pBuffLen), 0, 1);
L2:  clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, 0, 1);

    return rc;
}

ClRcT clLogMasterCompListGetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_OUT  ClUint32T  pNumStreams,CL_OUT  ClUint32T  pBuffLen,ClUint8T*  pBuffer)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClUint32T(&(pNumStreams), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(pBuffLen), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    goto Label1; 
L1:  clXdrMarshallClUint32T(&(pNumStreams), 0, 1);
L2:  clXdrMarshallClUint32T(&(pBuffLen), 0, 1);
L3:  clXdrMarshallPtrClUint8T(pBuffer, pBuffLen, 0, 1);

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
    return rc;
Label1:
    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clLogMasterCompListNotifyServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClUint32T  pNumEntries;
    ClLogCompDataT_4_0_0*  pCompData;

    memset(&(pNumEntries), 0, sizeof(ClUint32T));
    memset(&(pCompData), 0, sizeof(ClLogCompDataT_4_0_0*));


    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(pNumEntries));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallPtrClLogCompDataT_4_0_0( inMsgHdl,(void**)&(pCompData), pNumEntries);
    if (CL_OK != rc)
    {
        goto LL1;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(LogidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clLogMasterCompListNotify_4_0_0(pNumEntries, pCompData);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClUint32T(&(pNumEntries), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallPtrClLogCompDataT_4_0_0(pCompData, pNumEntries, 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
L2:    return rc;

LL1:  clXdrMarshallPtrClLogCompDataT_4_0_0(pCompData, pNumEntries, 0, 1);
LL0:  clXdrMarshallClUint32T(&(pNumEntries), 0, 1);

    return rc;

L0:  clXdrMarshallClUint32T(&(pNumEntries), 0, 1);
L1:  clXdrMarshallPtrClLogCompDataT_4_0_0(pCompData, pNumEntries, 0, 1);


    return rc;
}

ClRcT clLogMasterCompListNotifyResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(LogidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    

    

    clHandleCheckin(LogidlDatabaseHdl, idlHdl);
    clHandleDestroy(LogidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

