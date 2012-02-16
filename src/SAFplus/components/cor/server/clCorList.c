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
 * ModuleName  : cor
 * File        : clCorList.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains COR List functions
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>
#include <clCpmApi.h>


/* Internal Headers*/
#include "clCorDmProtoType.h"
#include "clCorRmDefs.h"
#include "clCorLog.h"
#include "clCorRMDWrap.h"

/* all these stuff have to be either removed / moved to globals
 */

static CORHashTable_h corList=0;
static Byte_h         rmBuffer=0;
static ClCorAddrT	  myself={0};
static ClUint32T     rmBufLen=0;
CORHashTable_h        cardMap=0;

/* todo: to fix the hash table issue. right now only
 * consulting the upper portion to locate the cor
 */

/** 
 * Init the list.
 *
 * API to corListInit <Deailed desc>. 
 *
 * @param 
 * 
 * @returns 
 *
 */
ClRcT
corListInit(ClCorCpuIdT id, ClIocPhysicalAddressT myIocAddr)
{
    CL_FUNC_ENTER();

    /* init the hash table */
    HASH_CREATE(&corList);
    HASH_CREATE(&cardMap);

    /* add current COR address to the cor List  */
    myself.nodeAddress = myIocAddr.nodeAddress;
    myself.portId = myIocAddr.portId;
  
    corListAdd(id, myIocAddr, 0);

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT
corListCleanUp(CORHashKey_h   key, 
            CORHashValue_h listBuf, 
            void *         userArg,
            ClUint32T     dataLength
            )
{
	ClUint32T* node = (ClUint32T*)listBuf;
        clHeapFree(node); 	
	return CL_OK;
}


ClRcT
cardMapCleanUp(CORHashKey_h   key, 
            CORHashValue_h cardBuf, 
            void *         userArg,
            ClUint32T     dataLength
            )
{
	ClUint32T* node = (ClUint32T*)cardBuf;
        clHeapFree(node); 	
	return CL_OK;
}


/* Function to delete all the entries from the hash table */
void corListFinalize()
{
	HASH_ITR_ARG(corList, corListCleanUp, NULL, 0);
	HASH_FINALIZE(corList);
	HASH_ITR_ARG(cardMap, cardMapCleanUp, NULL, 0);
	HASH_FINALIZE(cardMap);
}


/** 
 * Add to the cor list.
 *
 * API to corListAdd <Deailed desc>. 
 *
 * @param 
 * 
 * @returns 
 *
 *
 */
ClRcT
corListAdd(ClCorCpuIdT id, ClIocPhysicalAddressT ioc, CORFingerPrint_t inf)
{
    CORInfo_h tmp = NULL;
    ClIocPhysicalAddressT* iocH =0;
    ClRcT ret = CL_OK;
  
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "CORAdd (id:%x, ioc:%x finger print:%x)", 
                                        id, ioc.nodeAddress, inf));

    if (corList != 0) HASH_GET(corList, id, tmp); /* temp for now */
    else return CL_COR_SET_RC(CL_COR_INTERNAL_ERR_INVALID_COR_LIST);

    if(tmp!=0) {
        if(!COR_IS_DISABLED(*tmp)) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "COR Already present & discovered"));
            ret = CL_COR_SET_RC(CL_COR_ERR_ROUTE_PRESENT); 
            CL_FUNC_EXIT();
            return ret;
        }
    }

    /* malloc only if required, otherwise it might be just
     * a simple enable case 
     */
    if(!tmp)
    {
        tmp = (CORInfo_h) clHeapAllocate(sizeof(CORInfo_t));
        iocH = (ClIocPhysicalAddressT*) clHeapAllocate(sizeof(ClIocPhysicalAddressT));

        if(tmp==0 || iocH==0) 
        {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR_MEM_ALLOC_FAIL)); 
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        } 
         /* add to hash table */
         HASH_PUT(corList, id, tmp);
    }

    tmp->comm.addr.nodeAddress = ioc.nodeAddress;
    tmp->comm.addr.portId = ioc.portId;
    tmp->comm.timeout = CL_COR_DEFAULT_TIMEOUT;
    tmp->comm.maxRetries  = CL_COR_DEFAULT_MAX_RETRIES;
    tmp->comm.maxSessions = CL_COR_DEFAULT_MAX_SESSIONS;
    tmp->info = inf;
    tmp->lastContactTime = 0;
    tmp->flags = 0;
      
    COR_ENABLE(*tmp);
  
    if(iocH != 0)
    {
        iocH->nodeAddress = ioc.nodeAddress;
        iocH->portId = ioc.portId;
    
        /* add the card to card map */
        COR_RM_ADD_CARD(id, iocH);
    } 


    CL_FUNC_EXIT();
    return (ret);
}


/*
 * corListDel/corListGet are not in use now.
 * The corList has to be eventually re-worked, when GMS is used.
 */
#if 0
/** 
 * Remove from the cor list.
 *
 * API to remove from the cor list.
 *
 * @param 
 * 
 * @returns 
 *
 *
 */
ClRcT
corListDel(ClCorCpuIdT id)
{
    CORInfo_h tmp;
    ClRcT    ret = CL_OK;
  
    CL_FUNC_ENTER();
    

    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "CORDel (id:%x)", id));

    HASH_GET(corList, id, tmp); /* temp for now */

    if(tmp==0) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "COR List NOT present"));
        ret = CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT); 
    }
    else {
        COR_DISABLE(*tmp);
    }

    CL_FUNC_EXIT();
    return (ret);
}

/** returns the cor info 
 */
CORInfo_h
corListGet(ClCorAddrT id)
{
    CORInfo_h tmp = 0;
    ClIocNodeAddressT corId;
    CL_FUNC_ENTER();
    

    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return NULL;
    }

    corId = id.nodeAddress & 0xFFFF;
	
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "CORGet (id:%04x)", id.nodeAddress));
    HASH_GET(corList, corId, tmp);

    CL_FUNC_EXIT();
    return (tmp);
}
#endif

/* Enable COR 
 * --> for now just enable in the list, but syncup with other
 * cor will happen later
 */

#ifdef COR_TEST
ClRcT
corListCorEnable(ClCorAddrT id)
{
    CORInfo_h tmp = 0;
    ClRcT ret = CL_OK;
  
    CL_FUNC_ENTER();
    
    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "COR Enable (id:%04x)", id.nodeAddress));
    HASH_GET(corList, (id.nodeAddress & 0xffff), tmp);
    if(tmp == 0) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                 ( "COR Enable - Unknown COR referred (id:%04x)", id.nodeAddress));
        ret = CL_COR_SET_RC(CL_COR_ERR_UNKNOWN_COR_INSTANCE);
    }
    else {
        /* todo: check if already enabled ?? */
        COR_ENABLE(*tmp);
    }

    CL_FUNC_EXIT();
    return (ret);
}
#endif

/* Disable COR 
 */
ClRcT
corListCorDisable(ClCorAddrT id)
{
    CORInfo_h tmp = 0;
    ClRcT ret = CL_OK;
  
    CL_FUNC_ENTER();
    
    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "COR Disable (id:%04x)", id.nodeAddress));
    HASH_GET(corList, (id.nodeAddress & 0xffff), tmp);
    if(tmp == 0) {
        CL_DEBUG_PRINT(CL_DEBUG_WARN, 
                ( "Unable to disable COR - no COR Server detected on node (id:%04x)", id.nodeAddress));
        ret = CL_COR_SET_RC(CL_COR_ERR_UNKNOWN_COR_INSTANCE);
    }
    else {
        /* todo: check if already disabled ?? */
        COR_DISABLE(*tmp);
    }

    CL_FUNC_EXIT();
    return (ret);
}

/* function to pack COR Info
 */
ClRcT
corInfoPack(CORHashKey_h key, CORHashValue_h infoBuf, void * userArg, ClUint32T dataLength)
{
    CORInfo_h tmp;
    Byte_h buf = rmBuffer+rmBufLen;
    ClIocPhysicalAddressT* iocH;
    ClIocPhysicalAddressT  ioc;
	ClUint32T tmpPackL = 0;
	ClUint16T tmpPackS = 0;
  
    
    
    if( ( !infoBuf ) || (!rmBuffer ) )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    memset( &ioc, '\0', sizeof( ClIocPhysicalAddressT ) );
    tmp = (CORInfo_h) infoBuf;  

    STREAM_IN_COMM_HTON(buf,tmp->comm, tmpPackL , tmpPackS);
    STREAM_IN_HTONL(buf, &tmp->info, tmpPackL, sizeof(tmp->info));
    STREAM_IN_HTONL(buf, &tmp->lastContactTime, tmpPackL, sizeof(tmp->lastContactTime));
    STREAM_IN_HTONL(buf, &tmp->flags, tmpPackL, sizeof(tmp->flags));
	
    /* STREAM_IN_COMM(buf,tmp->comm);
    STREAM_IN(buf, &tmp->info, sizeof(tmp->info));
    STREAM_IN(buf, &tmp->lastContactTime, sizeof(tmp->lastContactTime));
    STREAM_IN(buf, &tmp->flags, sizeof(tmp->flags)); */
    /* todo: temp fix, streaming ioc address also 
     * along with
     */
    COR_RM_GET_IOC((tmp->comm.addr.nodeAddress & 0xFFFF), iocH);
    ioc.nodeAddress = iocH?iocH->nodeAddress:0;
    ioc.portId = iocH?iocH->portId:0;
    /* STREAM_IN(buf, &ioc, sizeof(ioc)); */
    STREAM_IN_HTONL(buf, &ioc.nodeAddress, tmpPackL, sizeof(ioc.nodeAddress)); 
    STREAM_IN_HTONL(buf, &ioc.portId, tmpPackL, sizeof(ioc.portId));
  
    rmBufLen += (buf - (rmBuffer+rmBufLen));
    return CL_OK;
}

/* to handle unpack COR Info
 * -> also need to handle any merge scenario if necessary
 */
ClUint32T
corInfoUnpack(void * contents, CORInfo_h* cih)
{
    Byte_h      buf = (Byte_h)contents;
    Byte_h      tmpBuf;
    CORInfo_h   tmp;
    ClIocPhysicalAddressT*  iocH;
    ClIocPhysicalAddressT   ioc;
  
    
    if( ( NULL == contents ) || ( NULL == cih ) )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
	tmpBuf = buf;

    /* should not call this */
    *cih = (CORInfo_h) clHeapAllocate(sizeof(CORInfo_t));
    if(*cih == NULL)
    {
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
	 CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
	 return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    tmp  = *cih;
    
    STREAM_OUT_COMM_NTOH(tmp->comm, buf);
    STREAM_OUT_NTOHL(&tmp->info, buf, sizeof(tmp->info));
    STREAM_OUT_NTOHL(&tmp->lastContactTime, buf, sizeof(tmp->lastContactTime));
    STREAM_OUT_NTOHL(&tmp->flags, buf, sizeof(tmp->flags));

    STREAM_OUT_NTOHL(&ioc.nodeAddress, buf, sizeof(ioc.nodeAddress));
    STREAM_OUT_NTOHL(&ioc.portId, buf, sizeof(ioc.portId));
    iocH = (ClIocPhysicalAddressT*) clHeapAllocate(sizeof(ClIocPhysicalAddressT));
    if(iocH)
    {
        ClIocPhysicalAddressT*  pTmpIocH;

        iocH->nodeAddress = ioc.nodeAddress;
        iocH->portId = ioc.portId;
  
        COR_RM_GET_IOC((tmp->comm.addr.nodeAddress &0xFFFF), pTmpIocH);
        if(pTmpIocH)
        {
            clLogTrace("SYN", "LSU", 
                    "Card/CPU Id [%x] to IOC addr map already present! dropped!", 
                    tmp->comm.addr.nodeAddress);
            clHeapFree(iocH);
        }
        else
        {
            COR_RM_ADD_CARD((tmp->comm.addr.nodeAddress & 0xFFFF), iocH);
        }
    }
    else
    {	
        clLogError("SYN", "LSU", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
    }
    
    /*clHeapFree(iocH);*/
    CL_FUNC_EXIT();  
    return (buf - tmpBuf);
}

/**
 * Pack the cor List. Returns the size that have been packed.
 */
ClUint32T
corListPack(void * buf)
{
    ClUint32T size=0;

    CL_FUNC_ENTER();
    
    rmBuffer = buf;

    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(rmBuffer)
    {
        if( rmBufLen > 0 )
        {
            /* already a request is happening, so just return back */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                     ( "COR List, packing is happening, can't handle two packs!"));
        }
        else
        {
            HASH_ITR(corList, corInfoPack);
#ifdef DEBUG
            dmBinaryShow("COR List Packed", rmBuffer, rmBufLen);
#endif
            size = rmBufLen;
            rmBufLen = 0; /* should be reset here ???  */
        }
    }
    CL_FUNC_EXIT();  
    return size;
}


/**
 * Unpacks the cor List
 * todo: need to add to the hash table
 */
ClRcT
corListUnpack(void * contents, ClUint32T size)
{
    ClUint32T i=0;
    Byte_h  buf = (Byte_h)contents;
  
    CL_FUNC_ENTER();

    if(corList && buf) {
#ifdef DEBUG
    dmBinaryShow("COR List Received", buf, size);
#endif
        /** loop thru the byte stream and unpack cor list */
        while(i<size) {
            CORInfo_h tmp; 
            CORInfo_h ci; 

            i+=corInfoUnpack(buf+i, &tmp);

            /* add the unpacked cor info to hash table */
            HASH_GET(corList, (tmp->comm.addr.nodeAddress &0xFFFF), ci);
            if(ci) {
                /* need to do a comparision, but for now simply ignoring!  */
                clLogTrace("SYN", "LSU", 
                         "Address %04x definition is identical!  Replacing!!", 
                         tmp->comm.addr.nodeAddress);
                HASH_REMOVE(corList, (tmp->comm.addr.nodeAddress &0xFFFF));
                clHeapFree(ci);
            } 
            /* add to the hash table */
            HASH_PUT(corList, (tmp->comm.addr.nodeAddress & 0xFFFF), tmp);
        }
#ifdef DEBUG
    corListShow();
#endif
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* 
 * This is the call back function to get the COR information.
 * This function will be called for each of the entry in the COR list.
 */
ClRcT
corListIdGet(CORHashKey_h key, CORHashValue_h infoBuf, void * userArg, ClUint32T dataLength)
{
    CORInfo_h tmp;
    ClCorInfoListT* pCorListInfo = NULL;

    CL_FUNC_ENTER();

    if( !infoBuf )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    tmp = (CORInfo_h) infoBuf;

    pCorListInfo = (ClCorInfoListT *) userArg;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Getting the information from COR list : nodeAddress : [0x%x]", tmp->comm.addr.nodeAddress));

    if(!COR_IS_DISABLED(*tmp))
    {    
        (*(pCorListInfo->pSyncList)) [(*(pCorListInfo->pCount))] = tmp->comm;
        (*(pCorListInfo->pCount))++;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 * This function is used to get the list of COR addresses.
 * Right now this function assumes that only two CORs will be available.
 * It will allocate memory for storing the COR information in the OUT parameter ppSyncList.
 * This needs to be freed by the user.
 */
ClRcT corListIdsGet(ClCorCommInfoT** ppSyncList, ClUint32T* sz)
{
    ClCorInfoListT corInfo;
    ClCorCommInfoT* pSyncList = NULL;
    ClUint32T count = 0;

    CL_FUNC_ENTER();
    
    if ( (sz == NULL) || (ppSyncList == NULL) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* This memory should be freed by the user */
    pSyncList = (ClCorCommInfoT*) clHeapAllocate(2 * sizeof(ClCorCommInfoT));
    if (pSyncList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }

    corInfo.pSyncList = &pSyncList;
    corInfo.pCount = &count;

    /* go through the list and get the list of COR information. */
     
    HASH_ITR_COOKIE(corList, corListIdGet, &corInfo);

    *sz = count;
    *ppSyncList = pSyncList;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/** 
 * Show information about given cor info object.
 *
 * API to corInfoShow <Deailed desc>. 
 *
 * @param 
 * 
 * @returns 
 *
 *
 */
ClRcT
corInfoShow(CORHashKey_h key, CORHashValue_h buf, void * userArg, ClUint32T dataLength)
{
    CORInfo_h tmp;
    ClBufferHandleT *pMsg = (ClBufferHandleT *)userArg;
    ClCharT corStr[CL_COR_CLI_STR_LEN];

  
    
    if( !buf )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr, "Input variable is NULL");
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
         }
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    tmp = (CORInfo_h) buf;
   
        if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr, "%04x",(int)(ClWordT)key);
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
         }
        if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr,"  [%04x:%04x %xms,retry:%d,%x]", 
           tmp->comm.addr.nodeAddress,
           tmp->comm.addr.portId, 
           tmp->comm.timeout, 
           tmp->comm.maxRetries,
           tmp->comm.maxSessions);
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
         }
        if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr, "%c  %04x    %05x    %04X \n",
             COR_ADDR_IS_EQUAL(tmp->comm.addr,myself)?'*':32,
             tmp->info,
             tmp->lastContactTime,
             tmp->flags);

         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
         }
    return CL_OK;
}


/** 
 * Show Card info.
 *
 * API to Show card info.
 *
 * @param 
 * 
 * @returns 
 *
 *
 */
#if 0
ClRcT
cardInfoShow(CORHashKey_h key, CORHashValue_h buf, void * userArg, ClUint32T dataLength)
{
    ClIocPhysicalAddressT* ioc;
    ClBufferHandleT *pMsg = (ClBufferHandleT *)userArg;
    ClCharT corStr[CL_COR_CLI_STR_LEN];
                                                                                                                             
    
    if( !buf )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
  
       if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr, "Input variable is NULL");
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
         }
  return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    ioc = (ClIocPhysicalAddressT*)buf;
       if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr," [%06x:%04x]", (*ioc).nodeAddress, (*ioc).portId);
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
        }
       if(userArg!= 0)
        {
             corStr[0]='\0';
         sprintf(corStr, "%c    %04x\n",COR_ADDR_IS_EQUAL(*ioc,myself)?'*':32,
             (int)key);
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
        }
    return CL_OK;
}

#endif
/* show list of cor's 
 */
ClRcT
corListShow(ClBufferHandleT* pMsgHdl)
{
    ClCharT corStr[CL_COR_CLI_STR_LEN];


    if( !corList )
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
         corStr[0]='\0';
         sprintf(corStr, "Input variable is NULL");
         clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    corStr[0]='\0';
    sprintf(corStr, "\n===========================================================\n"
           "%20c -- COR List -- \n COR-Id    COR-Comm-Info        FingerPrint Contact  Flags\n"
           "===========================================================\n",32);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    
  /*  HASH_ITR(corList, corInfoShow); */
    HASH_ITR_COOKIE(corList, corInfoShow, pMsgHdl);
	#if 0
    corStr[0]='\0';
    sprintf(corStr, "\n===========================================================\n"
           "%20c -- Card List -- \n     Ioc-Info      COR-Id\n"
           "===========================================================\n",32);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
   /* HASH_ITR(cardMap, cardInfoShow);*/
    HASH_ITR_COOKIE(cardMap, cardInfoShow, pMsgHdl);
    corStr[0]='\0';
    sprintf(corStr, "\n===========================================================\n");
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
	#endif
    return CL_OK;
}

ClRcT
clCorSlaveCorListGet(CORHashKey_h key, CORHashValue_h infoBuf, void * userArg, ClUint32T dataLength)
{
    ClRcT               rc = CL_OK;
	CORInfo_h           corInfo;
	ClIocNodeAddressT   *nodeAddressId = (ClUint32T *)userArg;	
	ClIocNodeAddressT   nodeId ;	
	
	corInfo = (CORInfo_h)infoBuf;
	CL_CPM_MASTER_ADDRESS_GET(&nodeId);
    if (CL_OK != rc)
    {
        clLogError("SLG", CL_LOG_CONTEXT_UNSPECIFIED, "Failed to get the master address. rc[0x%x]", rc);
        return rc;
    }
	
	if( corInfo->comm.addr.nodeAddress != nodeId && corInfo->flags != 0x8000)			
			*nodeAddressId = corInfo->comm.addr.nodeAddress;
	
	return CL_OK;
}

ClRcT clCorSlaveIdGet(ClIocNodeAddressT *iocNodeAdd )
{
	ClIocNodeAddressT addr = 0;
    HASH_ITR_COOKIE(corList, clCorSlaveCorListGet, &addr);
	*iocNodeAdd = addr;
	
	return CL_OK;
}

