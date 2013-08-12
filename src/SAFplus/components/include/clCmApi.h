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
 * ModuleName  : cm
 * File        : clCmApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Header File for Chassis Manager 
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of  Chassis Manager Related APIs
 *  \ingroup cm_apis
 */

/**
 ************************************
 *  \addtogroup cm_apis
 *  \{
 */

#ifndef _CLCMAPI_H_ 
#define _CLCMAPI_H_

#ifdef __cplusplus
extern "C" { 
#endif 

    /* If the flag is not defined this will define no-ops for all APIs that is enough to satisfy the compiler */
#ifndef CL_USE_CHASSIS_MANAGER
    
#include <clCommon.h>
#include <clCommonErrors.h>

#define UsingOpenHpi(x)

/* the EO automatically generates a call to this to initalize the CM library, so give it a NULL pointer if CM is off */    
#define clCmLibInitialize 0
#define clCmLibFinalize 0
    
/**
 *  Latest supported version of the Chassis Manager client service
 */
#define CL_CM_VERSION {(ClUint8T)'B', 0x1, 0x1}


/* Please note that the event publishing service by CM is currently
 * disabled, hence they are not doxygen-visible comments
 */
 
/*  
 *  Chassis Manager Event Channel on which events will be published.
 */
#define CL_CM_EVENT_CHANNEL "CM_EVENT_CHANNEL"


/* The following are the events published by Chassis Manager. */ 

/*
 * Event issued after hot-swapping of an FRU is done.
 */
#define CL_CM_FRU_STATE_TRANSITION_EVENT_STR "cmFruStateTranstionEvent"

/*
 *  Payload of the FRU Event being published on the FRU state transition.
 */
typedef struct ClCmFruEventInfo {

/*
 *  Id of the FRU in the physical slot.
 */ 
	 ClUint32T 	   fruId;
				
/*
 *  Physical slot of the FRU. Before using, always perform a check for the slot id against \c INVALID_PHYSICAL_SLOT_ID.
 */										   
	 ClInt32T 	   physicalSlot;

#if 0  /* Note HPI is NOT being used so these structures are just fake placeholders */
    /*
 * Previous state of the FRU.
 */										   
	 SaHpiHsStateT previousState; 

/*
 * Present state of the FRU.
 */      
	 SaHpiHsStateT presentState;
#endif    
} ClCmFruEventInfoT;

/*
 * Contents of buffer field in alarmPayload for sensor event payload.
 */
typedef struct ClCmSensorEventInfo {
#if 0  /* Note HPI is NOT being used so these structures are just fake placeholders */    
    SaHpiRptEntryT      rptEntry;
    SaHpiRdrT           rdr;
    SaHpiSensorEventT   sensorEvent;
#else
    int fake;
#endif    
} ClCmSensorEventInfoT;


typedef struct {
#if 0  /* Note HPI is NOT being used so these structures are just fake placeholders */    
    SaHpiEntityPathT eventReporter;
    SaHpiEntityPathT *pImpactedEntities;
#else
    int fake;
#endif    
} ClCmEventCorrelatorT;

#if 0    
typedef ClRcT (* ClCmEventCallBackT) (SaHpiSessionIdT sessionid , 
                                      SaHpiEventT *pEvent , 
                                      SaHpiRptEntryT  *pRptEntry, 
                                      SaHpiRdrT *pRdr);
#else
 typedef ClRcT (* ClCmEventCallBackT) (unsigned int sessionid , 
                                      void *pEvent , 
                                      void  *pRptEntry, 
                                      void *pRdr);   
#endif    
    
typedef struct {
#if 0  /* Note HPI is NOT being used so these structures are just fake placeholders */    
    SaHpiEntityPathT impactedEntity;
#endif    
    ClCmEventCallBackT userEventHandler;
} ClCmUserPolicyT;

#if 0    
typedef enum ClCmThresholdLevel
{
    
    CL_CM_THRESHOLD_LOWER_MINOR = SAHPI_ES_LOWER_MINOR,
    CL_CM_THRESHOLD_LOWER_MAJOR = SAHPI_ES_LOWER_MAJOR,
    CL_CM_THRESHOLD_LOWER_CRIT  = SAHPI_ES_LOWER_CRIT,
    CL_CM_THRESHOLD_UPPER_MINOR = SAHPI_ES_UPPER_MINOR,
    CL_CM_THRESHOLD_UPPER_MAJOR = SAHPI_ES_UPPER_MAJOR,
    CL_CM_THRESHOLD_UPPER_CRIT =  SAHPI_ES_UPPER_CRIT,
}ClCmThresholdLevelT;
#endif
    
/*
 * Event payload is \e ClAlarmInfoT defined in <em> clAlarmDefinitions.h </em>, the contents
 * of buffer field in alarmPayload is \e ClCmSensorEventInfo defined above.
 */
#define CL_CM_ALARM_EVENT_STR "cmAlarmEvent"


/*
 * These error is returned for any HPI failures occur at the server side,
 * HPI errors are printed on the console or any log file.
 */ 	

/**
 * Generic HPI error
 */
#define CL_ERR_CM_HPI_ERROR             (CL_ERR_COMMON_MAX+1)

/**
 * No such physical slot in the chassis or not FRU in physical slot
 */
#define CL_ERR_CM_HPI_INVALID_PHY_SLOT  (CL_ERR_COMMON_MAX+2)


/**
 *  Possible FRU operations for the clCmFruOperationRequest() function
 */ 
typedef enum {
    /**
     * Request to power on the FRU
     */
    CL_CM_POWERON_REQUEST = 1,
    /**
     * Request to power off the FRU
     */
    CL_CM_POWEROFF_REQUEST,
    /**
     * Request to power cycle the FRU
     */
    CL_CM_POWER_CYCLE_REQUEST,
    /**
     * Request an FRU insertion
     */
    CL_CM_INSERT_REQUEST,
    /**
     * Request an FRU extraction
     */
    CL_CM_EXTRACT_REQUEST,
    /**
     * Request a cold reset of FRU
     */
    CL_CM_RESET_REQUEST,
    /**
     * Request a warm reset of FRU
     */
    CL_CM_WARM_RESET_REQUEST,
    /**
     * Request to assert the reset state of FRU
     */
    CL_CM_RESET_ASSERT_REQUEST,
    /**
     * Request to de-assert the reset state of FRU
     */
    CL_CM_RESET_DEASSERT_REQUEST

} ClCmFruOperationT;
    
/**
 ************************************
 *  \brief Returns the state of an FRU.
 *
 *  \param hMoId MoId of the FRU.
 *  \param pState (out) Pointer to store the state of the FRU.
 *
 *  \retval CL_OK  The API executed successfully
 *  \retval CL_ERR_INVALID_PARAMETER  The hMoId value is not a valid MO Id
 *  \retval CL_ERR_NULL_POINTER  On passing a NULL pointer
 *  \retval CL_ERR_VERSION_MISMATCH  If client and server are version incompatible
 *  \retval CL_ERR_CM_HPI_ERROR For any server side HPI errors
 *  \retval CL_ERR_TRY_AGAIN If Chassis Manager is busy and cannot service the request immediately
 *
 *  \par Description:
 *  This API is used to return the state of a Field Replaceable Unit (FRU). These states are defined in
 *  the <em> SaHpi.h </em> header file. The first argument is the MoId of the FRU for which the
 *  state to be retrieved. Second argument is a pointer to the location that stores the
 *  state of the FRU.
 *										
 *  \note 
 *  On any HPI Error, refer to \c DBG_PRINTS on the Chassis Manager console or Log file.
 *
 * extern ClRcT clCmFruStateGet (CL_IN  ClCorMOIdPtrT hMoId,CL_OUT SaHpiHsStateT *pState);
 */
#define clCmFruStateGet(hMoId,pState) CL_RC(CL_CID_CM,CL_ERR_NOT_SUPPORTED)


/**
 ************************************
 *  \brief Operates on the FRU or any platform hardware.
 *
 *  \param hMoId MoId of the FRU.
 *  \param request Operation requested on the hardware.
 *
 *  \retval CL_OK The API executed successfully
 *  \retval CL_ERR_INVALID_PARAMETER The hMoId value is not a valid MO Id or the request value is invalid
 *  \retval CL_ERR_VERSION_MISMATCH If client and server are version incompatible
 *  \retval CL_ERR_CM_HPI_ERROR For any server side HPI errors
 *  \retval CL_ERR_TRY_AGAIN If Chassis manager is busy and cannot service the request immediately
 *
 *  \par Description:
 *  This API is used to operate on an FRU or a platform hardware. It accepts two
 *  arguments- the MoId of the FRU to operate and the operation requested. 
 *  Operations are listed in the enumeration \e ClCmFruOperationT.
 *
 *  \warning
 *  Setting up of hotswap state of the FRU must confirm to the HPI hotswap
 *  state transition sequence. Only valid state transitions are allowed, any
 *  invalid state Transition requests are rejected as \e invalid requests. 
 * 
 *  \note
 *  On any HPI Error, refer to \c DBG_PRINTS on the Chassis Manager console or Log file.
 *
 * extern ClRcT clCmFruOperationRequest (CL_IN  ClCorMOIdPtrT     hMoId, CL_OUT ClCmFruOperationT request);
 */
#define clCmFruOperationRequest(hMoId,request) CL_RC(CL_CID_CM,CL_ERR_NOT_SUPPORTED)

/**
 ************************************************
 *  \brief This api can be used to verify the version supported by the CM
 * 
 *  \param version (IN/OUT) Version requested (IN) and the closest version
 *                          supported by the service (OUT)
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_VERSION_MISMATCH If exact request version is not supported.
 *
 *  \par Description:
 *  This function should be used to verify the version that is assumed by the
 *  client against the version(s) supported by the service.
 *  Typically, the client should use the CL_CM_VERSION macro for basis of the
 *  comparision.
 *  This API allows detection of mismatch between the client and the cleint
 *  library (in case shared libraries are used).
 *
 * extern ClRcT clCmVersionVerify(CL_INOUT ClVersionT *version);
 */
#define clCmVersionVerify(version) CL_RC(CL_CID_CM,CL_ERR_NOT_SUPPORTED)

/**
 ************************************
 *  \brief Operates on the Blade or any platform hardware.
 *
 *  \param chassisId ChassisId of the blade
 *  \param physSlot Physical slot of the blade (per HPI terminology)
 *  \param request Operation requested on the hardware
 *
 *  \retval CL_OK The API executed successfully
 *  \retval CL_ERR_INVALID_PARAMETER Request valus is invalid
 *  \retval CL_ERR_VERSION_MISMATCH If client and server are version incompatible
 *  \retval CL_ERR_CM_HPI_ERROR For any server side HPI errors
 *  \retval CL_ERR_TRY_AGAIN If Chassis manager is busy and cannot service the request immediately
 *
 *  \par Description:
 *  This API is used to operate on an BLADE or a platform hardware. It accepts three
 *  arguments- the ChassisId & physical slot number of the FRU to operate and the operation requested. 
 *  Operations are listed in the enumeration \e ClCmFruOperationT.
 *
 *  \warning
 *  Setting up of hotswap state of the BLADE must confirm to the HPI hotswap
 *  state transition sequence. Only valid state transitions are allowed, any
 *  invalid state transition requests are rejected as \e invalid requests. 
 * 
 *  \note
 *  On any HPI Error, refer to \c DBG_PRINTS on the Chassis Manager console or Log file.
 *
 *  \note
 *  This function can only be used for \e main cards and cannot be used on \e nested
 *  FRUs such as AMCs.
extern ClRcT clCmBladeOperationRequest (CL_IN ClUint32T         chassisId,
                           CL_IN ClUint32T         physSlot,
                           CL_IN ClCmFruOperationT request);

 */
#define clCmBladeOperationRequest(chassisId,physSlot,request)  CL_RC(CL_CID_CM,CL_ERR_NOT_SUPPORTED)

       

    
#define clCmThresholdStateGet(slot, pLevel,pStateAsserted) CL_RC(CL_CID_CM,CL_ERR_NOT_SUPPORTED)

#else

#include <clCommon.h>
/* These are automatically called by the EO so not really part of the public interface */    
extern ClRcT clCmLibInitialize(void);
extern ClRcT clCmLibFinalize(void);
#include <clChassisMgrApi.h>
#define USING_OPENHPI
#define UsingOpenHpi(x) x
#include <SaHpi.h>
#include <oHpi.h>    
#endif
    
#ifdef __cplusplus
}
#endif 
#endif /* _CL_CHASSIS_MGR_API_H_ */

/**
 *  \}
 */
