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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clParserApi.h>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clIocUdpTransportApi.h>
#include <clIocParseConfig.h>

#define CL_IOC_CONFIG_FILE_NAME "clIocConfig.xml"

/*
 * We dont want to break existing test codes dependent on this variable.
 * Hence retaining the name.
 */
ClIocConfigT pAllConfig;
static ClCharT gClIocNodeName[256];

/*
 * Include the IOC config parser data.
*/
#define _CL_IOC_CONFIG_C_

#include "clIocConfigData.h"

#define IOC_LOG_AREA_IOC		"IOC"
#define IOC_LOG_AREA_NODE		"NODE"
#define IOC_LOG_CTX_PARSER		"PAR"
#define IOC_LOG_CTX_MULTICAST	"MULTICAST"
#define IOC_LOG_CTX_WATERMARK	"WMA"
#define IOC_LOG_CTX_INSTANCE	"INST"
#define IOC_LOG_CTX_DATA		"DATA"

ClUint32T clAspLocalId;

static ClRcT clIocLinkSupportsMulticastTagFmt(ClParserTagT *pTag,
                                              ClParserDataT *pData,
                                              ClPtrT pBase,
                                              ClCharT *pAttr,
                                              ClPtrT pValue,
                                              ClParserTagOpT op
                                              )
{
    ClRcT rc = CL_TAG_SKIP_CHILD;
    ClBoolT val ;
    if(!pValue)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_MULTICAST,"Invalid value\n");
        goto out;
    }
    if(strcmp(pTag->pTag,"linkSupportsMulticast"))
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_MULTICAST,"Invalid tag:%s\n",pTag->pTag);
        goto out;
    }
    val = *(ClBoolT*)pValue;
    if(val == CL_FALSE)
    {
        rc = CL_TAG_OK;
    }
    out:
    return rc;
}

static ClRcT clIocWaterMarkActionTagFmt(ClParserTagT *pTag,
                                        ClParserDataT *pData,
                                        ClPtrT pBase,
                                        ClCharT *pAttr,
                                        ClPtrT pValue,
                                        ClParserTagOpT op
                                        )
{
#if 0
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    ClEoActionInfoT *pWMActions = pBase;
    static const ClCharT *pActionTags[] = { "event","notification","log","custom",NULL};
    static ClUint32T actionBitMap[] = { 
        CL_EO_ACTION_EVENT,
        CL_EO_ACTION_NOT,
        CL_EO_ACTION_LOG,
        CL_EO_ACTION_CUSTOM
    };
    register ClUint32T i;
    ClUint32T bit;
    if(pData == NULL || pBase == NULL || pAttr == NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid param\n");
        goto out;
    }
    for(i = 0; pActionTags[i] && strcasecmp(pActionTags[i],pData->pTag); ++i);

    if(pActionTags[i] == NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid tag specified:%s\n",pData->pTag);
        goto out;
    }
    bit = actionBitMap[i];
    rc = CL_TAG_SKIP_UPDATE;
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            if(!strcasecmp(pAttr,"true"))
            {
                pWMActions->bitMap |= bit;
            }
            else if(!strcasecmp(pAttr,"false"))
            {
                /*
                 * Not really required. But just passing time:-)
                 */
                pWMActions->bitMap &= ~bit;
            }
            else
            {
                clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid attribute:%s\n",pAttr);
                rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            const ClCharT *pAction = "false";
            rc = CL_TAG_OK;
            if((pWMActions->bitMap & bit))
            {
                pAction = "true";
            }
            strncpy(pAttr,pAction,CL_PARSER_ATTR_SIZE);
        }
        break;
    default:
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid op:%d\n",op);
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    out:
    return rc;
#else
    return CL_OK;
#endif
}

static ClRcT clIocNodeInstanceNameTagFmt(ClParserTagT *pTag,
                                         ClParserDataT *pData,
                                         ClPtrT pBase,
                                         ClCharT *pAttr,
                                         ClPtrT pValue,
                                         ClParserTagOpT op
                                         )
{
#if 0
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(pAttr == NULL)
    {
        clLogError(IOC_LOG_AREA_NODE,IOC_LOG_CTX_INSTANCE,"Invalid param\n");
        goto out;
    }
    /*
     * Continue by skipping children and into the next instance
     * if name doesnt match the nodeInstance template.
     */
    rc = CL_TAG_CONT;
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            if(!strcasecmp(pAttr,gClIocNodeName))
            {
                rc = CL_TAG_SKIP_CHILD_INSTANCES | CL_TAG_SKIP_UPDATE;
            }
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            rc = CL_TAG_OK;
            strncpy(pAttr,gClIocNodeName,CL_PARSER_ATTR_SIZE);
        }
        break;
    default:
        clLogError(IOC_LOG_AREA_NODE,IOC_LOG_CTX_INSTANCE,"Invalid op:%d\n",op);
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    out:
    return rc;
#else
    return CL_OK;
#endif
}

static ClBoolT clIocQueueWMValidate(ClParserTagT *pTag,ClPtrT pValue)
{
    ClBoolT valid = CL_TRUE;
    if(pValue)
    {
        ClUint64T v = *(ClUint64T*)pValue;
        if(v > 100)
        {
            valid = CL_FALSE;
        }
    }
    return valid;
}


static ClRcT clIocWaterMarkDataInit(ClPtrT pParentBase,
                                    ClUint32T *pNumInstances,
                                    ClPtrT *pBase,
                                    ClUint32T *pBaseOffset,
                                    ClParserTagOpT op)
{
    register ClInt32T i;
    ClPtrT *pParentBases[] = { (ClPtrT*)&pAllConfig.iocConfigInfo.iocSendQInfo,
                               (ClPtrT*)&pAllConfig.iocConfigInfo.iocRecvQInfo,
                               NULL,
    };
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(pParentBase == NULL || pBase == NULL || pNumInstances == NULL
       || pBaseOffset == NULL
       )
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid param\n");
        goto out;
    }
    for(i = 0; pParentBases[i] && pParentBases[i] != pParentBase; ++i);

    if(pParentBases[i] == NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Parent not found\n");
        goto out;
    }

    *pBase = (ClPtrT)(&((ClIocQueueInfoT*)pParentBases[i])->queueWM);
    *pBaseOffset = 0;
    rc = CL_OK;

    out:
    return rc;
}

static ClRcT clIocWaterMarkActionDataInit(ClPtrT pParentBase,
                                          ClUint32T *pNumInstances,
                                          ClPtrT *pBase,
                                          ClUint32T *pBaseOffset,
                                          ClParserTagOpT op
                                          )
{
    ClPtrT pCurrentBase = NULL;
    register ClInt32T i;
    ClPtrT *pBases[] = { 
        (ClPtrT*)&pAllConfig.iocConfigInfo.iocSendQInfo.queueWM,
        (ClPtrT*)&pAllConfig.iocConfigInfo.iocRecvQInfo.queueWM,
        NULL,
    };
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(pParentBase == NULL || pBase == NULL || pBaseOffset == NULL ||
       pNumInstances == NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid Param\n");
        goto out;
    }

    for(i = 0; pBases[i] && pParentBase != pBases[i]; ++i);

    if(pBases[i] == NULL)
    {
        /*
         * Search for the matching parent base in water marks
         */
        ClPtrT *pActionBases[] = { 
            (ClPtrT*)&pAllConfig.wmActions[CL_IOC_SENDQ],
            (ClPtrT*)&pAllConfig.wmActions[CL_IOC_RECVQ],
            NULL,
        };
        for(i = 0; pActionBases[i] && pActionBases[i] != pParentBase; ++i);
        if(pActionBases[i] == NULL)
        {
            clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_WATERMARK,"Invalid parent base:%p\n",pParentBase);
            goto out;
        }
        pCurrentBase = pActionBases[i];
    }
    else
    {
        pCurrentBase = &pAllConfig.wmActions[i];
    }
    *pBase = pCurrentBase;
    *pBaseOffset = 0;
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clIocTransportDataInit(ClPtrT pParentBase,
                                    ClUint32T *pNumInstances,
                                    ClPtrT *pBase,
                                    ClUint32T *pBaseOffset,
                                    ClParserTagOpT op)
{
    ClIocConfigT *pConfig =(ClIocConfigT *)pParentBase;
    ClIocUserTransportConfigT *pTransport  = NULL;
    ClRcT rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);

    if(pParentBase == NULL || pNumInstances == NULL ||
       pBase == NULL || pBaseOffset ==NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Invalid param\n");
        goto out;
    }
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            ClUint32T numInstances = *pNumInstances;
            /*Allocate the memory for transport*/
            pTransport = (ClIocUserTransportConfigT*) calloc(numInstances,sizeof(ClIocUserTransportConfigT));
            rc = CL_PARSER_RC(CL_ERR_NO_MEMORY);
            if(pTransport == NULL)
            {
                clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Error allocating memory\n");
                goto out;
            }
            pConfig->numOfXports = *pNumInstances;
            pConfig->transport = pTransport;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            pTransport = pConfig->transport;
            *pNumInstances = pConfig->numOfXports;
        }
        break;
    default:
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Invalid op:%d\n",op);
        goto out;
    }
    *pBase = pTransport;
    *pBaseOffset = sizeof(ClIocUserTransportConfigT);
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clIocLinkDataInit(ClPtrT pParentBase,
                               ClUint32T *pNumInstances,
                               ClPtrT *pBase,
                               ClUint32T *pBaseOffset,
                               ClParserTagOpT op
                               )
{
    ClRcT rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);
    ClIocUserTransportConfigT *pTransport = (ClIocUserTransportConfigT *) pParentBase;
    ClIocUserLinkCfgT *pLinkCfg = NULL;
    if(pParentBase == NULL || pNumInstances == NULL || 
       pBase==NULL || pBaseOffset==NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Invalid param\n");
        goto out;
    }
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            ClUint32T numInstances = *pNumInstances;
            rc = CL_PARSER_RC(CL_ERR_NO_MEMORY);
            pLinkCfg = (ClIocUserLinkCfgT*) calloc(numInstances,sizeof(ClIocUserLinkCfgT));
            if(pLinkCfg == NULL)
            {
                clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Error allocating memory\n");
                goto out;
            }
            pTransport->numOfLinks = numInstances;
            pTransport->pLink = pLinkCfg;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            pLinkCfg = pTransport->pLink;
            *pNumInstances = pTransport->numOfLinks;
        }
        break;
    default:
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_DATA,"Invalid op: %d\n",op);
        goto out;
    }
    *pBase = pLinkCfg;
    *pBaseOffset = sizeof(ClIocUserLinkCfgT);
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clIocLocationDataInit(ClPtrT pParentBase,
                                   ClUint32T *pNumInstances,
                                   ClPtrT *pBase,
                                   ClUint32T *pBaseOffset,
                                   ClParserTagOpT op
                                   )
{
    ClRcT rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);
    ClIocUserLinkCfgT *pLinkCfg = (ClIocUserLinkCfgT *)pParentBase;
    ClIocLocationInfoT *pLocation = NULL;

    if(pParentBase== NULL || pNumInstances == NULL 
       || pBase==NULL || pBaseOffset ==NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid param\n");
        goto out;
    }
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            ClUint32T numInstances = *pNumInstances;
            rc = CL_PARSER_RC(CL_ERR_NO_MEMORY);
            pLocation = (ClIocLocationInfoT*) calloc(numInstances,sizeof(ClIocLocationInfoT));
            if(pLocation == NULL)
            {
                clLogError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Error allocating memory\n");
                goto out;
            }
            pLinkCfg->numOfNodes = numInstances;
            pLinkCfg->pNode = pLocation;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            pLocation = pLinkCfg->pNode;
            *pNumInstances = pLinkCfg->numOfNodes;
        }
        break;
    default:
        clLogError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid op:%d\n",op);
        goto out;
    }
    *pBase = pLocation;
    *pBaseOffset= sizeof(ClIocLocationInfoT);
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clIocParseConfig(const ClCharT *pNodeName,ClIocConfigT **ppIocConfig)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(ppIocConfig == NULL)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_PARSER,"Invalid param\n");
        goto out;
    }

    if(pNodeName)
    {
        strncpy(gClIocNodeName,pNodeName,sizeof(gClIocNodeName)-1);
    }

    rc = clParseXML(NULL,CL_IOC_CONFIG_FILE_NAME,&clIocConfigParserData);
    
    if(rc != CL_OK)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_PARSER,"Error parsing file:%s\n",CL_IOC_CONFIG_FILE_NAME);
        goto out;
    }
#ifdef DEBUG
    rc = clParseDisplay(&clIocConfigParserData);

    if(rc != CL_OK)
    {
        clLogError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_PARSER,"Error in parser display for ioc data\n");
    }
#endif

    /*Copy the parsed config.*/
    pAllConfig.iocConfigInfo.nodeAddress = clAspLocalId;
    pAllConfig.iocConfigInfo.iocMaxNumOfXports = CL_IOC_DEF_MAX_ALLOWED_XPORTS;
    pAllConfig.iocConfigInfo.version = CL_IOC_HEADER_VERSION;
    pAllConfig.iocConfigInfo.iocMaxNumOfPriorities = 0;
    *ppIocConfig = &pAllConfig;
    rc = CL_OK;

    out:
    return (rc);
}

/************************************************************************/
ClRcT clIocParserMemoryFree(void)
{
    ClRcT rc = CL_OK;
    ClIocUserTransportConfigT *pTransport = NULL;
    ClUint32T i = 0, j = 0;

    for (i = 0; i < pAllConfig.numOfXports; i++)
    {
        pTransport = pAllConfig.transport + i;
        for (j = 0; j < pTransport->numOfLinks; j++)
        {
            if(pTransport->pLink[j].pNode)
            {
                free(pTransport->pLink[j].pNode);
            }
        }
        free(pTransport->pLink);
    }
    free(pAllConfig.transport);
    pAllConfig.transport = NULL;
    return rc;
}

