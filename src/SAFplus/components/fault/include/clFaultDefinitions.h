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
 * ModuleName  : fault
 * File        : clFaultDefinitions.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file provides the definitions for the fault record structure.
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Definitions for the Fault record structure
 *  \ingroup fault_apis
 */

/**
 *  \addtogroup fault_apis
 *  \{
 */

#ifndef _CL_FAULT_DEFINITIONS_H_
#define _CL_FAULT_DEFINITIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clAlarmDefinitions.h>


/**
 *  The \e ClFaultEventT data structure is used by components like Alarm 
 *  Manager, Component Manager, and Chassis Manager to report faults.
 */

typedef struct ClFaultEvent {

    /**
     * The version number of the fault.
     */
    ClUint32T version;

    /**
     * Name of the component on which the fault occurred.
     */
    ClNameT compName;

    /**
     * The moID of the resource against which the fault is being raised.
     */
    ClCorMOIdT  moId;

   /**
    * Flag to indicate if the alarm was for assert or for clear.
    */
    ClAlarmStateT alarmState;
		
    /**
     * The category of the fault event.
     */
    ClAlarmCategoryTypeT category;

    /**
     * The severity of the fault event.
     */
    ClAlarmSeverityTypeT severity;

    /**
     * The timestamp of the fault event.
     */
    ClUint32T timeStamp;      

    /**
     * The probable cause of the fault event.
     */
    ClAlarmProbableCauseT cause;      

    /**
     * The specific problem of the fault event.
     */
    ClAlarmSpecificProblemT specificProblem;

    /**
     * This informs that the recovery action is taken by AMS.
     */
    ClUint32T recoveryActionTaken;    

    /**
     * The size of the additional information 
     * being passed in fault event.
     */
    ClUint32T addInfoLen;

    /**
     * The additional information being passed 
     * in the fault event.
     */
    ClInt8T additionalInfo[1];
} ClFaultEventT;

/** 
 * Type defining pointer to fault event.
 */ 

typedef ClFaultEventT*    ClFaultEventPtr;

/**
 *  Fault Manager uses the \e ClFaultRecordT data structure
 *  to store information regarding the faults that it has processed.
 *  The faults are stored as \e ClFaultRecordT as a part
 *  of fault probation history.
 */

typedef struct ClFaultRecord {

    /**
     * The sequence number of the fault record 
     * which is a count of the number of times
     * an identical fault has occurred within the 
     * probation period.
     */
    ClUint8T    seqNum;

    /**
     * The fault event structure.
     */
    ClFaultEventT   event;
} ClFaultRecordT;

/** 
 * Type-defining pointer to the fault record.
 */ 

typedef ClFaultRecordT*    ClFaultRecordPtr;

/** 
 * Type defining fault sequence table as a function pointer,
 * which accepts pointer to fault record as an argument.
 */ 

typedef ClRcT (*ClFaultSeqTblT) (ClFaultRecordPtr hRec);


#ifdef __cplusplus
}
#endif

#endif                           /* _CL_FAULT_DEFINITIONS_H_ */

/** \} */

