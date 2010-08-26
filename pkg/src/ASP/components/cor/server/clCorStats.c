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
 * ModuleName  : cor
 * File        : clCorStats.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the COR statistics
 *****************************************************************************/

/* INCLUDES */
#include <string.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>

/* Internal Headers*/
#include "clCorStats.h"
#include "clLogApi.h"
#include "clCorLog.h"


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

CORStat_h corStatTab = NULL;

/**
 *  Initialize the Clovis Object Registry statistics module.
 *
 *  This API initializes the Clovis Object Registry statistics module.
 *                                                                        
 *  @param none
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorStatisticsInitialize(void)
{

	if ((corStatTab = clHeapAllocate(sizeof(CORStat_t))) == NULL)
	{
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to allocate memory"));
		return(CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
	}
	memset(corStatTab, 0, sizeof(CORStat_t));

	return (CL_OK);
}

void clCorStatisticsFinalize(void)
{

	if(NULL != corStatTab )
		clHeapFree(corStatTab );
	
}


/**
 *  Display the COR statistics table.
 *
 *  This API displays the COR statistics gathered by the COR.
 *                                                                        
 *  @param none
 *
 *  @returns CL_OK  - Success<br>
 */
#if 0
void
clCorStatisticsShow(char** params)
{

	if (corStatTab == NULL)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "COR Statistics module not initlaized"));
		return;
	}

	clOsalPrintf("\nDisplaying COR Statistics information");
	clOsalPrintf("\nMaximum Transactions allowed............. %08d", 
		corStatTab->maxTrans);
	clOsalPrintf("\nMaximum Slaves/Targets managed........... %08d",  
		corStatTab->maxChannel);
	clOsalPrintf("\nCOR Mode of operation.................... %d", 
		corStatTab->corMode);
	clOsalPrintf("\nTotal Input events received.............. %08d", 
		corStatTab->inputEventCount);
	clOsalPrintf("\nCOR EO uptime............................ %08d", 
		corStatTab->upTime);
	clOsalPrintf("\nTotal number of NULL messages............ %08d", 
		corStatTab->nullMsgRxd);
	clOsalPrintf("\nTotal number of msgs with NULL payload... %08d",  
		corStatTab->nullPayload);
	clOsalPrintf("\nTotal number of msgs with invalid channel %08d", 
		corStatTab->invalidChan);
	clOsalPrintf("\nTotal number of events processed......... %08d", 
		corStatTab->validFSMEvents);
	clOsalPrintf("\nTotal number of invalid events received.. %08d", 
		corStatTab->invalidFSMEvent);
	clOsalPrintf("\nTotal number of HELLO ACKs received...... %08d", 
		corStatTab->helloAckRx);
	clOsalPrintf("\nTotal number of HELLO ACKs sent.......... %08d", 
		corStatTab->helloAckTx);
	clOsalPrintf("\nnTotal number of HELLO messages sent..... %08d", 
		corStatTab->helloTx);
}
#endif

