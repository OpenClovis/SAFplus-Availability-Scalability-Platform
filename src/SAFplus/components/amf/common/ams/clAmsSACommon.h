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
 * File        : clAmsSACommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the function call types used by the client library to
 * call the server and for the server to call its peer. 
 *****************************************************************************/

#ifndef _CL_AMS_SA_COMMON_H_
#define _CL_AMS_SA_COMMON_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>

#include <clAmsTypes.h>
#include <clAmsEntities.h>

/*
 * Function IDs to identify client callback function that the server needs to
 * call.
 */
typedef enum
{
    CL_AMS_CLIENT_RESERVED                              = 0,
    CL_AMS_CLIENT_INITIALIZE                            = 1,
    CL_AMS_CLIENT_FINALIZE                              = 2,
    CL_AMS_CLIENT_CSI_HA_STATE_GET                      = 3,
    CL_AMS_CLIENT_CSI_QUIESCING_COMPLETE                = 4,
    CL_AMS_CLIENT_PG_TRACK                              = 5,
    CL_AMS_CLIENT_PG_TRACK_STOP                         = 6,
    CL_AMS_CLIENT_RESPONSE                              = 7

} ClAmsClientCallbackRmdInterfaceT;

/*
 * Default RMD behavior for AMS
 */
#define CL_AMS_RMD_DEFAULT_TIMEOUT        10000  
#define CL_AMS_RMD_DEFAULT_RETRIES          3

/*-----------------------------------------------------------------------------
 * Type definitions for RMD client --> server function arguments:
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClAmsClientHandleT handle;
} clAmsClientInitializeRequestT;

typedef struct
{
    ClAmsClientHandleT handle;
} clAmsClientInitializeResponseT;

typedef struct
{
    ClAmsClientHandleT handle;
} clAmsClientFinalizeRequestT;

typedef struct
{
    int nothing;
} clAmsClientFinalizeResponseT;
 
typedef struct
{
    ClAmsClientHandleT      handle;
    ClNameT                 compName;             
    ClNameT                 csiName;             
} clAmsClientCSIHAStateGetRequestT;

typedef struct
{
    ClAmsHAStateT       haState;
} clAmsClientCSIHAStateGetResponseT;

 
typedef struct
{
    ClAmsClientHandleT      handle;
    ClInvocationT           invocation;             
    ClRcT                   error;             
} clAmsClientCSIQuiescingCompleteRequestT;

typedef struct
{
    int nothing;
} clAmsClientCSIQuiescingCompleteResponseT;

 
typedef struct
{
    int nothing;
} clAmsClientPGTrackRequestT;

typedef struct
{
    int nothing;
} clAmsClientPGTrackResponseT;

typedef struct
{
    ClAmsClientHandleT handle;
    const ClNameT    *csiName;
} clAmsClientPGTrackStopRequestT;

typedef struct
{
    int nothing;
} clAmsClientPGTrackStopResponseT;

typedef struct
{
    ClAmsClientHandleT      handle;
    ClInvocationT           invocation;             
    ClRcT                   error;             
} clAmsClientResponseRequestT;

typedef struct
{
    int nothing;
} clAmsClientResponseResponseT;

#ifdef  __cplusplus
}
#endif

#endif							/* _CL_AMS_SA_COMMON_H_ */ /*ifdef _CL_AMS_SA_COMMON_H_ */
