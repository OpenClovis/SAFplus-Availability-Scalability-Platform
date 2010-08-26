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
 * File        : clOampRtParser.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBufferApi.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clParserApi.h>
#include <clOampRtErrors.h>
#include <clOampRtApi.h>
#include <clLogApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCoveApi.h"
#endif


/**
 * Enumeration to record OAMP RT.xml file parsing stages.
 */ 
typedef enum ClOampRtParsingState{
    CL_OAMP_RT_INVALID,
    CL_OAMP_RT_COMPNAME_FOUND
}ClOampRtParsingStateT;

void clOampRtSetResourceDepth(ClOampRtResourceArrayT* pResourceArray)
{
    char* q;
    char* p;
    ClUint32T i = 0;
    ClUint32T depth = 0;
    ClUint32T noOfRes = pResourceArray->noOfResources;
    ClNameT tempName;

    for(i=0; i<noOfRes; i++)
    {
        clNameCopy(&tempName, &pResourceArray->pResources[i].resourceName);
        q = tempName.value;
        depth = 0;
        while(1)
        {
            p = strtok_r(q,"\\",&q);    
            if(NULL == p)
            {
                break;
            }
            depth++;
        }
        pResourceArray->pResources[i].depth = depth;
    }

    return;
}

static ClInt32T clOampRtResourceCmp(const void *res1, const void *res2)
{
    return ((ClOampRtResourceT*)res1)->depth - ((ClOampRtResourceT*)res2)->depth;

}

void clOampRtSortResource(ClOampRtResourceArrayT* pResourceArray)
{
    clOampRtSetResourceDepth(pResourceArray);
    qsort((void*)pResourceArray->pResources, pResourceArray->noOfResources, 
          sizeof(*pResourceArray->pResources), clOampRtResourceCmp);
}

ClRcT clOampRtComponentResourceInfoGet(ClParserPtrT top, ClNameT* pCompName, ClOampRtResourceArrayT* pResourceArray,
                                       ClListHeadT *pCompResourceList)
{
    ClRcT rc = 0;
    ClParserPtrT compInstances  = NULL;
    ClParserPtrT compInst       = NULL;    
    ClParserPtrT resource        = NULL;
    const char* compName = NULL;
    const char* pMoId = NULL;
    const char* pObjFlag = NULL;
    const ClCharT *pAutoCreateFlag = NULL;
    const char* pPrimaryOIFlag = NULL;
    ClBufferHandleT msgBuf = 0;
    ClOampRtResourceT rtResource = {0};
    ClUint32T length = 0;
    ClOampRtParsingStateT  foundCompName = CL_OAMP_RT_INVALID;
    ClInt32T lastCompReadOffset = 0;

    if(NULL == pResourceArray)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid argument ResourceArray is NULL"));
        return CL_OAMP_RT_RC(CL_ERR_NULL_POINTER);    
    }
    
    pResourceArray->noOfResources = 0;
    compInst = clParserChild(top, "compInst");
    if(NULL == compInst)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_WARN, ("Not able to get compInst from RT file"));
        return CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INTERNAL);
    }

    rc = clBufferCreate(&msgBuf);

    for(;compInst;compInst = compInst->next)
    {
        ClOampRtComponentResourceArrayT *pCompResourceArray =  NULL;
        ClUint32T compResources = 0;
        compName = clParserAttr(compInst, "compName");
        if (!pCompName
            || (0 == strcmp(compName, (char*)pCompName->value)))
        {
            foundCompName = CL_OAMP_RT_COMPNAME_FOUND;
            resource = clParserChild(compInst, "resource");
            if(!resource)
            {
                if(pCompName)
                {
                    clLogNotice("UTL", "OAM", 
                                "There is no resource to be managed for this component %s",
                                pCompName ? pCompName->value : "");
                    clParserFree(compInstances);
                    clBufferDelete(&msgBuf);
                    CL_FUNC_EXIT();
                    return CL_OK;
                }
                continue;
            }            

            if(pCompResourceList)
            {
                pCompResourceArray = clHeapCalloc(1, sizeof(*pCompResourceArray));
                CL_ASSERT(pCompResourceArray != NULL);
            }

            for(;resource;resource = resource->next)
            {
                pMoId = clParserAttr(resource, "moID");
                pObjFlag = clParserAttr(resource, "toBeCreatedByComponent");
                pAutoCreateFlag = clParserAttr(resource, "autoCreate");
                pPrimaryOIFlag = clParserAttr(resource, "primaryOI");

                if ((NULL == pMoId) || 
                    (NULL == pObjFlag && NULL == pAutoCreateFlag))
                {
                    clLogError("UTL", "OAM", 
                               "The rt.xml file doesn't contain MoId or "
                               "toBeCreatedByComponent/autoCreate attributes");
                    if(pCompResourceArray) 
                        clHeapFree(pCompResourceArray);
                    clBufferDelete(&msgBuf);
                    clParserFree(compInstances);
                    return CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INVALID_CONFIG);
                }

                strcpy(rtResource.resourceName.value, pMoId);
                rtResource.resourceName.length = strlen(pMoId)+1;
                rtResource.objCreateFlag = CL_FALSE;
                if(pObjFlag 
                   &&
                   (0 == strncmp(pObjFlag, "true", 4)))
                {
                    rtResource.objCreateFlag = CL_TRUE;
                }

                rtResource.autoCreateFlag = CL_FALSE;
                if(pAutoCreateFlag &&
                   (0 == strncmp(pAutoCreateFlag, "true", 4)))
                {
                    rtResource.autoCreateFlag = CL_TRUE;
                    rtResource.objCreateFlag = CL_TRUE;
                }

                if(pPrimaryOIFlag != NULL)
                {
                    if(0 == strcmp(pPrimaryOIFlag, "true"))
                    {
                        rtResource.primaryOIFlag = CL_TRUE;
                    }
                    else
                    {
                        rtResource.primaryOIFlag = CL_FALSE;
                    }
                }
                else
                    rtResource.primaryOIFlag = CL_TRUE;

                if(NULL == strstr(pMoId, "*"))
                {
                    rtResource.wildCardFlag = CL_FALSE;
                }
                else
                {
                    rtResource.wildCardFlag = CL_TRUE;
                }

                /*
                 *With wild card we can't have it precreated.
                 */
                if((CL_TRUE == rtResource.wildCardFlag) && 
                   (CL_TRUE == rtResource.objCreateFlag))
                {
                    CL_FUNC_EXIT();
                    clLogError("UTL", "OAM", 
                               "Wild card with pre-creating object is invalid "
                               "configuration. Resource Name : [%s]", 
                               rtResource.resourceName.value);
                    if(pCompResourceArray)
                        clHeapFree(pCompResourceArray);
                    clParserFree(compInstances);
                    clBufferDelete(&msgBuf);
                    return CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INVALID_CONFIG);
                }

                rc = clBufferNBytesWrite(msgBuf, (ClUint8T*)&rtResource, sizeof(ClOampRtResourceT));
                CL_ASSERT(rc == CL_OK);
                ++compResources;
                ++pResourceArray->noOfResources;
            }

            /*
             * Add to the per component resource map
             */
            if(pCompResourceList && pCompResourceArray)
            {
                ClUint32T compResourceLen = (ClUint32T)sizeof(rtResource) * compResources;
                rc = clBufferReadOffsetSet(msgBuf, lastCompReadOffset, CL_BUFFER_SEEK_SET);
                CL_ASSERT(rc == CL_OK);
                pCompResourceArray->resourceArray.pResources = clHeapCalloc(compResources, sizeof(rtResource));
                CL_ASSERT(pCompResourceArray->resourceArray.pResources != NULL);
                rc = clBufferNBytesRead(msgBuf, (ClUint8T*)pCompResourceArray->resourceArray.pResources, 
                                        &compResourceLen);
                CL_ASSERT(rc == CL_OK);
                CL_ASSERT(compResourceLen == sizeof(rtResource)*compResources);
                pCompResourceArray->resourceArray.noOfResources = compResources;
                strncpy(pCompResourceArray->compName, compName, sizeof(pCompResourceArray->compName)-1);
                clListAddTail(&pCompResourceArray->list, pCompResourceList);
                lastCompReadOffset += compResourceLen;
                rc = clBufferReadOffsetSet(msgBuf, 0, CL_BUFFER_SEEK_SET);
                CL_ASSERT(rc == CL_OK);
            }
        }
    }
    
    clParserFree(compInstances);

    if(CL_OAMP_RT_INVALID == foundCompName) 
    {
        clLogWarning("UTL", "OMP", 
                     "Failed while parsing the resource information file (<nodename>_rt.xml). No entry is present in this file for component [%s].", 
                     pCompName->value);
        clBufferDelete(&msgBuf);
        return CL_OAMP_RT_RC(CL_ERR_DOESNT_EXIST);
    }   

    rc = clBufferLengthGet(msgBuf, &length);

    pResourceArray->pResources = clHeapAllocate(length);
    if(NULL == pResourceArray->pResources)
    {
        rc = clBufferDelete(&msgBuf);
        CL_FUNC_EXIT();
#if 0
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Allocation failed : [%s]\r\n", pRtFileName->value));
#endif
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Allocation failed"));
        return CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INTERNAL);
    }

    rc = clBufferNBytesRead(msgBuf, (ClUint8T*)pResourceArray->pResources, &length);
    rc = clBufferDelete(&msgBuf);


    clOampRtSortResource(pResourceArray);
        
    CL_FUNC_EXIT();   
    return CL_OK;
}

ClRcT clOampRtResourceInfoGet(ClParserPtrT top, ClNameT* pCompName, ClOampRtResourceArrayT* pResourceArray)
{
    return clOampRtComponentResourceInfoGet(top, pCompName, pResourceArray, NULL);
}

ClRcT clOampRtResourceScanFiles(const ClCharT *dirName, const ClCharT *suffix, 
                                ClInt32T suffixLen, ClOampRtResourceArrayT **ppResourceArray,
                                ClUint32T *pNumScannedResources,
                                ClListHeadT *pCompResourceList)
{
    DIR *dir;
    struct dirent *dirent = NULL;
    ClOampRtResourceArrayT *pResourceArray = NULL;
    ClInt32T numScannedResources = 0;
    ClRcT rc = CL_OK;

    if(!ppResourceArray || !pNumScannedResources) 
        return CL_OAMP_RT_RC(CL_ERR_INVALID_PARAMETER);

    if(!dirName) dirName = getenv("ASP_CONFIG");
    if(!dirName) return CL_OAMP_RT_RC(CL_ERR_INVALID_PARAMETER);

    if(!suffix)
    {
        suffix = "_rt.xml";
        suffixLen = 7;
    }

    dir = opendir(dirName);
    if(!dir)
    {
        clLogError("OAMP", "SCAN", "opendir returned [%s] for dir [%s]", 
                   strerror(errno), dirName);
        return CL_OAMP_RT_RC(CL_ERR_NOT_EXIST);
    }

    while ( (dirent = readdir(dir)) )
    {
        ClInt32T nameLen;
#ifdef DT_REG
        if(dirent->d_type !=  DT_REG)
        {
            continue;
        }
#endif        
        nameLen = strlen(dirent->d_name);
        if(nameLen < suffixLen) continue;
        if(!strncmp(&dirent->d_name[nameLen - suffixLen], suffix, suffixLen))
        {
            ClParserPtrT configPtr = NULL;
            clLogNotice("OAMP", "SCAN", "Scanning resources for file [%s]",
                        dirent->d_name);
            if(!(numScannedResources & 3))
            {
                pResourceArray = clHeapRealloc(pResourceArray,
                                               (numScannedResources + 4) * sizeof(ClOampRtResourceArrayT));
                CL_ASSERT(pResourceArray != NULL);
                memset(pResourceArray + numScannedResources, 0, 
                       sizeof(*pResourceArray) * 4);
            }
            configPtr = clParserOpenFile(dirName, dirent->d_name);
            if(!configPtr)
            {
                clLogError("OAMP", "SCAN", "Config file [%s] open error", dirent->d_name);
                rc = CL_OAMP_RT_RC(CL_ERR_NOT_EXIST);
                goto out_free;
            }
            rc = clOampRtComponentResourceInfoGet(configPtr, NULL, 
                                                  pResourceArray + numScannedResources,
                                                  pCompResourceList);
            clParserFree(configPtr);
            if(rc != CL_OK || !pResourceArray[numScannedResources].noOfResources)
            {
                clLogNotice("OAMP", "SCAN", "Skipping resource file [%s]", dirent->d_name);
                if(pResourceArray[numScannedResources].pResources) 
                {
                    clHeapFree(pResourceArray[numScannedResources].pResources);
                    pResourceArray[numScannedResources].pResources = NULL;
                }
                pResourceArray[numScannedResources].noOfResources = 0;
                continue;
            }
            strncpy(pResourceArray[numScannedResources].resourceFile, dirent->d_name, 
                    sizeof(pResourceArray->resourceFile)-1);
            ++numScannedResources;
        }
    }

    *ppResourceArray = pResourceArray;
    *pNumScannedResources = numScannedResources;
    clLogNotice("OAMP", "SCAN", "Num scanned resource files [%d]", numScannedResources);

    closedir(dir);
    return CL_OK;
    
    out_free:
    if(pResourceArray) clHeapFree(pResourceArray);
    closedir(dir);
    while(!CL_LIST_HEAD_EMPTY(pCompResourceList))
    {
        ClListHeadT *top = pCompResourceList->pNext;
        ClOampRtComponentResourceArrayT *compResourceArray = CL_LIST_ENTRY(top,
                                                                      ClOampRtComponentResourceArrayT, 
                                                                      list);
        clListDel(&compResourceArray->list);
        clHeapFree(compResourceArray->resourceArray.pResources);
    }
    return rc;
}
