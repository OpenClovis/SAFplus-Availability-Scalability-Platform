/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : eo
 * File        : clSAClientSelect.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clCpmApi.h>
#include <clSAClientSelect.h>

#ifdef SOLARIS_BUILD 
#include <sys/time.h>
#include <string.h>
#endif

typedef ClRcT (*dispatchfun) ();

#define MAX_AREA 5

static ClSelectionObjectT fd[MAX_AREA];
static int numArea = 0;

dispatchfun dispatch[MAX_AREA];
ClHandleT *handle[MAX_AREA];

extern ClUint32T componentTerminate;

static void *clDispatchHandleAllCallBacks(void *threadArg)
{
    fd_set rfds;
    int nfds = 0;
    int i = 0;
    ClEoExecutionObjT *pEoObj = (ClEoExecutionObjT *) threadArg;
    int retCode = 0;

    clEoMyEoObjectSet(pEoObj);
    clEoMyEoIocPortSet(pEoObj->eoPort);

    FD_ZERO(&rfds);

    while (componentTerminate != CL_TRUE)
    {
        for (i = 0; i < numArea; i++)
        {
            if (nfds < fd[i])
                nfds = fd[i];   /* find max fd */
            FD_SET(fd[i], &rfds);
        }
        retCode = select(nfds + 1, &rfds, NULL, NULL, NULL);
        if (retCode <= 0)
        {
            continue;
        }
        for (i = 0; i < numArea; i++)
        {
            if (FD_ISSET(fd[i], &rfds))
            {
                (*dispatch[i]) (*handle[i], CL_DISPATCH_ONE);
            }
        }
    }
    return NULL;
}

ClRcT clDispatchThreadCreate(ClEoExecutionObjT *eoObj, ClOsalTaskIdT *taskId,
                             ClSvcHandleT svcHandle)
{
    ClRcT rc = CL_OK;

    if (svcHandle.cpmHandle != NULL)
    {
        clCpmSelectionObjectGet(*(svcHandle.cpmHandle), &fd[numArea]);

        dispatch[numArea] = clCpmDispatch;
        handle[numArea] = svcHandle.cpmHandle;
        numArea++;
    }

    rc = clOsalTaskCreateAttached("handleResponse", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                          clDispatchHandleAllCallBacks, eoObj, taskId);

    return rc;
}
