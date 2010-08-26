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
 * File        : clAlarmClientEngine.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module implements alarm engine functionality                      
 * 
 *
 *****************************************************************************/

/* standard includes */
#include <string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>

/* ASP includes */
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clCorApi.h>
#include <clOmApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>

/* alarm includes */
#include <clAlarmOMIpi.h>
#include <clAlarmUtil.h>
#include <clAlarmCommons.h>
#include <clAlarmOmClass.h>
#include <clAlarmErrors.h>
#include <clAlarmClientEngine.h>


#include <clIdlApi.h>
#include <clXdrApi.h>
#include <xdrClAlarmInfoIDLT.h>
#include <xdrClCorMOIdT.h>
#include <xdrClAlarmHandleInfoIDLT.h>

/*********************************************************************
 * Global 
 ********************************************************************/


ClRcT 
clAlarmClientEngineAlarmProcess(ClCorMOIdPtrT pMoId,
                                ClCorObjectHandleT hMSOObj, 
								ClAlarmInfoT* alarmInfo,
								ClAlarmHandleT *pAlarmHandle,
								ClUint32T lockApplied)
{
    ClRcT           rc = CL_OK;
    ClUint8T        alarmEnabled = 0;    
	ClUint8T        alarmContainmentValidEntry = 0;    
    ClUint32T       size = 0;
    ClUint32T       idx = 0;
    ClAlarmRuleEntryT   almEvt = {0}; 

	clLogTrace( "ALM",  "ALE", "Entering [%s]", __FUNCTION__);

    /* process it only if the alarm is either enabled or in a resume state */
    if ( alarmInfo != NULL )
    {
        almEvt.probableCause = alarmInfo->probCause; 
        almEvt.specificProblem = alarmInfo->specificProblem;
		clLogTrace( "ALM", "ALE", "Got the probable cause [%d]: Specific problem [%d]", 
                alarmInfo->probCause, alarmInfo->specificProblem);
    }
    else 
    {
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);
		clLogError( "ALM", "ALE", "Alarm Info structure passed is NULL. rc[0x%x]", rc);
        return rc; 
    }

    rc = alarmUtilAlmIdxGet(pMoId, hMSOObj, almEvt, &idx, lockApplied);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALE", "Failed to get the alarm index for the "
			"probable cause [%d]:Specific Problem [%d]. rc[0x%x]", 
            almEvt.probableCause, almEvt.specificProblem, rc);
        return rc;
    }

    size = sizeof(alarmEnabled);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ENABLE,
                       idx, &alarmEnabled, &size, lockApplied);
    if (CL_OK != rc)
    {
        clLogError("ALM", "ALE", "Failed to get the value of CL_ALARM_ENABLE. rc [0x%x]", rc);
        return rc;
    }
    
    if (!alarmEnabled)
    {
        clLogInfo("ALM", "ALE", "Alarm cannot be processed since it is disabled.");
        return CL_OK;
    }

    size = sizeof(alarmContainmentValidEntry);
    rc = GetAlmAttrValue(pMoId,hMSOObj, CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY,
                       idx, &alarmContainmentValidEntry, &size, lockApplied);
    if (CL_OK != rc)
    {
		clLogError( "ALM",  "ALE", "Failed to get the value of \
			CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY. rc[0x%x]", rc);
        return rc;
    }

    /* Store the alarm severity value into COR. */
    size = sizeof(ClUint8T);
    rc = SetAlmAttrValue(pMoId, NULL, hMSOObj, CL_ALARM_SEVERITY, 
            idx, &alarmInfo->severity, size);
    if (rc != CL_OK)
    {
		clLogError( "ALM",  "ALE", "Failed to set the value of \
			CL_ALARM_SEVERITY. rc [0x%x]", rc);
        return rc;
    }

    if ( alarmContainmentValidEntry == 0 )
    {
		clLogTrace( "ALM", "ALE",  "Alarm Cannot be processed since \
			its containment entry is invalid ");
        return CL_OK;
    }
    
    
	clLogDebug( "ALM", "ALE",  "Processing the alarm with probable cause [%d], "
            "Specific Problem [%d], severity [%d]  having alarm State [%d]", 
            almEvt.probableCause, almEvt.specificProblem, alarmInfo->severity, alarmInfo->alarmState);
				
    if (alarmInfo->alarmState == CL_ALARM_STATE_ASSERT)
        rc = clAlarmClientEngineAssertProcess(pMoId , hMSOObj, alarmInfo,pAlarmHandle,lockApplied);
    else if (alarmInfo->alarmState == CL_ALARM_STATE_CLEAR)
        rc = clAlarmClientEngineClearProcess(pMoId, hMSOObj, alarmInfo,pAlarmHandle,lockApplied);
    else
		clLogError( "ALM", "ALE"," Invalid alarm state supplied [%d]. ", alarmInfo->alarmState);

	clLogTrace( "ALM", "ALE", "Leaving [%s]", __FUNCTION__);
    return rc;
}


  /*       for assert
         * condition does not exist, no soaking, update curr, soaked, 
           process for affected
         * condition does not exist, has soaking, not under soakig, 
           start soaking
         * condition does not exist, has soaking, under soakig, update curr
         * condition exists, no soaking, ignore
         * condition exists,  has soaking, not under soaking, ignore
         * condition exists,  has soaking, under soaking, update curr
                                                                                            
          for clear:
        * condition does not exist, no soaking,  ignore
        * condition does not exist, has soaking,  ignore
        * condition exists, no soaking, process for affected alams
        * condition exists,  has soaking, not under soaking, start soaking, 
          update curr
        * condition exists,  has soaking, under soaking, update curr
  */
  
ClRcT 
clAlarmClientEngineAssertProcess(ClCorMOIdPtrT pMoId,
                              ClCorObjectHandleT hMSOObj, 
                              ClAlarmInfoT* alarmInfo,
                              ClAlarmHandleT* pAlarmHandle,
                              ClUint32T lockApplied)
{

    ClRcT           rc = CL_OK;
    ClUint32T       idx = 0;
    ClUint32T       size = sizeof(ClUint32T);
    ClUint32T       AssertSoakingTime = 0;
    ClUint32T       elapsedSoakingTime = 0;
    ClUint64T       currAlmBM = 0;
    ClUint64T       almsUnderSoakingBM = 0;
    ClUint64T       almsAfterSoakingBM = 0;
    ClAlarmRuleEntryT   almEvt ; 
    struct timeval  startTime = {0,0};

        
	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);
	
    if ( alarmInfo != NULL ) 
	{
        almEvt.probableCause = alarmInfo->probCause ; 
        almEvt.specificProblem = alarmInfo->specificProblem;
    }
    else 
	{
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);	
		clLogError( "ALM", "ALE", "Alarm Info structure is NULL. rc[0x%x] ", rc);
        return rc; 
    }

    /* Get the id for the probableCause. */
    rc = alarmUtilAlmIdxGet(pMoId, hMSOObj, almEvt, &idx, lockApplied);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "ALE", "Failed while getting the "
             "alarm index for the probable cause [%d]: Specific Problem [%d]. rc[0x%x]", 
             almEvt.probableCause, almEvt.specificProblem, rc);    
    }

    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, 
                         CL_ALARM_CURRENT_ALMS_BITMAP,
                         0, 
                         &currAlmBM, 
                         &size,
                         lockApplied);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", 
                "Failed to get the Current alarms bitmap. rc[0x%x] ", rc);
		return rc;
	}

    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, 
                         CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                         0, 
                         &almsAfterSoakingBM, 
                         &size,
                         lockApplied);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", " Failed to get the Alarms after soaking \
			bitmap. rc[0x%x] ", rc);
		return rc;
	}

    /* update the current alarms bit map*/
    currAlmBM |= ((ClUint64T) 1<<idx);
    size = sizeof(ClUint64T);
    rc = SetAlmAttrValue(pMoId,NULL,
                        hMSOObj, 
                         CL_ALARM_CURRENT_ALMS_BITMAP,
                         0, 
                         &currAlmBM, 
                         size);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", " Failed to set the value of current alarms \
			bitmap. rc[0x%x] ", rc);
		return rc;
	}

    if ( ! (almsAfterSoakingBM & ((ClUint64T) 1<<idx)) )
    { 
        /* condition does not exist */
        size = sizeof(ClUint32T);
        rc = clAlarmClientEngineProfAttrValueGet(pMoId, hMSOObj, 
                                                 CL_ALARM_ASSERT_SOAKING_TIME, 
                                                 idx,
                                                 &AssertSoakingTime, 
                                                 &size);
		if(CL_OK != rc)
		{
			clLogError( "ALM", "ALE", "Failed to get the configure value of assert \
				soaking time. rc[0x%x]", rc);
			return rc;
		}	

		clLogTrace( "ALM", "ALE", "The value of assert soaking time [%d] for alarm \
			Index [%d] ", AssertSoakingTime, idx);

        if (AssertSoakingTime)
        {
            size = sizeof(ClUint64T);
            rc = GetAlmAttrValue(pMoId, hMSOObj, 
                            CL_ALARM_ALMS_UNDER_SOAKING,
                            0, 
                            &almsUnderSoakingBM, 
                            &size,
                            lockApplied);
			if(CL_OK != rc)
			{
				clLogError( "ALM", "ALE", "Failed to get the alarms under soaking bitmap.\
				    rc[0x%x]", rc);
				return rc;
			}	

			clLogTrace( "ALM", "ALE", 
                    "The value of alarms under soaking bitmap [0x%llx]", 
                    almsUnderSoakingBM);

            if ( ! (almsUnderSoakingBM & ((ClUint64T) 1<<idx)) )
            { 
                /* not under soaking */
                /* start soaking */
                /* store the user data and after the lapse of the soaking time
                 * fill the stored data as part of payload and publish the
                 * event ravi  */ 

				clLogTrace( "ALM", "ALE", "Alarm [%d] is not under soaking so starting \
					the soaking process", idx);

                if(alarmInfo->len)
                {
                    /* ALways first store the length and then the data */ 
                    rc = SetAlmAttrValue (pMoId, NULL,hMSOObj , CL_ALARM_EVENT_PAY_LOAD_LEN ,
                    idx , (void *)&(alarmInfo->len),sizeof(ClUint32T));
					if(CL_OK != rc)
					{
                        clLogError( "ALM", "ALE", "Failed while setting the \
                                value of buffer length [%d] for alarm index[%d]. rc[0x%x]",
                                alarmInfo->len, idx, rc);
                        return rc;
					}

                    /*write (1, alarmInfo->buff, alarmInfo->len);*/
                    rc = SetAlmAttrValue (pMoId, NULL,hMSOObj , CL_ALARM_EVENT_PAY_LOAD,
                    idx , (void *)alarmInfo->buff ,alarmInfo->len );
					if(CL_OK != rc)
					{
                        clLogError( "ALM", "ALE", "Failed while setting the \
                                buffer for the alarm index [%d]. rc[0x%x]", idx, rc);
                        return rc;
					}
                }
				else 
				{
					clLogTrace( "ALM", "ALE", "Alarm [%d] is not having the payload to store", idx);
				}
                
				clLogTrace( "ALM", "ALE", "The alarm id [%d] has soaking enabled so start soaking",idx );
               
                elapsedSoakingTime =1;        
                
                /* put this value back to the COR */
                 size = sizeof(ClUint32T);
                rc = SetAlmAttrValue(pMoId,NULL,
                                hMSOObj, 
                                CL_ALARM_ELAPSED_ASSERT_SOAK_TIME, 
                                idx, 
                                &elapsedSoakingTime, 
                                size);
				if(CL_OK != rc)
				{
					clLogError( "ALM", "ALE", "Failed while setting the value \
						of elapsed assert soaking time for alarm Index [%d]. rc[0x%x]", idx, rc);
					return rc;
				}
    
                gettimeofday(&startTime, NULL);

                rc = SetAlmAttrValue(pMoId, NULL,
                                hMSOObj, 
                                CL_ALARM_SOAKING_START_TIME, 
                                idx, 
                                &startTime, 
                                size);                                

				if(CL_OK != rc)
				{
					clLogError( "ALM", "ALE", "Failed while setting the value \
						of soaking start time for alarm index [%d]. rc[0x%x]", idx, rc);
					return rc;
				}
 
                /* mark that this alarm is under soaking */
                almsUnderSoakingBM |= ((ClUint64T) 1<<idx);
                rc = SetAlmAttrValue(pMoId, NULL,
                                hMSOObj, 
                                CL_ALARM_ALMS_UNDER_SOAKING,
                                0, 
                                &almsUnderSoakingBM, 
                                sizeof(ClUint64T));                            
				if(CL_OK != rc)
				{
					clLogError( "ALM", "ALE", "Failed while setting the value \
						of alarms under soaking bitmap for alarm index[%d]. rc[0x%x]", idx, rc);
					return rc;
				}

				clLogTrace( "ALM", "ALE", "For alarm id [%d] the Elapsed Soaking Time is [%d] ",
                        idx, elapsedSoakingTime);
            }
            else
                clLogInfo("ALM", "ALE", "Alarm already present in under soaking bitmap.");
        }
        else
        { 
            /* no soaking time*/
            /* process for affected alarms and update soaked alms bitmap */

			clLogTrace( "ALM", "ALE", "The alarm id [%d] does not have soaking enabled ", idx );

            almsAfterSoakingBM |= ((ClUint64T) 1<<idx);
            size = sizeof(ClUint64T);
            rc = SetAlmAttrValue(pMoId, NULL,
                            hMSOObj, 
                            CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                            0, 
                            &almsAfterSoakingBM, 
                            size);
			if(CL_OK != rc)
			{
				clLogError( "ALM", "ALE", "Failed to set the value of alarm \
					after soaking bitmap for alarm index [%d]. rc[0x%x]", idx, rc);
				return rc;
			}	

            /* store the user data and after the lapse of the soaking time
             * fill the stored data as part of payload and publish the
             * event ravi  */ 

            if(alarmInfo->len)
            {
				clLogTrace( "ALM", "ALE", "Storing the payload and its length \
					internally for the alarm index [%d]", idx);

                /* ALways first store the length and then the data */ 
                rc = SetAlmAttrValue (pMoId, NULL,
                                hMSOObj , 
                                CL_ALARM_EVENT_PAY_LOAD_LEN ,
                                idx , 
                                (void *)&(alarmInfo->len),
                                sizeof(ClUint32T));
				if(CL_OK != rc)
				{
					clLogError( "ALM", "ALE", "Failed to set the value of \
						payload length for alarm index [%d]. rc[0x%x]", idx, rc);
					return rc;
				}	

                /*write (1, alarmInfo->buff, alarmInfo->len);*/
                rc = SetAlmAttrValue (pMoId, NULL,
                                hMSOObj , 
                                CL_ALARM_EVENT_PAY_LOAD,
                                idx , 
                                (void *)alarmInfo->buff ,
                                alarmInfo->len );
				if(CL_OK != rc)
				{
					clLogError( "ALM", "ALE", "Failed to set the value of \
						payload for alarm index [%d]. rc[0x%x]", idx, rc);
					return rc;
				}	
            }
			else
			{
				clLogTrace( "ALM", "ALE", "Payload length is zero for  \
					the alarm index [%d]", idx);
			}

            clAlarmClientEngineAffectedAlarmsProcess(pMoId, hMSOObj, idx, 1 /*assert*/,0/*no lock*/,pAlarmHandle);
                                                                                         
			clLogTrace( "ALM", "ALE", "For Alarm Index [%d] the Alarm After Soaking Bitmap is [0x%llx]", 
                                           idx, almsAfterSoakingBM);

        }

    }
    else
        clLogInfo("ALM", "ALE", "Alarm already raised, found in after soaking bitmap. ");
	
	clLogTrace( "ALM", "ALE", "Leaving [%s]", __FUNCTION__);

    return rc;
}


/* modified alarmProcess for assert end */ 



ClRcT 
clAlarmClientEngineClearProcess(ClCorMOIdPtrT pMoId,
                            ClCorObjectHandleT hMSOObj, 
                            ClAlarmInfoT* alarmInfo,
                            ClAlarmHandleT* pAlarmHandle,
							ClUint32T lockApplied )
{

    ClRcT           rc = CL_OK;
    ClUint32T       idx = 0;
    ClUint32T       size = sizeof(ClUint32T);
    ClUint32T       clearSoakingTime = 0;
    ClUint32T       elapsedSoakingTime = 0;
    ClUint64T       currAlmBM = 0;
    ClUint64T       almsUnderSoakingBM = 0;
    ClUint64T       almsAfterSoakingBM = 0;
    ClAlarmRuleEntryT   almEvt  = {0};
    struct timeval  startTime = {0,0};        

	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);

    if ( alarmInfo != NULL ) 
	{
        almEvt.probableCause = alarmInfo->probCause; 
        almEvt.specificProblem = alarmInfo->specificProblem; 
		clLogTrace( "ALM", "ALE", "Clearing the alarm for the probable cause [%d]"
                ": specific Problme [%d]", almEvt.probableCause, almEvt.specificProblem);
    }
    else 
	{
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);
		clLogError( "ALM", "ALE", "Alarm info structure passed is null. \
		   	rc [0x%x] ", rc);
        return rc; 
    }
        
	/* Get the id for the probableCause. */
    rc = alarmUtilAlmIdxGet(pMoId, hMSOObj, almEvt, &idx, lockApplied);
    if (CL_OK != rc)
    {
        clLogError("ALM", "ALE", 
                "Failed to get the index corresponding to the probable cause [%d] "
                ": specific Problem [%d]. rc[0x%x]",
                almEvt.probableCause, almEvt.specificProblem, rc);
        return rc;
    }

    clLogDebug("ALM", "ALE", 
            "Got the index [%d] for Probable cause [%d]: specific problem [%d]",
            idx, almEvt.probableCause, almEvt.specificProblem);

    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId,hMSOObj, 
                    CL_ALARM_CURRENT_ALMS_BITMAP,
                    0, 
                    &currAlmBM, 
                    &size,
                    lockApplied);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", 
                "Failed to get the current alarms bitmap. rc [0x%x]", rc);
		return rc;
	}

	size = sizeof(ClUint64T);
	rc = GetAlmAttrValue(pMoId, hMSOObj, 
                   CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                   0, 
                   &almsAfterSoakingBM, 
                   &size,
                   lockApplied);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", "Failed to get the alarms after soaking bitmap \
			rc [0x%x] ", rc);
		return rc;
	}

    /* update the current alarms bit map - clear*/
    currAlmBM &= ~((ClUint64T) 1<<idx);
    size = sizeof(ClUint64T);
    rc = SetAlmAttrValue(pMoId, NULL,
                    hMSOObj, 
                    CL_ALARM_CURRENT_ALMS_BITMAP,
                    0, 
                    &currAlmBM, 
                    size);
	if(CL_OK != rc)
	{
		clLogError( "ALM", "ALE", "Failed to get the current alarms bitmap \
			rc [0x%x] ", rc);
		return rc;
	}

    if ( almsAfterSoakingBM & ((ClUint64T) 1<<idx) )
    { 
        /* condition exists */
		clLogTrace( "ALM", "ALE", " Alarm [%d] updated in the alarm after soaking \
			bitmap ", idx);
        size = sizeof(ClUint32T);
        rc = clAlarmClientEngineProfAttrValueGet(pMoId, hMSOObj, 
                                            CL_ALARM_CLEAR_SOAKING_TIME, 
                                            idx,
                                            &clearSoakingTime, 
                                            &size);
		if(CL_OK != rc)
		{
			clLogError( "ALM", "ALE", " Failed while getting the value of clear \
				soaking time rc [0x%x] ", rc);
			return rc;
		}


        if (clearSoakingTime)
        {
			clLogTrace( "ALM", "ALE", " The clear soaking time for the \
				alarm index [%d] is [%d] ", idx, clearSoakingTime);	

            /* under saoking ??? */
            size = sizeof(ClUint64T);
            rc = GetAlmAttrValue(pMoId, hMSOObj, 
                            CL_ALARM_ALMS_UNDER_SOAKING, 
                            0,
                            &almsUnderSoakingBM, 
                            &size,
                            lockApplied);
			if(CL_OK != rc)
			{
				clLogError( "ALM", "ALE", " Failed while get alarms under soaking \
					bitmap while clearing the alarm index [%d]. rc[0x%x]", idx, rc);
				return rc;
			}
            
            if ( ! (almsUnderSoakingBM & ((ClUint64T) 1<<idx)) )
            {
				clLogTrace( "ALM", "ALE", " Alarm [%d] is not under soaking , \
                                so go for soaking ", idx);
                /* not under soaking ???*/
                /* start clear soaking */
                /* store the user data and after the lapse of the soaking time
                 * fill the stored data as part of payload and publish the
                 * event ravi  */ 

                if(alarmInfo->len > 0)
                {
				    clLogTrace( "ALM", "ALE", " The buffer for the alarm [%d] need to \
                            stored as the length is [%d]", idx, alarmInfo->len);

                    /* ALways first store the length and then the data */ 
                    rc = SetAlmAttrValue (pMoId, NULL,
                                    hMSOObj , 
                                    CL_ALARM_EVENT_PAY_LOAD_LEN ,
                                    idx , 
                                    (void *)&(alarmInfo->len),
                                    sizeof(ClUint32T));
                    if(CL_OK != rc)
                    {
				        clLogError( "ALM", "ALE", " Failed while setting the payload length for \
                             alarm index [%d]. rc[0x%x]", idx, rc);
                        return rc;
                    }

                    rc = SetAlmAttrValue (pMoId, NULL,
                                    hMSOObj , 
                                    CL_ALARM_EVENT_PAY_LOAD,
                                    idx , 
                                    (void *)alarmInfo->buff ,
                                    alarmInfo->len );
                    if(CL_OK != rc)
                    {
				        clLogError( "ALM", "ALE", " Failed while setting the payload buffer for \
                             alarm index [%d]. rc[0x%x]", idx, rc);
                        return rc;
                    }
                }
                else
                {
                    clLogTrace( "ALM", "ALE", "Buffer length is zero for \
                            the alarm index [%d]", idx);
                }
                
                clLogTrace( "ALM", "ALE", "For the Alarm Index [%d] clear soaking time is defined.", idx);
                                                                                             
                elapsedSoakingTime =1;
                                                                                             
                /* put this value back to the COR */
                size = sizeof(ClUint32T);
                rc = SetAlmAttrValue(pMoId, NULL,
                                hMSOObj, 
                               CL_ALARM_ELAPSED_CLEAR_SOAK_TIME, 
                               idx, 
                               &elapsedSoakingTime, 
                               size);
                if(CL_OK != rc)
                {
				    clLogError( "ALM", "ALE", "Failed while setting the \
                            elapsed clear soaking time for alarm index[%d]. rc[0x%x]", idx, rc);
                    return rc;
                }

                gettimeofday(&startTime, NULL);

                rc = SetAlmAttrValue(pMoId, NULL,
                                hMSOObj, 
                                CL_ALARM_SOAKING_START_TIME,
                                idx, 
                                &startTime, 
                                size);                                    
                if(CL_OK != rc)
                {
				    clLogError( "ALM", "ALE", "Failed while setting the \
                                soaking start time time for alarm index[%d]. rc[0x%x]", idx, rc);
                    return rc;
                }

                almsUnderSoakingBM |= ((ClUint64T) 1<<idx);
                rc = SetAlmAttrValue(pMoId, NULL,
                                hMSOObj, 
                                CL_ALARM_ALMS_UNDER_SOAKING,
                                0, 
                                &almsUnderSoakingBM, 
                                sizeof(ClUint64T));            
                if(CL_OK != rc)
                {
				    clLogError( "ALM", "ALE", "Failed while setting the \
                            alarms under soaking time for alarm index[%d]. rc[0x%x]", idx, rc);
                    return rc;
                }
                                                            
                                                                                             
                clLogTrace("ALM", "ALE", " For alarm index [%d] \
                        the Elapsed Soaking time is [%d]", idx, elapsedSoakingTime);
            }
        }
        else
        {
            /* alarm has cleared. process for affected alarms */
            clLogTrace( "ALM", "ALE", " For alarm index [%d] no clear \
                       soaking time defined, so start processing for affected alarms..\n", idx );

                                                                                             
            almsAfterSoakingBM &= ~((ClUint64T) 1<<idx);
            size = sizeof(ClUint64T);
            rc = SetAlmAttrValue(pMoId, NULL,
                            hMSOObj, 
                            CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                            0, 
                            &almsAfterSoakingBM, 
                            size);
            if(CL_OK != rc)
            {
			    clLogError( "ALM", "ALE", "Failed while setting the alarms after soaking \
                            bitmap [0x%llx] with alarm index[%d]. rc[0x%x]", almsAfterSoakingBM, idx, rc);
               
                return rc;
            }
                                                                                             
            clLogTrace( "ALM", "ALE", " For alarm index [%d] after soaking \
                    bitmap is [%llx] \n", idx, almsAfterSoakingBM);

            /* store the user data and after the lapse of the soaking time
             * fill the stored data as part of payload and publish the
             * event ravi  */ 

            if(alarmInfo->len > 0)
            {
                clLogTrace( "ALM", "ALE", "The buffer length is [%d] for alarm \
                        Index [%d]", alarmInfo->len, idx);

                /* ALways first store the length and then the data */ 
                rc = SetAlmAttrValue (pMoId, NULL,
                                hMSOObj , 
                                CL_ALARM_EVENT_PAY_LOAD_LEN ,
                                idx , 
                                (void *)&(alarmInfo->len),
                                sizeof(ClUint32T));
                if(CL_OK != rc)
                {
                    clLog (CL_LOG_SEV_ERROR , "ALM", "ALE", "Failed while setting the payload length \
                            for alarm index [%d]. rc[0x%x]", idx, rc);
                    return rc;
                }

                /*write (1, alarmInfo->buff, alarmInfo->len);*/
                rc = SetAlmAttrValue (pMoId, NULL,
                                hMSOObj , 
                                CL_ALARM_EVENT_PAY_LOAD,
                                idx , 
                                (void *)alarmInfo->buff ,
                                alarmInfo->len );
                if(CL_OK != rc)
                {
                    clLog (CL_LOG_SEV_ERROR , "ALM", "ALE", "Failed while setting the payload buffer \
                            for alarm index [%d]. rc[0x%x]", idx, rc);
                    return rc;
                }
            }
            else
            {
                clLogTrace( "ALM", "ALE", "The buffer length is zero for alarm index [%d]", idx);
            }

            clAlarmClientEngineAffectedAlarmsProcess(pMoId, hMSOObj, idx, 0 /*clear*/,0/*no lock*/,pAlarmHandle);
        }
    }

	clLogTrace( "ALM",  "ALE", " Leaving [%s]", __FUNCTION__);
    return rc;

}


/* modified alarmProcess for clear end */



void 
clAlarmClientEngineAffectedAlarmsProcess(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, ClUint32T
        idx, ClUint8T isAssert, ClUint32T lockApplied, ClAlarmHandleT* pAlarmHandle)
{
    ClRcT       rc = CL_OK;
    ClUint64T   affectedAlmBM = 0;
    ClUint32T   size = 0;
    ClUint32T   i = 0;
    ClUint8T    conditionMatches;
    ClUint8T    almToBGenerated = 0;;

	clLogTrace( "ALM",  "ALE", " Entering [%s]", __FUNCTION__);
    /* find the bit map for the affected alarms */    

    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_AFFECTED_ALARMS, idx, &affectedAlmBM, &size, lockApplied);

    if(CL_OK != rc)
    {
        clLogError( "ALM", "ALE", "Failed while getting the affected alarms \
                bitmap for alarm index [%d]. rc[0x%x]", idx, rc);
        return ;
    }

    clLogTrace( "ALM", "ALE", "For the alarm index [%d] affected alarm \
            bitmap [0x%llx]. Alarm Status [%d]",idx, affectedAlmBM, isAssert);

    /* for each alarm check the alarm condition */
    for (; ( (i < CL_ALARM_MAX_ALARMS) && affectedAlmBM) ; i++, affectedAlmBM >>=1 )
    {
        if (affectedAlmBM & 1)
        {

            /* check the condition for alm in index i */
            conditionMatches = clAlarmClientEngineAlarmConditionCheck(pMoId, hMSOObj, i, lockApplied);

            clLogTrace( "ALM", "ALE", " Alarm index [%d] affected alm Id: [%d], \
                    Alarm State [%d], condition check result [%d] \n", idx, i, isAssert, conditionMatches);
            almToBGenerated = 0;

            if ( (isAssert == 0) || (isAssert && conditionMatches) )
            {
                almToBGenerated = 1;            
                clLogTrace( "ALM", "ALE", " Condition matches for alarm index [%d]. Alarm state [%d]",
                         idx, isAssert);
            }

            if (almToBGenerated)
            {
                clLogTrace( "ALM", "ALE", " Generating the alarm for \
                        the alarm index [%d]",i);

                rc = clAlarmClientEnginePrepare2Generate(pMoId, hMSOObj, i, conditionMatches,lockApplied,pAlarmHandle);

                if (rc  != CL_OK)
                {
                    clLogError( "ALM","ALE", "Failed while generating the \
                            alarm for index [%d]. rc[0x%x]", i, rc);
                }
            }

        }
    }
	
	clLogTrace( "ALM",  "ALE", " Leaving [%s]", __FUNCTION__);
}


ClUint8T
clAlarmClientEngineAlarmConditionCheck(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, ClUint32T idx, ClUint32T lockApplied)
{
    ClUint64T    almCondition = 0;
    ClUint64T    almsAfterSoakingBM = 0;
    ClUint32T    size=0;
    ClAlarmRuleRelationT    relation;
    ClUint64T    almResult = 0;
    ClUint8T    conditionMatches = 0;
    ClRcT       rc = CL_OK;

	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);

    /* Get the condition Bitmap for the alarm */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_GENERATION_RULE, idx, &almCondition, &size, lockApplied);
    if(CL_OK != rc)
    {
        clLogError("ALM", "ALE", 
                "Failed while getting the generation rule for the alarm index[%d]. rc[0x%x]", idx, rc);
        return rc;
    }

    /* get the alms that have persisted soaking */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj,CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,0,&almsAfterSoakingBM,&size,lockApplied);
    if(CL_OK != rc)
    {
        clLogError("ALM", "ALE", "Failed while getting the alarm after soaking bitmap. rc[0x%x]", rc);
        return rc;
    }

    /* get the alm rule relation */
    size = sizeof(ClAlarmRuleRelationT);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_GENERATE_RULE_RELATION, idx, &relation, &size, lockApplied);
    if(CL_OK != rc)
    {
        clLogError("ALM", "ALE", "Failed while getting the Generation rule relation for alarm index [%d]. rc[0x%x]",
               idx, rc);
        return rc;
    }

    clLogTrace("ALM", "ALE", "Checking the generation rule validity: Alarm Index [0x%x], Generation Rule [0x%llx] , \
                                       Alarm After Soaking BitMap:[0x%llx], Relation Type [%d] ", idx, almCondition,
                                       almsAfterSoakingBM, relation);

    almResult = almCondition & almsAfterSoakingBM;

    clLogTrace("ALM", "ALE", "For the Alarm Index [0x%x] the alarm result is [0x%llx]", 
            idx, almResult);

    if ( (relation == CL_ALARM_RULE_LOGICAL_AND) && (almResult == almCondition) )
        conditionMatches = 1;
    else if ( ((relation == CL_ALARM_RULE_LOGICAL_OR) || (relation == CL_ALARM_RULE_NO_RELATION))   && almResult)
		conditionMatches = 1;

    clLogTrace("ALM", "ALE", "For the Alarm index [0x%x] the result of generation rule validation is [%d]", 
                                           idx, conditionMatches);

	clLogTrace( "ALM", "ALE", " Leaving [%s]", __FUNCTION__);
    return conditionMatches;
}



ClRcT 
clAlarmClientEnginePrepare2Generate(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, 
                      ClUint32T idx, ClUint8T isAssert,ClUint32T lockApplied, 
                      ClAlarmHandleT* pAlarmHandle)
{
    ClUint32T size=0;
    ClRcT     rc = CL_OK;
    ClUint32T loadlen  =0 ;
    ClUint32T       classIdx=0;    
    ClCorClassTypeT   myMOClassType;
    ClAlarmInfoT* pAlarmInfo = NULL;
    CL_OM_ALARM_CLASS*  omhandle = NULL ;     
    ClCpmHandleT cpmHandle = 0;
	ClCorMOIdPtrT hMOId;
	ClCorServiceIdT srvcId;
    ClNameT alarmClientCompName;

	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);

    if ((rc= clCorMoIdAlloc(&hMOId))!= CL_OK )
    {
        clLogError( "ALM", "ALE", "Failed to allocate the MoId. rc[0x%x] ",rc);
        return rc;
    }
	
	rc = clCorObjectHandleToMoIdGet(hMSOObj,hMOId,&srvcId);
	if(rc != CL_OK)
    {
        clLogError("ALM", "ALE", "Failed while getting the object handle. rc[0x%x]",rc);
		clCorMoIdFree(hMOId);
        return rc;
    }

    /* Get the additional information stored in the OM object */ 
    size = sizeof(ClUint32T);
    rc = GetAlmAttrValue(pMoId, hMSOObj,CL_ALARM_EVENT_PAY_LOAD_LEN ,idx,
                    (void *)& loadlen , &size, lockApplied); 
	if(rc != CL_OK)
    {
        clLogError("ALM", "ALE", "Failed while getting the payload length. rc[0x%x]",rc);
		clCorMoIdFree(hMOId);
        return rc;
    }


    if((pAlarmInfo = (ClAlarmInfoT*)clHeapAllocate(sizeof(ClAlarmInfoT)+loadlen  ))== NULL)
    {
        clLogError("ALM", "ALE", "Failed while allocating the memory. rc[0x%x]",rc);
		clCorMoIdFree(hMOId);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }
    memset(pAlarmInfo,0,sizeof(ClAlarmInfoT)+loadlen);
    

    clLogTrace( "ALM", "ALE", "Processing the alarm for alarm index[%d] ", idx );

    pAlarmInfo->eventTime = time(NULL);
    rc = clCorMoIdToClassGet(hMOId, CL_COR_MO_CLASS_GET,  &myMOClassType);
    if (CL_OK != rc)
    {
		clHeapFree(pAlarmInfo);
		clCorMoIdFree(hMOId);
        clLogError( "ALM", "ALE", "Getting the class Id from MOId  failed. rc [0x%x]", rc);
        return rc;
    }    
    
    /* get the index */
    while( (appAlarms[classIdx].moClassID != myMOClassType) && (appAlarms[classIdx].moClassID != 0) ) 
    { 
        classIdx++; 
    }    

    clLogTrace( "ALM", "ALE", "Got the classId [%d], classIdx [%d] for alarm index[%d] ", myMOClassType, classIdx, idx );

    pAlarmInfo->category = appAlarms[classIdx].MoAlarms[idx].category;
    pAlarmInfo->probCause = appAlarms[classIdx].MoAlarms[idx].probCause;
    pAlarmInfo->specificProblem = appAlarms[classIdx].MoAlarms[idx].specificProblem;
    pAlarmInfo->alarmState = isAssert;    

    /* Read the alarm severity value from COR. */
    size = sizeof(ClUint8T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_SEVERITY, idx,
                    (void *)&pAlarmInfo->severity, &size, lockApplied); 
	if(rc != CL_OK)
    {
        clLogError("ALM", "ALE", "Failed while getting the payload length. rc[0x%x]",rc);
        clHeapFree(pAlarmInfo);
		clCorMoIdFree(hMOId);
        return rc;
    }

    pAlarmInfo->len = loadlen;
	pAlarmInfo->moId=*hMOId;

    /* copy the component's name */
    memset (&alarmClientCompName,'\0',sizeof (alarmClientCompName));    
    rc = clCpmComponentNameGet(cpmHandle, &alarmClientCompName);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "ALE", "Failed while getting the component name. rc[0x%x]",rc);
		clHeapFree(pAlarmInfo);
		clCorMoIdFree(hMOId);
        CL_FUNC_EXIT();
        return rc;
    }
    memcpy(&(pAlarmInfo->compName), &alarmClientCompName, sizeof(alarmClientCompName));    

    if (loadlen)
    {
        clLogTrace( "ALM", "ALE", "Getting the payload buffer for \
                alarm index[%d], payload length is [%d] ", idx , loadlen);
        rc = GetAlmAttrValue(pMoId, hMSOObj , 
                       CL_ALARM_EVENT_PAY_LOAD,
                       idx, 
                       (void *)pAlarmInfo->buff, 
                       &loadlen,
					   lockApplied );
        if(CL_OK != rc)
        {
            clLogError( "ALM", "ALE", "Failed while getting the \
                    payload buffer. rc[0x%x]", rc);
            clHeapFree(pAlarmInfo);
            clCorMoIdFree(hMOId);
            CL_FUNC_EXIT();
            return rc;
        }
    }
  
    rc = clAlarmClientEngineAlarmAdd(pAlarmInfo,pAlarmHandle);
    if (CL_OK != rc)
    {
        clLogError("ALM", "ALE", "Failed while sending the request to the server for alarm processing. rc[0x%x] ", rc);
        clHeapFree(pAlarmInfo);
        clCorMoIdFree(hMOId);
        return rc;
    }

    
    if ( (rc = clOmMoIdToOmObjectReferenceGet(&(pAlarmInfo->moId), (void **) &omhandle ))!= CL_OK )
    {
        clLogError( "ALM", "ALE", " Failed while getting the MoId to \
                OM object reference. rc [0x%x] ",rc );
		clHeapFree(pAlarmInfo);
		clCorMoIdFree(hMOId);
        return rc;
    }

    if ((CL_OM_ALARM_CLASS*)omhandle)
    {
        if ( (((CL_OM_ALARM_CLASS*)omhandle)->alarmTransInfo[idx].payloadLen != 0) &&
				( ((CL_OM_ALARM_CLASS*)omhandle)->alarmTransInfo[idx].payload  !=NULL ) )  
        {
            clHeapFree( ((CL_OM_ALARM_CLASS*)omhandle)->alarmTransInfo[idx].payload );
            ((CL_OM_ALARM_CLASS*)omhandle)->alarmTransInfo[idx].payloadLen = 0;
        }
    
    }
    else
    {
        clLogInfo( "ALM", "ALE", "Either the omhandle or the loadlen is zero.\
                omHandle[%p], payload length [%d]", (ClPtrT)omhandle, loadlen);
    }
    clHeapFree(pAlarmInfo);
	clCorMoIdFree(hMOId);

	clLogTrace( "ALM",  "ALE", " Leaving [%s]", __FUNCTION__);

    return rc;

}



ClRcT
clAlarmClientEngineAlarmAdd(ClAlarmInfoT* pAlarmInfo,ClAlarmHandleT* pAlarmHandle)
{

    ClBufferHandleT inputMsg;    
    ClBufferHandleT outputMsg;    
    ClRmdOptionsT rmdOptions; 
    ClIocAddressT iocAddress;
    ClRcT        rc = CL_OK;
    ClUint32T length=0;
    ClAlarmVersionInfoT *alarmVersionInfo;

	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);
	
    alarmVersionInfo=clHeapAllocate(sizeof(ClAlarmVersionInfoT)+pAlarmInfo->len);
	if(alarmVersionInfo == NULL)
	{
        clLogError("ALM","ALE","Failed while allocating memory for alarm informaiton and buffer.");
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
	}
    
    memcpy((void *)&(alarmVersionInfo->alarmInfo),(void *)pAlarmInfo,sizeof(ClAlarmInfoT));
    memcpy((void *)(alarmVersionInfo->alarmInfo).buff,pAlarmInfo->buff,pAlarmInfo->len);
    CL_ALARM_VERSION_SET(alarmVersionInfo->version);
    
    if((rc = clBufferCreate( &inputMsg ))!= CL_OK )
    {
        clLogError( "ALM", "ALE", "Input Buffer handle creation failed.  rc [0x%x]",rc);
		clHeapFree(alarmVersionInfo);
        return rc;
    }

    if((rc = clBufferCreate( &outputMsg ))!= CL_OK )
    {
        clLogError("ALM", "ALE", "Output Buffer creation failed. rc[0x%x]",rc);
		clHeapFree(alarmVersionInfo);
        return rc;
    }

   rc = clBufferNBytesWrite( inputMsg , (ClUint8T *)alarmVersionInfo,
                               sizeof(ClAlarmVersionInfoT)+pAlarmInfo->len);
   if(rc != CL_OK)
   {
        clBufferDelete(&inputMsg);
        clBufferDelete(&outputMsg);
		clHeapFree(alarmVersionInfo);
        clLogError("ALM", "ALE", "Failed while writing to the buffer. rc[0x%x]",rc);
        return rc;
   }              

    iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    iocAddress.iocPhyAddress.portId      = CL_IOC_ALARM_PORT;

    clLogInfo( "ALM", "ALE", "Sending the RMD corresponding to alarm category [%d], severity [%d], probCause [%d], specProb [%d] to the server on the [0x%x:0x%x]",
            pAlarmInfo->category, pAlarmInfo->severity, pAlarmInfo->probCause, pAlarmInfo->specificProblem, 
            iocAddress.iocPhyAddress.nodeAddress, iocAddress.iocPhyAddress.portId);

    memset((void*)&rmdOptions, 0 ,sizeof(rmdOptions));
    rmdOptions.timeout = CL_RMD_DEFAULT_TIMEOUT;
    rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;
    rmdOptions.retries = CL_RMD_DEFAULT_RETRIES;

    if((rc = clRmdWithMsg( iocAddress, 
                           CL_ALARM_SERVER_API_HANDLER,
                           inputMsg,
                           outputMsg,
                           CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_NEED_REPLY,
                           &rmdOptions, NULL))!= CL_OK )
    {
        clLogError("ALM", "ALM", "RMD call to the Alarm Server failed. rc [0x%x]",rc);
        clBufferDelete ( &inputMsg );
        clBufferDelete ( &outputMsg );
        clHeapFree(alarmVersionInfo);
        return rc;        
    }
    
    clLogTrace( "ALM", "ALE", "Successfully got the response from the alarm Server.");

    /*Get the length of outputMsg and check whether it is a handle or
     * not.If outputMsg is proper copy the alarm handle from outputMsg to
     * pAlarmInfo(out parameter) */
   
    rc = clBufferLengthGet(outputMsg, &length);
	if(rc != CL_OK)
    {
        clBufferDelete(&inputMsg);
        clBufferDelete(&outputMsg);
		clHeapFree(alarmVersionInfo);
        clLogError( "ALM", "ALE", "Failed to get the buffer length. rc[0x%x]", rc);
        return rc;
    }              

    if(length != sizeof(ClAlarmHandleT))
    {
        clBufferDelete(&inputMsg);
        clBufferDelete(&outputMsg);
		clHeapFree(alarmVersionInfo);
        clLogError( "ALM", "ALE", "Not allocating handle at this point because "
                "alarm might have been already raised or "
                "alarm might have been suppressed because of the parent alarm or "
                "the change of state of the last alarm does not raise the dependent alarm");
        return rc;
    } 

    if(NULL != pAlarmHandle)
    {
		rc = clBufferNBytesRead(outputMsg, (ClUint8T *)pAlarmHandle, &length);
        if(rc != CL_OK)
        {
			clBufferDelete(&inputMsg);
            clBufferDelete(&outputMsg);
			clHeapFree(alarmVersionInfo);
            clLogError( "ALM", "ALE", "Failed while reading the alarm information \
                    sent by Alarm Server. rc[0x%x]",rc);
            return rc;
        }
	   
        clLogInfo( "ALM",  "ALE", " The alarm handle obtained from server is [%d] ", 
                *pAlarmHandle);
    }   
    

    clBufferDelete ( &inputMsg );
    clBufferDelete ( &outputMsg );
	clHeapFree(alarmVersionInfo);
	
	clLogTrace( "ALM",  "ALE", " Leaving [%s]", __FUNCTION__);

    return rc;        
}


ClUint32T clAlarmClientEngineTimeDiffCalc(struct timeval * tm1, struct timeval *tm2)
{
    ClUint32T msec;

	clLogTrace( "ALM", "ALE", " Entering [%s]", __FUNCTION__);

    msec = (tm2->tv_sec - tm1->tv_sec)*1000;

    if (tm2->tv_usec > tm1->tv_usec)
        msec += (tm2->tv_usec - tm1->tv_usec)/1000;
    else
        msec -= (tm1->tv_usec - tm2->tv_usec)/1000;

	clLogTrace( "ALM", "ALE", " The time difference is [%d]", msec);

	clLogTrace( "ALM", "ALE", " Leaving [%s]", __FUNCTION__);

    return (msec);
}

ClRcT 
clAlarmClientEngineProfAttrValueGet(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, ClUint32T attrId, 
                                 ClUint32T idx, void* value, ClUint32T* size)
{
    ClRcT        rc = CL_OK;
    ClUint32T       classIdx=0;    
    ClCorClassTypeT   myMOClassType;
    
#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM",  "ALE", " Entering [%s]", __FUNCTION__);
#endif

    /* check if the attribute is with alarm OM object */

    rc = clCorMoIdToClassGet(pMoId, CL_COR_MO_CLASS_GET,  &myMOClassType);

    if (CL_OK != rc)
    {
        clLogError("ALM", "ALE", "Failed while getting the class for the Alarm MSO. rc[0x%x]", rc);
        return rc;
    }
    
    /* get the index */

    while( (appAlarms[classIdx].moClassID != myMOClassType) && (appAlarms[classIdx].moClassID != 0) ) 
        { 
            classIdx++; 
        }    

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM",  "ALE", " Got the request for attribute [0x%x] for class Id [0x%x]",
            attrId, myMOClassType);
#endif
    /* get the alarm index*/
    
    switch(attrId)
    {
        case CL_ALARM_CATEGORY:
                 memcpy( value, &(appAlarms[classIdx].MoAlarms[idx].category), *size);    
                 break;
        case CL_ALARM_FETCH_MODE:
                 memcpy( value, &(appAlarms[classIdx].MoAlarms[idx].fetchMode), *size);
                 break;                        
        case CL_ALARM_ASSERT_SOAKING_TIME:
                 memcpy( value, &(appAlarms[classIdx].MoAlarms[idx].assertSoakingTime), *size);
                 break;            
        case CL_ALARM_CLEAR_SOAKING_TIME:
                 memcpy( value, &(appAlarms[classIdx].MoAlarms[idx].clearSoakingTime), *size);
                 break;            
        default:
            clLogError("ALM", "ALE", "Invalid profile attrid. attrid: %d \n", attrId);
            rc = CL_ERR_INVALID_PARAMETER;
            
    }

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "ALE", " Leaving [%s]", __FUNCTION__);
#endif

    return rc;

}


/**
 * Function to count the number of alarms associated with 
 * a particular resource type and returns that.
 */

static inline ClUint32T _clAlarmsGetNoOfAlarms (ClAlarmProfileT almProfile [])
{
    ClUint32T index = 0;

    for (index = 0; index < CL_ALARM_MAX_ALARMS; index ++)
    {
        if (almProfile[index].category == CL_ALARM_CATEGORY_INVALID )
            break;
    }

    return index;
}


/**
 * Function to give the count of the alarms configured on the 
 * MSO. This will be used to do retry if the alarm OM creation
 * is happening without properly configuring the Alarm MSO.
 */
ClRcT _clAlarmGetAlarmsCountOnResource (ClCorMOIdPtrT pMoId, ClUint32T *pExpectedIndex)
{
    ClRcT   rc = CL_OK;
    ClCorClassTypeT     moClassId = 0;
    ClUint32T           i = 0;
    
    rc = clCorMoIdToClassGet(pMoId, CL_COR_MO_CLASS_GET, &moClassId);
    if (CL_OK != rc)
    {
        clLogError("ALM", "ASE", "Failed to get the MOClass Id from MoId. rc[0x%x]", rc);
        return rc;
    }

    clLogTrace("ALM", "ASE", "Getting the expected alarms for MoClassId [0x%x]", moClassId);

    while (appAlarms[i].moClassID != 0) 
    {
        if (moClassId == appAlarms[i].moClassID)
        {
            *pExpectedIndex = _clAlarmsGetNoOfAlarms(appAlarms[i].MoAlarms);
            clLogTrace("ALM", "ASE", "The number of alarms for the MoClass [0x%x], [%d]",
                    moClassId, *pExpectedIndex);
            return CL_OK;
        }
        i++;
    }

    /* The flow should not come here.*/
    CL_ASSERT(0);

    return CL_ALARM_RC(CL_ERR_NOT_EXIST);
}


/**
 * Function to get the alarm handle for the given alarm Info.
 */
                                // coverity[pass_by_value]
ClRcT _clAlarmHandleFromInfoGet(ClAlarmInfoT alarmInfo, 
                                ClAlarmHandleT *pAlarmHandle)
{
    ClRcT           rc = CL_OK;
    ClRmdOptionsT   rmdOpts = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT   iocAddr = {{0}};
    ClBufferHandleT inMsgH = 0, outMsgH = 0;
    ClCorAddrT      addr;

    rc = clBufferCreate(&inMsgH);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AHG", "Failed to create the input message handle. rc[0x%x]", rc);
        return rc;
    }

    rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)(&alarmInfo, inMsgH, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AHG", "Failed to marshall the alarm info. rc[0x%x]", rc);
        clBufferDelete(&inMsgH);
        return rc;
    }

    rc = clBufferCreate(&outMsgH);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AHG", "Failed to create the out message handle. rc[0x%x]", rc);
        goto errorHandle;
    }
    
    /* Need to get the alarm handle from the alarm server running on the node where 
     * the alarm owner is running. Since the alarm owner will contact the local 
     * alarm server only.
     */
    rc = clCorMoIdToComponentAddressGet(&(alarmInfo.moId), &addr);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AHG", "Failed to get the alarm owner information from COR. rc [0x%x]", rc);
        goto errorHandle;
    }

	iocAddr.iocPhyAddress.nodeAddress = addr.nodeAddress;
	iocAddr.iocPhyAddress.portId    = CL_IOC_ALARM_PORT;

	rc = clRmdWithMsg (iocAddr,
					   CL_ALARM_HANDLE_FROM_INFO_GET,
					   inMsgH,
					   outMsgH,
					   CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_NEED_REPLY,
					   &rmdOpts,
					   NULL);
	if(rc != CL_OK)
	{
		clLogError( "ALM", "ALR", "Failed while Sending RMD to the client \
		    owning the alarm resource. rc[0x%x]", rc);
        goto errorHandle;
	}

    rc = clXdrUnmarshallClUint32T(outMsgH, pAlarmHandle);
    if (CL_OK != rc)
    {
        clLogError("ALM", "APG", "Failed to unmarshall the count of pending alarms. rc[0x%x]", rc);
        goto errorHandle;
    }

errorHandle:
    clBufferDelete(&outMsgH);
    clBufferDelete(&inMsgH);
    return rc;
}

/**
 * Internal function to add the alarm into the pending alarms list.
 */

ClRcT   
_clAlarmInfoAddToList ( ClCorMOIdPtrT pMoId, 
                        ClCorObjectHandleT objH, 
                        ClBufferHandleT pendingAlmList, 
                        ClInt32T index, 
                        ClAlarmStateT alarmState,
                        ClBoolT     getAlarmHandle)
{
    ClRcT                   rc = CL_OK;
    ClAlarmHandleInfoIDLT      alarmHandleInfoIdl = {0};
    ClUint32T               size = 0;
    ClAlarmInfoPtrT         pAlarmInfo = NULL;
    ClAlarmInfoT            alarmInfo = {0};
    ClNameT                 moIdName = {0};

    alarmHandleInfoIdl.alarmInfo.moId = *pMoId;
    
    size = sizeof (ClUint32T);
    rc = GetAlmAttrValue(pMoId, objH, 
            CL_ALARM_PROBABLE_CAUSE, 
            index, &alarmHandleInfoIdl.alarmInfo.probCause, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AIA", 
                "Failed to get the probable cause for alarm index [%d]. rc[0x%x]", index, rc);
        return rc;
    }

    size = sizeof (ClUint8T);
    rc = GetAlmAttrValue(pMoId, objH,
            CL_ALARM_CATEGORY, 
            index, &alarmHandleInfoIdl.alarmInfo.category, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AIA", 
                "Failed to get the category for alarm index [%d]. rc[0x%x]", index, rc);
        return rc;
    }

    size = sizeof (ClUint32T);
    rc = GetAlmAttrValue(pMoId, objH, 
            CL_ALARM_SPECIFIC_PROBLEM, 
            index, &alarmHandleInfoIdl.alarmInfo.specificProblem, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AIA", 
                "Failed to get the specific problem for alarm index [%d]. rc[0x%x]", index, rc);
        return rc;
    }

    size = sizeof (ClUint8T);
    rc = GetAlmAttrValue(pMoId, objH, 
            CL_ALARM_SEVERITY, 
            index, &alarmHandleInfoIdl.alarmInfo.severity, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "AIA", 
                "Failed to get the severity for alarm index [%d]. rc[0x%x]", index, rc);

        return rc;
    }

    alarmHandleInfoIdl.alarmInfo.alarmState = alarmState;

    if (CL_TRUE == getAlarmHandle)
    {
        alarmInfo.moId      = alarmHandleInfoIdl.alarmInfo.moId;
        alarmInfo.probCause = alarmHandleInfoIdl.alarmInfo.probCause;
        alarmInfo.category  = alarmHandleInfoIdl.alarmInfo.category;
        alarmInfo.specificProblem = alarmHandleInfoIdl.alarmInfo.specificProblem;
        alarmInfo.severity  = alarmHandleInfoIdl.alarmInfo.severity;
        alarmInfo.alarmState = alarmHandleInfoIdl.alarmInfo.alarmState;

        rc = _clAlarmHandleFromInfoGet(alarmInfo, &alarmHandleInfoIdl.alarmHandle);
        if (CL_OK != rc)
        {
            clLogNotice("ALM", "AIA",
                    "Failed to get the alarm handle from the server, putting handle as invalid.");
            rc = CL_OK;
            alarmHandleInfoIdl.alarmHandle = CL_HANDLE_INVALID_VALUE;
        }

        rc = clAlarmClientResourceInfoGet(alarmHandleInfoIdl.alarmHandle, &pAlarmInfo);
        if (rc != CL_OK)
        {
            clLogNotice("ALM", "AIA",
                    "Failed to get the payload information from the alarm server. rc [0x%x]", rc);
            rc = CL_OK;
        }
        else
        {
            /* All the information are filled up properly *
             * Need to add the payload information if present *
             */
            if (pAlarmInfo->len > 0)
            {
                alarmHandleInfoIdl.alarmInfo.len = pAlarmInfo->len;
                alarmHandleInfoIdl.alarmInfo.buff = clHeapAllocate(alarmHandleInfoIdl.alarmInfo.len);

                if (alarmHandleInfoIdl.alarmInfo.buff == NULL)
                {
                    clLogError("ALM", "AIA", "Failed to allocate memory");
                    return CL_ALARM_RC(CL_ERR_NO_MEMORY);
                }

                memcpy(alarmHandleInfoIdl.alarmInfo.buff, pAlarmInfo->buff, pAlarmInfo->len);
            }

            /* Free the memory allocated by clAlarmClientResourceInfoGet ipi */
            clHeapFree(pAlarmInfo);
        }
    }
    else
    {
        alarmHandleInfoIdl.alarmHandle = CL_HANDLE_INVALID_VALUE;
    }

    rc = clCorMoIdToMoIdNameGet(&alarmHandleInfoIdl.alarmInfo.moId, &moIdName);
    if (rc == CL_OK)
    {
        clLogInfo("ALM", "AIA", "Adding the alarm information: MoId : [%s], ProbCause [%d], "
                "SpecificProblem [%d], Category [%d], Severity [%d], "
                "alarm State [%d], alarmHandle [%d], payload length [%d]",
                moIdName.value,
                alarmHandleInfoIdl.alarmInfo.probCause, alarmHandleInfoIdl.alarmInfo.specificProblem, 
                alarmHandleInfoIdl.alarmInfo.category, alarmHandleInfoIdl.alarmInfo.severity, 
                alarmHandleInfoIdl.alarmInfo.alarmState, alarmHandleInfoIdl.alarmHandle, 
                alarmHandleInfoIdl.alarmInfo.len);
    }
    else
    {
        rc = CL_OK;
        clLogInfo("ALM", "AIA", "Adding the alarm information: ProbCause [%d], "
                "SpecificProblem [%d], Category [%d], Severity [%d], "
                "alarm State [%d], alarmHandle [%d], payload length [%d]",
                alarmHandleInfoIdl.alarmInfo.probCause, alarmHandleInfoIdl.alarmInfo.specificProblem, 
                alarmHandleInfoIdl.alarmInfo.category, alarmHandleInfoIdl.alarmInfo.severity, 
                alarmHandleInfoIdl.alarmInfo.alarmState, alarmHandleInfoIdl.alarmHandle, 
                alarmHandleInfoIdl.alarmInfo.len);
    }

    rc = VDECL_VER(clXdrMarshallClAlarmHandleInfoIDLT, 4, 0, 0)(&alarmHandleInfoIdl, pendingAlmList, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "AIA", "Failed to marshall AlarmHandleInfoIDLT. rc [0x%x]", rc);
        return rc;
    }

    /* Free the memory allocated */
    if (alarmHandleInfoIdl.alarmInfo.len > 0)
        clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);

    return CL_OK;
}

/**
 * Internal function to get the pending alarms for any given MO.
 */

ClRcT
_clAlarmClientPendingAlmsForMOGet ( ClCorMOIdPtrT pMoId, 
                                    ClBufferHandleT pendingAlmList)
{
    ClRcT               rc = CL_OK;
    ClUint32T           index = 0, size = 0;
    ClUint64T           publishedAlmBM = 0;
    ClUint64T           supAlmBM = 0;
    ClUint64T           almsAfterSoakingBM = 0;
    ClUint64T           almsUnderSoakingBM = 0;
    ClUint64T           pendingAlmBM = 0;
    ClCorObjectHandleT  objH = NULL;
    const ClUint64T     one = 1;
    ClUint32T           nPendingAlms = 0;
    ClNameT             moIdName;

    rc = clCorObjectHandleGet(pMoId, &objH);
    if (CL_OK != rc)
    {
        clLogError("ALM", "PMG", "Failed to get the object handle for the MO. rc[0x%x]", rc);
        return rc;
    }

    /* If the alarm is in published alarm bitmap. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, objH, 
            CL_ALARM_PUBLISHED_ALARMS, 
            CL_COR_INVALID_ATTR_IDX, 
            &publishedAlmBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "PMG", "Failed while getting the published alarms bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    /* If the alarm is in the suppressed alarm bitmap.*/
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, objH, 
            CL_ALARM_SUPPRESSED_ALARMS, 
            CL_COR_INVALID_ATTR_IDX, 
            &supAlmBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "PMG", "Failed while getting the suppressed alarms bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    /* If the alarm is under soaking. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, objH,
            CL_ALARM_ALMS_UNDER_SOAKING,
            0, 
            &almsUnderSoakingBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "PMG", 
                "Failed while getting the alarm under soaking bitmap. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    /* Getting the alarms after soaking bitmap. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, objH,
            CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
            CL_COR_INVALID_ATTR_IDX, 
            &almsAfterSoakingBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "PMG", 
                "Failed while getting the alarms after soaking bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    rc = clCorMoIdToMoIdNameGet(pMoId, &moIdName);

    clLogDebug("TST", "SEN", "Alarm Bitmaps : MoId : [%s], Published : [%llu], Suppressed : [%llu], "
            "Under Soaking : [%llu], After Soaking : [%llu]", moIdName.value, publishedAlmBM, supAlmBM, 
            almsUnderSoakingBM, almsAfterSoakingBM);

    /* Find out the no. of pending alarms needs to be retrieved */
    pendingAlmBM = (publishedAlmBM | supAlmBM | almsAfterSoakingBM | almsUnderSoakingBM );

    for (index = 0; index < CL_ALARM_MAX_ALARMS; index++)
    {
        if (pendingAlmBM & (one << index))
        {
            ++nPendingAlms;
        }
    }

    /* Marshall the no. of pending alarms */
    rc = clXdrMarshallClUint32T(&nPendingAlms, pendingAlmList, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "PMG", "Failed to Marshall ClUint32T. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (nPendingAlms == 0)
    {
        clLogDebug("ALM", "PMG", "There are no pending alarms for this resource");
        clCorObjectHandleFree(&objH);
        return CL_OK;
    }

    for (index = 0; index < CL_ALARM_MAX_ALARMS; index++)
    {
        if (publishedAlmBM & ( one << index))
        {
            rc = _clAlarmInfoAddToList(pMoId, objH, pendingAlmList, 
                    index, CL_ALARM_STATE_ASSERT, CL_TRUE);
            if (CL_OK != rc)
            {
                clLogError("ALM", "PMG", 
                        "Failed while adding the alarm information for alarm "
                        "index [%d]. rc[0x%x]", index, rc);
                clCorObjectHandleFree(&objH);
                return rc;
            }

            clLogDebug("ALM", "PMG", 
                    "Found the alarm in the published alarms bitmap [%lld], adding the "
                    "alarm index [%d] to the pending alarms list. ", publishedAlmBM, index);
            continue;
        }

        if (supAlmBM & ( one << index))
        {
            rc = _clAlarmInfoAddToList(pMoId, objH, pendingAlmList, 
                    index, CL_ALARM_STATE_SUPPRESSED, CL_FALSE);
            if (CL_OK != rc)
            {
                clLogError("ALM", "PMG", 
                        "Failed while adding the alarm information for alarm "
                        "index [%d]. rc[0x%x]", index, rc);
                clCorObjectHandleFree(&objH);
                return rc;
            }

            clLogDebug("ALM", "PMG", 
                    "Found the alarm in the suppressed alarm bitmap [%lld], adding the "
                    "alarm index [%d] to the pending alarms list.", supAlmBM, index);
            continue;
        }

        if (almsUnderSoakingBM & ( one << index))
        {
            rc = _clAlarmInfoAddToList(pMoId, objH, pendingAlmList, 
                    index, CL_ALARM_STATE_UNDER_SOAKING, CL_FALSE);
            if (CL_OK != rc)
            {
                clLogError("ALM", "PMG", 
                        "Failed while adding the alarm information for alarm "
                        "index [%d]. rc[0x%x]", index, rc);
                clCorObjectHandleFree(&objH);
                return rc;
            }

            clLogDebug("ALM", "PMG", 
                    "Found the alarm in the under soaking bitmap [%lld], adding the "
                    "alarm index [%d] to the pending alarms list. ", almsUnderSoakingBM, index);
            continue;
        }

        if (almsAfterSoakingBM & ( one << index))
        {
            rc = _clAlarmInfoAddToList(pMoId, objH, pendingAlmList, 
                    index, CL_ALARM_STATE_ASSERT, CL_FALSE);
            if (CL_OK != rc)
            {
                clLogError("ALM", "PMG", 
                        "Failed while adding the alarm information for alarm "
                        "index [%d]. rc[0x%x]", index, rc);
                clCorObjectHandleFree(&objH);
                return rc;
            }

            clLogDebug("ALM", "PMG", 
                    "Found the alarm in the after soaking bitmap [%lld], adding the "
                    "alarm index [%d] to the pending alarms list.", almsAfterSoakingBM, index);
            continue;
        }
    }
  
    clCorObjectHandleFree(&objH);
    return rc;
}


/**
 * Callback function of the object walk which will be collecting
 * all the pending alarms information.
 */
ClRcT   
_clAlarmPendingAlmCallback (ClPtrT data, ClPtrT pCookie)
{
    ClRcT                       rc = CL_OK; 
    ClCorMOIdT                  moId;
    ClCorServiceIdT             svcId = CL_COR_INVALID_SRVC_ID;
    ClCorObjectHandleT          objH = *(ClCorObjectHandleT *)data;
    ClAlarmPendingAlmListPtrT   pPendingAlmList = (ClAlarmPendingAlmListPtrT)pCookie;
    ClAlarmPendingAlmListT      tempAlmList = {0};

    CL_ASSERT(NULL != pCookie);

    clCorMoIdInitialize(&moId);

    rc = clCorObjectHandleToMoIdGet(objH, &moId, &svcId);
    if (CL_OK != rc)
    {
        clLogError("ALM", "APG", "Failed to get the MOId from the object handle. rc[0x%x]", rc);
        return rc;
    }
     
    if (svcId != CL_COR_SVC_ID_ALARM_MANAGEMENT)
    {
        clLogInfo("ALM", "APG", "Not an alarm MSO svc Id[%d], so doing no op", svcId);
        return CL_OK;
    }

    rc = _clAlarmGetPendingAlarmFromOwner(&moId, &tempAlmList);
    if (CL_OK != rc)
    {
        clLogError("ALM", "APG", "Failed to get the list of pending alarms for the MO. rc[0x%x]", rc);
        return rc;
    }

    if (tempAlmList.noOfPendingAlarm == 0)
    {
        clLogInfo("ALM", "APG", "There are no pending alarms found for this resource. ");
        return CL_OK;
    }
    else
    {
        pPendingAlmList->pAlarmList = clHeapRealloc(pPendingAlmList->pAlarmList, sizeof(ClAlarmHandleInfoT) *
                                                (pPendingAlmList->noOfPendingAlarm + tempAlmList.noOfPendingAlarm));
        if (NULL == pPendingAlmList->pAlarmList)
        {
            clLogError("ALM", "APG", 
                    "Failed to reallocate the memory for appending the pending "
                    "alarm list. ");
            clHeapFree(tempAlmList.pAlarmList);
            return rc;
        }

        memcpy (pPendingAlmList->pAlarmList + pPendingAlmList->noOfPendingAlarm, 
                tempAlmList.pAlarmList, 
                tempAlmList.noOfPendingAlarm * sizeof (ClAlarmHandleInfoT));

        pPendingAlmList->noOfPendingAlarm += tempAlmList.noOfPendingAlarm;

        clHeapFree(tempAlmList.pAlarmList);

        /* The pPendingAlmList->pAlarmInfo is allocated memory in _clAlarmGetPendingAlarmFromOwner
         * for all the alarm's information. So that should not be freed here, and needs to be 
         * freed by the user by calling clAlarmPendingAlarmListFree.
         */
    }

    return rc;
}

/**
 * Internal function which will find all the pending alarms in the system.
 */
ClRcT   
_clAlarmAllPendingAlmGet(ClAlarmPendingAlmListPtrT const pPendingAlmList)
{
    ClRcT                       rc = CL_OK;

    rc = clCorObjectWalk(NULL, NULL, _clAlarmPendingAlmCallback, CL_COR_MSO_WALK, pPendingAlmList); 
    if (CL_OK != rc)
    {
        clLogError("ALM", "APG", "Failed to get the list of all the pending alarms in the system. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}
