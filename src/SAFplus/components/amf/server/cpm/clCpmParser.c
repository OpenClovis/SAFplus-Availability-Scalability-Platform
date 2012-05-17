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

/**
 * This file implements the XML configuration file parser for CPM.
 */

/*
 * Standard header files 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clParserApi.h>
#include <clCorUtilityApi.h>
#include <clIocApiExt.h>

/*
 * CPM internal header files 
 */
#include <clCpmInternal.h>
#include <clCpmErrors.h>
#include <clCpmConfigApi.h>
#include <clCpmLog.h>
#include <clCpmCor.h>
#include <clCpmParser.h>
#include <clCpmExtApi.h>
#include <clCpmMgmt.h>

#define CPM_PARSER_NULL_CHECK(txt, recvText, problem)                   \
    txt = recvText;                                                     \
    if(txt == NULL)                                                     \
    {                                                                   \
        ClCharT logText[CL_MAX_NAME_LENGTH];                            \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,                                  \
                ("XML seems to have syntactical or syntax error.\n"));  \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,                                  \
                ("%s. Exitting ............... \n", problem));          \
        sprintf(logText, "%s%s Exitting ............... \n",            \
                "XML seems to have syntactical or syntax error.\n", problem); \
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, logText);     \
        exit(1);                                                        \
    }

#define CPM_PARSER_HANDLE_NULL_CHECK(txt, recvText, problem) txt = recvText

typedef struct
{
    ClUint32T level;
    ClCharT *compName;
} ClCpmAspCompMappingT;

typedef struct
{
    const ClCharT *su;
    ClCpmAspCompMappingT *suCompMap;
    ClBoolT suLoaded;
} ClCpmAspSUMappingT;

static ClCpmAspCompMappingT logSUMap[] = { 
    {1, "log" },
    {1,  NULL},
};

static ClCpmAspCompMappingT gmsSUMap[] = {
    {2, "gms"},
    {2, NULL},
};

static ClCpmAspCompMappingT eventSUMap[] = {
    {3, "event"},
    {3, NULL},
};

static ClCpmAspCompMappingT txnSUMap[] = {
    {3, "txn"},
    {3, NULL},
};

static ClCpmAspCompMappingT nameSUMap[] = {
    {4, "name"},
    {4, NULL},
};

static ClCpmAspCompMappingT corSUMap[] = {
    {4, "cor"},
    {4, NULL},
};

static ClCpmAspCompMappingT ckptSUMap[] = {
    {4, "ckpt"},
    {4, NULL},
};

static ClCpmAspCompMappingT oampSUMap[] = {
    {5, "fault"},
    {5, "alarm"},
    {5,  NULL},
};

#define ASP_NODE_SU_COMP_MAP1                   \
    {                                           \
        .su = "logSU",                          \
        .suCompMap = logSUMap,                  \
    },                                          \
    {                                           \
        .su = "gmsSU",                          \
        .suCompMap = gmsSUMap,                  \
    },                                          \
    {                                           \
        .su = "eventSU",                        \
        .suCompMap = eventSUMap,                \
    },                                          \
    {                                           \
        .su = "txnSU",                          \
        .suCompMap = txnSUMap,                  \
    },                                          \
    {                                           \
        .su = "nameSU",                         \
        .suCompMap = nameSUMap,                 \
    }

#define ASP_NODE_SU_COMP_MAP2                   \
    {                                           \
        .su = "ckptSU",                         \
        .suCompMap = ckptSUMap,                 \
    },                                          \
    {                                           \
        .su = "oampSU",                         \
        .suCompMap = oampSUMap,                 \
    }

static ClCpmAspSUMappingT scSUCompToLevelMapping[] = 
{
    ASP_NODE_SU_COMP_MAP1,
    {
        .su = "corSU",
        .suCompMap = corSUMap,
    },
    ASP_NODE_SU_COMP_MAP2,
};

#define CL_CPM_SC_ASP_SUS ((ClUint32T)sizeof(scSUCompToLevelMapping)/sizeof(scSUCompToLevelMapping[0]))

static ClCharT **cpmValgrindFilterList;
static ClCpmAspSUMappingT wbSUCompToLevelMapping[] =
{
    ASP_NODE_SU_COMP_MAP1,
    ASP_NODE_SU_COMP_MAP2,
};

#define CL_CPM_WB_ASP_SUS ((ClUint32T)sizeof(wbSUCompToLevelMapping)/sizeof(wbSUCompToLevelMapping[0]))

/*
 * linked list containing component information 
 */
ClCpmCompInfoT *cpmCompList = NULL;

#if CPM_DEBUG
static void displayCompList(ClCpmCompInfoT *compList)
{
    ClCpmCompInfoT *p = NULL;

    ClUint32T argIndex = 0;
    ClUint32T envIndex = 0;

    clOsalPrintf("**********Contents of the component linked list*********\n");

    for (p = compList; p != NULL; p = p->pNext)
    {
        clOsalPrintf("Component type : %s\n", p->compType);
        clOsalPrintf("Component property : %d\n", p->compConfig.compProperty);
        clOsalPrintf("Component processRel : %d\n",
                     p->compConfig.compProcessRel);
        clOsalPrintf("Component instCmd : %s\n",
                     p->compConfig.instantiationCMD);

        argIndex = 0;
        clOsalPrintf("Args :");
        while (p->compConfig.argv[argIndex] != NULL)
        {
            clOsalPrintf(" %s", p->compConfig.argv[argIndex]);
            argIndex++;
        }
        clOsalPrintf("\n");

        envIndex = 0;
        clOsalPrintf("Environments :\n");
        while ((strcmp(p->compConfig.env[envIndex].envName, NULL_STRING) != 0)
               && (strcmp(p->compConfig.env[envIndex].envValue, NULL_STRING) !=
                   0))
        {
            clOsalPrintf("Environment name : %s\n",
                         p->compConfig.env[envIndex].envName);
            clOsalPrintf("Environment value : %s\n",
                         p->compConfig.env[envIndex].envValue);
            envIndex++;
        }
        clOsalPrintf("\n");

        clOsalPrintf("Component termCmd : %s\n", p->compConfig.terminationCMD);
        clOsalPrintf("Component cleanupCmd : %s\n", p->compConfig.cleanupCMD);
        clOsalPrintf("Component compInstantiateTimeout : %d\n",
                     p->compConfig.compInstantiateTimeout);
        clOsalPrintf("Component compTerminateTimeout : %d\n",
                     p->compConfig.compTerminateTimeout);
        clOsalPrintf("Component compTerminateTimeout : %d\n",
                     p->compConfig.compCleanupTimeout);

        clOsalPrintf("\n");
    }
}
#endif

/*
 * This function is used to eval environment param
 * Format $(ASP_DIR)/var/run -> /root/asp/var/run
 *
 */
static void evalEnv(ClCharT *arg)
{
    ClUint32T tk = 0;
    ClUint32T j = 0;
    ClUint32T k = 0;
    ClUint32T i = 0;
    ClCharT buf[CPM_MAX_ARGS] = { 0 };
    ClCharT env[CPM_MAX_ARGS] = { 0 };

    while (i < strlen(arg))
    {
        if (arg[i] == '$' && arg[i + 1] == '(')
        {
            tk = 1;
            i++;
        } else if (arg[i] == ')') {
            ClCharT *envValue = getenv(env);
            ClUint32T len = strlen(env);
            if (envValue != NULL)
            {
                len = strlen(envValue);
                strncat(buf, envValue, len);
            } else {
                strncat(buf, "$(", 2);
                strncat(buf, env, len);
                strncat(buf, ")", 1);
            }
            k = strlen(buf);
            j = 0;
            env[j] = 0;
            tk = 0;
        } else if (tk == 1)
        {
            env[j++] = arg[i];
        } else {
            buf[k++] = arg[i];
        }
        i++;
    }
    strcpy(arg, buf);
}

static void cpmValgrindFilterInitialize(void)
{
    ClCharT *filterList = NULL;
    ClInt32T n = 0;
    ClCharT *token = NULL;
    ClCharT *nextToken = NULL;

    if(cpmValgrindFilterList) return ;
    cpmValgrindFilterList = clHeapCalloc(4, sizeof(*cpmValgrindFilterList));
    CL_ASSERT(cpmValgrindFilterList != NULL);

    for(; n < 4; ++n) cpmValgrindFilterList[n] = NULL;
    n = 0;

    filterList = getenv("ASP_VALGRIND_FILTER");
    if(!filterList) return;

    token = strtok_r(filterList, " ", &nextToken);
    while(token)
    {
        ClCharT *comp = clHeapCalloc(1, strlen(token)+1);
        CL_ASSERT(comp != NULL);
        strcpy(comp, token);
        if(n && !(n & 3))
        {
            cpmValgrindFilterList = clHeapRealloc(cpmValgrindFilterList, sizeof(*cpmValgrindFilterList) * (n + 4));
            CL_ASSERT(cpmValgrindFilterList != NULL);
        }
        cpmValgrindFilterList[n++] = comp;
        token = strtok_r(NULL, " ", &nextToken);
    }

    if(n && !(n & 3))
    {
        cpmValgrindFilterList = clHeapRealloc(cpmValgrindFilterList, sizeof(*cpmValgrindFilterList) * (n+1));
        CL_ASSERT(cpmValgrindFilterList != NULL);
    }
    cpmValgrindFilterList[n] = NULL;
}

ClBoolT cpmIsValgrindBuild(ClCharT *instantiationCMD)
{
    ClCharT *valgrindCmdStr = clParseEnvStr("ASP_VALGRIND_CMD");
    ClCharT *binary = NULL;
    ClInt32T i;

    if(!valgrindCmdStr) return CL_NO;
    if(!instantiationCMD) return CL_YES;
    if(!cpmValgrindFilterList) cpmValgrindFilterInitialize();
    if(!cpmValgrindFilterList[0]) 
        return CL_YES; /*all components hooking*/

    /*
     * Check for matching filter.
     */
    if( (binary = strrchr(instantiationCMD, '/')) )
    {
        ++binary;
    }
    else
    {
        binary = instantiationCMD;
    }

    for(i = 0; cpmValgrindFilterList[i]; ++i)
    {
        if(!strncmp(cpmValgrindFilterList[i], binary, strlen(binary)))
            return CL_YES;
    }

    return CL_NO;
}

void cpmModifyCompArgs(ClCpmCompConfigT *newConfig, ClUint32T *pArgIndex)
{
    ClCharT valgrindCmd[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *valgrindCmdStr = clParseEnvStr("ASP_VALGRIND_CMD");
    ClCharT *delim = " ";
    ClCharT *valCmd = NULL;
    ClUint32T argIndex = *pArgIndex;
    ClCharT *aspLogDir = getenv("ASP_LOGDIR");
    ClCharT logFileCmd[CL_MAX_NAME_LENGTH] = {0};

    CL_ASSERT(valgrindCmdStr != NULL);
    
    strncpy(valgrindCmd, valgrindCmdStr, CL_MAX_NAME_LENGTH-1);
    
    snprintf(logFileCmd, CL_MAX_NAME_LENGTH-1, 
             " --log-file=%s/%s.%lld", 
             aspLogDir,
             newConfig->instantiationCMD,
             clOsalStopWatchTimeGet());
    strcat(valgrindCmd, logFileCmd);
    valCmd = strtok(valgrindCmd, delim);
    while (NULL != valCmd && (argIndex < CPM_MAX_ARGS - 1))
    {
        newConfig->argv[argIndex] = clHeapAllocate(strlen(valCmd) + 1);
        if (!newConfig->argv[argIndex])
        {
            clLogError(CPM_LOG_AREA_CPM, CL_LOG_AREA_UNSPECIFIED,
                       "Unable to allocate memory");
            goto failure;
        }
        strcpy(newConfig->argv[argIndex], valCmd);
        argIndex++;

        valCmd = strtok(NULL, delim);
    }

    *pArgIndex = argIndex;

    return;

failure:
    return;
}


/*
 * Parses the information about each component, populating them
 * in the component linked list.
 */
static ClRcT cpmParseCompInfo(ClParserPtrT file, ClBoolT isAspComp)
{
    /*
     * For singly linked list. 
     */
    ClCpmCompInfoT *prev = NULL;
    ClCpmCompInfoT *newType = NULL;
    ClCpmCompInfoT *p = NULL;

    /*
     * Tag variables. 
     */
    ClParserPtrT compTypes = NULL;
    ClParserPtrT compType = NULL;
    ClParserPtrT args = NULL;
    ClParserPtrT argument = NULL;
    ClParserPtrT envs = NULL;
    ClParserPtrT nameValue = NULL;
    ClParserPtrT timeouts = NULL;
    ClParserPtrT healthCheck = NULL;

    /*
     * Attribute variables. 
     */
    const ClCharT *type = NULL;
    const ClCharT *name = NULL;
    const ClCharT *value = NULL;

    ClUint32T argIndex = 0;
    ClUint32T envIndex = 0;
    ClUint32T timeoutvalue = 0;

    /*
     * Temporary variable for holding XML tags. 
     */
    ClParserPtrT temp = NULL;

    /*
     * Temporary variable for holding text inside XML tags. 
     */
    ClCharT *str = NULL;

    ClRcT rc = CL_OK;

    CPM_PARSER_NULL_CHECK(compTypes,
                          clParserChild(file, CL_CPM_PARSER_TAG_COMP_TYPES),
                          "compTypes tag does not exist in XML file");

    CPM_PARSER_HANDLE_NULL_CHECK(compType,
                                 clParserChild(compTypes,
                                               CL_CPM_PARSER_TAG_COMP_TYPE),
                                 "compType tag does not exist");

    prev = newType = cpmCompList;
    /*
     * Ensure that each call to this function always adds
     * component information to the end of the component list.
     */
    if (prev != NULL)
    {
        for (p = cpmCompList; p != NULL; p = p->pNext)
            prev = p;
        newType = NULL;
    }

    while (compType != NULL)
    {
        /*
         * Allocate memory for each node of the linked list 
         * and fill in the component information.
         */
        newType = (ClCpmCompInfoT *) clHeapAllocate(sizeof(ClCpmCompInfoT));
        if (newType == NULL)
            CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                           CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);
        memset(newType,0,sizeof(ClCpmCompInfoT));
        

        CPM_PARSER_NULL_CHECK(type,
                              clParserAttr(compType,
                                           CL_CPM_PARSER_ATTR_COMP_TYPE_NAME),
                              "name in compType doesn't exist");
        strcpy(newType->compType, type);

        newType->compConfig.isAspComp = isAspComp;
        
        CPM_PARSER_NULL_CHECK(temp,
                              clParserChild(compType,
                                            CL_CPM_PARSER_TAG_COMP_TYPE_PROPERTY),
                              "property in compType doesn't exist");
        str = temp->txt;
        if (str)
        {
            if (strcmp(str, "CL_AMS_SA_AWARE") == 0)
                newType->compConfig.compProperty =
                    CL_AMS_COMP_PROPERTY_SA_AWARE;
            else if (strcmp(str, "CL_AMS_PROXIED_PREINSTANTIABLE") == 0)
                newType->compConfig.compProperty =
                    CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE;
            else if (strcmp(str, "CL_AMS_PROXIED_NON_PREINSTANTIABLE") == 0)
                newType->compConfig.compProperty =
                    CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE;
            else if (strcmp(str, "CL_AMS_NON_PROXIED_NON_PREINSTANTIABLE") == 0)
                newType->compConfig.compProperty =
                    CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE;
            else
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                               CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR, "property",
                               rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
        else
        {
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                           "property", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }

        CPM_PARSER_NULL_CHECK(temp,
                              clParserChild(compType,
                                            CL_CPM_PARSER_TAG_COMP_TYPE_PROCREL),
                              "processRel field in component doesn't exist");
        str = temp->txt;
        if (str)
        {
            if (strcmp(str, "CL_CPM_COMP_NONE") == 0)
                newType->compConfig.compProcessRel = CL_CPM_COMP_NONE;
            else if (strcmp(str, "CL_CPM_COMP_MULTI_PROCESS") == 0)
                newType->compConfig.compProcessRel = CL_CPM_COMP_MULTI_PROCESS;
            else if (strcmp(str, "CL_CPM_COMP_SINGLE_PROCESS") == 0)
                newType->compConfig.compProcessRel = CL_CPM_COMP_SINGLE_PROCESS;
            else if (strcmp(str, "CL_CPM_COMP_THREADED") == 0)
                newType->compConfig.compProcessRel = CL_CPM_COMP_THREADED;
            else
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                               CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                               "processRel", rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            }
        }
        else
        {
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                           "processRel", rc, rc, CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);
        }

        CPM_PARSER_NULL_CHECK(temp,
                              clParserChild(compType,
                                            CL_CPM_PARSER_TAG_COMP_TYPE_INST_CMD),
                              "instantiateCommand doesn't exist in compType ");
        if (newType->compConfig.compProperty !=
            CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE ||
            newType->compConfig.compProperty !=
            CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
        {
            str = temp->txt;
            if (str)
            {
                strcpy(newType->compConfig.instantiationCMD, str);
            }
            else
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                               CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                               "instantiateCommand", rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            }
        }

        argIndex = 0;

        if (cpmIsValgrindBuild(newType->compConfig.instantiationCMD))
        {
            cpmModifyCompArgs(&newType->compConfig, &argIndex);
        }

        /*
         * By default, put the image name as the first argument. 
         */
        {
            ClCharT *aspBinPath = getenv("ASP_BINDIR");


            if (cpmIsValgrindBuild(newType->compConfig.instantiationCMD))
            {
                newType->compConfig.argv[argIndex] =
                (ClCharT *)
                clHeapAllocate(strlen(aspBinPath) + strlen("/") +
                               strlen(newType->compConfig.instantiationCMD) + 1);
            }
            else
            {
                ClUint32T cmdLen = strlen(newType->compConfig.instantiationCMD) + 1;
                
                newType->compConfig.argv[argIndex] =
                (ClCharT *)
                clHeapAllocate(cmdLen);
            }

            if (newType->compConfig.argv[argIndex] == NULL)
                CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                               CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                               CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            if (cpmIsValgrindBuild(newType->compConfig.instantiationCMD))
            {
                strcpy(newType->compConfig.argv[argIndex],
                       aspBinPath);
                strcat(newType->compConfig.argv[argIndex], "/");
                strcat(newType->compConfig.argv[argIndex],
                       newType->compConfig.instantiationCMD);
            }
            else
            {
                strcpy(newType->compConfig.argv[argIndex],
                       newType->compConfig.instantiationCMD);
            }

            argIndex++;
        }

        /*
         * The args is mandatory. 
         */
        CPM_PARSER_HANDLE_NULL_CHECK(args,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_ARGS),
                                     "args doesn't exist in compType");
        if (args != NULL)
        {
            /*
             * However, argument inside args is not. 
             */
            argument = clParserChild(args, CL_CPM_PARSER_TAG_COMP_TYPE_ARG);
            while (argument != NULL && (argIndex < CPM_MAX_ARGS - 1))
            {
                CPM_PARSER_NULL_CHECK(value,
                                      clParserAttr(argument,
                                                   CL_CPM_PARSER_ATTR_COMP_TYPE_ARG_VALUE),
                                      "value field in argument doesn't exist");

                ClCharT evalvalue[CPM_MAX_ARGS] = { 0 };
                strncat(evalvalue, value, CPM_MAX_ARGS - 1);

                /*
                 * Evaluate env param
                 */
                evalEnv(evalvalue);

                newType->compConfig.argv[argIndex] =
                    (ClCharT *) clHeapAllocate(strlen(evalvalue) + 1);
                if (newType->compConfig.argv[argIndex] == NULL)
                    CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                                   CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);
                strcpy(newType->compConfig.argv[argIndex], evalvalue);
                argIndex++;
                argument = argument->next;
            }
        }
        newType->compConfig.argv[argIndex] = NULL;

        envIndex = 0;
        /*
         * The envs is mandatory. 
         */
        CPM_PARSER_HANDLE_NULL_CHECK(envs,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_ENVS),
                                     "envs doesn't exist in compType");
        if (envs != NULL)
        {
            /*
             * However, nameValue inside envs is not. 
             */
            nameValue =
                clParserChild(envs, CL_CPM_PARSER_TAG_COMP_TYPE_NAME_VALUE);
            while (nameValue != NULL)
            {
                CPM_PARSER_NULL_CHECK(name,
                                      clParserAttr(nameValue,
                                                   CL_CPM_PARSER_ATTR_COMP_TYPE_NAMVAL_NAME),
                                      "name field in nameValue doesn't exist");
                newType->compConfig.env[envIndex] =
                clHeapAllocate(sizeof(ClCpmEnvVarT));
                
                if (newType->compConfig.env[envIndex] == NULL)
                    CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                                   CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);

                strncpy(newType->compConfig.env[envIndex]->envName,
                        name,
                        strlen(name));

                CPM_PARSER_NULL_CHECK(value,
                                      clParserAttr(nameValue,
                                                   CL_CPM_PARSER_ATTR_COMP_TYPE_NAMVAL_VAL),
                                      "value field in nameValue doesn't exist");
                strncpy(newType->compConfig.env[envIndex]->envValue,
                        value,
                        strlen(value));
                
                envIndex++;
                nameValue = nameValue->next;
            }
        }
        newType->compConfig.env[envIndex] = NULL;

        CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_TERM_CMD),
                                     "terminateCommand doesn't exist in compType");
        if (temp != NULL)
        {
            str = temp->txt;
            if (str)
                strcpy(newType->compConfig.terminationCMD, str);
            else
                strcpy(newType->compConfig.terminationCMD, NULL_STRING);
        }
        else
            strcpy(newType->compConfig.terminationCMD, NULL_STRING);

        CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_CLEANUP_CMD),
                                     "cleanupCommand doesn't exist in compType");
        if (temp != NULL)
        {
            str = temp->txt;
            if (str)
                strcpy(newType->compConfig.cleanupCMD, str);
            else
                strcpy(newType->compConfig.cleanupCMD, NULL_STRING);
        }
        else
            strcpy(newType->compConfig.cleanupCMD, NULL_STRING);

        CPM_PARSER_HANDLE_NULL_CHECK(timeouts,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_TIME_OUTS),
                                     "timeouts field in compType doesn't exist");
        if (timeouts != NULL)
        {
            CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                         clParserChild(timeouts,
                                                       CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_INST),
                                         "instantiateTimeout field in timouts doesn't exist");
            if (temp != NULL)
            {
                str = temp->txt;
                if (str)
                {
                    timeoutvalue = atoi(str);
                    if (timeoutvalue == 0)
                    {
                        newType->compConfig.compInstantiateTimeout =
                            CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                    }
                    else
                        newType->compConfig.compInstantiateTimeout =
                            timeoutvalue;
                }
                else
                    newType->compConfig.compInstantiateTimeout =
                        CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            }
            else
                newType->compConfig.compInstantiateTimeout =
                    CL_CPM_COMPONENT_DEFAULT_TIMEOUT;

            CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                         clParserChild(timeouts,
                                                       CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_TERM),
                                         "terminateTimeout field in timouts doesn't exist");
            if (temp != NULL)
            {
                str = temp->txt;
                if (str)
                {
                    timeoutvalue = atoi(str);
                    if (timeoutvalue == 0)
                    {
                        newType->compConfig.compTerminateCallbackTimeOut =
                            CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                    }
                    else
                        newType->compConfig.compTerminateCallbackTimeOut =
                            timeoutvalue;
                }
                else
                {
                    newType->compConfig.compTerminateCallbackTimeOut =
                        CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                }
            }
            else
                newType->compConfig.compTerminateCallbackTimeOut =
                    CL_CPM_COMPONENT_DEFAULT_TIMEOUT;

            CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                         clParserChild(timeouts,
                                                       CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_CLEANUP),
                                         "cleanupTimeout field in timouts doesn't exist");
            if (temp != NULL)
            {
                str = temp->txt;
                if (str)
                {
                    timeoutvalue = atoi(str);
                    if (timeoutvalue == 0)
                    {
                        newType->compConfig.compCleanupTimeout =
                            CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                    }
                    else
                        newType->compConfig.compCleanupTimeout = timeoutvalue;
                }
                else
                {
                    newType->compConfig.compCleanupTimeout =
                        CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                }
            }
            else
            {
                newType->compConfig.compCleanupTimeout =
                    CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            }

            CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                         clParserChild(timeouts,
                                                       CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_PROXY_INST),
                                         "proxiedCompInstantiateTimeout field in timouts doesn't exist");
            if (temp != NULL)
            {
                str = temp->txt;
                if (str)
                {
                    timeoutvalue = atoi(str);
                    if (timeoutvalue == 0)
                    {
                        newType->compConfig.
                            compProxiedCompInstantiateCallbackTimeout =
                            CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                    }
                    else
                        newType->compConfig.
                            compProxiedCompInstantiateCallbackTimeout =
                            timeoutvalue;
                }
                else
                {
                    newType->compConfig.
                        compProxiedCompInstantiateCallbackTimeout =
                        CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                }
            }
            else
            {
                newType->compConfig.compProxiedCompInstantiateCallbackTimeout =
                    CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            }

            CPM_PARSER_HANDLE_NULL_CHECK(temp,
                                         clParserChild(timeouts,
                                                       CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_PROXY_CLN),
                                         "proxiedCompCleanupTimeout field in timouts doesn't exist");
            if (temp != NULL)
            {
                str = temp->txt;
                if (str)
                {
                    timeoutvalue = atoi(str);
                    if (timeoutvalue == 0)
                    {
                        newType->compConfig.
                            compProxiedCompCleanupCallbackTimeout =
                            CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
                    }
                    else
                        newType->compConfig.
                            compProxiedCompCleanupCallbackTimeout =
                            timeoutvalue;
                }
                else
                    newType->compConfig.compProxiedCompCleanupCallbackTimeout =
                        CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            }
            else
                newType->compConfig.compProxiedCompCleanupCallbackTimeout =
                    CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
        }
        else
        {
            newType->compConfig.compInstantiateTimeout =
                CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            newType->compConfig.compTerminateCallbackTimeOut =
                CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            newType->compConfig.compCleanupTimeout =
                CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            newType->compConfig.compProxiedCompInstantiateCallbackTimeout =
                CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
            newType->compConfig.compProxiedCompCleanupCallbackTimeout =
                CL_CPM_COMPONENT_DEFAULT_TIMEOUT;
        }

        CPM_PARSER_HANDLE_NULL_CHECK(healthCheck,
                                     clParserChild(compType,
                                                   CL_CPM_PARSER_TAG_COMP_TYPE_HEALTHCHECK),
                                     "healthCheck tag in compType doesn't exist");
        if (healthCheck)
        {
            CPM_PARSER_NULL_CHECK(temp,
                                  clParserChild(healthCheck,
                                                CL_CPM_PARSER_TAG_COMP_TYPE_HC_PERIOD),
                                  "period tag in healthCheck does not exist");
            str = temp->txt;
            if (str)
            {
                ClUint32T t = atoi(str);

                t = CL_MAX(t, CL_CPM_COMPONENT_MIN_HC_TIMEOUT);

                if (t) newType->compConfig.healthCheckConfig.period = t;
                else newType->compConfig.healthCheckConfig.period = CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT;
            }

            CPM_PARSER_NULL_CHECK(temp,
                                  clParserChild(healthCheck,
                                                CL_CPM_PARSER_TAG_COMP_TYPE_HC_MAX_DURAION),
                                  "period tag in healthCheck does not exist");
            str = temp->txt;
            if (str)
            {
                ClUint32T t = atoi(str);
                
                t = CL_MAX(t, 2 * CL_CPM_COMPONENT_MIN_HC_TIMEOUT);

                if (t) newType->compConfig.healthCheckConfig.maxDuration = t;
                else newType->compConfig.healthCheckConfig.maxDuration = 2 * CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT;
            }

            if (newType->compConfig.healthCheckConfig.maxDuration <
                2 * newType->compConfig.healthCheckConfig.period)
            {
                newType->compConfig.healthCheckConfig.maxDuration = 2 * newType->compConfig.healthCheckConfig.period;
                clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                             "Healthcheck max duration is less than "
                             "twice the period for component type [%s]. "
                             "Setting max duration to twice the period. "
                             "Period now is [%d], max duration is [%d]",
                             newType->compType,
                             newType->compConfig.healthCheckConfig.period,
                             newType->compConfig.healthCheckConfig.maxDuration);
            }
        }
        else
        {
            newType->compConfig.healthCheckConfig.period = CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT;
            newType->compConfig.healthCheckConfig.maxDuration = 2 * CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT;
        }

        if (prev == NULL)
        {
            cpmCompList = newType;
            prev = newType;
        }
        else
        {
            prev->pNext = newType;
            prev = newType;
        }
        compType = compType->next;
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Container related functions.
 */
int cpmNodeStoreCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

unsigned int cpmNodeStoreHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT)key % CL_CPM_CPML_TABLE_BUCKET_SIZE);
}

int cpmCompStoreCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

unsigned int cpmCompStoreHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT)key % CL_CPM_COMPTABLE_BUCKET_SIZE);
}

void cpmNodeDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    if(((ClCpmLT *)userData)->pCpmLocalInfo != NULL)
        clHeapFree(((ClCpmLT *) userData)->pCpmLocalInfo);
    clHeapFree((ClCpmLT *) userData);
}

void cpmCompDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    ClUint32T i = 0;

    if(userData == 0 || ((ClCpmComponentT *) userData)->compConfig == NULL)
        return;

    for (i = 0; ((ClCpmComponentT *) userData)->compConfig->argv[i]; ++i)
        clHeapFree(((ClCpmComponentT *) userData)->compConfig->argv[i]);

    for (i = 0; ((ClCpmComponentT *) userData)->compConfig->env[i]; ++i)
        clHeapFree(((ClCpmComponentT *) userData)->compConfig->env[i]);

    if (((ClCpmComponentT *) userData)->compConfig)
        clHeapFree(((ClCpmComponentT *) userData)->compConfig);

    clOsalMutexDelete(((ClCpmComponentT *) userData)->compMutex);

    /*
     * Delete all the timers 
     */
    if (((ClCpmComponentT *) userData)->cpmInstantiateTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->
                      cpmInstantiateTimerHandle);

    if (((ClCpmComponentT *) userData)->cpmTerminateTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->cpmTerminateTimerHandle);

    if (((ClCpmComponentT *) userData)->cpmHealthcheckTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->cpmHealthcheckTimerHandle);

    if (((ClCpmComponentT *) userData)->hbTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->hbTimerHandle);

    if (((ClCpmComponentT *) userData)->cpmProxiedInstTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->
                      cpmProxiedInstTimerHandle);

    if (((ClCpmComponentT *) userData)->cpmProxiedCleanupTimerHandle != 0)
        clTimerDelete(&((ClCpmComponentT *) userData)->
                      cpmProxiedCleanupTimerHandle);

    if (((ClCpmComponentT *) userData)->compMoId)
        clCorMoIdFree(((ClCpmComponentT *) userData)->compMoId);
    clHeapFree((ClCpmComponentT *) userData);
}

/*
 * Creates the component and service unit hash tables
 * and associated mutexes.
 */
ClRcT cpmTableInitialize(void)
{
    ClRcT rc = CL_OK;

    /*
     * Create and Initialize the component Hash Table and Mutex 
     */
    rc = clCntHashtblCreate(CL_CPM_COMPTABLE_BUCKET_SIZE, cpmCompStoreCompare,
                            cpmCompStoreHashFunc, cpmCompDelete, cpmCompDelete,
                            CL_CNT_NON_UNIQUE_KEY, &gpClCpm->compTable);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    gpClCpm->noOfComponent = 0;
    rc = clOsalMutexCreate(&(gpClCpm->compTableMutex));
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    return CL_OK;

  failure:
    return rc;
}

/*
 * Deallocates the mutexes associated with component 
 * and service unit hash tables.
 */
void cpmCompConfigFinalize(void)
{
    if (gpClCpm->compTableMutex)
        clOsalMutexDelete(gpClCpm->compTableMutex);
    if (gpClCpm->compTable)
        clCntDelete(gpClCpm->compTable);

    if (gpClCpm->cpmTableMutex)
        clOsalMutexDelete(gpClCpm->cpmTableMutex);
    if (gpClCpm->cpmTable)
        clCntDelete(gpClCpm->cpmTable);
}

/*
 * Parses the information about each node and 
 * fills it into CPM data structure.
 */
ClRcT cpmParseNodeInfo(ClParserPtrT file)
{
    /*
     * Tag variables. 
     */
    ClParserPtrT nodeInstances = NULL;
    ClParserPtrT nodeInstance = NULL;
    ClParserPtrT cpmConfigs = NULL;
    ClParserPtrT cpmConfig = NULL;

    /*
     * Attribute variables. 
     */
    const ClCharT *nodeName = NULL;
    const ClCharT *cpmNodeType = NULL;
    const ClCharT *nodeType = NULL;
    const ClCharT *nodeMoId = NULL;
    const ClCharT *cpmType = NULL;

    ClCpmLT *cpmL = NULL;
    ClUint16T nodeKey = 0;
    ClRcT rc = CL_OK;

    strcpy((ClCharT *) gpClCpm->pCpmConfig->nodeName, clCpmNodeName);

    /*
     * Find the node type of the CPM from the given node name. 
     */
    CPM_PARSER_NULL_CHECK(nodeInstances,
                          clParserChild(file, CL_CPM_PARSER_TAG_NODE_INSTS),
                          "nodeInstances tag does not exist");
    CPM_PARSER_NULL_CHECK(nodeInstance,
                          clParserChild(nodeInstances,
                                        CL_CPM_PARSER_TAG_NODE_INST),
                          "nodeInstance tag does not exist");
    while (nodeInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeName,
                              clParserAttr(nodeInstance,
                                           CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                              "field name doesn't exist in nodeInstance");
        if (strcmp(nodeName, clCpmNodeName) == 0)
        {
            /*
             * found required nodeType 
             */
            CPM_PARSER_NULL_CHECK(cpmNodeType,
                                  clParserAttr(nodeInstance,
                                               CL_CPM_PARSER_ATTR_NODE_INST_TYPE),
                                  "type doesn't exist in nodeInstance");

            /* For CPM-CM interaction */
            strcpy(gpClCpm->pCpmLocalInfo->nodeType.value, cpmNodeType);
            gpClCpm->pCpmLocalInfo->nodeType.length = 
                strlen(gpClCpm->pCpmLocalInfo->nodeType.value);
            clNameSet(&gpClCpm->pCpmLocalInfo->nodeIdentifier,
                      clCpmNodeName);
            /* For creation of COR object. */
            CPM_PARSER_NULL_CHECK(nodeMoId,
                                  clParserAttr(nodeInstance,
                                               CL_CPM_PARSER_ATTR_NODE_INST_NODE_MOID),
                                  "MoId doesn't exist in nodeInstance");
            strcpy(gpClCpm->pCpmLocalInfo->nodeMoIdStr.value, nodeMoId);
            gpClCpm->pCpmLocalInfo->nodeMoIdStr.length = 
                            strlen(gpClCpm->pCpmLocalInfo->nodeMoIdStr.value);

            break;
        }
        nodeInstance = nodeInstance->next;
    }
    if (nodeInstance == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Node Instance with the given node type not found"));
        rc = clCntHashtblCreate(CL_CPM_CPML_TABLE_BUCKET_SIZE,
                                cpmNodeStoreCompare, cpmNodeStoreHashFunc, cpmNodeDelete,
                                cpmNodeDelete, CL_CNT_NON_UNIQUE_KEY,
                                &gpClCpm->cpmTable);
        CL_ASSERT(rc == CL_OK);
        return CL_CPM_RC(CL_ERR_NOT_EXIST);
    }
    /*
     * Find the CPM configuration from the node type found above. 
     */
    CPM_PARSER_NULL_CHECK(cpmConfigs,
                          clParserChild(file,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CONFIGS),
                          "cpmConfigs doesn't exist in XML file");
    CPM_PARSER_NULL_CHECK(cpmConfig,
                          clParserChild(cpmConfigs,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CONFIG),
                          "cpmConfig doesn't exist in cpmConfigs");
    while (cpmConfig != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeType,
                              clParserAttr(cpmConfig,
                                           CL_CPM_PARSER_ATTR_NODE_TYPE),
                              "nodeType doesn't exist in cpmConfig");
        if (strcmp(nodeType, cpmNodeType) == 0)
        {
            /*
             * Found the required CPM configuration 
             */
            break;
        }
        cpmConfig = cpmConfig->next;
    }
    if (cpmConfig == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("cpmConfig for the node type not found. exiting .....\n"));
        exit(1);
    }

    /*
     * Get the CPM type 
     */
    CPM_PARSER_NULL_CHECK(cpmType,
                          clParserAttr(cpmConfig,
                                       CL_CPM_PARSER_ATTR_NODE_CPM_TYPE),
                          "cpmType doesn't exist in cpmConfig");
    if (strcmp(cpmType, "GLOBAL") == 0)
    {
        gpClCpm->pCpmConfig->cpmType = CL_CPM_GLOBAL;

        parseNodeList:

        rc = clCntHashtblCreate(CL_CPM_CPML_TABLE_BUCKET_SIZE,
                                cpmNodeStoreCompare, cpmNodeStoreHashFunc, cpmNodeDelete,
                                cpmNodeDelete, CL_CNT_NON_UNIQUE_KEY,
                                &gpClCpm->cpmTable);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        CPM_PARSER_NULL_CHECK(nodeInstances,
                              clParserChild(file, CL_CPM_PARSER_TAG_NODE_INSTS),
                              "nodeInstances doesn't exist in XML file");
        CPM_PARSER_NULL_CHECK(nodeInstance,
                              clParserChild(nodeInstances,
                                            CL_CPM_PARSER_TAG_NODE_INST),
                              "nodeInstance doesn't exist in nodeInstances");
        while (nodeInstance != NULL)
        {
            const ClCharT *chassisID = NULL;
            const ClCharT *slotID = NULL;

            CPM_PARSER_NULL_CHECK(nodeName,
                                  clParserAttr(nodeInstance,
                                               CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                                  "name doesn't exist in nodeInstance");
            /*
             * Add all nodes except itself 
             */
            /*if (strcmp(nodeName, clCpmNodeName) != 0)*/
            {
                cpmL = (ClCpmLT *) clHeapCalloc(1, sizeof(ClCpmLT));
                if (cpmL == NULL)
                {
                    CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                                   CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);
                }

                strcpy(cpmL->nodeName, nodeName);
                rc = clCksm16bitCompute((ClUint8T *) cpmL->nodeName,
                                        strlen(cpmL->nodeName), &nodeKey);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_CKSM_ERR,
                               cpmL->nodeName, rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);

                cpmL->pCpmLocalInfo = NULL;
                cpmL->noOfComponent = 0;

                nodeType = clParserAttr(nodeInstance, CL_CPM_PARSER_ATTR_NODE_INST_TYPE);
                if(nodeType)
                {
                    clNameSet(&cpmL->nodeType, nodeType);
                }

                chassisID = clParserAttr(nodeInstance, CL_CPM_PARSER_ATTR_NODE_INST_CHASSIS_ID);
                if(chassisID)
                {
                    cpmL->chassisID = atoi(chassisID);
                }
                
                slotID = clParserAttr(nodeInstance, CL_CPM_PARSER_ATTR_NODE_INST_SLOT_ID);
                if(slotID)
                {
                    cpmL->slotID = atoi(slotID);
                }

                rc = clCntNodeAdd(gpClCpm->cpmTable, (ClCntKeyHandleT)(ClWordT)nodeKey,
                                  (ClCntDataHandleT) cpmL, NULL);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                               CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc, rc,
                               CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                gpClCpm->noOfCpm += 1;
#if 0
                clOsalPrintf("%s %d key %d\n", nodeName, gpClCpm->noOfCpm,
                             nodeKey);
#endif
            }
            nodeInstance = nodeInstance->next;
        }
    }
    else if (strcmp(cpmType, "LOCAL") == 0)
    {
        gpClCpm->pCpmConfig->cpmType = CL_CPM_LOCAL;
        goto parseNodeList;
    }
    else
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_LOG_MESSAGE_1_INVALID_PARAMETER,
                       "cpmParseCompInfo", CL_CPM_RC(CL_ERR_INVALID_PARAMETER),
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    /*
     * clOsalPrintf("CPM type : %s\n", temp);
     */

    /*
     * Get the CPM default timeout for heartbeating. 
     * Set it to the default timeout.
     */
    gpClCpm->pCpmConfig->defaultFreq = CL_CPM_DEFAULT_TIMEOUT;

    /*
     * clOsalPrintf("CPM defaultFreq : %d\n",
     * gpClCpm->pCpmConfig->defaultFreq);
     */

    /*
     * Get the CPM maximum timeout for heartbeating.  
     */
    gpClCpm->pCpmConfig->threshFreq = CL_CPM_DEFAULT_MAX_TIMEOUT;

    /*
     * clOsalPrintf("CPM maxFreq : %d\n", gpClCpm->pCpmConfig->threshFreq);
     */

    return CL_OK;

  failure:
    /*
     * TODO: Cleanup 
     */
    cpmCompConfigFinalize();
    return rc;
}

void cpmParseUserConfigCompArgs(ClParserPtrT componentInstance,
                                ClCpmCompConfigT *compConfig)
{
    ClRcT rc = CL_OK;
    
    ClParserPtrT args = NULL;
    ClParserPtrT arg = NULL;
    ClParserPtrT exeName = NULL;

    ClUint32T argIndex = 0;

    const ClCharT *value = NULL;

    ClUint32T i = 0;
    
    CPM_PARSER_HANDLE_NULL_CHECK(exeName,
                                 clParserChild(componentInstance,
                                               "exeName"),
                                 "exeName doesn't exist in compType");
    if (exeName != NULL)
    {
        CPM_PARSER_NULL_CHECK(value,
                              clParserAttr(exeName,
                                           CL_CPM_PARSER_ATTR_COMP_TYPE_ARG_VALUE),
                              "value field in exeName doesn't exist");

        clHeapFree(compConfig->argv[0]);

        compConfig->argv[0] = (ClCharT *) clHeapAllocate(strlen(value)+1);
        if (compConfig->argv[0] == NULL)
            CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                           CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);

        strncpy(compConfig->argv[0], value, strlen(value));
    }

    CPM_PARSER_HANDLE_NULL_CHECK(args,
                                 clParserChild(componentInstance,
                                               CL_CPM_PARSER_TAG_COMP_TYPE_ARGS),
                                 "args doesn't exist in compType");
    if (args != NULL)
    {
        /*
         * Because compConfig->argv[0] is executable name !!
         */
        argIndex = 1;
        
        arg = clParserChild(args, CL_CPM_PARSER_TAG_COMP_TYPE_ARG);
        if (arg)
        {
            for (i = argIndex; compConfig->argv[i]; ++i)
            {
                clHeapFree(compConfig->argv[i]);
            }
        }
        while (arg != NULL)
        {
            CPM_PARSER_NULL_CHECK(value,
                                  clParserAttr(arg,
                                               CL_CPM_PARSER_ATTR_COMP_TYPE_ARG_VALUE),
                                  "value field in arg doesn't exist");

            compConfig->argv[argIndex] = 
            (ClCharT *) clHeapAllocate(strlen(value)+1);
            if (compConfig->argv[argIndex] == NULL)
                CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                               CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                               CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);

            strncpy(compConfig->argv[argIndex], value, strlen(value));

            argIndex++;
            arg = arg->next;
        }
        compConfig->argv[argIndex] = NULL;
    }
    
    return;
    
failure:
    for (i = 1; i < argIndex; ++i)
        clHeapFree(compConfig->argv[argIndex]);
}


/*
 * Parses the information about user defined
 * component instances and service unit instances
 * and populates corresponding configuration 
 * information into CPM data structure.
 */
ClRcT cpmParseConfig(ClParserPtrT file)
{
    /*
     * Tag variables. 
     */
    ClParserPtrT nodeInstances = NULL;
    ClParserPtrT nodeInstance = NULL;
    ClParserPtrT serviceUnitInstances = NULL;
    ClParserPtrT serviceUnitInstance = NULL;
    ClParserPtrT componentInstances = NULL;
    ClParserPtrT componentInstance = NULL;

    /*
     * Attribute variables. 
     */
    const ClCharT *nodeName = NULL;
    const ClCharT *compName = NULL;
    const ClCharT *componentType = NULL;

    /*
     * Pointer to traverse the component info list. 
     */
    ClCpmCompInfoT *pCompList;

    ClCpmComponentT *comp = NULL;
    ClUint16T compKey = 0;
    ClCpmComponentRefT *compRef;

    ClRcT rc = CL_OK;

    /*
     * Find the node instance with the given node name. 
     */
    CPM_PARSER_NULL_CHECK(nodeInstances,
                          clParserChild(file, CL_CPM_PARSER_TAG_NODE_INSTS),
                          "nodeInstances tag does not exist in XML file\n");
    CPM_PARSER_NULL_CHECK(nodeInstance,
                          clParserChild(nodeInstances,
                                        CL_CPM_PARSER_TAG_NODE_INST),
                          "nodeInstance tag does not exist in nodeInstances\n");
    while (nodeInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeName,
                              clParserAttr(nodeInstance,
                                           CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                              "name doesn't exist in nodeInstance \n");
        if (strcmp(nodeName, clCpmNodeName) == 0)
        {
            /*
             * Found the node instance. 
             */
            break;
        }
        nodeInstance = nodeInstance->next;
    }
    if (nodeInstance == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Node Instance not found\n"));
        return CL_CPM_RC(CL_ERR_NOT_EXIST);
    }

    /*
     * Now read through the service units and components configuration.
     */
    serviceUnitInstances =
        clParserChild(nodeInstance, CL_CPM_PARSER_TAG_NODE_SU_INSTS);
    if (serviceUnitInstances == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,
                       ("serviceUnitInstances doesn't exist in nodeInstance\n"));
        return CL_OK;
    }
    serviceUnitInstance =
        clParserChild(serviceUnitInstances, CL_CPM_PARSER_TAG_NODE_SU_INST);
    while (serviceUnitInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(componentInstances,
                              clParserChild(serviceUnitInstance,
                                            CL_CPM_PARSER_TAG_NODE_COMP_INSTS),
                              "componentInstances doesn't exist in serviceUnitInstance");
        CPM_PARSER_NULL_CHECK(componentInstance,
                              clParserChild(componentInstances,
                                            CL_CPM_PARSER_TAG_NODE_COMP_INST),
                              "componentInstance doesn't exist in serviceUnitInstance");
        while (componentInstance != NULL)
        {
            /*
             * For every component instance in this 
             * service unit instance get the component type.
             */
            CPM_PARSER_NULL_CHECK(componentType,
                                  clParserAttr(componentInstance,
                                               CL_CPM_PARSER_ATTR_NODE_COMP_INST_TYPE),
                                  "type doesn't exist in compinst");
            /*
             * clOsalPrintf("Component type : %s\n", componentType);
             */

            pCompList = cpmCompList;
            while (pCompList != NULL)
            {
                /*
                 * Look up the component type in the component list. 
                 */
                if (strcmp(pCompList->compType, componentType) == 0)
                {
                    /*
                     * Found the component type. 
                     * Fetch the component information 
                     * from the component list.
                     */
                    CPM_PARSER_NULL_CHECK(compName,
                                          clParserAttr
                                          (componentInstance,
                                           CL_CPM_PARSER_ATTR_NODE_COMP_INST_NAME),
                                          "name doesn't exist in compinst");
                    /*
                     * Populate the component name. 
                     */
                    strcpy(pCompList->compConfig.compName, compName);
                    rc = cpmCompConfigure(&(pCompList->compConfig),
                                          &comp);
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                                   pCompList->compConfig.compName, rc,
                                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

                    /*
                     * Can be enabled in future if required.
                     * Please refer to Bug 7320.
                     *
                     * cpmParseUserConfigCompArgs(componentInstance,
                     *                            comp->compConfig);
                     */
                            
                    rc = clCksm16bitCompute((ClUint8T *) comp->
                                            compConfig->compName,
                                            strlen(comp->compConfig->
                                                   compName), &compKey);
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_CNT_CKSM_ERR,
                                   comp->compConfig->compName, rc, rc,
                                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                    rc = clCntNodeAdd(gpClCpm->compTable,
                                      (ClCntKeyHandleT)(ClWordT)compKey,
                                      (ClCntDataHandleT) comp, NULL);
                    CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                                   CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED,
                                   rc, rc, CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);
                    /*
                     * Add component reference to the service unit 
                     */
                    compRef =
                    (ClCpmComponentRefT *)
                    clHeapAllocate(sizeof(ClCpmComponentRefT));
                    if (compRef == NULL)
                        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                                       CL_CPM_RC(CL_ERR_NO_MEMORY),
                                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                    compRef->ref = comp;
                    compRef->pNext = NULL;
                    /*
                     * Increase the number of count for components. 
                     */
                    gpClCpm->noOfComponent++;

                    /*
                     * Move on to the next component 
                     * in this service unit instance.
                     */
                    break;
                }
                else
                {
                    /*
                     * Keep searching for the 
                     * matching component type.
                     */
                    pCompList = pCompList->pNext;
                }
            }           /* End of while for component linked list. */
            if (pCompList == NULL)
            {
                /*
                 * Component type was not found. 
                 * TODO:take appropriate action.
                 */
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                             ("Component Type Not found.\n"),
                             CL_CPM_RC(CL_ERR_DOESNT_EXIST));
                exit(1);
            }
            componentInstance = componentInstance->next;
        }               /* End of while componentInstance. */
        serviceUnitInstance = serviceUnitInstance->next;
    }                           /* End of while serviceUnitInstance. */

    return rc;

  failure:
    /*
     * TODO: Cleanup 
     */
    cpmCompConfigFinalize();
    return rc;
}

/*
 * Parses the information about the ASP
 * component instances and service unit instances
 * and populates corresponding configuration 
 * information into CPM data structure.
 */
ClRcT cpmParseAspConfig(ClParserPtrT configFile,
                        ClParserPtrT defFile,
                        ClParserPtrT aspInstanceFile)
{
    /*
     * Tag variables. 
     */
    ClParserPtrT nodeTypes = NULL;
    ClParserPtrT nodeType = NULL;
    ClParserPtrT classType = NULL;
    ClParserPtrT nodeClassTypes = NULL;
    ClParserPtrT nodeClassType = NULL;
    ClParserPtrT serviceUnitInstances = NULL;
    ClParserPtrT serviceUnitInstance = NULL;
    ClParserPtrT nodeInstances = NULL;
    ClParserPtrT nodeInstance = NULL;
    ClParserPtrT componentInstances = NULL;
    ClParserPtrT componentInstance = NULL;

    ClCharT *cpmNodeClassType = NULL;

    /*
     * Attribute variables. 
     */
    const ClCharT *componentType = NULL;
    const ClCharT *nodeName = NULL;
    const ClCharT *compName = NULL;
    const ClCharT *nodeTypeName = NULL;
    const ClCharT *classTypeName = NULL;
    const ClCharT *cpmNodeType = NULL;

    /*
     * Pointer to traverse the component info list. 
     */
    ClCpmCompInfoT *pCompList;

    ClCpmComponentT *comp = NULL;
    ClUint16T compKey = 0;
    ClCpmComponentRefT *compRef;

    ClRcT rc = CL_OK;

    ClCharT aspCompName[CL_MAX_NAME_LENGTH];

    /*
     * Find the node type from given node name. 
     */
    CPM_PARSER_NULL_CHECK(nodeInstances,
                          clParserChild(configFile,
                                        CL_CPM_PARSER_TAG_NODE_INSTS),
                          "nodeInstances tag does not exist");
    CPM_PARSER_NULL_CHECK(nodeInstance,
                          clParserChild(nodeInstances,
                                        CL_CPM_PARSER_TAG_NODE_INST),
                          "nodeInstance tag does not exist");
    while (nodeInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeName,
                              clParserAttr(nodeInstance,
                                           CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                              "field name doesn't exist in nodeInstance");
        if (strcmp(nodeName, clCpmNodeName) == 0)
        {
            /*
             * Found the required node type. 
             */
            CPM_PARSER_NULL_CHECK(cpmNodeType,
                                  clParserAttr(nodeInstance,
                                               CL_CPM_PARSER_ATTR_NODE_INST_TYPE),
                                  "type doesn't exist in nodeInstance");
            break;
        }
        nodeInstance = nodeInstance->next;
    }
    if (nodeInstance == NULL)
    {
        ClCpmNodeConfigT nodeConfig = {{0}};
        rc = clCpmNodeConfigGet(clCpmNodeName, &nodeConfig);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Node Instance not found [rc %#x]. exiting .....\n", rc));
            exit(1);
        }
        if(!strcmp(nodeConfig.cpmType, "LOCAL"))
        {
            cpmNodeClassType = "CL_AMS_NODE_CLASS_C"; /*payloads can be added dynamically*/
        }
        else
        {
            cpmNodeClassType = "CL_AMS_NODE_CLASS_B";
        }
        goto load_classtype;
    }

    CPM_PARSER_NULL_CHECK(nodeTypes,
                          clParserChild(defFile, CL_CPM_PARSER_TAG_NODE_TYPES),
                          "nodeTypes tag does not exist in XML file\n");
    CPM_PARSER_NULL_CHECK(nodeType,
                          clParserChild(nodeTypes, CL_CPM_PARSER_TAG_NODE_TYPE),
                          "nodeType tag does not exist in nodeTypes\n");
    while (nodeType != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeTypeName,
                              clParserAttr(nodeType,
                                           CL_CPM_PARSER_ATTR_NODE_TYPE_NAME),
                              "name tag does not exist in nodeType\n");
        if (strcmp(nodeTypeName, cpmNodeType) == 0)
        {
            /*
             * Found the node class type. 
             */
            break;
        }
        nodeType = nodeType->next;
    }
    if (nodeType == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Node type with a given node name not found. exiting .....\n"));
        exit(1);
    }

    /*
     * Get the class type of that node. 
     */
    CPM_PARSER_NULL_CHECK(classType,
                          clParserChild(nodeType,
                                        CL_CPM_PARSER_TAG_NODE_CLASS_TYPE),
                          "classType tag does not exist in nodeType\n");
    cpmNodeClassType = classType->txt;
    if (cpmNodeClassType == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                       "classType", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        /*
         * Find the node with this class type. 
         */
        load_classtype:
        CPM_PARSER_NULL_CHECK(nodeClassTypes,
                              clParserChild(aspInstanceFile,
                                            CL_CPM_PARSER_TAG_NODE_CLASS_TYPES),
                              "nodeClassTypes doesn't exist in XML file\n");
        CPM_PARSER_NULL_CHECK(nodeClassType,
                              clParserChild(nodeClassTypes,
                                            CL_CPM_PARSER_TAG_NODE_CLASS_TYPE1),
                              "nodeClassType doesn't exist in nodeClassTypes tag\n");
        while (nodeClassType != NULL)
        {
            CPM_PARSER_NULL_CHECK(classTypeName,
                                  clParserAttr(nodeClassType,
                                               CL_CPM_PARSER_ATTR_NODE_CLASS_TYPE1_NAME),
                                  "name doesn't exist in nodeClassType\n");
            if (strcmp(classTypeName, cpmNodeClassType) == 0)
            {
                /*
                 * Found the node class type. 
                 */
                break;
            }
            nodeClassType = nodeClassType->next;
        }
        if (nodeClassType == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Node class type not found. exiting .....\n"));
            exit(1);
        }
    }

    /*
     * Now read through the service units and components configuration.
     */
    serviceUnitInstances = clParserChild(nodeClassType,
                                         CL_CPM_PARSER_TAG_NODE_SU_INSTS);
    serviceUnitInstance = clParserChild(serviceUnitInstances,
                                        CL_CPM_PARSER_TAG_NODE_SU_INST);
    while (serviceUnitInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(componentInstances,
                              clParserChild(serviceUnitInstance,
                                            CL_CPM_PARSER_TAG_NODE_COMP_INSTS),
                              "componentInstances doesn't exist in serviceUnitInstance");
        CPM_PARSER_NULL_CHECK(componentInstance,
                              clParserChild(componentInstances,
                                            CL_CPM_PARSER_TAG_NODE_COMP_INST),
                              "componentInstance doesn't exist in serviceUnitInstance");
        while (componentInstance != NULL)
        {
            /*
             * For every component instance in this 
             * service unit instance get the component type.
             */
            CPM_PARSER_NULL_CHECK(componentType,
                                  clParserAttr(componentInstance,
                                               CL_CPM_PARSER_ATTR_NODE_COMP_INST_TYPE),
                                  "type doesn't exist in compinst");
            /*
             * clOsalPrintf("Component type : %s\n", componentType);
             */

            pCompList = cpmCompList;
            while (pCompList != NULL)
            {
                /*
                 * Look up the component type in the component list. 
                 */
                if (strcmp(pCompList->compType, componentType) == 0)
                {
                    /*
                     * Found the component type. 
                     * Fetch the component information 
                     * from the component list.
                     */
                    CPM_PARSER_NULL_CHECK(compName,
                                          clParserAttr
                                          (componentInstance,
                                           CL_CPM_PARSER_ATTR_NODE_COMP_INST_NAME),
                                          "name doesn't exist in compinst");
                    /*
                     * Populate the component name. 
                     */
                    sprintf(aspCompName, "%s_%s", compName,
                            gpClCpm->pCpmConfig->nodeName);
                    /*
                     * FIXME: 
                     */
                    strcpy(pCompList->compConfig.compName,
                           aspCompName);
                    rc = cpmCompConfigure(&(pCompList->compConfig),
                                          &comp);
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR,
                                   pCompList->compConfig.compName, rc,
                                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

                    rc = clCksm16bitCompute((ClUint8T *) comp->
                                            compConfig->compName,
                                            strlen(comp->compConfig->
                                                   compName), &compKey);
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_CNT_CKSM_ERR,
                                   comp->compConfig->compName, rc, rc,
                                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                    rc = clCntNodeAdd(gpClCpm->compTable,
                                      (ClCntKeyHandleT)(ClWordT)compKey,
                                      (ClCntDataHandleT) comp, NULL);
                    CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                                   CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED,
                                   rc, rc, CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);
                    /*
                     * Add component reference to the service unit 
                     */
                    compRef = clHeapAllocate(sizeof(ClCpmComponentRefT));
                    if (compRef == NULL)
                        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                                       CL_CPM_RC(CL_ERR_NO_MEMORY),
                                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                    compRef->ref = comp;
                    compRef->pNext = NULL;
                    /*
                     * Increase the number of count for components. 
                     */
                    gpClCpm->noOfComponent++;

                    /*
                     * Move on to the next component 
                     * in this service unit instance.
                     */
                    break;
                }
                else
                {
                    /*
                     * Keep searching for the 
                     * matching component type.
                     */
                    pCompList = pCompList->pNext;
                }
            }           /* End of while for component linked list. */
            if (pCompList == NULL)
            {
                /*
                 * Component type was not found. 
                 * TODO:take appropriate action.
                 */
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                             ("Component Type Not found.\n"),
                             CL_CPM_RC(CL_ERR_DOESNT_EXIST));
                exit(1);
            }
            componentInstance = componentInstance->next;
        }               /* End of while componentInstance. */

        serviceUnitInstance = serviceUnitInstance->next;
    }                           /* End of while serviceUnitInstance. */

    return rc;

  failure:
    /*
     * TODO: Cleanup 
     */
    cpmCompConfigFinalize();
    return rc;
}

/*
 * Looks up if the boot row is already present in the XML file.
 * returns CL_TRUE if present else CL_FALSE.
 */
ClRcT cpmLookupBootRow(bootTableT *bmTable, ClUint32T bootLevel)
{
    bootTableT *pDList = bmTable;

    while (pDList != NULL)
    {
        if (pDList->bootLevel == bootLevel)
            return CL_TRUE;     /* the boot row is present already. */
        pDList = pDList->pDown;
    }

    return CL_FALSE;
}

/*
 * Creates an empty boot table with maximum number of boot rows
 * as specified in the XML file.
 */
ClRcT cpmBmInitTable(ClUint32T maxBootRow)
{
    ClUint32T bootRow = 0;

    ClUint32T rc = CL_OK;

    /*
     * For the doubly linked list. 
     */
    bootTableT *prevDList = NULL;
    bootTableT *newDList = NULL;

    prevDList = newDList = gpClCpm->bmTable->table;
    for (bootRow = 1; bootRow <= maxBootRow; ++bootRow)
    {
        newDList = (bootTableT *) clHeapAllocate(sizeof(bootTableT));
        if (newDList == NULL)
            CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                           CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);

        /*
         * Row of a bootLevel. 
         */
        newDList->bootLevel = bootRow;

        /*
         * Number of components in this row. 
         */
        newDList->numComp = 0;
        newDList->listHead = NULL;
        newDList->pUp = NULL;
        newDList->pDown = NULL;

        /*
         * Check if it is the first node. 
         */
        if (prevDList == NULL)
        {
            gpClCpm->bmTable->table = newDList;
            prevDList = newDList;
        }
        else
        {
            newDList->pUp = prevDList;
            prevDList->pDown = newDList;
            prevDList = newDList;
        }
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Adds a component compName to the row in the boot table
 * as specified by bootLevel.
 */
ClRcT cpmBmAddComponent(const ClCharT *compName, ClUint32T bootLevel)
{
    ClRcT rc = CL_OK;

    bootTableT *bootTable = NULL;
    bootRowT *bootRow = NULL;

    bootRowT *newComp = NULL;

    bootRowT *p = NULL;
    bootRowT *prev = NULL;

    bootTable = gpClCpm->bmTable->table;
    while (bootTable != NULL)
    {
        bootRow = bootTable->listHead;
        if (bootTable->bootLevel == bootLevel)
        {
            /*
             * Allocate memory for and append the component. 
             */
            newComp = (bootRowT *) clHeapAllocate(sizeof(bootRowT));
            if (newComp == NULL)
                CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                               CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                               CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            strcpy(newComp->compName, compName);
            newComp->pNext = NULL;

            prev = p = bootRow;
            if (p == NULL)
            {
                /*
                 * First component. 
                 */
                bootTable->listHead = newComp;
            }
            else
            {
                /*
                 * Get to the end of the list. 
                 */
                while (p->pNext != NULL)
                {
                    p = p->pNext;
                }
                p->pNext = newComp;
            }

            /*
             * Increment the number of components in this row. 
             */
            ++bootTable->numComp;

            break;
        }
        bootTable = bootTable->pDown;
    }
    if (bootTable == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid boot level %d\n", bootLevel));
    }

    return CL_OK;

  failure:
    return rc;
}

#if 0
void displayBootTable(bootTableT *table)
{
    bootTableT *pDList = NULL;
    bootRowT *pList = NULL;

    pDList = table;

    while (pDList != NULL)
    {
        clOsalPrintf("Boot Level : %d\n", pDList->bootLevel);
        clOsalPrintf("No of components : %d\n", pDList->numComp);
        clOsalPrintf("Components :");
        pList = pDList->listHead;
        while (pList != NULL)
        {
            clOsalPrintf(" %s", pList->compName);
            pList = pList->pNext;
        }
        clOsalPrintf("\n");
        pDList = pDList->pDown;
    }
}
#endif

/*
 * Check if the given SU is loaded.
 */
ClBoolT cpmIsAspSULoaded(const ClCharT *su)
{
    /*
     * For worker blades, check the status of COR on the master
     * to see if its running. For others, check if the boot manager
     * loaded COR. as that would be consistent with the call at
     * all phases.
     */
    if(CL_CPM_IS_SC())
    {
        ClCpmAspSUMappingT *suMap = scSUCompToLevelMapping;
        ClUint32T sus = CL_CPM_SC_ASP_SUS;
        ClInt32T i;
        if(!su) return CL_FALSE;
        for(i = 0; i < sus; ++i)
        {
            if(!strncmp(su, suMap[i].su, CL_MAX_NAME_LENGTH))
                return suMap[i].suLoaded;
        }
    }
    else
    {
        ClIocAddressT compAddress = {{0}};
        ClStatusT status = CL_STATUS_DOWN;
        ClRcT rc = clCpmMasterAddressGet(&compAddress.iocPhyAddress.nodeAddress);
        if(rc != CL_OK) return CL_FALSE;
        compAddress.iocPhyAddress.portId = CL_IOC_COR_PORT;
        rc = clCpmCompStatusGet(compAddress, &status);
        if(rc != CL_OK) return CL_FALSE;
        if(status == CL_STATUS_UP) return CL_TRUE;
    }
    return CL_FALSE;
}

static ClBoolT cpmIsAspSUPresent(const ClParserPtrT cpmConfig,
                                 const ClCharT *suName)
{
    ClParserPtrT aspServiceUnits = NULL;
    ClParserPtrT aspServiceUnit = NULL;

    const ClCharT *name = NULL;

    /*
     * For handling nodes which are created dynamically.
     */
    if (!cpmConfig) goto no;
    
    CL_ASSERT(suName != NULL);

    CPM_PARSER_NULL_CHECK(aspServiceUnits,
                          clParserChild(cpmConfig,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CFG_ASP_SUS),
                          "aspServiceUnits tag does not exist in cpmConfig");
    CPM_PARSER_NULL_CHECK(aspServiceUnit,
                          clParserChild(aspServiceUnits,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CFG_ASP_SU),
                          "aspServiceUnit tag does not exist in aspServiceUnits");

    while (aspServiceUnit != NULL)
    {
        CPM_PARSER_NULL_CHECK(name,
                              clParserAttr(aspServiceUnit,
                                           CL_CPM_PARSER_ATTR_NODE_CPM_CFG_ASP_SU_NAME),
                              "name doesn't exist in aspServiceUnit");
        
        if (!strcmp(name, suName))
        {
            goto yes;
        }

        aspServiceUnit = aspServiceUnit->next;
    }

no:
    return CL_NO;
yes:
    return CL_YES;
}

/*
 * Parses the deployment configuration file and stores it in CPM boot table
 * data structure. Reads configuration information from XML file 
 * "clAmfConfig.xml" in $ASP_CONFIG.
 */
ClRcT cpmBmParseDeployConfigFile(ClParserPtrT configFile)
{
    ClRcT rc = CL_OK;

    /*
     * Tag variables. 
     */
    ClParserPtrT nodeInstances = NULL;
    ClParserPtrT nodeInstance = NULL;
    ClParserPtrT cpmConfigs = NULL;
    ClParserPtrT cpmConfig = NULL;
    
    /*
     * Attribute variables. 
     */
    const ClCharT *nodeName = NULL;
    const ClCharT *cpmNodeType = NULL;
    const ClCharT *nodeType = NULL;
    const ClCharT *cpmType = NULL;
    const ClCharT *cpmConfigType = NULL;
    ClCharT aspCompName[CL_MAX_NAME_LENGTH];

    ClCpmAspSUMappingT *suCompToLevelMapping = NULL;
    ClUint32T nAspSUs = 0;
    
    ClUint32T i = 0;

#ifdef CPM_BFT
    ClParserPtrT bootConfigs = NULL;
    ClParserPtrT bootConfig = NULL;
    ClParserPtrT serviceUnitInstances = NULL;
    ClParserPtrT serviceUnitInstance = NULL;
    ClParserPtrT componentInstances = NULL;
    ClParserPtrT componentInstance = NULL;

    const ClCharT *bootProfile = NULL;
    const ClCharT *maxBootLevel = NULL;
    const ClCharT *defaultBootLevel = NULL;
    const ClCharT *level = NULL;

    const ClCharT *compName = NULL;
    ClUint32T levelNum = 0;
#endif

    /*
     * Find the CPM node type from given node name. 
     */
    CPM_PARSER_NULL_CHECK(nodeInstances,
                          clParserChild(configFile,
                                        CL_CPM_PARSER_TAG_NODE_INSTS),
                          "nodeInstances doesn't exist in XML file");
    CPM_PARSER_NULL_CHECK(nodeInstance,
                          clParserChild(nodeInstances,
                                        CL_CPM_PARSER_TAG_NODE_INST),
                          "nodeInstance doesn't exist in nodeInstances");
    while (nodeInstance != NULL)
    {
        CPM_PARSER_NULL_CHECK(nodeName,
                              clParserAttr(nodeInstance,
                                           CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                              "field name doesn't exist in nodeInstance");
        if (strcmp(nodeName, clCpmNodeName) == 0)
        {
            /*
             * Found the required node type. 
             */
            CPM_PARSER_NULL_CHECK(cpmNodeType,
                                  clParserAttr(nodeInstance,
                                               CL_CPM_PARSER_ATTR_NODE_INST_TYPE),
                                  "type doesn't exist in nodeInstance");
            break;
        }

        nodeInstance = nodeInstance->next;
    }
    if (nodeInstance == NULL)
    {
        ClCpmNodeConfigT nodeConfig = {{0}};
        rc = clCpmNodeConfigGet(clCpmNodeName, &nodeConfig);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Node Instance not found. exiting .....\n"));
            exit(1);
        }
        cpmConfigType = nodeConfig.cpmType;
    }

    /*
     * Find the CPM configuration from the node type found above. 
     */
    CPM_PARSER_NULL_CHECK(cpmConfigs,
                          clParserChild(configFile,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CONFIGS),
                          "cpmConfigs tag does not exist");
    CPM_PARSER_NULL_CHECK(cpmConfig,
                          clParserChild(cpmConfigs,
                                        CL_CPM_PARSER_TAG_NODE_CPM_CONFIG),
                          "cpmConfig tag does not exist");
    while (cpmConfig != NULL)
    {
        if(cpmNodeType)
        {
            CPM_PARSER_NULL_CHECK(nodeType,
                                  clParserAttr(cpmConfig,
                                               CL_CPM_PARSER_ATTR_NODE_TYPE),
                                  "nodeType doesn't exist in cpmConfig");
            if (!strcmp(nodeType, cpmNodeType))
            {
                /*
                 * Found the required CPM configuration. 
                 */
                break;
            }
        }
        else if(cpmConfigType)
        {
            /*
             * Search by cpm type
             */
            CPM_PARSER_NULL_CHECK(cpmType,
                                  clParserAttr(cpmConfig,
                                               CL_CPM_PARSER_ATTR_NODE_CPM_TYPE),
                                  "cpmType doesn't exist in cpmConfig");
            if(!strcmp(cpmType, cpmConfigType))
            {
                break;
            }
        }
        cpmConfig = cpmConfig->next;
    }

    if (cpmConfig == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("nodeType not found. exiting .....\n"));
        exit(1);
    }

    gpClCpm->bmTable->maxBootLevel = CL_CPM_DEFAULT_MAX_BOOT_LEVEL;
    gpClCpm->bmTable->defaultBootLevel = CL_CPM_DEFAULT_DEFAULT_BOOT_LEVEL;

#ifdef CPM_BFT
    if (strcmp(clCpmBootProfile, CL_CPM_DEFAULT_BOOT_PROFILE) == 0)
    {
        gpClCpm->bmTable->maxBootLevel = CL_CPM_DEFAULT_MAX_BOOT_LEVEL;
        gpClCpm->bmTable->defaultBootLevel = CL_CPM_DEFAULT_DEFAULT_BOOT_LEVEL;
    }
    else    
    {
        bootConfigs = clParserChild(cpmConfig,
                                    CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_CFGS);
        if (bootConfigs)
        {
            bootConfig = clParserChild(bootConfigs,
                                       CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_CFG);
            if (bootConfig)
            {
                while (bootConfig != NULL)
                {
                    CPM_PARSER_NULL_CHECK(bootProfile,
                                          clParserAttr(bootConfig,
                                                       CL_CPM_PARSER_ATTR_NODE_CPM_CFG_BOOT_PROF),
                                          "name doesn't exist in bootConfig");
                    if (strcmp(bootProfile, clCpmBootProfile) == 0)
                    {
                        /*
                         * Found the boot profile. 
                         */
                        break;
                    }
                    
                    bootConfig = bootConfig->next;
                }
                if (bootConfig == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("bootConfig for given boot profile not found. exiting .....\n"));
                    exit(1);
                }
		
                CPM_PARSER_NULL_CHECK(maxBootLevel,
                                      clParserAttr(bootConfig,
                                                   CL_CPM_PARSER_ATTR_MAX_BOOT_LEVEL),
                                      "maxBootLevel doesn't exist in bootConfig");
                gpClCpm->bmTable->maxBootLevel = atoi(maxBootLevel);

                CPM_PARSER_NULL_CHECK(defaultBootLevel,
                                      clParserAttr(bootConfig,
                                                   CL_CPM_PARSER_ATTR_DEFAULT_BOOT_LEVEL),
                                      "defaultBootLevel doesn't exist in bootConfig");
                gpClCpm->bmTable->defaultBootLevel = atoi(defaultBootLevel);
            }
        }
    }
#endif

    /*
     * Create an empty boot table. 
     */
    rc = cpmBmInitTable(gpClCpm->bmTable->maxBootLevel);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_INIT_TABLE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    /*
     * First populate the ASP components. 
     */
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
    {
        if (cpmIsAspSUPresent(cpmConfig, "cmSU"))
        {
            sprintf(aspCompName, "%s_%s",
                    "cmServer",
                    gpClCpm->pCpmConfig->nodeName);

            rc = cpmBmAddComponent(aspCompName, 5);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_COMP_ADD_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }

        if (cpmIsAspSUPresent(cpmConfig, "msgSU"))
        {
            sprintf(aspCompName, "%s_%s",
                    "msgServer",
                    gpClCpm->pCpmConfig->nodeName);

            rc = cpmBmAddComponent(aspCompName, 5);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_COMP_ADD_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        
        suCompToLevelMapping = scSUCompToLevelMapping;
        nAspSUs = CL_CPM_SC_ASP_SUS;
    }
    else if (gpClCpm->pCpmConfig->cpmType == CL_CPM_LOCAL)
    {
        if (cpmIsAspSUPresent(cpmConfig, "msgSU"))
        {
            sprintf(aspCompName, "%s_%s",
                    "msgServer",
                    gpClCpm->pCpmConfig->nodeName);

            rc = cpmBmAddComponent(aspCompName, 5);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_COMP_ADD_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        
        suCompToLevelMapping = wbSUCompToLevelMapping;
        nAspSUs = CL_CPM_WB_ASP_SUS;
    }
    else
    {
        CL_ASSERT(0);
    }
        
    for (i = 0; i < nAspSUs; ++i)
    {
        ClCpmAspSUMappingT *suMap = suCompToLevelMapping + i;
        if (suMap->su && suMap->suCompMap &&
            cpmIsAspSUPresent(cpmConfig, suMap->su))
        {
            ClInt32T j;
            suMap->suLoaded = CL_TRUE;
            for(j = 0; suMap->suCompMap[j].compName; ++j)
            {
                ClCpmAspCompMappingT *compMap = suMap->suCompMap + j;
                sprintf(aspCompName,
                        "%s%s_%s",
                        compMap->compName,
                        "Server",
                        gpClCpm->pCpmConfig->nodeName);
                rc = cpmBmAddComponent(aspCompName, compMap->level);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_COMP_ADD_ERR, rc, rc,
                               CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }

#ifdef CPM_BFT
    /*
     * Then the user defined components from the nodeInstance found above. 
     */
    CPM_PARSER_NULL_CHECK(nodeName,
                          clParserAttr(nodeInstance,
                                       CL_CPM_PARSER_ATTR_NODE_INST_NAME),
                          "field name doesn't exist in nodeInstance");

    serviceUnitInstances =
        clParserChild(nodeInstance, CL_CPM_PARSER_TAG_NODE_SU_INSTS);
    if (serviceUnitInstances == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,
                       ("serviceUnitInstances doesn't exist in nodeInstance\n"));
        return CL_OK;
    }
    serviceUnitInstance =
        clParserChild(serviceUnitInstances, CL_CPM_PARSER_TAG_NODE_SU_INST);
    while (serviceUnitInstance != NULL)
    {
        /* Get the bootlevel of the service unit. */
        level = clParserAttr(serviceUnitInstance,
                             CL_CPM_PARSER_ATTR_NODE_SU_INST_LEVEL);
        if(level != NULL)
        {
            levelNum = atoi(level);

            CPM_PARSER_NULL_CHECK(componentInstances,
                                  clParserChild(serviceUnitInstance,
                                                CL_CPM_PARSER_TAG_NODE_COMP_INSTS),
                                  "componentInstances doesn't exist in serviceUnitInstance");
            CPM_PARSER_NULL_CHECK(componentInstance,
                                  clParserChild(componentInstances,
                                                CL_CPM_PARSER_TAG_NODE_COMP_INST),
                                  "componentInstance doesn't exist in serviceUnitInstance");
	    
            while (componentInstance != NULL)
            {
                CPM_PARSER_NULL_CHECK(compName,
                                      clParserAttr(componentInstance,
                                                   CL_CPM_PARSER_ATTR_NODE_COMP_INST_NAME),
                                      "name doesn't exist in compinst");
                
                rc = cpmBmAddComponent(compName, levelNum);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_COMP_ADD_ERR,
                               rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                
                componentInstance = componentInstance->next;
            }
        }
        
        serviceUnitInstance = serviceUnitInstance->next;
    }
#endif

    return CL_OK;

    failure:
    return rc;
}

static void cpmParseGmsConfig(void)
{
    ClParserPtrT xml = NULL;
    ClParserPtrT gms = NULL;
    ClParserPtrT electionTimeout = NULL;
    const ClCharT *str = NULL;

    gpClCpm->cpmGmsTimeout = CL_CPM_GMS_DEFAULT_TIMEOUT;

    if( (str = getenv("CL_ASP_BOOT_ELECTION_TIMEOUT")))
    {
        gpClCpm->cpmGmsTimeout = atoi(str);
        clLogInfo("CPM", "GMS", "Using GMS timeout of [%d] secs",
                  gpClCpm->cpmGmsTimeout);
        return ;
    }
    if(!(str = getenv("ASP_CONFIG")))
    {
        return ;
    }
    xml = clParserOpenFile(str, CL_CPM_GMS_CONF_FILE_NAME);
    if(!xml)
    {
        return;
    }
    gms = clParserChild(xml, "gms");
    if(!gms)
    {
        clParserFree(xml);
        return ;
    }
    electionTimeout = clParserChild(gms, "bootElectionTimeout");
    if(!electionTimeout)
    {
        clParserFree(xml);
        return;
    }
    gpClCpm->cpmGmsTimeout = atoi(electionTimeout->txt);
    clParserFree(xml);
    clLogInfo("CPM", "GMS", "Using GMS timeout of [%d] secs",
              gpClCpm->cpmGmsTimeout);
}

/*
 * Parses the deployment configuration file and populates 
 * all the CPM data structures with the required configuration.
 */
ClRcT cpmGetConfig(void)
{
    /*
     * Parser structure for all the user and default configuration files. 
     */
    ClParserPtrT configFile = NULL;
    ClParserPtrT defFile = NULL;
    ClParserPtrT aspDefFile = NULL;
    ClParserPtrT aspInstFile = NULL;
    ClCharT *filePath = NULL;
    ClRcT rc = CL_OK;


    filePath = getenv(CL_ASP_CONFIG_PATH);
    if (filePath != NULL)
    {
        configFile = clParserOpenFile(filePath, CL_CPM_CONFIG_FILE_NAME);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR, rc,
                       rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);

        defFile = clParserOpenFile(filePath, CL_CPM_DEF_FILE_NAME);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR, rc,
                       rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);

        aspDefFile = clParserOpenFile(filePath, CL_CPM_ASP_DEF_FILE_NAME);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR, rc,
                       rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);

        aspInstFile = clParserOpenFile(filePath, CL_CPM_ASP_INST_FILE_NAME);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR, rc,
                       rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("ASP_CONFIG path is not set in the environment \n"));
        exit(1);
    }

    /*
     * Parse all the component definition. 
     */
    rc = cpmParseCompInfo(defFile, CL_FALSE);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INFO_PARSE_ERR,
                   "component", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmParseCompInfo(aspDefFile, CL_TRUE);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INFO_PARSE_ERR,
                   "component", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

#if CPM_DEBUG
    displayCompList(cpmCompList);
#endif

    rc = cpmParseNodeInfo(configFile);

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        ClCpmNodeConfigT nodeConfig = {{0}};
        ClTimerTimeOutT delays[] =  {  /*have the backoff tries as power of 2*/
            {.tsSec = 2, .tsMilliSec = 0 },
            {.tsSec = 4, .tsMilliSec = 0 },
            {.tsSec = 6, .tsMilliSec = 0 },
            {.tsSec = 8, .tsMilliSec = 0 },
        };
        ClInt32T maxTries = (ClUint32T)sizeof(delays)/sizeof(delays[0]) - 1;
        ClInt32T tries = 0;
        ClIocNodeAddressT masterAddress = 0;
        for(;;)
        {
            masterAddress = 0;
            rc = clCpmMasterAddressGet(&masterAddress);
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST
               ||
               CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)
            {
                if(clCpmIsSC())
                {
                    /*
                     * We ourselves are dynamically added CPM/G.
                     * Set the config. and go ahead.
                     */
                    strncpy(nodeConfig.nodeName, clCpmNodeName, sizeof(nodeConfig.nodeName)-1);
                    strncpy(nodeConfig.cpmType, "GLOBAL", sizeof(nodeConfig.cpmType)-1);
                    clNameSet(&nodeConfig.nodeType, clCpmNodeName);
                    clNameSet(&nodeConfig.nodeIdentifier, clCpmNodeName);
                    goto config_set;
                }
                tries &= maxTries;
                clOsalTaskDelay(delays[tries++]);
            }
            else break;
        }

        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                           ("CPM exiting because of master address get failure with [%#x]", rc));
            exit(1);
        }
        
        /*
         * Do a node config get with retries incase of failures
         * as master could be booting up.
         * The TIPC bitmap might not have been synced with the master
         * and this delay is specific to this case and cannot be generalized.
         * We could also be under a delayed node add from master CPM booting up
         * coz of COR dependency sequence.
         */

        tries = 0;
        do 
        {
            clOsalTaskDelay(delays[tries++]);
            tries &= maxTries;
            rc = clCpmNodeConfigGet(clCpmNodeName, &nodeConfig);
            clLogNotice("CONFIG", "GET", "Node config for [%s] returned [%#x]", clCpmNodeName, rc);
        } while ( 
                 (
                  CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST
                  ||
                  CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST
                  ||
                  CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE)
                 && 
                 --maxTries > 0);

        if(rc == CL_OK)
        {
            config_set:
            clNameCopy(&gpClCpm->pCpmLocalInfo->nodeType, &nodeConfig.nodeType);
            clNameCopy(&gpClCpm->pCpmLocalInfo->nodeIdentifier, &nodeConfig.nodeIdentifier);
            clNameCopy(&gpClCpm->pCpmLocalInfo->nodeMoIdStr, &nodeConfig.nodeMoIdStr);
            if(!strcmp(nodeConfig.cpmType, "LOCAL"))
            {
                gpClCpm->pCpmConfig->cpmType = CL_CPM_LOCAL;
            }
            else
            {
                gpClCpm->pCpmConfig->cpmType = CL_CPM_GLOBAL;
            }
        }
        else
        {
            /*
             * Just force a shutdown at this stage.
             */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CPM exiting..\n"));
            exit(1);
        }
    }

    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_PARSER_INFO_PARSE_ERR, "node",
                   rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmTableInitialize();
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to initialize the bmTable failed\n"),
                 rc);

    /*
     * Now populate the CPM Data Structures definition. 
     */
    /*
     * First from user defined configuration file. 
     */
    rc = cpmParseConfig(configFile);
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        ClCpmNodeConfigT nodeConfig = {{0}};
        rc = clCpmNodeConfigGet(clCpmNodeName, &nodeConfig);
        if(rc == CL_OK)
        {
            /*
             * Commenting it as its added dynamically when the remote comp.
             * instantiate comes through. If we add it now, we will lose
             * any cpm config updates before the component is instantiated 
             * 
             rc = cpmComponentsAddDynamic(clCpmNodeName);
            */
        }
    }

    CL_CPM_CHECK(CL_DEBUG_ERROR, ("cpmParseConfig failed\n"), rc);

    rc = cpmParseAspConfig(configFile, defFile, aspInstFile);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("cpmParseAspConfig failed\n"), rc);

    /*
     * Initialize the CPM-BM submodule data structure. 
     */
    rc = cpmBmInitDS();
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("InitCpmBmDS failed.\n"), rc);

    /*
     * Parse the deployment configuration file and populate
     * the CPM-BM data structure.
     */
    rc = cpmBmParseDeployConfigFile(configFile);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("ParseDeployConfigFile failed.\n"), rc);

    rc = cpmParseSlotFile();
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("ParseSlotConfigFile failed.\n"), rc);

    cpmParseGmsConfig();

    rc = CL_OK;

    failure:
    if (configFile != NULL)
        clParserFree(configFile);
    if (defFile != NULL)
        clParserFree(defFile);
    if (aspDefFile != NULL)
        clParserFree(aspDefFile);
    if (aspInstFile != NULL)
        clParserFree(aspInstFile);
    return rc;
}

/*
 * Container related functions.
 */
int cpmSlotStoreCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

unsigned int cpmSlotStoreHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT)key % CL_CPM_COMPTABLE_BUCKET_SIZE);
}

void cpmSlotDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((ClCpmNodeClassTypeT *) userData);
}

ClRcT cpmParseSlotFile(void)
{
    ClRcT rc = CL_OK;
    ClParserPtrT slotFile = NULL;
    ClParserPtrT slots = NULL;
    ClParserPtrT slot = NULL;
    ClParserPtrT classTypes = NULL;
    ClParserPtrT classType = NULL;
    const ClCharT *temp = NULL;
    ClCharT *filePath = NULL;
    ClUint32T   slotNumber = 0;
    ClCpmNodeClassTypeT *nodeClassType = NULL;

    filePath = getenv(CL_ASP_CONFIG_PATH);

    if (filePath != NULL)
    {
        slotFile = clParserOpenFile(filePath, CL_CPM_SLOT_FILE_NAME);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("ASP_CONFIG path is not set in the environment \n"));
        exit(1);
    }

    rc = clCntHashtblCreate(CL_CPM_COMPTABLE_BUCKET_SIZE, cpmSlotStoreCompare,
                            cpmSlotStoreHashFunc, NULL, cpmSlotDelete,
                            CL_CNT_NON_UNIQUE_KEY, &gpClCpm->slotTable);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    CPM_PARSER_NULL_CHECK(slots, clParserChild(slotFile, CL_CPM_PARSER_TAG_SLOTS),
                          "slots tag does not exist");
    CPM_PARSER_NULL_CHECK(slot, clParserChild(slots, CL_CPM_PARSER_TAG_SLOT),
                          "slot tag does not exist");

    while (slot != NULL)
    {
        CPM_PARSER_NULL_CHECK(temp,
                              clParserAttr(slot,
                                           CL_CPM_PARSER_ATTR_SLOT_NUM),
                              "slotNumber doesn't exist in slot");
        slotNumber = atoi(temp);

        CPM_PARSER_NULL_CHECK(classTypes, clParserChild(slot, CL_CPM_PARSER_TAG_CLASS_TYPES),
                              "classTypes tag does not exist");
        CPM_PARSER_NULL_CHECK(classType, clParserChild(classTypes, CL_CPM_PARSER_TAG_CLASS_TYPE),
                              "classType tag does not exist");
        while (classType != NULL)
        {
            nodeClassType = (ClCpmNodeClassTypeT *)clHeapAllocate(sizeof(ClCpmNodeClassTypeT));
            if (nodeClassType == NULL)
                CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                               CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                               CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);

            CPM_PARSER_NULL_CHECK(temp,
                                  clParserAttr(classType,
                                               CL_CPM_PARSER_ATTR_CLASS_TYPE_NAME),
                                  "name doesn't exist in identifier");
            strcpy(nodeClassType->name.value, temp);
            nodeClassType->name.length = strlen(nodeClassType->name.value);
            
            temp = clParserAttr(classType, CL_CPM_PARSER_ATTR_CLASS_TYPE_ID);
            if (temp != NULL)
            {
                strcpy(nodeClassType->identifier.value, temp);
                nodeClassType->identifier.length = strlen(nodeClassType->identifier.value);
            }
            else
            {
                strcpy(nodeClassType->identifier.value, "\0");
                nodeClassType->identifier.length = 0;
            }
            clOsalMutexLock(&gpClCpm->cpmMutex);
            rc = clCntNodeAdd(gpClCpm->slotTable, (ClCntKeyHandleT) (ClWordT)slotNumber,
                              (ClCntDataHandleT) nodeClassType, NULL);
            clOsalMutexUnlock(&gpClCpm->cpmMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                           CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            classType = classType->next;
        }

        slot = slot->next;
    }

    clParserFree(slotFile);
    return rc;

failure:
    if (nodeClassType != NULL)
        clHeapFree(nodeClassType);
    clParserFree(slotFile);
    return rc;
}

ClRcT cpmSlotClassTypesGet(ClCntHandleT slotTable,
                           ClUint32T slotNumber, 
                           ClCpmSlotClassTypesT *slotClassTypes)
{
    ClRcT       rc = CL_OK;
    ClUint32T   numSlot = 0;
    ClUint32T   i = 0;
    ClCntNodeHandleT hSlot = 0;
    ClCpmNodeClassTypeT *tempNodeClassType = NULL;

    /* Initialize to NULL to correctly identify failure of memory allocation */
    slotClassTypes->nodeClassTypes = NULL;

    rc = clCntNodeFind(slotTable, (ClCntKeyHandleT) (ClWordT)slotNumber, &hSlot);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                   "Unable to find node class types for slot [%d], "
                   "error [%#x]",
                   slotNumber,
                   rc);
        goto failure;
    }
    
    rc = clCntKeySizeGet(slotTable, (ClCntKeyHandleT) (ClWordT)slotNumber, &numSlot);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_KEY_SIZE_GET_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    slotClassTypes->numItems = numSlot;
    slotClassTypes->nodeClassTypes = 
        (ClCpmNodeClassTypeT *)clHeapAllocate(numSlot * sizeof(ClCpmNodeClassTypeT));
    if (slotClassTypes->nodeClassTypes == NULL)
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                       CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);
    i = 0;
    while (numSlot > 0)
    {
        rc = clCntNodeUserDataGet(slotTable, hSlot,
                                  (ClCntDataHandleT *) &tempNodeClassType);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR,
                       rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        memcpy(&(slotClassTypes->nodeClassTypes[i]), tempNodeClassType, 
                sizeof(ClCpmNodeClassTypeT));
        i++;
        numSlot--;
        if (numSlot)
        {
            rc = clCntNextNodeGet(slotTable, hSlot, &hSlot);
            CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR,
                           "slot", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }

    return rc;

failure:
    if(slotClassTypes->nodeClassTypes != NULL)
        clHeapFree(slotClassTypes->nodeClassTypes);
    return rc;
    
}

ClRcT cpmSlotClassAdd(ClNameT *type, ClNameT *identifier, ClUint32T slotNumber)
{
    ClCpmNodeClassTypeT *classType =  NULL;
    ClUint32T *slots = NULL;
    ClUint32T numSlots = 0;
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    rc = clCntSizeGet(gpClCpm->slotTable, &numSlots);
    if(rc != CL_OK)
    {
        goto out;
    }

    slots = clHeapCalloc(numSlots, sizeof(*slots));
    CL_ASSERT(slots != NULL);

    classType = clHeapCalloc(1, sizeof(*classType));
    CL_ASSERT(classType != NULL);
    clNameCopy(&classType->name, type);
    clNameCopy(&classType->identifier, identifier);
    numSlots = 0;
    if(slotNumber)
    {
        slots[numSlots++] = slotNumber;
    }
    else
    {
        /*
         * Add this entry to all the slots. as we wont know the slot address.
         * of the dynamic node till its started.
         */
        ClCntNodeHandleT nodeHandle = 0;
        ClCntKeyHandleT keyHandle = 0;
        ClUint32T lastSlot = 0;
        rc = clCntFirstNodeGet(gpClCpm->slotTable, &nodeHandle);
        while(rc == CL_OK && nodeHandle)
        {
            rc = clCntNodeUserKeyGet(gpClCpm->slotTable, nodeHandle, &keyHandle);
            if(rc != CL_OK) 
            {
                goto out;
            }
            slotNumber = (ClUint32T)(ClWordT)keyHandle;
            if(slotNumber != lastSlot)
            {
                slots[numSlots++] = slotNumber;
                lastSlot = slotNumber;
            }
            rc = clCntNextNodeGet(gpClCpm->slotTable, nodeHandle, &nodeHandle);
        }
        rc = CL_OK;
    }

    for(i = 0; i < numSlots; ++i)
    {
        if(!classType)
        {
            classType = clHeapCalloc(1, sizeof(*classType));
            CL_ASSERT(classType != NULL);
            clNameCopy(&classType->name, type);
            clNameCopy(&classType->identifier, identifier);
        }
        clLogInfo("SLOT", "ADD", "Class Type [%.*s] added to slot [%d]",
                  type->length, type->value, slots[i]);
        rc = clCntNodeAdd(gpClCpm->slotTable, (ClCntKeyHandleT)(ClWordT)slots[i],
                          (ClCntDataHandleT)classType, NULL);
        if(rc != CL_OK)
        {
            clHeapFree(classType);
            goto out;
        }
        classType = NULL;
    }

    out:    

    if(slots) clHeapFree(slots);

    if(rc != CL_OK)
    {
        clLogError("SLOT", "ADD", "Class Type [%.*s] slot add returned [%#x]",
                   type->length, type->value, rc);
    }

    return rc;
}

/*
 * Delete class type from all slot classes.
 */
ClRcT cpmSlotClassDelete(const ClCharT *type)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = CL_HANDLE_INVALID_VALUE;
    ClBoolT found = CL_FALSE;

    if(!type) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    rc = clCntFirstNodeGet(gpClCpm->slotTable, &nodeHandle);
    while(rc == CL_OK && nodeHandle)
    {
        ClCpmNodeClassTypeT *cpmClassType = NULL;
        ClCntNodeHandleT nextNodeHandle = 0;
        rc = clCntNextNodeGet(gpClCpm->slotTable, nodeHandle, &nextNodeHandle);
        if(clCntNodeUserDataGet(gpClCpm->slotTable, nodeHandle, (ClCntDataHandleT*)&cpmClassType) == CL_OK)
        {
            if(!strcmp(cpmClassType->name.value, type))
            {
                ClCntKeyHandleT key = 0;
                if(clCntNodeUserKeyGet(gpClCpm->slotTable, nodeHandle, &key) == CL_OK)
                {
                    clLogInfo("SLOT", "DELETE", "Deleting class type [%s] from slot [%d]",
                              type, (ClUint32T)(ClWordT)key);
                }
                clCntNodeDelete(gpClCpm->slotTable, nodeHandle);
                found = CL_TRUE;
            }
        }
        nodeHandle = nextNodeHandle;
    }
    
    if(found == CL_FALSE) return CL_CPM_RC(CL_ERR_NOT_EXIST);
    return CL_OK;
}
