#include <custom.h>

ClRcT
clAmsPeSIIsActiveAssignableCustom(CL_IN ClAmsSIT *si)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg;

    AMS_CHECK_SI (si);
    AMS_CHECK_SG (sg = (ClAmsSGT *) si->config.parentSG.ptr);
    
    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        if ( si->status.numActiveAssignments < sg->config.numPrefActiveSUsPerSI )
        {
            return clAmsEntityListWalkGetEntity(
                                                &si->config.siDependenciesList,
                                                (ClAmsEntityCallbackT)clAmsPeSIIsActiveAssignable2);
        }
    }

    return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
}

ClBoolT clAmsPeCheckAssignedCustom(ClAmsSUT *su, ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef;
    for(eRef = clAmsEntityListGetFirst(&su->status.siList); eRef;
        eRef = clAmsEntityListGetNext(&su->status.siList, eRef))
    {
        ClAmsSIT *assignedSI = (ClAmsSIT*)eRef->ptr;
        if(assignedSI == si) return CL_TRUE;
    }
    return CL_FALSE;
}

ClRcT
clAmsPeSGFindSIForActiveAssignmentCustom(
        CL_IN ClAmsSGT *sg,
        CL_INOUT ClAmsSIT **targetSI) 
{
    ClAmsEntityRefT *entityRef;
    ClAmsSIT* lookAfter = NULL;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !targetSI );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if (*targetSI) lookAfter = *targetSI;

    /*
     * For SI preference loading strategy, try to find the SI which has an assignable preferred SU first
     */
    if(sg->config.loadingStrategy == CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE)
    {
        for (entityRef = clAmsEntityListGetFirst(&sg->config.siList);
             entityRef !=  NULL;
             entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef)) 
        {
            ClAmsEntityRefT *suRef;
            ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
            AMS_CHECK_SI(si);
            if(lookAfter == si) continue;
            if(clAmsPeSIIsActiveAssignableCustom(si) != CL_OK)
                continue;
            for(suRef = clAmsEntityListGetFirst(&si->config.suList);
                suRef != NULL;
                suRef = clAmsEntityListGetNext(&si->config.suList, suRef))
            {
                ClAmsSUT *su = (ClAmsSUT*)suRef->ptr;
                if(clAmsPeSUIsAssignable(su) != CL_OK) 
                    continue;
                if(su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
                    continue;
                if(clAmsPeCheckAssignedCustom(su, si))
                    continue;
                if(su->status.numActiveSIs >= sg->config.maxActiveSIsPerSU)
                    continue;
                *targetSI = si;
                return CL_OK;
            }
        }
        lookAfter = NULL;
    }
    
    *targetSI = NULL;
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT
clAmsPeSGFindSUForStandbyAssignmentCustom(
        CL_IN ClAmsSGT *sg,
        CL_IN ClAmsSUT **su,
        CL_IN ClAmsSIT *si)
{

    ClAmsEntityRefT *eRef = NULL;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !su );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    *su = (ClAmsSUT *) NULL;

    switch ( sg->config.loadingStrategy )
    {
        /*
         * This loading strategy picks the SU based on the SI's preference.
         */

        case CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE:
        {
            for ( eRef = clAmsEntityListGetFirst(&si->config.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&si->config.suList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                if(clAmsPeSUIsAssignable(tmpSU) != CL_OK)
                {
                    tmpSU->status.numDelayAssignments = 0;
                    if(tmpSU->status.suAssignmentTimer.count > 0)
                    {
                        clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                             CL_AMS_SU_TIMER_ASSIGNMENT);
                    }
                    continue;
                }

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    /*
                     * Delay assignment if possible.
                     */
                    if(tmpSU->status.suAssignmentTimer.count > 0)
                    {
                        return CL_AMS_RC(CL_ERR_NOT_EXIST);
                    }

                    if(tmpSU->status.numDelayAssignments < 2 )
                    {
                        ++tmpSU->status.numDelayAssignments;

                        clLogDebug("SU", "PREF-ASSGN",
                                   "Delaying preferred standby SI [%s] assignment to SU [%s] "
                                   "by [%d] ms",
                                   si->config.entity.name.value, 
                                   tmpSU->config.entity.name.value,
                                   CL_AMS_SU_ASSIGNMENT_DELAY);

                        AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT*)tmpSU, 
                                                         CL_AMS_SU_TIMER_ASSIGNMENT) );
                        return CL_AMS_RC(CL_ERR_NOT_EXIST);
                    }
                    
                    tmpSU->status.numDelayAssignments = 0;
                    continue;
                }

                if(clAmsPeCheckAssignedCustom(tmpSU, si))
                    continue;

                tmpSU->status.numDelayAssignments = 0;

                if(tmpSU->status.suAssignmentTimer.count > 0)
                {
                    clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                         CL_AMS_SU_TIMER_ASSIGNMENT);
                }
                
                if ( tmpSU->status.numStandbySIs < sg->config.maxStandbySIsPerSU )
                {
                    *su = tmpSU;

                    return CL_OK;
                }
            }

            /*
             * Now check if any of the SU in the SG are waiting for a preferred
             * SU. If so, delay assignment of this guy.
             */
            if(clAmsPeSGCheckSUAssignmentDelay(sg) == CL_OK)
            {
                return CL_AMS_RC(CL_ERR_NOT_EXIST);
            }

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

        default:
        {
            AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }
}

ClRcT
clAmsPeSGFindSUForActiveAssignmentCustom(
        CL_IN ClAmsSGT *sg,
        CL_IN ClAmsSUT **su,
        CL_IN ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !su );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    *su = (ClAmsSUT *) NULL;

    switch ( sg->config.loadingStrategy )
    {
        /*
         * This loading strategy picks the SU based on the SI's preference.
         */
        case CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE:
        {
            for ( eRef = clAmsEntityListGetFirst(&si->config.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&si->config.suList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                if(clAmsPeCheckSUReassignOp(tmpSU, si, CL_TRUE))
                {
                    *su = tmpSU;
                    return CL_OK;
                }

                if(clAmsPeSUIsAssignable(tmpSU) != CL_OK)
                {
                    tmpSU->status.numDelayAssignments = 0;
                    if(tmpSU->status.suAssignmentTimer.count > 0)
                    {
                        clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                             CL_AMS_SU_TIMER_ASSIGNMENT);
                    }
                    continue;
                }

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    /*
                     * Delay assignments if possible
                     */
                    if(tmpSU->status.suAssignmentTimer.count > 0)
                    {
                        return CL_AMS_RC(CL_ERR_NOT_EXIST);
                    }
                    
                    if(tmpSU->status.numDelayAssignments < 2 )
                    {
                        ++tmpSU->status.numDelayAssignments;
                        
                        clLogDebug("SU", "PREF-ASSGN",
                                   "Delaying preferred active SI [%s] assignment to SU [%s] "
                                   "by [%d] ms",
                                   si->config.entity.name.value, 
                                   tmpSU->config.entity.name.value,
                                   CL_AMS_SU_ASSIGNMENT_DELAY);

                        AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT*)tmpSU, 
                                                         CL_AMS_SU_TIMER_ASSIGNMENT) );
                        return CL_AMS_RC(CL_ERR_NOT_EXIST);
                    }

                    tmpSU->status.numDelayAssignments = 0;

                    continue;
                }
                
                if(clAmsPeCheckAssignedCustom(tmpSU, si))
                    continue;

                tmpSU->status.numDelayAssignments = 0;

                if(tmpSU->status.suAssignmentTimer.count > 0)
                {
                    clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                         CL_AMS_SU_TIMER_ASSIGNMENT);
                }

                if ( tmpSU->status.numActiveSIs < sg->config.maxActiveSIsPerSU )
                {
                    *su = tmpSU;

                    return CL_OK;
                }
            }

            /*
             * Now check if any of the SU in the SG are waiting for a preferred
             * SU. If so, delay assignment of this guy.
             */
            if(clAmsPeSGCheckSUAssignmentDelay(sg) == CL_OK)
            {
                return CL_AMS_RC(CL_ERR_NOT_EXIST);
            }

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

        default:
        {
            AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);

        }
    }
}

ClRcT
clAmsPeSGAssignSUCustom(
                        CL_IN ClAmsSGT *sg
                        )
{
    ClAmsSIT **scannedSIList = NULL;
    ClUint32T numScannedSIs = 0;
    ClUint32T numMaxSIs = 0;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    /*
     * Find SU assignments for SIs requiring active assignments
     */
 
    {
        ClRcT rc1 = CL_OK;
        ClRcT rc2 = CL_OK;
        ClAmsSIT *lastSI = NULL;
        ClAmsSUT *lastSU = NULL;

        while ( 1 )
        {
            ClAmsSUT *su = NULL;
            ClAmsSIT *si=NULL;

            rc1 = clAmsPeSGFindSIForActiveAssignmentCustom(sg, &si);

            if ( rc1 != CL_OK ) 
            {
                break;
            }
            clLogInfo("SG", "ASI",
                              "SI [%.*s] needs assignment...",
                              si->config.entity.name.length-1,
                              si->config.entity.name.value);

            rc2 = clAmsPeSGFindSUForActiveAssignmentCustom(sg, &su, si);

            if ( rc2 != CL_OK )
            {
                break;
            }

            if( (lastSI == si) && (lastSU == su) )
            {
                AMS_LOG(CL_DEBUG_ERROR, 
                        ("Assign active to SG - Current SI and SU same as "\
                         "last selection. Breaking out of assignment\n"));
                break;
            }

            lastSI = si;
            lastSU = su;
            su->status.numWaitAdjustments = 0;
            AMS_CALL ( clAmsPeSUAssignSI(su, si, CL_AMS_HA_STATE_ACTIVE) );
        }

        if ( (rc1 != CL_OK) && (CL_GET_ERROR_CODE(rc1) != CL_ERR_NOT_EXIST) )
        {
            return rc1;
        }

        if ( (rc2 != CL_OK) && (CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST) )
        {
            return rc2;
        }
    }

    /*
     * Find SU assignments for SIs requiring standby assignments
     */
 
    {
        ClRcT rc1 = CL_OK;
        ClRcT rc2 = CL_OK;
        ClAmsSIT *lastSI = NULL;
        ClAmsSUT *lastSU = NULL;

        while ( 1 )
        {
            ClAmsSIT *si = NULL;
            ClAmsSUT *su = NULL;

            rc1 = clAmsPeSGFindSIForStandbyAssignment(sg, &si, scannedSIList, numScannedSIs);

            if ( rc1 != CL_OK ) 
            {
                break;
            }

            rc2 = clAmsPeSGFindSUForStandbyAssignmentCustom(sg, &su, si);

            if ( rc2 != CL_OK )
            {
                break;
            }

            if( (lastSI == si) && (lastSU == su) )
            {
                AMS_LOG(CL_DEBUG_ERROR, 
                        ("Assign standby to SG - Current SI and SU same as "\
                         "last selection. Breaking out of assignment step\n"));
                break;
            }

            lastSI = si;
            lastSU = su;

            rc2 = clAmsPeSUAssignSI(su, si, CL_AMS_HA_STATE_STANDBY);

            if(rc2 != CL_OK)
            {
                if(CL_GET_ERROR_CODE(rc2) == CL_ERR_DOESNT_EXIST
                   ||
                   CL_GET_ERROR_CODE(rc2) == CL_ERR_NOT_EXIST)
                {
                    /*
                     * We could be encountering fixed slot protection config.
                     * So skip this SI and check for other SIs that could be
                     * assigned as standby
                     */
                    ClUint32T numSIs = sg->config.siList.numEntities;

                    if(!numSIs) 
                    {
                        if(scannedSIList) clHeapFree(scannedSIList);
                        return rc2;
                    }

                    if(numSIs > numMaxSIs)
                    {
                        numMaxSIs = numSIs;

                        scannedSIList = clHeapRealloc(scannedSIList,
                                                      numSIs * sizeof(*scannedSIList));
                        CL_ASSERT(scannedSIList != NULL);
                    }
                    scannedSIList[numScannedSIs++] = si;
                    rc2 = CL_OK;
                    continue;
                }
                else
                {
                    if(scannedSIList) clHeapFree(scannedSIList);
                    return rc2;
                }
            }
        }

        if ( (rc1 != CL_OK) && (CL_GET_ERROR_CODE(rc1) != CL_ERR_NOT_EXIST) )
        {
            if(scannedSIList) clHeapFree(scannedSIList);
            return rc1;
        }

        if ( (rc2 != CL_OK) && (CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST) )
        {
            if(scannedSIList) clHeapFree(scannedSIList);
            return rc2;
        }
    }

    if(scannedSIList) clHeapFree(scannedSIList);
    return CL_OK;
}

