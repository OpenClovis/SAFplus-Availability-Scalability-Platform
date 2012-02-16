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
 * ModuleName  : hpiEventFilter
 * File        : clHpiEventFilter.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file mainly contains the various hpi event filter definitions.
 *
 *
 *****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <SaHpi.h>
#include <oh_utils.h>
#include <clOsalApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clEoApi.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _EVENT_FILTER_H
#define _EVENT_FILTER_H

/*Debug level 1 : Extensive debug prints */
#ifdef PM_EVENTFILTER_DEBUG
#define PM_DEBUG_PRINT(str) CL_DEBUG_PRINT(CL_DEBUG_ERROR, str)
#else
#define PM_DEBUG_PRINT(str) 
#endif

/*Debug level 2 : Specific/Limited Debug prints identifying events*/
#ifdef PM_EVENTFILTER_DEBUG_LEVEL2
#define PM_DEBUG2_PRINT(str) CL_DEBUG_PRINT(CL_DEBUG_ERROR, str)
#else
#define PM_DEBUG2_PRINT(str) 
#endif

/*Event Filter Types*/
typedef enum {
                PM_HPI_NONE,
                PM_HPI_SENSOR_EVENT_FILTER,
                PM_HPI_WATCHDOG_EVENT_FILTER,
                PM_HPI_HOTSWAP_EVENT_FILTER
}ClHpiEventFilterTypeT;


/* Call back function to registered application for events */
typedef void (*ClHpiEventFilterCallbackT)(SaHpiEventT *event,
                               SaHpiRptEntryT *rpt_entry,
                               SaHpiRdrT *rdr);

/* Event Structure for Sensor Events */
typedef struct ClHpiSensorEventFilter
{
    SaHpiSensorNumT sensorId;    /* negative value to turn off */
    SaHpiSensorTypeT sensorType; /* negative value to turn off */
    SaHpiEventCategoryT sensorEventCategory; /* negative value to turn off */
    ClCharT *idString;
}ClHpiSensorEventFilterT;

/* Event Structure for HotSwap Events */
typedef SaHpiHotSwapEventT ClHpiHotswapEventFilterT;

/* Event Structure for WatchDog Events */
typedef struct ClHpiWatchdogEventFilter
{
    SaHpiWatchdogNumT watchdogNum;
    SaHpiWatchdogTimerUseT watchdogUse;
}ClHpiWatchdogEventFilterT;

/* HPI Event filter structure */
typedef struct ClHpiEventFilter
{
    SaHpiEntityTypeT entityType; 	/* To provide Filtering at the Entity Level.
										SAHPI_ENT_UNSPECIFIED to disable this*/
    SaHpiEntityPathT entityPath; 	/* Specify Entity Type.SAHPI_ENT_UNSPECIFIED in the first 
                                    	entry to turn off checking */
    ClHpiEventFilterTypeT eventType;   /* HPI Event type in ClHpiEventFilterTypeT */
    void *eventFilter;           			/* This can be ClHpiSensorEventFilterT or 
											ClHpiHotswapEventFilterT or ClHpiWatchdogEventFilterT   */
    SaHpiSeverityT eventSeverity;			/* negative value to turn off */
    ClHpiEventFilterCallbackT callback;     /* This cannot be NULL */
}ClHpiEventFilterT;    

/* Structure holding info related Filter Table */
typedef struct ClHpiEventFilterTable
{
    ClUint32T noOfFilters; /* Holder for number of events registered */
    ClHpiEventFilterT *filter;/* Pointer to main hpi event filter */
}ClHpiEventFilterTableT;



/**
 ************************************
 *
 *  \par Synopsis:
 *  Register user for reception of specified HPI events.
 *
 *  \par Description:
 *  This API is used to register with hpiEventFilter library for the reception of HPI events  
 *  specified by the user.On Successfull user registration , user gets a valid handle.This handle 
 *  need to be passed to hpi event filter lib while unregistering.
 *
 *  \par Syntax:
 *  \code 	extern ClRcT clHpiEventFilterRegister (
 * 				CL_IN  ClHpiEventFilterTableT *filter_table, 
 				CL_OUT ClHandleT *handle);
 *  \endcode
 *
 *  \param filter_table: Pointer to filter holding event list.
 *  \param handle : (out) Pointer to handle after successfull registartion.
 *
 *  \retval CL_OK:  The API executed successfully.
 *  \retval CL_ERR_ALREADY_EXIST :  User already registered.
 *  \retval CL_ERR_INVALID_PARAMETER:  On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY :  Not enough memory available with system.
 *										
 *  \note 
 *  On any HPI Error, refer to \c DBG_PRINTS on the HPI Filter console .
 *
 */
extern ClRcT 
clHpiEventFilterRegister(ClHpiEventFilterTableT *filter_table, ClHandleT *handle);



/**
 ************************************
 *
 *  \par Synopsis:
 *  Un-Register user from reception of specified HPI events earlier.
 *
 *  \par Description:
 *  This API is used to un-register with hpiEventFilter library for the reception of HPI events  
 *  .On Successfully removing the user registration , success(CL_OK) will be returned. 
 *
 *  \par Syntax:
 *  \code 	extern ClRcT clHpiEventFilterUnregister(
 				CL_IN ClHandleT handle);
 *  \endcode
 *
 *  \param handle : (in) Handle recived by user after successfull registartion.
 *
 *  \retval CL_OK:  The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER:  On passing an invalid parameter.ex: User not registered
 *										
 *  \note 
 *  On any HPI Error, refer to \c DBG_PRINTS on the HPI Filter console .
 *
 */
extern ClRcT 
clHpiEventFilterUnregister(ClHandleT handle);

#endif

#ifdef __cplusplus
}
#endif
