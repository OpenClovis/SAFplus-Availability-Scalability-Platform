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
 * ModuleName  : med
 * File        : clMedDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module implements Mediation component
 *
 *
 *****************************************************************************/


/** @pkg med*/
#include <clMedApi.h>
#include <clMedDs.h>
#include <clCommonErrors.h>
#include<clCommon.h>
#include <clDebugApi.h>
#include<string.h>
#include <clAlarmDefinitions.h>
#include <clCorTxnApi.h>

#include <ipi/clAlarmIpi.h>

#ifdef MORE_CODE_COVERAGE
#include <clCodeCovStub.h>
#endif

#include <signal.h>
#include<clLogApi.h>
#include<clMedLog.h>
#include <clMedDebug.h>


#define CL_MED_CLI_SRC2TGT_PRINT(A, B, C, D)      \
    {                             \
        ClCharT data[1024];       \
        sprintf(data, "SRC[%s] -> DST[%s]:AttrID[%d]\n",A, B, C);  \
        clBufferNBytesWrite(D, (ClUint8T*)data, strlen(data));      \
    }                                               \

static ClInt32T dumpMoidTable(ClMedIdXlationT *pIdx, void* pNativeId, ClUint32T length, ClBufferHandleT msgHdl)
{
    ClCntNodeHandleT     currentNode = 0;
    ClCntNodeHandleT     nextNode = 0;
    ClCorMOIdPtrT  hMoId = NULL;
    ClInt32T rc;
    ClNameT nameMoId;

    if(length != 0 )
        if(length != pIdx->nativeId.len || ((0 != strncmp((char*)pIdx->nativeId.id, (char*)pNativeId, length))))
            return CL_OK;

    if (clCntFirstNodeGet(pIdx->moIdTbl, &currentNode) != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));

        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
    }
    if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->moIdTbl,
                    currentNode, (ClCntDataHandleT*)&hMoId)))
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
    }
    clCorMoIdToMoIdNameGet(hMoId, &nameMoId);
    CL_MED_CLI_SRC2TGT_PRINT(pIdx->nativeId.id, nameMoId.value, pIdx->tgtId->info.corId.attrId[0], msgHdl);
    while(clCntNextNodeGet(pIdx->moIdTbl, currentNode, &nextNode) == CL_OK)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->moIdTbl,
                        nextNode, (ClCntDataHandleT*)&hMoId)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
        }

        clCorMoIdToMoIdNameGet(hMoId, &nameMoId);
        CL_MED_CLI_SRC2TGT_PRINT(pIdx->nativeId.id, nameMoId.value, pIdx->tgtId->info.corId.attrId[0], msgHdl);
        currentNode = nextNode;
        nextNode = 0;
    }
    return CL_OK;	

}
#if 0
static ClInt32T dumpMoIdStoreTable(ClMedIdXlationT    *pIdx)
{
    ClCntNodeHandleT     currentNode = 0;
    ClCntNodeHandleT     nextNode = 0;
    ClMedMOIdInfoT   *pMoIdInfo= NULL;
    ClInt32T rc;

    if (clCntFirstNodeGet(pIdx->pMed->moIdStore, &currentNode) != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
    }
    if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->pMed->moIdStore,
                    currentNode, (ClCntDataHandleT*)&pMoIdInfo)))
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
    }
    clCorMoIdShow(&pMoIdInfo->moId);
    while(clCntNextNodeGet(pIdx->pMed->moIdStore, currentNode, &nextNode) == CL_OK)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->pMed->moIdStore,
                        nextNode, (ClCntDataHandleT*)&pMoIdInfo)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 			  		 		  		 				
        }
        clCorMoIdShow(&pMoIdInfo->moId);
        currentNode = nextNode;
        nextNode = 0;
    }
    return CL_OK;	
}
static void clMedPrintSrcIndex(void* pAgentInst, ClUint32T instLenght)
{
    ClUint32T i = 0;
    for(i = 0; i < (instLenght)/sizeof(ClUint32T); i++)
    {
        printf("%x ",*((ClUint32T*)pAgentInst));
        pAgentInst = (ClCharT*)pAgentInst+sizeof(ClUint32T);
    }
    return;
    
}
#endif

        
static ClInt32T dumpInstnaceXlationTbl(ClMedIdXlationT *pIdx, void* pNativeId, ClUint32T length, ClBufferHandleT msgHdl)
{
    ClCntNodeHandleT     currentNode = 0;
    ClCntNodeHandleT     nextNode = 0;
    ClMedInstXlationT    *pInstXln = NULL;
    ClInt32T rc;
    ClNameT nameMoId;
    
    
    if(length != 0 )
        if(length != pIdx->nativeId.len || ((0 != strncmp((char*)pIdx->nativeId.id, (char*)pNativeId, length))))
            return CL_OK;
    
    if(clCntFirstNodeGet(pIdx->instXlationTbl, &currentNode) != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_WARN, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);		 		 				 		 		 		 		 	
    }
    if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->instXlationTbl,
                    currentNode, (ClCntDataHandleT*)&pInstXln)))
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
    }
    clCorMoIdToMoIdNameGet(&pInstXln->moId, &nameMoId);
    CL_MED_CLI_SRC2TGT_PRINT(pIdx->nativeId.id, nameMoId.value, pInstXln->attrId[0], msgHdl);

    while(clCntNextNodeGet(pIdx->instXlationTbl, currentNode, &nextNode) == CL_OK)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->instXlationTbl,
                        nextNode, (ClCntDataHandleT*)&pInstXln)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
        }

        clCorMoIdToMoIdNameGet(&pInstXln->moId, &nameMoId);
        CL_MED_CLI_SRC2TGT_PRINT(pIdx->nativeId.id, nameMoId.value, pInstXln->attrId[0], msgHdl);
        currentNode = nextNode;
        nextNode = 0;
    }
    return CL_OK;
}


static ClRcT clMedXlationTblWalk (ClCntKeyHandleT userKey, ClCntDataHandleT userData, 
                                            ClCntArgHandleT userArg, ClUint32T dataLength)
{
    ClMedIdXlationT *pIdXlation = (ClMedIdXlationT*)userData;
    ClMedDebugXlationWalkDataT* pWalkData = (ClMedDebugXlationWalkDataT*)userArg;
    if(pWalkData->displayType == CL_MED_DEBUG_MOID)
    {
        dumpMoidTable(pIdXlation, pWalkData->pNativeId, pWalkData->length, pWalkData->msgHdl);
    }
    else
    {
        dumpInstnaceXlationTbl(pIdXlation, pWalkData->pNativeId, pWalkData->length, pWalkData->msgHdl);
    }
    
    return CL_OK;
}


    
static ClRcT clMedDebugPrint(ClMedHdlPtrT medHdl, void* pNativeId, ClUint32T length, ClCharT **ppRetStr, ClMedDebugInfoTypeT type)
{
    ClRcT rc = CL_OK;
    ClMedDebugXlationWalkDataT walkData;
    ClUint32T retStrlen = 0;
    ClCharT   *retStr   = NULL;
    
    ClMedObjT *pMm = (ClMedObjT*)medHdl;
    
    clBufferCreate(&walkData.msgHdl);
    walkData.length = length;
    walkData.pNativeId = pNativeId;
    walkData.displayType = type;

    rc = clCntWalk(pMm->idXlationTbl, clMedXlationTblWalk, &walkData, 0);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, "DBG", "Container walk on mediation table failed. rc [0x%x]", rc);
        goto failed;
    }

    rc = clBufferLengthGet(walkData.msgHdl, &retStrlen);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, "DBG", "Buffer length get failed. rc [0x%x]", rc);
        goto failed;
    }

    retStr = clHeapAllocate(retStrlen + 1);
    if (NULL == retStr)
    {
        clLogError(CL_MED_AREA, "DBG", "Memory allocation failed.");
        goto failed;
    }

    rc = clBufferFlatten(walkData.msgHdl, (ClUint8T**)ppRetStr);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, "DBG", "Buffer flatten failed. rc [0x%x]", rc);
        clHeapFree(retStr);
        goto failed;
    }

    strncpy(retStr, *ppRetStr, retStrlen);
    retStr[retStrlen] = 0;
    clHeapFree(*ppRetStr);

    *ppRetStr = retStr;

failed:    
    clBufferDelete(&walkData.msgHdl);
    return rc;
}


static ClRcT cliMedInstShow(int argc, char **argv, char **retStr);
static ClRcT cliMedTargetIdShow(int argc, char **argv, char **retStr);


static ClDebugFuncEntryT gClMedCliTab[] = {
    {(ClDebugCallbackT) cliMedInstShow, "medInstShow", "Show Entire Med Data base"} ,
    {(ClDebugCallbackT) cliMedTargetIdShow, "medTgtIdShow", "Show MOID and attribute ID"},
};

ClDebugModEntryT clModTab[] = {
    {"Med", "Med", gClMedCliTab, "Mediation Layer Commands"},
    {"", "", 0, ""}
};

static ClHandleT gMedDebugReg = 0;
static ClMedHdlPtrT gMedHdl = 0;

void clMedCliStrPrint(char *str, char **retStr)
{
    *retStr = clHeapAllocate(strlen(str) + 1);
    if (NULL == *retStr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Malloc Failed \n"));
        return;
    }
    sprintf(*retStr, str);
    return;
}

static ClRcT cliMedInstShow(int argc, char **argv, char **retStr)
{
    if (argc != 1 && argc != 2)
    {
        clMedCliStrPrint("Usage: medInstShow [OID]", retStr);
        return CL_OK;
    }
    if(argc == 1)
    {
        clMedDebugPrint(gMedHdl, NULL, 0, retStr, CL_MED_DEBUG_INST);
    }
    else
    {
        clMedDebugPrint(gMedHdl, argv[1], strlen(argv[1])+1, retStr, CL_MED_DEBUG_INST);
    }
    
    return CL_OK;
}

static ClRcT cliMedTargetIdShow(int argc, char **argv, char **retStr)
{
    if (argc != 1 && argc != 2)
    {
        clMedCliStrPrint("Usage: medTgtIdShow [OID]", retStr);
        return CL_OK;
    }
    if(argc == 1)
    {
        clMedDebugPrint(gMedHdl, NULL, 0, retStr, CL_MED_DEBUG_MOID);
    }
    else
    {
        clMedDebugPrint(gMedHdl, argv[1], strlen(argv[1])+1, retStr, CL_MED_DEBUG_MOID);
    }
    return CL_OK;
}

ClRcT clMedDebugRegister(ClMedHdlPtrT medHdl)
{
    ClRcT rc = CL_OK;

    rc = clDebugPromptSet("MED");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }

    gMedHdl = medHdl;
    
    return clDebugRegister(gClMedCliTab,
                           sizeof(gClMedCliTab) / sizeof(gClMedCliTab[0]), 
                           &gMedDebugReg);
}

ClRcT clMedDebugDeregister()
{
    return clDebugDeregister(gMedDebugReg);
}
