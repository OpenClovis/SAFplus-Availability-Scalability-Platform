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
 * File        : clFaultHistory.h
 *******************************************************************************/

/*****************************************************************************
 * Description :                                                                
 * This file has the definitions for the fault history record               
 *****************************************************************************/
#ifndef _CL_FAULT_HISTORY_H_
#define _CL_FAULT_HISTORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clClistApi.h>
#include <clFaultDefinitions.h>

typedef struct
{
ClCntHandleT	bucketHandle;
ClCharT 			timeString[80];
} ClFH2MinBucketT;

typedef ClFH2MinBucketT* ClFH2MinBucketPtr;


typedef enum
{
CL_ALL_BUCKETS,
CL_LATEST_RECORD_IN_ANY_BUCKET,
CL_LATEST_RECORD_IN_CURRENT_BUCKET,
CL_BUCKET_TYPE_INVALID,
} ClFaultBucket_e;

/* forward declarations and externs */
ClRcT 
clFaultHistoryDataLock();

ClRcT 
clFaultHistoryDataUnLock();

void 
clFaultHis2MinBucketDelete(ClClistDataT userData);

ClRcT 
clFaultHistoryMoIdRecordsDelete(ClCorMOIdPtrT hMOId);

ClRcT 
clFaultHistoryRecordDelete(ClFH2MinBucketT* bucketH, ClHandleT hOm);

ClRcT 
clFaultMOIDRecordsInBucketSearch(ClFaultRecordPtr pFE, ClHandleT hOM,
    						ClCntHandleT hBucket, ClUint8T category, 
							ClUint8T severity, 
                            ClAlarmSpecificProblemT specificProblem,
                            ClAlarmProbableCauseT cause,
                            ClUint8T* recordFound);

void
clFaultMOIdRecordCheck(ClFaultRecordPtr pFE, ClUint8T catBitMap , 
                        ClUint8T sevBitMap,ClAlarmSpecificProblemT specificProblem,
                        ClAlarmProbableCauseT cause,ClUint8T* recordMatches);

void 
clFaultRecordRecursivePack ( ClFaultRecordPtr	rec );

extern ClRcT 
clFaultProcessFaultRecordQuery(ClCorMOIdPtrT hMOId, ClUint8T category, 
                                 ClUint8T severity,ClAlarmSpecificProblemT specificProblem, 
								 ClAlarmProbableCauseT cause,
 							     ClFaultBucket_e bucketInfo, 
								 ClFaultRecordPtr pFE, ClUint8T* recordFound);
#ifdef __cplusplus
}
#endif

#endif	/*	 _CL_FAULT_HISTORY_H_ */
