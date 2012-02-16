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
 * File        : clAlarmOmClass.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains alarm OM related definitions
 *****************************************************************************/

#ifndef _CL_ALARM_OM_CLASS_H_
#define _CL_ALARM_OM_CLASS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorApi.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clDebugApi.h>

#include <clAlarmClient.h>

/** 
 * Assigning values for the OM
 * related operations of alarm.
 */

typedef enum
{
    CL_ALARM_OM_OPERATION_GET,
    CL_ALARM_OM_OPERATION_SET,
    CL_ALARM_OM_OPERATION_INVALID,
}ClAlmOMOperationT;

/** 
 *  Assigning values for alarm OM attributes.
 */

typedef enum
{
    CL_ALARM_OM_ATTR_START=CL_ALARM_PROFILE_ATTR_END ,

    CL_ALARM_ELAPSED_POLLING_INTERVAL,

    /**
     * Current alarm status.
     */
    CL_ALARM_CURRENT_ALMS_BITMAP,         
    CL_ALARM_ALMS_UNDER_SOAKING,
    CL_ALARM_ELAPSED_ASSERT_SOAK_TIME,
    CL_ALARM_ELAPSED_CLEAR_SOAK_TIME,
    CL_ALARM_EVENT_PAY_LOAD_LEN,
    CL_ALARM_EVENT_PAY_LOAD,
    CL_ALARM_SOAKING_START_TIME,
    CL_ALARM_EVE_HANDLE,

    /**
     * OM attribute end.
     */
    CL_ALARM_OM_ATTR_END,
                                                                                
    CL_ALARM_INVALID_ATTRID
}ClAlarmOMAttrIdsT;

ClRcT clAlarmOmObjectOperation(ClCorMOIdPtrT pMoId,
                                ClUint32T         attrId,
                                ClUint32T         idx,
                                void*             val,
                                ClAlmOMOperationT oprId);

#ifdef __cplusplus
}
#endif 
                                                                                
#endif /* _CL_ALARM_OM_CLASS_H_ */


