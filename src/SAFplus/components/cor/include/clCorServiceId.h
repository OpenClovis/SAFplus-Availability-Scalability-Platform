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
 * ModuleName  : cor                                                           
 * File        : clCorServiceId.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains the service ids(fixed) for all the MSPs.
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of COR Service Ids
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */


#ifndef _CL_COR_SERVICE_ID_H_
#define _CL_COR_SERVICE_ID_H_

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * The members of this enumeration type contains the service ID for all MSPs.
 * \arg CL_COR_SVC_ID_FAULT_MANAGEMENT - represents the OpenClovis Fault Manager.
 * \arg CL_COR_SVC_ID_ALARM_MANAGEMENT - represents the OpenClovis Alarm Agent.
 * \arg CL_COR_SVC_ID_PROVISIONING_MANAGEMENT - represents the OpenClovis Provisioning Manager.
 * \arg CL_COR_SVC_ID_DUMMY_MANAGEMENT - represents the OpenClovis Dummy MSP.
 */
typedef enum {

    CL_COR_INVALID_SRVC_ID = -1,

/**
 * OpenClovis Dummy MSP.
 */
	CL_COR_SVC_ID_DUMMY_MANAGEMENT = 0,
 
/**
 * OpenClovis Fault Manager.
 */
	CL_COR_SVC_ID_FAULT_MANAGEMENT,

/**
 * OpenClovis Alarm Agent.
 */
	CL_COR_SVC_ID_ALARM_MANAGEMENT,

/**
 * OpenClovis Provisioning Manager.
 */
	CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,

/*
 * Openclovis PM manager. 
 */
   CL_COR_SVC_ID_PM_MANAGEMENT,
      
/*
 * OpenClovis AMF manager
 */
    CL_COR_SVC_ID_AMF_MANAGEMENT,

/*
 * Openclovis Chassis manager
 */
   CL_COR_SVC_ID_CHM_MANAGEMENT,

/**
 * End of OpenClovis Service IDs.
 */
	CL_COR_SVC_ID_MAX,

	CL_COR_SVC_ID_FORCED = CL_FORCED_TO_16BITS

} ClCorServiceIdT;

#ifdef __cplusplus

}

#endif
#endif  /* _CL_COR_SERVICE_ID_H_ */


/** \} */
