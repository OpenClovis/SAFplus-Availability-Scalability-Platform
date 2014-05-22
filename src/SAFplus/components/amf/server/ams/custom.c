#include <custom.h>

#define CUSTOM_ASSIGNMENT_KEY "CUSTOM_ASSIGNMENT"

typedef struct ClAmsCustomAssignmentIter
{
    ClUint32T curIndex;
    ClAmsSISURefBufferT buffer;
    ClAmsSIT *si;
}ClAmsCustomAssignmentIterT;

typedef struct ClAmsSUAdjustCustom
{
    ClAmsSUT *su;
    ClAmsSIT *si;
    ClAmsHAStateT haState;
    ClListHeadT list;
}ClAmsSUAdjustCustomT;

static ClRcT 
clAmsCustomAssignmentIterInit(ClAmsCustomAssignmentIterT *iter, ClAmsSIT *si);

static ClAmsSISURefT *
clAmsCustomAssignmentIterNext(ClAmsCustomAssignmentIterT *iter);

static void
clAmsCustomAssignmentIterEnd(ClAmsCustomAssignmentIterT *iter);

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
        CL_INOUT ClAmsSIT **targetSI,
        CL_INOUT ClAmsSUT **targetSU) 
{
    ClAmsEntityRefT *entityRef;
    ClAmsSIT* lookAfter = NULL;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !targetSI );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if (*targetSI) lookAfter = *targetSI;
    if (targetSU) *targetSU = NULL;

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
            ClAmsSGT *sg;
            AMS_CHECK_SI(si);
            sg = (ClAmsSGT*)si->config.parentSG.ptr;
            if(!sg) continue;
            if(lookAfter == si) continue;
            if(clAmsPeSIIsActiveAssignableCustom(si) != CL_OK
               ||
               sg->status.adjustTimer.count > 0)
            {
                continue;
            }
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
    else
    {
        /*
         * Check assignment preference enqueued in each of the SIs
         */
        *targetSI = NULL;
        for(entityRef = clAmsEntityListGetFirst(&sg->config.siList);
            entityRef != NULL;
            entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef))
        {
            ClAmsSISURefT *siSURef = NULL;
            ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
            ClAmsSGT *sg = NULL;
            ClAmsSUT *foundSu = NULL;
            ClAmsCustomAssignmentIterT iter = {0};
            if(!si) 
                continue;
            sg = (ClAmsSGT*)si->config.parentSG.ptr;
            if(!sg) continue;
            if(clAmsPeSIIsActiveAssignableCustom(si) != CL_OK
               ||
               sg->status.adjustTimer.count > 0) 
            {
                continue;
            }

            ClBoolT preferred = CL_FALSE;

            clAmsCustomAssignmentIterInit(&iter, si);
            while( (siSURef = clAmsCustomAssignmentIterNext(&iter)) )
            {
                if ((siSURef->haState == CL_AMS_HA_STATE_ACTIVE)||(siSURef->haState == CL_AMS_HA_STATE_STANDBY))
                {
                    ClAmsSUT *su = (ClAmsSUT*)siSURef->entityRef.ptr;
                    if(clAmsPeSUIsAssignable(su) != CL_OK)
                        continue;
                    if(su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
                        continue;
                    // This code blocks multiple SIs from being assigned to one SU
                    //if(clAmsPeCheckAssignedCustom(su, si))
                    //    continue;
                    if(su->status.numActiveSIs >= sg->config.maxActiveSIsPerSU)
                        continue;

                    // Ok it passed all checks, this SI/SU pair is a candidate for Active.
                    
                    *targetSI = si;
                    foundSu = su;
                    if (targetSU) *targetSU = su;
                    if (siSURef->haState == CL_AMS_HA_STATE_ACTIVE)  // Its the preferred active so take it.  If its standby we'll keep looking
                    {
                        preferred = CL_TRUE;
                        break;
                    }
                }                
            }
            clAmsCustomAssignmentIterEnd(&iter);

            if ((!preferred)&&(foundSu)&&(foundSu->status.numDelayAssignments < 4 ))
            {
                ++foundSu->status.numDelayAssignments;
                clLogDebug("CUST", "ASSIGN", "Delaying preferred standby SI [%s] active assignment to SU [%s] by [%d] ms. Delay [%d] of [4]",
                           si->config.entity.name.value, 
                           foundSu->config.entity.name.value, CL_AMS_SU_ASSIGNMENT_DELAY, foundSu->status.numDelayAssignments );

                AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT*)foundSu, CL_AMS_SU_TIMER_ASSIGNMENT) );
                *targetSI = NULL;
                foundSu = NULL;
            }
            else
            {
                if (foundSu) foundSu->status.numDelayAssignments = 0;            
                if(*targetSI) return CL_OK;
            }
            

        }
    }
    *targetSI = NULL;
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT
clAmsPeSGFindSIForStandbyAssignmentCustom(
                                          CL_IN ClAmsSGT *sg,
                                          CL_OUT ClAmsSIT **targetSI,
                                          CL_OUT ClAmsSUT **targetSU,
                                          CL_IN ClAmsSIT **scannedSIList,
                                          CL_IN ClUint32T numScannedSIs)
{
    ClAmsEntityRefT *entityRef = NULL;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !targetSI );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    *targetSI = NULL;
    if(targetSU)
        *targetSU = NULL;

    for(entityRef = clAmsEntityListGetFirst(&sg->config.siList);
        entityRef != NULL;
        entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef))
    {
        ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
        ClAmsSGT *sg = (ClAmsSGT*)si->config.parentSG.ptr;
        ClAmsSISURefT *siSURef = NULL;
        ClAmsCustomAssignmentIterT iter = {0};
        if(clAmsPeSIIsStandbyAssignable(si) != CL_OK
           ||
           sg->status.adjustTimer.count > 0)
        {
            continue;
        }
        clAmsCustomAssignmentIterInit(&iter, si);
        while ( (siSURef = clAmsCustomAssignmentIterNext(&iter) ) )
        {
            ClAmsSUT *su ;
            /* These lines cause the assignments to be taken literally;
               an entity configured as active will not be assigned standby, for example.
               They are commented out to implement more of a DWIM-style custom
               redundancy mode.
            if(siSURef->haState != CL_AMS_HA_STATE_STANDBY)
                continue;
            */
            su = (ClAmsSUT*)siSURef->entityRef.ptr;
            if(clAmsPeSUIsAssignable(su) != CL_OK)
                continue;
            if(su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
                continue;
            if(clAmsPeCheckAssignedCustom(su, si))
                continue;
            if(su->status.numStandbySIs >= sg->config.maxStandbySIsPerSU)
                continue;
            *targetSI = si;
            if(targetSU)
                *targetSU = su;
            break;
        }
        clAmsCustomAssignmentIterEnd(&iter);
        if(*targetSI)
            return CL_OK;
    }
    return clAmsPeSGFindSIForStandbyAssignment(sg, targetSI, scannedSIList, numScannedSIs);
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
            break;
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
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
            break;
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
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

            rc1 = clAmsPeSGFindSIForActiveAssignmentCustom(sg, &si, &su);

            if ( rc1 != CL_OK ) 
            {
                break;
            }
            
            clLogInfo("SG", "ASI",
                      "SI [%.*s] needs assignment...",
                      si->config.entity.name.length-1,
                      si->config.entity.name.value);
            
            if(!su)
            {
                rc2 = clAmsPeSGFindSUForActiveAssignmentCustom(sg, &su, si);

                if ( rc2 != CL_OK )
                {
                    break;
                }
            }

            if( (lastSI == si) && (lastSU == su) )
            {
                AMS_LOG(CL_LOG_SEV_ERROR, 
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

            rc1 = clAmsPeSGFindSIForStandbyAssignmentCustom(sg, &si, &su, scannedSIList, numScannedSIs);

            if ( rc1 != CL_OK ) 
            {
                break;
            }

            if(!su)
            {
                rc2 = clAmsPeSGFindSUForStandbyAssignmentCustom(sg, &su, si);

                if ( rc2 != CL_OK )
                {
                    break;
                }
            }

            if( (lastSI == si) && (lastSU == su) )
            {
                AMS_LOG(CL_LOG_SEV_ERROR, 
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

                        scannedSIList = (ClAmsSIT**) clHeapRealloc(scannedSIList, numSIs * sizeof(*scannedSIList));
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

ClRcT clAmsCustomAssignmentIterInit(ClAmsCustomAssignmentIterT *iter, ClAmsSIT *si)
{
    SaNameT customAssignmentKey = { sizeof(CUSTOM_ASSIGNMENT_KEY) -1,  CUSTOM_ASSIGNMENT_KEY };
    ClRcT rc;
    ClUint8T *customAssignmentBuffer = NULL;
    ClUint32T customAssignmentBufferLen = 0;
    ClBufferHandleT inMsgHandle = 0;
    memset(iter, 0, sizeof(*iter));
    rc = _clAmsEntityUserDataGetKey(&si->config.entity.name, &customAssignmentKey,
                                    (ClCharT**)&customAssignmentBuffer, &customAssignmentBufferLen);
    if(rc != CL_OK)
        goto out;
    rc = clBufferCreate(&inMsgHandle);
    if(rc != CL_OK)
        goto out_free;
    rc = clBufferNBytesWrite(inMsgHandle, customAssignmentBuffer, customAssignmentBufferLen);
    if(rc != CL_OK)
        goto out_free;
    rc = VDECL_VER(clXdrUnmarshallClAmsSISURefBufferT, 4, 0, 0)(inMsgHandle,
                                                                &iter->buffer);
    if(rc != CL_OK)
        goto out_free;
    if(!iter->buffer.count)
    {
        if(iter->buffer.entityRef)
        {
            clHeapFree(iter->buffer.entityRef);
            iter->buffer.entityRef =  NULL;
        }
        goto out_free;
    }

    iter->si = si;

    out_free:
    if(customAssignmentBuffer)
        clHeapFree(customAssignmentBuffer);
    clBufferDelete(&inMsgHandle);

    out:
    return rc;
}

ClAmsSISURefT *clAmsCustomAssignmentIterNext(ClAmsCustomAssignmentIterT *iter)
{
    ClAmsSISURefT *ref = NULL;
    ClRcT rc = CL_OK;
    if(iter->curIndex >= iter->buffer.count)
        return NULL;
    ref = &iter->buffer.entityRef[iter->curIndex++];
    if(!ref->entityRef.ptr)
    {
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                     &ref->entityRef);
        if(rc != CL_OK)
        {
            --iter->curIndex;
            return NULL;
        }
    }
    return ref;
}

void clAmsCustomAssignmentIterEnd(ClAmsCustomAssignmentIterT *iter)
{
    if(iter->buffer.entityRef)
    {
        clHeapFree(iter->buffer.entityRef);
    }
    memset(iter, 0, sizeof(*iter));
}

ClRcT clAmsPeDequeueAssignmentCustom(ClAmsSIT *si, ClAmsSUT *activeSU)
{
    SaNameT customAssignmentKey = { sizeof(CUSTOM_ASSIGNMENT_KEY)-1, CUSTOM_ASSIGNMENT_KEY };
    ClUint8T *customAssignmentBuffer = NULL;
    ClUint32T customAssignmentBufferLen = 0;
    ClAmsSISURefBufferT buffer = {0};
    ClBufferHandleT inMsgHdl = 0;
    ClUint32T i;
    ClRcT rc;

    rc = clBufferCreate(&inMsgHdl);
    if(rc != CL_OK)
        goto out;

    rc = _clAmsEntityUserDataGetKey(&si->config.entity.name, &customAssignmentKey,
                                    (ClCharT**)&customAssignmentBuffer, &customAssignmentBufferLen);
    if(rc == CL_OK)
    {
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T*)customAssignmentBuffer, customAssignmentBufferLen);
        if(rc != CL_OK)
            goto out_free;
        rc = VDECL_VER(clXdrUnmarshallClAmsSISURefBufferT, 4, 0, 0)(inMsgHdl, &buffer);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Unmarshalling entity buffer for SI [%s] returned with [%#x]",
                       si->config.entity.name.value, rc);
            goto out_free;
        }
        clBufferClear(inMsgHdl);
        /*
         * Check if the su is already part of the queue.
         */
        for(i = 0; i < buffer.count; ++i)
        {
            if (!strncmp((const ClCharT*) buffer.entityRef[i].entityRef.entity.name.value,
                            (const ClCharT*) activeSU->config.entity.name.value, activeSU->config.entity.name.length))
            {
                clLogNotice("CUSTOM", "ASSIGNMENT", "Dequeuing SU [%s] from SI [%s]", activeSU->config.entity.name.value,
                                si->config.entity.name.value);
                break;
            }
        }
        if(i < buffer.count)
        {
            /*
             * I would ideally like to just zero off the deleting index and live with 
             * sparse arrays, but a memmove and memcpy won't hurt for small footprints.
             */
            ClAmsSISURefT *newRef = NULL;
            --buffer.count;
            if(buffer.count > 0)
            {
                newRef = (ClAmsSISURefT*) clHeapCalloc(buffer.count, sizeof(*buffer.entityRef));
                CL_ASSERT(newRef != NULL);
                if(i != buffer.count)
                {
                    memmove(buffer.entityRef + i, 
                            buffer.entityRef + i + 1,
                            sizeof(*buffer.entityRef) * ( buffer.count - i) );
                }
                memcpy(newRef, buffer.entityRef, buffer.count * sizeof(*buffer.entityRef));
            }
            if(buffer.entityRef)
                clHeapFree(buffer.entityRef);
            buffer.entityRef = newRef;
        }
        else rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    
    if(rc != CL_OK)
        goto out_free;

    if(buffer.entityRef)
    {
        if(customAssignmentBuffer)
        {
            clHeapFree(customAssignmentBuffer);
            customAssignmentBuffer = NULL;
        }
        rc = VDECL_VER(clXdrMarshallClAmsSISURefBufferT, 4, 0, 0)(
                                                                  (void*)&buffer, inMsgHdl, 0);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Marshalling custom assignment buffer for si [%s], "
                       "returned with [%#x]", si->config.entity.name.value, rc);
            goto out_free;
        }
        rc = clBufferLengthGet(inMsgHdl, &customAssignmentBufferLen);
        if(rc != CL_OK)
            goto out_free;

        rc = clBufferFlatten(inMsgHdl, &customAssignmentBuffer);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Custom assignment buffer flatten for si [%s] "
                       "returned with [%#x]", si->config.entity.name.value, rc);
            goto out_free;
        }
        rc = _clAmsEntityUserDataSetKey(&si->config.entity.name,
                                        &customAssignmentKey, (ClCharT*)customAssignmentBuffer, 
                                        customAssignmentBufferLen);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Custom assignment data set for SI [%s] returned with [%#x]",
                       si->config.entity.name.value, rc);
            goto out_free;
        }
        customAssignmentBuffer = NULL;
    }
    else
    {
        /*
         * Delete the key.
         */
        rc = _clAmsEntityUserDataDeleteKey(&si->config.entity.name, &customAssignmentKey);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Custom assignment key delete for SI [%s] failed with [%#x]",
                       si->config.entity.name.value, rc);
            goto out_free;
        }
    }

    out_free:
    if(inMsgHdl)
        clBufferDelete(&inMsgHdl);
    if(buffer.entityRef) 
        clHeapFree(buffer.entityRef);
    if(customAssignmentBuffer)
        clHeapFree(customAssignmentBuffer);

    out:
    return rc;
}

ClRcT clAmsPeEnqueueAssignmentCustom(ClAmsSIT *si, ClAmsSUT *activeSU, ClAmsHAStateT haState)
{
    SaNameT customAssignmentKey = {  sizeof(CUSTOM_ASSIGNMENT_KEY)-1, CUSTOM_ASSIGNMENT_KEY };
    ClUint8T *customAssignmentBuffer = NULL;
    ClUint32T customAssignmentBufferLen = 0;
    ClAmsSISURefBufferT buffer = {0};
    ClBufferHandleT inMsgHdl = 0;
    ClUint32T i;
    ClRcT rc;

    rc = clBufferCreate(&inMsgHdl);
    if(rc != CL_OK)
        goto out;

    rc = _clAmsEntityUserDataGetKey(&si->config.entity.name, &customAssignmentKey,
                                    (ClCharT**)&customAssignmentBuffer, &customAssignmentBufferLen);
    if(rc == CL_OK)
    {
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T*)customAssignmentBuffer, customAssignmentBufferLen);
        if(rc != CL_OK)
            goto out_free;
        rc = VDECL_VER(clXdrUnmarshallClAmsSISURefBufferT, 4, 0, 0)(inMsgHdl, &buffer);
        if(rc != CL_OK)
        {
            clLogError("CUSTOM", "ASSIGNMENT", "Unmarshalling entity buffer for SI [%s] returned with [%#x]",
                       si->config.entity.name.value, rc);
            goto out_free;
        }
        clBufferClear(inMsgHdl);
        /*
         * Check if the su is already part of the queue.
         */
        for(i = 0; i < buffer.count; ++i)
        {
            if (!strncmp((const ClCharT*) buffer.entityRef[i].entityRef.entity.name.value,
                            (const ClCharT*) activeSU->config.entity.name.value, activeSU->config.entity.name.length))
            {
                clLogNotice("CUSTOM", "ASSIGNMENT", "Skipping enqueueing SU [%s] to SI [%s] as its already enqueued",
                                activeSU->config.entity.name.value, si->config.entity.name.value);
                goto out_free;
            }
        }
        /*
         * Reallocate the entity buffer to add the new one.
         */
        buffer.entityRef = (ClAmsSISURefT*) clHeapRealloc(buffer.entityRef, sizeof(*buffer.entityRef) * (buffer.count+1));
        CL_ASSERT(buffer.entityRef != NULL);
        memset(buffer.entityRef + buffer.count, 0, sizeof(*buffer.entityRef));
        memcpy(&buffer.entityRef[buffer.count].entityRef.entity, &activeSU->config.entity, 
               sizeof(buffer.entityRef[buffer.count].entityRef.entity));
        buffer.entityRef[buffer.count].haState = haState;
        ++buffer.count;
    }
    else
    {
        buffer.entityRef = (ClAmsSISURefT*) clHeapCalloc(1, sizeof(*buffer.entityRef));
        CL_ASSERT(buffer.entityRef != NULL);
        memcpy(&buffer.entityRef->entityRef.entity, &activeSU->config.entity, 
               sizeof(buffer.entityRef->entityRef.entity));
        buffer.entityRef->haState = haState;
        buffer.count = 1;
    }

    clLogNotice("CUSTOM", "ASSIGNMENT", "Enqueued SU [%s] to SI [%s] with ha state [%s]. Queued entities [%d]",
                activeSU->config.entity.name.value, 
                si->config.entity.name.value,
                CL_AMS_STRING_H_STATE(haState), buffer.count);

    rc = VDECL_VER(clXdrMarshallClAmsSISURefBufferT, 4, 0, 0)(
                                                              (void*)&buffer, inMsgHdl, 0);
    if(rc != CL_OK)
    {
        clLogError("CUSTOM", "ASSIGNMENT", "Marshalling custom assignment preference for hastate [%s], "
                   "entity [%s] returned with [%#x]", CL_AMS_STRING_H_STATE(haState),
                   activeSU->config.entity.name.value, rc);
        goto out_free;
    }
    if(customAssignmentBuffer)
    {
        clHeapFree(customAssignmentBuffer);
        customAssignmentBuffer = NULL;
        customAssignmentBufferLen = 0;
    }
    rc = clBufferLengthGet(inMsgHdl, &customAssignmentBufferLen);
    if(rc != CL_OK)
        goto out_free;

    rc = clBufferFlatten(inMsgHdl, &customAssignmentBuffer);
    if(rc != CL_OK)
    {
        clLogError("CUSTOM", "ASSIGNMENT", "Custom assignment buffer flatten for entity [%s], haState [%s] "
                   "returned with [%#x]", activeSU->config.entity.name.value, 
                   CL_AMS_STRING_H_STATE(haState), rc);
        goto out_free;
    }
    rc = _clAmsEntityUserDataSetKey(&si->config.entity.name,
                                    &customAssignmentKey, (ClCharT*)customAssignmentBuffer, customAssignmentBufferLen);
    if(rc != CL_OK)
    {
        clLogError("CUSTOM", "ASSIGNMENT", "Custom assignment data set for SU [%s] with hastate [%s] "
                   "returned with [%#x]", activeSU->config.entity.name.value, 
                   CL_AMS_STRING_H_STATE(haState), rc);
        goto out_free;
    }
    customAssignmentBuffer = NULL;

    out_free:
    if(inMsgHdl)
        clBufferDelete(&inMsgHdl);
    if(buffer.entityRef) 
        clHeapFree(buffer.entityRef);
    if(customAssignmentBuffer)
        clHeapFree(customAssignmentBuffer);

    out:
    return rc;
}

/*
 * From the SI, get the SU having haState.
 */
static ClRcT clAmsPeSISUHAStateCustom(ClAmsSIT *si, ClAmsHAStateT haState, ClAmsSUT **targetSU)
{
    ClAmsEntityRefT *ref = NULL;

    *targetSU = NULL;
    for(ref = clAmsEntityListGetFirst(&si->status.suList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&si->status.suList, ref))
    {
        ClAmsSISURefT *siSURef = (ClAmsSISURefT*)ref;
        ClAmsSUT *su = (ClAmsSUT*)siSURef->entityRef.ptr;
        ClAmsEntityRefT *compRef = NULL;

        for(compRef = clAmsEntityListGetFirst(&su->config.compList);
            compRef != NULL;
            compRef = clAmsEntityListGetNext(&su->config.compList, compRef))
        {
            ClAmsCompT *comp = (ClAmsCompT*)compRef->ptr;
            ClAmsEntityRefT *csiRef = NULL;
            for(csiRef = clAmsEntityListGetFirst(&comp->status.csiList);
                csiRef != NULL;
                csiRef = clAmsEntityListGetNext(&comp->status.csiList, csiRef))
            {
                ClAmsCompCSIRefT *compCSIRef = (ClAmsCompCSIRefT*)csiRef;
                ClAmsCSIT *csi = (ClAmsCSIT*)compCSIRef->entityRef.ptr;
                ClAmsSIT *targetSI = (ClAmsSIT*)csi->config.parentSI.ptr;

                if(targetSI == si && compCSIRef->haState == haState)
                {
                    *targetSU = su;
                    return CL_OK;
                }
            }
        }
    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

static ClRcT clAmsPeSUSIHAStateCustom(ClAmsSIT *si, ClAmsSUT *su, ClAmsHAStateT *haState)
{
    ClAmsEntityRefT *ref = NULL;

    if(*haState != CL_AMS_HA_STATE_NONE) return CL_OK;

    for(ref = clAmsEntityListGetFirst(&su->config.compList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&su->config.compList, ref))
    {
        ClAmsCompT *comp = (ClAmsCompT*)ref->ptr;
        ClAmsCompCSIRefT *csiRef = NULL;
        for(csiRef = (ClAmsCompCSIRefT*)clAmsEntityListGetFirst(&comp->status.csiList);
            csiRef != NULL;
            csiRef = (ClAmsCompCSIRefT*)clAmsEntityListGetNext(&comp->status.csiList, 
                                                               (ClAmsEntityRefT*)csiRef))
        {
            ClAmsCSIT *csi = (ClAmsCSIT*)csiRef->entityRef.ptr;
            ClAmsSIT *parentSI = (ClAmsSIT*)csi->config.parentSI.ptr;
            if(parentSI == si)
            {
                *haState = csiRef->haState;
                break;
            }
        }
    }

    return CL_OK;
}

ClRcT clAmsPeSIAssignSUCustom(ClAmsSIT *si, ClAmsSUT *activeSU, ClAmsSUT *standbySU)
{
    ClAmsSGT *sg = NULL;
    ClRcT rc = CL_OK;
    ClInt32T i;
    ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;
    struct ClAmsSUSIAssignmentMap
    {
        ClAmsSUT *su;
        ClAmsHAStateT haState;
        ClAmsHAStateT currentHAState;
    } suSIAssignmentMap[2] = {
        {
             activeSU,
             CL_AMS_HA_STATE_ACTIVE,
             CL_AMS_HA_STATE_NONE,
        },
        {
            standbySU,
            CL_AMS_HA_STATE_STANDBY,
            CL_AMS_HA_STATE_NONE,
        },
    };

    sg = (ClAmsSGT*)si->config.parentSG.ptr;
    if(!sg)
      {
         clLogError("CUSTOM", "SI-ASSIGN-SU", "Misconfiguration: SI [%.*s] is not connected to a SG.", si->config.entity.name.length-1,si->config.entity.name.value);
         goto out;
      }    
    if(sg->config.redundancyModel != CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM)
    {
        clLogError("CUSTOM", "SI-ASSIGN-SU", "SG redundancy mode is invalid [%d]", sg->config.redundancyModel);
        goto out;
    }

    clAmsPeSIComputeAdminState(si, &adminState);
    
    if(activeSU == NULL)
    {
        if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED)
        {
            rc = clAmsPeSISUHAStateCustom(si, CL_AMS_HA_STATE_ACTIVE, &suSIAssignmentMap[0].su);
            if(rc != CL_OK)
            {
                rc = CL_OK;
                clLogWarning("CUSTOM", "SI-ASSIGN-SU", "No active SU found for SI [%s]. Skipping active SU remove", si->config.entity.name.value);
            }
            else
            {
                activeSU = suSIAssignmentMap[0].su;
                suSIAssignmentMap[0].haState = CL_AMS_HA_STATE_NONE; /*mark for removal*/
                suSIAssignmentMap[0].currentHAState = CL_AMS_HA_STATE_ACTIVE;
            }
        }
    }

    if(standbySU == NULL)
    {
        if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED)
        {
            rc = clAmsPeSISUHAStateCustom(si, CL_AMS_HA_STATE_STANDBY, &suSIAssignmentMap[1].su);
            if(rc != CL_OK)
            {
                rc = CL_OK;
                clLogWarning("CUSTOM", "SI-ASSIGN-SU", "No standby SU found for SI [%s]. "
                             "Skipping standby SU remove", si->config.entity.name.value);
            }
            else
            {
                standbySU = suSIAssignmentMap[1].su;
                suSIAssignmentMap[1].haState = CL_AMS_HA_STATE_NONE;
                suSIAssignmentMap[1].currentHAState = CL_AMS_HA_STATE_STANDBY;
            }
        }
    }

    if (suSIAssignmentMap[0].su) clAmsPeSUSIHAStateCustom(si, suSIAssignmentMap[0].su,  &suSIAssignmentMap[0].currentHAState);
    if (suSIAssignmentMap[1].su) clAmsPeSUSIHAStateCustom(si, suSIAssignmentMap[1].su,  &suSIAssignmentMap[1].currentHAState);

    if(suSIAssignmentMap[1].currentHAState == CL_AMS_HA_STATE_ACTIVE)
    {
        /*
         * Flip assignment sequence.
         */
        struct ClAmsSUSIAssignmentMap saveMap = {0};
        
        memcpy(&saveMap, &suSIAssignmentMap[0], sizeof(saveMap));

        memcpy(&suSIAssignmentMap[0], &suSIAssignmentMap[1], 
               (ClUint32T)sizeof(suSIAssignmentMap[0]));

        memcpy(&suSIAssignmentMap[1], &saveMap, 
               (ClUint32T)sizeof(suSIAssignmentMap[1]));

    }

    for(i = 0; i < 2; ++i)
    {
        if(!suSIAssignmentMap[i].su)
        {
            clLogWarning("CUSTOM", "SI-ASSIGN-SU", "Skipping assignment index [%d]", i);
            continue;
        }

        if(suSIAssignmentMap[i].haState == CL_AMS_HA_STATE_ACTIVE
           ||
           suSIAssignmentMap[i].haState == CL_AMS_HA_STATE_STANDBY)
        {
            clAmsPeEnqueueAssignmentCustom(si, suSIAssignmentMap[i].su, suSIAssignmentMap[i].haState);
        }

        if(clAmsPeSUIsAssignable(suSIAssignmentMap[i].su) != CL_OK)
        {
            clLogNotice("CUSTOM", "SI-ASSIGN-SU", "SU [%s] is unassignable. "
                        "Deferring SI [%s] assignment to [%s]",
                        suSIAssignmentMap[i].su->config.entity.name.value,
                        si->config.entity.name.value,
                        CL_AMS_STRING_H_STATE(suSIAssignmentMap[i].haState));
            continue;
        }
        /*
         * If the SU is not instantiated, defer assignments.
         */
        if(suSIAssignmentMap[i].su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED)
        {
            clLogNotice("CUSTOM", "SI-ASSIGN-SU", "Deferring SI [%s] assignment to [%s] "
                        "sg [%s] started state [%d], SU presence state [%s]",
                        si->config.entity.name.value,
                        suSIAssignmentMap[i].su->config.entity.name.value,
                        sg->config.entity.name.value, sg->status.isStarted,
                        CL_AMS_STRING_P_STATE(suSIAssignmentMap[i].su->status.presenceState));
            continue;
        }

        if(suSIAssignmentMap[i].currentHAState != suSIAssignmentMap[i].haState)
        {
            const ClCharT *op = "Assigning";
            const ClCharT *target = "to";
            //ClRcT (*csiFn)(ClAmsSUT *su, ClAmsSIT *si, ClUint32T mode); c++ build checks for exact prototypes
            ClAmsHAStateT haState = suSIAssignmentMap[i].haState;

            if(haState == CL_AMS_HA_STATE_NONE)
            {
                target = "from";
                op = "Removing";
                clLogNotice("CUSTOM", "SI-ASSIGN-SU", "%s SI [%s] %s %s SU [%s]", op, si->config.entity.name.value, 
                        haState != CL_AMS_HA_STATE_NONE ? CL_AMS_STRING_H_STATE(haState) : "", target, suSIAssignmentMap[i].su->config.entity.name.value);
                rc = clAmsPeSURemoveSI(suSIAssignmentMap[i].su, si, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
            
#if 0
                if(suSIAssignmentMap[i].currentHAState == CL_AMS_HA_STATE_ACTIVE)
                {
                    csiFn = clAmsPeSUQuiesceSI;
                    op = "Quiescing";
                }
#endif
            }
            else
            {

                clLogNotice("CUSTOM", "SI-ASSIGN-SU", "%s SI [%s] %s %s SU [%s]", op, si->config.entity.name.value, 
                        haState != CL_AMS_HA_STATE_NONE ? CL_AMS_STRING_H_STATE(haState) : "", target, suSIAssignmentMap[i].su->config.entity.name.value);
                 rc = clAmsPeSUAssignSI(suSIAssignmentMap[i].su, si, haState);
            }
            if(rc != CL_OK)
            {
                clLogError("CUSTOM", "SI-ASSIGN-SU", "SU SI [%s] operation returned [%#x]", op, rc);
                goto out;
            }
        }
    }

    out:
    return rc;
}

ClRcT
clAmsPeSGAutoAdjustCustom(ClAmsSGT *sg)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT *eRef = NULL;
    ClUint32T numActiveSUs = 0;
    ClUint32T numStandbySUs = 0;
    ClUint32T numAdjustSUs = 0;
    ClUint32T suPendingInvocations = 0;
    ClBoolT reAdjust = CL_FALSE;
    CL_LIST_HEAD_DECLARE(suActiveAdjustList);
    CL_LIST_HEAD_DECLARE(suStandbyAdjustList);

    if(!sg->config.autoAdjust)
        return CL_OK;

    if(sg->status.adjustTimer.count)
    {
        reAdjust = CL_TRUE;
        clAmsEntityTimerStop((ClAmsEntityT*)sg, CL_AMS_SG_TIMER_ADJUST);
    }

    clLogInfo("SG", "ADJUST", "Adjusting SG [%.*s]",
              sg->config.entity.name.length-1,
              sg->config.entity.name.value);
    
    /*
     * We scan in the normal way looking out for the least preferred actives.
     * and switching them over with more preferred standbys
     */

    /* But first see if I need to defer */
    for(eRef = clAmsEntityListGetFirst(&sg->config.siList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->config.siList, eRef))
    {
        ClAmsSIT *si = (ClAmsSIT*)eRef->ptr;
        ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;
        clAmsPeSIComputeAdminState(si, &adminState);
        if(adminState != CL_AMS_ADMIN_STATE_UNLOCKED
           ||
           si->status.operState == CL_AMS_OPER_STATE_DISABLED)
        {
            clLogInfo("SG", "ADJUST", "SI [%s] is not assignable. Skipping adjustment.",
                      si->config.entity.name.value);
            continue;
        }
        ClAmsCustomAssignmentIterT iter = {0};
        ClAmsSISURefT *siSURef = NULL;
        clAmsCustomAssignmentIterInit(&iter, si);
        while ( (siSURef = clAmsCustomAssignmentIterNext(&iter) ) )
        {
            ClAmsSUT *su = (ClAmsSUT*)siSURef->entityRef.ptr;
            ClAmsHAStateT haState = CL_AMS_HA_STATE_NONE;
            /*
             * Skip SU in the probation time.
             */
            if(su->status.suProbationTimer.count) continue;

            suPendingInvocations = clAmsInvocationsPendingForSU(su);
            
            if(suPendingInvocations)
            {
                /*
                 * Deferring adjustment as we dont want to risk causing unnecessary 
                 * active-standby switches while we are in an intermediate state.
                 */
                clLogInfo("SG", "ADJUST", "SU [%.*s] has pending invocations. "
                          "Deferring SG [%.*s] adjustment by [%d] secs",
                          su->config.entity.name.length-1,
                          su->config.entity.name.value,
                          sg->config.entity.name.length-1,
                          sg->config.entity.name.value,
                          CL_AMS_SG_ADJUST_DURATION/1000);
                clAmsCustomAssignmentIterEnd(&iter);
                goto out_defer;
            }
            clAmsPeSUSIHAStateCustom(si, su, &haState);
            clLogInfo("SG", "ADJUST", "SU [%s] ha state for SI [%s] is [%s]",
                      su->config.entity.name.value, si->config.entity.name.value,
                      CL_AMS_STRING_H_STATE(haState));
            if(siSURef->haState != haState)
            {
                ClAmsSUAdjustCustomT *customEntry = NULL;
                if(haState == CL_AMS_HA_STATE_NONE &&
                   (clAmsPeSUIsAssignable(su) != CL_OK
                    ||
                    su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE))
                {
                    clLogInfo("SG", "ADJUST", "SU [%s] ha state for SI [%s] not assignable",
                              su->config.entity.name.value, si->config.entity.name.value);
                    continue;
                }
                customEntry = (ClAmsSUAdjustCustomT*) clHeapCalloc(1, sizeof(*customEntry));
                CL_ASSERT(customEntry != NULL);
                customEntry->su = su;
                customEntry->si = si;
                customEntry->haState = haState;
                if(haState == CL_AMS_HA_STATE_ACTIVE)
                {
                    clLogInfo("SG", "ADJUST", "Adjustment target SU [%s], SI [%s], haState [%s]",
                              su->config.entity.name.value, si->config.entity.name.value,
                              CL_AMS_STRING_H_STATE(haState));
                    clListAddTail(&customEntry->list, &suActiveAdjustList);
                    ++numActiveSUs;
                }
                else
                {
                    clLogInfo("SG", "ADJUST", "Adjustment target SU [%s], SI [%s] haState [%s]",
                              su->config.entity.name.value, si->config.entity.name.value,
                              CL_AMS_STRING_H_STATE(haState));
                    clListAddTail(&customEntry->list, &suStandbyAdjustList);
                    ++numStandbySUs;
                }
            }
        }
        clAmsCustomAssignmentIterEnd(&iter);
    }
    numAdjustSUs = numActiveSUs + numStandbySUs;
    if(numAdjustSUs 
       && 
       numActiveSUs == numStandbySUs)
    {
        clLogInfo("SG", "ADJUST", "SUs to be adjusted is [%d]", numAdjustSUs);
        reAdjust = CL_TRUE;
        while(!CL_LIST_HEAD_EMPTY(&suStandbyAdjustList))
        {
            ClListHeadT *head = suStandbyAdjustList.pNext;
            ClAmsSUAdjustCustomT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustCustomT, list);
            ClAmsSUT *su = adjustEntry->su;
            ClAmsSIT *si = adjustEntry->si;
            ClAmsHAStateT haState = adjustEntry->haState;
            clListDel(head);
            clHeapFree(adjustEntry);
            if(haState == CL_AMS_HA_STATE_STANDBY
               &&
               !clAmsInvocationsPendingForSU(su))
            {
                clLogInfo("SG", "ADJUST", "Removing standby assignments from SU [%.*s] "
                          "for SI [%.*s]",
                          su->config.entity.name.length-1, su->config.entity.name.value,
                          si->config.entity.name.length-1, si->config.entity.name.value);
                clAmsPeSURemoveSI(su, si, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
            } 
        }
        while(!CL_LIST_HEAD_EMPTY(&suActiveAdjustList))
        {
            ClListHeadT *head = suActiveAdjustList.pNext;
            ClAmsSUAdjustCustomT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustCustomT, list);
            ClAmsSUT *su = adjustEntry->su;
            ClAmsSIT *si = adjustEntry->si;
            clListDel(head);
            clHeapFree(adjustEntry);
            if(!clAmsInvocationsPendingForSU(su))
            {
                clLogInfo("SG", "ADJUST", "Removing active assignments from SU [%.*s] "
                          "for SI [%.*s]",
                          su->config.entity.name.length-1, su->config.entity.name.value,
                          si->config.entity.name.length-1, si->config.entity.name.value);
                clAmsPeSURemoveSI(su, si, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
            }
        }
    }
    rc = CL_OK;

    if(reAdjust)
    {        
        out_defer:  /* Must clean up the lists even if we defer */
        reAdjust = CL_FALSE;
        clLogInfo("SG", "ADJUST", 
                  "Starting SG [%.*s] adjust timer of [%d] secs to readjust SG in a clean state",
                  sg->config.entity.name.length-1,
                  sg->config.entity.name.value,
                  CL_AMS_SG_ADJUST_DURATION/1000);
        clAmsEntityTimerStart((ClAmsEntityT*)sg,
                              CL_AMS_SG_TIMER_ADJUST);
    }
    
    while(!CL_LIST_HEAD_EMPTY(&suActiveAdjustList))
    {
        ClListHeadT *head = suActiveAdjustList.pNext;
        ClAmsSUAdjustCustomT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustCustomT, list);
        clListDel(head);
        clHeapFree(adjustEntry);
    }

    while(!CL_LIST_HEAD_EMPTY(&suStandbyAdjustList))
    {
        ClListHeadT *head = suStandbyAdjustList.pNext;
        ClAmsSUAdjustCustomT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustCustomT, list);
        clListDel(head);
        clHeapFree(adjustEntry);
    }
    
    clLogInfo("SG", "ADJUST", "Done Adjusting SG [%.*s]",
              sg->config.entity.name.length-1,
              sg->config.entity.name.value);
   
    return rc;

}
