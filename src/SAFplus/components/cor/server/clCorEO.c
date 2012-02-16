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
 * ModuleName  : cor
 * File        : clCorEO.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module provides COR EO functionality
 *****************************************************************************/

/* INCLUDES */

#define __SERVER__
#include <string.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clEoApi.h>
#include <clTimerApi.h>
#include <clCpmApi.h>
#include <clOsalApi.h>
#include <clBufferApi.h>

/* Internal Headers*/
#include "clCorSync.h"
#include "clCorEO.h"
#include "clCorDmDefs.h"
#include "clCorRmDefs.h"
#include "clCorStats.h"
#include "clCorPvt.h"
#include "clCorClient.h"
#include "clCorEoFunc.h"
#include "clCorTxnInterface.h"
#include "clCorLog.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* MACROS */
#define WAIT()        (clOsalMutexLock(corEO.sem))
#define UNWAIT()      (clOsalMutexUnlock(corEO.sem))


#ifdef DISTRIBUTED_COR
static ClRcT _corEODMAttrHandler(ClUint32T cData, ClBufferHandleT  inMsgHandle,
                        ClBufferHandleT  outMsgHandle);
#endif

#ifdef GETNEXT
extern  ClRcT
_corObjectGetNextOp(ClUint32T eoArg, corObjGetNextInfo_t* pData, int len,
			char* pOutData, int* outlen);
#endif

#undef __CLIENT__
#include "clCorServerFuncTable.h"

/* GLOBALS */
extern CORStat_h corStatTab;
extern ClUint32T corRunningMode;

/* LOCALS */
static int corEOInitDone = 0;
/* corEO mgr data */
static COREoData_t corEO;

/**
 *  COR Execution Object entity initialization routine.
 *
 *  This API contains the code for the Execution Object entity
 *  initialization routine.
 *
 *  @param param1 - par1
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
corEOInit()
{
    ClRcT    rc     = CL_OK;
    ClEoExecutionObjT *pEOObj;

    if (corEOInitDone)
	{
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL, 
					CL_LOG_MESSAGE_0_EO_ALREADY_INIT);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "COR EO interface is already initialized."));
        return (CL_COR_SET_RC(CL_COR_ERR_ALREADY_INIT));
	}

    memset(&corEO, 0, sizeof(corEO));

    clOsalMutexCreate(&corEO.sem);

    corEO.state = CL_EO_STATE_INIT;
    corEO.someQ = 0;

    /* create EO to represent corEO */

     rc = clEoMyEoObjectGet(&pEOObj);
     if(CL_OK != rc)
     {
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
				CL_LOG_MESSAGE_1_EO_OBJECT_GET, rc);
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get EO object [%x]",rc));
         return (rc);
     }

    if ( (rc = clEoClientInstallTables(pEOObj, 
                                       CL_EO_SERVER_SYM_MOD(gAspFuncTable, COR))) != CL_OK)
	{
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_1_NATIVE_FUNCTION_TABLE_INIT, rc);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Native function table installation Failed. rc [0x%x]", rc));
        return (rc);
	}

	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "COR EO: Installed the native function table"));

   	corEO.state = CL_EO_STATE_ACTIVE;

    /* delay, then start the negotiation with SD for data transfer */
    corEOInitDone = 1;

    return(rc);
} /* corEOInit */

/* This function finalizes the cor EO */
void corEoFinalize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEOObj;

    corEOInitDone = 0;

    rc = clEoMyEoObjectGet(&pEOObj);
    if(CL_OK != rc)
     {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get EO object [%x]",rc));
         return;
     }  

    clEoClientUninstall(pEOObj, CL_EO_NATIVE_COMPONENT_TABLE_ID);
}


#ifdef DISTRIBUTED_COR
static ClRcT
_corEODMAttrHandler(ClUint32T cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT ret = CL_OK;
    ClCorObjectHandleT  objHandle;
    ClCorAttrIdT        attrId;
    ClCorAttrPathPtrT   pAttrPath = NULL;
    ClUint32T      index;
    ClUint32T      size = 0;
    void         *param;
    void         *value;
    CL_FUNC_ENTER();

   if((ret = clBufferFlatten(inMsgHandle, (ClUint8T **)&param))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
         CL_FUNC_EXIT();
         return ret;
    }

    if ( (NULL == param) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Null arguments"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    STREAM_OUT( &objHandle, param, sizeof(ClCorObjectHandleT));
    pAttrPath = param;
    param = (ClUint8T *)param + sizeof(ClCorAttrPathT);
    STREAM_OUT( &attrId, param, sizeof(ClCorAttrIdT));
    STREAM_OUT( &index, param, sizeof(index));
    STREAM_OUT( &size,  param, sizeof(size));

    /* Get the size of the outMsg  */
    value = clHeapAllocate(size);
    if(!value)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Memory Allocation Failed"));
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    ret = _clCorObjectAttributeGet(objHandle, pAttrPath, attrId, index, value, &size);

    /* Write to the outMsg*/
    clBufferNBytesWrite(outMsgHandle, (ClUint8T *)value, size);

    clHeapFree(value);
    CL_FUNC_EXIT();
    return ret;
}
#endif

