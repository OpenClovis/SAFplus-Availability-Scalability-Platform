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
 * ModuleName  : debug
 * File        : clDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains DebugCli related APIs 
 *****************************************************************************/
#define __SERVER__
#include <string.h>
#include <stdarg.h>

#include <clHandleApi.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clIocIpi.h>
#include <clRmdIpi.h>
#include <ipi/clHandleIpi.h>
#include <clEoApi.h>
#include <clEoIpi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clXdrApi.h>
#include <clVersionApi.h>
#include "clDebug.h"
#include "clDebugLog.h"
#include <clDebugRmd.h>


#define CL_DEBUG_MAX_PRINT 512

/*
 * this particular return code is not error, but this is required for multiple 
 * registrations, as this is not error code, not keeping in clDebugErrors.h
 */
#define CL_DBG_INFO_CMD_FOUND    100 

static ClRcT VDECL(clDebugInvoke)(ClEoDataT data,
                                  ClBufferHandleT inMsgHdl,
                                  ClBufferHandleT outMsgHdl);

static ClRcT VDECL(clDebugGetContext)(ClEoDataT data,
                                      ClBufferHandleT inMsgHdl,
                                      ClBufferHandleT outMsgHdl);

/*
 * Its possible to include the same guy twice with SERVER/CLIENT
 * and remove the idempotence behaviour in the dependent header files.
 * But lets take a cleaner approach by splitting the client to another file.
 */
#include "clDebugCliFuncTable.h"

static ClVersionT clVersionSupported[]={
{'B',0x01 , 0x01}
};
/*Version Database*/
static ClVersionDatabaseT versionDatabase={
sizeof(clVersionSupported)/sizeof(ClVersionT),
clVersionSupported
}; 

typedef struct ClDebugDeferContext
{
    ClBufferHandleT outMsgHdl;
    ClBoolT defer;
} ClDebugDeferContextT;


#ifdef SOLARIS_BUILD
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>

/* 
 * dump stack trace up to length count into buffer 
 */

int backtrace(void **buffer, int count)
{
    /* Not yet implemented */

    return 0;
}
char **backtrace_symbols(void *const *array, int size)
{
    char **buf;
    size_t buflen = size * sizeof(char *);

    /* Not yet implemented */
    /* Allocating memory so that the caller doesn't crash */

    buf = (char **)malloc(buflen);
    memset(buf, 0, buflen);
    return buf;
}

void backtrace_symbols_fd(void *const *array, int size, int fd) {
    /* Workaround implemented */
    printstack(fd);
}

#endif /* SOLARIS_BUILD */

#define DEBUG_CHECK(x) \
        resp = (ClCharT*)clHeapAllocate(strlen(x) + 1);\
        if (NULL != resp) \
        {\
            strcpy(resp, x);\
        }\

ClRcT
clDebugFuncInvokeCallback(ClHandleDatabaseHandleT  hHandleDB,
                          ClHandleT                handle,
                          ClPtrT                   pData)
{
    ClRcT                 rc          = CL_OK;
    ClDebugFuncGroupT     *pFuncGroup = NULL;
    ClUint32T             i           = 0;
    ClDebugInvokeCookieT  *pCookie    = (ClDebugInvokeCookieT *) pData;

    rc = clHandleCheckout(hHandleDB, handle, (void **) &pFuncGroup); 
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clHandleCheckout(): rc[0x %x]", rc);
        return rc;
    }

    for (i = 0; i < pFuncGroup->numFunc; i++)
    {
        if (!strcasecmp(pFuncGroup->pFuncDescList[i].funcName,
                        pCookie->pCommandName))
        {
            pCookie->pFuncEntry = &(pFuncGroup->pFuncDescList[i]);
            clHandleCheckin(hHandleDB, handle);
            /*
             * Specifically returning NON CL_OK, to quit the container walk as 
             * already found proper function name 
             */
            return CL_DBG_INFO_CMD_FOUND;
        }
    }
    clHandleCheckin(hHandleDB, handle);

    /* 
     * If you are returning non CL_OK, container walk will not proceed.
     * this will break for mutiple registrations of debug commands.
     */
    return rc; 
}

ClRcT clDebugResponseDefer(ClRmdResponseContextHandleT *pResponseHandle, ClBufferHandleT *pOutMsgHandle)
{
    ClDebugDeferContextT *deferContext = NULL;
    ClRcT rc;
    ClEoExecutionObjT *pEoObj = NULL;
    ClDebugObjT *pDebugObj = NULL;

    if(!pResponseHandle)
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clEoPrivateDataGet( pEoObj, CL_EO_DEBUG_OBJECT_COOKIE_ID,(void**) &pDebugObj);
    if (CL_OK != rc)
    {
        return rc;
    }
    
    rc = clOsalTaskDataGet(pDebugObj->debugTaskKey, (ClOsalTaskDataT*)&deferContext);
    if(rc != CL_OK || !deferContext)
    {
        clLogError("DEFER", "CLI", "Task data get failed on the debug defer context");
        return rc;
    }
    
    rc = clRmdResponseDefer(pResponseHandle);
    if(rc != CL_OK)
    {
        return rc;
    }
    deferContext->defer = CL_TRUE;
    if(pOutMsgHandle)
        *pOutMsgHandle = deferContext->outMsgHdl;

    return rc;
}

static ClRcT clDebugResponseMarshall(ClCharT *resp, ClRcT retCode, ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    if(resp && outMsgHdl)
    {
        ClUint32T i = strlen(resp);
        rc = clXdrMarshallClUint32T(&i,outMsgHdl,0);
        if (CL_OK != rc)
        {
            return rc;
        }
        rc = clXdrMarshallArrayClCharT(resp,i,outMsgHdl,0);  
        if (CL_OK != rc)
        {
            return rc;
        }
        rc = clXdrMarshallClUint32T(&retCode, outMsgHdl, 0);
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    return rc;
}

ClRcT clDebugResponseSend(ClRmdResponseContextHandleT responseHandle,
                          ClBufferHandleT *pOutMsgHandle,
                          ClCharT *respBuffer,
                          ClRcT retCode)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT outMsgHandle = 0;
    if(!responseHandle)
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);

    if(pOutMsgHandle)
    {
        outMsgHandle = *pOutMsgHandle;
        *pOutMsgHandle = 0;
    }
    rc = clDebugResponseMarshall(respBuffer, retCode, outMsgHandle);
    if(rc != CL_OK)
    {
        if(outMsgHandle)
        {
            if(clBufferDelete(&outMsgHandle) != CL_OK)
                clHeapFree((void*)outMsgHandle);
        }
        return rc;
    }
    rc = clRmdSyncResponseSend(responseHandle, outMsgHandle, retCode);
    return rc;
}

ClRcT VDECL(clDebugInvoke)(ClEoDataT        data,
                           ClBufferHandleT  inMsgHdl,
                           ClBufferHandleT  outMsgHdl)
{
    ClRcT 	              rc           = CL_OK;
    ClUint32T 	          i            = 0;
    ClUint32T 	          argc         = 0;
    ClCharT	              *argv[MAX_ARGS];
    ClCharT        	      argBuf[(MAX_ARG_BUF_LEN+1) * MAX_ARGS]; /* +1 for NULL termination */
    ClDebugObjT	          *pDebugObj   = (ClDebugObjT*) data;
    ClCharT   	          *resp        = NULL;
    ClVersionT            version      = {0};
    ClDebugInvokeCookieT  invokeCookie = {0};
    ClDebugDeferContextT  deferContext = {0};
    ClRcT                 retCode      = CL_OK;

    if ((NULL == pDebugObj) || (0 == outMsgHdl))
    {
        clLogWrite( CL_LOG_HANDLE_APP,CL_LOG_SEV_CRITICAL,CL_DEBUG_LIB_CLIENT,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clXdrUnmarshallClVersionT(inMsgHdl,&version);
    if (CL_OK != rc)
    {
        return rc;
    }
    rc = clVersionVerify(&versionDatabase,&version);
    if (CL_OK != rc)
    {
        clXdrMarshallClVersionT(&version,outMsgHdl,0);
        return rc;
    }
    rc = clXdrUnmarshallClUint32T(inMsgHdl, &argc);
    if (CL_OK != rc)
    {
        return rc;
    }

    if (argc > MAX_ARGS)
    {
        DEBUG_CHECK("\r\n too many arguments");
        goto L1;
    }

    for (i = 0; i < argc; i++)
    {
        ClUint32T stringLength = 0;

        argv[i] = &argBuf[i * (MAX_ARG_BUF_LEN+1)];

        rc = clXdrUnmarshallClUint32T(inMsgHdl,&stringLength);
        if (CL_OK != rc)
        {
            return rc;
        }

        if (stringLength > MAX_ARG_BUF_LEN)
        {
            DEBUG_CHECK("\r\n argument too big");
            goto L1;
        }

        rc = clXdrUnmarshallArrayClCharT(inMsgHdl,argv[i],stringLength);
        if (CL_OK != rc)
        {
            return rc;
        }
        argv[i][stringLength] = '\0';
    }

    if (!strncasecmp(argv[0], "help", 4) && (argc > 1))
    {
        argv[0] = argv[1];
        argc = 0;
    }
    
    invokeCookie.pCommandName = argv[0];
    invokeCookie.pFuncEntry   = NULL;

    rc = clHandleWalk(pDebugObj->hDebugFnDB, clDebugFuncInvokeCallback, 
                      (void *) &invokeCookie);
    if( CL_DBG_INFO_CMD_FOUND == rc )
    {
        /* this is the indication for command found */
        rc = CL_OK;
    }
    if( CL_OK != rc )
    {
        DEBUG_CHECK("\r\ncommand not found");
        goto L1;
    }

    if (invokeCookie.pFuncEntry && invokeCookie.pFuncEntry->fpCallback)
    {
        deferContext.outMsgHdl = outMsgHdl;
        deferContext.defer = CL_FALSE;
        clOsalTaskDataSet(pDebugObj->debugTaskKey, (ClOsalTaskDataT)&deferContext);
        rc = invokeCookie.pFuncEntry->fpCallback(argc, argv, &resp);
    }
    else
    {
        DEBUG_CHECK("\r\ncommand not found");
    }

L1: 
    retCode = rc;
    rc      = CL_OK;
    if (!resp)
    {
        resp = (ClCharT*) clHeapAllocate(1);
        if (resp) resp[0]= '\0';
    }
    
    if (NULL != resp)
    {
        /*
         * Marshall the response to the out buffer if we aren't deferred.
         */
        if(!deferContext.defer)
            rc = clDebugResponseMarshall(resp, retCode, outMsgHdl);
        clHeapFree(resp);
        resp = NULL;
    }
    return rc;
}

ClRcT
clDebugContexDetailsPack(ClHandleDatabaseHandleT  hHandleDB,
                         ClHandleT                handle,
                         ClPtrT                   pData)
{
    ClRcT              rc          = CL_OK;
    ClBufferHandleT    outMsgHdl   = (ClBufferHandleT) pData;
    ClDebugFuncGroupT  *pFuncGroup = NULL;
    ClUint32T          i           = 0;

    rc = clHandleCheckout(hHandleDB, handle, (void **) &pFuncGroup); 
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"clHandleCheckout(): rc[0x %x]", rc);
        return rc;
    }

    for (i = 0; i < pFuncGroup->numFunc; i++)
    {
        rc = clXdrMarshallArrayClCharT(pFuncGroup->pFuncDescList[i].funcName,
                                       CL_DEBUG_FUNC_NAME_LEN,
                                       outMsgHdl,0);
        if (CL_OK != rc)
        {
            clHandleCheckin(hHandleDB, handle);
            return rc;
        }

        rc = clXdrMarshallArrayClCharT(pFuncGroup->pFuncDescList[i].funcHelp,
                                       CL_DEBUG_FUNC_HELP_LEN,
                                       outMsgHdl,0);
        
        if (CL_OK != rc)
        {
            clHandleCheckin(hHandleDB, handle);
            return rc;
        }
    }

    clHandleCheckin(hHandleDB, handle);

    return rc;
}

ClRcT VDECL(clDebugGetContext)(ClEoDataT        data,
                               ClBufferHandleT  inMsgHdl,
                               ClBufferHandleT  outMsgHdl)
{
    ClRcT        rc         = CL_OK;
    ClDebugObjT  *pDebugObj = (ClDebugObjT *) data;
    ClVersionT   version    = {0};
    ClIocPhysicalAddressT srcAddr = {0};

    if ((NULL == pDebugObj) || (0 == outMsgHdl) || (0 == inMsgHdl))
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_WARNING,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_1_INVALID_PARAMETER,"Invalid debugObj");
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * Enable the comp status for the debug client to avoid
     * response failures from node representative in case the bit isnt enabled for cases when
     * the comp arrival from peer noderep. reaches late.
     */
    if(clRmdSourceAddressGet(&srcAddr) == CL_OK)
        clIocCompStatusEnable(srcAddr);
    
    rc = clXdrUnmarshallClVersionT(inMsgHdl,&version);
    if (CL_OK != rc)
    {
        return rc;
    }
    rc = clVersionVerify(&versionDatabase,&version);
    if (CL_OK != rc)
    {
        clXdrMarshallClVersionT(&version,outMsgHdl,0);
        return rc;
    }

    rc = clXdrMarshallArrayClCharT(pDebugObj->compName,
                                   CL_DEBUG_COMP_NAME_LEN,
                                   outMsgHdl,0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallArrayClCharT(pDebugObj->compPrompt,
                                   CL_DEBUG_COMP_PROMPT_LEN,
                                   outMsgHdl,0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClUint32T((&pDebugObj->numFunc),
                                outMsgHdl,0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clHandleWalk(pDebugObj->hDebugFnDB, clDebugContexDetailsPack,
                      (ClPtrT) outMsgHdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    return rc;
}

ClRcT clDebugLibInitialize(void)
{
    ClEoExecutionObjT  *pEoObj     = NULL;
    ClDebugObjT        *pDebugObj  = NULL;
    ClRcT              rc          = CL_OK;
    SaNameT            compName    = {0};
    
    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        return rc;
    }

    pDebugObj = (ClDebugObjT*) clHeapCalloc(1, sizeof(ClDebugObjT));
    if (NULL == pDebugObj)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_CRITICAL,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clOsalTaskKeyCreate(&pDebugObj->debugTaskKey, NULL);
    if(rc != CL_OK)
    {
        clLogError("DBG","TSK","Debug task key create returned with [%#x]\n", rc);
        clHeapFree(pDebugObj);
        return CL_DEBUG_RC(CL_GET_ERROR_CODE(rc));
    }

    rc = clHandleDatabaseCreate(NULL, &pDebugObj->hDebugFnDB);
    if( CL_OK != rc )
    {
        clLogError("DBG","DBC","clHandleDatabaseCreate(): rc[0x %x]",
                rc);
        clHeapFree(pDebugObj);
        return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
    }

    /* Getting the compName from CPM */
    rc = clCpmComponentNameGet(0, &compName);
    if( CL_OK != rc )
    {
        clLogError("DBG","INI","clCpmComponentNameGet(): rc[0x %x]",
                    rc);
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_WARNING,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_1_INVALID_PARAMETER, 
                   "CompNameGet is not proper");
        return rc;
    }

    pDebugObj->numFunc       = 0;
    /* Assining the compName */
    memset(pDebugObj->compName, '\0', CL_DEBUG_COMP_NAME_LEN);
    if( compName.length < CL_DEBUG_COMP_NAME_LEN )
    {
        memcpy(pDebugObj->compName, compName.value, compName.length);
        pDebugObj->compName[compName.length] = '\0';
    }
    else
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_WARNING,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_1_INVALID_PARAMETER,"CompName is Invalid");
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    }

    /* Assigning compPrompt to DEFAULT */
    memset(pDebugObj->compPrompt, '\0', CL_DEBUG_COMP_PROMPT_LEN);
    strcpy(pDebugObj->compPrompt, "DEFAULT");

    rc = clEoClientInstallTablesWithCookie( pEoObj, 
                                            CL_EO_SERVER_SYM_MOD(gAspFuncTable, DEBUGCli),
                                            pDebugObj);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"clEoClientInstall(): rc[0x %x]", rc);
        clHandleDatabaseDestroy(pDebugObj->hDebugFnDB);
        clHeapFree(pDebugObj);
        return rc;
    }
    rc = clDebugClientTableRegister(pEoObj);
    if(CL_OK != rc)
    {
        clEoClientUninstallTables(pEoObj, 
                                  CL_EO_SERVER_SYM_MOD(gAspFuncTable, DEBUGCli));
        clHandleDatabaseDestroy(pDebugObj->hDebugFnDB);
        clHeapFree(pDebugObj);
        return rc;
    }

    rc = clEoPrivateDataSet(pEoObj, CL_EO_DEBUG_OBJECT_COOKIE_ID, pDebugObj);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"clEoPrivateDataSet(): rc[0x %x]", rc);
        clEoClientUninstallTables(pEoObj, 
                                  CL_EO_SERVER_SYM_MOD(gAspFuncTable, DEBUGCli));
        clHandleDatabaseDestroy(pDebugObj->hDebugFnDB);
        clHeapFree(pDebugObj);
        return rc;
    }
    
    clEoDebugRegister();

    return rc;
}

ClRcT clDebugLibFinalize(void)
{
    ClEoExecutionObjT  *pEoObj    = NULL; 
    ClDebugObjT        *pDebugObj = NULL;
    ClRcT              rc         = CL_OK;

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clEoPrivateDataGet( pEoObj, CL_EO_DEBUG_OBJECT_COOKIE_ID,
                             (void**) &pDebugObj);
    if (CL_OK != rc)
    {
        return rc;
    }

    clEoDebugDeregister();

    if( CL_OK != (rc = clHandleDatabaseDestroy(pDebugObj->hDebugFnDB))) 
    {
        clLogError("DBG","DBC","clHandleDatabaseDestroy():" 
                       "rc[0x %x]", rc);
    }
    
    clDebugClientTableDeregister(pEoObj);

    if( CL_OK != (rc = clEoClientUninstallTables(pEoObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, DEBUGCli))) )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"clEoClientUninstall(): rc[0x %x]",
                    rc);
    }

    clHeapFree(pDebugObj);

    return rc;
}

ClRcT
clDebugPromptSet(const ClCharT  *pCompPrompt)
{
    ClEoExecutionObjT  *pEoObj    = NULL; 
    ClDebugObjT        *pDebugObj = NULL;
    ClRcT              rc         = CL_OK;

    if( NULL == pCompPrompt )
    {
        clLogError("DBG","PRMT","Passed Prompt is NULL");
        return CL_DEBUG_RC(CL_ERR_NULL_POINTER);
    }
    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        clLogError("DBG","PRMT","clEoMyEoObjectGet(): rc[0x %x]",
                    rc);
        return rc;
    }
    rc = clEoPrivateDataGet( pEoObj,
                             CL_EO_DEBUG_OBJECT_COOKIE_ID,
                             (void**) &pDebugObj);
    if (CL_OK != rc)
    {
        clLogError("DBG","PRMT","clEoPrivateDataGet(): rc[0x %x]",
                    rc);
        return rc;
    }

    if (strlen(pCompPrompt) < CL_DEBUG_COMP_PROMPT_LEN)
    {
        strcpy(pDebugObj->compPrompt, pCompPrompt);
    }
    else
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_WARNING,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_1_INVALID_PARAMETER,"Prompt is Invalid");
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    }

    return CL_OK;
}

ClRcT
clDebugDuplicateDetectWalk(ClHandleDatabaseHandleT  hHandleDB, 
                           ClHandleT                handle, 
                           ClPtrT                   pData)
{
    ClRcT                 rc           = CL_OK;
    ClDebugFuncGroupT     *pFuncGroup  = NULL;
    ClUint32T             i            = 0;
    ClUint32T             j            = 0;
    ClDebugFuncGroupT     *pPassedData = (ClDebugFuncGroupT *) pData;

    rc = clHandleCheckout(hHandleDB, handle, (void **) &pFuncGroup); 
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"clHandleCheckout(): rc[0x %x]", rc);
        return rc;
    }

    for (i = 0; i < pPassedData->numFunc; i++)
    {
        for(j = 0; j < pFuncGroup->numFunc; j++ )
        {
            if (!strcasecmp(pFuncGroup->pFuncDescList[j].funcName,
                        pPassedData->pFuncDescList[i].funcName))
            {
                clHandleCheckin(hHandleDB, handle);
                clLogError("DUP","DTW","Duplicate command found : %s", 
                            pFuncGroup->pFuncDescList[j].funcName);
                return CL_DEBUG_RC(CL_ERR_DUPLICATE);
            }
        }
    }
    clHandleCheckin(hHandleDB, handle);

    return CL_OK;
}

ClRcT
clDebugDuplicateCommandDetect(ClDebugObjT        *pDebugObj, 
                              ClDebugFuncEntryT  *pFuncArray,
                              ClUint32T          numFunc)
{
    ClUint32T          i         = 0;
    ClUint32T          j         = 0;
    ClDebugFuncGroupT  funcGroup = {0};
    ClRcT              rc        = CL_OK;

    /*
     * Detect duplicate entries are there in the passed 
     * List of commands
     */
    for( i = 0; i < numFunc; i++ )
    {
        for( j = 0; j < numFunc; j++ )
        {
            if( i != j )
            {
                if( !(strcasecmp(pFuncArray[i].funcName,
                            pFuncArray[j].funcName)) )
                {
                    clLogError("DUP","CMD","Duplicate command found : %s", 
                                pFuncArray[i].funcName);
                    return CL_DEBUG_RC(CL_ERR_DUPLICATE);
                }
            }
        }
    }

    funcGroup.pFuncDescList = pFuncArray;
    funcGroup.numFunc       = numFunc;
    /*
     * This will check all the commands registered by this eoObj already 
     * with passed funcGroup.
     */
    rc = clHandleWalk(pDebugObj->hDebugFnDB, clDebugDuplicateDetectWalk, 
                      &funcGroup);
    if( CL_OK != rc )
    {
        clLogError("DUP","CMD","clHandleWalk(); rc[0x %x]", rc);
    }

    return rc;
}

ClRcT clDebugRegister(ClDebugFuncEntryT  *funcArray,
                      ClUint32T          funcArrayLen,
                      ClHandleT          *phDebugReg)/* - by user - needs to be exposed*/
{
    ClRcT              rc          = CL_OK;
    ClDebugObjT        *pDebugObj  = NULL;
    ClUint32T          i           = 0;
    ClEoExecutionObjT  *pEoObj     = NULL; 
    ClDebugFuncGroupT  *pFuncGroup = NULL;

    if ((0 == funcArrayLen) || (NULL == funcArray))
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_WARNING,CL_DEBUG_LIB_CLIENT, CL_LOG_MESSAGE_1_INVALID_PARAMETER,"Arguments are Invalid");
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        clLogError("DBG","REG","clEoMyEoObjectGet(): rc[0x %x]",rc);
        return rc;
    }
    rc = clEoPrivateDataGet( pEoObj,CL_EO_DEBUG_OBJECT_COOKIE_ID,(void**) &pDebugObj);
    if (CL_OK != rc)
    {
        clDbgCodeError(rc,("Debug CLI library has not been initialized"));
        return rc;
    }
    
     /* Detecting dupilcate entries are there or not */
    rc = clDebugDuplicateCommandDetect(pDebugObj, funcArray, funcArrayLen);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCreate(pDebugObj->hDebugFnDB, sizeof(ClDebugFuncGroupT), phDebugReg);
    if( CL_OK != rc )
    {
        clLogError("DBG","REG","clHandleCreate(): rc[0x %x]", rc);
        return rc;
    }

    rc = clHandleCheckout(pDebugObj->hDebugFnDB, *phDebugReg, 
                          (void **) &pFuncGroup);
    if( CL_OK != rc )
    {
        clLogError("DBG","REG","clHandleCheckout(): rc[0x %x]", rc);
        clHandleDestroy(pDebugObj->hDebugFnDB, *phDebugReg);
        return rc;
    }

    pFuncGroup->pFuncDescList = (ClDebugFuncEntryT*) clHeapAllocate(sizeof(ClDebugFuncEntryT) * funcArrayLen);
    if (NULL == pFuncGroup->pFuncDescList)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_CRITICAL,CL_DEBUG_LIB_CLIENT,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clHandleCheckin(pDebugObj->hDebugFnDB, *phDebugReg);
        clHandleDestroy(pDebugObj->hDebugFnDB, *phDebugReg);
        return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
    }

    for (i = 0; i < funcArrayLen; i++)
    {
         pFuncGroup->pFuncDescList[i] = funcArray[i];
    }

    pFuncGroup->numFunc = funcArrayLen;
    pDebugObj->numFunc += pFuncGroup->numFunc;
    if( CL_OK != (rc = clHandleCheckin(pDebugObj->hDebugFnDB, *phDebugReg)))
    {
        clLogError("DBG","REG","clHandleCheckin(): rc[0x %x]", rc);
    }

    return CL_OK;
}

ClRcT clDebugDeregister(ClHandleT  hReg)
{
    ClDebugObjT        *pDebugObj  = NULL;
    ClRcT              rc          = CL_OK;
    ClEoExecutionObjT  *pEoObj     = NULL; 
    ClDebugFuncGroupT  *pFuncGroup = NULL;

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        clLogError("DBG","DRG","clEoMyEoObjectGet(): rc[0x %x]",
                    rc);
        return rc;
    }
    rc = clEoPrivateDataGet( pEoObj, CL_EO_DEBUG_OBJECT_COOKIE_ID, (void**) &pDebugObj);
    if (CL_OK != rc)
    {
        clLogError("DBG","DRG","clEoPrivateDataGet(): rc[0x %x]",
                    rc);
        return rc;
    }

    rc = clHandleCheckout(pDebugObj->hDebugFnDB, hReg, (void **) &pFuncGroup);
    if( CL_OK != rc )
    {
        clLogError("DBG","DRG","clHandleCheckout(): rc[0x %x]", rc);
        return rc;
    }
    pDebugObj->numFunc -= pFuncGroup->numFunc;

    clHeapFree(pFuncGroup->pFuncDescList);

    if( CL_OK != (rc = clHandleCheckin(pDebugObj->hDebugFnDB, hReg)))
    {
        clLogError("DBG","DRG","clHandleCheckin(): rc[0x %x]", rc);
    }
    rc = clHandleDestroy(pDebugObj->hDebugFnDB, hReg);
    if( CL_OK != rc )
    {
        clLogError("DBG","DRG","clHandleDestroy(): rc[0x %x]", rc);
    }

    return CL_OK;
}

ClRcT
clDebugPrintExtended(ClCharT **retstr, ClInt32T *maxBytes, ClInt32T *curBytes, 
                     const ClCharT *format, ...)
{
    va_list ap;
    ClInt32T len;
    ClCharT c;
    if(!retstr || !maxBytes || !curBytes)
        return CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
    va_start(ap, format);
    len = vsnprintf(&c, 1, format, ap);
    va_end(ap);
    if(!len) return CL_OK;
    ++len;
    if(!*maxBytes) *maxBytes = CL_MAX(512, len<<1);
    if(!*retstr || (*curBytes + len) >= *maxBytes)
    {
        if(!*retstr) *curBytes = 0;
        *maxBytes *= ( *curBytes ? 2 : 1 );
        if(*curBytes + len >= *maxBytes)
            *maxBytes += (len<<1);
        *retstr = (ClCharT*) clHeapRealloc(*retstr, *maxBytes);
        CL_ASSERT(*retstr != NULL);
    }
    va_start(ap, format);
    *curBytes += vsnprintf(*retstr + *curBytes, *maxBytes - *curBytes, format, ap);
    va_end(ap);
    return CL_OK;
}

ClRcT clDebugPrintInitialize(ClDebugPrintHandleT* handle)
{
    return clBufferCreate((ClBufferHandleT*)handle);
}

ClRcT clDebugPrint(ClDebugPrintHandleT handle, const char* fmtStr, ...)
{
    char buf[CL_DEBUG_MAX_PRINT];
    char *space = buf;
    va_list args;
    ClRcT rc = CL_OK;
    ClBufferHandleT msg = (ClBufferHandleT)handle;
    ClUint32T c = 0, bytes = 0;

    /*
     * Estimate the space required for the buffer
     */
    va_start(args, fmtStr);
    bytes = vsnprintf((ClCharT*)&c, 1, fmtStr, args);
    va_end(args);
    
    if(!bytes) goto failure;

    if(bytes >= sizeof(buf))
    {
        space = (char*) clHeapCalloc(1, bytes + 1);
        CL_ASSERT(space != NULL);
    }

    va_start(args, fmtStr);
    vsnprintf(space, bytes+1, fmtStr, args);
    va_end(args);

    if (0 != msg)
    {
        rc = clBufferNBytesWrite(msg, (ClUint8T*)space, bytes);
        if (CL_OK != rc)
        {
            goto failure;
        }
    }
    else
    {
        rc = CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

failure:
    if(space != buf)
        clHeapFree(space);

    return rc;
}

ClRcT clDebugPrintFinalize(ClDebugPrintHandleT* handle, char** buf)
{
    ClUint32T len = 0;
    ClRcT rc = CL_OK;
    ClBufferHandleT* msg = (ClBufferHandleT*)handle;

    if (NULL == buf)
    {
        return CL_DEBUG_RC(CL_ERR_NULL_POINTER);
    }

    rc = clBufferLengthGet(*msg, &len);
    if (CL_OK != rc)
    {
        return rc;
    }

    *buf = (char*)clHeapAllocate(len + 1);
    if (CL_OK != rc)
    {
        return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clBufferNBytesRead(*msg, (ClUint8T*)*buf, &len);
    if (CL_OK != rc)
    {
        clHeapFree(*buf);
        *buf = NULL;
        return rc;
    }
    (*buf)[len] = '\0';

    clBufferDelete(msg);
    *msg = 0;

    return rc;
}


ClRcT clDebugPrintDestroy(ClDebugPrintHandleT* handle)
{
    return clBufferDelete(handle);
}

ClRcT clDebugVersionCheck(ClVersionT *pVersion)
{
     ClRcT  rc = CL_OK;
     if(pVersion == NULL)
     {
        rc = CL_ERR_NULL_POINTER;
        return rc;
     }
     rc = clVersionVerify(&versionDatabase,pVersion);
     if(rc != CL_OK)
     {
          return rc;
     }
     return rc;
}
