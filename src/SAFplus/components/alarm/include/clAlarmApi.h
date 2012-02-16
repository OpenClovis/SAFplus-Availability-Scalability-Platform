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
 * ModuleName  : alarm
 * File        : clAlarmApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module contains Alarm Service related APIs
 *
 *****************************************************************************/


#ifndef _CL_ALARM_API_H_
#define _CL_ALARM_API_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <clCommon.h>
#include <clAlarmDefinitions.h>
#include <clAlarmOMIpi.h>

/**
 * \file 
 * \brief Header file of Alarm Service related APIs
 * \ingroup alarm_apis
 */

/**
 *  \addtogroup alarm_apis
 *  \{
 */

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/


/*****************************************************************************
 *  Alarm Management functions
 *****************************************************************************/
 
/**
 *****************************************
 *  \brief Initializes the Alarm Library.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_CLIENT_ERR_INIT_FAILED If alarm service initialization failed.
 *
 *  \par Description:
 *  This function is used to initialize the alarm client library and start up the alarm
 *  related services for a given component. It initializes the various callbacks
 *  and allocates resources. This function must be invoked before invoking any other 
 *  alarm-related function.
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa clAlarmLibFinalize()
 *
 */

ClRcT clAlarmLibInitialize();

/********************************************************************/


/**
 *****************************************
 *  \brief Cleans up the Alarm Library.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters:
 *  None.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_CLIENT_ERR_FINALIZE_FAILED If alarm service finalization failed.
 *
 *  \par Description:
 *  This function is used to clean-up alarm library linked to the component and
 *  frees resources allocated to it at the time of initialization. It
 *  is called when the alarm-related services are no longer required.
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa clAlarmLibInitialize()
 *
 *
 */
ClRcT clAlarmLibFinalize();

/********************************************************************/


/**
 ************************************
 *  \brief Raises an alarm on a component.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \param alarmInfo (in)Pointer to the structure that contains the alarm information.
 *
 *  \param pAlarmHandle (out)Pointer to the alarm handle. The alarm handle is used by Fault Manager(FM) 
 *  to query alarm information and pass the alarm information to the fault repair handlers configured
 *  by the user.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_ERR_INVALID_MOID On passing invalid \e MoId, the Managed Object Idetifier.
 *  \retval CL_ALARM_ERR_NO_OWNER The Managed object passed to the alarm raise doen't have any owner registered in COR.
 *  \retval CL_ALARM_ERR_NULL_POINTER On passing either pAlarmInfo or the pAlarmHandle as NULL. 
 *  \retval CL_ALARM_ERR_INVALID_PARAM On passing the length of the user buffer (pAlarmInfo->len) 
*   as negative or the probable cause (pAlarmInfo->probCause) is not configured.
 *  \retval CL_ALARM_ERR_NO_MEMORY There is no memory left in this appication to raise the alarm.
 *
 *  \par Description:
 *  This function is used to generate an alarm on a Managed Object (MO). It can be used
 *  by a component to raise an alarm on a resource or on a component itself.
 *  While invoking this function, you must fill the \e ClAlarmInfoT structure
 *  which has the following attributes:
 *
 *  \arg \e moId : The resource on which the alarm is being raised.
 *  \arg \e compName : The name of component which is raising the alarm.
 *  \arg \e alarmState : Flag to indicate if the alarm was for assert or for clear.
 *  \arg \e probCause : Probable cause of the alarm being raised.
 *  
 *  There are certain optional fields of the \e ClAlarmInfoT structure which can be
 *  provided while publishing the alarm:
 *  \arg \e category: The category of the alarm to which it belongs.
 *  \arg \e specificProblem: Specific problem of the alarm. This is to be specified by user.
 *  \arg \e severity : Severity of the alarm.
 *  \arg \e buff : Additional information about the alarm to be specified by the user. 
 *                 This buffer would be allocated by the user and freed after calling 
 *                 this function.
 *  \arg \e len : Length of the additional data about the alarm.
 *
 *  A unique alarm handle is generated by the alarm server and provided back to the component
 *  that raises the alarm. The alarm handle is used by FM to query alarm information
 *  and pass the alarm information to the repair handlers configured while modeling.
 *
 *  \par Library File:
 *  ClAlarmClient
 */

ClRcT clAlarmRaise(CL_IN ClAlarmInfoT* pAlarmInfo,CL_OUT ClAlarmHandleT *pAlarmHandle);


/**
 ************************************************
 *  \brief  This function is used to verify if a version of the
 *  function is supported by AM.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \param version (in/out) As an input parameter, this contains the version
 *  you require to query or the current version. The
 *  output parameter is the Version supported.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_ERR_NULL_POINTER   The null pointer is passed for the version parameter.
 *  \retval CL_ALARM_ERR_VERSION_UNSUPPORTED If current version of this
 *   function is not supported.
 *
 *  \par Description:
 *  This function is used to verify if a version is supported or not. In case
 *  the version is not supported, then an error is returned with
 *  the value of supported version filled in the output parameter.
 *
 *  \par Library File:
 *  ClAlarmClient
 */
ClRcT
clAlarmVersionVerify( CL_INOUT ClVersionT *pVersion);

/**
 ************************************************
 *  \brief This function is used to get the decoded data 
 *   in host format of the alarm data obtained after receiving the event.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \param pData (in)The data received through the event on the alarm event channel.
 *  \param size  (in)The size of pData.
 *  \param pAlarmHandleInfo (out)This parameter is filled with the decoded data into alarm info.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_ERR_NULL_POINTER On passing null pointer in pData or pAlarmHandleInfo.
 *
 *  \par Description:
 *  The Alarm Server encodes the alarm information into network byte format before 
 *  publishing the event. This function is used to get the decoded alarm information
 *  into host byte format in the events delivery callback function. 
 *      Before using this function user needs to get the data and its size from the event
 *   client using the clEventDataGet() API and pass that information to this function. This function 
 *   converts the encoded data into host format and fills that into pAlarmHandleInfo parameter.
 *
 *  \par Library File:
 *  ClAlarmClient
 * 
 */
ClRcT
clAlarmEventDataGet(CL_IN ClUint8T *pData, CL_IN ClSizeT size, CL_OUT ClAlarmHandleInfoT *pAlarmHandleInfo);

/**
 ************************************************
 *  \brief Informs the consumer of the event about the events published by the alarm server. 
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \param  pAlarmInfo  (in) pointer to the ClAlarmInfoT structure. This structure will 
 *                           populated by the alarm client upon receiving the event on 
 *                           CL_ALARM_EVENT_CHANNEL and then the callback function will
 *                           called.
 *  \retval  CL_OK The callback function successfully executed. 
 *
 *  \par Description:
 *  This function is registered by using the function clAlarmEventSubscribe in order to receive the
 *  events published by the alarm server. The alarm client listens for all the events published on 
 *  event channel CL_ALARM_EVENT_CHANNEL by the alarm server. When the event is published, the alarm 
 *  client would call this registered function.
 *
 *  \par Library File:
 *  ClAlarmClient
 * 
 *  \sa
 *   clAlarmEventSubscribe
 */
typedef ClRcT (*ClAlarmEventCallbackFuncPtrT) (CL_IN const ClAlarmHandleInfoT* pAlarmInfo);

/**
 ************************************************
 *  \brief This function is used to subscribe for the events 
 *         published by the alarm server.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \param pAlarmEvtCallbackFuncFP (in) Pointer to the function which would be 
 *  called once the event is published by the alarm server.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_ERR_NULL_POINTER The pointer to the function passed is NULL.
 *  \retval CL_ALARM_ERR_EVT_INIT Failed while initializing the event handle.
 *  \retval CL_ALARM_ERR_EVT_CHANNEL Failed while opening the alarm event channel.
 *  \retval CL_ALARM_ERR_EVT_SUBSCRIBE Failed while setting the alarm event patten in subscribe.
 *  \retval CL_ALARM_ERR_EVT_SUBSCRIBE_AGAIN Failed while doing duplicate subscription.
 *
 *  \par Description:
 *  This function is used to subscribe for the events published by the alarm 
 *  server. This function takes the function pointer as parameter which would
 *  be called whenever the event is published by the alarm server on the event 
 *  channel CL_ALARM_EVENT_CHANNEL. After receiving the event the callback function 
 *  would be called after filling the alarm information in the ClAlarmHandleInfoT 
 *  structure.
 *  The event subscription done via this function can be unsubscribed using the 
 *  clAlarmEventUnsubscribe().
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa
 *  clAlarmEventUnsubscribe()
 */
ClRcT
clAlarmEventSubscribe( CL_IN const ClAlarmEventCallbackFuncPtrT pAlarmEvtCallbackFuncFP);


/**
 ************************************************
 *  \brief This function is used to unsubscribe for the events 
 *         published by the alarm server.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters 
 *  None.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_ALARM_ERR_EVT_CHANNEL_CLOSE The function failed while closing the event channel.
 *  \retval CL_ALARM_ERR_EVT_FINALIZE The function failed while doing event finalize.
 *
 *  \par Description:
 *  This function is used to unsubscribe for the event published by the alarm
 *  server on the channel CL_ALARM_EVENT_CHANNEL. After this function is called,  
 *  the callback registered by the function clAlarmEventSubscribe will not be called      
 *  any further on the event publish. This function will finalize the event handle
 *  and will close the event channel opened by calling clAlarmEventSubscribe().
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa
 *  clAlarmEventSubscribe()
 */
ClRcT 
clAlarmEventUnsubscribe();

/**
 ****************************************************************
 *  \breif  Function to get the current state of the alarm.
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters
 *  \param pAlarmInfo (in) : Pointer to the structure of type ClAlarmInfoT. This structure
 *                      should be populated with the resource MOID and the alarm
 *                      information like probable cause, category, severity and specific
 *                      problem of the alarm.
 *  \param pAlarmState (out) : Pointer to the ClAlarmStateT. On successful return, the function
 *                             would populate the current state of the alarm in this parameter.
 *
 *  \retval CL_OK                       : The function completed successfully.
 *  \retval CL_ERR_NOT_EXIST            : The alarm information doesn't exit on the managed resource.
 *  \retval CL_ALARM_ERR_INVALID_MOID   : The MOID passed is invalid that is no object exist for this MOID in COR.
 *  \retval CL_ALARM_ERR_NO_OWNER       : There is no registered owner for the alarm resource.
 *  \retval CL_ERR_NULL_POINTER         : NULL pointer passed for either pAlarmInfo or pAlarmState.
 *  \retval CL_ERR_NO_MEMORY            : No memory avaible.
 *  
 *  \par Description
 *  The function is used to get the latest state of any alarm. The function takes the alarm information
 *  that is the resource MOID, probable cause, specific problem and category. After successful return
 *  the pAlarmState would contain the latest state of the alarm provided.
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa 
 *  clAlarmPendingAlarmsQuery()
 */
ClRcT 
clAlarmStateQuery (CL_IN const ClAlarmInfoT *pAlarmInfo,
                   CL_OUT ClAlarmStateT * const pAlarmState);


/**
 ***************************************************************************************
 *  \breif Function to get the pending alarms on a specific object or in the whole system.
 *  
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters
 *  \param pMoId (in)               :   It is the pointer to the resource MOID for which the query for 
 *                                      all the alarms needs to be done. If NULL is passed, then the function
 *                                      will return all the pending alarms in the system.
 *  \param pPendingAlmList (out)    :   It is a pointer to the structure of type ClAlarmPendingAlmListT. After
 *                                      the api returns successfully, this parameter will contain the count and
 *                                      the array of pending alarms information which is of type ClAlarmInfoT.
 *
 *  \retval CL_OK                   : The function got executed successfully.
 *  \retval CL_ALARM_ERR_NO_OWNER   : There is no registered owner for the alarm resource.
 *  \retval CL_ERR_NULL_POINTER     : NULL pointer passed for either pMoId or pPendingAlmList.
 *  \retval CL_ERR_NO_MEMORY        : No memory avaible.
 *
 *  \par Description
 *  The function is used to get list of all the pending alarms. The function is either called with a specific
 *  resource MOID for getting all the pending alarms on that managed object, or it can be used for getting 
 *  all the pending alarms present in the system. For getting all the pending alarms in the system, the first
 *  parameter should be given as NULL.
 *  The function would fill all the pending alarms in the second parameter. It would fill the count of pending
 *  alarms in pPendingAlarms->noOfPendingAlarm and the list of pending alarms in the pPendingAlarms->pAlarmList.
 *  The pAlarmList is array of ClAlarmHandleInfoT, so it contains alarm handle and other parameters of the alarm.
 *  The alarm handle for the alarm which was raised successfully would be a valid handle generated by alarm server.
 *  The alarms which were suppressed, masked or ones for which generation rule didn't satisfy, the alarm handle
 *  will be a constant equal to CL_HANDLE_INVALID_VALUE.
 *
 *  \par Note:
 *  The user need to call the function clAlarmPendingAlarmListFree() in order to free the list of pending alarms.
 *
 *  A pending alarm is the one which is either in assert state (or after soaking), under soaking state, 
 *  or in suppressed state. The ClAlarmStateT describes all the possible state of an alarm.
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa 
 *  clAlarmPendingAlarmListFree()
 */
ClRcT 
clAlarmPendingAlarmsQuery (CL_IN ClCorMOIdPtrT const pMoId, 
                           CL_OUT ClAlarmPendingAlmListPtrT const pPendingAlmList);


/**
 ****************************************************************
 *  \breif  Function to free the pending alarm list populated by clAlarmPendingAlarmsQuery.
 *  
 *
 *  \par Header File:
 *  clAlarmApi.h
 *
 *  \par Parameters
 *  pPendingAlmList (in out) : The pointer to the structure containing the list of 
 *                             pending alarms.
 *
 *  \retval     CL_OK               : The function exection was successful.
 *  \retval     CL_ERR_NULL_POINTER : The pointer is passed to the function.
 *
 *
 *  \par Description
 *  The function should be used to free the pending alarm list returned by the 
 *  clAlarmPendingAlarmsQuery () function. 
 *
 *
 *  \par Library File:
 *  ClAlarmClient
 *
 *  \sa 
 *  clAlarmPendingAlarmsQuery()
 */
ClRcT
clAlarmPendingAlarmListFree(CL_INOUT ClAlarmPendingAlmListPtrT const pPendingAlmList);
/***************************************************************************/

ClRcT clAlarmConfigDataGet(ClCorMOIdPtrT pMoId, ClAlarmInfoT** ppAlarmInfo, ClUint32T* pCount);

ClRcT clAlarmReset(ClCorMOIdPtrT pMoId, VDECL_VER(ClAlarmIdT, 4, 1, 0)* pAlarmId, ClUint32T count);

#ifdef __cplusplus
}
#endif

#endif /* _CL_ALARM_API_H_ */

/** \} */
