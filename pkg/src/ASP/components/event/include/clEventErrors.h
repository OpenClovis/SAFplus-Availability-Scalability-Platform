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
 * File        : clEventErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header contains the various EM error codes.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Event Manager Error Codes
 *  \ingroup event_apis 
 */

/**
 ************************************
 *  \addtogroup event_apis
 *  \{
 */

#ifndef _CL_EVENT_ERRORS_H_
#define _CL_EVENT_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * EM ERROR codes 
 */

/**
 * Appends the Event Component ID to the Error Code.
 */
#define CL_EVENTS_RC(ERR_ID)                        (CL_RC(CL_CID_EVENTS, ERR_ID))

/**
 * Event Library has not been initialized.
 */
#define CL_EVENT_ERR_INIT_NOT_DONE                  (0x100)   /* 0x100 */

/**
 * An unexpected error has occured in the Event Library and it may not be able to process the request.
 */
#define CL_EVENT_ERR_INTERNAL                       (0x101)   /* 0x101 */

/**
 * The parameter passed to the API is invalid.
 */
#define CL_EVENT_ERR_INVALID_PARAM                  (0x102)   /* 0x102 */

/**
 * The flags specified are invalid - out of range or not compatible with each other.
 */
#define CL_EVENT_ERR_BAD_FLAGS                      (0x103)   /* 0x103 */

/**
 * The channel handle passed is invalid - the channel has not been created. It may be closed/deleted/unlinked.
 */
#define CL_EVENT_ERR_INVALID_CHANNEL_HANDLE         (0x104)   /* 0x104 */

/**
 * The event handle specified is invalid - has not been allocated or has been already deleted. 
 */
#define CL_EVENT_ERR_BAD_HANDLE                     (0x105)   /* 0x105 */

/**
 * The Event Library is out of memory and no memory can be allocated at this point in time.
 */
#define CL_EVENT_ERR_NO_MEM                         (0x106)   /* 0x106 */

/**
 * The parameter passed to the API is NULL.
 */
#define CL_EVENT_ERR_NULL_PTR                       (0x107)   /* 0x107 */

/**
 * The channel has been already openend by the EO.
 */
#define CL_EVENT_ERR_CHANNEL_ALREADY_OPENED         (0x108)   /* 0x108 */

/**
 * The channel specified by the channel handle has not been opened.
 */
#define CL_EVENT_ERR_CHANNEL_NOT_OPENED             (0x109)   /* 0x109 */

/**
 * The EO has already subscribed on the specified channel using the given subscription ID.
 */
#define CL_EVENT_ERR_DUPLICATE_SUBSCRIPTION         (0x10a)   /* 0x10a */

/**
 * The channel has not been opened for subscription and an attempt to subscribe was made on the same.
 */
#define CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION    (0x10b)   /* 0x10b */

/**
 * The channel has not been opened for publish and an attempt to subscribe was made on the same. 
 */
#define CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH         (0x10c)   /* 0x10c */

/**
 * This return code is used to check the normal termination of the event info walk.   
 */
#define CL_EVENT_ERR_INFO_WALK_STOP                 (0x10d)   /* 0x10d */

/**
 * The Event Library has already been initialized by this EO.
 */
#define CL_EVENT_ERR_ALREADY_INITIALIZED            (0x10e)   /* 0x10e */

/**
 * This return code is used to check the normal termination of a walk to remove subscribers.   
 */
#define CL_EVENT_ERR_STOP_WALK                      (0x10f)   /* 0x10f */

/**
 * The memory allocated by the user for the event data (payload) is insufficient.
 */
#define CL_EVENT_ERR_NO_SPACE                       (0x110)   /* 0x110 */

/**
 * The version specified is not supported or is incorrect.
 */
#define CL_EVENT_ERR_VERSION                        (0x111)   /* 0x111 */

/**
 * The specified resource such as Subscription ID, Channel, and so on already exists.
 */
#define CL_EVENT_ERR_EXIST                          (0x112)   /* 0x112 */

/**
 * The Event Library is out of some resource other than memory like file descriptors.
 */
#define CL_EVENT_ERR_NO_RESOURCE                    (0x113)   /* 0x113 */

/**
 * The ChannelOpen Callback is NULL 
 */
#define CL_EVENT_ERR_INIT                           (0x114)   /* 0x114 */ 

#ifdef __cplusplus
}
#endif

#endif                          /* _CL_EVENT_ERRORS_H_ */

/**
 *  \}
 */


