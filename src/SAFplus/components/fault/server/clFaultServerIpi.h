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
 * File        : clFaultServerIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains fault server related IPIs
 *
 *
 *****************************************************************************/

#ifndef _CL_FAULT_SERVER_IPI_H_
#define _CL_FAULT_SERVER_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ASP includes */
#include <clCommon.h>
#include <clCorMetaData.h>
#include <clEventApi.h>

/* fault related includes */
#include <clFaultDefinitions.h> 
#include <clFaultErrorId.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/******************************************************************************
 *  Data Types
 *****************************************************************************/
                                                                                           
typedef enum
{
	/** 
	 * History key for fault manager local.
	 */ 
	CL_FAULT_HISTORY_KEY = 1,
}ClFaultContainerAttrIdT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/* forward declarations */

#if 0
ClRcT clFaultCorSubscribe(void);
void clFaultNotificationHandler( ClEventSubscriptionIdT subscriptionId, 
                               ClEventHandleT eventHandle, 
                               ClSizeT eventDataSize );
extern ClRcT clFaultCreateNotifyProcess(ClCorTxnIdT txnId, ClCorTxnJobIdT jobId);
#endif

ClRcT clFaultNativeClientTableInit(ClEoExecutionObjT* eoObj);

/* externs */

extern ClFaultSeqTblT **fmSeqTbls[];
extern ClUint32T clFaultLocalProbationPeriod;
extern ClRcT clFaultRepairProcess(ClFaultRecordPtr hRec);
extern ClRcT clFaultHistoryInit(ClUint32T);
extern ClRcT clFaultHistoryShow(ClCorMOIdPtrT);

#ifdef __cplusplus
}
#endif

                                                                                
#endif /* _CL_FAULT_SERVER_IPI_H_ */
