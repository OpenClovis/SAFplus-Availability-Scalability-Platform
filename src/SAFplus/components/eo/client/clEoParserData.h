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
#include <clEoLibs.h>

#ifndef _CL_EO_PARSER_C_
#error "_CL_EO_PARSER_C_ undefined.This file should be included only from eoparser file"
#endif

#define CPDI_EO_LEAF(ptag,pTags,pBase) { ptag,CL_PARSER_NUM(pTags),pTags,NULL,pBase,0,1,CL_FALSE,0,NULL }
#define CPDI_EO(ptag,pTags,pDataInit,numInstances) { ptag,CL_PARSER_NUM(pTags),pTags,pDataInit,0,0,numInstances,CL_FALSE,0,NULL }
#define CPDI_EO_NODE(ptag,pTags,pDataInit,numInstances,pTagChildren) { ptag,CL_PARSER_NUM(pTags),pTags,pDataInit,0,0,numInstances,CL_FALSE,CL_PARSER_NUM(pTagChildren),pTagChildren }
#define CPDI_EO_BASE(ptag,pTags,pBase,numInstances,pTagChildren) { ptag,CL_PARSER_NUM(pTags),pTags,NULL,pBase,0,numInstances,CL_FALSE,CL_PARSER_NUM(pTagChildren),pTagChildren }


/*
 * Data Init callbacks for the generic parser
 */
static ClParserDataInitFunctionT clHeapConfigPoolDataInit;
static ClParserDataInitFunctionT clBufferConfigPoolDataInit;
static ClParserDataInitFunctionT clWaterMarkActionDataInit;

/*
 * Tag fmt. callbacks for the generic parser.
*/
static ClParserTagFmtFunctionT   clEoConfigTagFmt;
static ClParserTagFmtFunctionT   clHeapConfigModeTagFmt;
static ClParserTagFmtFunctionT   clBufferConfigModeTagFmt;
static ClParserTagFmtFunctionT   clWaterMarkActionTagFmt;
static ClParserTagFmtFunctionT   clHeapConfigNameTagFmt;
static ClParserTagFmtFunctionT   clBufferConfigNameTagFmt;
static ClParserTagFmtFunctionT   clMemConfigNameTagFmt;
static ClParserTagFmtFunctionT   clIocConfigNameTagFmt;

/*
 * Validate callbacks for the generic parser.
*/
static ClParserTagValidateFunctionT clWaterMarkValidate;

static ClParserTagT clEoConfigParserTags[] = {
    {
        "name", CL_PARSER_STR_TAG, CL_TAG_SIZE(ClEoParseInfoT,eoName), CL_TAG_OFFSET(ClEoParseInfoT,eoName), clEoConfigTagFmt, NULL
    },
};

static ClParserTagT clEoMemConfigParserTags[] = {
    {
        "heapConfig",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClEoParseInfoT,heapConfigName),
        CL_TAG_OFFSET(ClEoParseInfoT,heapConfigName),
    },
    {
        "bufferConfig",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClEoParseInfoT,buffConfigName),
        CL_TAG_OFFSET(ClEoParseInfoT,buffConfigName),
    },
    {
        "memoryConfig",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClEoParseInfoT,memConfigName),
        CL_TAG_OFFSET(ClEoParseInfoT,memConfigName),
    },
};
static ClParserTagT clEoIocNameConfigParserTags[] = {
    {
        "iocConfig",
        CL_PARSER_STR_TAG,
        CL_TAG_SIZE(ClEoParseInfoT,iocConfigName),
        CL_TAG_OFFSET(ClEoParseInfoT,iocConfigName),
    },
};

static ClParserDataT clEoMemConfigParserData[] = {
    CPDI_EO_LEAF("eoMemConfig",clEoMemConfigParserTags,(ClPtrT)&parseConfigs),
    CPDI_EO_LEAF("eoIocConfig",clEoIocNameConfigParserTags,(ClPtrT)&parseConfigs),
#if 0
    {
        .pTag = "eoMemConfig",
        .numTags = CL_PARSER_NUM(clEoMemConfigParserTags),
        .pTags= clEoMemConfigParserTags,
        .pBase = (ClPtrT)&parseConfigs,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
    },
    {
        .pTag = "eoIocConfig",
        .numTags = CL_PARSER_NUM(clEoIocNameConfigParserTags),
        .pTags= clEoIocNameConfigParserTags,
        .pBase = (ClPtrT)&parseConfigs,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
    },
#endif
};

static ClParserDataT clEoConfigParserData[] = {
    CPDI_EO_BASE("eoConfig",clEoConfigParserTags,(ClPtrT)&parseConfigs,0,clEoMemConfigParserData),
#if 0
    {
        .pTag = "eoConfig",
        .numTags = CL_PARSER_NUM(clEoConfigParserTags),
        .pTags = clEoConfigParserTags,
        .numInstances = 0,
        .pBase = (ClPtrT)&parseConfigs,
        .baseOffset = 0,
        .pDataInit = NULL,
        .pTagChildren = clEoMemConfigParserData,
        .numChild = CL_PARSER_NUM(clEoMemConfigParserData),
    }
#endif
};

static ClParserDataT clEoConfigListParserData = CPDI_EO_BASE("EOList",((ClParserTagT *)NULL),(ClPtrT)&parseConfigs,1,clEoConfigParserData);
#if 0
{
    .pTag = "EOList",
    .numTags = 0,
    .pTags = NULL,
    .pBase = (ClPtrT)&parseConfigs,
    .baseOffset = 0,
    .pDataInit = NULL,
    .numInstances = 1,
    .numChild = CL_PARSER_NUM(clEoConfigParserData),
    .pTagChildren = clEoConfigParserData,
};
#endif

/*
 * The data for clEoDefinitions.xml which is a bit tricky.
 */

/*
 * Define the tags for heapConfigPool
 *
 */
static ClParserTagT clHeapConfigParserTags[] = {
    {
        "name",
        CL_PARSER_CUSTOM_TAG,
        CL_PARSER_ATTR_SIZE,
        /*
         * Since we are going to skip update for this value,
         * we go for an invalid tag offset.
         */
        0,
        clHeapConfigNameTagFmt,
    },
    {
        "mode",
        CL_PARSER_CUSTOM_TAG,
        CL_TAG_SIZE(ClHeapConfigT,mode),
        CL_TAG_OFFSET(ClHeapConfigT,mode),
        clHeapConfigModeTagFmt,
    },
    {
        "lazyMode",
        CL_PARSER_BOOL_TAG,
        CL_TAG_SIZE(ClHeapConfigT,lazy),
        CL_TAG_OFFSET(ClHeapConfigT,lazy),
        NULL,
    },
};

static ClParserTagT clPoolConfigParserTags[] = {
    {
        "chunkSize",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClPoolConfigT,chunkSize),
        CL_TAG_OFFSET(ClPoolConfigT,chunkSize),
        NULL,
    },
    {
        "initialSize",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClPoolConfigT,initialPoolSize),
        CL_TAG_OFFSET(ClPoolConfigT,initialPoolSize),
        NULL,
    },
    {
        "incrementSize",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClPoolConfigT,incrementPoolSize),
        CL_TAG_OFFSET(ClPoolConfigT,incrementPoolSize),
        NULL,
    },
    {
        "maxSize",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClPoolConfigT,maxPoolSize),
        CL_TAG_OFFSET(ClPoolConfigT,maxPoolSize),
        NULL
    },
};

static ClParserDataT clHeapConfigChildrenParserData[] = {
    CPDI_EO("pool",clPoolConfigParserTags,clHeapConfigPoolDataInit,0),
#if 0
    {
        .pTag = "pool",
        .numTags = CL_PARSER_NUM(clPoolConfigParserTags),
        .pTags = clPoolConfigParserTags,
        .numInstances = 0,
        .pDataInit = clHeapConfigPoolDataInit,
        .numChild = 0,
        .pTagChildren = NULL,
    },
#endif

};

static ClParserDataT clHeapChildrenParserData[] = {
    CPDI_EO_BASE("heapConfig",clHeapConfigParserTags,(ClPtrT)&gClEoHeapConfig,0,clHeapConfigChildrenParserData),
#if 0
    {
        .pTag = "heapConfig",
        .numTags = CL_PARSER_NUM(clHeapConfigParserTags),
        .pTags = clHeapConfigParserTags,
        .pBase = (ClPtrT)&gClEoHeapConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 0,
        .numChild = CL_PARSER_NUM(clHeapConfigChildrenParserData),
        .pTagChildren = clHeapConfigChildrenParserData,
    },
#endif
};

/*
 * Define tags for buffer config
 *
 */
static ClParserTagT clBufferConfigParserTags[] = {
    {
        "name",
        CL_PARSER_CUSTOM_TAG,
        CL_PARSER_ATTR_SIZE,
        0,
        clBufferConfigNameTagFmt,
    },

    {
        "lazyMode",
        CL_PARSER_BOOL_TAG,
        CL_TAG_SIZE(ClBufferPoolConfigT,lazy),
        CL_TAG_OFFSET(ClBufferPoolConfigT,lazy),
        NULL,
    },

    {
        "mode",
        CL_PARSER_CUSTOM_TAG,
        CL_TAG_SIZE(ClBufferPoolConfigT,mode),
        CL_TAG_OFFSET(ClBufferPoolConfigT,mode),
        clBufferConfigModeTagFmt,
    },

};

/*
 * Define the tags for memoryConfig.
 */
static ClParserTagT clMemConfigParserTags[] = {
    {
        "name",
        CL_PARSER_CUSTOM_TAG,
        CL_PARSER_ATTR_SIZE,
        0,
        clMemConfigNameTagFmt,
    },
    {
        "processUpperLimit",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClEoMemConfigT,memLimit),
        CL_TAG_OFFSET(ClEoMemConfigT,memLimit),
        NULL,
    },
};

/*
 * Define the watermark tags
 */
static ClParserTagT clWaterMarkParserTags[] = {
    {
        "lowLimit",
        CL_PARSER_UINT64_TAG,
        CL_TAG_SIZE(ClWaterMarkT,lowLimit),
        CL_TAG_OFFSET(ClWaterMarkT,lowLimit),
        NULL,
        clWaterMarkValidate,
    },
    {
        "highLimit",
        CL_PARSER_UINT64_TAG,
        CL_TAG_SIZE(ClWaterMarkT,highLimit),
        CL_TAG_OFFSET(ClWaterMarkT,highLimit),
        NULL,
        clWaterMarkValidate,
    },
};

/*
 * Define the action tags.
 */
static ClParserTagT clWaterMarkActionParserTags[] = {
    {
        "enable",
        CL_PARSER_CUSTOM_TAG,
        CL_TAG_SIZE(ClEoActionInfoT,bitMap),
        CL_TAG_OFFSET(ClEoActionInfoT,bitMap),
        clWaterMarkActionTagFmt,
    },
};

/*
 * Define the parser data for the buffer config pool
 */
static ClParserDataT clBufferConfigChildrenParserData[] = {
    CPDI_EO("pool",clPoolConfigParserTags,clBufferConfigPoolDataInit,0),
#if 0
    {
        .pTag = "pool",
        .numTags = CL_PARSER_NUM(clPoolConfigParserTags),
        .pTags = clPoolConfigParserTags,
        .pDataInit = clBufferConfigPoolDataInit,
        .numInstances = 0,
        .numChild = 0,
        .pTagChildren = NULL,
    },
#endif
};

static ClParserDataT clBufferChildrenParserData[] = {
    CPDI_EO_BASE("bufferConfig",clBufferConfigParserTags,(ClPtrT)&gClEoBuffConfig,1,clBufferConfigChildrenParserData),
#if 0
    {
        .pTag = "bufferConfig",
        .numTags = CL_PARSER_NUM(clBufferConfigParserTags),
        .pTags = clBufferConfigParserTags,
        .pBase = (ClPtrT)&gClEoBuffConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clBufferConfigChildrenParserData),
        .pTagChildren = clBufferConfigChildrenParserData,
    },
#endif
};

static ClParserDataT clWaterMarkActionChildrenParserData[] = {
    CPDI_EO("event",clWaterMarkActionParserTags,clWaterMarkActionDataInit,1),
    CPDI_EO("notification",clWaterMarkActionParserTags,clWaterMarkActionDataInit,1),
    CPDI_EO("log",clWaterMarkActionParserTags,clWaterMarkActionDataInit,1),
    CPDI_EO("custom",clWaterMarkActionParserTags,clWaterMarkActionDataInit,1),
#if 0
    {
        .pTag = "event",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "notification",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "log",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "custom",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
#endif
};

static ClParserDataT clWaterMarkActionParserData[] = {
    CPDI_EO_NODE("action",((ClParserTagT *)NULL),clWaterMarkActionDataInit,1,clWaterMarkActionChildrenParserData),
#if 0
    {
        .pTag = "action",
        .numTags = 0,
        .pTags = NULL,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clWaterMarkActionChildrenParserData),
        .pTagChildren = clWaterMarkActionChildrenParserData,
    },
#endif
};

static ClParserDataT clMemConfigChildrenParserData[] = {
    CPDI_EO_BASE("highWaterMark",clWaterMarkParserTags,(ClPtrT)&gClEoMemConfig.memHighWaterMark,1,clWaterMarkActionParserData),
    CPDI_EO_BASE("mediumWaterMark",clWaterMarkParserTags,(ClPtrT)&gClEoMemConfig.memMediumWaterMark,1,clWaterMarkActionParserData),
    CPDI_EO_BASE("lowWaterMark",clWaterMarkParserTags,(ClPtrT)&gClEoMemConfig.memLowWaterMark,1,clWaterMarkActionParserData),
#if 0
    {
        .pTag = "highWaterMark",
        .numTags = CL_PARSER_NUM(clWaterMarkParserTags),
        .pTags = clWaterMarkParserTags,
        .pBase = (ClPtrT)&gClEoMemConfig.memHighWaterMark,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clWaterMarkActionParserData),
        .pTagChildren = clWaterMarkActionParserData,
    },
    {
        .pTag = "mediumWaterMark",
        .numTags = CL_PARSER_NUM(clWaterMarkParserTags),
        .pTags = clWaterMarkParserTags,
        .pBase = (ClPtrT)&gClEoMemConfig.memMediumWaterMark,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clWaterMarkActionParserData),
        .pTagChildren = clWaterMarkActionParserData,
    },
    {
        .pTag = "lowWaterMark",
        .numTags = CL_PARSER_NUM(clWaterMarkParserTags),
        .pTags = clWaterMarkParserTags,
        .pBase = (ClPtrT)&gClEoMemConfig.memLowWaterMark,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clWaterMarkActionParserData),
        .pTagChildren = clWaterMarkActionParserData,
    },
#endif
};

static ClParserDataT clMemChildrenParserData[] = {
    CPDI_EO_BASE("memoryConfig",clMemConfigParserTags,(ClPtrT)&gClEoMemConfig,0,clMemConfigChildrenParserData),
#if 0
    {
        .pTag = "memoryConfig",
        .numTags = CL_PARSER_NUM(clMemConfigParserTags),
        .pTags = clMemConfigParserTags,
        .pBase = (ClPtrT)&gClEoMemConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 0,
        .numChild = CL_PARSER_NUM(clMemConfigChildrenParserData),
        .pTagChildren = clMemConfigChildrenParserData,
    },
#endif
};

static ClParserDataT clIocWaterMarkActionChildrenParserData[] = {
    CPDI_EO_LEAF("event",clWaterMarkActionParserTags,(ClPtrT)&gClIocRecvQActions),
    CPDI_EO_LEAF("notification",clWaterMarkActionParserTags,(ClPtrT)&gClIocRecvQActions),
    CPDI_EO_LEAF("log",clWaterMarkActionParserTags,(ClPtrT)&gClIocRecvQActions),
    CPDI_EO_LEAF("custom",clWaterMarkActionParserTags,(ClPtrT)&gClIocRecvQActions),
#if 0
    {
        .pTag = "event",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pBase = (ClPtrT)&gClIocRecvQActions,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "notification",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pBase = (ClPtrT)&gClIocRecvQActions,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "log",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pBase = (ClPtrT)&gClIocRecvQActions,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
    {
        .pTag = "custom",
        .numTags = CL_PARSER_NUM(clWaterMarkActionParserTags),
        .pTags = clWaterMarkActionParserTags,
        .pBase = (ClPtrT)&gClIocRecvQActions,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = 0,
        .pTagChildren = NULL,
    },
#endif
};

static ClParserDataT clIocWaterMarkActionParserData[] = {
    CPDI_EO_BASE("action",((ClParserTagT *)NULL),(ClPtrT)&gClIocRecvQActions,1,clIocWaterMarkActionChildrenParserData),
#if 0
    {
        .pTag = "action",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClIocRecvQActions,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkActionChildrenParserData),
        .pTagChildren = clIocWaterMarkActionChildrenParserData,
    },
#endif
};
static ClParserDataT clIocConfigRecvQChildrenParserData[] = {
    CPDI_EO_BASE("waterMark",clWaterMarkParserTags,(ClPtrT)&(gClIocRecvQInfo.queueWM),1,clIocWaterMarkActionParserData),
#if 0
    {
        .pTag = "waterMark",
        .numTags = CL_PARSER_NUM(clWaterMarkParserTags),
        .pTags = clWaterMarkParserTags,
        .pBase = (ClPtrT)&(gClIocRecvQInfo.queueWM),
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocWaterMarkActionParserData),
        .pTagChildren = clIocWaterMarkActionParserData,
    },
#endif
};
static ClParserTagT clIocRecvQConfigParserTags[] = {
    {
        "size",
        CL_PARSER_UINT32_TAG,
        CL_TAG_SIZE(ClIocQueueInfoT,queueSize),
        CL_TAG_OFFSET(ClIocQueueInfoT,queueSize),
        NULL,
    },
};
static ClParserTagT clIocConfigParserTags[] = {
    {
        "name",
        CL_PARSER_CUSTOM_TAG,
        CL_PARSER_ATTR_SIZE,
        0,
        clIocConfigNameTagFmt,
    }
}; 
static ClParserDataT clIocConfigRecvQParserData[] = {
    CPDI_EO_BASE("receiveQueue",clIocRecvQConfigParserTags,(ClPtrT)&gClIocRecvQInfo,1,clIocConfigRecvQChildrenParserData),
#if 0
    {
        .pTag = "receiveQueue",
        .numTags = CL_PARSER_NUM(clIocRecvQConfigParserTags),
        .pTags = clIocRecvQConfigParserTags,
        .pBase = (ClPtrT)&gClIocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL, 
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocConfigRecvQChildrenParserData),
        .pTagChildren = clIocConfigRecvQChildrenParserData,
    },
#endif 
};
static ClParserDataT clIocConfigChildrenParserData[] = {
    CPDI_EO_BASE("iocConfig",clIocConfigParserTags,(ClPtrT)&gClIocRecvQInfo,0,clIocConfigRecvQParserData),
#if 0
    {
        .pTag = "iocConfig",
        .numTags = CL_PARSER_NUM(clIocConfigParserTags),
        .pTags = clIocConfigParserTags,
        .pBase = (ClPtrT)&gClIocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 0, 
        .numChild = CL_PARSER_NUM(clIocConfigRecvQParserData),
        .pTagChildren = clIocConfigRecvQParserData,
    },
#endif
};
static ClParserDataT clIocConfigPoolParserData[] = {
    CPDI_EO_BASE("iocConfigPool",((ClParserTagT *)NULL),(ClPtrT)&gClIocRecvQInfo,1,clIocConfigChildrenParserData),
#if 0
    {
        .pTag = "iocConfigPool",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClIocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1, 
        .numChild = CL_PARSER_NUM(clIocConfigChildrenParserData),
        .pTagChildren = clIocConfigChildrenParserData,
    },
#endif
};
/*  
 * Now define the data for the top level children of MemoryConfiguration.
 */
static ClParserDataT clEoMemChildrenParserData[] = {
    CPDI_EO_BASE("heapConfigPool",((ClParserTagT *)NULL),(ClPtrT)&gClEoHeapConfig,1,clHeapChildrenParserData),
    CPDI_EO_BASE("bufferConfigPool",((ClParserTagT *)NULL),(ClPtrT)&gClEoBuffConfig,1,clBufferChildrenParserData),
    CPDI_EO_BASE("memoryConfigPool",((ClParserTagT *)NULL),(ClPtrT)&gClEoMemConfig,1,clMemChildrenParserData),
#if 0
    {
        .pTag = "heapConfigPool",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClEoHeapConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clHeapChildrenParserData),
        .pTagChildren = clHeapChildrenParserData,
    },
    {
        .pTag = "bufferConfigPool",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClEoBuffConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clBufferChildrenParserData),
        .pTagChildren = clBufferChildrenParserData,
    },
    {
        .pTag = "memoryConfigPool",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClEoMemConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clMemChildrenParserData),
        .pTagChildren = clMemChildrenParserData,
    },
#endif
};

static ClParserDataT clEoDefinitionChildrenParserData[] = {
    CPDI_EO_BASE("memoryConfiguration",((ClParserTagT *)NULL),(ClPtrT)&gClEoMemConfig,1,clEoMemChildrenParserData),
    CPDI_EO_BASE("iocConfiguration",((ClParserTagT *)NULL),(ClPtrT)&gClIocRecvQInfo,1,clIocConfigPoolParserData),
#if 0
    {
        .pTag = "memoryConfiguration",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClEoMemConfig,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clEoMemChildrenParserData),
        .pTagChildren = clEoMemChildrenParserData,
    },
    {
        .pTag = "iocConfiguration",
        .numTags = 0,
        .pTags = NULL,
        .pBase = (ClPtrT)&gClIocRecvQInfo,
        .baseOffset = 0,
        .pDataInit = NULL,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clIocConfigPoolParserData), 
        .pTagChildren = clIocConfigPoolParserData,
    },
#endif
};

static ClParserDataT clEoDefinitionsParserData = CPDI_EO_BASE("eoDefinitions",((ClParserTagT *)NULL),(ClPtrT)&gClEoMemConfig,1,clEoDefinitionChildrenParserData);
#if 0
{
    .pTag = "eoDefinitions",
    .numTags = 0,
    .pTags = NULL,
    .pBase = (ClPtrT)&gClEoMemConfig,
    .baseOffset = 0,
    .pDataInit = NULL,
    .numInstances = 1,
    .numChild = CL_PARSER_NUM(clEoDefinitionChildrenParserData),
    .pTagChildren = clEoDefinitionChildrenParserData,
};
#endif

/*
 * End of EO config parser data.
*/
