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
#include "ckptEockptServerActivePeerServer.h"
#include "ckptEoServer.h"

extern ClUint32T  ckptEoidlSyncKey;
extern ClHandleDatabaseHandleT  ckptEoidlDatabaseHdl;



ClRcT clCkptRemSvrCkptInfoSyncServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptActHdl;
    ClNameT  pCkptName;
    CkptCPInfoT_4_0_0  pCpInfo;
    CkptDPInfoT_4_0_0  pDpInfo;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptActHdl), 0, sizeof(ClHandleT));
    memset(&(pCkptName), 0, sizeof(ClNameT));
    memset(&(pCpInfo), 0, sizeof(CkptCPInfoT_4_0_0));
    memset(&(pDpInfo), 0, sizeof(CkptDPInfoT_4_0_0));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClNameT( inMsgHdl,&(pCkptName));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallCkptCPInfoT_4_0_0( inMsgHdl,&(pCpInfo));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallCkptDPInfoT_4_0_0( inMsgHdl,&(pDpInfo));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
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
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrCkptInfoSync_4_0_0(&(pVersion), ckptActHdl, &(pCkptName), &(pCpInfo), &(pDpInfo));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClNameT(&(pCkptName), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallCkptCPInfoT_4_0_0(&(pCpInfo), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallCkptDPInfoT_4_0_0(&(pDpInfo), 0, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

L5:    return rc;

LL4:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL3:  clXdrMarshallCkptDPInfoT_4_0_0(&(pDpInfo), 0, 1);
LL2:  clXdrMarshallCkptCPInfoT_4_0_0(&(pCpInfo), 0, 1);
LL1:  clXdrMarshallClNameT(&(pCkptName), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
L1:  clXdrMarshallClNameT(&(pCkptName), 0, 1);
L2:  clXdrMarshallCkptCPInfoT_4_0_0(&(pCpInfo), 0, 1);
L3:  clXdrMarshallCkptDPInfoT_4_0_0(&(pDpInfo), 0, 1);

L4:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrCkptInfoSyncResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L5:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptRemSvrCkptInfoGetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptActHdl;
    ClUint32T  peerAddr;
    CkptInfoT_4_0_0  pCkptInfo;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptActHdl), 0, sizeof(ClHandleT));
    memset(&(peerAddr), 0, sizeof(ClUint32T));
    memset(&(pCkptInfo), 0, sizeof(CkptInfoT_4_0_0));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(peerAddr));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrCkptInfoGet_4_0_0(&(pVersion), ckptActHdl, peerAddr, &(pCkptInfo));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(peerAddr), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallCkptInfoT_4_0_0(&(pCkptInfo), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

L4:    return rc;

LL2:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL1:  clXdrMarshallClUint32T(&(peerAddr), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
L1:  clXdrMarshallClUint32T(&(peerAddr), 0, 1);

L2:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
L3:  clXdrMarshallCkptInfoT_4_0_0(&(pCkptInfo), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrCkptInfoGetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion,CL_OUT  CkptInfoT_4_0_0  pCkptInfo)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallCkptInfoT_4_0_0(&(pCkptInfo), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    goto Label1; 
L3:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
L4:  clXdrMarshallCkptInfoT_4_0_0(&(pCkptInfo), 0, 1);

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
    return rc;
Label1:
    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptRemSvrSectionInfoUpdateServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptActHdl;
    CkptUpdateFlagT_4_0_0  updateFlag;
    CkptSectionInfoT_4_0_0  pSecInfo;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptActHdl), 0, sizeof(ClHandleT));
    memset(&(updateFlag), 0, sizeof(CkptUpdateFlagT_4_0_0));
    memset(&(pSecInfo), 0, sizeof(CkptSectionInfoT_4_0_0));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallCkptUpdateFlagT_4_0_0( inMsgHdl,&(updateFlag));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallCkptSectionInfoT_4_0_0( inMsgHdl,&(pSecInfo));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrSectionInfoUpdate_4_0_0(&(pVersion), ckptActHdl, updateFlag, &(pSecInfo));
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallCkptUpdateFlagT_4_0_0(&(updateFlag), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallCkptSectionInfoT_4_0_0(&(pSecInfo), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
		return rc;
    }

LL3:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL2:  clXdrMarshallCkptSectionInfoT_4_0_0(&(pSecInfo), 0, 1);
LL1:  clXdrMarshallCkptUpdateFlagT_4_0_0(&(updateFlag), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
L1:  clXdrMarshallCkptUpdateFlagT_4_0_0(&(updateFlag), 0, 1);
L2:  clXdrMarshallCkptSectionInfoT_4_0_0(&(pSecInfo), 0, 1);

L3:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrSectionInfoUpdateResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L4:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptRemSvrCkptDeleteServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptActHdl;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptActHdl), 0, sizeof(ClHandleT));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
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
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrCkptDelete_4_0_0(&(pVersion), ckptActHdl);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

L2:    return rc;

LL1:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActHdl), 0, 1);

L1:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrCkptDeleteResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L2:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptRemSvrCkptWriteServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptHandle;
    ClUint32T  nodeAddr;
    ClUint32T  portId;
    ClUint32T  numberOfElements;
    ClCkptIOVectorElementT_4_0_0*  pIoVector;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptHandle), 0, sizeof(ClHandleT));
    memset(&(nodeAddr), 0, sizeof(ClUint32T));
    memset(&(portId), 0, sizeof(ClUint32T));
    memset(&(numberOfElements), 0, sizeof(ClUint32T));
    memset(&(pIoVector), 0, sizeof(ClCkptIOVectorElementT_4_0_0*));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptHandle));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(nodeAddr));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(portId));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(numberOfElements));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    rc = clXdrUnmarshallPtrClCkptIOVectorElementT_4_0_0( inMsgHdl,(void**)&(pIoVector), numberOfElements);
    if (CL_OK != rc)
    {
        goto LL4;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
    if (CL_OK != rc)
    {
        goto LL5;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrCkptWrite_4_0_0(&(pVersion), ckptHandle, nodeAddr, portId, numberOfElements, pIoVector);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptHandle), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(nodeAddr), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallClUint32T(&(portId), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    rc = clXdrMarshallClUint32T(&(numberOfElements), 0, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    rc = clXdrMarshallPtrClCkptIOVectorElementT_4_0_0(pIoVector, numberOfElements, 0, 1);
    if (CL_OK != rc)
    {
        goto L5;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L6;
    }

L6:    return rc;

LL5:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL4:  clXdrMarshallPtrClCkptIOVectorElementT_4_0_0(pIoVector, numberOfElements, 0, 1);
LL3:  clXdrMarshallClUint32T(&(numberOfElements), 0, 1);
LL2:  clXdrMarshallClUint32T(&(portId), 0, 1);
LL1:  clXdrMarshallClUint32T(&(nodeAddr), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptHandle), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptHandle), 0, 1);
L1:  clXdrMarshallClUint32T(&(nodeAddr), 0, 1);
L2:  clXdrMarshallClUint32T(&(portId), 0, 1);
L3:  clXdrMarshallClUint32T(&(numberOfElements), 0, 1);
L4:  clXdrMarshallPtrClCkptIOVectorElementT_4_0_0(pIoVector, numberOfElements, 0, 1);

L5:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrCkptWriteResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L6;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L6:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptRemSvrSectionExpTimeSetServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClVersionT  pVersion;
    ClHandleT  ckptHdl;
    ClCkptSectionIdT_4_0_0  sectionId;
    ClInt64T  exprTime;

    memset(&(pVersion), 0, sizeof(ClVersionT));
    memset(&(ckptHdl), 0, sizeof(ClHandleT));
    memset(&(sectionId), 0, sizeof(ClCkptSectionIdT_4_0_0));
    memset(&(exprTime), 0, sizeof(ClInt64T));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClCkptSectionIdT_4_0_0( inMsgHdl,&(sectionId));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClInt64T( inMsgHdl,&(exprTime));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    rc = clXdrUnmarshallClVersionT( inMsgHdl,&(pVersion));
    if (CL_OK != rc)
    {
        goto LL3;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptRemSvrSectionExpTimeSet_4_0_0(&(pVersion), ckptHdl, &(sectionId), exprTime);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClCkptSectionIdT_4_0_0(&(sectionId), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallClInt64T(&(exprTime), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

L4:    return rc;

LL3:  clXdrMarshallClVersionT(&(pVersion), 0, 1);
LL2:  clXdrMarshallClInt64T(&(exprTime), 0, 1);
LL1:  clXdrMarshallClCkptSectionIdT_4_0_0(&(sectionId), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptHdl), 0, 1);
L1:  clXdrMarshallClCkptSectionIdT_4_0_0(&(sectionId), 0, 1);
L2:  clXdrMarshallClInt64T(&(exprTime), 0, 1);

L3:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    return rc;
}

ClRcT clCkptRemSvrSectionExpTimeSetResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode,CL_INOUT  ClVersionT  pVersion)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clXdrMarshallClVersionT(&(pVersion), outMsgHdl, 1);
    if (CL_OK != rc)
    {
        goto L4;
    }

    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    
L4:  clXdrMarshallClVersionT(&(pVersion), 0, 1);

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptCheckpointReplicaRemoveServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClHandleT  ckptActiveHdl;
    ClUint32T  replicaAddr;

    memset(&(ckptActiveHdl), 0, sizeof(ClHandleT));
    memset(&(replicaAddr), 0, sizeof(ClUint32T));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActiveHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(replicaAddr));
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
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptCheckpointReplicaRemove_4_0_0(ckptActiveHdl, replicaAddr);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
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

LL1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
L1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);


    return rc;
}

ClRcT clCkptCheckpointReplicaRemoveResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptReplicaAppInfoNotifyServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClHandleT  ckptActiveHdl;
    ClUint32T  replicaAddr;
    ClUint32T  portId;

    memset(&(ckptActiveHdl), 0, sizeof(ClHandleT));
    memset(&(replicaAddr), 0, sizeof(ClUint32T));
    memset(&(portId), 0, sizeof(ClUint32T));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActiveHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(replicaAddr));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(portId));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptReplicaAppInfoNotify_4_0_0(ckptActiveHdl, replicaAddr, portId);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallClUint32T(&(portId), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
L3:    return rc;

LL2:  clXdrMarshallClUint32T(&(portId), 0, 1);
LL1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
L1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
L2:  clXdrMarshallClUint32T(&(portId), 0, 1);


    return rc;
}

ClRcT clCkptReplicaAppInfoNotifyResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

ClRcT clCkptActiveCallbackNotifyServer_4_0_0(ClEoDataT eoData, ClBufferHandleT inMsgHdl, ClBufferHandleT outMsgHdl)
{
    ClIdlContextInfoT *pIdlCtxInfo = NULL;
    ClRcT rc = CL_OK;
    ClHandleT  ckptActiveHdl;
    ClUint32T  replicaAddr;
    ClUint32T  portId;

    memset(&(ckptActiveHdl), 0, sizeof(ClHandleT));
    memset(&(replicaAddr), 0, sizeof(ClUint32T));
    memset(&(portId), 0, sizeof(ClUint32T));


    rc = clXdrUnmarshallClHandleT( inMsgHdl,&(ckptActiveHdl));
    if (CL_OK != rc)
    {
        goto LL0;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(replicaAddr));
    if (CL_OK != rc)
    {
        goto LL1;
    }

    rc = clXdrUnmarshallClUint32T( inMsgHdl,&(portId));
    if (CL_OK != rc)
    {
        goto LL2;
    }

    pIdlCtxInfo = (ClIdlContextInfoT *)clHeapAllocate(sizeof(ClIdlContextInfoT));
    if(pIdlCtxInfo == NULL)
    {
       return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }
    memset(pIdlCtxInfo, 0, sizeof(ClIdlContextInfoT));
    pIdlCtxInfo->idlDeferMsg = outMsgHdl; 
    pIdlCtxInfo->inProgress  = CL_FALSE;
    rc = clIdlSyncPrivateInfoSet(ckptEoidlSyncKey, (void *)pIdlCtxInfo);
    if (CL_OK != rc)
    {
        clHeapFree(pIdlCtxInfo);
        goto L0;
    }
    rc = clCkptActiveCallbackNotify_4_0_0(ckptActiveHdl, replicaAddr, portId);
    if(pIdlCtxInfo->inProgress == CL_FALSE)
    {
      clHeapFree(pIdlCtxInfo);
      pIdlCtxInfo = NULL;
    }
    if (CL_OK != rc)
    {
       goto L0;
    }
    
    rc = clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
    if (CL_OK != rc)
    {
        goto L2;
    }

    rc = clXdrMarshallClUint32T(&(portId), 0, 1);
    if (CL_OK != rc)
    {
        goto L3;
    }

    if(pIdlCtxInfo != NULL)
    {
      clHeapFree(pIdlCtxInfo);
      return rc;
    }
    
L3:    return rc;

LL2:  clXdrMarshallClUint32T(&(portId), 0, 1);
LL1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
LL0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);

    return rc;

L0:  clXdrMarshallClHandleT(&(ckptActiveHdl), 0, 1);
L1:  clXdrMarshallClUint32T(&(replicaAddr), 0, 1);
L2:  clXdrMarshallClUint32T(&(portId), 0, 1);


    return rc;
}

ClRcT clCkptActiveCallbackNotifyResponseSend_4_0_0(ClIdlHandleT idlHdl,ClRcT retCode)
{
    ClIdlSyncInfoT    *pIdlSyncDeferInfo = NULL;
    ClRcT              rc                = CL_OK;
    ClBufferHandleT outMsgHdl     = 0;
    
    rc = clHandleCheckout(ckptEoidlDatabaseHdl,idlHdl,(void **)&pIdlSyncDeferInfo);
    if( rc != CL_OK)
    {
      goto Label0; 
    }
    outMsgHdl = pIdlSyncDeferInfo->idlRmdDeferMsg;
    
    rc = clIdlSyncResponseSend(pIdlSyncDeferInfo->idlRmdDeferHdl,outMsgHdl,
                                retCode);
    

    

    clHandleCheckin(ckptEoidlDatabaseHdl, idlHdl);
    clHandleDestroy(ckptEoidlDatabaseHdl, idlHdl);
Label0:
    return rc;
}

