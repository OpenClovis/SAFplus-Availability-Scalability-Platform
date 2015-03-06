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
 * File        : clEventApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains the various API available with EM.
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

#ifndef _CL_EVENT_API_H_
# define _CL_EVENT_API_H_

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


/******************************************************************************
                                EM Definitions
******************************************************************************/

/**
 * Opens a local event channel.
 */
# define CL_EVENT_LOCAL_CHANNEL      (0x0)

/**
 * Opens a global event channel.
 */
# define CL_EVENT_GLOBAL_CHANNEL     (0x1)

/**
 * Opens an event channel for subscribing events.
 */
# define CL_EVENT_CHANNEL_SUBSCRIBER (0x2)

/**
 * Opens an event channel for publishing events.
 */
# define CL_EVENT_CHANNEL_PUBLISHER  (0x4)

/**
 * Creates an event channel if it does not exist.
 */
# define CL_EVENT_CHANNEL_CREATE     (0x8)

/**
 * Highest priority.
 */
# define CL_EVENT_HIGHEST_PRIORITY   (0x0)

/**
 * Lowest priority.
 */
# define CL_EVENT_LOWEST_PRIORITY    (0x3)

/**
 * Latest Supported SAF Version for Event Service.
 */
# define CL_EVENT_VERSION {(ClUint8T)'B', 0x1, 0x1}
# define CL_EVENT_VERSION_SET(version) (version).releaseCode = (ClUint8T)'B', \
                                      (version).majorVersion = 0x1,\
                                      (version).minorVersion = 0x1

/******************************************************************************
                                EM Data types
******************************************************************************/

/**
 * The type of an event identifier. Values ranging from 0 to 1000 have special meanings
 * and cannot be used by the event service to identify regular events.
 */
    typedef ClUint64T ClEventIdT;

/**
 * Event priority type - it ranges from \c CL_EVENT_LOWEST_PRIORITY to
 * \c CL_EVENT_HIGHEST_PRIORITY.
 */
    typedef ClUint8T ClEventPriorityT;

/**
 * Event Channel open flag. It can have one of the following values:
 * \arg \c CL_EVENT_LOCAL_CHANNEL
 * \arg \c CL_EVENT_GLOBAL_CHANNEL
 * \arg \c CL_EVENT_CHANNEL_PUBLISHER
 * \arg \c CL_EVENT_CHANNEL_SUBSCRIBER
 * \arg \c CL_EVENT_CHANNEL_CREATE
 *  \param evtChannelOpenFlags (in) The requested access modes and scope of the event channel. The
 *  value of this flag is obtained by a bitwise or can be set as one of the following:
 *  \arg \c CL_EVENT_CHANNEL_PUBLISHER: to enable the EO to use the event channel handle returned by
 *  the \e clEventPublish function.
 *  \arg \c CL_EVENT_CHANNEL_SUBSCRIBER: to enable the EO to use the event channel handle returned by
 *  the \e clEventSubscribe function.
 *  \arg \c CL_EVENT_CHANNEL_CREATE: to open and create an event channel that does not exist.
 *  \arg \c CL_EVENT_GLOBAL_CHANNEL and \c CL_EVENT_LOCAL_CHANNEL flags specify the scope of the event channel.
 *
 */
    typedef ClUint8T ClEventChannelOpenFlagsT;



    /*
     * Various Event handles
     */

/**
 * The type of the handle supplied by the EM to an EO during the initialization
 * of the EM library. The EO uses this to whenever it invokes an EM function so that the
 * EM can recognize the EO.
 */
    typedef ClHandleT ClEventInitHandleT;

/**
 * The type of a handle to an event.
 */
    typedef ClHandleT ClEventHandleT;

/**
 * The type of a handle to an open event channel.
 *
 */
    typedef ClHandleT ClEventChannelHandleT;

/**
 * The type of an identifier for a particular subscription by a particular EO on a
 * particular event channel. This identifier is used to associate the delivery of events
 * for that subscription to the EO.
 */
    typedef ClUint32T ClEventSubscriptionIdT;



/******************************************************************************
                                EM Callbacks
******************************************************************************/

/**
 *   \brief Event delivery callback.
 *
 *   \param subscriptionId An identifier that an EO supplied to an invocation of 
 *    clEventSubscribe() to determine which subscription resulted in the delivery 
 *    of the event.
 *
 *   \param eventHandle The handle to the event delivered by this callback.
 *
 *   \param ClSizeT The size of the data associated with the event.
 *
 *   \par Description:
 *   The EM invokes this callback function to notify the subscriber EO that
 *   an event has been received. This callback is invoked in the subscriber EO context.
 *   A published event is received when it has an event pattern matching the filter of a
 *   subscription of this EO, established by the clEventSubscribe() call.
 *   \par
 *   After successful completion of this call, the EO may invoke the
 *   clEventAttributesGet() function to obtain the attributes and the
 *   clEventDataGet() function to obtain the data associated with the event.
 *   \par
 *   It is the responsibility of the EO to free the event by invoking the
 *   clEventFree() function.
 *   \par
 *   The validity of the \e eventHandle parameter is not limited to the scope of this callback
 *   function.
 */
    typedef void (*ClEventDeliverCallbackT)(
                                            ClEventSubscriptionIdT subscriptionId,
                                            ClEventHandleT eventHandle,
                                            ClSizeT eventDataSize);

/**
 *   \brief Open channel callback.
 *
 *   \param invocation This parameter was supplied by an EO in the corresponding
 *   invocation of the \e clEventChannelOpenAsync() function and is used by the
 *   EM in this callback. It allows the EO to match the invocation of the 
 *   function with the invocation of this callback.
 *
 *   \param channelHandle The handle that designates the event channel.
 *
 *   \param error This parameter indicates whether the clEventChannelOpenAsync
 *                function executed successfully.
 *
 *   \par Description:
 *   The EM invokes this callback function when the operation requested by the
 *   clEventChannelOpenAsync() function completes. This callback is invoked in the
 *   context of EO that initialized the EM.
 *   \par
 *   On successful execution, the handle to the opened or created event channel is returned
 *   in \e channelHandle, else, an error is returned in the error parameter.
 *
 */
    typedef void (*ClEventChannelOpenCallbackT) (
                                                 ClInvocationT invocation,
                                                 ClEventChannelHandleT channelHandle,
                                                 ClRcT error);


/**
 *   The callback structure supplied by an EO to the EM containing the
 *   callback functions that can be invoked by the EM.
 */
    typedef struct
    {

/**
 * Asynchronous Channel open callback.
 */
        ClEventChannelOpenCallbackT clEvtChannelOpenCallback;

/**
 * Event delivery callback.
 */
        ClEventDeliverCallbackT clEvtEventDeliverCallback;

    } ClEventCallbacksT;

    typedef struct
    {
        ClUint8T version;
        ClEventCallbacksT callbacks;
    } ClEventVersionCallbacksT;

/******************************************************************************
                                EM Data structures
******************************************************************************/

/**
 * An Event pattern may contain a name (for example: a process name, checkpoint name,
 * service instance name, and so on). Alternatively, an event pattern may characterize
 * an event (for example, timedOut, newComponent, overload, and so on).
 *
 */

    typedef struct
    {

/**
 * Size of the buffer allocated to receive the pattern value.
 */
        ClSizeT allocatedSize;

/**
 * Actual size of the buffer allocated to receive the pattern value.
 */
        ClSizeT patternSize;

/**
 * Pointer to a buffer where the pattern value will be copied.
 */
        ClUint8T *pPattern;

    } ClEventPatternT;

/**
 * The type of an event pattern array.
 */
    typedef struct
    {

/**
 * Number of entries allocated in the pattern buffer.
 */
        ClSizeT allocatedNumber;

/**
 * Actual number of patterns in the event.
 */
        ClSizeT patternsNumber;

/**
 * Pointer to a buffer where the array of pattern will be copied.
 */
        ClEventPatternT *pPatterns;

    } ClEventPatternArrayT;


/**
 * Filter Related Information.
 */

    typedef enum
    {
/**
 * The entire filter must match the first \e filterSize characters of the event pattern.
 */
        CL_EVENT_PREFIX_FILTER = 1,
/**
 * The entire filter must match the last \e filterSize characters of the event pattern.
 */
    CL_EVENT_SUFFIX_FILTER = 2,
/**
 * The entire filter must exactly match the entire event pattern.
 */
    CL_EVENT_EXACT_FILTER = 3,
/**
 * Always matches, regardless of the filter or event pattern.
 */
    CL_EVENT_PASS_ALL_FILTER = 4,

    } ClEventFilterTypeT;

/**
 * The event filter structure defines the filter type and the filter
 *  pattern to be applied on an event pattern when filtering events
 *  on an event channel.
 */
    typedef struct
    {

/**
 * Filter type. This can have values as described in ClEventFilterTypeT.
 */
        ClEventFilterTypeT filterType;

/**
 * Filter pattern information.
 */
        ClEventPatternT filter;

    } ClEventFilterT;

/**
 * The event filter array structure defines one or more filters.
 * Filters are passed to the Event Service by a subscriber process via the
 * clEvtEventSubscribe() call. The Event Service does the filtering to decide whether a
 * published event is delivered to a subscriber for a given subscription by matching the
 * first filter (contents and type) against the first pattern in the event pattern array, the
 * second filter against the second pattern in the event pattern array, and so on up to
 * the last filter. An event matches a given subscription if all patterns of the event match
 * all filters provided in an invocation of the clEvtEventSubscribe() call.
 */
    typedef struct
    {

/**
 * Number of filters.
 */
        ClSizeT filtersNumber;

/**
 * Pointer to filter pattern.
 */
        ClEventFilterT *pFilters;

    } ClEventFilterArrayT;

/******************************************************************************
                                EM External functions
******************************************************************************/
 
/**
 ************************************
 *  \brief Initializes EM library.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \par Warning:
 *  This function can be called by an EO <em>only once</em>.
 *
 *  \param pEvtHandle (out) Pointer to the handle designating this particular initialization of
 *  the EM to be returned.
 *
 *  \param pEvtCallbacks If \e evtCallbacks is set to NULL, no callback is registered;
 *  else, it is a pointer to \e ClEventCallbacksT structure, containing the callback
 *  functions of the EO that the EM may invoke. Only non-NULL callback functions
 *  in this structure will be registered.
 *
 *  \param pVersion (in/out) This can be either an input or an output parameter.
 *  \arg As an input parameter, version is a pointer to the required EM version.
 *  Here, \e minorVersion is ignored and should be set to 0x00.
 *
 *  \arg As an output parameter, the version actually supported by the EM is delivered.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *  This function is used to initialize the EM for the invoking EO and registers
 *  the various callback functions. It must be invoked prior to the invocation
 *  of any other EM functionality. The handle \e pEvtHandle is returned as the reference
 *  to this association between the EO and the EM. The EO uses this handle in subsequent
 *  communication with the EM. 
 *  \par
 *  If the implementation supports the required \e releaseCode, and a \e majorVersion greater
 *  than or equal to the required \e majorVersion, \c CL_AIS_OK is returned.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventSelectionObjectGet(), clEventDispatch(), clEventFinalize()
 *
 */

    ClRcT clEventInitialize(CL_OUT ClEventInitHandleT *pEvtHandle,
                            CL_IN const ClEventCallbacksT * pEvtCallbacks,
                            CL_INOUT ClVersionT *pVersion);

    ClRcT clEventInitializeWithVersion(CL_OUT ClEventInitHandleT *pEvtHandle,
                                       CL_IN  const ClEventVersionCallbacksT *pEvtCallbackTable,
                                       CL_IN  ClUint32T numCallbacks,
                                       CL_INOUT ClVersionT *pVersion);

/**
 ************************************
 *  \brief Helps detect pending callbacks.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param evtHandle The handle, obtained through the clEventInitialize()
 *   function, designating this particular initialization of the Event Service.
 *
 *  \param pSelectionObject (out) A pointer to the operating system handle that
 *  the invoking EO can use to detect pending callbacks.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *  This function returns the operating system handle, \e pSelectionObject,
 *  associated with the handle \e evtHandle. The invoking EO can use
 *  this handle to detect pending callbacks, instead of repeatedly
 *  invoking clEventDispatch() for this purpose.
 *  The \e pSelectionObject returned by this function, is a file descriptor
 *  that can be used with poll() or select() systems call detect incoming
 *  callbacks.
 *  The \e selectionObject returned by this function is valid until
 *  clEventFinalize() is invoked on the same handle \e evtHandle.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventInitialize(), clEventDispatch(), clEventFinalize()
 *
 */
    ClRcT clEventSelectionObjectGet(ClEventInitHandleT evtHandle,
                                    ClSelectionObjectT * pSelectionObject);



/**
 ************************************
 *  \brief Invokes the pending callback in context of the EO.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param evtHandle The handle, obtained through the clEventInitialize()
 *   function, designating this particular initialization of the Event Service.
 *
 *  \param dispatchFlags Flags that specify the callback execution behavior
 *  clEventDispatch() function, which have the values \c CL_DISPATCH_ONE,
 *  \c CL_DISPATCH_ALL or \c CL_DISPATCH_BLOCKING, as defined in clCommon.h.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *  This function invokes, in the context of the calling EO, pending callbacks for
 *  handle \e evtHandle in a way that is specified by the \e dispatchFlags
 *  parameter.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventInitialize(), clEventSelectionObjectGet()
 *
 */
    ClRcT clEventDispatch(ClEventInitHandleT evtHandle, ClDispatchFlagsT dispatchFlags);


/**
 ************************************
 *  \brief Finalizes EM library.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param evtHandle Handle obtained through \e clEventInitialize() function designating
 *  this particular initialization of the EM.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *
 *  \par Description:
 *  This function is used to close the association, represented by \e evtHandle,
 *  between the invoking EO and EM. The EO must call the clEventInitialize()
 *  function before it calling this function. 
 *  \par
 *  If the clEventFinalize() function executed successfully, all resources are released which were
 *  acquired by the \e clEventInitialize() function. Moreover, it also removes all the open event channels
 *  for the particular handle and cancels all pending callbacks related to the particular handle.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \note
 *  As the callback invocation is asynchronous, some callback calls can still be processed even after
 *  calling this function.
 *
 *  \sa clEventInitialize()
 *
 */
    ClRcT clEventFinalize(CL_IN ClEventInitHandleT evtHandle);

/**
 ************************************
 *  \brief Opens an event channel.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \note
 *  An event channel cannot be opened multiple times by the same EO. It can be opened only for
 *  subscribing and/or publishing an event or a group of events.
 *
 *  \param evtHandle The handle obtained by the \e clEventInitialize() function designating
 *  this particular initialization of the EM.
 *
 *  \param pEvtChannelName Pointer to the name of the event channel to identify the channel.
 *
 *  \param evtChannelOpenFlags The requested access modes and scope of the event channel. The
 *  value of this flag is obtained by a bitwise or can be set as one of the following:
 *  \arg \c CL_EVENT_CHANNEL_PUBLISHER: To enable the EO to use the event channel handle returned by
 *  the \e clEventPublish() function.
 *  \arg \c CL_EVENT_CHANNEL_SUBSCRIBER: To enable the EO to use the event channel handle returned by
 *  the \e clEventSubscribe() function.
 *  \arg \c CL_EVENT_CHANNEL_CREATE: To open and create an event channel that does not exist.
 *  \arg \c CL_EVENT_GLOBAL_CHANNEL and \c CL_EVENT_LOCAL_CHANNEL flags specify the scope of the event channel.
 *
 *  \param timeout Time-out for calling this function. \n
 *  <B> Note:</B>
 *  An event channel can be created even after the time-out has expired.
 *
 *  \param pChannelHandle (out) Pointer to the handle of the event channel, provided by the
 *  invoking EO in the address space of the EO. If the event channel is opened
 *  successfully, the EM stores the handle in \e pChannelHandle. This handle is used by the EO
 *  to access the channel in subsequent invocations of the Event Service functions.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_BAD_FLAGS On passing an invalid flag.
 *  \retval CL_EVENT_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_CHANNEL_ALREADY_OPENED If a channel is already opened by an EO with the same name and scope.
 *
 *  \par Description:
 *  This function is used to open an event channel. If the event channel does not exist and the
 *  \c CL_EVENT_CHANNEL_CREATE flag is set in \e channelOpenFlags, the event channel is created.
 *  This function is a blocking operation and returns a new event channel handle.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventInitialize(), clEventChannelClose()
 *
 */
    ClRcT clEventChannelOpen(CL_IN ClEventInitHandleT evtHandle,
                             CL_IN const SaNameT *pEvtChannelName,
                             CL_IN ClEventChannelOpenFlagsT evtChannelOpenFlag,
                             CL_IN ClTimeT timeout,
                             CL_OUT ClEventChannelHandleT *pChannelHandle);


/**
 ************************************
 *  \brief Opens an event channel asynchronously.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param evtHandle Handle obtained by the clEventInitialize() function designating
 *   this particular initialization of the EM.
 *
 *  \param invocation It allows the invoking component to match the invocation
 *   of \e clEventChannelOpenAsync() function with the corresponding callback.
 *
 *  \param pEvtChannelName Pointer to the name of the event channel that identifies the event
 *   channel.
 *
 *  \param evtChannelOpenFlags The requested access modes and scope of the event channel. The
 *  value of this flag is obtained bitwise or can be set as one of the following:
 *  \arg \c CL_EVENT_CHANNEL_PUBLISHER: To enable the EO to use the event channel handle returned by
 *  the \e clEventPublish() function.
 *  \arg \c CL_EVENT_CHANNEL_SUBSCRIBER: To enable the EO to use the event channel handle returned by
 *  the \e clEventSubscribe() function.
 *  \arg \c CL_EVENT_CHANNEL_CREATE: To open and create an event channel that does not exist.
 *  \arg \c CL_EVENT_GLOBAL_CHANNEL and \c CL_EVENT_LOCAL_CHANNEL flags are the scope of the event channel.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_BAD_FLAGS On passing an invalid flag.
 *  \retval CL_EVENT_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_CHANNEL_ALREADY_OPENED If a channel is already opened by an EO with the same name and scope.
 *
 *  \par Description:
 *  This function is used to open an event channel asynchronously. If the event channel does
 *  not exist and the \c CL_EVENT_CHANNEL_CREATE flag is set in the \e channelOpenFlags parameter,
 *  the event channel is created. 
 *  \par
 *  The successful execution of this function is signaled by an invocation of the associated
 *  \e clEvtChannelOpenCallback() callback function supplied while the EO calls the \e clEventInitialize() function. 
 *  \par
 *  The EO supplies the value of invocation while calling the \e clEventChannelOpenAsync() function. This value
 *  is supplied back to the application by the EM while calling the \e clEvtChannelOpenCallback() function.
 *  \note
 *  The invocation parameter is a mechanism that enables the EO to determine which callback is
 *  triggered by which call.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEvtChannelOpenCallbackT(), clEventInitialize(), clEventChannelClose()
 *
 */
    ClRcT clEventChannelOpenAsync(CL_IN ClEventInitHandleT evtHandle,
                                  CL_IN ClInvocationT invocation,
                                  CL_IN const SaNameT *pEvtChannelName,
                                  CL_IN ClEventChannelOpenFlagsT
                                  channelOpenFlags);

/**
 ************************************
 *  \brief Closes an event channel.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param channelHandle Handle of the event channel to close. It must have
 *   been obtained by an earlier invocation of either \e clEventChannelOpen() function
 *   or \e clEvtChannelOpenCallback() function.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *
 *  \par Description:
 *   This function is used to close the event channel, designated by \e channelHandle, which was
 *   opened by an earlier invocation of \e clEventChannelOpen() or \e clEventChannelOpenAsync()
 *   functions. After the invocation of this function, the \e channelHandle is no longer valid.
 *  \par
 *   On successful execution of this function, if no EO has the event channel open,
 *   the event channel is deleted immediately if its deletion was pending as
 *   a result of a clEventChannelUnlink() function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventChannelOpen(), clEventChannelOpenAsync(), 
 *      clEvtChannelOpenCallbackT, clEventChannelUnlink()
 *
 */
    ClRcT clEventChannelClose(CL_IN ClEventChannelHandleT channelHandle);


/**
 ************************************
 *  \brief Deletes an event channel.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param evtHandle Handle obtained by the invocation of \e clEventInitialize() designating this particular initialization
 *   of the EM.
 *  \param pEvtChannelName Pointer to the name of the event channel to be unlinked.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *
 *  \par Description:
 *   This function is used to delete an existing event channel, identified by
 *   \e pEvtChannelName, from the cluster. 
 *  \par
 *   After the successful execution of this function:
 *     - The name \e pEvtChannelName is no longer valid, that is, any invocation of a function of
 *       the EM function that uses the event channel name returns an error, unless an event channel is re-created
 *       with this name.
 *       The event channel can be re-created by specifying the same name of the event channel to be
 *       unlinked in an open call with the \c CL_EVENT_CHANNEL_CREATE flag set. This way, a new instance
 *       of the event channel is created while the old instance of the event channel has been still not deleted.
 *     - If none of the EOs has an event channel open any longer, the event channel is deleted immediately.
 *     - Any EO that has the event channel open can still continue to access it. Deletion of the event
 *       channel will occur when the last \e clEventChannelClose() is executed.
 *     .
 *   The deletion of an event channel frees all resources allocated by the EM, such as - published events with
 *   non-zero retention time.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *   \note
 *   An event channel is only deleted from the cluster namespace when clEventChannelUnlink() is invoked on it.
 *
 *  \sa clEventInitialize(), clEventChannelClose()
 *
 */
     ClRcT clEventChannelUnlink(CL_IN ClEventInitHandleT evtHandle,
                                CL_IN const SaNameT *pEvtChannelName);


/**
 ************************************
 *  \brief Allocates an event header.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param channelHandle Handle of the event channel on which the event is to be published.
 *   It must be obtained earlier either by \e clEventChannelOpen() function or \e clEvtChannelOpenCallback() function.
 *
 *  \param pEventHandle (out) Pointer to the handle for the newly allocated event. The invoking EO is
 *   responsible to allocate memory for the \e eventHandle before calling this function.
 *   The EM will assign the value of the \e eventHandle when this function is invoked.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function is used to allocate memory for the event header. The event allocated by this function must be
 *   freed by the \e clEventFree() function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventFree(), clEventPublish()
 *
 */

    ClRcT clEventAllocate(CL_IN ClEventChannelHandleT channelHandle,
                          CL_OUT ClEventHandleT *pEventHandle);

    ClRcT clEventAllocateWithVersion(CL_IN ClEventChannelHandleT channelHandle,
                                     CL_IN ClUint8T version,
                                     CL_OUT ClEventHandleT *pEventHandle);


/**
 ************************************
 *  \brief Frees an event header.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param eventHandle Handle of the event for which memory is to be freed by the EM.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *
 *  \par Description:
 *   This function is used to give permission to the EM to deallocate the memory that contains the attributes
 *   and the event data, if present, of the event that is associated with eventHandle. It frees the events
 *   allocated by \e clEventAllocate() function or \e clEvtEventDeliverCallback() function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventAllocate(), clEventChannelOpen(), clEventChannelOpenAsync()
 *
 */
    ClRcT clEventFree(CL_IN ClEventHandleT eventHandle);


/**
 ************************************
 *  \brief Sets the event attributes.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param eventHandle Handle of the event for which attributes are to be set.
 *
 *  \param pPatternArray Pointer to a structure that contains the array of pattern to be
 *   copied into the event pattern array and the number of such pattern.
 *
 *  \param priority Priority of the event.
 *
 *  \param retentionTime Duration for which the event will be retained. NOTE: Event Retention
 *  is not currently supported and this arguments has no effect.
 *
 *  \param pPublisherName Pointer to the name of the event publisher.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH \c CL_EVENT_CHANNEL_PUBLISHER flag was not
 *   set for the event channel on which the event to be published was allocated, i.e,
 *   the event channel was not opened for publisher access.
 *
 *  \par Description:
 *   This function is used to set all the write-able event attributes, such as - <em>pPatternArray,
 *   priority, retentionTime,</em> and \e pPublisherName, in the header of the event, designated by
 *   \e eventHandle. If \e pPatternArray or \e pPublisherName is NULL, the corresponding
 *   attributes are not changed.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa
 *  clEventAllocate(), clEventFree(), ::ClEventDeliverCallbackT,
 *  clEventAttributesGet(), clEventChannelOpen(), clEventChannelOpenAsync()
 *
 */
    ClRcT clEventAttributesSet(CL_IN ClEventHandleT eventHandle,
                               CL_IN const ClEventPatternArrayT *pPatternArray,
                               CL_IN ClEventPriorityT priority,
                               CL_IN ClTimeT retentionTime,
                               CL_IN const SaNameT *pPublisherName);


/**
 ************************************
 *  \brief Returns the event attributes.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param eventHandle Handle of the event for which attributes are to be retrieved.
 *
 *  \param pPatternArray (in/out) Pointer to a structure that describes the event pattern array
 *   and the number of patterns to be retrieved.
 *   - If the caller sets pPatternArray->patterns to NULL, the EM ignores the
 *   pPatternArray->allocatedNumber field, and it will allocate memory for the pattern array
 *   and the individual patterns and will set the fields pPatternArray->patternsNumber,
 *   pPatternArray->patterns, pPatternArray->patterns[i].pattern, and
 *   pPatternArray->patterns[i].patternSize accordingly. 
 *   - The invoking EO is then responsible for de-allocating the corresponding memory, that is,
 *   pPatternArray->patterns and each pPatternArray->patterns[i].pattern.
 *   - Alternatively, the invoking EO can allocate the memory to retrieve all event patterns
 *   and set the fields pPatternArray->allocatedNumber, pPatternArray->patterns,
 *   pPatternArray->patterns[i].allocatedSize, and pPatternArray->patterns[i].pattern accordingly.
 *   - Here, these fields are in parameters and will not be modified by the EM. The EM copies the
 *   patterns into the successive entries of pPatternArray->patterns, starting with the first entry and
 *   continuing until all event patterns are copied. 
 *   - If pPatternArray->allocatedNumber is smaller than the number of event patterns or if the size
 *   of the buffer allocated for one of the patterns is then the actual size of the pattern, the invocation
 *   fails and the \c CL_EVENT_ERR_NO_SPACE error is returned. 
 *   - If such an error happens, it is not specified whether some buffers pointed to by
 *   pPatternArray->patterns[i].pattern were changed by the EM. 
 *   - Regardless of whether such an error occurs, the EM sets the pPatternArray->patternsNumber and
 *   pPatternArray->patterns[i].patternSize fields for all pPatternArray->allocatedNumber
 *   individual patterns, to indicate the number of event patterns and the size of each pattern.
 *
 *  \param pPriority (out) Pointer to the priority of the event.
 *
 *  \param pRetentionTime (out) Pointer to the duration for which the publisher will retain the event.
 *
 *  \param pPublisherName (out) Pointer to the name of the event publisher.
 *
 *  \param pPublishTime (out) Pointer to the time when the publisher published the event.
 *  NOTE: The nodes may required to synched wrt time to use this parameter effectively.
 *
 *  \param pEventId (out) Pointer to the event identifier.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_SPACE The buffer provided by the process is too small to hold
 *   the data associated with the delivered event.
 *
 *  \par Description:
 *   This function is used to retrieve the value of the attributes of the event designated by
 *   \e eventHandle. 
 *  \par
 *   For each of the out or in/out parameters, if the invoking EO provides a NULL reference,
 *   the EM does not return the out value. 
 *  \par
 *   This function can be called on any event allocated by the \e clEventAllocate() function
 *   or received via the \e clEvtEventDeliverCallback() function. This can also be
 *   modified by the \e clEventAttributesSet() function. 
 *  \arg 
 *   If this function is invoked on a received event, the attributes "publish time" and
 *   "event id" have the values set by the EM at event publishing time. 
 *  \arg
 *   Else, the attributes will either have the initial values set by the EM while
 *   allocating the event or the attributes set by a call to \e clEventAttributesSet function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa
 *  clEventAllocate(), clEventFree(),
 *  clEventChannelOpen(), clEventChannelOpenAsync(),
 *  clEventAttributesSet()
 *
 */
    ClRcT clEventAttributesGet(CL_IN ClEventHandleT eventHandle,
                               CL_IN ClEventPatternArrayT *pPatternArray,
                               CL_OUT ClEventPriorityT * pPriority,
                               CL_OUT ClTimeT *pRetentionTime,
                               CL_OUT SaNameT *pPublisherName,
                               CL_OUT ClTimeT *pPublishTime,
                               CL_OUT ClEventIdT * pEventId);


/**
 ************************************
 *  \brief Returns data associated with an earlier event.
 *
 *  \par Header File:
 *   clEventApi.h
 *
 *  \param eventHandle Handle to the event delivered by
 *   \e clEvtEventDeliverCallback().
 *
 *  \param pEventData (in/out) Pointer to a buffer provided by the EO in which the EM
 *   stores the data associated with the delivered event.
 *   If \e pEventData is NULL, the value of \e pEventDataSize provided by the invoking EO
 *   is ignored, and the buffer is provided by the EM library. The buffer must be
 *   deallocated by the calling EO after returning from the \e clEventDataGet call.
 *   NOTE: This argument cannot be NULL as the API needs a pointer to pointer to return 
 *   allocated memory to the callee. This is a bug in the SAF sepcification B.01.01 and
 *   has been fixed in the newer version of the specification. The user is always required
 *   allocate and also free the memory.
 *
 *  \param pEventDataSize (in/out) If \e pEventData is not NULL, the in value of this parameter is
 *   same as the \e pEventData buffer provided by the invoking EO.
 *   \arg If this buffer is not large enough to hold all of the data associated with this event,
 *   then no data will be copied into the buffer, and the error code \c CL_EVENT_ERR_NO_SPACE
 *   is returned. 
 *   \arg
 *   If pEventData is NULL, the in value of pEventDataSize is ignored.
 *   The out value of \e pEventDataSize is set when the function returns either \c CL_AIS_OK or
 *   \c CL_EVENT_ERR_NO_SPACE, and its size is same as that of the data associated with this event,
 *   which may be less than, equal to, or greater than the in value of \e pEventDataSize.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_SPACE The buffer provided by the process is too small to hold
 *   the data associated with the delivered event.
 *
 *  \par Description:
 *   This function is used to retrieve the data associated with an event earlier delivered by
 *   \e clEvtEventDeliverCallback().
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa
 *  ::ClEventDeliverCallbackT, clEventFree(),
 *  clEventChannelOpen(), clEventChannelOpenAsync()
 *
 */

    ClRcT clEventDataGet(CL_IN ClEventHandleT eventHandle,
                         CL_INOUT void *pEventData,
                         CL_INOUT ClSizeT *pEventDataSize);


/**
 ************************************
 *  \brief Returns the cookie.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param eventHandle Handle to the event delivered by
 *   \e clEvtEventDeliverCallback().
 *
 *  \param ppCookie (out) Cookie information while subscribing for event.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function is used to return the cookie passed by the EO on subscription
 *   to an event. It can be invoked from  the registered callback function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventSubscribe()
 *
 */
    ClRcT clEventCookieGet(CL_IN ClEventHandleT eventHandle,
                           CL_OUT void **ppCookie);

/**
 ************************************
 *  \brief Publishes an event.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param eventHandle Handle of the event to be published. The event must
 *   be allocated by the \e clEventAllocate() function or obtained vthrough the \e clEvtEventDeliverCallback()
 *   function. The patterns must be set by \e clEventAttributesSet() function, if changes are required.
 *
 *  \param pEventData Pointer to a buffer that contains additional event information for
 *   the event being published. This parameter is set to NULL if no additional information
 *   is associated with the event. The EO may deallocate the memory for \e pEventData
 *   when \e clEventPublish() function returns.
 *
 *  \param eventDataSize The number of bytes in the buffer pointed to by \e pEventData.
 *   This parameter is ignored if \e pEventData is NULL.
 *
 *  \param pEventId (out) Pointer to the identifier of the event.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH \c CL_EVENT_CHANNEL_PUBLISHER flag was not
 *   set for the event channel on which the event to be published was allocated, i.e,
 *   the event channel was not opened for publisher access.
 *
 *  \par Description:
 *   This function is used to  publish an event on the channel for which the event specified by
 *   \e eventHandle has been allocated or obtained through the \e clEvtEventDeliverCallback() function.
 *   It returns the event identifier in \e eventId. The event to be published consists of a
 *   standard set of attributes, such as - the event header, and an optional data part.
 *   \par
 *   The event channel must be opened by the EO, on which this event is published with the
 *   \c CL_EVENT_CHANNEL_PUBLISHER flag set for successful execution of this function.
 *   \par
 *   Before an event can be published, the publisher EO can call the \e clEventAttributesSet()
 *   function to set the write-able event attributes. The published event is delivered to the
 *   subscribers whose subscription filters match the event patterns.
 *   \par
 *   When the EM publishes an event, it automatically sets the following read-only
 *   event attributes into the published event:
 *   \arg Event publish time
 *   \arg Event identifier
 *   NOTE: The Event identifier is not being set as Event Retention is not supported yet and
 *   this parameter does't come into picture. However, the Event publish time would give the
 *   time of publish of the event.
 *
 *   In addition to the event attributes, an EO also supply values for the \e pEventData and
 *   \e eventDataSize parameters for publication as part of the event. The event attributes and
 *   the event data of the event identified by \e eventHandle are not affected by this function.
 *   \par
 *   This function copies the event attributes and the event data into internal memory of the EM. The
 *   invoking EO can free the event using \e clEventFree() after \e clEventPublish() returns successfully.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa
 *   clEventAllocate(), clEventFree(), ::ClEventDeliverCallbackT,
 *   clEventAttributesSet(), clEventSubscribe()
 *
 *
 */
    ClRcT clEventPublish(CL_IN ClEventHandleT eventHandle,
                         CL_IN const void *pEventData,
                         CL_IN ClSizeT eventDataSize,
                         CL_OUT ClEventIdT * pEventId);


/**
 ************************************
 *  \brief Subscribes to an event identified by an event type (filter).
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param channelHandle Handle of the event channel on which the EO is subscribing
 *   to receive events. The parameter channelHandle must have been obtained
 *   previously by either \e clEventChannelOpen() or \e clEvtChannelOpenCallback() functions.
 *
 *  \param pFilters Pointer to a ClEventFilterArrayT structure, allocated by the EO,
 *   that defines filter patterns to use to filter events received on the event channel.
 *   The EO may deallocate the memory for the filters when \e clEventSubscribe() returns.
 *
 *  \param pSubscriptionId Identifies a specific subscription on this instance of the
 *   opened event channel corresponding to the \e channelHandle and that is used as a
 *   parameter of \e clEvtEventDeliverCallback().
 *
 *  \param pCookie Cookie passed by you, which can be given back by the
 *  \e clEvtEventUtilsCookiGet() function.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION The channel, identified by
 *   channelHandle, was not opened with the \c CL_EVENT_CHANNEL_SUBSCRIBER
 *   flag set, i.e, the event channel was not opened for subscriber access.
 *
 *  \par Description:
 *   This function enables an EO to subscribe for events on an event channel by registering one
 *   or more filters on that event channel. Events are delivered using
 *   \e clEvtEventDeliverCallback() function supplied when the EO called the \e clEventInitialize() function.
 *   \par
 *   The EO must open the event channel, designated by \e channelHandle, with the
 *   \c CL_EVENT_CHANNEL_SUBSCRIBER flag set for successful execution of this function.
 *   \par
 *   The memory associated with the filters is not deallocated by the clEventSubscribe() function. It
 *   is the responsibility of the invoking EO to deallocate the memory when the \e clEventSubscribe()
 *   function returns. 
 *   \par
 *   For a given subscription, the filters parameter cannot be modified. To change the filters
 *   parameter without losing events, an EO must establish a new subscription with the new
 *   filters parameter. Once the new subscription is established, the old subscription can be
 *   removed using the \e clEventUnsubscribe() function.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa
 *  clEventUnsubscribe(), clEventDataGet(),
 *  ::ClEventDeliverCallbackT, clEventAttributesGet()
 *
 */
    ClRcT clEventSubscribe(CL_IN ClEventChannelHandleT channelHandle,
                           CL_IN const ClEventFilterArrayT *pFilters,
                           CL_IN ClEventSubscriptionIdT subscriptionId,
                           CL_IN void *pCookie);


/**
 ************************************
 *  \brief Unsubscribes from an event.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param channelHandle The event channel for which the subscriber is requesting the
 *   EM to delete the subscription. This parameter must be obtained earlier by
 *   either \e clEventChannelOpen() function or \e clEvtChannelOpenCallback() function.
 *
 *  \param subscriptionId Identifier of the subscription.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_EVENT_ERR_NO_MEM On memory allocation failure.
 *
 *  \par Description:
 *   This function allows an EO to stop receiving events for a particular subscription on
 *   an event channel by removing the subscription. 
 *  \par
 *   The execution of this function is successful if the \e subscriptionId matches with an
 *   earlier registered subscription. Events queued to be delivered to the EO, and
 *   ones that no longer match any subscription if the \e clEventUnsubscribe() function are purged.
 *   An EO that wants to modify a subscription without losing any events must establish
 *   the new subscription before removing the existing subscription.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventSubscribe()
 *
 */
    ClRcT clEventUnsubscribe(CL_IN ClEventChannelHandleT channelHandle,
                             CL_IN ClEventSubscriptionIdT subscriptionId);


/*
 ************************************
 *  \brief Clears the retention event.
 *
 *  \par Header File:
 *  clEventApi.h
 *
 *  \param channelHandle The handle of the event channel on which the event has been
 *   published. \e channelHandle must be obtained earlier by either the
 *   \e clEventChannelOpen() function or \e clEvtChannelOpenCallback() function.
 *
 *  \param eventId Identifier of the event.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library has not been initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE On passing an invalid handle.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred within the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function is used to clear the retention time of a published event, designated
 *   by \e eventId. It indicates that EM need not  keep the event any longer
 *   for potential new subscribers. Once the value of the retention time is reset to
 *   zero, the event is no longer available for new subscribers.
 *
 *  \par Library File:
 *  ClEventClient
 *
 *  \sa clEventPublish(), ::ClEventDeliverCallbackT
 *
 */
    ClRcT clEventRetentionTimeClear(CL_IN ClEventChannelHandleT channelHandle,
                                    CL_IN const ClEventIdT eventId);

# ifdef __cplusplus
}
# endif


#endif                          /* _CL_EVENT_API_H_ */

/**
 *  \}
 */





