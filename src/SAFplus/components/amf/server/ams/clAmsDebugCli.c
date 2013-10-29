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
 * ModuleName  : amf
 * File        : clAmsDebugCli.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * clAmsDebugCli.c: This file processes the commands from debug cli.
******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clAmsDebugCli.h>
#include <clDebugApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsErrors.h>
#include <clBufferApi.h>
#include <clAms.h>
#include <clAmsSAServerApi.h>
#include <clAmsEventServerApi.h>
#include <clAmsModify.h>
#include <clAmsServerUtils.h>
#include <clAmsEntityTrigger.h>
#include <clTaskPool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

/******************************************************************************
 * Global structures and macros  
 *****************************************************************************/

#define CL_AMS_DEBUG_ENABLE_COMMAND  1
#define CL_AMS_DEBUG_DISABLE_COMMAND  2
#define CL_AMS_DEBUG_GET_COMMAND  3
#define MAX_BUFFER_SIZE  1024

#define CL_AMS_DEBUG_PRINT_FILE  "amsDbPrint.tmp"
#define CL_AMS_DEBUG_PRINT_XML_FILE  "ams_db_print.xml"

/*
 * Global variables to be used across all management API initialization
 */

static  ClVersionT  gVersion = {'B', 01, 01 };
static  ClBoolT gClInitialized = CL_FALSE;
FILE  *debugPrintFP = NULL;
ClBoolT debugPrintAll = CL_NO;
typedef struct ClAmsForceLockContext
{
    ClRmdResponseContextHandleT responseHandle;
    ClBufferHandleT outMsgHandle;
    ClCharT *respBuffer;
    ClCharT entity[CL_MAX_NAME_LENGTH];
}ClAmsForceLockContextT;

#if defined (CL_AMS_MGMT_HOOKS)
static ClRcT clAmsMgmtEntityAdminResponse(ClAmsEntityTypeT type,
                                          ClAmsMgmtAdminOperT oper,
                                          ClRcT retCode)
{
    clOsalPrintf("Response received for type [0x%x], oper [0x%x], retCode [0x%x]\n",type,oper,retCode);
    return CL_OK;
}

static ClAmsMgmtCallbacksT  gAmsMgmtCallbacks = 
{
    clAmsMgmtEntityAdminResponse
};
ClAmsMgmtHandleT  gHandle = CL_HANDLE_INVALID_VALUE;
ClAmsMgmtCCBHandleT gCcbHandle = CL_HANDLE_INVALID_VALUE;
#else

ClAmsMgmtCallbacksT  gAmsMgmtCallbacks = {0};
ClAmsMgmtHandleT  gHandle = CL_HANDLE_INVALID_VALUE;
ClAmsMgmtCCBHandleT gCcbHandle = CL_HANDLE_INVALID_VALUE;
#endif

ClRcT  
clAmsDebugCliMakeEntityStruct(
        CL_OUT  ClAmsEntityT  *entity,
        CL_IN  ClCharT  *entityType,
        CL_IN  ClCharT  *entityName )
{

    AMS_FUNC_ENTER (("\n"));

    if ( !strcasecmp (entityType,"node"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_NODE;
    }

    else if ( !strcasecmp (entityType,"sg"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_SG;
    }

    else if ( !strcasecmp (entityType,"su"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_SU;
    }
        
    else if ( !strcasecmp (entityType,"si"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_SI;
    }

    else if ( !strcasecmp (entityType,"comp"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_COMP;
    }

    else if ( !strcasecmp (entityType,"csi"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_CSI;
    }

    else if ( !strcasecmp (entityType,"entity"))
    {
        entity->type = CL_AMS_ENTITY_TYPE_ENTITY;
    }

    else
    {
        return CL_AMS_RC (CL_AMS_ERR_INVALID_ARGS);
    }

    strcpy ((ClCharT*)entity->name.value,entityName);
    entity->name.length = strlen (entityName) + 1;

    return CL_OK;

}

ClRcT
clAmsDebugCliEntityAlphaFactor(
                               CL_IN  ClUint32T  argc,
                               CL_IN  ClCharT  **argv,
                               CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};
    ClUint32T alphaFactor = 0;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate(MAX_BUFFER_SIZE+1);
    if(!*ret)
    {
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    }

    if(argc != 2 && argc != 3)
    {
        strncpy(*ret, "amsalpha sgname  [ alpha factor]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clAmsDebugCliMakeEntityStruct(
                                       &entity,
                                       "sg",
                                       argv[1]);

    if(rc != CL_OK)
    {
        snprintf(*ret, MAX_BUFFER_SIZE, "entity struct make failed with [%#x]\n", rc);
        return rc;
    }

    if(argc == 3)
    {
        alphaFactor = atoi(argv[2]);

        if ( ( rc = clAmsMgmtEntitySetAlphaFactor(
                                                  gHandle,
                                                  &entity, alphaFactor ))
             != CL_OK)
        {
            snprintf (*ret, MAX_BUFFER_SIZE, 
                      "entity set alpha factor failed with [%#x]\n",
                      rc);
            return rc;
        }
    }
    else
    {
        ClAmsSGConfigT *sgConfig = NULL;
        rc = clAmsMgmtEntityGetConfig(gHandle, &entity, 
                                      (ClAmsEntityConfigT**)&sgConfig);
        if(rc != CL_OK)
        {
            snprintf(*ret, MAX_BUFFER_SIZE, 
                     "entity get config failed with [%#x]\n", rc);
            return rc;
        }
        snprintf(*ret, MAX_BUFFER_SIZE, "%d\n", sgConfig->alpha);
        clAmsFreeMemory(sgConfig);
    }
    return rc;
    
}

ClRcT
clAmsDebugCliEntityBetaFactor(
                               CL_IN  ClUint32T  argc,
                               CL_IN  ClCharT  **argv,
                               CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};
    ClUint32T betaFactor = 0;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate(MAX_BUFFER_SIZE+1);
    if(!*ret)
    {
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    }

    if(argc != 2 && argc != 3)
    {
        strncpy(*ret, "amsbeta sgname  [ beta factor]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clAmsDebugCliMakeEntityStruct(
                                       &entity,
                                       "sg",
                                       argv[1]);

    if(rc != CL_OK)
    {
        snprintf(*ret, MAX_BUFFER_SIZE, "entity struct make failed with [%#x]\n", rc);
        return rc;
    }

    if(argc == 3)
    {
        betaFactor = atoi(argv[2]);

        if ( ( rc = clAmsMgmtEntitySetBetaFactor(
                                                  gHandle,
                                                  &entity, betaFactor ))
             != CL_OK)
        {
            snprintf (*ret, MAX_BUFFER_SIZE, 
                      "entity set beta factor failed with [%#x]\n",
                      rc);
            return rc;
        }
    }
    else
    {
        ClAmsSGConfigT *sgConfig = NULL;
        rc = clAmsMgmtEntityGetConfig(gHandle, &entity, 
                                      (ClAmsEntityConfigT**)&sgConfig);
        if(rc != CL_OK)
        {
            snprintf(*ret, MAX_BUFFER_SIZE, 
                     "entity get config failed with [%#x]\n", rc);
            return rc;
        }
        snprintf(*ret, MAX_BUFFER_SIZE, "%d\n", sgConfig->beta);
        clAmsFreeMemory(sgConfig);
    }
    return rc;
    
}

ClRcT
clAmsDebugCliEntityLockAssignment(
       CL_IN  ClUint32T  argc,
       CL_IN  ClCharT  **argv,
       CL_OUT ClCharT  **ret,
       CL_IN  ClUint32T  retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
                    &entity,
                    argv[1],
                    argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SG
            || entity.type == CL_AMS_ENTITY_TYPE_SU || entity.type == CL_AMS_ENTITY_TYPE_SI) 
            || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node, sg, su, and si\n",retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityLockAssignmentExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[lock assignment] failed\n", (retLen-strlen(*ret)-1));
        return rc;
    }

    return rc;
    
}

ClRcT
clAmsDebugCliEntityLockInstantiation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT ClCharT  **ret,
        CL_IN  ClUint32T  retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
                    &entity,
                    argv[1],
                    argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SG
            || entity.type == CL_AMS_ENTITY_TYPE_SU ) || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node, sg, and su\n", retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityLockInstantiationExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[lock instantiation] failed\n", (retLen-strlen(*ret)-1));
        return rc;
    } 
    
    return rc;

}

ClRcT
clAmsDebugCliEntityUnlock(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
                                       &entity,
                                       argv[1],
                                       argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SG
            || entity.type == CL_AMS_ENTITY_TYPE_SU || entity.type == CL_AMS_ENTITY_TYPE_SI) 
            || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node, sg, su, and si\n", retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityUnlockExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[unlock] failed\n",(retLen-strlen(*ret)-1));
        return rc;
    } 
    
    return rc;

}

ClRcT
clAmsDebugCliEntityShutdown(
        CL_IN  ClUint32T argc,
        CL_IN  ClCharT **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
            &entity,
            argv[1],
            argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SG
            || entity.type == CL_AMS_ENTITY_TYPE_SU || entity.type == CL_AMS_ENTITY_TYPE_SI) 
            || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node, sg, su, and si\n", retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityShutdownExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[shutdown] failed\n", (retLen-strlen(*ret)-1));
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliEntityShutdownWithRestart(
        CL_IN  ClUint32T argc,
        CL_IN  ClCharT **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
            &entity,
            argv[1],
            argv[2]);

    if (entity.type != CL_AMS_ENTITY_TYPE_SU
        ||
        rc != CL_OK)
    {
        strncpy (*ret,"invalid entity type, valid entity types are su\n", retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityShutdownWithRestartExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[shutdown with restart] failed\n", (retLen-strlen(*ret)-1));
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliEntityRestart(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n")); 
    
    rc = clAmsDebugCliMakeEntityStruct(
            &entity,
            argv[1],
            argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SU 
                || entity.type == CL_AMS_ENTITY_TYPE_COMP) 
            || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node, su, and comp\n",retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityRestartExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[restart] failed\n", (retLen-strlen(*ret)-1));
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliEntityRepaired(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  ** ret,
        CL_IN  ClUint32T  retLen)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    rc = clAmsDebugCliMakeEntityStruct(
            &entity,
            argv[1],
            argv[2]);

    if ( !(entity.type == CL_AMS_ENTITY_TYPE_NODE || entity.type == CL_AMS_ENTITY_TYPE_SU)
            || (rc != CL_OK) )
    {
        strncpy (*ret,"invalid entity type, valid entity types are node and su\n",retLen-1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtEntityRepairedExtended(
                    gHandle,
                    &entity,
                    CL_FALSE ))
            != CL_OK)
    {
        strncat (*ret, "admin operation[entity repaired] failed\n",(retLen-strlen(*ret)-1));
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliFinalization( void )
{

    AMS_FUNC_ENTER (("\n"));
    
    if(gClInitialized == CL_FALSE)
    {
        return CL_OK;
    }

    gClInitialized = CL_FALSE;

    AMS_CALL( clAmsMgmtCCBFinalize(gCcbHandle) );

    AMS_CALL( clAmsMgmtFinalize(gHandle) );

    return CL_OK;

}

ClRcT
clAmsDebugCliInitialization( void )
{

    AMS_FUNC_ENTER (("\n"));
    
    if(gClInitialized == CL_TRUE)
    {
        return CL_OK;
    }

    AMS_CALL( clAmsMgmtInitialize(
                &gHandle,
                &gAmsMgmtCallbacks,
                &gVersion) );

    AMS_CALL( clAmsMgmtCCBInitialize(
                gHandle,
                &gCcbHandle) );

    gClInitialized = CL_TRUE;

    return CL_OK;

}

ClRcT   
clAmsDebugCliAdminAPI(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)

{

    ClRcT rc = CL_OK;
    
    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapCalloc (1,MAX_BUFFER_SIZE + 1);

    if ( argc != 3 )
    {
        clAmsDebugCliUsage (*ret, MAX_BUFFER_SIZE + 1);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * Parse the arguments and call the appropriate function
     */

    if ( !strcasecmp (argv[0],"amsLockAssignment") )
    {
        rc = clAmsDebugCliEntityLockAssignment(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsLockInstantiation") )
    {
        rc = clAmsDebugCliEntityLockInstantiation(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsUnlock") )
    {
        rc = clAmsDebugCliEntityUnlock(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsShutdown") )
    {
        rc = clAmsDebugCliEntityShutdown(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsShutdownRestart") )
    {
        rc = clAmsDebugCliEntityShutdownWithRestart(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsRestart") )
    {
        rc = clAmsDebugCliEntityRestart(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    else if ( !strcasecmp (argv[0],"amsRepaired") )
    {
        rc = clAmsDebugCliEntityRepaired(argc,argv,ret,MAX_BUFFER_SIZE + 1);
    }

    return rc;

}

ClRcT clAmsDebugCliSGAdjust(
                            CL_IN ClUint32T argc,
                            CL_IN ClCharT **argv,
                            CL_OUT ClCharT **ret)
{
    ClRcT rc = CL_OK;
    ClBoolT adjust = CL_TRUE;
    AMS_FUNC_ENTER(("\n"));

    if(ret == NULL) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    *ret = clHeapAllocate(MAX_BUFFER_SIZE+1);
    CL_ASSERT(*ret != NULL);

    if(argc != 2 && argc != 3)
    {
        strncat(*ret, "amssgadjust sgname [off]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(argc > 2)
    {
        if(!strncasecmp(argv[2], "off", 3))
        {
            adjust = CL_FALSE;
        }
        else
        {
            strncat(*ret, "amssgadjust sgname [off]\n", MAX_BUFFER_SIZE);
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }
    rc = clAmsMgmtSGAdjustExtended(gHandle, (const ClCharT*)argv[1], adjust, CL_FALSE);
    if(rc != CL_OK)
    {
        strncat(*ret, "admin operation[sg adjust] failed\n", MAX_BUFFER_SIZE);
        return rc;
    }
    return CL_OK;
}

ClRcT clAmsDebugCliSISwap(
                          CL_IN  ClUint32T argc,
                          CL_IN  ClCharT **argv,
                          CL_OUT ClCharT **ret)
{
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER(("\n"));

    if(ret == NULL) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *ret = clHeapAllocate(MAX_BUFFER_SIZE+1);

    CL_ASSERT(*ret != NULL);

    if(argc != 2)
    {
        strncat(*ret, "amssiswap [siname]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtSISwapExtended(
                    gHandle,
                    (const ClCharT*)argv[1],
                    CL_FALSE))
            != CL_OK)
    {
        strncat (*ret, "admin operation[si swap] failed\n", MAX_BUFFER_SIZE);
        return rc;
    }

    return CL_OK;
}

static ClRcT amsForceLockTask(ClPtrT cookie)
{
    ClAmsForceLockContextT *lockContext = (ClAmsForceLockContextT*)cookie;
    ClCharT *respBuffer = lockContext->respBuffer;
    ClBufferHandleT outMsgHandle = lockContext->outMsgHandle;
    ClRmdResponseContextHandleT responseHandle = lockContext->responseHandle;
    ClAmsEntityT entity = {0};
    ClRcT rc;
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    saNameSet(&entity.name, lockContext->entity);
    clHeapFree(lockContext);
    respBuffer[0] = 0; /*zero off the response buffer*/
    rc = clAmsMgmtEntityLockInstantiation(gHandle, (const ClAmsEntityT*)&entity);
    if(rc != CL_OK)
    {
        snprintf(respBuffer, MAX_BUFFER_SIZE, "Force lock instantiation returned with [%#x]", rc);
        clAmsMgmtEntityForceLockExtended(gHandle, (const ClAmsEntityT*)&entity, 0); /* force unlock*/
    }
    rc = clDebugResponseSend(responseHandle, &outMsgHandle, respBuffer, rc);
    clHeapFree(respBuffer);
    return rc;
}

ClRcT clAmsDebugCliForceLock(
                          CL_IN  ClUint32T argc,
                          CL_IN  ClCharT **argv,
                          CL_OUT ClCharT **ret)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};
    ClAmsForceLockContextT *lockContext = NULL;
    ClRmdResponseContextHandleT responseHandle = 0;
    ClBufferHandleT outMsgHandle = 0;
    static ClTaskPoolHandleT pool = 0;

    AMS_FUNC_ENTER(("\n"));

    if(ret == NULL) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *ret = clHeapCalloc(1, MAX_BUFFER_SIZE+1);

    CL_ASSERT(*ret != NULL);
    
    if(argc != 2)
    {
        strncat(*ret, "amsforcelock [suname]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * Defer the context before firing the first RMD since that would overwrite the current
     * rmdrecv threadspecific context.
     */
    lockContext = clHeapCalloc(1, sizeof(*lockContext));
    CL_ASSERT(lockContext != NULL);
    strncpy(lockContext->entity, argv[1], sizeof(lockContext->entity)-1);
    lockContext->respBuffer = *ret;
    rc = clDebugResponseDefer(&lockContext->responseHandle, &lockContext->outMsgHandle);
    if(rc != CL_OK)
    {
        clLogError("ADM", "LOCK-FORCE", "Debug response defer returned with [%#x]", rc);
        strncat(*ret, "Failed to defer force-lock request\n", MAX_BUFFER_SIZE);
        clHeapFree(lockContext);
        return rc;
    }
    responseHandle = lockContext->responseHandle;
    outMsgHandle = lockContext->outMsgHandle;

    entity.type = CL_AMS_ENTITY_TYPE_SU;
    saNameSet(&entity.name, (const ClCharT*)argv[1]);
    
    if ( ( rc = clAmsMgmtEntityForceLockExtended(
                                                 gHandle,
                                                 (const ClAmsEntityT*)&entity,
                                                 1) )
         != CL_OK)
    {
        strncat (*ret, "admin operation[force lock] failed\n", MAX_BUFFER_SIZE);
        clHeapFree(lockContext);
        return clDebugResponseSend(responseHandle, &outMsgHandle, *ret, rc);
    }
    if(!pool)
    {
        clTaskPoolInitialize();
        rc = clTaskPoolCreate(&pool, 1, NULL, NULL);
        if(rc != CL_OK)
        {
            clLogError("ADM", "LOCK-FORCE", "Task pool create returned with [%#x]", rc);
            clAmsMgmtEntityForceLockExtended(gHandle, (const ClAmsEntityT*)&entity, 0);
            strncat(*ret, "admin operation [force lock] failed to create defer task\n", MAX_BUFFER_SIZE);
            clHeapFree(lockContext);
            return clDebugResponseSend(responseHandle, &outMsgHandle, *ret, rc);
        }
    }
    if( (rc = clTaskPoolRun(pool, amsForceLockTask, (void*)lockContext) ) != CL_OK)
    {
        snprintf(*ret, MAX_BUFFER_SIZE, "Debug cli deferred lock taskpool run failed with [%#x]", rc);
        clAmsMgmtEntityForceLockExtended(gHandle, (const ClAmsEntityT*)&entity, 0);
        clHeapFree(lockContext);
        return clDebugResponseSend(responseHandle, &outMsgHandle, *ret, rc);
    }
    *ret = NULL; /* let it be done by the task pool task*/
    return rc;
}

static ClRcT amsForceLockInstantiationTask(ClPtrT cookie)
{
    ClAmsForceLockContextT *lockContext = (ClAmsForceLockContextT*)cookie;
    ClCharT *respBuffer = lockContext->respBuffer;
    ClBufferHandleT outMsgHandle = lockContext->outMsgHandle;
    ClRmdResponseContextHandleT responseHandle = lockContext->responseHandle;
    ClAmsEntityT entity = {0};
    ClRcT rc;
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    saNameSet(&entity.name, lockContext->entity);
    clHeapFree(lockContext);
    respBuffer[0] = 0; /*zero off the response buffer*/
    rc = clAmsMgmtEntityForceLockInstantiation(gHandle, (const ClAmsEntityT*)&entity);
    if(rc != CL_OK)
    {
        snprintf(respBuffer, MAX_BUFFER_SIZE, "Force lock instantiation returned with [%#x]", rc);
    }
    rc = clDebugResponseSend(responseHandle, &outMsgHandle, respBuffer, rc);
    clHeapFree(respBuffer);
    return rc;
}

ClRcT clAmsDebugCliForceLockInstantiation(
                          CL_IN  ClUint32T argc,
                          CL_IN  ClCharT **argv,
                          CL_OUT ClCharT **ret)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};
    ClAmsForceLockContextT *lockContext = NULL;
    ClRmdResponseContextHandleT responseHandle = 0;
    ClBufferHandleT outMsgHandle = 0;
    static ClTaskPoolHandleT pool = 0;

    AMS_FUNC_ENTER(("\n"));

    if(ret == NULL) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *ret = clHeapCalloc(1, MAX_BUFFER_SIZE+1);

    CL_ASSERT(*ret != NULL);
    
    if(argc != 2)
    {
        strncat(*ret, "amsForceLockInstantiation [suname]\n", MAX_BUFFER_SIZE);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * Defer the context before firing the first RMD since that would overwrite the current
     * rmdrecv threadspecific context.
     */
    lockContext = clHeapCalloc(1, sizeof(*lockContext));
    CL_ASSERT(lockContext != NULL);
    lockContext->entity[0] = 0;
    strncat(lockContext->entity, argv[1], sizeof(lockContext->entity)-1);
    lockContext->respBuffer = *ret;
    rc = clDebugResponseDefer(&lockContext->responseHandle, &lockContext->outMsgHandle);
    if(rc != CL_OK)
    {
        clLogError("ADM", "LOCKI-FORCE", "Debug response defer returned with [%#x]", rc);
        strncat(*ret, "Failed to defer force-locki request\n", MAX_BUFFER_SIZE);
        clHeapFree(lockContext);
        return rc;
    }
    responseHandle = lockContext->responseHandle;
    outMsgHandle = lockContext->outMsgHandle;
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    saNameSet(&entity.name, (const ClCharT*)argv[1]);
    if(!pool)
    {
        clTaskPoolInitialize();
        rc = clTaskPoolCreate(&pool, 1, NULL, NULL);
        if(rc != CL_OK)
        {
            clLogError("ADM", "LOCKI-FORCE", "Task pool create returned with [%#x]", rc);
            clAmsMgmtEntityForceLockInstantiationExtended(gHandle, (const ClAmsEntityT*)&entity, CL_FALSE);
            strncat(*ret, "admin operation [force locki] failed to create defer task\n", MAX_BUFFER_SIZE);
            clHeapFree(lockContext);
            return clDebugResponseSend(responseHandle, &outMsgHandle, *ret, rc);
        }
    }
    if( (rc = clTaskPoolRun(pool, amsForceLockInstantiationTask, (void*)lockContext) ) != CL_OK)
    {
        snprintf(*ret, MAX_BUFFER_SIZE, 
                 "Debug cli deferred lock instantiation taskpool run failed with [%#x]", rc);
        clAmsMgmtEntityForceLockInstantiationExtended(gHandle, (const ClAmsEntityT*)&entity, CL_FALSE);
        clHeapFree(lockContext);
        return clDebugResponseSend(responseHandle, &outMsgHandle, *ret, rc);
    }
    *ret = NULL; /* let it be done by the task pool task*/
    return rc;
}

void 
clAmsDebugCliUsage(
        CL_INOUT  ClCharT  *ret,
        CL_IN     ClUint32T retLen)
{

    AMS_FUNC_ENTER (("\n"));

    strncpy(ret,"usage: commandname entity_type entityname \n", retLen-1);
    strncat(ret,"example : amsunlock node nodename\n", (retLen-strlen(ret)-1));
    strncat(ret,"valid entity types are:\n", (retLen-strlen(ret)-1));
    strncat(ret,"amslockassignment    [ node,sg,su,si ]\n", (retLen-strlen(ret)-1));
    strncat(ret,"amslockinstantiation [ node,sg,su    ]\n", (retLen-strlen(ret)-1));
    strncat(ret,"amsunlock            [ node,sg,su,si ]\n", (retLen-strlen(ret)-1));
    strncat(ret,"amsshutdown          [ node,sg,su,si ]\n", (retLen-strlen(ret)-1));
    strncat(ret,"amsrestart           [ node,su,comp  ]\n", (retLen-strlen(ret)-1));
    strncat(ret,"amsrepaired          [ node,su       ]\n", (retLen-strlen(ret)-1));

}

void 
clAmsDebugCliDebugCommandUsage(
        CL_INOUT  ClCharT  *ret,
        CL_IN     ClUint32T retLen,
        CL_IN     ClUint32T debugCommand )
{

    AMS_FUNC_ENTER (("\n"));

    if ( debugCommand == CL_AMS_DEBUG_ENABLE_COMMAND )
    {
        strncpy(ret,"enables debug messages for an ams entity or entire ams\n", retLen-1);
        strncat(ret,"usage: amsdebugenable  entitytype entityname debugflags\n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugenable  node nodename all \n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugenable  all  all \n", (retLen-strlen(ret)-1));
    }

    if ( debugCommand == CL_AMS_DEBUG_DISABLE_COMMAND )
    {
        strncpy(ret,"disables debug messages for an ams entity or entire ams\n", retLen-1);
        strncat(ret,"usage: amsdebugdisable  entitytype entityname debugflags\n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugdisable  node nodename all \n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugdisable  all  all \n", (retLen-strlen(ret)-1));
    }

    if ( debugCommand == CL_AMS_DEBUG_GET_COMMAND )
    {
        strncpy(ret,"returns debug flags for ams entity or entire ams\n", retLen-1);
        strncat(ret,"usage: amsdebugget  entitytype entityname\n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugget  node nodename\n", (retLen-strlen(ret)-1));
        strncat(ret,"amsdebugget  all\n", (retLen-strlen(ret)-1));
    }

    strncat(ret,"valid entitytypes are [ node, sg, su, comp, si, csi ] \n", (retLen-strlen(ret)-1));
    strncat(ret,"valid debug flags are [msg,timer,state_change,function_enter,all]\n", (retLen-strlen(ret)-1));
    strncat(ret,"msg           : debug flag for ams messages \n", (retLen-strlen(ret)-1));
    strncat(ret,"timer         : debug flag for timer related ams messages \n", (retLen-strlen(ret)-1));
    strncat(ret,"state_change  : debug flag for state change related ams messages \n", (retLen-strlen(ret)-1));
    strncat(ret,"function_enter: debug flag for function enter ams messages \n", (retLen-strlen(ret)-1));
    strncat(ret,"all           : debug flag for all ams messages \n", (retLen-strlen(ret)-1));


}

ClRcT   
clAmsDebugCliFaultReport(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;
    SaNameT  compName = {0};
    ClTimeT  errorDetectionTime = 0;
    ClAmsLocalRecoveryT  recommendedRecovery = 0;
    ClUint32T  dummy = 0;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1 );

    if ( argc != 3 )
    { 
        strcpy(*ret," USAGE: commandName componentName/nodeName recommended_recovery \n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    strcpy ((ClCharT*)compName.value,argv[1]);
    compName.length = strlen(argv[1]) +1;
    recommendedRecovery = atoi (argv[2]);

    if ( ( rc = _clAmsSAComponentErrorReport(
                    &compName,
                   errorDetectionTime,
                   recommendedRecovery,
                    dummy, 0))
            != CL_OK )
    {
        strcpy(*ret,"Error in clAmsGetFaultReport function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliNodeJoin(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  ** ret)
{

    ClRcT  rc = CL_OK;
    SaNameT  nodeName = {0};

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1 );

    if ( argc != 2 )
    { 
        strcpy(*ret," USAGE: commandName nodeName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    strcpy ((ClCharT*)nodeName.value,argv[1]);
    nodeName.length = strlen(argv[1]) +1;

    if ( ( rc = _clAmsSANodeJoin(
                    &nodeName ))
            != CL_OK )
    {
        strcpy(*ret,"Error in clAmsPeNodeJoinCluster function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliPGTrackAdd(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;
    SaNameT  csiName = {0};
    ClUint8T  trackFlags = 1;
    ClIocAddressT  iocAddress;
    ClCpmHandleT  cpmHandle = -1;
    ClAmsPGNotificationBufferT  *notificationBuffer = NULL;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1 );

    if ( argc != 4 )
    { 
        strcpy(*ret," USAGE: commandName csiName csiTrackFlags nodeAddress\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    strcpy ((ClCharT*)csiName.value,argv[1]);
    csiName.length = strlen(argv[1]) +1;

    trackFlags = atoi(argv[2]);
    iocAddress.iocPhyAddress.nodeAddress = atoi(argv[3]);
    iocAddress.iocPhyAddress.portId = 0;
        
    if ( ( rc = _clAmsSAPGTrackAdd(
                    iocAddress,
                    cpmHandle,
                    &csiName,
                    trackFlags,
                    notificationBuffer))
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsClientPGTrack function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliPGTrackStop(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;
    SaNameT  csiName = {0};
    ClIocAddressT  iocAddress;
    ClCpmHandleT  cpmHandle = -1;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1 );

    if ( argc != 3 )
    { 
        strcpy(*ret," USAGE: commandName csiName iocAddress\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    strcpy ((ClCharT*)csiName.value,argv[1]);
    csiName.length = strlen(argv[1]) +1;

    iocAddress.iocPhyAddress.nodeAddress = atoi(argv[2]);
    iocAddress.iocPhyAddress.portId = 0;

        
    if ( ( rc = _clAmsSAPGTrackStop(
                    iocAddress,
                    cpmHandle,
                    &csiName ))
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsClientPGTrack function \n");
        return rc;
    }

    return rc;

}


ClRcT   
clAmsDebugCliPrintAmsDB(
        ClUint32T argc,
        ClCharT **argv,
        ClCharT** ret)
{

    ClRcT  rc = CL_OK;
    ClUint32T  size = 0;
    struct stat  buf;
    FILE  *fp = NULL;

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: commandName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    debugPrintFP = fopen (CL_AMS_DEBUG_PRINT_FILE,"w+");

    if ( debugPrintFP == NULL )
    {
        goto error_print;
    }

    if ( ( rc = _clAmsSAPrintDB()) != CL_OK )
    {
        goto error;
    }
    
    fclose(debugPrintFP);
    debugPrintFP = NULL;

    if ( stat (CL_AMS_DEBUG_PRINT_FILE,&buf) != 0 )
    {
        goto error_print;
    }

    size = buf.st_size;


    fp = fopen (CL_AMS_DEBUG_PRINT_FILE,"r");
    if ( fp == NULL )
    {
        goto error_print; 
    }


    clAmsFreeMemory (*ret);
    *ret = clHeapAllocate (size + 1);

    if ( !fread (*ret,1,size,fp))
    {
        fclose(fp);
        goto error_print;
    }

    (*ret)[size] = '\0';
    unlink(CL_AMS_DEBUG_PRINT_FILE);
    fclose(fp);
    return rc;

error: 
    
    fclose(debugPrintFP);
    debugPrintFP = NULL;
    unlink(CL_AMS_DEBUG_PRINT_FILE);

error_print:

    strcpy(*ret,"Error in _clAmsSAPrintDB function \n");
    return rc;
}

ClRcT   
clAmsDebugCliPrintAmsDBXML(ClUint32T argc,
                           ClCharT **argv,
                           ClCharT** ret)
{

    ClRcT  rc = CL_AMS_RC(CL_ERR_LIBRARY);
    extern ClRcT cpmPrintDBXML(FILE *fp);
    ClCharT buf[256]={0};

    *ret = clHeapCalloc(1,1024);

    if(! *ret)
    {
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    }

    if ( argc > 3 ||
         ((argc == 2 &&
           strcmp(argv[1], "console")) &&
          (argc == 2 &&
           strcmp(argv[1], "all"))) ||
         ((argc == 3 &&
           (strcmp(argv[1], "console") ||
            strcmp(argv[2], "all")))))
    { 
        strncpy(*ret," USAGE: commandName [ console ] [all]\n",255);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if (argc == 2 && !strcmp(argv[1], "all"))
    {
        debugPrintAll = CL_YES;
    }
    else if (argc == 3 && !strcmp(argv[2], "all"))
    {
        debugPrintAll = CL_YES;
    }
    else
    {
        debugPrintAll = CL_NO;
    }
    
    debugPrintFP = fopen (CL_AMS_DEBUG_PRINT_XML_FILE, "w+");

    if ( debugPrintFP == NULL )
    {
        goto error_print;
    }
    
    CL_AMS_PRINT_OPEN_TAG("asp");

    if ( ( rc = _clAmsSAPrintDBXML()) != CL_OK )
    {
        CL_AMS_PRINT_CLOSE_TAG("asp");
        goto error;
    }
    
    cpmPrintDBXML(debugPrintFP);

    CL_AMS_PRINT_CLOSE_TAG("asp");
    
    fclose(debugPrintFP);

    if(!getcwd(buf,sizeof(buf))) buf[0] = 0;

    snprintf(*ret,1024,"Success - Output written to [%s/%s]\n",buf,CL_AMS_DEBUG_PRINT_XML_FILE);

    /*
     * Also dump to the console for utilities to utilize the output
     */
    if(argc > 1 && !strncasecmp(argv[1], "console", 7))
    {
        struct stat stat_buf;
        ClInt32T err = 0;
        debugPrintFP = fopen(CL_AMS_DEBUG_PRINT_XML_FILE, "r");
        if(!debugPrintFP)
        {
            snprintf(*ret, 1024, "Console output fopen error [%s]\n", 
                     strerror(errno));
            return CL_AMS_RC(CL_ERR_LIBRARY);
        }
        err = stat(CL_AMS_DEBUG_PRINT_XML_FILE, &stat_buf);
        if(err < 0)
        {
            snprintf(*ret, 1024, "Console output stat error [%s]\n",
                     strerror(errno));
            fclose(debugPrintFP);
            return CL_AMS_RC(CL_ERR_LIBRARY);
        }
        else
        {
            *ret = clHeapRealloc(*ret, (ClUint32T)stat_buf.st_size+1);
            if(!*ret)
            {
                fclose(debugPrintFP);
                return CL_AMS_RC(CL_ERR_NO_MEMORY);
            }
            memset(*ret, 0, stat_buf.st_size + 1);
            if(!fread(*ret, 1, stat_buf.st_size, debugPrintFP))
            {
                snprintf(*ret, stat_buf.st_size, "Console output "\
                         "fread error [%s]\n",
                         strerror(errno));
                fclose(debugPrintFP);
                return CL_AMS_RC(CL_ERR_LIBRARY);
            }
            (*ret)[stat_buf.st_size] = 0;
            fclose(debugPrintFP);
        }
    }

    return rc;

    error: 
    
    fclose(debugPrintFP);
    debugPrintFP = NULL;
    unlink(CL_AMS_DEBUG_PRINT_XML_FILE);

    error_print:

    strcpy(*ret,"Error in _clAmsSAPrintDBXML function \n");
    return rc;
}

ClRcT   
clAmsDebugCliEntityPrint(
        CL_IN  ClUint32T argc,
        CL_IN  ClCharT **argv,
        CL_OUT  ClCharT** ret)
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};
    ClUint32T  size = 0;
    struct stat  buf;
    FILE  *fp = NULL;

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 3 )
    { 

        strcpy(*ret," USAGE: amsEntityPrint entityType entityName \n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    }

    rc = clAmsDebugCliMakeEntityStruct(
                    &entity,
                    argv[1],
                    argv[2]);

    if ( rc != CL_OK )
    {
        strcpy (*ret,"invalid entity type, valid entity types are node, sg, su,\
                si, comp and csi\n");
        return rc;
    }

    debugPrintFP = fopen (CL_AMS_DEBUG_PRINT_FILE,"w+");

    if ( debugPrintFP == NULL )
    {
        goto error_print;
    }


    if ( (rc = _clAmsSAEntityPrint(&entity))!= CL_OK )
    {
        goto error;
    }

    fclose(debugPrintFP);
    debugPrintFP = NULL;

    if ( stat (CL_AMS_DEBUG_PRINT_FILE,&buf) != 0 )
    {
        goto error_print;
    }

    size = buf.st_size;

    fp = fopen (CL_AMS_DEBUG_PRINT_FILE,"r");
    if ( fp == NULL )
    {
        goto error_print; 
    }

    clAmsFreeMemory (*ret);
    *ret = clHeapAllocate (size + 1);

    if ( !fread (*ret,1,size,fp))
    {
        fclose(fp);
        goto error_print;
    }

    (*ret)[size] = '\0';
    unlink(CL_AMS_DEBUG_PRINT_FILE);
    fclose(fp);
    return rc;

error: 
    
    fclose(debugPrintFP);
    debugPrintFP = NULL;
    unlink(CL_AMS_DEBUG_PRINT_FILE);

error_print:

    strcpy(*ret,"Error in _clAmsSAEntityPrint function \n");
    return rc;
}

ClRcT   
clAmsDebugCliXMLizeDB(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: commandName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSAXMLizeDB()) 
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsSAXMLizeDB function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliXMLizeInvocation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: commandName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSAXMLizeInvocation()) 
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsSAXMLizeInvocation function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliDeXMLizeDB(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: commandName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSADeXMLizeDB()) 
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsSADeXMLizeDB function \n");
        return rc;
    }

    return rc;

}

ClRcT   
clAmsDebugCliDeXMLizeInvocation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: commandName\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSADeXMLizeInvocation()) 
            != CL_OK )
    {
        strcpy(*ret,"Error in _clAmsSADeXMLizeInvocation function \n");
        return rc;
    }

    return rc;

}


ClRcT   
clAmsDebugCliSCStateChange(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret)
{

    ClRcT  rc = CL_OK;
    ClUint32T  mode = 0;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 2 )
    { 
        strcpy(*ret," USAGE: amsStateChange mode\n");
        strcat(*ret," Valid modes are ActiveToStandby[1], StandbyToActive[2] Reset[3]\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    mode = atoi (argv[1]);

    if ( mode == 1 )
    {
        mode = CL_AMS_STATE_CHANGE_ACTIVE_TO_STANDBY; 
    }

    else if ( mode == 2)
    {
        mode = CL_AMS_STATE_CHANGE_STANDBY_TO_ACTIVE|CL_AMS_STATE_CHANGE_USE_CHECKPOINT; 
    }

    else if ( mode == 3 )
    {
        mode = CL_AMS_STATE_CHANGE_RESET; 
    }

    else
    {
        strcpy(*ret," USAGE: amsStateChange mode\n");
        strcat(*ret," Valid modes are ActiveToStandby[1], StandbyToActive[2] Reset[3]\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSAAmsStateChange(mode)) 
            != CL_OK )
    {
        strcpy(*ret,"operation [state-change] failed\n");
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliEntityDebugEnable(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};
    ClUint8T  subAreaFlags = 0; 
    ClBoolT  flag = CL_FALSE; 
    
    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 4 )
    {
        if ( argc == 3 )
        {
            if ( !strcasecmp (argv[1],"all"))
            {
                flag = CL_TRUE;
            }
        }
        if ( !flag )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_ENABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if ( flag )
    {
        entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
        if ( (rc = clAmsDebugCliParseDebugFlagsStr(
                        argv[2],
                        &subAreaFlags ))
                != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_ENABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }
    else
    {
        if ( ( rc = clAmsDebugCliMakeEntityStruct(
                        &entity,
                        argv[1],
                        argv[2])) != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_ENABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
        if ( (rc = clAmsDebugCliParseDebugFlagsStr(
                        argv[3],
                        &subAreaFlags ))
                != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_ENABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if ( ( rc = clAmsMgmtDebugEnable(
                    gHandle,
                    &entity,
                   subAreaFlags))
            != CL_OK)
    {
        strcat (*ret, "operation [debug-enable] failed\n");
        return rc;
    }

    return rc;

}


ClRcT
clAmsDebugCliEntityDebugDisable(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    ClBoolT  flag = CL_FALSE;
    ClAmsEntityT  entity = {0};
    ClUint8T  subAreaFlags = 0;

    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);
    
    if ( argc != 4 )
    {
        if ( argc == 3 )
        {
            if ( !strcasecmp (argv[1],"all"))
            {
                flag = CL_TRUE;
            }
        }
        if ( !flag )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_DISABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }


    if ( flag )
    {
        entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
        if ( (rc = clAmsDebugCliParseDebugFlagsStr(
                        argv[2],
                        &subAreaFlags ))
                != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_DISABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }
    else
    {
        if ( ( rc = clAmsDebugCliMakeEntityStruct(
                        &entity,
                        argv[1],
                        argv[2])) != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_DISABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
        if ( (rc = clAmsDebugCliParseDebugFlagsStr(
                        argv[3],
                        &subAreaFlags ))
                != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_DISABLE_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if ( ( rc = clAmsMgmtDebugDisable(
                    gHandle,
                    &entity,
                   subAreaFlags))
            != CL_OK)
    {
        strcat (*ret, "operation [debug-disable] failed\n");
        return rc;
    } 

    return rc;

}

ClRcT
clAmsDebugCliEntityDebugGet(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    ClUint8T  subAreaFlags = 0;
    ClAmsEntityT  entity = {0};
    ClBoolT  flag = CL_FALSE;     
    ClCharT  *debugFlagsStr = NULL;
        
    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 3 )
    {
        if ( argc == 2 )
        {
            if ( !strcasecmp (argv[1],"all"))
            {
                flag = CL_TRUE;
            }
        }
        if ( !flag )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_GET_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if ( flag )
    {
        entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
    }
    else
    {
        if ( ( rc = clAmsDebugCliMakeEntityStruct(
                        &entity,
                        argv[1],
                        argv[2])) != CL_OK )
        {
            clAmsDebugCliDebugCommandUsage(*ret,MAX_BUFFER_SIZE + 1,CL_AMS_DEBUG_GET_COMMAND );
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if ( ( rc = clAmsMgmtDebugGet(
                    gHandle,
                    &entity,
                    &subAreaFlags))
            != CL_OK)
    {

        strcat (*ret, "operation [debug-get] failed\n");
        return rc;
    }

    AMS_CALL( clAmsDebugCliParseDebugFlags(
                &debugFlagsStr,
                subAreaFlags) );

    if ( flag )
    {
        sprintf ( *ret , "debug flags for ams are [%s]\n", debugFlagsStr);
    }

    else
    { 
        sprintf ( *ret , "debug flags for entity[%s] are [%s]\n", argv[2],debugFlagsStr);
    }

    clHeapFree (debugFlagsStr);

    return rc;

}

ClRcT
clAmsDebugCliParseDebugFlagsStr(
        CL_IN  ClCharT  *debugFlagsStr,
        CL_OUT  ClUint8T  *debugFlags )
{

    ClCharT  *ptr = NULL; 
    ClCharT  *strtokPtr = NULL; 

    AMS_FUNC_ENTER (("\n"));

    *debugFlags = 0;

    ptr = strtok_r (debugFlagsStr,"|",&strtokPtr);

    AMS_CHECKPTR ( !ptr );

    while ( ptr)
    {

        if ( !strcasecmp (ptr,"msg"))
        {
            *debugFlags|= CL_AMS_MGMT_SUB_AREA_MSG;
        }

        else if (!strcasecmp (ptr,"timer"))
        {
            *debugFlags|= CL_AMS_MGMT_SUB_AREA_TIMER;
        }

        else if (!strcasecmp (ptr,"state_change"))
        {
            *debugFlags|= CL_AMS_MGMT_SUB_AREA_STATE_CHANGE;
        }

        else if (!strcasecmp (ptr,"function_enter"))
        {
            *debugFlags|= CL_AMS_MGMT_SUB_AREA_FN_CALL;
        }

        else if (!strcasecmp (ptr,"all"))
        {
            *debugFlags |= CL_AMS_MGMT_SUB_AREA_MSG|CL_AMS_MGMT_SUB_AREA_TIMER
                |CL_AMS_MGMT_SUB_AREA_STATE_CHANGE|CL_AMS_MGMT_SUB_AREA_FN_CALL;
        }

        else
        {
            return CL_AMS_ERR_INVALID_ARGS;
        }

        ptr = strtok_r (strtokPtr,"|",&strtokPtr);

    }

    return CL_OK;

}

ClRcT
clAmsDebugCliParseDebugFlags(
        CL_OUT  ClCharT  **debugFlagsStr,
        CL_IN  ClUint8T  debugFlags )
{

    ClBoolT  delimiter = CL_FALSE;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !debugFlagsStr );

    *debugFlagsStr = clHeapAllocate (MAX_BUFFER_SIZE);

    memset ( *debugFlagsStr,0, MAX_BUFFER_SIZE);

    strcpy (*debugFlagsStr,"");

    if ( debugFlags&CL_AMS_MGMT_SUB_AREA_MSG )
    {
        strcat (*debugFlagsStr,"msg");
        delimiter = CL_TRUE;
    }

    if ( debugFlags&CL_AMS_MGMT_SUB_AREA_TIMER )
    {

        if (delimiter)
        {
            strcat (*debugFlagsStr,"|");
        }

        strcat (*debugFlagsStr,"timer");
        delimiter = CL_TRUE;

    }

    if ( debugFlags&CL_AMS_MGMT_SUB_AREA_STATE_CHANGE )
    {

        if (delimiter)
        {
            strcat (*debugFlagsStr,"|");
        }

        strcat (*debugFlagsStr,"state_change");
        delimiter = CL_TRUE;

    }

    if ( debugFlags&CL_AMS_MGMT_SUB_AREA_FN_CALL )
    {
        if (delimiter)
        {
            strcat (*debugFlagsStr,"|");
        }

        strcat (*debugFlagsStr,"function_enter");
        delimiter = CL_TRUE;

    }

    return CL_OK;

}

ClRcT   
clAmsDebugCliEventTest(
        ClUint32T argc,
        ClCharT **argv,
        ClCharT** ret)
{
    ClRcT   rc = CL_OK;

    *ret = clHeapAllocate(MAX_BUFFER_SIZE + 1);
    if ( argc != 1 )
    { 
        strcpy(*ret," USAGE: amsEventTest\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = _clAmsSAEventServerTest () ) 
            != CL_OK )
    {
        strcpy(*ret,"operation [event-test] failed\n");
        return rc;
    }

    return rc;
}

ClRcT
clAmsDebugCliEnableLogToConsole(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    
    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    {
        strcpy ( *ret, " USAGE: amsDebugEnableLogToConsole \n" );
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtDebugEnableLogToConsole( gHandle ) ) != CL_OK )
    {
        strcat (*ret, "operation[debug_enable_log_to_console] failed\n");
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliDisableLogToConsole(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret )
{

    ClRcT  rc = CL_OK;
    
    AMS_FUNC_ENTER (("\n"));

    *ret = clHeapAllocate (MAX_BUFFER_SIZE + 1);

    if ( argc != 1 )
    {
        strcpy ( *ret, " USAGE: amsDebugDisableLogToConsole \n" );
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ( ( rc = clAmsMgmtDebugDisableLogToConsole( gHandle ) ) != CL_OK )
    {
        strcat (*ret, "operation[debug_disable_log_to_console] failed\n");
        return rc;
    }

    return rc;

}

ClRcT
clAmsDebugCliEntityTrigger(
                           CL_IN  ClUint32T  argc,
                           CL_IN  ClCharT  **argv,
                           CL_OUT  ClCharT  **ret )
{
    ClAmsEntityT entity = {0};
    ClMetricT metric = {0};
    ClMetricIdT id = CL_METRIC_ALL;
    ClBoolT doReset = CL_FALSE;

    *ret = clHeapAllocate(MAX_BUFFER_SIZE+1);
    if(!*ret)
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    
    /*
     * Simple case. Fire all triggers for all thresholds.
     */
    if(argc == 1)
    {
        return clAmsEntityTriggerLoadTriggerAll(id);
    }

    if(argc == 2)
    {
        if(!strncasecmp(argv[1], "cpu", 3))
        {
            return clAmsEntityTriggerLoadTriggerAll(CL_METRIC_CPU);
        }
        else if(!strncasecmp(argv[1], "mem", 3))
        {
            return clAmsEntityTriggerLoadTriggerAll(CL_METRIC_MEM);
        }
        else if(!strncasecmp(argv[1], "all", 3))
        {
            return clAmsEntityTriggerLoadTriggerAll(CL_METRIC_ALL);
        }
        else if(!strncasecmp(argv[1], "reset", 5))
        {
            return clAmsEntityTriggerLoadDefaultAll(CL_METRIC_ALL);
        }
        else if(!strncasecmp(argv[1], "recovery", 8))
        {
            return clAmsEntityTriggerRecoveryResetAll(CL_METRIC_ALL);
        }
        else
        {
            snprintf(*ret, MAX_BUFFER_SIZE, "Invalid arg  [%s]\n", argv[1]);
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    if(argc > 4 )
    {
        snprintf(*ret, MAX_BUFFER_SIZE, "Invalid arg count. Should be less or equal to 4\n");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if(!strncasecmp(argv[1], "cpu", 3)
       ||
       !strncasecmp(argv[1], "mem", 3)
       ||
       !strncasecmp(argv[1], "all", 3))
    {

        if(!strncasecmp(argv[1], "cpu", 3)) id = CL_METRIC_CPU;
        if(!strncasecmp(argv[1], "mem", 3)) id = CL_METRIC_MEM;
        if(!strncasecmp(argv[1], "all", 3)) id = CL_METRIC_ALL;

        /*
         * Check if the third arg. is integer complete
         */
        ClCharT *pStr = argv[2];
        while(*pStr && isdigit(*pStr)) ++pStr;
        if(*pStr)
        {
            /*
             * Entity name for the trigger or a reset.
             */
            if(!strncasecmp(argv[2], "reset", 5))
            {
                if(argc == 3)
                {
                    return clAmsEntityTriggerLoadDefaultAll(id);
                }
                doReset = CL_TRUE;
                goto reset_entity;
            }
            else 
            {
                if(argc > 3)
                {
                    snprintf(*ret, MAX_BUFFER_SIZE, "Invalid arguments. Expected 3\n");
                    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                }
                memset(entity.name.value, 0, sizeof(entity.name.value));
                strncpy((ClCharT*)entity.name.value, argv[2], sizeof(entity.name.value)-1);
                entity.name.length = strlen((const ClCharT*)entity.name.value)+1;
                return clAmsEntityTriggerLoadTrigger(&entity, id);
            }
        }
        /*
         * We are here when the 3rd arg is an integer complete.
         */
        metric.id = id;
        metric.currentThreshold = atoi(argv[2]);
        if(metric.currentThreshold > 100)
        {
            metric.currentThreshold = 100;
        }
        if(argc == 3)
        {
            return clAmsEntityTriggerLoadAll(&metric);
        }

        reset_entity:

        memset(entity.name.value, 0, sizeof(entity.name.value));
        strncpy((ClCharT*)entity.name.value, argv[3], sizeof(entity.name.value)-1);
        entity.name.length = strlen((const ClCharT*)entity.name.value)+1;

        if(doReset == CL_TRUE)
        {
            return clAmsEntityTriggerLoadDefault(&entity, id);
        }
        return clAmsEntityTriggerLoad(&entity, &metric);
    }

    if(!strncasecmp(argv[1], "recovery", 8))
    {
        ClCharT *pEntity = NULL;
        if(!strncasecmp(argv[2], "cpu", 3)) id = CL_METRIC_CPU;
        else if(!strncasecmp(argv[2], "mem", 3)) id = CL_METRIC_MEM;
        else if(!strncasecmp(argv[2], "all", 3)) id = CL_METRIC_ALL;
        else
        {
            if(argc > 3)
            {
                snprintf(*ret, MAX_BUFFER_SIZE, 
                         "Invalid arg count. Expected 3\n");
                return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            }
            id = CL_METRIC_ALL;
            pEntity = argv[2];
            goto entity_recovery;
        }
        if(argc == 3)
        {
            return clAmsEntityTriggerRecoveryResetAll(id);
        }
        pEntity = argv[3];

        entity_recovery:
        memset(entity.name.value, 0, sizeof(entity.name.value));
        strncpy((ClCharT*)entity.name.value, pEntity, sizeof(entity.name.value)-1);
        entity.name.length = strlen((const ClCharT*)entity.name.value)+1;
        return clAmsEntityTriggerRecoveryReset(&entity, id);
    }

    snprintf(*ret, MAX_BUFFER_SIZE, "Invalid argument [%s]", argv[1]);
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}
