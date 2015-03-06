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
 * File        : clEventExtApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Event Manager Related APIs
 *  \ingroup event_apis
 */

/**
 ************************************
 *  \addtogroup event_apis
 *  \{
 */


#ifndef _CL_EVENT_EXT_API_H_
# define _CL_EVENT_EXT_API_H_


# ifdef __cplusplus
extern "C"
{
# endif

# include "clCommon.h"
# include "clCommonErrors.h"
# include "clOsalApi.h"
# include "clCntApi.h"
# include "clCommon.h"
# include "clEventErrors.h"
# include "clEventApi.h"

/**
 *  This is extension to SAF filter ClEventFilterTypeT to provide non-zero matching.
 */
# define CL_EVENT_NON_ZERO_MATCH CL_EVENT_PASS_ALL_FILTER+1

/*******************************************************************************
NOTE:-
    The following table depicts the mapping between the normal and wrapper API. 
    The wrapper API is obtained by inserting 'Ext' in the corresponding API. 

    clEventSubscribe        - clEventExtSubscribe
    clEvtEventAttributeSet  - clEvtEventExtAttributeSet
    clEvtEventAttributeGet  - clEvtEventExtAttributeGet
*******************************************************************************/

/**
 ************************************
 *  \brief Subscribes to an event identified by event type (constant integer instead of filter).
 * 
 *  \par Header File:
 *  clEventExtApi.h
 *
 *  \param channelHandle Handle of the event channel on which the EO is subscribing
 *   to receive events. This parameter must be obtained earlier by \e clEventChannelOpen API
 *   or \e clEvtChannelOpenCallback() function.
 *   
 *  \param eventType Event type within event channel.
 *
 *  \param pSubscriptionId Identifies a specific subscription on this instance of the opened 
 *   event channel corresponding to the \e channelHandle and that is used as a parameter of 
 *   \e clEvtEventDeliverCallback().
 *
 *  \param pCookie Cookie passed by you, which can be sent back by the
 *  \e clEvtEventUtilsCookiGet() function. 
 *  
 *  \retval CL_OK The API completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE On initialization failure. 
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION The channel, identified by 
 *   channelHandle, was not opened with the \c CL_EVENT_CHANNEL_SUBSCRIBER
 *   flag set, i.e, the event channel was not opened for subscriber access.
 *
 *  \par Description:
 *   This function serves as a wrapper around \e clEventSubscribe() and enables an EO to subscribe for events on an event channel   *   by registering a fixed length
 *   event type on that event channel instead of filters. 
 *  \par
 *   Events are delivered via the invocation of the \e clEvtEventDeliverCallback() callback
 *   function supplied when the EO called the \e clEventInitialize() function.
 *  \par
 *   The EO must open the event channel, designated by channelHandle, with the 
 *   \c CL_EVENT_CHANNEL_SUBSCRIBER flag set for successful execution of this function.
 *   The memory associated with the filters is not deallocated by the
 *   clEventExtSubscribe function. It is the responsibility of the invoking EO to deallocate
 *   the memory when the \e clEventExtSubscribe function returns.
 *  \par
 *   For a given subscription, the filters parameter cannot be modified. To change the filters
 *   parameter without losing events, an EO must establish a new subscription
 *   with the new filters parameter. Once a new subscription is established, the old 
 *   subscription can be removed by invoking the \e clEventUnsubscribe() function.
 * 
 *  \par Library File:
 *  ClEventClient
 * 
 *  \sa clEventSubscribe(), clEventExtWithRbeSubscribe()
 *            
 */

    ClRcT clEventExtSubscribe(CL_IN ClEventChannelHandleT channelHandle,
                              CL_IN ClUint32T eventType,
                              CL_IN ClEventSubscriptionIdT subscriptionId,
                              CL_IN void *pCookie);





/**
 ************************************
 *  \brief Subscribes to an event identified by an RBE.
 * 
 *  \par Header File:
 *  clEventExtApi.h
 *
 *  \param channelHandle Handle of the event channel on which the EO is subscribing
 *   to receive events. This parameter must be obtained earlier by \e clEventChannelOpen() API
 *   or \e clEvtChannelOpenCallback() function.
 *   
 *  \param pRbeExpr Pointer to RBE expression, allocated by the EO, that defines filter patterns 
 *   using RBE expression. The EO may deallocate the memory for the filters when 
 *   clEventExtWithRbeSubscribe returns.
 *   
 *  \param pSubscriptionId Identifies a specific subscription on this instance of the opened 
 *   event channel corresponding to the \e channelHandle and that is used as a parameter of 
 *   \e clEvtEventDeliverCallback() function.
 *
 *  \param pCookie Cookie passed by you, which can be given back by the
 *  \e clEvtEventUtilsCookiGet() function. 
 *  
 *  \retval CL_OK The API completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE On initialization failure. 
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION The channel, identified by 
 *   channelHandle, was not opened with the \c CL_EVENT_CHANNEL_SUBSCRIBER
 *   flag set, i.e, the event channel was not opened for subscriber access.
 *
 *  \par Description:
 *   This function is an extension to clEventSubscribe function. This function enables an EO 
 *   to subscribe for events on an event channel by registering RBE expression 
 *   on that event channel. 
 *  \par
 *   Events are delivered via the invocation of the clEvtEventDeliverCallback() callback
 *   function, which must have been supplied when the EO called the clEventInitialize()
 *   function.
 *  \par
 *   The EO must have opened the event channel, designated by \e channelHandle,
 *   with the \c CL_EVENT_CHANNEL_SUBSCRIBER flag set for an invocation of this function
 *   to be successful.
 *  \par
 *   The memory associated with the RBE is not deallocated by the clEventExtWithRbeSubscribe 
 *   function. It is the responsibility of the invoking EO to deallocate
 *   the memory when the clEventExtWithRbeSubscribe API returns. 
 *  \par
 *   For a given subscription, the filters parameter cannot be modified. To change the filters
 *   parameter without losing events, an EO must establish a new subscription
 *   with the new filters parameter. After the new subscription is established, the old 
 *   subscription can be removed by invoking the clEventUnsubscribe function.
 * 
 *  \par Library File:
 *  ClEventClient
 * 
 *  \sa clEventSubscribe(), clEventExtSubscribe()
 *
 */

    ClRcT clEventExtWithRbeSubscribe(CL_IN const ClEventChannelHandleT
                                     channelHandle, CL_IN ClRuleExprT *pRbeExpr,
                                     CL_IN ClEventSubscriptionIdT
                                     subscriptionID, CL_IN void *pCookie);





/**
 ************************************
 *  \brief Sets event attributes.
 * 
 *  \par Header File:
 *  clEventExtApi.h
 *
 *  \param eventHandle Handle of the event for which attributes are to be set.
 *  \param eventType Event type that wants to publish on given channel. 
 *  \param priority Priority of the event.
 *  \param retentionTime The duration till when the event will be retained.
 *  \param pPublisherName Pointer to the name of the event publisher. 
 *
 *  \retval CL_OK The API completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE On initialization failure. 
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function serves as a wrapper around \e clEventAttributesSet() taking a fixed length event type
 *   instead of a filter. It is used to set all the write-able event attributes, like - <em>fixed length 
 *   event type, priority, retentionTime,</em> and \e pPublisherName, in the header of the event,
 *   designated by the \e eventHandle. If \e pPatternArray or \e pPublisherName are NULL, 
 *   the corresponding attributes are not changed.
 * 
 *  \par Library File:
 *  ClEventClient
 * 
 *  \sa clEventAttributesSet(), clEventExtAttributesGet()
 *            
 */
    ClRcT clEventExtAttributesSet(CL_IN ClEventHandleT eventHandle,
                                  CL_IN ClUint32T eventType,
                                  CL_IN ClEventPriorityT priority,
                                  CL_IN ClTimeT retentionTime,
                                  CL_IN const SaNameT *pPublisherName);



/**
 ************************************
 *  \brief Returns event attributes designated by \e eventHandle. 
 * 
 *  \par Header File:
 *  clEventExtApi.h
 *
 *  \param eventHandle Handle of the event for which attributes are to be retrieved.
 *  \param pEventType (out) Pointer to an event type.
 *  \param pPriority (out) Pointer to the priority of the event.
 *  \param pRetentionTime (out) Pointer to the duration for which the publisher will retain the
 *   event.
 *  \param pPublisherName (out) Pointer to the name of the publisher of the event.
 *  \param pPublishTime (out) Pointer to the time at which the publisher published the event.
 *  \param pEventId (out) Pointer to the event identifier.
 *  
 *  \retval CL_OK The API completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE On initialization failure. 
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function servers as a wrapper around \e clEventAttributesGet() taking a fixed length event type
 *   instead of a filter. It retrieves the value of the attributes of the event designated by
 *   \e eventHandle. 
 *  \par
 *   For each of the out or in/out parameters, if the invoking EO provides a NULL reference,
 *   the EM does not return the out value. 
 *  \par
 *   This function can be called on any event allocated by the \e clEventAllocate function
 *   or received via the \e clEvtEventDeliverCallback() function. It can also be 
 *   modified by the \e clEventExtAttributesSet() API. 
 *  \arg
 *   If this function is invoked on a received event, the attributes "publish time" and 
 *   "event id" have the values set by the EM at event publishing time. 
 *  \arg
 *   Else, the attributes will either have the initial values set by 
 *   the EM when allocating the event or the attributes set by a prior invocation 
 *   of the \e clEventExtAttributesSet() function.
 * 
 *  \par Library File:
 *  ClEventClient
 * 
 *  \sa clEventAttributesSet(), clEventExtAttributesGet()
 *  
 *            
 */
    ClRcT clEventExtAttributesGet(CL_IN ClEventHandleT eventHandle,
                                  CL_OUT ClUint32T *pEventType,
                                  CL_OUT ClEventPriorityT * pPriority,
                                  CL_OUT ClTimeT *pRetentionTime,
                                  CL_OUT SaNameT *pPublisherName,
                                  CL_OUT ClTimeT *pPublishTime,
                                  CL_OUT ClEventIdT * pEventId);


# ifdef __cplusplus
}
# endif


#endif                          /* _CL_EVENT_EXT_API_H_ */

/**
 *  \}
 */



