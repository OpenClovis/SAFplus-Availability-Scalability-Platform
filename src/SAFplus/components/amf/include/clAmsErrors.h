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

/**
 *  \file
 *  \brief Header file of error Messages that are AMS specific.
 *  \ingroup ams_apis
 */

/**
 *  \addtogroup ams_apis
 *  \{
 */

#ifndef _CL_AMS_ERRORS_H_
#define _CL_AMS_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/**
 * The component is invalid.
 */
    
#define CL_AMS_ERR_INVALID_COMP                 0x100    

/**
 * The AMS entity (node, service group, service unit, service
 * instance, component or component service instance) is invalid.
 */
#define CL_AMS_ERR_INVALID_ENTITY               0x101

/**
 * The administrative state of an entity (node, service group, service
 * unit or service instance) is invalid.
 */
#define CL_AMS_ERR_INVALID_ENTITY_STATE         0x102

/**
 * The entity list name is invalid. (Used by AMS management APIs.)
 */
#define CL_AMS_ERR_INVALID_ENTITY_LIST          0x103

/**
 * The entity state is invalid w.r.t to the requested operation.
 */
#define CL_AMS_ERR_ENTITY_NOT_ENABLED           0x104

/**
 * Invalid AMF configuration.
 */
#define CL_AMS_ERR_BAD_CONFIG                   0x105

/**
 * Invalid arguments. (Mostly used in AMF debug CLI)
 */
#define CL_AMS_ERR_INVALID_ARGS                 0x106

/**
 * The requested operation is (currently) invalid.
 */
#define CL_AMS_ERR_INVALID_OPERATION            0x107

/**
 * The requested operation could not be completed.
 */
#define CL_AMS_ERR_OPERATION_FAILED             0x108

/**
 * The service instance cannot be assigned an active HA state.
 */
#define CL_AMS_ERR_SI_NOT_ASSIGNABLE            0x109

/**
 * The component service instance cannot be assigned an active HA
 * state.
 */
#define CL_AMS_ERR_CSI_NOT_ASSIGNABLE           0x10a

/**
 * Unmarshalling of the AMS buffer failed.
 */
#define CL_AMS_ERR_UNMARSHALING_FAILED          0x10b

/**
 * Error macro definitions for AMS.
 */
#define CL_AMS_RC(ERROR_CODE)  CL_RC(CL_CID_AMS, (ERROR_CODE))

#ifdef __cplusplus
}
#endif

#endif                          /* _CL_AMS_ERRORS_H_ */

/** \} */
