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
 * ModuleName  : event
 * File        : clEvtLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains the definitions needed by the  
 *          EM Modules for logging.
 *****************************************************************************/


#include "clEventLog.h"


const char *clEventLogMsg[] = {

    "Internal Error, rc[%#x]",
    "The EO{port[%#x], evtHandle[%#llx]} is already subscribed with ID[%#x] on Channel[%.*s], rc[%#x]",
    "Check pointing Initialize Failed for EO{port[%#x], evtHandle[%#llx]}, rc[%#x]",
    "Check pointing Finalize Failed for EO{port[%#x], evtHandle[%#llx]}, rc[%#x]",
    "Check pointing Channel Open Failed for Channel{name[%.*s] handle[%#llx]} for EO{port[%#x], evtHandle[%#llx]}, rc[%#x]",
    "Check pointing Channel Close Failed for Channel [%.*s] & EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Check pointing Subscribe [Subscription ID[%#x] Failed on Channel{name[%.*s] handle[%#llx]} for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Check pointing Unsubscribe [Subscription ID[%#x] Failed on Channel [%.*s] for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Attempting Event Server Recovery...",
    "... Event Server Recovery Successful",
    "Event Server Recovery Failed, rc[%#x]",
    "Invalid Flag Passed, flag[%#x]",
    "Event Channel [%*.s] already opened by this EO, rc[%#x]",
    "Invalid parameter [%s] passed",
    "NULL passed for [%s]",
    "Event Initialization Failed for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Finalize Failed for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Channel Open Failed for Channel [%.*s], rc[%#x]",
    "Event Channel Close [%.*s] Failed for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Subscription [Subscription ID[%#x] Failed on Channel [%.*s] for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Un-subscription [Subscription ID[%#x] Failed on Channel [%.*s] for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Publish Failed on Channel [%.*s] for EO{port[%#x], evtHandle[%#llx]} rc[%#x]",
    "Event Database Cleanup due to the Failure of Node port[%#x]...",
    "Event Database Cleanup due to the Failure of Node port[%#x] Failed, rc[%#x]",
    "Invoking the Async Channel Open Callback ...",
    "Async Channel Open Successful",
    "Async Channel Open Failed, rc[%#x]",
    "Invoking the Event Deliver Callback...",
    "Queueing the Event Deliver Callback...",
    "Queueing the Async Channel Open Callback...",
    "Version( %s ) Not Compatible - Specified{'%c', %#x, %#x} != Supported{'%c', %#x, %#x}",
    "%s NACK received from Node[%#x:%#x] - Supported Version {'%c', %#x, %#x}",
};
