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
 * File        : clEventServerFuncTable.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains defination of client-to-server client function tables.
 *****************************************************************************/

#ifndef __CL_EVENT_SERVER_FUNC_TABLE_H__
#define __CL_EVENT_SERVER_FUNC_TABLE_H__

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#include "clEventServerIpi.h"
#include <clVersion.h>

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(evtNativeFuncList)[] = 
{
    VSYM_EMPTY(NULL, EVT_FUNC_ID(0)),                               /* 0 */
    VSYM(clEvtNackSend, EO_CL_EVT_NACK_SEND),                      /* 1 */
    VSYM(clEvtInitializeLocal, EO_CL_EVT_INTIALIZE),               /* 2 */
    VSYM(clEvtChannelOpenLocal, EO_CL_EVT_CHANNEL_OPEN),           /* 3 */
    VSYM(clEvtEventSubscribeLocal, EO_CL_EVT_SUBSCRIBE),           /* 4 */
    VSYM(clEvtEventUnsubscribeAllLocal, EO_CL_EVT_UNSUBSCRIBE),    /* 5 */
    VSYM(clEvtEventPublishLocal, EO_CL_EVT_PUBLISH),               /* 6 */
    VSYM(clEvtEventPublishProxy, EO_CL_EVT_PUBLISH_PROXY),         /* 7 */
#ifdef RETENTION_ENABLE
    VSYM(clEvtQueryRetentionDB, EO_CL_EVT_RETENTION_QUERY),        /* 8 */
#endif
    VSYM(clEvtEventPublishExternal, EO_CL_EVT_PUBLISH_EXTERNAL),               /* 9 */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, EVT)[] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, evtNativeFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
