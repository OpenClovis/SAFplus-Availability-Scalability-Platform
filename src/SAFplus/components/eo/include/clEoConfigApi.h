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
 * ModuleName  : eo
 * File        : clEoConfigApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header contains Execution Object [EO] config definitions.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of EO Config Definitions
 *  \ingroup eo_apis
 */

/**
 *  \addtogroup eo_apis
 *  \{
 */




#ifndef _CL_EO_CONFIG_API_H_
#define _CL_EO_CONFIG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>


/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 * The type of the EOId, assigned to an EO as part of registration
 * to the Component Manager.
 */
typedef ClUint64T               ClEoIdT;

/**
 * Maximum length of the name of the EO.
 */
#define CL_EO_MAX_NAME_LEN      32
/**
 * Default number of threads required for an EO.
 */
#define CL_EO_DEFAULT_THREADS   1

/**
 * Gives the name of the Execution Object. 
 */
#define CL_EO_NAME clEoNameGet()

/**
 * Default name of the Execution Object in case EO doesn't
 * provide any name
 */
#define CL_EO_DEFAULT_NAME "3RD_PARTY_COMP"



/******************************************************************************
 *  Data Types
 *****************************************************************************/

/**
 * This is the EO state enumeration.
 */
typedef enum {

/**
 * This indicates the initial state of the EO.
 */
    CL_EO_STATE_INIT        = 0x1,

/**
 * This indicates that the EO is in active state.
 */
    CL_EO_STATE_ACTIVE      = 0x2,

/**
 * This indicates that the EO is in standby state.
 */
    CL_EO_STATE_STDBY       = 0x4,

/**
 * This indicates that the EO is in suspended state.
 */
    CL_EO_STATE_SUSPEND     = 0x8,

/**
 * This indicates that the EO is in stopped state.
 */
    CL_EO_STATE_STOP        = 0x10,

/**
 * This indicates that the EO is in killed state.
 */
    CL_EO_STATE_KILL        = 0x20,

/**
 * This indicates that the state of the EO is resumed from the standby state.
 */
    CL_EO_STATE_RESUME      = 0x40,

/**
 * This indicates that the EO is in failed state.
 */
    CL_EO_STATE_FAILED      = 0x80,

/**
 * This indicates that the EO is thread safe.
 */
    CL_EO_STATE_THREAD_SAFE = 0x100,

/**
 * All states
 */
    CL_EO_STATE_BITS        = 9,


} ClEoStateT;

typedef enum {

/**
 * Use main thread for RMD message receive. If you select this, the main
 * thread must not be blocked in the \e ClEoAppCreateCallbackT and must return
 * immediately. Later, the main thread will be used for RMD message receive.
 */
    CL_EO_USE_THREAD_FOR_RECV   = CL_TRUE,

/**
 * Give main thread to user application. If you select this, the main
 * thread must be blocked in the \e ClEoAppCreateCallbackT or used by the
 * application and must return only when the finalize \e ClEoAppDeleteCallbackT
 * is called.
 */
    CL_EO_USE_THREAD_FOR_APP    = CL_FALSE

}ClEoApplicationTypeT;

typedef enum {

/**
 * This indicated that the Component Manager will stop heartbeat of an EO
 * if \c CL_CPM_DONT_POLL is received in the heartbeat response.
 */
    CL_EO_DONT_POLL    = 0,

/**
 * This indicated that the Component Manager will increase heartbeat
 * timeout to maximum polling timeout.
 */
    CL_EO_BUSY_POLL    = 1,

/**
 * This indicated that the Component Manager will continue with
 * default heartbeat timeout.
 */
    CL_EO_DEFAULT_POLL = 2

} ClEoPollingTypeT;

/**
 * Feedback sent by the software component being polled in response of
 * heartbeat [is-Alive].
 */
typedef struct {

/**
 * This indicates the polling Type \e ClEoPollingTypeT.
 */
    ClEoPollingTypeT   freq;

/**
 * This indicates the health of the EO.
 */
    ClRcT              status;

}ClEoSchedFeedBackT;

/**
 * The application should initialize itself in this function callback.
 * Implementation of this is mainly dependent on the
 * \e ClEoApplicationTypeT selected by you.
 */
typedef ClRcT  (* ClEoAppCreateCallbackT)(

/**
 * This indicates the count of arguments passed.
 */
        CL_IN ClUint32T argc,

/**
 * This indicates the array of pointer pointing to the arguments list for the function.
 */
        CL_IN char *argv[]);



/**
 * The application should change the state the service it is providing.
 * This service can be provided by the following mechanisms: \n
<BR>
 * \par 1. Using EO interface:
 *      It is achieved in EO library [by masking the RMD received calls].
 *      You need not specify anything in this callback. \n
<BR>
 * \par 2. By some other interface:
 *      You must fill in this callout so that it can be called whenever
 *      state change is required.
 *
 */
typedef ClRcT (* ClEoAppStateChgCallbackT)(

/**
 * This indicates the state in which application moves.
 */
    CL_IN ClEoStateT state);

/**
 * The application performs cleanup in this function callback.
 */
typedef ClRcT   (* ClEoAppDeleteCallbackT)();

/**
 * The application checks the health status in this function callback.
 */
typedef ClRcT   (* ClEoAppHealthCheckCallbackT)(

/**
 * This needs to be filled-in by you in response of a healthCheck.
 */
        CL_OUT ClEoSchedFeedBackT  *schFeedback);

/**
 * The application performs custom action in this callback
 */
typedef ClRcT (*ClEoCustomActionT)(ClCompIdT compId, ClWaterMarkIdT wmId, 
        ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList);

/**
 * This structure is passed during the clEoCreate API and contains the EO related
 * configuration parameters.
 */
typedef struct {

/**
 * Indicates the EO thread priority.
 */
    ClOsalThreadPriorityT   pri;

/**
 * Indicates the number of RMD threads.
 */
    ClUint32T               noOfThreads;

/**
 * The requested IOC communication port.
 */
    ClIocPortT              reqIocPort;

/**
 * Indicates the maximum number of EO client.
 */
    ClUint32T               maxNoClients;

/**
 * Indicates whether the application needs main thread or not.
 */
    ClEoApplicationTypeT    appType;

/**
 * Application function that is called from main during the
 * initialization process.
 */
    ClEoAppCreateCallbackT  clEoCreateCallout;

/**
 * Application function that is called when EO needs to be
 * terminated.
 */
    ClEoAppDeleteCallbackT  clEoDeleteCallout;

/**
 * Application function that is called when EO is moved into
 * the suspended state.
 */
    ClEoAppStateChgCallbackT    clEoStateChgCallout;

/**
 * Application function that is called when EO healthcheck is
 * performed by Component Manager.
 */
    ClEoAppHealthCheckCallbackT clEoHealthCheckCallout;

/**
 * Application function that has to be called when a Water Mark
 * has been hit.
 */
    ClEoCustomActionT clEoCustomAction;

    ClBoolT needSerialization;

}ClEoConfigT;

/**
 * Gives the name of the executable for the EO. 
 */
ClRcT clEoProgNameGet(ClCharT *pName,ClUint32T maxSize);

/**
 * Gives the name of the EO. 
 */
ClCharT* clEoNameGet(void);

/***********************************************************************/
/*                      external symbols we expect here                */
/***********************************************************************/

/*
 * This is the configuration for an EO. Since we can only do one EO
 * per process in RC1, this can be here. But when support for multiple
 * EOs per process shows up, this has to be fixed in some way.
 */

extern ClEoConfigT clEoConfig;

extern ClCharT *clEoProgName;

#ifdef __cplusplus
}

#endif

#endif /* _CL_EO_CONFIG_API_H_ */

/** \} */

