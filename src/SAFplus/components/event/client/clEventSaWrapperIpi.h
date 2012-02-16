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
 * ModuleName  :                                                          
File        : clEventSaWrapperIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header file contains the structures internal to the
 *          Saf Wrapper.
 *
 *
 *****************************************************************************/

#ifndef _CL_EVENT_SA_WRAPPER_IPI_H_
# define _CL_EVENT_SA_WRAPPER_IPI_H_

# ifdef __cplusplus
extern "C"
{
# endif

# define SA_EVT_INIT_INFO_HANDLE 1
# define SA_EVT_CHANNEL_INFO_HANDLE 2 

typedef struct {
    ClUint8T handleType;

} SaEvtHdlDbDataT;

typedef struct {
    ClUint8T handleType;
    SaEvtCallbacksT  saEventCallbacks;

} SaEvtInitInfoT;

typedef struct {
    ClUint8T handleType;
    SaEvtHandleT evtHandle;

} SaEvtChanInitInfoT;

typedef struct {
    SaEvtHandleT evtHandle;
    SaInvocationT invocation;
} SaEvtInvocationInfoT;

typedef struct {
    SaEvtHandleT evtHandle;

} SaEvtSubscribeCookieT;

# ifdef __cplusplus
}
# endif

#endif     /*_CL_EVENT_SA_WRAPPER_IPI_H_*/               

