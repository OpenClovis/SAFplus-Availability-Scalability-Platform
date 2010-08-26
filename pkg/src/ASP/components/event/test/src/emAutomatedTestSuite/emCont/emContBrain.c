/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emCont/emContBrain.c $
 * $Author: bkpavan $
 * $Date: 2006/11/10 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include "string.h"
#include "clDebugApi.h"
#include "clCpmApi.h"
#include "clCpmIpi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include "clEventApi.h"
#include "clEventExtApi.h"
#include "emTestTemplate.h"
#include "../common/emAutoCommon.h"
#include "emCont.h"

#include <unistd.h>

ClCntHandleT gEvtContSubInfo = 0;
ClCntHandleT gEvtContPubInfo = 0;
ClCntHandleT gEvtContSubTempInfo = 0;

void clEvtContResultPrint(ClRcT retCode, ClEvtContTestHeadT *pTestHead)
{

    switch (pTestHead->operation)
    {
        case CL_EVT_CONT_INIT:
            clOsalPrintf("INITIALIZE ::  ");
            break;
        case CL_EVT_CONT_FIN:
            clOsalPrintf("FINALIZE :: ");
            break;
        case CL_EVT_CONT_OPEN:
            clOsalPrintf("OPEN :: ");
            break;
        case CL_EVT_CONT_CLOSE:
            clOsalPrintf("CLOSE :: ");
            break;
        case CL_EVT_CONT_SUB:
            clOsalPrintf("SUBSCRIBE :: ");
            break;
        case CL_EVT_CONT_UNSUB:
            clOsalPrintf("UNSUBSCRIBE :: ");
            break;
        case CL_EVT_CONT_PUB:
            clOsalPrintf("PUBLISH :: ");
            break;
        case CL_EVT_CONT_ALLOC:
            clOsalPrintf("ALLOCATE :: ");
            break;
        case CL_EVT_CONT_SET_ATTR:
            clOsalPrintf("SET :: \t");
            break;
        case CL_EVT_CONT_GET_ATTR:
            clOsalPrintf("GET :: \t");
            break;
        case CL_EVT_CONT_KILL:
            clOsalPrintf("KILL :: ");
            break;
        case CL_EVT_CONT_RESTART:
            clOsalPrintf("RESTART :: ");
            break;
    }

    clOsalPrintf("\t\t\t @ %s ", pTestHead->testAppName.value);
    if (retCode != pTestHead->expectedResult)
    {
        clOsalPrintf("\t\t :: FAILED \r\n");
    }
    else
    {
        clOsalPrintf("\t\t :: PASSED \r\n");
    }

    return;
}

void clEvtContUtilsNameCpy(ClNameT *pNameDst, const ClNameT *pNameSrc)
{
    CL_FUNC_ENTER();

    pNameDst->length = pNameSrc->length;
    memcpy(pNameDst->value, pNameSrc->value, pNameSrc->length + 1);

    CL_FUNC_EXIT();
    return;
}

ClInt32T clEvtContUtilsNameCmp(ClNameT *pName1, ClNameT *pName2)
{
    ClUint32T result;

    result = pName1->length - pName2->length;

    CL_FUNC_ENTER();

    if (0 == result)
    {
        result = memcmp(pName1->value, pName2->value, pName1->length);
        CL_FUNC_EXIT();
        return result;
    }
    CL_FUNC_EXIT();
    return result;
}

ClRcT clEvtContRmd(ClEvtContTestHeadT *pTestHead, ClUint32T funcNo,
                   void *pInData, ClUint32T dataLen, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;
    ClIocAddressT destAddr;
    ClRmdOptionsT rmdOptions = { 0 };
    ClBufferHandleT inMsg;
    ClBufferHandleT outMsg;
    ClUint32T readLen = sizeof(ClUint32T);


    (void) clBufferCreate(&inMsg);
    (void) clBufferCreate(&outMsg);


    clBufferNBytesWrite(inMsg, (ClUint8T *) pInData, dataLen);

    rc = clEvtContIocAddreGet(&pTestHead->testAppName, &destAddr.iocPhyAddress);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("IOCAddrGet Failed:: [%s]\r\n",
                        pTestHead->testAppName.value));
        return rc;
    }

    rmdOptions.priority = 0;
    rmdOptions.retries = 0;
    rmdOptions.timeout = 1000 * 5;

    rc = clRmdWithMsg(destAddr, funcNo, inMsg, outMsg, CL_RMD_CALL_NEED_REPLY,
                      &rmdOptions, NULL);
    if (CL_OK != rc)
    {
        return rc;
    }

    clBufferNBytesRead(outMsg, (ClUint8T *) pRetCode, &readLen);
    return CL_OK;
}

ClRcT clEvtContInit(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_INIT,
                      (void *) pTestHead->pTestInfo, sizeof(ClNameT), pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD->INIT failed  :: [0x%X]\r\n", rc));
        return rc;
    }


    return CL_OK;
}

ClRcT clEvtContFin(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_FIN, (void *) pTestHead->pTestInfo,
                      sizeof(ClNameT), pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD->FIN failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvtContOpen(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_OPEN,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContChOpenT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD->Open failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvntContClose(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_CLOSE,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContChCloseT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("RMD->Close failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvntContSub(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;
    ClEvtContSubKey *pKey;
    ClEvtContSubT *pSub = (ClEvtContSubT *) pTestHead->pTestInfo;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_SUB, (void *) pTestHead->pTestInfo,
                      sizeof(ClEvtContSubT), pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD->SUB failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    pKey = clHeapAllocate(sizeof(ClEvtContSubKey));

    clEvtContUtilsNameCpy(&pKey->channelName, &pSub->channelName);
    pKey->filterNo = pSub->filterNo;

    rc = clCntNodeAdd(gEvtContSubInfo, (ClCntKeyHandleT) pKey,
                      (ClCntDataHandleT) pTestHead, NULL);

    return CL_OK;
}

ClRcT clEvntContUnsub(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_UNSUB,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContUnsubT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("RMD->UNSUB failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvtContSubInfoWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                           ClCntArgHandleT userArg, ClUint32T dataLength)
{

    ClEvtContTestHeadT *pTestHead = (ClEvtContTestHeadT *) userData;



    clEvtContSubInfoInc(&pTestHead->testAppName);

    return CL_OK;
}

ClRcT clEvtContRest(ClIocAddressT destAddr)
{
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = { 0 };
    ClBufferHandleT inMsg;
    ClUint32T readLen = sizeof(ClEvtContResultT);

    (void) clBufferCreate(&inMsg);


    clBufferNBytesWrite(inMsg, (ClUint8T *) &readLen, sizeof(ClUint32T));
    rmdOptions.priority = 0;
    rmdOptions.retries = 0;
    rmdOptions.timeout = 1000 * 5;

    rc = clRmdWithMsg(destAddr, EM_TEST_APP_RESTART, inMsg, 0, 0, &rmdOptions,
                      NULL);
    if (CL_OK != rc)
    {
        return rc;
    }

    return CL_OK;
}

ClRcT clEvtContSubResultGet(ClIocAddressT destAddr, ClUint32T noOfSubs)
{
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = { 0 };
    ClBufferHandleT inMsg;
    ClBufferHandleT outMsg;
    ClUint32T readLen = sizeof(ClEvtContResultT);
    ClEvtContResultT result;

    (void) clBufferCreate(&inMsg);
    (void) clBufferCreate(&outMsg);


    clBufferNBytesWrite(inMsg, (ClUint8T *) &readLen, sizeof(ClUint32T));
    rmdOptions.priority = 0;
    rmdOptions.retries = 0;
    rmdOptions.timeout = 1000 * 5;

    rc = clRmdWithMsg(destAddr, EM_TEST_APP_RESULT_GET, inMsg, outMsg,
                      CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    if (CL_OK != rc)
    {
        return rc;
    }

    clBufferNBytesRead(outMsg, (ClUint8T *) &result, &readLen);

    clOsalPrintf
        ("Got the result ................. local [%d]\t ::: Remote :: [%d %d] \r\n",
         noOfSubs, result.noOfSubs, result.priority);

    return CL_OK;
}

ClEvtContPubT *gpPubInfo;
ClEvtContPubKey gEvtContPubKey;
ClRcT clEvtContPrepareInfo(ClEvtContTestHeadT *pTestHead,
                           ClEvtContPubT *pPubInfo)
{
    ClRcT rc = CL_OK;
    ClEvtContPubKey pubKey;
    ClEvtContAttrSetT *pAttr;

    gpPubInfo = pPubInfo;
    clEvtContUtilsNameCpy(&pubKey.appName, &pTestHead->testAppName);
    clEvtContUtilsNameCpy(&pubKey.channelName, &pPubInfo->channelName);

    rc = clCntDataForKeyGet(gEvtContPubInfo, (ClCntKeyHandleT) &pubKey,
                            (ClCntDataHandleT *) &pAttr);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Not able to get the attriute info from containter ::: [0x%X]\r\n",
                        rc));
    }

    rc = clCntWalk(gEvtContSubInfo, clEvtContSubInfoWalk,
                   (ClCntArgHandleT) NULL, 0);

    return CL_OK;
}



ClRcT clEvntContPub(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;
    ClEvtContPubT *pPubInfo = (ClEvtContPubT *) pTestHead->pTestInfo;

    clEvtContAppInfoReset();
    rc = clEvtContPrepareInfo(pTestHead, pPubInfo);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_PUB, (void *) pTestHead->pTestInfo,
                      sizeof(ClEvtContPubT), pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD->PUB failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    clEvtContValidateResult();

    return CL_OK;
}

ClRcT clEvntContAlloc(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_ALLOC,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContAllocT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("RMD->Allocate failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvntContSetAttr(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;
    ClEvtContPubKey *pKey;
    ClEvtContAttrSetT *pAttr = (ClEvtContAttrSetT *) pTestHead->pTestInfo;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_ATTR_SET,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContAttrSetT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("RMD->Set Attribute failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    pKey = clHeapAllocate(sizeof(ClEvtContPubKey));

    clEvtContUtilsNameCpy(&pKey->appName, &pTestHead->testAppName);
    clEvtContUtilsNameCpy(&pKey->channelName, &pAttr->channelName);

    rc = clCntNodeAdd(gEvtContPubInfo, (ClCntKeyHandleT) pKey,
                      (ClCntDataHandleT) pAttr, NULL);

    return CL_OK;
}

ClRcT clEvntContGetAttr(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    rc = clEvtContRmd(pTestHead, EM_TEST_APP_ATTR_GET,
                      (void *) pTestHead->pTestInfo, sizeof(ClEvtContAttrGetT),
                      pRetCode);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("RMD->Get Attribute failed  :: [0x%X]\r\n", rc));
        return rc;
    }

    return CL_OK;
}


ClRcT clEvntContKill(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    ClNameT compName = { sizeof("eventServer_") - 1, "eventServer_" };
    ClNameT *pNodeName = (ClNameT *) pTestHead->pTestInfo;

    ClCpmLcmReplyT srcInfo;

    srcInfo.srcIocAddress = clIocLocalAddressGet();
    srcInfo.srcPort = 0x1233;
    srcInfo.rmdNumber = 2;

    strncat(compName.value, pNodeName->value, pNodeName->length);
    compName.length += pNodeName->length;

    clOsalPrintf("\nKilling [%.*s]\n", compName.length, compName.value);

    rc = clCpmComponentCleanup(&compName, pNodeName, &srcInfo);
    *pRetCode = rc;
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Killing eventServer on %*.s Failed [0x%X]\r\n",
                        pNodeName->length, pNodeName->value, rc));
        return rc;
    }
    else
    {
        sleep(3);
    }

    return CL_OK;
}

ClRcT clEvntContRestart(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode)
{
    ClRcT rc = CL_OK;

    ClNameT compName = { sizeof("eventServer_") - 1, "eventServer_" };
    ClNameT *pNodeName = (ClNameT *) pTestHead->pTestInfo;


    ClCpmLcmReplyT srcInfo;

    srcInfo.srcIocAddress = clIocLocalAddressGet();
    srcInfo.srcPort = 0x1233;
    srcInfo.rmdNumber = 2;

    strncat(compName.value, pNodeName->value, pNodeName->length);
    compName.length += pNodeName->length;

    clOsalPrintf("\nRestarting [%.*s]\n", compName.length, compName.value);

    rc = clCpmComponentInstantiate(&compName, pNodeName, &srcInfo);
    *pRetCode = rc;
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Killing eventServer on %*.s Failed [0x%X]\r\n",
                        pNodeName->length, pNodeName->value, rc));
        return rc;
    }
    else
    {
        sleep(3);
    }

    return CL_OK;
}
