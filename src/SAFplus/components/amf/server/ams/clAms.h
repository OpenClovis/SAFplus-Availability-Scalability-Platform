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
 * File        : clAms.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains AMS internal definitions.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_H_
#define _CL_AMS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/
#include <stdio.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clHeapApi.h>

#include <clAmsErrors.h>
#include <clAmsTypes.h>
#include <clAmsDatabase.h>
#include <clAmsDefaultConfig.h>

#include <clHandleApi.h>

#include <clCpmApi.h>
#include <clCpmAms.h>
#include <clCkptApi.h>
#include <clEventApi.h>
#include <clDifferenceVector.h>

/******************************************************************************
 * AMS Operations
 *
 * Keep track of operations. Currently only track counters, will add more
 * features in future.
 *****************************************************************************/

typedef struct
{
    ClUint32T               lastOp;         /* operation counter             */
    ClUint32T               currentOp;      /* current operation id          */
} ClAmsOpsT;

#define CL_AMS_OPS_MAX_COUNT    10000

#define AMS_OP_INCR(ops) \
{\
    (ops)->lastOp = ((((ops)->lastOp)+1) % (CL_AMS_OPS_MAX_COUNT)); \
    (ops)->currentOp = (ops)->lastOp ;  \
    AMS_LOG(CL_DEBUG_TRACE, ("--- New Operation [ID = %05d]\n", (ops)->currentOp)); \
}

/******************************************************************************
 * AMS INSTANCE
 *
 * All AMS config and state is stored in a structure of type ClAmsT. One
 * instance of this structure is created for each AMS domain. If entire 
 * system is one domain then only one instance is ever created.
 *
 * Future: Add support for domains
 *****************************************************************************/

typedef struct
{
    ClBoolT                 isEnabled;      /* is AMS enabled? (not used)    */
    ClAmsServiceStateT      serviceState;   /* status of AMS                 */
    ClUint32T               mode;           /* mode of operation for AMS     */

    ClOsalMutexIdT          mutex;          /* control access to database    */
    ClAmsDbT                db;             /* the config and state db       */

    ClOsalMutexIdT          terminateMutex; /* control access to cond var    */
    ClOsalCondIdT           terminateCond;  /* terminate condition var       */
    ClUint32T               timerCount;     /* count of running timers       */
    ClUint8T                debugFlags;     /* debug flags for all of AMS    */
    ClTimeT                 serverepoch;    /* time server was started       */
    ClTimeT                 epoch;          /* time cluster was started      */

    ClUint32T               logCount;       /* counter for logs              */

    ClAmsOpsT               ops;            /* information about operations  */
    ClUint32T               invocationCount;
    ClCntHandleT            invocationList;
    ClBoolT                 debugLogToConsole; /* log the messages to console */
    ClOsalMutexT            ckptMutex;      /* mutex to serialize ckpt write */


#define CL_AMS_DB_INVOCATION_PAIRS      0x2

    ClEoExecutionObjT       *eoObject;     /* Execution objects for threads */
    ClCkptSvcHdlT           ckptInitHandle;
    ClCkptHdlT              ckptOpenHandle;
    ClEventInitHandleT      eventInitHandle;
    ClEventChannelHandleT   eventChannelOpenHandle;
    ClEventHandleT          eventHandle;
    ClBoolT                 eventServerReady;
    ClBoolT                 eventServerInitialized;
    ClNameT                 ckptName;
    ClNameT                 ckptDBSections[CL_AMS_DB_INVOCATION_PAIRS];
    ClNameT                 ckptInvocationSections[CL_AMS_DB_INVOCATION_PAIRS];
    ClDifferenceVectorKeyT  ckptDifferenceVectorKeys[CL_AMS_DB_INVOCATION_PAIRS];
    ClNameT                 ckptCurrentSection;
    ClNameT                 ckptVersionSection;
    ClBoolT                 ckptServerReady;

    ClHandleDatabaseHandleT ccbHandleDB;
    ClBoolT                 cpmRecoveryQuiesced; /* if cpm has disabled amf recovery */
} ClAmsT;

extern ClAmsT   gAms;

extern FILE *debugPrintFP;
extern ClBoolT debugPrintAll;

extern ClBoolT gAmsDBRead;

#define CL_AMS_TERMINATE_TIMEOUT 30000      /* time to wait for termination  */

#define CL_AMS_LOG_MAX_COUNT    100000

#define AMS_LOG_INCR(X)   ((X) = ((X)+1) % CL_AMS_LOG_MAX_COUNT)

/******************************************************************************
 * AMS Function Prototypes
 *****************************************************************************/

extern ClRcT clAmsInstantiate(
        CL_IN       ClAmsT              *ams,
        CL_IN       ClUint32T           mode);

extern ClRcT clAmsTerminate(
        CL_IN  ClAmsT  *ams,
        CL_IN  const ClUint32T  mode);

extern ClRcT clAmsInitialize(
        CL_IN  ClAmsT  *ams,
        CL_IN  ClCpmAmsToCpmCallT  *cpmFuncs,
        CL_IN  ClCpmCpmToAmsCallT  *amsFuncs);

extern ClRcT clAmsStart(
        CL_IN  ClAmsT  *ams,
        CL_IN  const ClUint32T  mode);

extern ClRcT clAmsInitializeMgmtInterface();

/*
 * clAmsFinalize
 * --------------
 * Finalizes/deletes an AMS entity. This function stops the AMS service, all
 * entities under the purview of AMS are sent terminate signals and then
 * the AMS database is cleaned up. Typically, you would call this only if
 * you need to do a full cluster reset.
 *
 */

extern ClRcT clAmsFinalize(
        CL_IN       ClAmsT      *ams,
        CL_IN       ClUint32T   mode);

extern ClRcT clAmsMgmtServerFuncInstall ( void );

extern ClRcT clAmsFaultQueueAdd(ClAmsEntityT *entity);

extern ClRcT clAmsFaultQueueDelete(ClAmsEntityT *entity);

extern ClRcT clAmsFaultQueueFind(ClAmsEntityT *entity, void **entry);

extern ClRcT clAmsFaultQueueDestroy(void);

extern ClRcT clAmsCheckNodeJoinState(const ClCharT *pNodeName);

extern void clAmsSetInstantiateCommand(ClInt32T argc, ClCharT **argv);

extern const ClCharT *clAmsGetInstantiateCommand(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_H_ */
