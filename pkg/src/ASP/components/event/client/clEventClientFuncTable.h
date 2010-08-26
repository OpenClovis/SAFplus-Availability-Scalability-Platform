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
 * File        : clEventClientFuncTable.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains defination of server-to-client client function tables.
 *****************************************************************************/

#ifndef __CL_EVENT_CLIENT_FUNC_TABLE_H__
#define __CL_EVENT_CLIENT_FUNC_TABLE_H__

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#include "clEventClientIpi.h"
#include <clEventCommonIpi.h>


#ifdef __cplusplus
extern "C" {
#endif

    CL_EO_CALLBACK_TABLE_DECL(evtCltNativeFuncList)[] = 
    {
        VSYM_EMPTY(NULL, EVT_FUNC_ID(0)),                               /* 0 */
        VSYM(clEvtEventReceive, CL_EVT_EVENT_DELIVERY_FN_ID),          /* 1 */
        VSYM_NULL,
    };

    CL_EO_CALLBACK_TABLE_LIST_DECL(gAspCltFuncTable, EVT)[] =
    {
        CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_EVT_CLIENT_TABLE_ID, evtCltNativeFuncList),
        CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
    };

#ifdef __cplusplus
}
#endif

#endif
