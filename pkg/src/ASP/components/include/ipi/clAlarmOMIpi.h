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
 * ModuleName  : alarm
 * File        : clAlarmOM.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains OM definitions for alarm object
 *
 *
 *****************************************************************************/

/**
 *  \file 
 *  \brief Header file of OM definitions for Alarm Object
 *  \ingroup alarm_apis
 */

/**
 *  \addtogroup alarm_apis
 *  \{
 */

#ifndef _CL_ALARM_O_M_H_
#define _CL_ALARM_O_M_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <clCommon.h>
#include <clAlarmDefinitions.h>
#include <clOmObjectManage.h>
#include <clOmCommonClassTypes.h>
#include <clOmBaseClass.h>
#include <ipi/clAlarmIpi.h>
#include <sys/time.h>


/** 
 *  \e ClAlarmTransInfoT data structure is used to
 *  store transient alarm information. 
 */

typedef struct 
{
    /**
     * Elapsed asserted soaking time in milliseconds. 
     */
    ClUint32T        elapsedAssertSoakingTime; 

    /**
     * Elapsed clear soaking time in milliseconds.
     */
    ClUint32T      elapsedClearSoakingTime;    
     
    /**
     *  Size of the payload.
     */
    ClUint32T      payloadLen; 
   
    /**
     *  Payload.
     */
    void *         payload; 
   
     /**
     * Time at which the soaking started.
     */
    struct timeval SoakingStartTime;    
   
    /**
     * Alarm event handler.
     */
    ClUint32T      alarmEveH;    
} ClAlarmTransInfoT;


/** 
 *  Default alarm OM class.
 */

CL_OM_BEGIN_CLASS(CL_OM_BASE_CLASS, CL_OM_ALARM_CLASS)

/**
 *  This is the time interval elapsed, since the polling started
 *  for this resource which corresponds to the \e MOID.
 */ 
    ClTimeT          elapsedPollingInterval;
/**
 *  This is the bitmap of the current alarms for the
 *  resource. This is an indicator of the existing alarms
 *  or active alarms.
 */
    ClUint64T        clcurrentAlmsBitMap;
/**
 *  The bitmap of the alarms for the resource. This
 *  is an indicator of the alarms that are
 *  being soaked.
 */
    ClUint64T        clalmsUnderSoakingBitMap;
/**
 * This is an array of alarms of type ClAlarmTransInfoT.
 * The maximum number is 64.                
 */
    ClAlarmTransInfoT alarmTransInfo[CL_ALARM_MAX_ALARMS];
/**
 *  This is the mutex used for locking and unlocking before 
 *  and after calling the user-defined polling function.
 */
    ClOsalMutexIdT    cllock;
/**
 *  The function pointer for the user-defined 
 *  polling function.
 */
    ClRcT    (*fpAlarmObjectPoll)(CL_OM_ALARM_CLASS*   objPtr, 
                                ClCorMOIdPtrT        hMoId,
                                ClAlarmPollInfoT alarmsToPoll[]);

CL_OM_END
                                                                                

#ifdef __cplusplus
}
#endif

#endif /* _CL_ALARM_O_M_H_ */

/** \} */



