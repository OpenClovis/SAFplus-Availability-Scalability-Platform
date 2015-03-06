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
 * File        : clEventLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_EVENT_LOG_H_
# define _CL_EVENT_LOG_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include <clLogApi.h>

# define CL_EVENT_LIB_NAME   "EVT"

    extern const char *clEventLogMsg[];


    /*
     * "Internal Error, rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_1_INTERNAL_ERROR                   clEventLogMsg[0]

    /*
     ** Server specific Log Messages.
     */

    /*
     * "The EO [port=%#x, evtHandle=%#x] is already subscribed with ID
     * [%#x] on Channel [%.*s], rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_6_ALREADY_SUBSCRIBED               clEventLogMsg[1] // NTC

    /*
     * "Check pointing Initialize Failed for EO{port[%#x], evtHandle[%#x]} 
     * rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_CKPT_INIT_FAIL                   clEventLogMsg[2]

    /*
     * "Check pointing Finalize Failed for EO{port[%#x], evtHandle[%#x]}
     * rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_CKPT_FIN_FAIL                    clEventLogMsg[3]

    /*
     * "Check pointing Channel Open Failed for Channel{name[%.*s] handle[%#x]}
     * for EO{port[%#x], evtHandle[%#x]}, rc[%#x]",
     */
# define CL_EVENT_LOG_MSG_6_CKPT_OPEN_FAIL                   clEventLogMsg[4]

    /*
     * "Check pointing Channel Close Failed for Channel [%.*s] & EO [IOC
     * Port=%#x, evtHandle=%#x], rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_5_CKPT_CLOSE_FAIL                  clEventLogMsg[5]

    /*
     * "Check pointing Subscribe [Subscription ID[%#x] Failed on Channel{name[%.*s] handle[%#x]}
     * for EO{port[%#x], evtHandle[%#x]} rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_7_CKPT_SUBS_FAIL                   clEventLogMsg[6]

    /*
     * "Check pointing Unsubscribe [Subscription ID[%#x] Failed on Channel
     * [%.*s] for EO{port[%#x], evtHandle[%#x]} rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_6_CKPT_UNSUB_FAIL                  clEventLogMsg[7]


    /*
     ** Event Server Recovery related messages.
     */

    /*
     * "Attempting Event Server Recovery..." 
     */
# define CL_EVENT_LOG_MSG_0_ATTEMPTING_RECOVERY              clEventLogMsg[8]

    /*
     * "... Event Server Recovery Successful" 
     */
# define CL_EVENT_LOG_MSG_0_RECOVERY_SUCCESSFUL              clEventLogMsg[9]

    /*
     * "Event Server Recovery Failed, rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_1_RECOVERY_FAILED                  clEventLogMsg[10]


    /*
     ** Client Specific Log messages.
     */

    /*
     * "Invalid Flag Passed, flag=[%#x]" 
     */
# define CL_EVENT_LOG_MSG_1_BAD_FLAGS                        clEventLogMsg[11]

    /*
     * "Event Channel [%.*s] already opened by this EO, rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_CHANNEL_ALREADY_OPEN             clEventLogMsg[12]

    /*
     * "Invalid parameter [%s] passed" 
     */
# define CL_EVENT_LOG_MSG_1_INVALID_PARAMETER                clEventLogMsg[13]

    /*
     * "NULL passed for [%s]" 
     */
# define CL_EVENT_LOG_MSG_1_NULL_ARGUMENT                    clEventLogMsg[14]

    /*
     * "Event Initialization Failed for EO{port[%#x], evtHandle[%#x]}
     * rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_INITIALIZATION_FAILED            clEventLogMsg[15]

    /*
     * "Event Finalize Failed for EO{port[%#x], evtHandle[%#x]}
     * rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_FINALIZE_FAILED                  clEventLogMsg[16]

    /*
     * "Event Channel Open Failed for Channel [%.*s], rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_3_CHANNEL_OPEN_FAILED              clEventLogMsg[17]

    /*
     * "Event Channel Close [%.*s] Failed for EO [port=%#x,
     * evtHandle=%#x], rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_5_CHANNEL_CLOSE_FAILED             clEventLogMsg[18]

    /*
     * "Event Subscription [Subscription ID[%#x] Failed on Channel [%.*s] for
     * EO{port[%#x], evtHandle[%#x]} rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_6_SUBSCRIBE_FAILED                 clEventLogMsg[19]

    /*
     * "Event Un-subscription [Subscription ID[%#x] Failed on Channel [%.*s]
     * for EO{port[%#x], evtHandle[%#x]} rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_6_UNSUBSCRIBE_FAILED               clEventLogMsg[20]

    /*
     * "Event Publish Failed on Channel [%.*s] for EO [port=%#x,
     * evtHandle=%#x], rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_5_PUBLISH_FAILED                   clEventLogMsg[21]

    /*
     * "Event Database Cleanup due to the Failure of Node [port=%#x]..." 
     */
# define CL_EVENT_LOG_MSG_2_CPM_CLEANUP                      clEventLogMsg[22]

    /*
     * "Event Database Cleanup due to the Failure of Node [port=%#x]
     * Failed, rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_2_CPM_CLEANUP_FAILED               clEventLogMsg[23]

    /*
     * "Invoking the Async Channel Open Callback ..." 
     */
# define CL_EVENT_LOG_MSG_0_ASYNC_CHAN_OPEN_CALLBACK         clEventLogMsg[24]

    /*
     * "Async Channel Open Successful" 
     */
# define CL_EVENT_LOG_MSG_0_ASYNC_CHANNEL_OPEN_SUCCESS       clEventLogMsg[25]

    /*
     * "Async Channel Open Failed, rc[%#x]" 
     */
# define CL_EVENT_LOG_MSG_1_ASYNC_CHANNEL_OPEN_FAILED        clEventLogMsg[26]

    /*
     * "Invoking the Event Deliver Callback..." 
     */
# define CL_EVENT_LOG_MSG_0_EVENT_DELIVER_CALLBACK           clEventLogMsg[27]

    /*
     * "Queueing the Event Deliver Callback..." 
     */
# define CL_EVENT_LOG_MSG_0_EVENT_QUEUED                     clEventLogMsg[28]

    /*
     * "Queueing the Async Channel Open Callback..." 
     */
# define CL_EVENT_LOG_MSG_0_ASYNC_CHAN_OPEN_CB_QUEUED        clEventLogMsg[29]

    /*
     * "Version( %s ) Not Compatible - Specified{'%c', %#x, %#x} !=
     * Supported{'%c', %#x, %#x}" 
     */
# define CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE             clEventLogMsg[30]

    /*
     * "%s NACK received from Node[%#x:%#x] -
     * Supported Version {'%c', %#x, %#x}"
     */
# define CL_EVENT_LOG_MSG_6_VERSION_NACK                     clEventLogMsg[31]


# ifdef __cplusplus
}
# endif

#endif                          /* _CL_EVENT_LOG_H_ */
