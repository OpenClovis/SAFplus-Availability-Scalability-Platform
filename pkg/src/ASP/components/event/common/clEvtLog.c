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
 * ModuleName  : event
 * File        : clEvtLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains the definitions needed by the  
 *          EM Modules for logging.
 *****************************************************************************/


#include "clEventLog.h"


ClCharT *clEventLogMsg[] = {

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
