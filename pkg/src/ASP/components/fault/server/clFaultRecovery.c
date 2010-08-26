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
 * ModuleName  : fault                                                         
 * File        : clFaultRecovery.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *                                                                        
 * This module schedule the Fault Recovery action(s)                      
 ********************************************************************************/

/* system includes */
#include <stdio.h>
#include <string.h>

/* ASP includes */
#include <clQueueApi.h>
#include <clCommon.h>
#include <clCntApi.h>
#include <clEoApi.h>
#include <clOsalApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>

/* fault includes */
#include <clFaultDefinitions.h>
#include <clFaultErrorId.h>
#include <clFaultRecovery.h>
#include <clFaultClientServerCommons.h>
#include <clFaultHistory.h>
#include <clFaultLog.h>

extern ClFaultSeqTblT ***faultactiveSeqTbls;

ClRcT clFaultRepairProcess(ClFaultRecordPtr hRec)
{
	ClRcT				rc = CL_OK;
	ClUint8T			catIndex = 0, sevIndex = 0;
	ClFaultRecordPtr 	historyRec = NULL;
	ClFaultRecordPtr 	fRec = NULL;
	ClUint8T    		recordFound = 0;	

	
	if ( (rc = clFaultCategory2IndexTranslate((hRec->event).category, &catIndex)) != CL_OK )
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("FM REPAIR, Invalid CATEGORY. CANNOT DO \
                           ANY REPAIR\r\n"));
    	return rc;
	}
	if( (rc=clFaultSeverity2IndexTranslate((hRec->event).severity, &sevIndex)) != CL_OK )
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("FM REPAIR, Invalid SEVERITY. CANNOT DO \
                       ANY REPAIR\r\n"));
    	return rc;
	}

	if (!faultactiveSeqTbls)		
	{
     	     	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("FM REPAIR, No sequence table found for \
                               this MO  \r\n"));
    	return CL_FAULT_RC(CL_FAULT_ERR_REPAIR_SEQ_TBL_NULL);
	}

	historyRec = (ClFaultRecordPtr)clHeapAllocate(sizeof(ClFaultRecordT));	
    if(!historyRec)
    {
        clLogError("SER", NULL,
                "Error in allocating memory for historyRec");
        return CL_ERR_NO_MEMORY;
    }
	fRec = (ClFaultRecordPtr)clHeapAllocate(sizeof(ClFaultRecordT));
    if(!fRec)
    {
        clLogError("SER", NULL,
                "Error in allocating memory for fault record fRec");
        clHeapFree(historyRec);
        return CL_ERR_NO_MEMORY;
    }


	rc = clFaultHistoryDataLock();
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(" REPAIR: Not able to get history data lock rc:0x%x \n", rc));
        clHeapFree(fRec);
        clHeapFree(historyRec);
        return rc;
    }
	rc = clFaultProcessFaultRecordQuery(&(hRec->event.moId),
                                           (hRec->event).category,
                                           (hRec->event).severity,
                                           (hRec->event).specificProblem,
                                           (hRec->event).cause,
										   CL_LATEST_RECORD_IN_CURRENT_BUCKET,/* Should not be CL_ALL_BUCKETS */
                                           fRec, 
                                           &recordFound);
    if(CL_OK != rc)
    {
        clLogDebug("SER", NULL,
                "Querying record with category[%s], severity[%s], cause[%s] returned failure",
                _clFaultCatStr(catIndex+1),
                _clFaultSevStr(sevIndex+1),
                _clFaultProbCauseStr((hRec->event).cause));
    }
	if(!recordFound)
    {
        /* record not found */
        hRec->seqNum = 0;
		clLogInfo("SER", NULL, "Record not found");
	}
	else
	{
		/* record found */
		hRec->seqNum = fRec->seqNum;
		clLogInfo("SER", NULL, "Found an existing record, seqno [%d]", 
					hRec->seqNum);		
	}
	hRec->event.category = clFaultInternal2CategoryTranslate((hRec->event).category);
	hRec->event.severity = clFaultInternal2SeverityTranslate((hRec->event).severity);

	if (faultactiveSeqTbls[catIndex][sevIndex][hRec->seqNum]){
		(faultactiveSeqTbls[catIndex][sevIndex][hRec->seqNum])(hRec);
	}
	else
	{
		clLogError("SER", NULL, "No repair handler provided");
	}
	
	clFaultRecordShow(hRec);
	hRec->event.category = clFaultCategory2InternalTranslate((hRec->event).category);
	hRec->event.severity = clFaultSeverity2InternalTranslate((hRec->event).severity);
	hRec->seqNum = (hRec->seqNum+1)  % 5;

	memcpy(historyRec, hRec, sizeof(ClFaultRecordT));
	/*
	 * Put this fault record into the fault history
	 */
	rc =clFaultEventHistoryAdd(historyRec); 
	if(rc!=CL_OK)
	{
        clLogError("SER", NULL,
                "Error in adding event history node, rc [0x%x]", 
                rc);
	}
	clFaultHistoryDataUnLock();	
    clHeapFree(fRec);
	return rc;
}

ClRcT 
clFaultRecordShow(ClFaultRecordPtr hRec)
{
	clLogDebug("SER", NULL, "Category [%s], Specific Problem [%d], Severity [%s], Cause [%s], Seq No [%d]",
                _clFaultCatStr((hRec->event).category),
				(hRec->event).specificProblem,
                _clFaultSevStr((hRec->event).severity),
				_clFaultProbCauseStr((hRec->event).cause),
                hRec->seqNum);
	return CL_OK;
}

ClCharT*    _clFaultCatStr(ClUint32T    category)
{
    if(category > 5) 
    {
        clLogError("SER", NULL,
                "Invalid category [%d]", category);
        return "INVALID";
    }
    return clFaultCategoryString[category];
}
ClCharT*    _clFaultSevStr(ClUint32T    severity)
{
    if(severity > 6 )
    {
        clLogError("SER", NULL,
                "Invalid severity [%d]", severity);
        return "INVALID";
    }
    return clFaultSeverityString[severity];
}
ClCharT*    _clFaultProbCauseStr(ClUint32T    probCause)
{
    if(probCause > 57)
    {
        clLogError("SER", NULL,
                "Invalid probable cause [%d]", probCause);
        return "INVALID";
    }
    return clFaultProbableCauseString[probCause];
}
