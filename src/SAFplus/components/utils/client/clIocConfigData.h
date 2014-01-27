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
#ifndef _CL_IOC_CONFIG_C_
#error "_CL_IOC_CONFIG_C_ undefined.This file should be only included from IOC config parsing file"
#endif

/*
 * IOC Config Data init parser callbacks.
*/
static ClParserDataInitFunctionT clIocTransportDataInit;
static ClParserDataInitFunctionT clIocLinkDataInit;
static ClParserDataInitFunctionT clIocLocationDataInit;
static ClParserDataInitFunctionT clIocWaterMarkActionDataInit;
static ClParserDataInitFunctionT clIocWaterMarkDataInit;
/*
 * Tag fmt parser callbacks.
 *
*/
static ClParserTagFmtFunctionT clIocLinkSupportsMulticastTagFmt;
static ClParserTagFmtFunctionT clIocWaterMarkActionTagFmt;
static ClParserTagFmtFunctionT clIocNodeInstanceNameTagFmt;

/*
 * Validate TAG callbacks.
*/
static ClParserTagValidateFunctionT clIocQueueWMValidate;

/*Fill up the entries for IOC*/
static ClParserTagT clIocParserTags[] = {
    { 
         "reassemblyTimeout",
         CL_PARSER_UINT32_TAG,
         CL_TAG_SIZE(ClIocLibConfigT,iocReassemblyTimeOut),
         CL_TAG_OFFSET(ClIocConfigT,iocConfigInfo) + CL_TAG_OFFSET(ClIocLibConfigT,iocReassemblyTimeOut),
         NULL,
    },
    { 
         "heartBeatInterval",
         CL_PARSER_UINT32_TAG,
         CL_TAG_SIZE(ClIocLibConfigT,iocHeartbeatTimeInterval),
         CL_TAG_OFFSET(ClIocConfigT,iocConfigInfo) + CL_TAG_OFFSET(ClIocLibConfigT,iocHeartbeatTimeInterval),
         NULL,
    },
};

/*Tags for the children of IOC*/
static ClParserTagT clIocTransportParserTags[] = {
    { 
         "transportName",
         CL_PARSER_STR_TAG,
         CL_TAG_SIZE(ClIocUserTransportConfigT,pName),
         CL_TAG_OFFSET(ClIocUserTransportConfigT,pName),
         NULL,
    },
    { 
        "transportPriority",
        CL_PARSER_UINT8_TAG,
        CL_TAG_SIZE(ClIocUserTransportConfigT,priority),
        CL_TAG_OFFSET(ClIocUserTransportConfigT,priority),
        NULL,
    },
    { 
        "transportId",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClIocUserTransportConfigT,id),
        CL_TAG_OFFSET(ClIocUserTransportConfigT,id),
        NULL,
    },
};

/*Tags for the children of transport*/
static ClParserTagT clIocLinkParserTags[] = {
    { 
        "linkName",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,pName),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,pName),
        NULL,
    },
    { 
        "linkPriority",
        CL_PARSER_UINT8_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,priority),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,priority),
        NULL,
    },
    { 
        "linkInterface",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,pInterface),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,pInterface),
        NULL,
    },
    { 
        "linkMulticastAddress",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,pMcastAddress),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,pMcastAddress),
        NULL,
    },
    { 
        "linkSupportsMulticast",
        CL_PARSER_BOOL_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,isMcastSupported),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,isMcastSupported),
        clIocLinkSupportsMulticastTagFmt,
    },
    { 
        "linkSupportsChksum",
        CL_PARSER_BOOL_TAG,
        CL_TAG_SIZE(ClIocUserLinkCfgT,isCksumSupported),
        CL_TAG_OFFSET(ClIocUserLinkCfgT,isCksumSupported),
        NULL,
    },
};

static ClParserTagT clIocLocationParserTags[] = {
    { 
        "slot",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClIocLocationInfoT,slotNum),
        CL_TAG_OFFSET(ClIocLocationInfoT,slotNum),
        NULL,
    },
    { 
        "interfaceAddress",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClIocLocationInfoT,pInterfaceAddress),
        CL_TAG_OFFSET(ClIocLocationInfoT,pInterfaceAddress),
        NULL,
    },
};

static ClParserTagT clIocQueueParserTags[] = {
    {
        "size",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClIocQueueInfoT,queueSize),
        CL_TAG_OFFSET(ClIocQueueInfoT,queueSize),
        NULL,
    },
};

/*
 * Define the action tags.
 */
static ClParserTagT clIocWaterMarkActionParserTags[] = {
    {
        "enable",
        CL_PARSER_CUSTOM_TAG,
        CL_TAG_SIZE(ClEoActionInfoT,bitMap),
        CL_TAG_OFFSET(ClEoActionInfoT,bitMap),
        clIocWaterMarkActionTagFmt,
    },
};

static ClParserTagT clIocWaterMarkParserTags[] = {
    {
        "lowLimit",
        CL_PARSER_UINT64_TAG,
        CL_TAG_SIZE(ClWaterMarkT,lowLimit),
        CL_TAG_OFFSET(ClWaterMarkT,lowLimit),
        NULL,
        clIocQueueWMValidate,
    },
    {
        "highLimit",
        CL_PARSER_UINT64_TAG,
        CL_TAG_SIZE(ClWaterMarkT,highLimit),
        CL_TAG_OFFSET(ClWaterMarkT,highLimit),
        NULL,
        clIocQueueWMValidate,
    },
};

static ClParserTagT clIocNodeInstanceParserTags[] = {
    {
        "name",
        CL_PARSER_STR_TAG,
        0,
        0,
        clIocNodeInstanceNameTagFmt,
    },
};

#define CPDI_LEAF(ptag,pTags,pDataInit) { ptag,CL_PARSER_NUM(pTags),pTags,pDataInit,0,0,1,CL_FALSE,0,NULL }
#define CPDI_NODE(ptag,pTags,pDataInit,numInstances,pTagChildren) { ptag,CL_PARSER_NUM(pTags),pTags,pDataInit,0,0,numInstances,CL_FALSE,CL_PARSER_NUM(pTagChildren),pTagChildren }
#define CPDI_BASE(ptag,pTags,pBase,numInstances,pTagChildren) { ptag,CL_PARSER_NUM(pTags),pTags,NULL,pBase,0,numInstances,CL_FALSE,CL_PARSER_NUM(pTagChildren),pTagChildren }



static ClParserDataT clIocLocationParserData[] = {
    {
        "location",
        CL_PARSER_NUM(clIocLocationParserTags),
        clIocLocationParserTags,
        /*.pDataInit = */ clIocLocationDataInit,
        0,0,
        /* .numInstances = */ 0,
        CL_FALSE,
        0,
        NULL
    },
};

static ClParserDataT clIocLinkParserData[]  = {
    CPDI_NODE("link",clIocLinkParserTags,clIocLinkDataInit,0,clIocLocationParserData)
#if 0    
    {
        .pTag = "link",
        .numTags = CL_PARSER_NUM(clIocLinkParserTags),
        .pTags = clIocLinkParserTags,
        .pDataInit = clIocLinkDataInit,
        .numInstances = 0,
        .pTagChildren = clIocLocationParserData,
        .numChild = CL_PARSER_NUM(clIocLocationParserData),
    },
#endif    
};

static ClParserDataT clIocWaterMarkActionChildrenParserData[] = {
    CPDI_LEAF("event",clIocWaterMarkActionParserTags,clIocWaterMarkActionDataInit),
    CPDI_LEAF("notification",clIocWaterMarkActionParserTags,clIocWaterMarkActionDataInit),
    CPDI_LEAF("log",clIocWaterMarkActionParserTags,clIocWaterMarkActionDataInit),
    CPDI_LEAF("custom",clIocWaterMarkActionParserTags,clIocWaterMarkActionDataInit)
#if 0    
    {
        .pTag = "event",
        .numTags = CL_PARSER_NUM(clIocWaterMarkActionParserTags),
        .pTags = clIocWaterMarkActionParserTags,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "notification",
        .numTags = CL_PARSER_NUM(clIocWaterMarkActionParserTags),
        .pTags = clIocWaterMarkActionParserTags,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "log",
        .numTags = CL_PARSER_NUM(clIocWaterMarkActionParserTags),
        .pTags = clIocWaterMarkActionParserTags,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "custom",
        .numTags = CL_PARSER_NUM(clIocWaterMarkActionParserTags),
        .pTags = clIocWaterMarkActionParserTags,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
#endif    
};

static ClParserDataT clIocWaterMarkChildrenParserData[] = {
    CPDI_NODE("action",((ClParserTagT *)NULL),clIocWaterMarkActionDataInit,1,clIocWaterMarkActionChildrenParserData)
#if 0    
    {
        .pTag = "action",
        .numTags = 0,
        .pTags = NULL,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkActionChildrenParserData),
        .pTagChildren = clIocWaterMarkActionChildrenParserData,
    },
#endif
};

static ClParserDataT clIocQueueChildrenParserData[] = {
    CPDI_NODE("waterMark",clIocWaterMarkParserTags,clIocWaterMarkDataInit,1,clIocWaterMarkChildrenParserData)
#if 0    
    {
        .pTag = "waterMark",
        .numTags = CL_PARSER_NUM(clIocWaterMarkParserTags),
        .pTags = clIocWaterMarkParserTags,
        .pDataInit = clIocWaterMarkDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkChildrenParserData),
        .pTagChildren = clIocWaterMarkChildrenParserData,
    },
#endif
};

static ClParserDataT clIocNodeInstanceChildrenParserData[] = {
    CPDI_BASE("sendQueue",clIocQueueParserTags,(ClPtrT)&pAllConfig.iocConfigInfo.iocSendQInfo,1,clIocQueueChildrenParserData),
    CPDI_BASE("receiveQueue",clIocQueueParserTags,(ClPtrT)&pAllConfig.iocConfigInfo.iocRecvQInfo,1,clIocQueueChildrenParserData),
    
#if 0    
    {
        .pTag = "sendQueue",
        .numTags = CL_PARSER_NUM(clIocQueueParserTags),
        .pTags = clIocQueueParserTags,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo.iocSendQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocQueueChildrenParserData),
        .pTagChildren = clIocQueueChildrenParserData,
    },
    {
        .pTag = "receiveQueue",
        .numTags = CL_PARSER_NUM(clIocQueueParserTags),
        .pTags = clIocQueueParserTags,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo.iocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocQueueChildrenParserData),
        .pTagChildren = clIocQueueChildrenParserData,
    },
#endif
};

static ClParserDataT clIocNodeInstancesChildrenParserData[] = {
    CPDI_BASE("nodeInstance",clIocNodeInstanceParserTags,(ClPtrT)&pAllConfig.iocConfigInfo,0,clIocNodeInstanceChildrenParserData),
#if 0    
    {
        .pTag = "nodeInstance",
        .numTags = CL_PARSER_NUM(clIocNodeInstanceParserTags),
        .pTags = clIocNodeInstanceParserTags,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .pTagChildren = clIocNodeInstanceChildrenParserData,
        .numChild = CL_PARSER_NUM(clIocNodeInstanceChildrenParserData),
    },
#endif
};

static ClParserDataT clIocChildrenParserData[] = {    
    CPDI_BASE("sendQueue",clIocQueueParserTags,(ClPtrT)&pAllConfig.iocConfigInfo.iocSendQInfo,1,clIocQueueChildrenParserData),
    CPDI_BASE("receiveQueue",clIocQueueParserTags,(ClPtrT)&pAllConfig.iocConfigInfo.iocRecvQInfo,1,clIocQueueChildrenParserData),
    CPDI_NODE("transport",clIocTransportParserTags,clIocTransportDataInit,0,clIocLinkParserData),
    CPDI_BASE("nodeInstances",((ClParserTagT *)NULL),(ClPtrT)&pAllConfig.iocConfigInfo,1,clIocNodeInstancesChildrenParserData),
    
#if 0    
    {
        .pTag = "sendQueue",
        .numTags = CL_PARSER_NUM(clIocQueueParserTags),
        .pTags = clIocQueueParserTags,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo.iocSendQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocQueueChildrenParserData),
        .pTagChildren = clIocQueueChildrenParserData,
    },
    {
        .pTag = "receiveQueue",
        .numTags = CL_PARSER_NUM(clIocQueueParserTags),
        .pTags = clIocQueueParserTags,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo.iocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocQueueChildrenParserData),
        .pTagChildren = clIocQueueChildrenParserData,
    },
    {
        .pTag = "transport",
        .numTags = CL_PARSER_NUM(clIocTransportParserTags),
        .pTags = clIocTransportParserTags,
        .pDataInit = clIocTransportDataInit,
        .numInstances = 0,
        .pTagChildren = clIocLinkParserData,
        .numChild = CL_PARSER_NUM(clIocLinkParserData),
    },
    {
        .pTag = "nodeInstances",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&pAllConfig.iocConfigInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .pTagChildren = clIocNodeInstancesChildrenParserData,
        .numChild = CL_PARSER_NUM(clIocNodeInstancesChildrenParserData),
    }
#endif

};

static ClParserDataT clIocParserData[] = {
    CPDI_BASE("ioc",clIocParserTags,(ClPtrT)&pAllConfig,1,clIocChildrenParserData),
#if 0        
    {
        .pTag = "ioc",
        .numTags = CL_PARSER_NUM(clIocParserTags),
        .pTags = clIocParserTags,
        .pBase = (ClPtrT)&pAllConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .pTagChildren = clIocChildrenParserData,
        .numChild = CL_PARSER_NUM(clIocChildrenParserData),
    }
#endif   
};

static ClParserDataT clIocConfigParserData =  CPDI_BASE("ioc:BootConfig",((ClParserTagT *)NULL),(ClPtrT)&pAllConfig,1,clIocParserData);
#if 0
{
    .pTag = "ioc:BootConfig",
    .numTags = 0,
    .pTags = NULL,
    .pBase = (ClPtrT)&pAllConfig,
    .baseOffset = 0,
    .pDataInit = NULL,
    .numInstances = 1,
    .numChild = CL_PARSER_NUM(clIocParserData),
    .pTagChildren = clIocParserData,
};
#endif
/*
 * End of IOC config parser data.
*/
