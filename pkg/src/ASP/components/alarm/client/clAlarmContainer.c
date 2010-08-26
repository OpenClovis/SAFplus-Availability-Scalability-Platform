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
 * Build: 2.2
 */
/*******************************************************************************
 * ModuleName  : alarm                                                         
 * File        : clAlarmContainer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file provides the container implementation of the statically configured 
 * information and also the omhandle and subscription ids. This file will be
 * enhanced with more information relating to the om, when the om class definition
 * would be scrapped off.
 *
 *****************************************************************************/
/* Standard Inculdes */
#include<string.h>

/* ASP Includes */
#include <clCommon.h>
#include <clCntApi.h>
#include <clAlarmErrors.h>

/* cor apis */

/* alarm includes */
#include <clAlarmUtil.h>
#include <clAlarmClient.h>
#include <clAlarmCommons.h>
#include <clAlarmContainer.h>
#include <ipi/clAlarmIpi.h>

/******************************************************************************
 *  Global
 *****************************************************************************/

/*************************** Mutex variables ********************************/
/*
 * The mutex used to protect the container
 */
extern ClOsalMutexIdT gClAlarmCntMutex;
/*
 * The handle returned identifying the container
 */
extern ClCntHandleT     gresListHandle;

/* 
 * Instead of having the containment attribute spread out for 16 instances
 * we can allocate memory for only the number of alarms that have been 
 * configured, thereby saving memory. This function gets the number of alarms
 * that have been configure.
 */
ClRcT clAlarmGetNumOfConfiguredAlarms(ClCorMOIdPtrT pMoId, ClUint32T *ptrIndex,ClCorObjectHandleT hMSOObj)
{
	ClRcT rc=0;
	ClUint32T indexMax = 0;
	ClAlarmProbableCauseT probCause = 0;
	ClUint32T size = 0;

    for (indexMax =0; indexMax < CL_ALARM_MAX_ALARMS; indexMax++)
    {
		size = sizeof(probCause);
		if ( (rc = GetAlmAttrValue(pMoId, 
                                   hMSOObj, 
								   CL_ALARM_PROBABLE_CAUSE, 
								   indexMax,
								   &probCause, 
								   &size,
								   0)) != CL_OK )
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("GetAlmAttrValue failed with rc = %x\n", rc));
			return rc;
		}
		if(probCause == CL_ALARM_ID_INVALID)
			break;
	}
	*ptrIndex = indexMax;
	return rc;
}

/*
 * The cnt add function
 * The information is read from COR and then populated in the container
 * This api would be called as and when the object is created and the corresponding entry 
 * within the container needs to be added.
 * Now as only a single container would be there, this function would just be filling in
 * the values for caching of the configuration and object handle information.
 * TODO : Make use of bulk read.
 * TODO : Assert soaking time, clear soaking time and fetch mode needs to be cached once they are part of COR.
 * TODO : Need to do a bulk read while getting all the attributes. Need to store all the pointers for the jobs
 *        so that the values can be read once the callback is called in an asynchronous mode. All these would
 *        need to be stored in a cnt with cor session handle as the key. For synchrnous mode we can live
 *        without storing these values.
 */
ClRcT clAlarmStaticInfoCntAdd(ClCorMOIdPtrT pMoId, ClAlarmResCntDataT *resInfo)
{
	ClRcT rc = CL_OK;
    ClCorClassTypeT classId = 0;
    ClUint32T i = 0;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pAlarmResData = NULL;
    ClCorObjectHandleT objH = NULL;
    VDECL_VER(ClCorAlarmProfileDataIDLT, 4, 1, 0)* pAlarmProfile = NULL;

    rc = clCorMoIdToClassGet(pMoId, CL_COR_MO_CLASS_GET, &classId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "OMCONFIGURE", "Failed to get classId from MoId. rc [0x%x]", rc);
        return rc;
    }

    rc = clAlarmClientConfigTableResDataGet(classId, &pAlarmResData);
    if (rc != CL_OK)
    {
        clLogError("ALM", "OMCONFIGURE", "Failed to get resource data for the classId [%d]. rc [0x%x]", classId, rc);
        return rc;
    }

    resInfo->alarmContInfo = clHeapAllocate(pAlarmResData->noOfAlarms * sizeof(ClAlarmContInfoT));
    if (! resInfo->alarmContInfo)
    {
        clLogError("ALM", "OMCONFIGURE", "Failed to allocate memory.");
        return CL_ALARM_RC(CL_ERR_NO_MEMORY); 
    }

    rc = clCorMoIdToObjectHandleGet(pMoId, &objH);
    if (rc != CL_OK)
    {
        clLogError("ALM", "OMCONFIGURE", "Failed to get object handle from MoId. rc [0x%x]", rc);
        return rc;
    }

    resInfo->hMSOObj = objH;
    resInfo->numofValidAlarms = pAlarmResData->noOfAlarms;
    
    for (i=0; i<pAlarmResData->noOfAlarms; i++)
    {
        pAlarmProfile = (pAlarmResData->pAlarmProfile+i);

        CL_ASSERT(pAlarmProfile);

#ifdef ALARM_POLL 
        resInfo->msoToBePolled = pAlarmProfile->pollAlarm;
        resInfo->pollingInterval = pAlarmProfile->pollInterval;
#endif

        resInfo->alarmContInfo[i].generationRule = pAlarmProfile->genRule;
        resInfo->alarmContInfo[i].generationRuleRelation = pAlarmProfile->genRuleRel;
        resInfo->alarmContInfo[i].suppressionRule = pAlarmProfile->supRule;
        resInfo->alarmContInfo[i].suppressionRuleRelation = pAlarmProfile->supRuleRel; 
        resInfo->alarmContInfo[i].alarmCategory = pAlarmProfile->category;
        resInfo->alarmContInfo[i].probCause = pAlarmProfile->probCause;
        resInfo->alarmContInfo[i].specProb = pAlarmProfile->specProb;
        resInfo->alarmContInfo[i].affectedAlarms = pAlarmProfile->affectedAlarms;
    }

    return rc;
}

/*
 * The cnt data get api
 * TODO : Instead of getting all the attributes one by one, get all the attributes at a shot
 * as the container is internal implementation and then retrieve the specific information.
 * This will get rid of the switch case and make the data retrieval faster. The problems
 * with this approach is we have to maintain the information and selectively retrieve the
 * information. We can't blindly call the get alarm attribute value and we will have to 
 * optimise the code so that after a call to get the data from the container we will have 
 * to use the data efficiently to retrieve all the information without making further calls
 * to the cnt. This requires more effort, as this would involve modifying almost all of the
 * api's and this would also involve selectively fetching the information as and when 
 * required. The optimization achieved via this would take away the readability of the code.
 * TODO : Need to take care of assert soaking time, clear soaking time and fetch mode in the
 * switch case once they become part of COR.
 */

ClRcT clAlarmStaticInfoCntDataGet(ClCorMOIdPtrT pMoId, 
                                   ClUint32T attrId,
                                   ClUint32T index,
                                   void *value, 
                                   ClUint32T lockApplied)
{
	ClRcT rc = CL_OK;
	ClUint32T tempIndex = 0;
	ClAlarmResCntDataT *resInfo;
	//ClAlarmProbableCauseT probCause;
	ClAlarmProbableCauseT *alarmId;


/* The parameter lockApplied is being passed to ascertain if this call made inside any of the
 * callbacks of the container walk function. Earlier the lock was being applied in all these
 * function which was resulting in the restarting of the application linking to the alarm client
 * library */
	if(!lockApplied)
	{
		clOsalMutexLock(gClAlarmCntMutex);        
		rc = clCntDataForKeyGet(gresListHandle , (ClCntKeyHandleT)pMoId,
										(ClCntDataHandleT*)&resInfo);
		clOsalMutexUnlock(gClAlarmCntMutex);    
	}
	else
	{
		rc = clCntDataForKeyGet(gresListHandle , (ClCntKeyHandleT)pMoId,
										(ClCntDataHandleT*)&resInfo);
	}

    if (CL_OK != rc)
    {
		clLogInfo("ALM", "STATICINFO", "clCntDataForKeyGet failed with rc [0x%x]", rc);
		return rc;
    }

	switch(attrId)
	{
		case CL_ALARM_POLLING_INTERVAL:
			memcpy(value,&(resInfo->pollingInterval),sizeof(ClUint32T));
			break;

		case CL_ALARM_MSO_TO_BE_POLLED :
			memcpy(value,&(resInfo->msoToBePolled),sizeof(ClUint8T));
			break;

		case CL_ALARM_ID :
			if(index == -1)
			{
				alarmId = (ClAlarmProbableCauseT*) value;
				for(tempIndex = 0;tempIndex < (resInfo->numofValidAlarms);tempIndex ++)
					alarmId[tempIndex] = resInfo->alarmContInfo[tempIndex].probCause;
			}
			else
				memcpy(value,&(resInfo->alarmContInfo[index].probCause),sizeof(ClAlarmProbableCauseT));
			break;

		case CL_ALARM_PROBABLE_CAUSE :
			memcpy(value,&(resInfo->alarmContInfo[index].probCause),sizeof(ClUint32T));
			break;

		case CL_ALARM_SPECIFIC_PROBLEM :
			memcpy(value,&(resInfo->alarmContInfo[index].specProb),sizeof(ClUint32T));
			break;

		case CL_ALARM_CATEGORY :
			memcpy(value,&(resInfo->alarmContInfo[index].alarmCategory),sizeof(ClUint8T));
			break;

		case CL_ALARM_AFFECTED_ALARMS :
			memcpy(value,&(resInfo->alarmContInfo[index].affectedAlarms),sizeof(ClUint64T));
			break;

		case CL_ALARM_GENERATION_RULE :
			memcpy(value,&(resInfo->alarmContInfo[index].generationRule),sizeof(ClUint64T));
			break;

		case CL_ALARM_SUPPRESSION_RULE :
			memcpy(value,&(resInfo->alarmContInfo[index].suppressionRule),sizeof(ClUint64T));
			break;

		case CL_ALARM_GENERATE_RULE_RELATION :
			memcpy(value,&(resInfo->alarmContInfo[index].generationRuleRelation),sizeof(ClAlarmRuleRelationT));
			break;

		case CL_ALARM_SUPPRESSION_RULE_RELATION :
			memcpy(value,&(resInfo->alarmContInfo[index].suppressionRuleRelation),sizeof(ClAlarmRuleRelationT));
			break;

		default :
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid attribute Id \n"));
    }        
	return rc;
}

/* 
 * The object handle get api
 * TODO : Instead of bifurcating to COR or to cnt within the same api
 * use two explicit api's.
 */
ClRcT clAlarmStaticInfoCntMoIdToHandleGet(ClCorMOIdPtrT pMoId,ClCorObjectHandleT* hMSOObj)
{
	ClRcT rc=CL_OK;
	ClAlarmResCntDataT *resInfo;
	//resInfo = clHeapAllocate(sizeof(ClAlarmStaticInfoT));
	ClCntNodeHandleT nodeH;

    rc = clCntNodeFind(gresListHandle ,(ClCntKeyHandleT)pMoId,&nodeH);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntNodeFind failed w rc:0x%x\n", rc));
		return rc;
    }        
	else
	{
		rc = clCntDataForKeyGet(gresListHandle , (ClCntKeyHandleT)pMoId,
										(ClCntDataHandleT*)&resInfo);
		if (CL_OK != rc)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet failed [%x]",rc));
			return rc;
		}
		//memcpy(hMSOObj.tree,(resInfo->hMSOObj).tree,sizeof(ClCorObjectHandleT));
		*hMSOObj=resInfo->hMSOObj;
	}
	return rc;
}


/* 
 * The cnt node find api.
 */
ClRcT clAlarmStaticInfoCntFind(ClCorMOIdPtrT pMoId, ClUint32T lockApplied)
{
	ClRcT rc = CL_OK;

	ClCntNodeHandleT nodeH;

/* The parameter lockApplied is being passed to ascertain if this call made inside any of the
 * callbacks of the container walk function. Earlier the lock was being applied in all these
 * function which was resulting in the restarting of the application linking to the alarm client
 * library */
	if(!lockApplied)
	{
		clOsalMutexLock(gClAlarmCntMutex);        
		rc = clCntNodeFind(gresListHandle ,(ClCntKeyHandleT)pMoId,&nodeH);
		clOsalMutexUnlock(gClAlarmCntMutex);    
	}
	else
	{
		rc = clCntNodeFind(gresListHandle ,(ClCntKeyHandleT)pMoId,&nodeH);
	}
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("clCntNodeFind failed w rc:0x%x\n", rc));
	}
	return rc;
}
