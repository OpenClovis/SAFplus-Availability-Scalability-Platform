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
 * File        : clAlarmServerEngine.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module implements alarm engine functionality                      
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include<time.h>
#include <clCommon.h>
#include <sys/time.h>
#include <clDebugApi.h>
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clCorMetaData.h>
#include <clOmApi.h>

#include <clAlarmDefinitions.h>
#include <clAlarmServerAlarmUtil.h>
#include <clAlarmServerEngine.h>
#include <clAlarmErrors.h>
#include <clAlarmCommons.h>
#include <clAlarmInfoCnt.h>
#include <clAlarmPayloadCont.h>
#include <clFaultApi.h>
#include <clEventApi.h>
#include <clEventExtApi.h>
#include <clBitApi.h>

#include <ipi/clAlarmIpi.h>

#include <clIdlApi.h>
#include <clXdrApi.h>
#include <xdrClAlarmHandleInfoIDLT.h>

/********************************** Global *****************************/

/********************************** Static *****************************/
static ClRcT
asEngineAlarmGenerate(ClCorObjectHandleT hMSOObj,ClAlarmInfoT *pAlarmInfo, ClUint32T index, 
        ClAlarmProcessT* pAlarmValues, ClAlarmHandleT alarmHandle);

static ClRcT 
_clAlarmNotify ( ClAlarmInfoT *pAlarmInfo , ClAlarmHandleT alarmHandle);

static ClRcT 
clAlarmGetAlarmValues(ClCorObjectHandleT objH, ClUint32T idx, ClAlarmProcessT* pAlarmValues);

/********************************* Extern ******************************/
extern ClEventChannelHandleT     gAlarmEvtChannelHandle;

/***********************************************************************/

ClRcT 
clAlarmServerEngineAlarmProcess(ClCorObjectHandleT hMSOObj, ClAlarmInfoT *alarmInfo,ClBufferHandleT outMsgHandle)
{
    ClRcT                   rc          = CL_OK;
    ClUint32T               index = 0;
    ClAlarmStateT           alarmState  = alarmInfo->alarmState;
    ClAlarmProbableCauseT   alarmId     = alarmInfo->probCause;
    ClAlarmRuleEntryT       alarmKey    = { .probableCause = alarmInfo->probCause,
                                            .specificProblem = alarmInfo->specificProblem};
    ClAlarmProcessT         alarmValues = {0};

    if ( (rc = clAlarmUtilAlmIdxGet(hMSOObj , 
                    alarmKey , &index))!= CL_OK){
        clLogError("ALE", "ALR", "Unable to get the index for the "
                "probable cause: [%d], Specific Problem [%d]. rc[0x%x] ",alarmId, alarmInfo->specificProblem, rc);
        return rc;
    }    

    clLogDebug ("ALE", "ALR", "Obtained the index [%d] for Probable Cause [%d] and Specific Problem [%d]",
            index, alarmKey.probableCause, alarmKey.specificProblem);

    /* Retrieve the fields necessary for processing this alarm from COR in a 
     * single RMD.
     */
    rc = clAlarmGetAlarmValues(hMSOObj, index, &alarmValues);
    if (rc != CL_OK)
    {
        clLogError("ALE", "ALR", "Failed to retrieve the alarm values from COR. rc [0x%x]", rc);
        return rc;
    }

    clLogTrace("ALE", "ALR", "Going for the suppression and the masking check...");

    rc = clAlarmServerEngineAlarmSuppressionCheck(hMSOObj, index, alarmState, alarmInfo, &alarmValues, outMsgHandle);
    if(CL_OK != rc)
    {
	    clLogError("ALE", "ALR", "Failed while doing suppression check for Alarm Idx [%d]. rc[0x%x]", index, rc);
	    return rc;
    }

    rc = asEnginePEFAttrProcess(hMSOObj, index, alarmState, alarmValues.suppressedAlarms, alarmInfo);
    if(CL_OK != rc)
    {
	    clLogError("ALE", "ALE", "Failed while doing hierarchical masking for alarm index [%d]. rc[0x%x]", 
                index, rc);
        return rc;
    }

    return rc;
}


ClRcT 
clAlarmServerEngineAlarmSuppressionCheck(ClCorObjectHandleT hMSOObj, ClUint32T idx, ClUint8T isAssert, 
        ClAlarmInfoT* pAlarmInfo, ClAlarmProcessT* pAlarmValues, ClBufferHandleT outMsgHandle)
{
    ClRcT           rc = CL_OK;
    ClAlarmHandleT  alarmHandle=0;
    ClUint32T       size=0;
    ClUint64T       alreadyRaisedBM = 0;
    ClUint64T       suppressedAlmBM = 0;
    ClAlarmAttrListT attrList = {0};

    clLogTrace("ASE", "ASC", "Checking the suppression condition for the alarm index [%d] \
                where the published alarms bitmap is [0x%llx], and the suppression alarms bitmap is [0x%llx]",
                idx, pAlarmValues->publishedAlarms, pAlarmValues->suppressedAlarms);

    alreadyRaisedBM = pAlarmValues->publishedAlarms;
    suppressedAlmBM = pAlarmValues->suppressedAlarms;

    if (isAssert == CL_ALARM_STATE_ASSERT)
    {
        /* if not_already_raised || already_suppressed
            check_for_suppression conditions 
        */
        if ( !(alreadyRaisedBM & ((ClUint64T) 1<<idx) ) || 
                      (suppressedAlmBM & ((ClUint64T) 1<<idx)) )
        {

            if ( clAlarmServerEngineAlmToBeSuppressed(hMSOObj, idx, pAlarmValues) == CL_OK )
            {
                clLogTrace("ASE", "ASC", "Suppressing the alarm with alarm index [%d]", idx);

                /* set the alarm as suppressed */
                suppressedAlmBM |= ((ClUint64T) 1 << idx);
                size = sizeof(ClUint64T);
                
                memset(&attrList, 0, sizeof(ClAlarmAttrListT));

                attrList.numOfAttr = 1;
                attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
                attrList.attrInfo[0].index = 0;
                attrList.attrInfo[0].pValue = &suppressedAlmBM;
                attrList.attrInfo[0].size = size;

                rc = clAlarmAttrValueSet(hMSOObj, &attrList);
                if(CL_OK != rc)
                {
                    clLogError("ASE", "ASC", 
                            "Failed while setting the value of suppression \
                            alarms bitmap [0x%llx] for alarm index [%d]. rc[0x%x]", 
                            suppressedAlmBM, idx, rc);
                    return rc;
                }

                clLogTrace("ASE", "ASC", "Alarm Suppression Check: Alarm Index [%d] \
                        Suppressed Alarm Bitmap is [0x%llx]", idx , suppressedAlmBM);
                /*
                 * Code for storing the payload info in the container
                 */
                if(pAlarmInfo->len == 0)
                {
                    clLogTrace("ASE", "ASC", "Alarm Info len is zero for alarm index [%d]", idx);
                }
                else
                {
                    clLogTrace("ASE", "ASC", "Alarm Info len is non-zero[%d] for alarm index [%d]", 
                            pAlarmInfo->len, idx);
                    rc = clAlarmPayloadCntAdd(pAlarmInfo);
                    if (rc != CL_OK)
                    {
                        clLogError("ASE", "ASC", "Failed while adding the alarm information for \
                                alarm index [%d]. rc[0x%x]", idx, rc);
                        return rc;
                    }
                }
            }
            else
            {
                clLogTrace("ASE", "ASC", "Alarm Suppression Check: Alarm index [%d] \
                        not to be suppressed", idx );
                /* not to be suppressed, publish it */
                /* Add alarm Info to container and pass the
                 *  handle back to alarm client */

                rc = clAlarmAlarmInfoAdd(pAlarmInfo,&alarmHandle);   
                if (rc != CL_OK)
                {
                    clLogError("ASE", "ASC", "Failed while adding the alarm information locally for \
                            alarm index [%d]. rc[0x%x]", idx, rc);
                    return rc;
                }

                rc =  clBufferNBytesWrite(outMsgHandle,
                                        (ClUint8T *)&alarmHandle,
                                        sizeof(ClAlarmHandleT));
                 if (rc != CL_OK)
                 {
                     clLogError("ASE", "ASC", "Failed while writing the \
                             alarm handle [%d] in the buffer message. rc[0x%x]", 
                             alarmHandle, rc); 
                     return rc;
                 }

                 rc = asEngineAlarmGenerate(hMSOObj, pAlarmInfo, idx, pAlarmValues, alarmHandle);
                 if (rc != CL_OK)
                 {
                     clLogError("ASE", "ASC", "Alarm generation failed for \
                             alarm handle [%d], alarm index [%d]. rc[0x%x]",
                             alarmHandle, idx, rc);
                     return rc;
                 }
                           
                /* make the alarm as not suppressed */

                if (suppressedAlmBM & ((ClUint64T) 1 << idx))
                {
                    clLogError("ASE", "ASC", "Alarm Suppression Check: For \
                            alarm Index [%d], unsetting the alarm index in \
                            suppressed alarm bitmap.", idx);

                    suppressedAlmBM &= ~((ClUint64T) 1<<idx);
                    /* call clCorObjectSimpleAttributeSet  */
                    size = sizeof(ClUint64T);

                    memset(&attrList, 0, sizeof(ClAlarmAttrListT));

                    attrList.numOfAttr = 1;
                    attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
                    attrList.attrInfo[0].index = 0;
                    attrList.attrInfo[0].pValue = &suppressedAlmBM;
                    attrList.attrInfo[0].size = size;

                    rc = clAlarmAttrValueSet(hMSOObj, &attrList);
                    if(CL_OK != rc)
                    {
                        clLogError("ASE", "ASC", "Failed while setting the \
                                suppressed alarm bitmap [0x%llx] for alarm handle [%d] \
                                and alarm index [%d]. rc[0x%x]", 
                                suppressedAlmBM, alarmHandle, idx, rc);
                        return rc;
                    }

                    clLogTrace("ASE", "ASC", "Alarm Suppression Check: Unsetting the \
                            alarm index [%d] from the suppressed alarms bitmap [0x%llx]", 
                            idx, suppressedAlmBM);
                }
            }
        }
    }
    else
    {
        clLogTrace("ASE", "ASC", " Alarm Suppression Check: Alarm Index [%d] for \
                the alarm clear.", idx );

        /*  change to forward a alarm clear, even if 
        the assert for it is not received yet. 
        This is for system bring up time */

        /* check for first time raise */

        /* if already_raised - clear */
        if ( alreadyRaisedBM & ((ClUint64T) 1<<idx) )
        {
            clLogTrace("ASE", "ASC", 
                    "Alarm Suppression Check: Alarm Index [%d] alarm already raised so clearing it. ", idx);

            /* Delete alarm Info from the  container */
            rc = clAlarmAlarmInfoToHandleGet(pAlarmInfo,&alarmHandle);
            if (rc != CL_OK)
            {
                if( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
                    clLogError("ASE", "ASC", "Failed while getting the alarm information alarm index[%d]. rc[0x%x]",
                        idx, rc);
                else
                    rc = CL_OK;
                return rc;
            }

            rc = clAlarmAlarmInfoDelete(alarmHandle);   
            if (rc != CL_OK)
            {
                clLogError("ASE", "ASC", "Failed while deleting the alarm information for alarm index[%d]. rc[0x%x]", idx, rc);
                return rc;
            }

            rc =  clBufferNBytesWrite(outMsgHandle,
                                    (ClUint8T *)&alarmHandle,
                                    sizeof(ClAlarmHandleT));
            if (rc != CL_OK)
            {
                clLogError("ASE", "ASC", 
                        "Failed while writing the data into buffer for alarm index[%d] and handle [%d]. rc[0x%x]",
                            idx, alarmHandle, rc);
                return rc;
            }

            rc = asEngineAlarmGenerate(hMSOObj,pAlarmInfo, idx, pAlarmValues, alarmHandle);
            if (rc != CL_OK)
            {
                clLogError("ASE", "ASC", "Failed while generating the alarm for the alarm handle [%d]. rc[0x%x]",
                        alarmHandle, rc);
                return rc;
            }
        }

        if ( suppressedAlmBM & ((ClUint64T) 1<<idx) )
        {
            /* if suppressed, clear the suppression condition */
            /* check condition for affected alarms */

            clLogTrace("ASE", "ASC", "Alarm Suppression Check: alarm index [%d] alarm is suppressed so clearing it.", idx);

            suppressedAlmBM &= ~((ClUint64T) 1<<idx);

            /* call clCorObjectSimpleAttributeSet  */
            size = sizeof(ClUint64T);

            memset(&attrList, 0, sizeof(ClAlarmAttrListT));

            attrList.numOfAttr = 1;
            attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
            attrList.attrInfo[0].index = 0;
            attrList.attrInfo[0].pValue = &suppressedAlmBM;
            attrList.attrInfo[0].size = size;

            rc = clAlarmAttrValueSet(hMSOObj, &attrList);
            if(CL_OK != rc)
            {
                clLogError("ASE", "ALC", "Failed while setting the suppression \
                        alarm bitmap [0x%llx] for alarm index [%d]. rc[0x%x]", 
                        suppressedAlmBM, idx, rc);
                return rc;
            }

            clLogTrace("ASE", "ACE", "Alarm Suppression Check: For alarm index [%d] the \
                    suppressed Alarm Bitmap is[0x%llx]", idx, suppressedAlmBM);

            return rc;
        }
    }

    return rc;
}

ClRcT 
asEnginePEFAttrProcess(ClCorObjectHandleT hMSOObj, ClUint32T idx, 
                        ClUint8T isAssert, ClUint64T suppressedAlmBM, 
                        ClAlarmInfoT* pAlarmInfo)
{
    ClRcT        rc = CL_OK;
    ClUint32T size=0;
	PefInfoT PEFInfo;
    ClAlarmCategoryTypeT alarmCategory = CL_ALARM_CATEGORY_INVALID;
    ClAlarmAttrListT    attrList = {0};

    if (pAlarmInfo == NULL)
    {
        clLogError("ASE", "PEM", "Alarm info passed is NULL.");
        return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
    }

    alarmCategory = pAlarmInfo->category;

    size = sizeof(ClUint8T);

    attrList.numOfAttr = 1;
    attrList.attrInfo[0].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.attrInfo[0].index = 0;
    attrList.attrInfo[0].pValue = &(PEFInfo.isPEFpresent);
    attrList.attrInfo[0].size = size;

    rc = clAlarmAttrValueGet(hMSOObj, &attrList); 
    if (CL_OK != rc)
    {
        clLogError("ASE", "PEM", "Failed while getting the PARENT_ENTITY_FAILED attribute of alarm MSO. rc[0x%x]", rc);
        return rc;
    }

	PEFInfo.isPEFpresent |= clAlarmCategory2BitmapTranslate(alarmCategory);

	clLogTrace("ASE", "PEG", "The parent entity failed bitmap for parent is [%d]", PEFInfo.isPEFpresent);

	if (isAssert == CL_ALARM_STATE_ASSERT)
	{
	    /* Get the latest value of the suppressed alarm bitmap for the alarm raised */
        size = sizeof(ClUint64T);

        attrList.numOfAttr = 1;
        attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS; 
        attrList.attrInfo[0].index = 0;
        attrList.attrInfo[0].pValue = &(suppressedAlmBM); 
        attrList.attrInfo[0].size = size;

        rc = clAlarmAttrValueGet(hMSOObj, &attrList);
        if (CL_OK != rc)
        {
            clLogError("ASE", "PEM", "Failed while getting the suppressed alarm bitmap. rc[0x%x]", rc);
            return rc;
        }

		clLogTrace("ASE", "PEM", "ASSERT: The suppressed alarm bitmap is [%lld] , alarm index [%d]", suppressedAlmBM, idx);

		if ( (suppressedAlmBM & ( (ClUint64T) 1 << idx)) != 0)
		{
			clLogError("ASE", "PEM", "No need to update the parent-entity-failed bitmap "
							"for the children as the current alarm with index [%d] has been suppressed", idx);
			return CL_OK;
		}

		PEFInfo.alarmState = 1;
	}
	else
	{
		clLogTrace("ASE", "PEM", "CLEAR: The suppressed alarm bitmap is [%lld] , alarm index [%d]", suppressedAlmBM, idx);

		if ( (suppressedAlmBM & ((ClUint64T) 1 << idx)) != 0 )
		{
			clLogError("ASE", "PEM", "CLEAR: No need to update the parent-entity-failed bitmap for the children"
							" as the current alarm was in suppressed state before being cleared. ");
			return CL_OK;
		}

		PEFInfo.alarmState = 0;
	}

	PEFInfo.probCauseId = idx;
	PEFInfo.moID = pAlarmInfo->moId;
	
	rc = clCorObjectWalk(&(pAlarmInfo->moId), NULL, asEnginePefSet, 
					 CL_COR_MSO_WALK, &PEFInfo);
	if( CL_OK != rc )
	{
		clLogError("ASE", "PEM", "Failed while walking all the children ALARM MSOs. rc[0x%x]", rc);
		return rc;
	}            

	return CL_OK;
}

ClRcT 
asEnginePefSet(void* pData, void *cookie)
{
    ClCorObjectHandleT      hCorObj = *(ClCorObjectHandleT*)pData;    
    ClCorServiceIdT         SvcId = CL_COR_INVALID_SVC_ID;
    ClCorMOIdPtrT           hMOId = NULL;
    ClRcT                   rc = CL_OK;
    ClUint32T               size;
    ClUint64T               suppressedAlmBM=0;
    ClUint64T               newSuppressedAlmBM=0;
    ClUint16T               index=0;
	ClUint8T                pefEntry=0;
    ClUint8T                alarmEnabled = 0;
	ClUint8T                alarmContainmentValidEntry = 0;    
    ClUint8T                severity = 0;
	PefInfoT                PEFInfo = *(PefInfoT*)cookie;
    ClAlarmHandleT          alarmHandle;
    ClAlarmInfoT            *CurrentAlarm;
    ClAlarmProcessT         alarmValues = {0};
    ClAlarmAttrListT        attrList = {0};

    /*
     * payload specific info
     */
    ClAlarmProbableCauseT alarmprobCause;
    ClAlarmSpecificProblemT alarmspecificProblem;
    ClUint8T     alarmCategory;
    ClAlarmPayloadCntT *payloadInfo = NULL;

    if ( ( (rc = clCorMoIdAlloc(&hMOId)) != CL_OK) || (hMOId == NULL) )
    {
        clLogError("ASE", "PEG", "Failed to allocate buffer for MOID. rc [0x%x]", rc);
        return rc;
    }

    if ( ( rc = clCorObjectHandleToMoIdGet(hCorObj, hMOId, &SvcId)) != CL_OK)
    {
        clLogError("ASE", "PEG", 
                "Failed to get MO ID from an MO obj. rc[0x%x]", rc);
        clCorMoIdFree (hMOId);
        return rc;
    }
    
    if (SvcId != CL_COR_SVC_ID_ALARM_MANAGEMENT)
    {
        /* Not a alarm mso, return CL_OK */
        clCorMoIdFree(hMOId);
        return CL_OK;
    }

    if ( clCorMoIdCompare( &(PEFInfo.moID), hMOId) == 0 )
    {
        clCorMoIdFree (hMOId);
        return CL_OK;
    }
    
    size = sizeof(ClUint8T);

    attrList.numOfAttr = 1;
    attrList.attrInfo[0].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.attrInfo[0].index = 0;
    attrList.attrInfo[0].pValue = &(pefEntry);
    attrList.attrInfo[0].size = size;

    rc = clAlarmAttrValueGet(hCorObj, &attrList);
	if (CL_OK != rc)
	{
		clLogError("ASE", "PEG", "Failed while getting the value of the PEF attribute. rc[0x%x]", rc);
		clCorMoIdFree(hMOId);
		return rc;
	}

	if(PEFInfo.alarmState == CL_ALARM_STATE_ASSERT)
		PEFInfo.isPEFpresent |= pefEntry;
	else
		PEFInfo.isPEFpresent = (~PEFInfo.isPEFpresent) & pefEntry;

	clLogTrace("ASE", "PEG", "The parent entity failed bitmap for the child is [%d]", PEFInfo.isPEFpresent);

    /**
    * If assert is true for the given alarm, then turn on parent entity failed to true for each descendent 
    */
    size = sizeof(ClUint8T);

    memset(&attrList, 0, sizeof(ClAlarmAttrListT));

    attrList.numOfAttr = 1;
    attrList.attrInfo[0].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.attrInfo[0].index = 0;
    attrList.attrInfo[0].pValue = &(PEFInfo.isPEFpresent);
    attrList.attrInfo[0].size = size;

    rc  = clAlarmAttrValueSet(hCorObj, &attrList);
	if (CL_OK != rc)
	{
		clLogError("ASE", "PEG", "Failed while setting the PEF attribute. rc[0x%x]", rc);
		clCorMoIdFree(hMOId);
		return rc;
	}

	if(PEFInfo.alarmState == CL_ALARM_STATE_CLEAR)
	{
		size = sizeof(ClUint64T);

        attrList.numOfAttr = 1;
        attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
        attrList.attrInfo[0].index = 0;
        attrList.attrInfo[0].pValue = &suppressedAlmBM;
        attrList.attrInfo[0].size = size;

		rc = clAlarmAttrValueGet(hCorObj, &attrList);
        if (CL_OK != rc)
        {
            clLogError("ASE", "PEG", "Failed to get the suppression alarms bitmap. rc[0x%x]", rc);
		    clCorMoIdFree(hMOId);
            return rc;
        }

		index = 0;
		newSuppressedAlmBM = suppressedAlmBM;
		while(suppressedAlmBM)
		{
            rc = clAlarmGetAlarmValues(hCorObj, index, &alarmValues);
            if (rc != CL_OK)
            {
                clLogError("ALE", "ALR", "Failed to retrieve the alarm values from COR. rc [0x%x]", 
                        rc);
                clCorMoIdFree(hMOId);
                return rc;
            }

            alarmEnabled = alarmValues.alarmEnable;
            if (!alarmEnabled)
            {
				clLogError("ASE", "PEG", 
                        "The Alarm with index [%d] cannot be processed since it is  suspended ", index);
				suppressedAlmBM >>= 1;
				index++;
				continue;
            }

            alarmContainmentValidEntry = alarmValues.contAttrValidEntry;

			if ( alarmContainmentValidEntry == 0 )
			{
				clLogError("ASE", "PEG", 
                            "The Alarm with index [%d] cannot be processed since"
                            " the containment entry is invalid", index);
				suppressedAlmBM >>= 1;
				index++;
				continue;
			}

            alarmCategory = alarmValues.category;

			if(!(PEFInfo.isPEFpresent &
						clAlarmCategory2BitmapTranslate(alarmCategory)))
			{
				clLogTrace("ASE", "PEG", "Got through the parent-entity-failed validation "
										"check for alarm index [%d] with category [%d]",
										alarmCategory, index);

				/* set the category, subcat and severity */                    
				if(suppressedAlmBM & 1)
				{
					clLogTrace("ASE", "PEG", "The alarm with index [%d] is suppressed, doing the "
									"suppression check again", index);

					rc = clAlarmServerEngineAlmToBeSuppressed(hCorObj, 
															  index,
															  &alarmValues);
					if (CL_OK != rc )
					{
						clLogTrace("ASE", "PEG", "The alarm index [%d] is not to be suppressed.", index);

						newSuppressedAlmBM &= ~( (ClUint64T) 1<<index);
                        
                        alarmprobCause = alarmValues.probableCause;
                        alarmspecificProblem = alarmValues.specificProblem;
                        severity = alarmValues.severity;

                        /*
                         * Getting the payload info from the container 
                         */
                        rc = clAlarmPayloadCntDataGet(alarmprobCause, alarmspecificProblem, hMOId, &payloadInfo);
						if (CL_OK == rc)
						{
                            CurrentAlarm = clHeapAllocate(sizeof(ClAlarmInfoT)+payloadInfo->len);
                            if(CurrentAlarm == NULL)
                            {
                                clLogError("ASE", "PEG", 
                                        "Failed to allocate the memory for index[%d]", index);
                                clCorMoIdFree (hMOId);                            
                                return rc;
                            }

                            CurrentAlarm->len = payloadInfo->len;
                            memcpy(CurrentAlarm->buff, payloadInfo->buff,payloadInfo->len);
                            /*
                             * Deleting the node from the container 
                             */
                            rc = clAlarmPayloadCntDelete(hMOId, alarmprobCause, alarmspecificProblem);
                            if (CL_OK != rc)
                            {
                                clLogError("ASE", "PEG", "Failed to delete the node from the"
                                           " Payload repository. rc[0x%x]", rc);
                                clHeapFree(CurrentAlarm);
                                clCorMoIdFree (hMOId);                            
                                return rc; 
                            }
						}
                        else
                        {
                            clLogInfo("ASE", "PEG", 
                                    "The paylaod for the alarm index [%d] does not exist", index);
                            CurrentAlarm = clHeapAllocate(sizeof(ClAlarmInfoT));
                            if(CurrentAlarm == NULL)
                            {
                                clLogError("ASE", "PEG", "Failed to allocate the"
                                        " memory for alarm information for alarm index[%d]", index);
                                clCorMoIdFree (hMOId);                            
                                return rc;
                            }
                        }

                        CurrentAlarm->category = alarmCategory;
                        CurrentAlarm->probCause = alarmprobCause;
                        CurrentAlarm->specificProblem = alarmspecificProblem;

                        CurrentAlarm->severity = severity;
						CurrentAlarm->moId = *hMOId;
						CurrentAlarm->alarmState = CL_ALARM_STATE_ASSERT;
						
                        rc = clAlarmAlarmInfoAdd(CurrentAlarm,&alarmHandle);   
                        if (rc != CL_OK)
                        {
                            clLogError("ASE", "PEG" ,
                                    "Failed while adding the alarm info for alarm index [%d]. rc[0x%x]", index, rc);
                            clCorMoIdFree(hMOId);
                            clHeapFree(CurrentAlarm);
                            return rc;
                        }

						/* none of the peer alarms are suppressing it.so generate the alarm */
						rc = asEngineAlarmGenerate(hCorObj, CurrentAlarm, index, &alarmValues, alarmHandle);
                        if (CL_OK != rc)
                        {
                            clLogError("ASE", "PEG",
                                    "Failed while raising the alarm for index [%d]. rc[0x%x]",
                                    index, rc);
                        }
                        clHeapFree(CurrentAlarm);
						/* SSS: to be fixed. AS may also need to have its own OM object */
					}
				}
			}
			suppressedAlmBM >>= 1;
			index++;
		}

		size = sizeof(ClUint64T);

        memset(&attrList, 0, sizeof(ClAlarmAttrListT));

        attrList.numOfAttr = 1;
        attrList.attrInfo[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
        attrList.attrInfo[0].index = 0;
        attrList.attrInfo[0].pValue = &newSuppressedAlmBM;
        attrList.attrInfo[0].size = size;

		rc = clAlarmAttrValueSet(hCorObj, &attrList); 
		if (CL_OK != rc)
		{
			clLogError("ASE", "PEG", 
                    "Failed while setting the suppressed alarm bitmap. rc[0x%x]", rc);
			clCorMoIdFree (hMOId);            
			return rc;
		}
	}

    clCorMoIdFree (hMOId);
    return CL_OK;
}

ClRcT
clAlarmServerEngineAlmToBeSuppressed(ClCorObjectHandleT hMSOObj, 
                                     ClUint32T idx,
                                     ClAlarmProcessT* pAlarmValues)
{
	ClRcT                   rc = CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
	ClUint8T                parentEntityFailed = 0;
	ClUint64T               suppressCondition = 0;
	ClUint64T               almsAfterSoaking = 0;
	ClUint32T               almResult = 0;
	ClAlarmRuleRelationT    relation = CL_ALARM_RULE_NO_RELATION;
    ClAlarmCategoryTypeT    alarmCategory = 0;

	clLogTrace("ASE", "SCK", "Checking for the suppression alarms. Alarm Index[ %d] ", idx );

    parentEntityFailed = pAlarmValues->parentEntityFailed;
    alarmCategory = pAlarmValues->category;
    suppressCondition = pAlarmValues->supRule;
    relation = pAlarmValues->supRuleRel;
    almsAfterSoaking = pAlarmValues->afterSoakingBM;

    if(parentEntityFailed & clAlarmCategory2BitmapTranslate(alarmCategory))
    {
		clLogNotice("ASE", "SCK", "Suppressing the alarm as its parent entity failed earlier ..");
        return CL_OK;
    }

	clLogTrace("ASE", "SCK", "For the alarm Id: [%d], the "
				   "suppressCondition: [%llx], almsAfterSoaking:[%llx], "
                   "relation:[%d] ", idx ,suppressCondition, almsAfterSoaking, 
                   relation);

	almResult = suppressCondition & almsAfterSoaking;

	clLogTrace("ASE", "SCK", "For the alarm ID: [%d], the alarm suppression check result is [%d]",
				   idx, almResult );


	if ( (relation == CL_ALARM_RULE_LOGICAL_AND) && (almResult == suppressCondition) )
		rc = CL_OK;
	else if ( ((relation == CL_ALARM_RULE_LOGICAL_OR) || (relation == CL_ALARM_RULE_NO_RELATION)) 
			 && almResult )
		rc = CL_OK;
	else
		rc = CL_ERR_NOT_EXIST;

	if (CL_OK == rc)
		clLogNotice("ASE", "SCK", "The alarm with index [%d] is suppressed as the "
						"suppression rule is satisfied.", idx);

    return rc;
}




/**
 * Function to publish the event in both clear or raise condition.
 * It also send the request to fault manager incase of non-service 
 * impacting alarms.
 */

ClRcT 
asEngineAlarmGenerate(ClCorObjectHandleT hMSOObj,
            ClAlarmInfoT *pAlarmInfo, 
            ClUint32T index, 
            ClAlarmProcessT* pAlarmValues, 
            ClAlarmHandleT alarmHandle)
{
    ClRcT           rc = CL_OK;
    ClUint32T       size = 0;
    ClUint64T       publishedAlms = 0;
    struct timeval  alarmTime  = {0};
    ClAlarmAttrListPtrT pAttrList = NULL;

    CL_ALARM_FUNC_ENTER();

    if ( NULL == pAlarmInfo )
    {
        clLogError("ASE", "AMP", "The pointer to the alarm info structure passed is NULL. rc[0x%x]", rc);
        CL_ALARM_FUNC_EXIT();
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    pAttrList = (ClAlarmAttrListPtrT ) clHeapAllocate (
            sizeof(ClAlarmAttrListT) + (2 * sizeof(ClAlarmAttrInfoT)));
    if (pAttrList == NULL)
    {
        clLogError("ASE", "AMP", "Failed to allocate memory.");
        CL_ALARM_FUNC_EXIT();
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    memset(pAttrList, 0, sizeof(ClAlarmAttrListT) + (2 * sizeof(ClAlarmAttrInfoT)));

    pAttrList->numOfAttr = 3;

    /* mark the alarm as published already */
    publishedAlms = pAlarmValues->publishedAlarms;

    if (pAlarmInfo->alarmState == CL_ALARM_STATE_ASSERT)
            publishedAlms |= ((ClUint64T) 1<<index);
    else
            publishedAlms &= ~((ClUint64T) 1<<index);

    size = sizeof(ClUint64T);

    pAttrList->attrInfo[0].attrId = CL_ALARM_PUBLISHED_ALARMS;
    pAttrList->attrInfo[0].index = index;
    pAttrList->attrInfo[0].pValue = &publishedAlms;
    pAttrList->attrInfo[0].size = size;

	/* make use of the info from sCurrentAlmEvent; */
    size = sizeof(pAlarmInfo->alarmState);

    pAttrList->attrInfo[1].attrId = CL_ALARM_ACTIVE; 
    pAttrList->attrInfo[1].index = index;
    pAttrList->attrInfo[1].pValue = &(pAlarmInfo->alarmState); 
    pAttrList->attrInfo[1].size = size;
    
    if(CL_ALARM_STATE_CLEAR == pAlarmInfo->alarmState)
            pAlarmInfo->eventTime = 0;
	if(CL_ALARM_STATE_ASSERT == pAlarmInfo->alarmState &&
			pAlarmInfo->eventTime == 0)
	{
		gettimeofday(&alarmTime,NULL);
        pAlarmInfo->eventTime = alarmTime.tv_sec;
	}

    size = sizeof(pAlarmInfo->eventTime);

    pAttrList->attrInfo[2].attrId = CL_ALARM_EVENT_TIME; 
    pAttrList->attrInfo[2].index = index;
    pAttrList->attrInfo[2].pValue = &(pAlarmInfo->eventTime); 
    pAttrList->attrInfo[2].size = size;

    rc = clAlarmAttrValueSet(hMSOObj, pAttrList);
    if (rc != CL_OK)
    {
        clLogError("ASE", "AMP", "Failed to set the value of CL_ALARM_PUBLISHED_ALARMS, "
                "CL_ALARM_ACTIVE, and CL_ALARM_EVENT_TIME while raising/clearing the alarm [%d]. rc [0x%x]",
                alarmHandle, rc);
        clHeapFree(pAttrList);
        return rc;
    }

    clHeapFree(pAttrList);

    rc = _clAlarmNotify ( pAlarmInfo, alarmHandle );
    if(CL_OK != rc)
    {
        clLogError("ASE", "AMP", "Failed while publishing the alarm for the alarm handle [%d]. rc[0x%x]", 
                alarmHandle, rc);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }

    clLogDebug("ASE", "ALR", " Successfully publishing the event for the alarm: cat [%d], severity [%d], "
            "probCause [%d], specProb [%d]  with  handle [%d].", 
            pAlarmInfo->category, pAlarmInfo->severity, pAlarmInfo->probCause, 
            pAlarmInfo->specificProblem, alarmHandle);

	if( pAlarmInfo->severity >= CL_ALARM_SEVERITY_MINOR &&
			pAlarmInfo->severity <= CL_ALARM_SEVERITY_CLEAR)
	{
        ClCorMOIdT moId = {{{0}}};

        clLogTrace("ASE", "ALR", "Non-Service-Impacting alarm... so calling fault manager report function");

        if (CL_ALARM_STATE_CLEAR == pAlarmInfo->alarmState)
        {
            clLogInfo ("ASE", "ALR", "As the alarm state is CLEAR, so not doing fault reporting for this state.");
            CL_ALARM_FUNC_EXIT();
            return CL_OK;

        }

        memcpy(&moId, &(pAlarmInfo->moId), sizeof(ClCorMOIdT));

        clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);

        rc = clFaultReport(&(pAlarmInfo->compName),
                  &moId,
                  pAlarmInfo->alarmState,
                  pAlarmInfo->category,
                  pAlarmInfo->specificProblem,
                  pAlarmInfo->severity,
                  pAlarmInfo->probCause,
                  pAlarmInfo->buff,
                  pAlarmInfo->len);

        if (CL_OK != rc)
        {
            clLogError("ASE", "AMP", 
                    "Failed while reporting the non-service impacting alarms to fault manager. rc[0x%x]", rc);
            CL_ALARM_FUNC_EXIT();
            return rc;
        }
	}
    
    CL_ALARM_FUNC_EXIT();
    return rc;
}

/**
 * Internal function to publish the alarm event. 
 */ 

ClRcT 
_clAlarmNotify ( ClAlarmInfoT *pAlarmInfo , ClAlarmHandleT alarmHandle)
{
    ClRcT                   rc = CL_OK;
    ClEventIdT              eventId = 0;
    ClNameT                 publisherName = {strlen("ALARM_SERVER"), "ALARM_SERVER"}; 
    ClSizeT                 eventSize = 0;
    ClUint32T               eventType = 0, bufferSize = 0;
    ClBufferHandleT         inMsgHandle;
    ClUint8T                *pData = NULL;
    ClAlarmHandleInfoIDLT   alarmHandleInfoIdl = {0};
    ClEventHandleT          alarmEvtHdl = 0;
    ClNameT                 moIdName;

    CL_ALARM_FUNC_ENTER();

    if(NULL == pAlarmInfo)
    {
        clLogError("ASE", "EVT", "The pointer to the Alarm information passed is NULL. ");
        CL_ALARM_FUNC_EXIT();
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

	alarmHandleInfoIdl.alarmInfo.buff = clHeapAllocate(pAlarmInfo->len);
    if(NULL == alarmHandleInfoIdl.alarmInfo.buff)
    {
        clLogError("ASE", "EVT", "Failed while allocating the memory for the alarm buffer. rc[0x%x] ", rc);
        CL_ALARM_FUNC_EXIT();
        return (CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
    }    


	alarmHandleInfoIdl.alarmInfo.probCause = pAlarmInfo->probCause;

	alarmHandleInfoIdl.alarmInfo.compName.length = (pAlarmInfo->compName).length;

	if ( alarmHandleInfoIdl.alarmInfo.compName.length  > 0 )
    {
	    memcpy ( alarmHandleInfoIdl.alarmInfo.compName.value, 
                    ( pAlarmInfo->compName ).value,
                    alarmHandleInfoIdl.alarmInfo.compName.length );
    }

	alarmHandleInfoIdl.alarmInfo.moId = pAlarmInfo->moId;

	alarmHandleInfoIdl.alarmInfo.alarmState = pAlarmInfo->alarmState;
	
    alarmHandleInfoIdl.alarmInfo.category = pAlarmInfo->category;
	
    alarmHandleInfoIdl.alarmInfo.specificProblem = pAlarmInfo->specificProblem;
	
    alarmHandleInfoIdl.alarmInfo.severity = pAlarmInfo->severity;
	
    alarmHandleInfoIdl.alarmInfo.eventTime = pAlarmInfo->eventTime;
	
    alarmHandleInfoIdl.alarmInfo.len = pAlarmInfo->len;
	
    memcpy ( alarmHandleInfoIdl.alarmInfo.buff,
                 pAlarmInfo->buff, 
                 alarmHandleInfoIdl.alarmInfo.len);
    
    alarmHandleInfoIdl.alarmHandle = alarmHandle;

    rc = clCorMoIdToMoIdNameGet(&alarmHandleInfoIdl.alarmInfo.moId, &moIdName);
    if (rc != CL_OK)
    {
        clLogError("ASE", "EVT", "Failed to get moId name from moId from cor. rc [0x%x]", rc);
        clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }

	clLogNotice( "ASE",  "EVT", " Alarm Info of the alarm being [%s] is \n, \
				MoId [%s], Probable cause [%d], Component Name [%s], Alarm State [%d], \
			    Category [%d], Specific problem [%d], Severity [%d], Event Time [%lld] , \
			    Payload length [%d], Alarm Handle [%d]",
			    alarmHandleInfoIdl.alarmInfo.alarmState ? "Raised" : "Cleared", moIdName.value,
			    alarmHandleInfoIdl.alarmInfo.probCause, alarmHandleInfoIdl.alarmInfo.compName.value,
			    alarmHandleInfoIdl.alarmInfo.alarmState, alarmHandleInfoIdl.alarmInfo.category,
			    alarmHandleInfoIdl.alarmInfo.specificProblem, alarmHandleInfoIdl.alarmInfo.severity,
			    alarmHandleInfoIdl.alarmInfo.eventTime, alarmHandleInfoIdl.alarmInfo.len, 
                alarmHandleInfoIdl.alarmHandle);	

    rc = clBufferCreate(&inMsgHandle);
    if (rc != CL_OK)
    {
        clLogError("ASE", "EVT", "Could not create buffer for alarm output message . rc[0x%x]", rc);
        clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }
    
    rc = VDECL_VER(clXdrMarshallClAlarmHandleInfoIDLT, 4, 0, 0)((void *)&alarmHandleInfoIdl,inMsgHandle,0);
    if (CL_OK != rc)
    {
        clBufferDelete(&inMsgHandle);
        clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
        clLogError("ASE", "EVT", "Failed while writing the alarm data into buffer message. rc[0x%x]", rc );
        CL_ALARM_FUNC_EXIT();
        return rc;
    }

    rc = clBufferFlatten(inMsgHandle,&pData);
    if (CL_OK != rc)
    {
        clBufferDelete(&inMsgHandle);
        clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
        clLogError("ASE", "EVT", "Failed while flattening the buffer. rc[0x%x]", rc);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }

    clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);

    rc = clBufferLengthGet(inMsgHandle, &bufferSize);
    if (CL_OK != rc)
    {
        /**
         * The Event length contains the size of the ClAlarmHandleInfoIDLT structure + sizeof the 
         * alarm buffer passed. From this we need to substract the sizeof the alarm buffer pointer
         * which is part of the structure, as this is not counted in the Marshaled data.
         */ 
        eventSize = sizeof(ClAlarmHandleInfoIDLT) + pAlarmInfo->len 
          - sizeof (alarmHandleInfoIdl.alarmInfo.buff);

        clLogNotice("ASE", "EVT", "Failed while getting the size from the buffer so calculating it \
                on its own. rc[0x%x]", rc);
    }
    else
        eventSize = bufferSize;

    clBufferDelete(&inMsgHandle);
    
    rc = clEventAllocate(gAlarmEvtChannelHandle, &alarmEvtHdl);    
    if (CL_OK != rc)
    {
        clLogError("ASE", "EVT", "Failed while allocating event handle. rc[0x%x]", rc);
        clHeapFree(pData);
        CL_ALARM_FUNC_EXIT();
        return (rc);
    }    
    
    /* Setting the event type in a network byte format. */
    eventType = CL_BIT_H2N32(CL_ALARM_EVENT);

    /* Set the attributes */
    rc = clEventExtAttributesSet(alarmEvtHdl, eventType, CL_EVENT_HIGHEST_PRIORITY,
            0, &publisherName);
    if (CL_OK != rc)
    {
        clLogError("ASE", "EVT", "Failed while setting the event set. rc[0x%x] ", rc);
        clEventFree(alarmEvtHdl);    
        clHeapFree(pData);
        CL_ALARM_FUNC_EXIT();
        return (rc);
    }    

    rc = clEventPublish(alarmEvtHdl, pData, eventSize, &eventId );
    if (CL_OK != rc)
    {
        clLogError("ASE", "EVT", "Failed while publishing the event. rc[0x%x] ", rc);
        clEventFree(alarmEvtHdl);    
        clHeapFree(pData);
        CL_ALARM_FUNC_EXIT();
        return (rc);
    }    

    clHeapFree(pData);

    rc = clEventFree(alarmEvtHdl);    
    if (CL_OK != rc)
    {
        clLogError("ASE", "EVT", "Failed while freeing the event handle. rc [0x%x]", rc);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }            

    CL_ALARM_FUNC_EXIT();
    return rc;
}

ClRcT clAlarmGetAlarmValues(ClCorObjectHandleT objH, ClUint32T idx, ClAlarmProcessT* pAlarmValues)
{
    ClRcT rc = CL_OK;
    ClCorBundleHandleT bundleHandle = CL_HANDLE_INVALID_VALUE;
    ClCorBundleConfigT bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClUint32T i = 0;

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to initialize the bundle. rc [0x%x]", rc);
        return rc;
    }

    attrList.numOfDescriptor = 13;
    attrList.pAttrDescriptor = (ClCorAttrValueDescriptorT *) clHeapAllocate (
            attrList.numOfDescriptor * sizeof(ClCorAttrValueDescriptorT));
    if (attrList.pAttrDescriptor == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    memset(attrList.pAttrDescriptor, 0, attrList.numOfDescriptor * sizeof(ClCorAttrValueDescriptorT));

    /* Suppressed Alarms */
    attrList.pAttrDescriptor[0].attrId = CL_ALARM_SUPPRESSED_ALARMS;
    attrList.pAttrDescriptor[0].index = -1;
    attrList.pAttrDescriptor[0].bufferPtr = &(pAlarmValues->suppressedAlarms);
    attrList.pAttrDescriptor[0].bufferSize = sizeof(ClUint64T);
    attrList.pAttrDescriptor[0].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[0].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        clHeapFree(attrList.pAttrDescriptor);
        clCorBundleFinalize(bundleHandle);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    /* Published Alarms */
    attrList.pAttrDescriptor[1].attrId = CL_ALARM_PUBLISHED_ALARMS;
    attrList.pAttrDescriptor[1].index = -1;
    attrList.pAttrDescriptor[1].bufferPtr = &(pAlarmValues->publishedAlarms);
    attrList.pAttrDescriptor[1].bufferSize = sizeof(ClUint64T);
    attrList.pAttrDescriptor[1].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[1].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Parent Entity Failed Bitmap */
    attrList.pAttrDescriptor[2].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.pAttrDescriptor[2].index = -1;
    attrList.pAttrDescriptor[2].bufferPtr = &(pAlarmValues->parentEntityFailed);
    attrList.pAttrDescriptor[2].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[2].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[2].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* After Soaking Bitmap */
    attrList.pAttrDescriptor[3].attrId = CL_ALARM_ALMS_AFTER_SOAKING_BITMAP;
    attrList.pAttrDescriptor[3].index = -1;
    attrList.pAttrDescriptor[3].bufferPtr = &(pAlarmValues->afterSoakingBM);
    attrList.pAttrDescriptor[3].bufferSize = sizeof(ClUint64T);
    attrList.pAttrDescriptor[3].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[3].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Information : Suppression Rule */
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[4].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[4].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[4].attrId = CL_ALARM_SUPPRESSION_RULE;
    attrList.pAttrDescriptor[4].index = -1;
    attrList.pAttrDescriptor[4].bufferPtr = &(pAlarmValues->supRule);
    attrList.pAttrDescriptor[4].bufferSize = sizeof(ClUint64T);
    attrList.pAttrDescriptor[4].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[4].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));

        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Information : Suppression Rule Relation */
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[5].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[5].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[5].attrId = CL_ALARM_SUPPRESSION_RULE_RELATION;
    attrList.pAttrDescriptor[5].index = -1;
    attrList.pAttrDescriptor[5].bufferPtr = &(pAlarmValues->supRuleRel);
    attrList.pAttrDescriptor[5].bufferSize = sizeof(ClUint32T);
    attrList.pAttrDescriptor[5].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[5].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Suspend */
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[6].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[6].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[6].attrId = CL_ALARM_SUSPEND;
    attrList.pAttrDescriptor[6].index = -1;
    attrList.pAttrDescriptor[6].bufferPtr = &(pAlarmValues->alarmSuspend);
    attrList.pAttrDescriptor[6].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[6].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[6].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Category */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[7].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[7].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[7].attrId = CL_ALARM_CATEGORY;
    attrList.pAttrDescriptor[7].index = -1;
    attrList.pAttrDescriptor[7].bufferPtr = &(pAlarmValues->category);
    attrList.pAttrDescriptor[7].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[7].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[7].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Severity */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[8].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[8].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[8].attrId = CL_ALARM_SEVERITY;
    attrList.pAttrDescriptor[8].index = -1;
    attrList.pAttrDescriptor[8].bufferPtr = &(pAlarmValues->severity);
    attrList.pAttrDescriptor[8].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[8].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[8].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Containment Valid Entry */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[9].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[9].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[9].attrId = CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY;
    attrList.pAttrDescriptor[9].index = -1;
    attrList.pAttrDescriptor[9].bufferPtr = &(pAlarmValues->contAttrValidEntry);
    attrList.pAttrDescriptor[9].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[9].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[9].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Probable Cause */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[10].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[10].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[10].attrId = CL_ALARM_PROBABLE_CAUSE;
    attrList.pAttrDescriptor[10].index = -1;
    attrList.pAttrDescriptor[10].bufferPtr = &(pAlarmValues->probableCause);
    attrList.pAttrDescriptor[10].bufferSize = sizeof(ClUint32T);
    attrList.pAttrDescriptor[10].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[10].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Specific Problem */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[11].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[11].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[11].attrId = CL_ALARM_SPECIFIC_PROBLEM;
    attrList.pAttrDescriptor[11].index = -1;
    attrList.pAttrDescriptor[11].bufferPtr = &(pAlarmValues->specificProblem);
    attrList.pAttrDescriptor[11].bufferSize = sizeof(ClUint32T);
    attrList.pAttrDescriptor[11].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[11].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    /* Alarm Enable */ 
    rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[12].pAttrPath));
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }
    
    rc = clCorAttrPathAppend(attrList.pAttrDescriptor[12].pAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to append attribute path. rc [0x%x]", rc);
        goto free_and_exit;
    }

    attrList.pAttrDescriptor[12].attrId = CL_ALARM_ENABLE; 
    attrList.pAttrDescriptor[12].index = -1;
    attrList.pAttrDescriptor[12].bufferPtr = &(pAlarmValues->alarmEnable);
    attrList.pAttrDescriptor[12].bufferSize = sizeof(ClUint8T);
    attrList.pAttrDescriptor[12].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
    if (attrList.pAttrDescriptor[12].pJobStatus == NULL)
    {
        clLogError("ASE", "ALR", "Failed to allocate memory for job status. rc [0x%x]",
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        goto free_and_exit;
    }

    rc = clCorBundleObjectGet(bundleHandle, &objH, &attrList);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed to add GET object to the bundle. rc [0x%x]", rc);
        goto free_and_exit;
    }

    rc = clCorBundleApply(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ASE", "ALR", "Failed the apply the bundle. rc [0x%x]", rc);
        goto free_and_exit;
    }

free_and_exit:
    for (i=0; i<12; i++)
    {
        clCorAttrPathFree(attrList.pAttrDescriptor[i].pAttrPath);

        if (attrList.pAttrDescriptor[i].pJobStatus)
            clHeapFree(attrList.pAttrDescriptor[i].pJobStatus);
    }

    clHeapFree(attrList.pAttrDescriptor);
    clCorBundleFinalize(bundleHandle);

    return rc;
}
