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

/**
 * This file implements debug cli commands of CPM.
 */

/*
 * Standard header files 
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
/*
 * ASP header files 
 */
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>

/*
 * CPM internal header files 
 */
#include <clCpmInternal.h>
#include <clCpmCliCommands.h>
#include <clCpmIpi.h>
#include <clCpmExtApi.h>

static ClUint32T cpmCliStrToInt(const ClCharT *str)
{
    long int nLong = 0;

    ClUint32T nInt = 0;

    nLong = strtol(str, NULL, 0);

    /* Truncation !!! */
    nInt = (ClUint32T)nLong;

    return nInt;
}

/* static void cpmDebugPrintInit(ClDebugPrintHandleT *msg) */
/* { */
/*     ClRcT rc = CL_OK; */
    
/*     rc = clDebugPrintInitialize(msg); */
/*     if (CL_OK != rc) */
/*     { */
/*         clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CLI, */
/*                    "Failed to do debug print initialize, error [%#x]", */
/*                    rc); */
/*     } */
/* } */

/* static void cpmDebugPrintFinalize(ClDebugPrintHandleT *msg, ClCharT **buf) */
/* { */
/*     ClRcT rc = CL_OK; */
    
/*     rc = clDebugPrintFinalize(msg, buf); */
/*     if (CL_OK != rc) */
/*     { */
/*         clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CLI, */
/*                    "Failed to do debug print finalize, error [%#x]", */
/*                    rc); */
/*     } */
/* } */

static void cpmCliPrint(ClCharT **retStr, const ClCharT *fmt, ...)
{
    va_list args;
    ClCharT tempStr[CL_MAX_NAME_LENGTH] = {0};

    va_start(args, fmt);
    vsnprintf(tempStr, CL_MAX_NAME_LENGTH-1, fmt, args);
    va_end(args);

    *retStr = (ClCharT *) clHeapAllocate(strlen(tempStr) + 1);
    if (!*retStr) 
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CLI,
                   "Failed to allocate memory, error [%#x]",
                   CL_CPM_RC(CL_ERR_NULL_POINTER));
        return;
    }
    strcpy(*retStr, tempStr);
}

/**
 *  Name: clCpmExecutionObjectListShow 
 *
 *  This function provides "ps" equivalent functionality
 *
 *  @returns 
 *    CL_OK                - everything is ok <br>
 *    CL_CPM_RC(CL_ERR_NULL_POINTER)      - NULL pointer
 *    CL_ERR_VERSION_MISMATCH - version mismatch
 */

ClRcT clCpmExecutionObjectListShow(ClInt32T argc,
                                   ClIocNodeAddressT compAddr,
                                   ClUint32T flag,
                                   ClEoIdT eoId,
                                   ClCharT **retStr)
{
    /*
     * ClCpmEOListNodeT* ptr = gpClCpm->eoList;
     */
    ClCpmEOListNodeT *ptr = NULL;
    ClCharT name[32] = "\0";
    ClCharT state[10] = "\0";
    ClCharT status[10] = "\0";
    ClUint32T compCount = 0;
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClRcT rc = CL_OK;
    ClCharT tempStr[256];
    ClCharT *tmpStr = NULL;
    ClBufferHandleT message = 0;

    rc = clBufferCreate(&message);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to create message %x\n", rc), rc);

    if (argc != ONE_ARGUMENT)
    {
        sprintf(tempStr, "Usage: EOShow");
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                        strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message %x\n", rc), rc);
        goto done;
    }


    sprintf(tempStr,
            "\n   ID  |   Port   |   Name    |   Health   |   EO State   | Recv Threads ");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                    strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message %x\n", rc), rc);

    sprintf(tempStr,
            "\n ===================================================================== ");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                    strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message %x\n", rc), rc);

    /*
     * take the semaphore 
     */
    if ((rc = clOsalMutexLock(gpClCpm->eoListMutex)) != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Could not get Lock successfully------\n"));

    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR, ("Unable to Get First component \n"), rc);

    rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                              (ClCntDataHandleT *) &comp);
    CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR, ("Unable to Get Node  Data \n"), rc);

    compCount = gpClCpm->noOfComponent;
    while (compCount != 0)
    {
        ptr = comp->eoHandle;

        if (flag == 0)
        {
            while (ptr != NULL && ptr->eoptr != NULL)
            {
                strcpy(name, ptr->eoptr->name);
                /*
                 * Obtain the state and status in string format 
                 */
                compMgrStateStatusGet(ptr->status, ptr->eoptr->state, status, sizeof(status),
                                      state, sizeof(state));
                if (ptr->eoptr->appType == CL_EO_USE_THREAD_FOR_RECV)
                    sprintf(tempStr, "\n 0x%llx| 0x%x | %10s | %10s | %10s | %04d ",
                            ptr->eoptr->eoID, ptr->eoptr->eoPort, name, status,
                            state, (ptr->eoptr->noOfThreads + 1));
                else
                    sprintf(tempStr, "\n 0x%llx| 0x%x | %10s | %10s | %10s | %04d ",
                            ptr->eoptr->eoID, ptr->eoptr->eoPort, name, status,
                            state, ptr->eoptr->noOfThreads);
                rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                                strlen(tempStr));
                CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR,
                                  ("\n Unable to write message \n"), rc);
                ptr = ptr->pNext;
            }
        }
        else
        {
            while (ptr != NULL && ptr->eoptr != NULL)
            {
                if (ptr->eoptr->eoID == eoId)
                {
                    /*
                     * obtain the state and status in string format 
                     */
                    compMgrStateStatusGet(ptr->status, ptr->eoptr->state,
                                          status, sizeof(status), state, sizeof(state));
                    strcpy(name, ptr->eoptr->name);

                    if (ptr->eoptr->appType == CL_EO_USE_THREAD_FOR_RECV)
                        sprintf(tempStr, "\n 0x%llx| 0x%x | %10s | %10s | %10s | %04d | ",
                                ptr->eoptr->eoID, ptr->eoptr->eoPort, name,
                                status, state, ptr->eoptr->noOfThreads + 1);
                    else
                        sprintf(tempStr, "\n 0x%llx| 0x%x | %10s | %10s | %10s | %04d | ",
                                ptr->eoptr->eoID, ptr->eoptr->eoPort, name,
                                status, state, ptr->eoptr->noOfThreads);
                    rc = clBufferNBytesWrite(message,
                                                    (ClUint8T *) tempStr,
                                                    strlen(tempStr));
                    CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR,
                                      ("\n Unable to write message \n"), rc);
                    break;
                }
                ptr = ptr->pNext;
            }
#if 0
            if (ptr == NULL)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EOID not found\n"));
#endif
        }
        compCount--;
        if (compCount)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR,
                              ("\n Unable to Get Node  Data \n"), rc);
            rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                      (ClCntDataHandleT *) &comp);
            CL_CPM_LOCK_CHECK(CL_DEBUG_ERROR,
                              ("\n Unable to Get Node  Data \n"), rc);
        }
    }
    /*
     * Release the semaphore 
     */
    rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
    CL_CPM_CHECK(CL_DEBUG_ERROR,
                 ("COMP_MGR: Could not UnLock successfully------\n"), rc);

    /*
     * Bug 4986 :
     * Moved the code to NULL terminate the string
     * below the done: label, so that the usage string
     * written to the buffer is also NULL terminated.
     */
  done:
    /*
     * NULL terminate the string 
     */
    sprintf(tempStr, "%s", "\0");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr, 1);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    /*
     * Construct the return buffer 
     */
    rc = clBufferFlatten(message, (ClUint8T **) &tmpStr);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to flatten the message \n"), rc);

    *retStr = tmpStr;
    
    clBufferDelete(&message);

    return (CL_OK);

  withlock:
    /*
     * Release the semaphore 
     */
    rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
    CL_CPM_CHECK(CL_DEBUG_ERROR,
                 ("COMP_MGR: Could not UnLock successfully------\n"), rc);
  failure:
    clBufferDelete(&message);
    return rc;
}

/**
 *  Name: clCpmExecutionObjectListShow
 *
 *  API for displaying the list of EOs 
 *
 *  @param compAddr : addr of component manager
 *         flag     : whether one EO or all the EOs 
 *                    0 - all the EOs
 *                    1 - a particular EO
 *         eoId     : eoid of the EO
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 */

ClRcT cliEOListShow(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClEoIdT eoId;

    {

        if (argc == ONE_ARGUMENT)
        {
            rc = clCpmExecutionObjectListShow(argc,
                                              clIocLocalAddressGet(),
                                              0,
                                              0,
                                              retStr);
        }
        else
        {
            eoId = (ClEoIdT) cpmCliStrToInt(argv[1]);
            rc = clCpmExecutionObjectListShow(argc,
                                              clIocLocalAddressGet(),
                                              1,
                                              eoId,
                                              retStr);
        }
    }

    return rc;
}

#if 0
/*
 * cliEOStateSet
 * This cli command is for setting the state of EO/EOs from 
 * command line
 */

ClRcT cliEOSetState(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClEoIdT eoId;
    ClEoStateT state;
    ClCharT tempStr[256];

    if (argc < THREE_ARGUMENT)
    {
        sprintf(tempStr, "%s%s%s", "Usage: EOStateSet <eoId> <state>\n",
                "\teoId[DEC]: Id assigned to an EO\n",
                "\tState[STRING]: SUSPEND/RESUME\n");
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto done;
    }
    else
    {
        eoId = cpmCliStrToInt(argv[1]);
        
        if (0 == strcmp(argv[2], "SUSPEND"))
            state = CL_EO_STATE_SUSPEND;
        else if (0 == strcmp(argv[2], "RESUME"))
            state = CL_EO_STATE_RESUME;
        else
        {
            sprintf(tempStr, "\n Improper state: State: SUSPEND/RESUME\n");
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto done;
        }

        rc = clCpmExecutionObjectStateSet(clIocLocalAddressGet(), eoId, state);
        if (rc == CL_OK)
            sprintf(tempStr, "\n State update Successful");
        else
            sprintf(tempStr, "\n State update Failed");
    }

  done:
    *retStr = (ClCharT *) clHeapAllocate(strlen(tempStr) + 1);
    if (*retStr == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n MAlloc Failed \n"),
                     CL_CPM_RC(CL_ERR_NO_MEMORY));
    strcpy(*retStr, tempStr);

  failure:
    return rc;
}
#endif

ClRcT clCpmComponentReport(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClNameT compName;
    ClCharT buffer[100] = "\0";
    ClRcT rc = CL_OK;
    ClTimeT time;
    ClAmsLocalRecoveryT recommendedRecovery;
    ClUint32T alarmHandle = 0;
    ClUint32T length = 0;

    if (!strcasecmp("compReport", argv[0]))
    {
        if (argc != 4)
        {
            sprintf(buffer,
                    "Usage: %s <Component Name> <time> <recommondedRecovery>\n, ",
                    argv[0]);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto level1;
        }

        /*
         * Source Info 
         */
        strcpy(compName.value, argv[1]);
        compName.length = strlen(argv[1]);
        /*
         * with All parameters
         */
        time = (ClTimeT) cpmCliStrToInt(argv[2]);
        recommendedRecovery = (ClAmsLocalRecoveryT) cpmCliStrToInt(argv[3]);

        rc = clCpmComponentFailureReport(0, &compName, time,
                                         recommendedRecovery, alarmHandle);
        if (rc == CL_OK)
            sprintf(buffer, "%s\n", "Sucessfully sent request for the "
                    "failure report command");
        else
            sprintf(buffer, "Failed to report error %x\n", rc);
    }
    else if (!strcasecmp("compClear", argv[0]))
    {
        if (argc != 2)
        {
            sprintf(buffer,
                    "Usage: %s <Component Name>\n",
                    argv[0]);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto level1;
        }
        strcpy(compName.value, argv[1]);
        compName.length = strlen(argv[1]);

        rc = clCpmComponentFailureClear(0, &compName);
        if (rc == CL_OK)
            sprintf(buffer, "%s\n", "Sucessfully sent request for the "
                    "failure clear command");
        else
            sprintf(buffer, "Failed to report clear of an error %x\n", rc);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        sprintf(buffer, "%s", "Unknown Command ............");
    }

  level1:
    length = strlen(buffer) + 1;
    *retStr = (ClCharT *) clHeapAllocate(length);
    if (*retStr != NULL)
        strcpy(*retStr, buffer);
    else
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);

    return rc;
}

ClRcT clCpmCompGet(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClNameT compName = { 0 };
    ClIocAddressT compAddress;
    ClUint32T compId = 0;
    ClCharT buffer[2048] = "\0";
    ClUint32T length = 0;
    ClIocNodeAddressT nodeAddress;

    if (!strcasecmp("compAddressGet", argv[0]))
    {
        if (argc != THREE_ARGUMENT)
        {
            sprintf(buffer, "%s",
                    "Usage: compAddressGet <CompName> <nodeIocAddress>\n"
                    "\tcompName[STRING] - component Name\n"
                    "\tnodeIocAddress[DEC] - node ioc Address, where the component exist\n");
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto done;
        }
        strcpy(compName.value, argv[1]);
        compName.length = strlen(argv[1]);
        nodeAddress = (ClIocNodeAddressT) cpmCliStrToInt(argv[2]);
        rc = clCpmComponentAddressGet(nodeAddress, &compName, &compAddress);
        if (rc == CL_OK)
            sprintf(buffer, "Address %d Port %x\n",
                    compAddress.iocPhyAddress.nodeAddress,
                    compAddress.iocPhyAddress.portId);
        else
            sprintf(buffer, "Failed get component Address, rc = 0x%x", rc);
    }
    else if (!strcasecmp("compIdGet", argv[0]))
    {
        if (argc != TWO_ARGUMENT)
        {
            sprintf(buffer, "%s",
                    "Usage: compIdGet <CompName> \n"
                    "\tcompName[STRING] - component Name\n");
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
           goto done;
        }
        strcpy(compName.value, argv[1]);
        compName.length = strlen(argv[1]);
        rc = clCpmComponentIdGet(0, &compName, &compId);
        if (rc == CL_OK)
            sprintf(buffer, "compId is %d\n", compId);
        else
            sprintf(buffer, "Failed get component Id, rc = 0x%x", rc);
    }
    else if (!strcasecmp("compPIDGet", argv[0]))
    {
        if (argc != TWO_ARGUMENT)
        {
            sprintf(buffer, "%s",
                    "Usage: compPidGet <CompName> \n"
                    "\tcompName[STRING] - component Name\n");
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto done;
        }
        strcpy(compName.value, argv[1]);
        compName.length = strlen(argv[1]);
        rc = clCpmComponentPIDGet(&compName, &compId);
        if (rc == CL_OK)
            sprintf(buffer, "compPId is %d\n", compId);
        else
            sprintf(buffer, "Failed get component PID, rc = 0x%x", rc);
    }
    else if (!strcasecmp("compTraceGet", argv[0]))
    {
        ClPtrT ppOutMem = NULL;
        ClUint32T segmentSize = 0;
        ClFdT fd = 0;
#ifndef POSIX_BUILD
        ClCharT compShmSegment[CL_MAX_NAME_LENGTH];
        segmentSize = getpagesize();
        if (argc != TWO_ARGUMENT)
        {
            sprintf(buffer, "%s",
                    "Usage: compTraceGet <CompName> \n"
                    "\tcompName[STRING] - component Name \n");
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto done;
        }
        strcpy(compName.value, argv[1]);
        snprintf(compShmSegment, sizeof(compShmSegment), "/CL_%s_exception_%d", compName.value, clIocLocalAddressGet());
        rc = clOsalShmOpen(compShmSegment, O_RDONLY, 0777, &fd);
        if(rc == CL_OK)
        {
            rc = clOsalMmap(0, segmentSize, PROT_READ, MAP_PRIVATE, fd, 0, &ppOutMem);
        }
#endif
        if (ppOutMem)
        {
            sprintf(buffer, "%s\n", (ClCharT*)ppOutMem);
            clOsalMunmap(ppOutMem, segmentSize);
            close(fd);
        }
        else
            sprintf(buffer, "%s\n", "Unable to get the component Stack Trace");
    }
  done:
    length = strlen(buffer) + 1;
    *retStr = (ClCharT *) clHeapAllocate(length);
    if (*retStr != NULL)
        strcpy(*retStr, buffer);
    else
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);

    return rc;
}

ClRcT _clCpmComponentListAll(ClInt32T argc, ClCharT **retStr)
{
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T rc = CL_OK, count;
    ClCpmEOListNodeT *eoList = NULL;
    ClCharT state[10] = "\0";
    ClCharT status[10] = "\0";
    ClCharT tempStr[256];
    ClCharT *tmpStr = NULL;
    ClBufferHandleT message;
    ClCharT cpmCompName[CL_MAX_NAME_LENGTH] = {0};
    
    rc = clBufferCreate(&message);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to create message %x\n", rc), rc);

    if (argc != ONE_ARGUMENT)
    {
        sprintf(tempStr, "Usage: compList");
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                 strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message %x\n", rc), rc);
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto done;
    }

    count = gpClCpm->noOfComponent;
    snprintf(cpmCompName,
             CL_MAX_NAME_LENGTH-1,
             "%s_%s",
             CL_CPM_COMPONENT_NAME,
             gpClCpm->pCpmLocalInfo->nodeName);

    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get First component \n"), rc);

    sprintf(tempStr, "%s",
            "################### List Of Components ########################\n");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                             strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                              (ClCntDataHandleT *) &comp);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"), rc);

    sprintf(tempStr, "%s",
            "        CompName              | compId  |  eoPort  |   PID   | RestartCount  | PresenceState\n");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                             strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    sprintf(tempStr, "%s",
            "\t\t     ID |     Port |     Name  |    Health |Recv Threads \n");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                             strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    sprintf(tempStr, "%s",
            "========================================================================================\n");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                             strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    while (count)
    {
        if (!strcmp(comp->compConfig->compName, cpmCompName))
        {
            --count;
            if(count > 0)
            {
                rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to Get Node  Data %x\n", rc),
                             rc);

                rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                          (ClCntDataHandleT *) &comp);
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                             ("Unable to Get Node user Data %x\n", rc), rc);
            }
            continue;
        }

        eoList = comp->eoHandle;
            
        if (comp->compConfig->compProperty != CL_AMS_COMP_PROPERTY_SA_AWARE)
        {
            sprintf(tempStr, "%30s| 0x%x | 0x%x |%8d |%14d |%15s\n",
                    comp->compConfig->compName, comp->compId,
                    comp->eoPort, comp->processId, comp->compRestartCount,
                    _cpmPresenceStateNameGet(comp->compPresenceState));

            if(!eoList || !eoList->eoptr)
            {
                snprintf(tempStr + strlen(tempStr), 
                         sizeof(tempStr) - strlen(tempStr), 
                         "\t\t   0x%x  |  0x%x   |%10s |%10s |%04d \n",
                         0, 0, "-", "-", 0);
            }
        }
        else
        {
            sprintf(tempStr, "%30s| 0x%x | 0x%x |%8d |%14d |%15s\n",
                    comp->compConfig->compName, comp->compId,
                    comp->eoPort, comp->processId, comp->compRestartCount,
                    _cpmPresenceStateNameGet(comp->compPresenceState));
        }
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                 strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

        while (eoList != NULL && eoList->eoptr != NULL)
        {
            compMgrStateStatusGet(eoList->status, eoList->eoptr->state, status, sizeof(status),
                                  state, sizeof(state));
            if (eoList->eoptr->appType == CL_EO_USE_THREAD_FOR_RECV)
                sprintf(tempStr, "\t\t   0x%llx  |  0x%x   |%10s |%10s |%04d \n",
                        eoList->eoptr->eoID, eoList->eoptr->eoPort,
                        eoList->eoptr->name, status,
                        eoList->eoptr->noOfThreads + 1);
            else
                sprintf(tempStr, "\t\t   0x%llx  |  0x%x   |%10s |%10s |%04d \n",
                        eoList->eoptr->eoID, eoList->eoptr->eoPort,
                        eoList->eoptr->name, status,
                        eoList->eoptr->noOfThreads);
            rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                     strlen(tempStr));
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);
            eoList = eoList->pNext;
        }

        count--;
        if (count)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to Get Node  Data %x\n", rc),
                         rc);

            rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                      (ClCntDataHandleT *) &comp);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to Get Node user Data %x\n", rc), rc);
        }
        sprintf(tempStr, "%s",
                "-----------------------------------------------------------------------------------------\n");
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                 strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    }

    /*
     * Bug 4986 :
     * Moved the code to NULL terminate the string
     * below the done: label, so that the usage string
     * written to the buffer is also NULL terminated.
     */
    done:
    /*
     * NULL terminate the string 
     */
    sprintf(tempStr, "%s", "\0");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr, 1);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    rc = clBufferFlatten(message, (ClUint8T **) &tmpStr);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to flatten the message \n"), rc);

    *retStr = tmpStr;
    
    clBufferDelete(&message);
    return rc;

    failure:
    clBufferDelete(&message);
    return rc;
}

ClRcT clCpmComponentListAll(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClRcT rc = CL_OK;

    rc = _clCpmComponentListAll(argc, retStr);

    return rc;
}

ClRcT _cpmClusterConfigList(ClInt32T argc, ClCharT **retStr)
{
    ClCpmLT *cpmL = NULL;
    ClRcT rc = CL_OK;
    ClCntNodeHandleT hNode = 0;
    ClUint32T cpmLCount = 0;
    ClCharT tempStr[256];
    ClCharT *tmpStr = NULL;
    ClBufferHandleT message;

    rc = clBufferCreate(&message);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to create message \n"), rc);

    if (argc != ONE_ARGUMENT)
    {
        sprintf(tempStr, "Usage: clusterList");
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                        strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message %x\n", rc), rc);
        goto done;
    }

    /*
     * Print the local stuff first 
     */
    sprintf(tempStr, "%s\n", "nodeName | status | iocAddress | iocPort ");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                    strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    sprintf(tempStr, "%s\n",
            "-----------------------------------------------------------------------");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                    strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    /*
     * Now even the CPM/G information is stored in the list.
     * So no need of printing the above information.
     */
#if 0
    if (gpClCpm->pCpmLocalInfo->status == CL_CPM_EO_DEAD)
        sprintf(tempStr, "%10s | DEAD | %8d |   0x%x\n",
                gpClCpm->pCpmLocalInfo->nodeName,
                gpClCpm->pCpmLocalInfo->cpmAddress.nodeAddress,
                gpClCpm->pCpmLocalInfo->cpmAddress.portId);
    else
        sprintf(tempStr, "%10s | ALIVE | %8d |   0x%x\n",
                gpClCpm->pCpmLocalInfo->nodeName,
                gpClCpm->pCpmLocalInfo->cpmAddress.nodeAddress,
                gpClCpm->pCpmLocalInfo->cpmAddress.portId);
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                    strlen(tempStr));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);
#endif
    /*_cpmListAllSU();*/
    /*
     * Get all the CPMs one by one and delete the stuff.
     */
    rc = clCntFirstNodeGet(gpClCpm->cpmTable, &hNode);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get first cpmTable Node %x\n", rc),
                 rc);
    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                              (ClCntDataHandleT *) &cpmL);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get container Node data %x\n", rc),
                 rc);
    cpmLCount = gpClCpm->noOfCpm;

    while (cpmLCount)
    {
        if (cpmL->pCpmLocalInfo != NULL)
        {
            if (cpmL->pCpmLocalInfo->status == CL_CPM_EO_DEAD)
            {
                sprintf(tempStr, "%10s | DEAD  | %8d |   0x%x\n",
                        cpmL->nodeName,
                        cpmL->pCpmLocalInfo->cpmAddress.nodeAddress,
                        cpmL->pCpmLocalInfo->cpmAddress.portId);
            }
            else
            {
                sprintf(tempStr, "%10s | ALIVE | %8d |   0x%x\n",
                        cpmL->nodeName,
                        cpmL->pCpmLocalInfo->cpmAddress.nodeAddress,
                        cpmL->pCpmLocalInfo->cpmAddress.portId);
            }
            rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                            strlen(tempStr));
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);
        }
        else
        {
            sprintf(tempStr, "%10s \n", cpmL->nodeName);
            rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                            strlen(tempStr));
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);
        }
#if 0
        rc = clCntFirstNodeGet(cpmL->suTable, &hSU);
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to get first su Node in cpmL %x\n", rc), rc);
        if (cpmL->pCpmLocalInfo != NULL)
            clOsalPrintf("%10s | %8d | %8d | 0x%8x| 0x%8x \n",
                         cpmL->pCpmLocalInfo->nodeName,
                         cpmL->pCpmLocalInfo->status,
                         cpmL->pCpmLocalInfo->nodeId,
                         cpmL->pCpmLocalInfo->cpmAddress.nodeAddress,
                         cpmL->pCpmLocalInfo->cpmAddress.portId);
        else
            clOsalPrintf("%10s \n", cpmL->nodeName);

        suCount = cpmL->noOfsu;
        while (suCount)
        {
            rc = clCntNodeUserDataGet(cpmL->suTable, hSU,
                                      (ClCntDataHandleT *) &su);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to get first container su Node data %x\n",
                          rc), rc);

            clOsalPrintf("\t %10s |%15s |%11s |%16s \n", su->suName,
                         _cpmPresenceStateNameGet(su->suPresenceState),
                         _cpmOperStateNameGet(su->suOperState),
                         _cpmReadinessStateNameGet(su->suReadinessState));
            tempCompRef = su->suCompList;
            while (tempCompRef != NULL)
            {
                clOsalPrintf
                    ("-----------------------------------------------------------------------\n");
                clOsalPrintf("\t\t%10s %14d |%15s |%11s |%16s \n",
                             tempCompRef->ref->compConfig->compName,
                             tempCompRef->ref->compRestartCount,
                             _cpmPresenceStateNameGet(tempCompRef->ref->
                                                      compPresenceState),
                             _cpmOperStateNameGet(tempCompRef->ref->
                                                  compOperState),
                             _cpmReadinessStateNameGet(tempCompRef->ref->
                                                       compReadinessState));
                tempCompRef = tempCompRef->pNext;
            }
            suCount--;
            if (suCount)
            {
                rc = clCntNextNodeGet(cpmL->suTable, hSU, &hSU);
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                             rc);
            }
        }
#endif
        sprintf(tempStr, "%s",
                "-----------------------------------------------------------------------\n");
        rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr,
                                        strlen(tempStr));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);
        cpmLCount--;
        if (cpmLCount)
        {
            rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                         rc);
            rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                                      (ClCntDataHandleT *) &cpmL);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to get container Node data %d\n", rc), rc);
        }
    }

    /*
     * Bug 4986 :
     * Moved the code to NULL terminate the string
     * below the done: label, so that the usage string
     * written to the buffer is also NULL terminated.
     */
  done:
    /*
     * NULL terminate the string 
     */
    sprintf(tempStr, "%s", "\0");
    rc = clBufferNBytesWrite(message, (ClUint8T *) tempStr, 1);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to write message \n"), rc);

    rc = clBufferFlatten(message, (ClUint8T **) &tmpStr);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to flatten message \n"), rc);

    *retStr = tmpStr;

    clBufferDelete(&message);
    return rc;

  failure:
    clBufferDelete(&message);
    return rc;
}

ClRcT clCpmClusterListAll(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClRcT rc = CL_OK;

    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
        rc = _cpmClusterConfigList(argc, retStr);
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("CPM Local Doesn't contain the cluster wide configuration \n"));
        rc = CL_CPM_RC(CL_ERR_BAD_OPERATION);
    }

    return rc;
}

static ClRcT nodeErrorAssert(ClIocNodeAddressT nodeAddress, ClBoolT assertFlag)
{
    ClCmCpmMsgT cpmMsg;
    memset(&cpmMsg, 0, sizeof(cpmMsg));
    cpmMsg.physicalSlot = nodeAddress;
    cpmMsg.cmCpmMsgType = assertFlag ? CL_CM_BLADE_NODE_ERROR_REPORT : CL_CM_BLADE_NODE_ERROR_CLEAR;
    return clCpmHotSwapEventHandle(&cpmMsg);
}

ClRcT clCpmNodeErrorClear(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClUint32T rc = CL_OK;
    if (2 == argc)
    {
        ClIocNodeAddressT nodeAddress = 0;
        nodeAddress = (ClIocNodeAddressT) cpmCliStrToInt(argv[1]);
        if (nodeAddress <= 0) 
        {
            cpmCliPrint(retStr, "Invalid node address [%s]", argv[1]);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = nodeErrorAssert(nodeAddress, CL_FALSE);
        if (CL_OK != rc)
        {
            cpmCliPrint(retStr,
                        "Node error clear failed, error [%#x]",
                        rc);
            goto failure;
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        cpmCliPrint(retStr,
                    "Usage : nodeErrorClear <node address>\n"
                    "\tnode address[in decimal] - IOC/TIPC address "
                    "of the node");
        goto failure;
    }

    return CL_OK;

 failure:
    return rc;
}

ClRcT clCpmNodeErrorReport(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClUint32T rc = CL_OK;
    if (2 == argc)
    {
        ClIocNodeAddressT nodeAddress = 0;
        nodeAddress = (ClIocNodeAddressT) cpmCliStrToInt(argv[1]);
        if (nodeAddress <= 0) 
        {
            cpmCliPrint(retStr, "Invalid node address [%s]", argv[1]);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = nodeErrorAssert(nodeAddress, CL_TRUE);
        if (CL_OK != rc)
        {
            cpmCliPrint(retStr,
                        "Node error report failed, error [%#x]",
                        rc);
            goto failure;
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        cpmCliPrint(retStr,
                    "Usage : nodeErrorReport <node address>\n"
                    "\tnode address[in decimal] - IOC/TIPC address "
                    "of the node");
        goto failure;
    }

    return CL_OK;

 failure:
    return rc;
}

ClRcT clCpmShutDown(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClUint32T rc = CL_OK;

    if (!CL_CPM_IS_ACTIVE())
    {
        cpmCliPrint(retStr,
                    "Node shutdown operation can be performed "
                    "only on CPM/G active");
        rc = CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);
        goto failure;
    }

    if (2 == argc)
    {
        ClIocNodeAddressT nodeAddress = 0;

        nodeAddress = (ClIocNodeAddressT) cpmCliStrToInt(argv[1]);
        if (nodeAddress <= 0) 
        {
            cpmCliPrint(retStr, "Invalid node address [%s]", argv[1]);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clCpmNodeShutDown(nodeAddress);
        if (CL_OK != rc)
        {
            cpmCliPrint(retStr,
                        "Failed to shutdown the node, error [%#x]",
                        rc);
            goto failure;
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        cpmCliPrint(retStr,
                    "Usage : nodeShutdown <node address>\n"
                    "\tnode address[in decimal] - IOC/TIPC address "
                    "of the node");
        goto failure;
    }

    return CL_OK;

 failure:
    return rc;
}


ClRcT clCpmNodeNameGet(ClUint32T argc, ClCharT *argv[], ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClNameT nodeName={0};
    *retStr = NULL;
    if(argc > 1 )
    {
        ClCharT tempStr[0x100];
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        snprintf(tempStr, sizeof(tempStr),
                 "Usage: nodename\n");
        *retStr = (ClCharT *) clHeapAllocate(strlen(tempStr) + 1);
        if (*retStr)
            strcpy(*retStr, tempStr);
        goto out;
    }
    rc = clCpmLocalNodeNameGet(&nodeName);
    if(rc != CL_OK)
    {
        rc = CL_CPM_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    *retStr = clHeapCalloc(1,nodeName.length+1);
    if(!*retStr)
    {
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        goto out;
    }
    strncpy(*retStr,nodeName.value,nodeName.length);
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clCpmHeartbeat(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NO_MEMORY);

    *retStr = clHeapAllocate(50);
    if(!*retStr)
    {
        goto out;
    }
    rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    if(argc != 2)
    {
        strncpy(*retStr,"Usage: heartbeat [enable | disable | status]\n",50);
        goto out;
    }
    if(!strcasecmp(argv[1],"enable"))
    {
        cpmEnableHeartbeat();
    }
    else if(!strcasecmp(argv[1],"disable"))
    {
        cpmDisableHeartbeat();
    }
    else if(!strcasecmp(argv[1],"status"))
    {
        strncpy(*retStr,cpmStatusHeartbeat() ? "enabled" : "disabled", 50);
    }
    else
    {
        strncpy(*retStr,"Usage: heartbeat [enable | disable]\n", 50);
        goto out;
    }
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clCpmLogFileRotate(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NO_MEMORY);

    *retStr = clHeapAllocate(50);
    if(!*retStr)
    {
        goto out;
    }
    rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    if(argc != 1)
    {
        strncpy(*retStr,"Usage: logrotate\n",50);
        goto out;
    }
    cpmLoggerRotateEnable();
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clCpmRestart(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NO_MEMORY);

    *retStr = clHeapAllocate(2*CL_MAX_NAME_LENGTH);
    if (!*retStr)
    {
        goto out;
    }

    rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    if (argc < 2 || argc > 3)
    {
        strncpy(*retStr,
                "Usage : nodeRestart node-address [graceful]\n"
                "        node-address - Address of the node to be restarted\n"
                "        graceful - value > 1 indicates graceful restart\n"
                "                 - value 0 indicates ungraceful restart\n"
                "                 default value is 1\n",
                2*CL_MAX_NAME_LENGTH);
    }
    else if (argc == 2)
    {
        rc = clCpmNodeRestart(cpmCliStrToInt(argv[1]), CL_TRUE);
        if (rc != CL_OK)
        {
            strncpy(*retStr,
                    "Restarting node failed with error [%#x]",
                    rc);
            goto out;
        }
    }
    else if (argc == 3)
    {
        rc = clCpmNodeRestart(cpmCliStrToInt(argv[1]),
                              cpmCliStrToInt(argv[2]) ? CL_TRUE: CL_FALSE);
        if (rc != CL_OK)
        {
            strncpy(*retStr,
                    "Restarting node failed with error [%#x]",
                    rc);
            goto out;
        }
    }

    return CL_OK;
    
out:
    return rc;
}
