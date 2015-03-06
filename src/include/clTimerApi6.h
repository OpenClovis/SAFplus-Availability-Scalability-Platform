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
 * ModuleName  : timer
 * File        : clTimerApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * The Timer Service provides timers that allows a user call back
 * function to be invoked when a time out happens. The motivation of the
 * implementation comes from the need to have large number of outstanding
 * timer at any point in time. The timer service library provides a
 * set of API's which are discussed in the following sections.
 *
 *
 *****************************************************************************/

/**
 * \file
 * \brief Timer APIs
 * \ingroup timer_apis
 *
 */

/**
 * \addtogroup timer_apis
 * \{
 */


#ifndef _CL_TIMER_API_H_
#define _CL_TIMER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon6.h>
#include <clBufferApi6.h>

/******************************************************************************
 *  Data Types
 *****************************************************************************/

/**
 * The type fo the callback fucntion that will be called on timer expiry.
 * Cast your function to 'ClRcT (*) (void *)'
 *
 */
typedef ClRcT (*ClTimerCallBackT) (void *);

/*
 *
 * Cluster timer replication callback
 */
typedef ClRcT (*ClTimerReplicationCallbackT) (ClBufferHandleT);

/**
 * The type of the handle identifying the timer.
 *
 *
 */

typedef ClPtrT ClTimerHandleT;

/**
 * The timeout value in seconds and milliseconds.
 */
typedef struct {
    /** Number of seconds of the timeout */
    ClUint32T   tsSec;
    /** Number of Milliseconds.  Its ok for this to be > 1000 (i.e. more than 1 second) */
    ClUint32T   tsMilliSec;
} ClTimerTimeOutT;


/** 
 * It contains the timer library configuration information.  It specifies 
 * the resolution of the timer and the priority of the timer library's task.
 */
typedef struct {
  /** 
   * Timer resolution in milliseconds. It cannot be less than 10ms. 
   * The default is 10 milliseconds. 
   */
  ClUint32T   timerResolution;
  /** 
   * Timer task priority. This value can vary between 1 and 160. 
   * The default value is 150.
   */
  ClUint32T   timerTaskPriority;
} ClTimerConfigT;



/**
 *type of action on timer expiry. repitive start automaticaly starts after timeouts. CL_TIMER_ONE_SHOT:Timer has not started after timeout.
 */
typedef enum {
    /** Fire just once */
    CL_TIMER_ONE_SHOT = 0,
    /** Fire periodically */
    CL_TIMER_REPETITIVE,
    /* Fire once and deleted automatically*/
    CL_TIMER_VOLATILE,
    CL_TIMER_MAX_TYPE,
} ClTimerTypeT;

/**
 * When the timer expires, decides the method of invocation of the timer
 * callback function. Either timer callback function will be called from same 
 * thread context as the timer itself or new separate thread context.
 * Timer task context is slightly more efficient and accurate.  However, if you
 * chose timer task context, your callback function must complete rapidly and
 * not block -- or you will be denying other timer expirations from being 
 * handled.
 * 
 */
typedef enum {
  /** Use the timer thread */
    CL_TIMER_TASK_CONTEXT = 0,
    /** A new thread will be created to invoke the callback. */
    CL_TIMER_SEPARATE_CONTEXT,
    CL_TIMER_MAX_CONTEXT,
} ClTimerContextT;

#define CL_TIMER_TYPE_STR(type) ( (type) == CL_TIMER_ONE_SHOT ? "one shot" : \
                                  ((type) == CL_TIMER_REPETITIVE) ? "repetitive" : "volatile" )

#define CL_TIMER_CONTEXT_STR(ctxt) ( (ctxt) == CL_TIMER_SEPARATE_CONTEXT ? "thread": "inline" )

typedef struct ClTimerStats
{
    ClTimerTypeT type;
    ClTimerContextT context;
    ClTimeT timeOut;
    ClTimeT expiry;
} ClTimerStatsT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \brief Configures the Timer library.
 *
 *  \param pConfigData Pointer to instance of configuration structure.
 *  You must pass a pointer to a \e ClTimerConfigT as the input.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_TIMER_ERR_INVLD_PARAM On passing an invalid parameter.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to configure the timer service library. The configurable
 *  parameters in \e ClTimerConfigT are:
 *   -# <B> timerResolution </B>:
 *  This value is in miliseconds and cannot be less than 10ms. Default value is 10 milliseconds. \n
 *   -# <B> timerTaskPriority </B>:
 *  This value can vary between 1 and 160. Default value is 150.
 *
 */
ClRcT
clTimerConfigInitialize(void* pConfigData);

/**
 ************************************
 *  \brief Initializes the Timer library.
 *
 *  \param pConfig
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval ERROR On failure.
 *
 *  \par Description:
 *  This API is used to initialize the timer service library. After invoking this API
 *  you can create, start, stop and destroy timers.
 *
 *  \sa clTimerFinalize()
 */

ClRcT
clTimerInitialize (ClPtrT pConfig);

/**
 ************************************
 *  \brief Cleans up the Timer library.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval ERROR On failure.
 *
 *  \par Description:
 *  This API is used to clean up the timer service library. This API is invoked
 *  during the system shutdown process or when timers are no longer needed.
 *
 */

ClRcT
clTimerFinalize (void);

/**
 ************************************
 *  \brief Creates a timer.
 *
 *  \param timeOut Timeout value of the timer.
 *
 *  \param type Type of the timer to be created. It can be either One-shot or repetitive.
 *
 *  \param timerTaskSpawn Determines whether the user-function invoked is
 *  in a separate task or in the same context as the timer-task.
 *
 *  \param fpAction Function to be called after timer expiry.
 *
 *  \param pActionArgument Argument to be passed to the callback
 *  function (fpAction in this case).
 *
 *  \param pTimerHandle (out) Pointer to the memory location where the timer handle
 *  created is being copied.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_TIMER_ERR_INVLD_PARAM On passing invalid parameters.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_TIMER_ERR_NULL_CALLBACK_FUNCTION On passing a NULL callback function.
 *  \retval CL_TIMER_ERR_INVALID_TYPE On passing an invalid type.
 *  \retval CL_TIMER_ERR_INVALID_CONTEXT_TYPE On passing an invalid context.
 *
 *  \par Description:
 *  This API is used to create a new timer. This timer would remain inactive
 *  until the timer is started. The callback function would be executed in the
 *  context of the timer task when the timer expires. This API returns a
 *  handle which needs to be specified whenever you want to start, stop, restart or destroy
 *  the timer.
 *
 */

ClRcT
clTimerCreate (ClTimerTimeOutT      timeOut, /* the timeout, in clockticks */
        ClTimerTypeT    type, /* one shot or repetitive */
        ClTimerContextT       timerTaskSpawn, /* whether to spawn off the timer function
                                                 * as a separate task or invoke it in the
                                                 * same context as the timer-task
                                                 */
        ClTimerCallBackT     fpAction, /* the function to be called on timeout */
        void*            pActionArgument, /* the argument to the function called on timeout */
        ClTimerHandleT*      pTimerHandle);/* The pointer to the timer handle */

/**
 ************************************
 *  \brief Deletes a timer.
 *
 *  \note
 *  If the timer being deleted is active, then it is made inactive and deleted.
 *
 *  \param pTimerHandle (out) Pointer to timer handle being deleted. The contents are set to 0.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to delete an existing timer. It is invoked by the
 *  application after the timer has expired.
 *  Typically, this API is invoked during the time of application exit, but
 *  it can also be called at any other time.
 *
 */

ClRcT
clTimerDelete (ClTimerHandleT* pTimerHandle);

ClRcT
clTimerDeleteAsync(ClTimerHandleT *pTimerHandle);

/**
 ************************************
 *  \brief Starts a timer.
 *
 *  \param timerHandle Handle of the timer being started.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to start a timer. Before the timer can be started,
 *  the timer must be created. The callback API
 *  is executed when the timeout occurs. The callback
 *  API would be executed in the context of the timer task.
 *
 */

ClRcT
clTimerStart (ClTimerHandleT  timerHandle);

/**
 ************************************
 *  \breif Stops a timer.
 *
 *  \note
 *  This API only stops the timer and does not destroy it.
 *
 *  \param timerHandle Handle of the timer being stopped.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to stop a timer. After invoking this API, the timer becomes inactive.
 *
 */

ClRcT
clTimerStop (ClTimerHandleT  timerHandle);

/**
 ************************************
 *  \brief Creates a new timer and activates it.
 *
 *  \param timeOut Timeout value of the timer.
 *  \param type Type of the timer to be created. It can be either One-shot or repetitive.
 *  \param timerTaskSpawn Determines whether the user-function invoked is
 *  in a separate task or in the same context as the timer-task.
 *  \param fpAction Function to be called after timer expiry.
 *  \param actionArgument Argument to be passed to the callback function (fpAction in this case).
 *  \param  pTimerHandle (out) Pointer to the memory location where the timer handle
 *  created is being copied.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER  On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_TIMER_ERR_NULL_CALLBACK_FUNCTION On passing an invalid callback function.
 *  \retval CL_TIMER_ERR_INVALID_TYPE If type is invalid.
 *
 *  \par Description:
 *  This API is used to create a new timer and activate it. It is essentially a
 *  combination of clTimerCreate() and clTimerStart().
 *  This API is useful when you want to create a new timer and
 *  activate it at the time of its creation.
 */

ClRcT
clTimerCreateAndStart (ClTimerTimeOutT      timeOut, /* the timeout, in clockticks */
        ClTimerTypeT    type, /* one shot or repetitive */
        ClTimerContextT timerTaskSpawn, /* whether to spawn off the timer function
						      * as a separate task or invoke it in the
						      * same context as the timer-task
						      */
        ClTimerCallBackT    fpAction, /* the function to be called on timeout */
		void                *pActionArgument, /* the argument to the function
						       * called on timeout */
		ClTimerHandleT      *pTimerHandle);

/**
 ************************************
 *  \brief Restarts a timer.
 *
 *  \param timerHandle Handle of the timer being restarted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVLD_STATE If timer is in invalid state.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to restart a timer.
 *
 */

ClRcT
clTimerRestart (ClTimerHandleT  timerHandle);

/**
 ************************************
 *  \brief Updates a timer.
 *
 *  \param timerHandle Handle of the timer being updated.
 *  \param newTimeout New timeout value for the timer.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to update the timeout value of a timer.
 *
 */

ClRcT
clTimerUpdate(ClTimerHandleT timerHandle,
        ClTimerTimeOutT newTimeout);

/**
 ************************************
 *  \brief Returns the timer type.
 *
 *  \param timerHandle Handle of the timer.
 *
 *  \param  pTimerType  (out) The pointer to the location to which the type of the
 *  timer is being copied. It can have two possible values:
 *  If the value is -
 *  \arg  0: The timer type is One-shot.
 *  \arg  1: The timer type is repetitive.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_TIMER_ERR_INVALID If the internal timer representation is invalid.
 *
 *  \par Description:
 *  This API is used to query and return for a specific type of timer, whether it is one-shot or repetitive.
 *
 */

ClRcT
clTimerTypeGet (ClTimerHandleT timerHandle,
        ClUint32T* pTimerType);

ClRcT clTimerClusterRegister(ClTimerCallBackT clusterCallback,
                             ClTimerReplicationCallbackT replicationCallback);

ClRcT clTimerCreateCluster(ClTimerTimeOutT timeOut, 
                           ClTimerTypeT timerType,
                           ClTimerContextT timerContext,
                           void *timerData,
                           ClUint32T timerDataSize,
                           ClTimerHandleT *pTimerHandle);

ClRcT clTimerCreateAndStartCluster(ClTimerTimeOutT timeOut, 
                                   ClTimerTypeT timerType,
                                   ClTimerContextT timerContext,
                                   void *timerData,
                                   ClUint32T timerDataSize,
                                   ClTimerHandleT *pTimerHandle);

ClRcT clTimerClusterPack(ClTimerHandleT timer, ClBufferHandleT msg);

ClRcT clTimerClusterPackAll(ClBufferHandleT msg);

ClRcT clTimerClusterUnpack(ClBufferHandleT msg, ClTimerHandleT *pTimerHandle);

ClRcT clTimerClusterUnpackAll(ClBufferHandleT msg);

ClRcT clTimerClusterFree(ClTimerHandleT *pTimerHandle);

ClRcT clTimerClusterConfigureAll(void);

ClRcT clTimerClusterConfigure(ClTimerHandleT *pTimerHandle);

ClRcT clTimerClusterSync(void);

ClRcT clTimerClusterRegister(ClTimerCallBackT clusterCallback,
                             ClTimerReplicationCallbackT replicationCallback);

ClRcT clTimerIsRunning(ClTimerHandleT timerHandle, ClBoolT *pState);

ClRcT clTimerIsStopped(ClTimerHandleT timerHandle, ClBoolT *pState);

ClRcT clTimerStatsGet(ClTimerStatsT **ppStats, ClUint32T *pNumTimers);

ClRcT clTimerCheckAndDelete(ClTimerHandleT *pTimerHandle);

#ifdef __cplusplus
}
#endif

/**
 * \}
 */

#endif  /* _CL_TIMER_API_H_ */


