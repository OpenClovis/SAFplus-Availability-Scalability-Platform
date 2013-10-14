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
        .pTag = "heapConfig",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClEoParseInfoT,heapConfigName),
        .tagOffset = CL_TAG_OFFSET(ClEoParseInfoT,heapConfigName),
    },
    {
        .pTag = "bufferConfig",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClEoParseInfoT,buffConfigName),
        .tagOffset = CL_TAG_OFFSET(ClEoParseInfoT,buffConfigName),
    },
    {
        .pTag = "memoryConfig",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClEoParseInfoT,memConfigName),
        .tagOffset = CL_TAG_OFFSET(ClEoParseInfoT,memConfigName),
    },
};
static ClParserTagT clEoIocNameConfigParserTags[] = {
    {
        .pTag = "iocConfig",
        .tagType = CL_PARSER_STR_TAG,
        .tagSize = CL_TAG_SIZE(ClEoParseInfoT,iocConfigName),
        .tagOffset = CL_TAG_OFFSET(ClEoParseInfoT,iocConfigName),
    },
};

static ClParserDataT clEoMemConfigParserData[] = {
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
};

static ClParserDataT clEoConfigParserData[] = {
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
};

static ClParserDataT clEoConfigListParserData = {
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

/*
 * The data for clEoDefinitions.xml which is a bit tricky.
 */

/*
 * Define the tags for heapConfigPool
 *
 */
static ClParserTagT clHeapConfigParserTags[] = {
    {
        .pTag = "name",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_PARSER_ATTR_SIZE,
        /*
         * Since we are going to skip update for this value,
         * we go for an invalid tag offset.
         */
        .tagOffset = 0,
        .pTagFmt = clHeapConfigNameTagFmt,
    },
    {
        .pTag = "mode",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_TAG_SIZE(ClHeapConfigT,mode),
        .tagOffset = CL_TAG_OFFSET(ClHeapConfigT,mode),
        .pTagFmt = clHeapConfigModeTagFmt,
    },
    {
        .pTag = "lazyMode",
        .tagType = CL_PARSER_BOOL_TAG,
        .tagSize = CL_TAG_SIZE(ClHeapConfigT,lazy),
        .tagOffset = CL_TAG_OFFSET(ClHeapConfigT,lazy),
        .pTagFmt = NULL,
    },
};

static ClParserTagT clPoolConfigParserTags[] = {
    {
        .pTag = "chunkSize",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize  = CL_TAG_SIZE(ClPoolConfigT,chunkSize),
        .tagOffset = CL_TAG_OFFSET(ClPoolConfigT,chunkSize),
        .pTagFmt = NULL,
    },
    {
        .pTag = "initialSize",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClPoolConfigT,initialPoolSize),
        .tagOffset = CL_TAG_OFFSET(ClPoolConfigT,initialPoolSize),
        .pTagFmt = NULL,
    },
    {
        .pTag = "incrementSize",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClPoolConfigT,incrementPoolSize),
        .tagOffset = CL_TAG_OFFSET(ClPoolConfigT,incrementPoolSize),
        .pTagFmt = NULL,
    },
    {
        .pTag = "maxSize",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClPoolConfigT,maxPoolSize),
        .tagOffset = CL_TAG_OFFSET(ClPoolConfigT,maxPoolSize),
        .pTagFmt = NULL
    },
};

static ClParserDataT clHeapConfigChildrenParserData[] = {
    {
        .pTag = "pool",
        .numTags = CL_PARSER_NUM(clPoolConfigParserTags),
        .pTags = clPoolConfigParserTags,
        .numInstances = 0,
        .pDataInit = clHeapConfigPoolDataInit,
        .numChild = 0,
        .pTagChildren = NULL,
    },
};

static ClParserDataT clHeapChildrenParserData[] = {
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
};

/*
 * Define tags for buffer config
 *
 */
static ClParserTagT clBufferConfigParserTags[] = {
    {
        .pTag = "name",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_PARSER_ATTR_SIZE,
        .tagOffset = 0,
        .pTagFmt = clBufferConfigNameTagFmt,
    },

    {
        .pTag = "lazyMode",
        .tagType = CL_PARSER_BOOL_TAG,
        .tagSize = CL_TAG_SIZE(ClBufferPoolConfigT,lazy),
        .tagOffset = CL_TAG_OFFSET(ClBufferPoolConfigT,lazy),
        .pTagFmt = NULL,
    },

    {
        .pTag = "mode",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_TAG_SIZE(ClBufferPoolConfigT,mode),
        .tagOffset = CL_TAG_OFFSET(ClBufferPoolConfigT,mode),
        .pTagFmt = clBufferConfigModeTagFmt,
    },

};

/*
 * Define the tags for memoryConfig.
 */
static ClParserTagT clMemConfigParserTags[] = {
    {
        .pTag = "name",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_PARSER_ATTR_SIZE,
        .tagOffset = 0,
        .pTagFmt = clMemConfigNameTagFmt,
    },
    {
        .pTag = "processUpperLimit",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize = CL_TAG_SIZE(ClEoMemConfigT,memLimit),
        .tagOffset = CL_TAG_OFFSET(ClEoMemConfigT,memLimit),
        .pTagFmt = NULL,
    },
};

/*
 * Define the watermark tags
 */
static ClParserTagT clWaterMarkParserTags[] = {
    {
        .pTag = "lowLimit",
        .tagType = CL_PARSER_UINT64_TAG,
        .tagSize = CL_TAG_SIZE(ClWaterMarkT,lowLimit),
        .tagOffset = CL_TAG_OFFSET(ClWaterMarkT,lowLimit),
        .pTagFmt = NULL,
        .pTagValidate = clWaterMarkValidate,
    },
    {
        .pTag = "highLimit",
        .tagType = CL_PARSER_UINT64_TAG,
        .tagSize = CL_TAG_SIZE(ClWaterMarkT,highLimit),
        .tagOffset = CL_TAG_OFFSET(ClWaterMarkT,highLimit),
        .pTagFmt = NULL,
        .pTagValidate = clWaterMarkValidate,
    },
};

/*
 * Define the action tags.
 */
static ClParserTagT clWaterMarkActionParserTags[] = {
    {
        .pTag = "enable",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_TAG_SIZE(ClEoActionInfoT,bitMap),
        .tagOffset = CL_TAG_OFFSET(ClEoActionInfoT,bitMap),
        .pTagFmt = clWaterMarkActionTagFmt,
    },
};

/*
 * Define the parser data for the buffer config pool
 */
static ClParserDataT clBufferConfigChildrenParserData[] = {
    {
        .pTag = "pool",
        .numTags = CL_PARSER_NUM(clPoolConfigParserTags),
        .pTags = clPoolConfigParserTags,
        .pDataInit = clBufferConfigPoolDataInit,
        .numInstances = 0,
        .numChild = 0,
        .pTagChildren = NULL,
    },
};

static ClParserDataT clBufferChildrenParserData[] = {
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
};

static ClParserDataT clWaterMarkActionChildrenParserData[] = {
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
};

static ClParserDataT clWaterMarkActionParserData[] = {
    {
        .pTag = "action",
        .numTags = 0,
        .pTags = NULL,
        .pDataInit = clWaterMarkActionDataInit,
        .numInstances = 1,
        .numChild = CL_PARSER_NUM(clWaterMarkActionChildrenParserData),
        .pTagChildren = clWaterMarkActionChildrenParserData,
    },
};

static ClParserDataT clMemConfigChildrenParserData[] = {
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
};

static ClParserDataT clMemChildrenParserData[] = {
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
};

static ClParserDataT clIocWaterMarkActionChildrenParserData[] = {
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
};

static ClParserDataT clIocWaterMarkActionParserData[] = {
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
};
static ClParserDataT clIocConfigRecvQChildrenParserData[] = {
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
};
static ClParserTagT clIocRecvQConfigParserTags[] = {
    {
        .pTag = "size",
        .tagType = CL_PARSER_UINT32_TAG,
        .tagSize  = CL_TAG_SIZE(ClIocQueueInfoT,queueSize),
        .tagOffset = CL_TAG_OFFSET(ClIocQueueInfoT,queueSize),
        .pTagFmt = NULL,
    },
};
static ClParserTagT clIocConfigParserTags[] = {
    {
        .pTag = "name",
        .tagType = CL_PARSER_CUSTOM_TAG,
        .tagSize = CL_PARSER_ATTR_SIZE,
        .tagOffset = 0,
        .pTagFmt = clIocConfigNameTagFmt,
    }
}; 
static ClParserDataT clIocConfigRecvQParserData[] = {
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
};
static ClParserDataT clIocConfigChildrenParserData[] = {
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
};
static ClParserDataT clIocConfigPoolParserData[] = {
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
};
/*  
 * Now define the data for the top level children of MemoryConfiguration.
 */
static ClParserDataT clEoMemChildrenParserData[] = {
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
};

static ClParserDataT clEoDefinitionChildrenParserData[] = {
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
};

static ClParserDataT clEoDefinitionsParserData = {
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

/*
 * End of EO config parser data.
*/
