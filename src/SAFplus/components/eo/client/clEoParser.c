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
 * ModuleName  : eo
File        : clEoParser.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provides for reading the EO related XML files.
 *
 *
 ****************************************************************************/


#include <string.h>
#include <clEoParser.h>
#include <clEoLibs.h>
#include <clParserApi.h>
#include <clDebugApi.h>

/* 
 * Defines 
 */
#define  CL_LOG_AREA "EO"
#define  CL_LOG_CTXT "PRS"

# define CL_ASP_CONFIG_PATH             "ASP_CONFIG"
# define CL_EO_DEFINITIONS_FILE_NAME "clEoDefinitions.xml"
# define CL_EO_CONFIG_FILE_NAME      "clEoConfig.xml"
/*
 * The behaviour is the same for watermark data init. So why waste text space.
 */
#define clWaterMarkDataInit clEoMemConfigDataInit

#define CL_EO_CONFIG_ESSENTIAL (0x3)
#define CL_EO_CONFIG_VALID() ( isConfigValid == numConfigsRead )

/* 
 * TypeDefs 
 */
typedef struct ClEoParseInfo
{
    /*
     * number of eo configs: Not really used.
     * Just needed for generic parser display stuff.
     */
    ClCharT eoName[CL_MAX_NAME_LENGTH];
    ClCharT memConfigName[CL_MAX_NAME_LENGTH];
    ClCharT heapConfigName[CL_MAX_NAME_LENGTH];
    ClCharT buffConfigName[CL_MAX_NAME_LENGTH];
    ClCharT iocConfigName[CL_MAX_NAME_LENGTH];
} ClEoParseInfoT;

static ClEoParseInfoT parseConfigs = {{0}};
static ClUint32T isConfigValid;
static ClUint32T numConfigsRead;

/*
 * Include the EO config parser data.
 */
#define _CL_EO_PARSER_C_

#include "clEoParserData.h"

static ClRcT clEoConfigTagFmt(ClParserTagT *pTag,
                              ClParserDataT *pData,
                              ClPtrT pBase,
                              ClCharT *pAttr,
                              ClPtrT pValue,
                              ClParserTagOpT op
                              )
{
    ClRcT rc = CL_TAG_CONT;
    if(!strcmp((ClCharT*)pValue,CL_EO_NAME))
    {
        isConfigValid = 1;
        rc = CL_TAG_STOP_AFTER_CHILD;
        goto out;
    }
    out:
    return rc;
}

static ClRcT clEoMemConfigDataInit(ClPtrT pParentBase,
                                   ClUint32T *pNumInstances,
                                   ClPtrT *pBase,
                                   ClUint32T *pBaseOffset,
                                   ClParserTagOpT op,
                                   ClPtrT pData
                                   )
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    if(pBase == NULL || pBaseOffset == NULL || pNumInstances == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    *pBase = pData;
    *pBaseOffset = 0;
    if(op == CL_PARSER_TAG_DISPLAY)
    {
        *pNumInstances = 1;
    }
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clHeapConfigPoolDataInit(ClPtrT pParentBase,
                                ClUint32T *pNumInstances,
                                ClPtrT *pBase,
                                ClUint32T *pBaseOffset,
                                ClParserTagOpT op
                                )
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    ClPoolConfigT *pPoolConfig = NULL;
    ClHeapConfigT *pHeapConfig = pParentBase;

    if(pParentBase == NULL || pNumInstances == NULL ||  pBase == NULL
       || pBaseOffset == NULL
       )
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            pPoolConfig = calloc(*pNumInstances,sizeof(ClPoolConfigT));
            if(pPoolConfig == NULL)
            {
                rc = CL_EO_RC(CL_ERR_NO_MEMORY);
                clLogError(CL_LOG_AREA,CL_LOG_CTXT,"calloc error\n");
                goto out;
            }
            pHeapConfig->numPools = *pNumInstances;
            pHeapConfig->pPoolConfig = pPoolConfig;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            *pNumInstances = pHeapConfig->numPools;
            pPoolConfig = pHeapConfig->pPoolConfig;
        }
        break;
    default:
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid parser op: %d\n",op);
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }

    *pBase = pPoolConfig;
    *pBaseOffset = sizeof(ClPoolConfigT);
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clBufferConfigPoolDataInit(ClPtrT pParentBase,
                                        ClUint32T *pNumInstances,
                                        ClPtrT *pBase,
                                        ClUint32T *pBaseOffset,
                                        ClParserTagOpT op
                                        )
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    ClPoolConfigT *pPoolConfig = NULL;
    ClBufferPoolConfigT *pBufferConfig = pParentBase;

    if(pParentBase == NULL || pNumInstances == NULL ||  pBase == NULL
       || pBaseOffset == NULL
       )
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            pPoolConfig = calloc(*pNumInstances,sizeof(ClPoolConfigT));
            if(pPoolConfig == NULL)
            {
                rc = CL_EO_RC(CL_ERR_NO_MEMORY);
                clLogError(CL_LOG_AREA,CL_LOG_CTXT,"calloc error\n");
                goto out;
            }
            pBufferConfig->numPools = *pNumInstances;
            pBufferConfig->pPoolConfig = pPoolConfig;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            *pNumInstances = pBufferConfig->numPools;
            pPoolConfig = pBufferConfig->pPoolConfig;
        }
        break;
    default:
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid parser op: %d\n",op);
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }

    *pBase = pPoolConfig;
    *pBaseOffset = sizeof(ClPoolConfigT);
    rc = CL_OK;
    out:

    return rc;
}

static ClRcT clWaterMarkActionDataInit(ClPtrT pParentBase,
                                       ClUint32T *pNumInstances,
                                       ClPtrT *pBase,
                                       ClUint32T *pBaseOffset,
                                       ClParserTagOpT op
                                       )
{
    ClPtrT pCurrentBase = NULL;
    register ClUint32T i;
    
    ClPtrT *pBases[] = { 
        (ClPtrT)&gClEoMemConfig.memLowWaterMark,
        (ClPtrT)&gClEoMemConfig.memHighWaterMark,
        (ClPtrT)&gClEoMemConfig.memMediumWaterMark,
        NULL,
    };
    if(pParentBase == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"parent base NULL\n");
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }
    for(i = 0; pBases[i] && pParentBase != pBases[i]; ++i);
    if(pBases[i] == NULL)
    {
        /*
         * Search for the matching parent base in water marks
         * Height of laziness as we could have done this in 4 different
         * data inits for each action child.
         */
        ClPtrT *pActionBases[] = { 
            (ClPtrT)&gMemWaterMark[CL_WM_LOW],
            (ClPtrT)&gMemWaterMark[CL_WM_HIGH],
            (ClPtrT)&gMemWaterMark[CL_WM_MED],
            NULL,
        };
        for(i = 0; pActionBases[i] && pActionBases[i] != pParentBase; ++i);
        if(pActionBases[i] == NULL)
        {
            clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid parent base:%p\n",pParentBase);
            return CL_EO_RC(CL_ERR_UNSPECIFIED);
        }
        pCurrentBase = pActionBases[i];
    }
    else
    {
        pCurrentBase = &gMemWaterMark[(ClWaterMarkIdT)i];
    }
        
    return clWaterMarkDataInit(pParentBase,pNumInstances,pBase,pBaseOffset,op,
                               (ClPtrT)pCurrentBase);
}

static ClRcT clHeapConfigModeTagFmt(ClParserTagT *pTag,
                                    ClParserDataT *pData,
                                    ClPtrT pBase,
                                    ClCharT *pAttr,
                                    ClPtrT pValue,
                                    ClParserTagOpT op
                                    )
{
    ClHeapConfigT *pHeapConfig = pBase;
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    if(pBase == NULL || pAttr == NULL || pValue == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    rc = CL_TAG_SKIP_UPDATE ;
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            ClHeapModeT mode;
            if(!strcasecmp(pAttr,"PAM"))
            {
                mode = CL_HEAP_PREALLOCATED_MODE;
            }
            else if(!strcasecmp(pAttr,"NM"))
            {
                rc |= CL_TAG_SKIP_CHILD;
                mode = CL_HEAP_NATIVE_MODE;
            }
            else if(!strcasecmp(pAttr,"CM"))
            {
                mode = CL_HEAP_CUSTOM_MODE;
            }
            else 
            {
                clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid mode:%s\n",pAttr);
                rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
            pHeapConfig->mode = mode;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            const ClCharT *pMode = NULL;
            if(pHeapConfig->mode == CL_HEAP_NATIVE_MODE)
            {
                rc |= CL_TAG_SKIP_CHILD;
                pMode = "NM";
            }
            else if(pHeapConfig->mode == CL_HEAP_CUSTOM_MODE)
            {
                pMode = "CM";
            }
            else if(pHeapConfig->mode == CL_HEAP_PREALLOCATED_MODE)
            {
                pMode = "PAM" ;
            }
            else 
            {
                pMode = "Unknown";
            }
            strncpy(pAttr,pMode,CL_PARSER_ATTR_SIZE);
        }
        break;
    default:
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Unknown op:%d\n",op);
        goto out;
    }
    out:
    return rc;
}

static ClRcT clBufferConfigModeTagFmt(ClParserTagT *pTag,
                                      ClParserDataT *pData,
                                      ClPtrT pBase,
                                      ClCharT *pAttr,
                                      ClPtrT pValue,
                                      ClParserTagOpT op
                                      )
{
    ClBufferPoolConfigT *pBufferConfig = pBase;
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    if(pBase == NULL || pAttr == NULL || pValue == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    rc = CL_TAG_SKIP_UPDATE ;
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            ClBufferModeT mode;
            if(!strcasecmp(pAttr,"PAM"))
            {
                mode = CL_BUFFER_PREALLOCATED_MODE;
            }
            else if(!strcasecmp(pAttr,"NM"))
            {
                mode = CL_BUFFER_NATIVE_MODE;
            }
            else 
            {
                clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid mode:%s\n",pAttr);
                rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
            pBufferConfig->mode = mode;
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            const ClCharT *pMode = NULL;
            if(pBufferConfig->mode == CL_BUFFER_NATIVE_MODE)
            {
                pMode = "NM";
            }
            else if(pBufferConfig->mode == CL_BUFFER_PREALLOCATED_MODE)
            {
                pMode = "PAM" ;
            }
            else 
            {
                pMode = "Unknown";
            }
            strncpy(pAttr,pMode,CL_PARSER_ATTR_SIZE);
        }
        break;
    default:
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Unknown op:%d\n",op);
        goto out;
    }
    out:
    return rc;
}

static ClRcT clWaterMarkActionTagFmt(ClParserTagT *pTag,
                                     ClParserDataT *pData,
                                     ClPtrT pBase,
                                     ClCharT *pAttr,
                                     ClPtrT pValue,
                                     ClParserTagOpT op
                                     )
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
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
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    for(i = 0; pActionTags[i] && strcasecmp(pActionTags[i],pData->pTag); ++i);

    if(pActionTags[i] == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid tag specified:%s\n",pData->pTag);
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
                clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid attribute:%s\n",pAttr);
                rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
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
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid op:%d\n",op);
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    out:
    return rc;
}

static ClRcT clEoConfigNameTagFmt(ClParserTagT *pTag,
                                  ClParserDataT *pData,
                                  ClPtrT pBase,
                                  ClCharT *pAttr,
                                  ClPtrT value,
                                  ClParserTagOpT op,
                                  const ClCharT *pName
                                  )
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    if(pAttr == NULL || pName == NULL)
    {
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid param\n");
        goto out;
    }
    /*
     * Continue by skipping children and into the next instance
     * if name doesnt match the eoConfig template.
     */
    rc = CL_TAG_CONT;
    switch(op)
    {
    case CL_PARSER_TAG_FMT:
        {
            if(!strcasecmp(pAttr,pName))
            {
                rc = CL_TAG_SKIP_CHILD_INSTANCES | CL_TAG_SKIP_UPDATE;
                ++isConfigValid;
            }
        }
        break;
    case CL_PARSER_TAG_DISPLAY:
        {
            rc = CL_TAG_OK;
            strncpy(pAttr,pName,CL_PARSER_ATTR_SIZE);
        }
        break;
    default:
        clLogError(CL_LOG_AREA,CL_LOG_CTXT,"Invalid op:%d\n",op);
        rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    out:
    return rc;
}

static ClRcT clHeapConfigNameTagFmt(ClParserTagT *pTag,
                                    ClParserDataT *pData,
                                    ClPtrT pBase,
                                    ClCharT *pAttr,
                                    ClPtrT value,
                                    ClParserTagOpT op
                                    )
{
    return clEoConfigNameTagFmt(pTag,pData,pBase,pAttr,value,op,parseConfigs.heapConfigName);
}

static ClRcT clBufferConfigNameTagFmt(ClParserTagT *pTag,
                                      ClParserDataT *pData,
                                      ClPtrT pBase,
                                      ClCharT *pAttr,
                                      ClPtrT value,
                                      ClParserTagOpT op
                                      )
{
    return clEoConfigNameTagFmt(pTag,pData,pBase,pAttr,value,op,parseConfigs.buffConfigName);
}

static ClRcT clMemConfigNameTagFmt(ClParserTagT *pTag,
                                   ClParserDataT *pData,
                                   ClPtrT pBase,
                                   ClCharT *pAttr,
                                   ClPtrT value,
                                   ClParserTagOpT op
                                   )
{
    return clEoConfigNameTagFmt(pTag,pData,pBase,pAttr,value,op,parseConfigs.memConfigName);
}

static ClRcT clIocConfigNameTagFmt(ClParserTagT *pTag,
                                   ClParserDataT *pData,
                                   ClPtrT pBase,
                                   ClCharT *pAttr,
                                   ClPtrT value,
                                   ClParserTagOpT op
                                   )
{
    return clEoConfigNameTagFmt(pTag,pData,pBase,pAttr,value,op,parseConfigs.iocConfigName);
}

/*
 * Simpe watermark validate routine
 *
*/
static ClBoolT clWaterMarkValidate(ClParserTagT *pTag,ClPtrT pValue)
{
    ClBoolT isValid = CL_FALSE;
    ClUint64T value;

    if(pValue == NULL)
    {
        goto out;
    }
    value = *(ClUint64T*)pValue;
    if(value > 100)
    {
        goto out;
    }
    isValid = CL_TRUE;
    out:
    return isValid;
}


ClRcT clEoGetConfig(ClCharT* compCfgFile)
{
    ClRcT rc = CL_OK;

    ClCharT *filePath = NULL; 

    filePath = getenv(CL_ASP_CONFIG_PATH);
    if (NULL == filePath)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT,
            "ASP_CONFIG environment variable not set");
        goto failure;
    }

    clLogDebug(CL_LOG_AREA, CL_LOG_CTXT,
            "ASP_CONFIG env variable value is %s",filePath);

    numConfigsRead = 1;

    rc = clParseXML(filePath,CL_EO_CONFIG_FILE_NAME,&clEoConfigListParserData);
    
    if(CL_OK != rc)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT,
              "Could not read or parse XML file [%s/%s], return code [0x%x]",
              filePath, CL_EO_CONFIG_FILE_NAME, rc);

        if (compCfgFile)
        {
            ClCharT fileName[CL_MAX_NAME_LENGTH*2];
            FILE* fp;

            snprintf(fileName,CL_MAX_NAME_LENGTH*2, "%s/%s", filePath,compCfgFile);
            fp = fopen(fileName,"r");
            if (fp) /* Does the file exist */
            {
                fclose(fp);            
                rc = clParseXML(filePath,compCfgFile,&clEoConfigListParserData);
                if(CL_OK != rc)
                {
                    clLog(CL_LOG_WARNING, CL_LOG_AREA, CL_LOG_CTXT,
                          "Could not parse optional component configuration XML file [%s], return code [0x%x]",
                          fileName, rc);
                }
            }
            else
            {
                clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT,
                      "Optional component configuration XML file [%s] does not exist.",
                      fileName);
            }
        }
    }

    
    if(!CL_EO_CONFIG_VALID())
    {
        /*
         * Try using the defaults incase the EO config is missing.
         */
        clLogWarning(CL_LOG_AREA, CL_LOG_CTXT,
                      "No configuration found for EO [%s]. Using defaults",
                      CL_EO_NAME);
        strncpy(parseConfigs.heapConfigName, "Default", sizeof(parseConfigs.heapConfigName)-1);
        strncpy(parseConfigs.buffConfigName, "Default", sizeof(parseConfigs.buffConfigName)-1);
        strncpy(parseConfigs.memConfigName, "Default", sizeof(parseConfigs.memConfigName)-1);
        isConfigValid = 1; /*+1 for the EO name match*/

#if 0
        /* Multiline logging wont work at this point since the heap is not initialized */
        clLog(
            CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT,
            "Configuration file [%s/%s] failed validation, return code [0x%x]\n"
            "EO names of ASP components have been changed in this release to 3-letter acronyms\n"
            "Please check and update your clEoConfig.xml file.",
            filePath, CL_EO_CONFIG_FILE_NAME, rc);
          Old and new names:\n
        "AlarmServer   --> ALM\n"
            "CpmServer     --> AMF\n"
            "CkptServer    --> CKP\n"
            "CmServer      --> CHM\n"
            "CorServer     --> COR\n"
            "DebugServer   --> DBG\n"
            "EventServer   --> EVT\n"
            "FaultServer   --> FLT\n"
            "GmsServer     --> GMS\n"
            "LogServer     --> LOG\n"
            "NameServer    --> NAM\n"
            "SnmpServer    --> SNS\n"
            "TxnServer     --> TXN",
        goto failure;
#endif        
    }

    /* 
     * some configurations may not be present in clEoConfig.xml files 
     * due to which CL_EO_CONFIG_VALID will fail.
     * increase the numConfigsRead variable only if there is a configuration 
     * present,which will be used in CL_EO_CONFIG_VALID for validation.
     * We anyway need the essential mem. config. So adding that overhead.
     */
    numConfigsRead += CL_EO_CONFIG_ESSENTIAL;

    if (strlen(parseConfigs.iocConfigName) > 0)
    {
        ++numConfigsRead;
    }

    rc = clParseXML(filePath,CL_EO_DEFINITIONS_FILE_NAME,&clEoDefinitionsParserData);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT,
            "Could not read or parse XML file [%s/%s] in 2nd pass, error [0x%x]",
            filePath, CL_EO_CONFIG_FILE_NAME, rc);
        goto failure;
    }

    rc = CL_EO_RC(CL_ERR_NOT_EXIST);
    
    if(!CL_EO_CONFIG_VALID())
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT,
            "Configuration file [%s/%s] failed validation in 2nd pass, error [0x%x]",
            filePath, CL_EO_CONFIG_FILE_NAME, rc);
        goto failure;
    }
    
#ifdef CL_EO_DEBUG
    clParseDisplay(&clEoConfigListParserData);
    clParseDisplay(&clEoDefinitionsParserData);
#endif
    
    clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT,
        "Configuration file [%s/%s] loaded",
        filePath, CL_EO_CONFIG_FILE_NAME);

    /* Reaching here means all reading was successful. */

    rc = CL_OK;

    failure:

    return rc;
}
