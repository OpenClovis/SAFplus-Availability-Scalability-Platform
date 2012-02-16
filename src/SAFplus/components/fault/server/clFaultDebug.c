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
 * File        : clFaultDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file processes the commands from fault cli.
 *
 *****************************************************************************/

/* system includes */
#include <string.h>

/* ASP includes */
#include <clCpmApi.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clRmdApi.h>
#include <clCommon.h>
#include <clCorApi.h>
#include <clDebugApi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clCorNotifyApi.h>
#include <clEventApi.h>

/* fault includes */
#include <clFaultClientServerCommons.h>
#include <clFaultDefinitions.h>
#include <clFaultDebug.h>
#include <clFaultHistory.h>
#include <clFaultServerIpi.h>
#include <clFaultLog.h>

/*
 * This file contains the defintion of 
 * three functions which are going to
 * be called from the debug prompt
 *
 * 1)clFaultDebugCompleteHistoryShow
 *
 * 		This function prints the entire fault 
 * 	 history on the screen of the debug prompt
 *	 corresponding to the instance of the node
 * 	 on which it is currently running
 *
 * 2)clFaultDebugHistoryShow
 *		This function queries for a fault record
 * 	 by providing attributes of the fault record.
 * 	 On a match i.e if a fault exists the fault
 * 	 structure consisting of the fualt attributes
 * 	 is printed on the debug prompt screen
 *	 
 * 3)clFaultDebugGenerateFault
 *		This function is a simulation of a fault
 * 	 generation from the debug prompt on the node
 *	 on which the debug prompt is currently running.
 * 	 
 */

/*
 * gHistoryString is a global pointer of type ClCharT.
 * This is used for getting the fault history packaged
 * into this and pass it back to the debug server 
 * wherein the same will be printed when the control 
 * moves back to the debug server.
 */
extern ClCharT *gHistoryString;

/* 
 * this api is used to print the error messages 
 * within the context where the fault cli comman
 * is being used
 */
void clFaultCliStrPrint(ClCharT** ppRet, ClCharT* pErrMsg, ...)
{
    ClUint32T   len = strlen(pErrMsg) + 100; 
    va_list     arg;

    va_start(arg, pErrMsg);
    *ppRet = (ClCharT *)clHeapAllocate(len);
    if( NULL != *ppRet )
    {
        memset(*ppRet, '\0', len);
        vsnprintf(*ppRet, len, pErrMsg, arg);
        va_end(arg);
        return;
    }
    else
    {
        va_end(arg);
        return;
    }
    

    /*
    ClUint32T len = strlen(str);

    *retStr = clHeapAllocate(len+1);
    if(NULL == retStr)
    {
        clLogWrite(CL_LOG_HANDLE_SYS, CL_LOG_CRITICAL,	CL_FAULT_SERVER_LIB,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return;
    }
    snprintf(*retStr, len+1, str);
    return;
    */
}

/*
 * This Api is used for packaging the fault structure
 * on query from the debug prompt
 */
ClRcT 
clFaultRecordPack(ClFaultRecordPtr hRec,ClCharT** retStr){

	ClUint32T categoryStringIndex=0,severityStringIndex=0,probableCauseStringIndex=0;
	
    if(hRec->seqNum==0)
    {
        clFaultCliStrPrint(retStr, "no record found");
    }
    else
    {
        ClCharT *str1 = NULL;
        ClCharT *tmp = NULL;


        str1 = clHeapAllocate(2000);/* The allocation size is a close indication
                                       of the number of bytes used to store the
                                       fault record structure */	   
        if(!str1)
            return CL_FAULT_RC(CL_ERR_NO_MEMORY);

        tmp = clHeapAllocate(2000);/* The allocatino size is a close measure of
                                      the number of bytes used to store each 
                                      attribute of a fault record structure 
                                      attached with a meaningful message
                                      preceeding it */
        if(!tmp)
            return CL_FAULT_RC(CL_ERR_NO_MEMORY);

        sprintf (tmp,"existing record found\n");
        strcat(str1,tmp);

        categoryStringIndex = clFaultInternal2CategoryTranslate((hRec->event).category);
        sprintf (tmp," Category........... %s\n",clFaultCategoryString[categoryStringIndex]);
        strcat(str1,tmp);

        sprintf (tmp," SpecificProblem.... %d\n",(hRec->event).specificProblem);
        strcat(str1,tmp);

        severityStringIndex = clFaultInternal2SeverityTranslate((hRec->event).severity);
        sprintf (tmp," Severity........... %s\n",clFaultSeverityString[severityStringIndex]);
        strcat(str1,tmp);

        if((hRec->event).cause >= CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL &&
               (hRec->event).cause <= CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN)
            probableCauseStringIndex = (hRec->event).cause;
        sprintf (tmp," cause.............. %s\n",clFaultProbableCauseString[probableCauseStringIndex]);
        strcat(str1,tmp);

        sprintf (tmp," SequenceNumber..... %d\n",(hRec->seqNum)-1);
        strcat(str1,tmp);
        clFaultCliStrPrint(retStr, str1);
        clHeapFree(str1);
        clHeapFree(tmp);
    }
	return CL_OK;
}

/* 
 * this api is used to for querying the entire fault
 * history from the cli.
 */
ClRcT
clFaultCliDebugCompleteHistoryShow( ClUint32T argc, 
                                    ClCharT **argv, 
                                    ClCharT** ret)
{
    ClRcT               rc = CL_OK;
    ClNameT             moIdName = {0};
    ClCorMOIdT          moid;
    ClCorObjectHandleT  hMSOObj;

    if ( argc != 2 )
    {
        clFaultCliStrPrint(ret, "\nUsage : queryCompleteFaultHistory <Moid>\n"
                "\tMoid [STRING]     : This is the absolute path of the MOID"
                "i.e \\Class_Chassis:0\\Class_GigeBlade:1 \n");
        return CL_OK;
    }
    //rc = clFaultXlateMOPath (argv[1], &moid );
    strcpy(moIdName.value, argv[1]);
    moIdName.length = strlen(argv[1]);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moid);
    if ( CL_OK == rc)
    {
        rc = clCorObjectHandleGet(&moid, &hMSOObj);
        if (CL_OK != rc)
        {
            clFaultCliStrPrint(ret, "clCorObjectHandleGet failed w/rc :%x \n", rc);
            return rc;
        }
        rc = clFaultHistoryShow(&moid);	
        if(CL_OK != rc)
        {
            clFaultCliStrPrint(ret, 
                    "Error in fault history show, rc [0x%x]", rc);
            return rc;

        }
        clFaultCliStrPrint(ret, gHistoryString);
        clHeapFree(gHistoryString);
    }
    else
        clFaultCliStrPrint(ret, " MOId name to MOId conversion failed ..... \n");
        
    return CL_OK;
}

/* 
 * this api is used to for querying the fault history
 * for a particular set of attributes from the cli.
 */
ClRcT
clFaultCliDebugHistoryShow( ClUint32T argc, 
                            ClCharT **argv, 
                            ClCharT** ret)
{
    ClRcT               rc = CL_OK;
    ClNameT             moIdName = {0};
    ClUint8T            catIndex, sevIndex;
    ClUint8T    		recordFound = 0;	
    ClCorMOIdT          moid;
    ClFaultRecordT*     fRecord;
    ClFaultRecordPtr 	historyRec;
    ClCorObjectHandleT  hMSOObj;

    if ( argc != 6 )
    {
        clFaultCliStrPrint(ret, "\nUsage : queryFaulthistory <Moid>"
                " <Category#> <SpecificProblem#> <Severity#> <Cause#>\n"
                "\tMoid [STRING]    : This is the absolute path of the MOID"
                "i.e \\Class_Chassis:0\\Class_GigeBlade:1 \n"
                "\tCategory [DEC]   : Category of the fault\n"
                "\tValid values are : 1 for COMMUNICATIONS, 2 for QUALITY OF SERVICE,\n\t\t\t   3 for PROCESSING ERROR, 4 for EQUIPMENT,\n\t\t\t   5 for ENVIRONMENTAL\n\n"
                "\tSpecProblem[DEC] : Specific problem of the fault\n"
                "\tSeverity [DEC]   : Severity of the fault\n"
                "\tValid values are : 1 for CRITICAL, 2 for MAJOR,\n\t\t\t   3 for MINOR, 4 for WARNING,\n\t\t\t   5 for INTERMEDIATE, 6 for CLEAR\n\n"
                "\tCause [DEC]      : Cause of the fault\n"
                "\tValid values are : 1 to 57. Refer ClAlarmProbableCauseT for more description\n");
        return CL_OK;
    }
    //rc = clFaultXlateMOPath (argv[1], &moid );
    strcpy(moIdName.value, argv[1]);
    moIdName.length = strlen(argv[1]);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moid);
    if ( CL_OK == rc)
    {
        fRecord =(ClFaultRecordT*)clHeapAllocate(sizeof(ClFaultRecordT));

        historyRec = (ClFaultRecordPtr)clHeapAllocate(sizeof(ClFaultRecordT));	

        if(fRecord == NULL || historyRec == NULL)
        {
            clLogWrite(CL_LOG_HANDLE_SYS, CL_LOG_CRITICAL, CL_FAULT_SERVER_LIB,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            return CL_FAULT_RC(CL_ERR_NO_MEMORY);
        }
                            
        rc = clCorObjectHandleGet(&moid, &hMSOObj);
        if (CL_OK != rc)
        {
            clFaultCliStrPrint(ret, " Invalid MoId passed..... \n");
            clHeapFree(fRecord);
            clHeapFree(historyRec);
            return rc;
        }
        (fRecord->event).category = atoi(argv[2]);
        (fRecord->event).specificProblem = atoi(argv[3]);
        (fRecord->event).severity = atoi(argv[4]);
        (fRecord->event).cause = atoi(argv[5]);
        if((fRecord->event).cause < CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL 
                        ||
           (fRecord->event).cause > CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN)
        {
            clFaultCliStrPrint(ret, 
                    "Invalid probable cause [%s] supplied. Please see usage for the valid range of values\n");
            return CL_FAULT_ERR_INVLD_VAL;
        }
        (fRecord->event).moId = moid;	
                            
        (fRecord->event).category=
            clFaultCategory2InternalTranslate((fRecord->event).category);
        (fRecord->event).severity=
            clFaultSeverity2InternalTranslate((fRecord->event.severity));

        if ( (rc = clFaultCategory2IndexTranslate(
                        (fRecord->event).category, &catIndex)) != CL_OK )
        {
            clFaultCliStrPrint(ret, 
                    "\nFM REPAIR, Invalid CATEGORY. CANNOT PROCESS QUERY\n");
            clHeapFree(fRecord);
            clHeapFree(historyRec);
            return CL_OK;
        }
        if( (rc = clFaultSeverity2IndexTranslate(
                        (fRecord->event).severity, &sevIndex)) != CL_OK )
        {
            clFaultCliStrPrint(ret, 
                    "\nFM REPAIR, Invalid SEVERITY. CANNOT PROCESS QUERY\n");
            clHeapFree(fRecord);
            clHeapFree(historyRec);
            return rc;
        }

        rc = clFaultHistoryDataLock();
        if (rc != CL_OK)
        {
            clFaultCliStrPrint(ret, 
                    " REPAIR: Not able to get history data lock rc [0x%x]\n", rc);
            clHeapFree(fRecord);
            clHeapFree(historyRec);
            return rc;
        }
        rc = clFaultProcessFaultRecordQuery(&(fRecord->event.moId),
                                               (fRecord->event).category,
                                               (fRecord->event).severity,
                                               (fRecord->event).specificProblem,
                                               (fRecord->event).cause,
                                               CL_ALL_BUCKETS,historyRec,
                                               &recordFound);
        if(!recordFound)
        {
            /* record not found */
            fRecord->seqNum = 0;
        }
        else
        {
            /* record found */
            fRecord->seqNum = historyRec->seqNum;
        }
        rc = clFaultHistoryDataUnLock();
        if (rc != CL_OK)
        {
            clFaultCliStrPrint(ret, 
                    "REPAIR: Not able to get history data lock rc:0x%x \n", rc);
            clHeapFree(fRecord);
            clHeapFree(historyRec);
            return rc;
        }
    
        clFaultRecordPack(fRecord,ret);
        clHeapFree(fRecord);
        clHeapFree(historyRec);
    }
    else
        clFaultCliStrPrint(ret, 
                " MOId name to MOId conversion failed ..... \n");
    return CL_OK;
}

/* 
 * this api is used for generating the fault
 * from the cli.
 */
ClRcT
clFaultCliDebugGenerateFault(ClUint32T argc, 
                             ClCharT **argv, 
                             ClCharT** ret)
{
	ClRcT               rc = CL_OK;
    ClNameT             moIdName = {0};
	ClCorMOIdT          moid;
	ClFaultRecordT*     fRecord;
	ClCorObjectHandleT  hMSOObj;

	if ( argc != 7 )
    {
		clFaultCliStrPrint(ret, 
                "\nUsage : generateFault <Moid#>"
                " <Category#> <SpecificProblem#> <Severity#> <Cause#> <alarmState>\n"
                "\tMoid [STRING]    : This is the absolute path of the MOID"
                "Ex:- \\Class_Chassis:0\\Class_GigeBlade:1 \n"
                "\tCategory [DEC]   : Category of the fault\n"
                "\tValid values are : 1 for COMMUNICATIONS, 2 for QUALITY OF SERVICE,\n\t\t\t   3 for PROCESSING ERROR, 4 for EQUIPMENT,\n\t\t\t   5 for ENVIRONMENTAL\n\n"
                "\tSpecProb [DEC]   : Specific problem of the fault\n"
                "\tSeverity [DEC]   : Severity of the fault\n"
                "\tValid values are : 1 for CRITICAL, 2 for MAJOR,\n\t\t\t   3 for MINOR, 4 for WARNING,\n\t\t\t   5 for INTERMEDIATE, 6 for CLEAR\n\n"
                "\tCause [DEC]      : Cause of the fault\n"
                "\tValid values are : 1 to 57. Refer ClAlarmProbableCauseT for more description\n"
                "\tAlarm State [DEC]: State of the fault\n"
                "\tValid values are : 0 for CLEAR, 1 for ASSERT\n");
		return CL_OK;
    }
//	rc = clFaultXlateMOPath (argv[1], &moid );
    strcpy(moIdName.value, argv[1]);
    moIdName.length = strlen(argv[1]);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moid);
	if ( CL_OK == rc)
    {
		fRecord =(ClFaultRecordT*)clHeapAllocate(sizeof(ClFaultRecordT));
		if(fRecord == NULL)
		{
            clFaultCliStrPrint(ret,
                    "Heap allocation error. Error in generating fault\n");
			return CL_FAULT_RC(CL_ERR_NO_MEMORY);
		}
		
        clCorMoIdShow(&moid);
		rc = clCorObjectHandleGet(&moid, &hMSOObj);
		if (CL_OK != rc)
		{
			clFaultCliStrPrint(ret,
                    "clCorObjectHandleGet for MOId [%s] failed with error [0x%x]. Error in generating fault\n", 
                    moIdName.value, rc);
			clHeapFree(fRecord);
            return rc;
		}
		(fRecord->event).category = atoi(argv[2]);
        (fRecord->event).specificProblem = atoi(argv[3]);
        (fRecord->event).severity = atoi(argv[4]);
        (fRecord->event).cause = atoi(argv[5]);
        if((fRecord->event).cause < CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL 
                        ||
           (fRecord->event).cause > CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN)
        {
            clFaultCliStrPrint(ret, 
                    "Invalid probable cause [%s] supplied. Please see usage for the valid range of values\n");
            return CL_FAULT_ERR_INVLD_VAL;
        }
        (fRecord->event).alarmState = atoi(argv[6]);
		(fRecord->event).moId = moid;	

		rc = clFaultValidateCategory((fRecord->event).category);
        if (CL_OK != rc)
        {
			clHeapFree(fRecord);
	        clFaultCliStrPrint(ret, 
                    "Invalid Category [%s] of Alarm. Please see usage for the valid range of values\n",
                    argv[2]);
            return rc;
        }

		rc = clFaultValidateSeverity((fRecord->event).severity);
        if (CL_OK != rc)
        {
			clHeapFree(fRecord);
	        clFaultCliStrPrint(ret,
                    "Invalid Severity [%s] of Alarm. Please use usage for the valid range of values\n",
                    argv[4]);
            return rc;
        }

		if((fRecord->event).alarmState == 1 ||
			(fRecord->event).alarmState == 0)
		{
								
			(fRecord->event).category=
				clFaultCategory2InternalTranslate((fRecord->event).category);
			(fRecord->event).severity=
				clFaultSeverity2InternalTranslate((fRecord->event.severity));
			rc = clFaultRepairProcess(fRecord);
            if(CL_OK != rc)
            {
                clFaultCliStrPrint(ret, 
                        "Error in generating fault error[0x%x]\n");
                clHeapFree(fRecord);
                return rc;
            }
			clHeapFree(fRecord);	
            clFaultCliStrPrint(ret, 
                    "Successfully generated fault\n");
            return CL_OK;
		}
		else
        {
			clFaultCliStrPrint(ret, 
                    " Invalid Alarm State. Error in generating fault \n");
            clHeapFree(fRecord);
            return rc;
        }
    }
	else
    {
		CL_DEBUG_PRINT( CL_DEBUG_ERROR,
                ("MOId name to MOId conversion failed. Error in generating fault\n"));
        clFaultCliStrPrint(ret,
                "Name to MOId conversion failed for MOId[%s], error [0x%x]\n", 
                moIdName.value, rc);
    }

	return rc;
}
