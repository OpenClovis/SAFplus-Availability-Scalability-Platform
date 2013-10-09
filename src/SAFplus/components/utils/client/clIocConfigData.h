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
        .pTag  = "reassemblyTimeout",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClIocLibConfigT,iocReassemblyTimeOut),
        .tagOffset = CL_TAG_OFFSET(ClIocConfigT,iocConfigInfo) + CL_TAG_OFFSET(ClIocLibConfigT,iocReassemblyTimeOut),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "heartBeatInterval",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClIocLibConfigT,iocHeartbeatTimeInterval),
        .tagOffset = CL_TAG_OFFSET(ClIocConfigT,iocConfigInfo) + CL_TAG_OFFSET(ClIocLibConfigT,iocHeartbeatTimeInterval),
        .pTagFmt = NULL,
    },
};

/*Tags for the children of IOC*/
static ClParserTagT clIocTransportParserTags[] = {
    { 
        .pTag  = "transportName",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserTransportConfigT,pName),
        .tagOffset = CL_TAG_OFFSET(ClIocUserTransportConfigT,pName),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "transportPriority",
        .tagType = CL_PARSER_UINT8_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserTransportConfigT,priority),
        .tagOffset = CL_TAG_OFFSET(ClIocUserTransportConfigT,priority),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "transportId",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserTransportConfigT,id),
        .tagOffset = CL_TAG_OFFSET(ClIocUserTransportConfigT,id),
        .pTagFmt = NULL,
    },
};

/*Tags for the children of transport*/
static ClParserTagT clIocLinkParserTags[] = {
    { 
        .pTag  = "linkName",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,pName),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,pName),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "linkPriority",
        .tagType = CL_PARSER_UINT8_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,priority),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,priority),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "linkInterface",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,pInterface),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,pInterface),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "linkMulticastAddress",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,pMcastAddress),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,pMcastAddress),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "linkSupportsMulticast",
        .tagType = CL_PARSER_BOOL_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,isMcastSupported),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,isMcastSupported),
        .pTagFmt = clIocLinkSupportsMulticastTagFmt,
    },
    { 
        .pTag  = "linkSupportsChksum",
        .tagType = CL_PARSER_BOOL_TAG,
        .tagSize = CL_TAG_SIZE(ClIocUserLinkCfgT,isCksumSupported),
        .tagOffset = CL_TAG_OFFSET(ClIocUserLinkCfgT,isCksumSupported),
        .pTagFmt = NULL,
    },
};

static ClParserTagT clIocLocationParserTags[] = {
    { 
        .pTag  = "slot",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClIocLocationInfoT,slotNum),
        .tagOffset = CL_TAG_OFFSET(ClIocLocationInfoT,slotNum),
        .pTagFmt = NULL,
    },
    { 
        .pTag  = "interfaceAddress",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClIocLocationInfoT,pInterfaceAddress),
        .tagOffset = CL_TAG_OFFSET(ClIocLocationInfoT,pInterfaceAddress),
        .pTagFmt = NULL,
    },
};

static ClParserTagT clIocQueueParserTags[] = {
    {
        .pTag = "size",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClIocQueueInfoT,queueSize),
        .tagOffset = CL_TAG_OFFSET(ClIocQueueInfoT,queueSize),
        .pTagFmt = NULL,
    },
};

/*
 * Define the action tags.
 */
static ClParserTagT clIocWaterMarkActionParserTags[] = {
    {
        .pTag = "enable",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_TAG_SIZE(ClEoActionInfoT,bitMap),
        .tagOffset = CL_TAG_OFFSET(ClEoActionInfoT,bitMap),
        .pTagFmt = clIocWaterMarkActionTagFmt,
    },
};

static ClParserTagT clIocWaterMarkParserTags[] = {
    {
        .pTag = "lowLimit",
        .tagType = CL_PARSER_UINT64_TAG,
        .tagSize = CL_TAG_SIZE(ClWaterMarkT,lowLimit),
        .tagOffset = CL_TAG_OFFSET(ClWaterMarkT,lowLimit),
        .pTagFmt = NULL,
        .pTagValidate = clIocQueueWMValidate,
    },
    {
        .pTag = "highLimit",
        .tagType = CL_PARSER_UINT64_TAG,
        .tagSize = CL_TAG_SIZE(ClWaterMarkT,highLimit),
        .tagOffset = CL_TAG_OFFSET(ClWaterMarkT,highLimit),
        .pTagFmt = NULL,
        .pTagValidate = clIocQueueWMValidate,
    },
};

static ClParserTagT clIocNodeInstanceParserTags[] = {
    {
        .pTag = "name",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = 0,
        .tagOffset = 0,
        .pTagFmt = clIocNodeInstanceNameTagFmt,
    },
};

#define CPDI(ptag,numTags,pTags,pDataInit,numInstances,pTagChildren,numChild) { ptag,numTags,pTags,pDataInit,0,0,numInstances,CL_FALSE,numChild,pTagChildren }

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
    CPDI("link",CL_PARSER_NUM(clIocLinkParserTags),clIocLinkParserTags,clIocLinkDataInit,0,clIocLocationParserData,CL_PARSER_NUM(clIocLocationParserData))
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
};

static ClParserDataT clIocWaterMarkChildrenParserData[] = {
    {
        .pTag = "action",
        .numTags = 0,
        .pTags = NULL,
        .pDataInit = clIocWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkActionChildrenParserData),
        .pTagChildren = clIocWaterMarkActionChildrenParserData,
    },
};

static ClParserDataT clIocQueueChildrenParserData[] = {
    {
        .pTag = "waterMark",
        .numTags = CL_PARSER_NUM(clIocWaterMarkParserTags),
        .pTags = clIocWaterMarkParserTags,
        .pDataInit = clIocWaterMarkDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkChildrenParserData),
        .pTagChildren = clIocWaterMarkChildrenParserData,
    },
};

static ClParserDataT clIocNodeInstanceChildrenParserData[] = {
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
};

static ClParserDataT clIocNodeInstancesChildrenParserData[] = {
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
};

static ClParserDataT clIocChildrenParserData[] = {
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
    },
};

static ClParserDataT clIocParserData[] = {
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
};

static ClParserDataT clIocConfigParserData = {
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

/*
 * End of IOC config parser data.
*/
