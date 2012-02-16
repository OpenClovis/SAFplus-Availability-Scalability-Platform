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
/*****************************************************************************
 *
 * lockutil.c
 *
 ****************************** Description **********************************
 *
 * lockutil.c: This file contains code to interface with ams to request
 *             changes to the lock status of clovis sample applications
 *****************************************************************************/




#include <stdio.h>
#include <unistd.h>

/*
 * I know that every other bit of code in the system insists on including
 * clovis header files with angle brackets.  And if everyone was jumping
 * off a cliff, would you do that too?
 * The practice seems to stem from the mistaken belief that only files
 * included with angle brackets pay attention to include directories
 * specified with -I at compile time
 */
#include <clOsalApi.h>
#include <clAmsMgmtClientApi.h>
#include "clBufferApi.h"
#include "clCommon.h"
#include "clCommonErrors.h"
#include "clRmdApi.h"
#include "clEoApi.h"
#include "clIocApi.h"
#include "clIocApiExt.h"
#include "ev.h"
#include "common.h"

#define CL_IOC_LOCKUTIL_PORT (CL_IOC_USER_RESERVERD_COMMPORT_END-1)

static ClRcT appInitialize(ClUint32T argc, ClCharT* argv[]);
static ClRcT appFinalize(void);
static ClRcT appStateChange(ClEoStateT eoState);
static ClRcT appHealthCheck(ClEoSchedFeedBackT* schFeedBack);
#if defined(NOTDEFINED)
// TODO: Remove this when we know we don't need it
static ClRcT appTerminate(ClInvocationT invocation, const ClNameT* compName);

static ClRcT
appTerminate(ClInvocationT invocation, const ClNameT* compName)
{
    ClRcT rc = CL_OK;
    rc = clCpmComponentUnregister(gCpmHandle, compName, NULL);
    rc = clCpmClientFinalize(gCpmHandle);
    rc = clCpmResponse(gCpmHandle, 0, CL_OK);
    return CL_OK;
}
#endif

static void usage();

static ClVersionT               gVersion = {'B', 01, 01};
static ClAmsMgmtCallbacksT gMgmtCallbacks =
{
    1   // This is a dummy
};

ClAmsMgmtHandleT gMgmtHandle;


ClEoConfigT clEoConfig = {
    "lockutil",             // EO Name
    1,                      // Thread Priority
    1,                      // 1 listener thread
    0,                      // our IOC port: let IOC layer decide
    MY_MAX_EO_CLIENT_ID,
    CL_EO_USE_THREAD_FOR_APP,
    appInitialize,
    appFinalize,
    appStateChange,
    appHealthCheck
};
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,            /* osal */
    CL_TRUE,            /* timer */
    CL_TRUE,            /* buffer */
    CL_TRUE,            /* ioc */
    CL_TRUE,            /* rmd */
    CL_TRUE,            /* eo */
    CL_FALSE,           /* om */
    CL_FALSE,           /* hal */
    CL_FALSE,           /* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_FALSE,           /* cor */
    CL_FALSE,           /* cm */
    CL_FALSE,           /* name */
    CL_FALSE,           /* log */
    CL_FALSE,           /* trace */
    CL_FALSE,           /* diag */
    CL_FALSE,           /* txn */
    CL_FALSE,           /* hpi */
    CL_FALSE,           /* cli */
    CL_FALSE,           /* alarm */
    CL_FALSE,           /* debug */
    CL_FALSE,           /* gms */
    CL_FALSE            /* pm */
};

static ClEoPayloadWithReplyCallbackT serverFuncList[] =
{
        (ClEoPayloadWithReplyCallbackT)NULL,
};

typedef enum {
    CL_LOCK_ASSIGNMENT,
    CL_LOCK_INSTANTIATE,
    CL_LOCK_UNLOCK,
    CL_LOCK_INVALID_OP
} ClLockOperation;

ClLockOperation
getRequestedOp(ClUint32T argc, const ClCharT** argv)
{
    if (strcmp(argv[1], "la") == 0)
    {
        return CL_LOCK_ASSIGNMENT;
    }
    else if (strcmp(argv[1], "li") == 0)
    {
        return CL_LOCK_INSTANTIATE;
    }
    else if (strcmp(argv[1], "ul") == 0)
    {
        return CL_LOCK_UNLOCK;
    }
    return CL_LOCK_INVALID_OP;
}

const char *
descLockOp(ClLockOperation op)
{
    switch (op)
    {
    case CL_LOCK_ASSIGNMENT:
        return "LockAssignment";
    case CL_LOCK_INSTANTIATE:
        return "LockInstantiation";
    case CL_LOCK_UNLOCK:
        return "Unlock";
    default:
        return "INVALID_OP";
    }
}

ClRcT
fillEntity(int argc, const char **argv, ClAmsEntityT *entity)
{
    const char* grp     = argv[2];
    const char* prefix  = "csa";
    const char* suffix  = "SGI0";
    char*       groupName = 0;
    char*       ptr     = 0;

    //
    // The "grp" string should be non-null string and should be all
    // decimal digits.
    if ((strlen(grp) == 0))
    {
        clOsalPrintf("not a legitimate group identifier: ''\n");
        return CL_ERR_INVALID_PARAMETER;
    }

    //
    // initialize the entity
    memset(entity, 0, sizeof entity);

    //
    // The string we have to identify the group can be either a numeric
    // string or a string with one or more non-numeric characters.  If
    // the former, then it's a group number and we construct the name
    // of the group.  If the later, then we assume that it IS the name
    // of the group.
    // NOTE that the entity->name.value field has already been filled
    // with zeros and we refuse to overwrite last character of that
    // field below, so it's not necessary to nul terminate.
    if (strspn(grp, "0123456789") != strlen(grp))
    {
        ptr = groupName = entity->name.value;
        strncpy(ptr, grp, sizeof entity->name.value - 1);
    } 
    else
    {
        ptr = groupName = entity->name.value;
        strncpy(ptr, prefix, sizeof entity->name.value - 1);
        ptr += strlen(ptr);
        strncpy(ptr, grp, (sizeof entity->name.value - 1) - (ptr - groupName));
        ptr += strlen(ptr);
        strncpy(ptr, suffix, (sizeof entity->name.value - 1) -
                    (ptr - groupName));
    }

    entity->type = CL_AMS_ENTITY_TYPE_SG;
    entity->name.length = strlen(groupName) + 1;
    entity->debugFlags = 0;

    return CL_OK;

}

char *
getEntityName(const ClAmsEntityT *entity, char *buf, int size)
{
    int n;                  // number of characters to copy
    if (entity->name.length <= (size - 1))
    {
        n = entity->name.length; // value will fit in buf, with nul terminator
    }
    else
    {
        n = size - 1;       // value will not fit, save room for nul terminator
    }
    // copy what fits into the buffer and null terminate just in case necessary
    strncpy(buf, entity->name.value, n);
    buf[n] = 0;
    return buf;
}

ClRcT
lockEntity(const ClAmsEntityT *entity, ClLockOperation op)
{
    char nameBuf[sizeof entity->name.value + 1];
    ClRcT rc = CL_OK;
    //
    // TODO: do some consistency checking here
    switch (op)
    {
    case CL_LOCK_ASSIGNMENT:
        rc = clAmsMgmtEntityLockAssignment(gMgmtHandle, entity);
        break;
    case CL_LOCK_INSTANTIATE:
        rc = clAmsMgmtEntityLockInstantiation(gMgmtHandle, entity);
        break;
    case CL_LOCK_UNLOCK:
        rc = clAmsMgmtEntityUnlock(gMgmtHandle, entity);
        break;
    default:
        rc = CL_ERR_INVALID_PARAMETER;
        clOsalPrintf("Invalid Operation requested of lockEntity()\n");
        break;
    }
    if (rc != CL_OK)
    {
        clOsalPrintf("failed [0x%x] to lock entity: %s\n", rc,
                    getEntityName(entity, nameBuf, sizeof nameBuf));
        return rc;
    }
    clOsalPrintf("Successfully changed state of %s to %s\n",
                getEntityName(entity, nameBuf, sizeof nameBuf),
                descLockOp(op));
    return CL_OK;
}

ClRcT
handleLockCommand(ClUint32T argc, const ClCharT** argv)
{
    ClAmsEntityT entity;
    ClRcT           rc = CL_OK;
    //
    // Do some minimal parameter checking
    if (argc != 3)
    {
        usage();
        return CL_OK;
    }

    //
    // Find which lock operation is being requested: {Unlock, Lock for Assign,
    // Lock for Instantiation} or an invalid op.
    ClLockOperation op = getRequestedOp(argc, argv);

    //
    // Find the entity being manipulated
    if ((rc = fillEntity(argc, (const char**)argv, &entity)) != 0)
    {
        clOsalPrintf("failed to find entity from args\n");
        return -1;
    }
    if ((rc = lockEntity(&entity, op)) != CL_OK)
    {
        return rc;
    }

    return CL_OK;
}

static ClRcT
appStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

static ClRcT
appHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq  = CL_EO_BUSY_POLL;
    schFeedback->status = 0;    // CL_CPM_EO_ALIVE
    return CL_OK;
}

ClRcT
clientTerminate()
{
    clAmsMgmtFinalize(gMgmtHandle);
    return CL_OK;
}

static ClRcT
appFinalize(void)
{
    ClEoExecutionObjT*  eoObj;
    ClRcT               rc = CL_OK;

    if ((rc = clEoMyEoObjectGet(&eoObj)) != CL_OK)
    {
        clOsalPrintf("failed [0x%x] to get EO Object\n", rc);
    }
    if ((rc = clEoClientUninstall(eoObj, MY_EO_CLIENT_ID)) != CL_OK)
    {
        clOsalPrintf("failed [0x%x] to uninstall EO Object\n", rc);
    }
    if ((rc = clAmsMgmtFinalize(gMgmtHandle)) != CL_OK)
    {
        clOsalPrintf("failed [0x%x] to finalize ams management\n", rc);
    }
    return CL_OK;
}

static ClRcT
appInitialize(ClUint32T argc, ClCharT** argv)
{
    ClRcT               rc      = CL_OK;
    ClEoExecutionObjT*  eoObj   = NULL;

    if ((rc = clEoMyEoObjectGet(&eoObj)) != CL_OK)
    {
        clOsalPrintf("failed: [0x%x] to get my EoObject at line %d\n",
                    rc, __LINE__);
        return rc;
    }
    if ((rc = clEoClientInstall(eoObj,
                            MY_EO_CLIENT_ID,
                            serverFuncList,
                            (ClEoDataT)eoObj,
                            sizeof serverFuncList / sizeof serverFuncList[0]))
        != CL_OK)
    {
        clOsalPrintf("failed: [0x%x] to install client at line %d\n",
                    rc, __LINE__);
        return rc;
    }
    if ((rc = clAmsMgmtInitialize(&gMgmtHandle, &gMgmtCallbacks, &gVersion))
                != CL_OK)
    {
        clOsalPrintf("failed: [0x%x] to init ams management at line %d\n",
                    rc, __LINE__);
        return rc;
    }

    // I have to cast to const???  Gods, but I miss C++
    rc = handleLockCommand(argc, (const ClCharT**)argv);
    if (rc != CL_OK)
    {
        clOsalPrintf("LockCommand failed: [0x%x]\n", rc);
        appFinalize();
        return rc;
    }

    rc = appFinalize();

    return rc;
}

static void
usage()
{
    clOsalPrintf("Usage: lockutil lock_cmd group_id\n");
}

