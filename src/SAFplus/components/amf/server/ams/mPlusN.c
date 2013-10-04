#include <mPlusN.h>
/*
 * Try distributing least sis per SU.
*/

static ClRcT
clAmsPeSGDefaultLoadingStrategyActiveMPlusN(ClAmsSGT *sg, 
                                            ClAmsSUT **su,
                                            ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef = NULL;
    ClUint32T leastLoad = 0;

    /*
     * first check if a spare SU can be assigned.
     */
    if ( sg->status.numCurrActiveSUs < clAmsPeSGComputeMaxActiveSU(sg) )
    {
        for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
              eRef != (ClAmsEntityRefT *) NULL;
              eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
        {
            ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
            
            if(tmpSU->status.numQuiescedSIs) continue;
            
            if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
            {
                *su = tmpSU;

                return CL_OK;
            }
        }
    }

    /*
     * A spare SU could not be found or cannot be assigned as there 
     * are already sufficient active SUs. So search for a slot in 
     * the currently assigned active SUs.
     */

    leastLoad = sg->config.maxActiveSIsPerSU;

    for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
    {
        ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

        if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
        {
            continue;
        }
        
        /*
         * Check for assignable since this SU could be in the middle of a node exit
         */
        if ( clAmsPeSUIsAssignable(tmpSU) != CL_OK)
        {
            continue;
        }

        if( clAmsPeCheckSUReassignOp(tmpSU, si, CL_TRUE) 
            &&
            tmpSU->status.numActiveSIs < leastLoad)
        {
            *su = tmpSU;
            return CL_OK;
        }

        if ( tmpSU->status.numStandbySIs || tmpSU->status.numQuiescedSIs)
        {
            continue;
        }

        if ( tmpSU->status.numActiveSIs < leastLoad )
        {
            leastLoad = tmpSU->status.numActiveSIs;
            *su = tmpSU;
        }
    }

    if ( *su )
        return CL_OK;

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * Primary goal is to ensure that the least number of standby
 * SUs are brought into service if a SU with active assignments
 * fails. Thus if two SIs are assigned as active to an SU, then
 * it is preferred that they are both assigned standby to the
 * same SU if possible. The logic below tries to ensure this.
*/

static ClRcT
clAmsPeSGDefaultLoadingStrategyStandbyMPlusN(ClAmsSGT *sg,
                                             ClAmsSUT **su,
                                             ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef = NULL;
    ClAmsEntityRefT *eRef1 = NULL;
    ClAmsEntityRefT *eRef2 = NULL;
    ClAmsSUT *activeSU = NULL;

    /*
     * Step 1 is to find the SU to which this SI is assigned active
     */
    for ( eRef = clAmsEntityListGetFirst(&si->status.suList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(&si->status.suList, eRef) )
    {
        ClAmsSISURefT *activeSURef = (ClAmsSISURefT *) eRef;

        if ( activeSURef->haState == CL_AMS_HA_STATE_ACTIVE
             ||
             activeSURef->haState == CL_AMS_HA_STATE_QUIESCING)
        {
            AMS_CHECK_SU ( activeSU = (ClAmsSUT *) eRef->ptr );

            break;
        }
    }

    if(NULL == activeSU)
    {
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    /*
     * Now if activeSU is found (and no reason why not), then step 2 is
     * to traverse the list of other SIs assigned to it and find out if
     * they are assigned as standbys to any SUs. These SUs are preferred
     * because of the 'least impact on failure policy'.
     */

    for ( eRef1 = clAmsEntityListGetFirst(&activeSU->status.siList);
          eRef1 != (ClAmsEntityRefT *) NULL;
          eRef1 = clAmsEntityListGetNext(&activeSU->status.siList, eRef1))
    {
        ClAmsSUSIRefT *otherSIRef = (ClAmsSUSIRefT *) eRef1;
        ClAmsSIT *otherSI = (ClAmsSIT *) eRef1->ptr;

        if ( otherSI == si )
        {
            continue;
        }

        if ( otherSIRef->haState != CL_AMS_HA_STATE_ACTIVE )
        {
            continue;
        }

        /*
         * Check if any of the SUs that are already being used for other
         * SIs are usable.
         */

        for ( eRef2 = clAmsEntityListGetFirst(&otherSI->status.suList);
              eRef2 != (ClAmsEntityRefT *) NULL;
              eRef2 = clAmsEntityListGetNext(&otherSI->status.suList,eRef2))
        {
            ClAmsSUT *otherSU = (ClAmsSUT *) eRef2->ptr;

            if ( otherSU->status.readinessState != 
                 CL_AMS_READINESS_STATE_INSERVICE )
            {
                continue;
            }

            if ( otherSU->status.numActiveSIs ||
                 otherSU->status.numQuiescedSIs)
            {
                continue;
            }

            /*
             * The standby SU could be in the middle of a node switchover
             * with one of the SIs removed.
             */
            if( clAmsPeSUIsAssignable(otherSU) != CL_OK)
            {
                continue;
            }

            /*
             * Check if this standby SU is having a remove pending against other standby SIs
             */
            if ( clAmsEntityOpPending((ClAmsEntityT*)otherSU, 
                                      &otherSU->status.entity, 
                                      CL_AMS_ENTITY_OP_REMOVES_MPLUSN |
                                      CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN))
            {
                continue;
            }

            if ( clAmsPeSGColocationCheckFails(sg, activeSU, otherSU) )
            {
                continue;
            }

            if ( otherSU->status.numStandbySIs < 
                 sg->config.maxStandbySIsPerSU )
            {
                *su = otherSU;

                return CL_OK;
            }
        }
    }

    /*
     * If the logic falls to here, then it means that no SU was found that 
     * was a candidate for the 'least impact on failure' policy. This can 
     * also happen when the first standby assignment is made for a SI.
     *
     * Now as per loading strategy, check if a spare SU can be assigned.
     */

    if ( sg->status.numCurrStandbySUs < clAmsPeSGComputeMaxStandbySU(sg) )
    {
        for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
              eRef != (ClAmsEntityRefT *) NULL;
              eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
        {
            ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

            if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
            {
                continue;
            }
                    
            if(tmpSU->status.numQuiescedSIs)
            {
                continue;
            }

            if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
            {
                *su = tmpSU;

                return CL_OK;
            }
        }
    }

    /*
     * A spare SU could not be found or cannot be assigned as there 
     * are already sufficient standby SUs. So search for a slot in 
     * the currently assigned standby SUs.
     */

    ClUint32T leastLoad;

    leastLoad = sg->config.maxStandbySIsPerSU;

    for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
    {
        ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

        if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
        {
            continue;
        }

        if ( tmpSU->status.numActiveSIs || tmpSU->status.numQuiescedSIs)
        {
            continue;
        }

        /*
         * The standby SU could be in the middle of a node switchover
         * with one of the SIs removed.
         */

        if( clAmsPeSUIsAssignable(tmpSU) != CL_OK)
        {
            continue;
        }

        /*
         * Check if this standby SU is having a remove pending against other standby SIs
         */
        if ( clAmsEntityOpPending((ClAmsEntityT*)tmpSU,
                                  &tmpSU->status.entity, 
                                  CL_AMS_ENTITY_OP_REMOVES_MPLUSN | 
                                  CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
        {
            continue;
        }

        if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
        {
            continue;
        }

        if ( tmpSU->status.numStandbySIs < leastLoad )
        {
            leastLoad = tmpSU->status.numStandbySIs;
            *su = tmpSU;
        }
    }
    if ( *su )
        return CL_OK;

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * clAmsPeSGFindSUForActiveAssignmentMPlusN
 * ----------------------------------------
 * This fn finds the next preferred SU that can be given an active SI
 * assignment. The selection of SU is based on the loading strategy
 * as well as the redundancy model. This fn only handles the MPlusN
 * model.
 *
 * Note, support for multiple loading strategies is an enhancement over
 * SAF. The loading strategy implicitly mentioned in the AMF B.1.1 spec
 * is least_si_per_su.
 */
ClRcT
clAmsPeSGFindSUForActiveAssignmentMPlusN(
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
         * This strategy maximizes the use of available SUs and assigns
         * the least load to each SU.
         */

        case CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU:
        {
            return clAmsPeSGDefaultLoadingStrategyActiveMPlusN(sg, su, si);
        }

        /*
         * This strategy minimizes the number of SUs assigned at the cost
         * of loading each SU to its limit. It is useful when the cost of 
         * creating another SU is more than the incremental cost of assigning 
         * another workload to an existing SU, such as when both SUs are on 
         * the same node.
         */

        case CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED:
        {
            ClUint32T leastLoad;

            /*
             * First, go thru the assigned SU list and see if there are any
             * open slots for active assignment.
             */

            leastLoad = sg->config.maxActiveSIsPerSU;

            for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    continue;
                }

                /*
                 * Check for assignable since this SU could be in the middle of a node exit
                 */
                if ( clAmsPeSUIsAssignable(tmpSU) != CL_OK)
                {
                    continue;
                }
                
                if(clAmsPeCheckSUReassignOp(tmpSU, si, CL_TRUE)
                   &&
                   tmpSU->status.numActiveSIs < leastLoad)
                {
                    *su = tmpSU;
                    return CL_OK;
                }

                if ( tmpSU->status.numStandbySIs || tmpSU->status.numQuiescedSIs )
                {
                    continue;
                }

                if ( tmpSU->status.numActiveSIs < leastLoad )
                {
                    leastLoad = tmpSU->status.numActiveSIs;
                    *su = tmpSU;
                }
            }

            if ( *su )
                return CL_OK;
            
            /*
             * If SU was not found, check if a spare SU can be assigned.
             */

            if ( sg->status.numCurrActiveSUs < clAmsPeSGComputeMaxActiveSU(sg) )
            {
                for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
                      eRef != (ClAmsEntityRefT *) NULL;
                      eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
                {
                    ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
                    ClAmsNodeT *node;

                    AMS_CHECK_NODE(node = (ClAmsNodeT*)tmpSU->config.parentNode.ptr);

                    if(tmpSU->status.numQuiescedSIs) continue;

                    if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
                    {
                        *su = tmpSU;

                        return CL_OK;
                    }
                }
            }

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

        /*
         * This loading strategy picks the SU with the least "load". The
         * definition of load can be system specific and obtained through
         * a callout. The algorithm could also choose to compute the load
         * on a node (either as system load or load of all SIs on node)
         * when deciding which SU to select. Load is in the range of 0..100.
         */

        case CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU:
        {
            ClUint32T leastLoad;
            ClUint32T suLoad;

            /*
             * Allow a new SU to be assigned only if so permitted. Then find the SU
             * with the least load among the SUs in the inservicespare and assigned
             * SU lists.
             */

            leastLoad = 100;

            if ( sg->status.numCurrActiveSUs < clAmsPeSGComputeMaxActiveSU(sg) )
            {
                for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
                      eRef != (ClAmsEntityRefT *) NULL;
                      eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
                {
                    ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
                    
                    if(tmpSU->status.numQuiescedSIs) continue;
                    
                    if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
                    {
                        suLoad = clAmsPeSUComputeLoad(tmpSU);

                        if ( suLoad < leastLoad )
                        {
                            leastLoad = suLoad;
                            *su = tmpSU;
                        }
                    }
                }
            }

            for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    continue;
                }

                /*
                 * Check for assignable since this SU could be in the middle of a node exit
                 */
                if ( clAmsPeSUIsAssignable(tmpSU) != CL_OK)
                {
                    continue;
                }

                if(clAmsPeCheckSUReassignOp(tmpSU, si, CL_TRUE))
                {
                    *su = tmpSU;
                    return CL_OK;
                }

                if ( tmpSU->status.numStandbySIs || tmpSU->status.numQuiescedSIs)
                {
                    continue;
                }

                suLoad = clAmsPeSUComputeLoad(tmpSU);

                if ( suLoad < leastLoad )
                {
                    leastLoad = suLoad;
                    *su = tmpSU;
                }
            }

            if ( *su )
                return CL_OK;

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

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

                tmpSU->status.numDelayAssignments = 0;

                if(tmpSU->status.suAssignmentTimer.count > 0)
                {
                    clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                         CL_AMS_SU_TIMER_ASSIGNMENT);
                }

                if ( tmpSU->status.numStandbySIs || tmpSU->status.numQuiescedSIs)
                {
                    continue;
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

            return clAmsPeSGDefaultLoadingStrategyActiveMPlusN(sg, su, si);
        }

        /*
         * This keeps the option open that a system designer may choose
         * any other scheme which is invoked here via a callout.
         */

        case CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED:
        {
            // Future
        }

        default:
        {
            AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);

        }
    }
}

/*
 * clAmsPeSGFindSUForStandbyAssignmentMPlusN
 * -----------------------------------------
 * This fn finds the next preferred SU that can be given a standby SI
 * assignment. The selection of SU is based on the loading strategy as 
 * well as the redundancy model and the assignments of other SI assigned 
 * to a SU. This fn only handles the MPlusN model.
 *
 * The loading strategies are used in a slightly different way for standby
 * assignments than for active assignments. When standby assignments are 
 * made, the primary goal is to ensure that the least number of standby 
 * SUs will be brought into service when an active SU fails. Otherwise a 
 * single active SU failure can bring more than one standby SU active thus 
 * reducing the benefit of having multiple standby SUs. This "least SUs 
 * impacted" rule overrides the loading strategy, except in the case of 
 * the "by si preference" approach, where it is assumed that the designer
 * knows what they are doing when defining the SI preferences.
 */
ClRcT
clAmsPeSGFindSUForStandbyAssignmentMPlusN(
        CL_IN ClAmsSGT *sg,
        CL_IN ClAmsSUT **su,
        CL_IN ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef, *eRef1, *eRef2;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !su );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    *su = (ClAmsSUT *) NULL;

    switch ( sg->config.loadingStrategy )
    {
        case CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU:
        {
            return clAmsPeSGDefaultLoadingStrategyStandbyMPlusN(sg, su, si);
        }

        /*
         * This strategy minimizes the number of SUs assigned at the cost
         * of loading each SU to its limit. This strategy is useful when
         * the cost of creating another SU is more than the incremental
         * cost of assigning another workload to an existing SU, such as
         * when both SUs are on the same node.
         */

        case CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED:
        {
            /*
             * primary goal is to ensure that the least number of standby
             * SUs are brought into service if a SU with active assignments
             * fails. Thus if two SIs are assigned as active to an SU, then
             * it is preferred that they are both assigned standby to the
             * same SU if possible. The logic below tries to ensure this.
             */

            /*
             * Step 1 is to find the SU to which this SI is assigned active
             */

            ClAmsSUT *activeSU = NULL;

            for ( eRef = clAmsEntityListGetFirst(&si->status.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&si->status.suList, eRef) )
            {
                ClAmsSISURefT *activeSURef = (ClAmsSISURefT *) eRef;

                if ( activeSURef->haState == CL_AMS_HA_STATE_ACTIVE 
                     ||
                     activeSURef->haState == CL_AMS_HA_STATE_QUIESCING)
                {
                    AMS_CHECK_SU ( activeSU = (ClAmsSUT *) eRef->ptr );

                    break;
                }
            }

            if(NULL == activeSU)
            {
                return CL_AMS_RC(CL_ERR_NOT_EXIST);
            }

            /*
             * Now if activeSU is found (and no reason why not), then step 2 is
             * to traverse the list of other SIs assigned to it and find out if
             * they are assigned as standbys to any SUs. These SUs are preferred
             * because of the 'least impact on failure policy'.
             */
            for ( eRef1 = clAmsEntityListGetFirst(&activeSU->status.siList);
                  eRef1 != (ClAmsEntityRefT *) NULL;
                  eRef1 = clAmsEntityListGetNext(&activeSU->status.siList, eRef1))
            {
                ClAmsSUSIRefT *otherSIRef = (ClAmsSUSIRefT *) eRef1;
                ClAmsSIT *otherSI = (ClAmsSIT *) eRef1->ptr;

                if ( otherSI == si )
                {
                    continue;
                }

                if ( otherSIRef->haState != CL_AMS_HA_STATE_ACTIVE )
                {
                    continue;
                }

                /*
                 * Check if any of the SUs that are already being used for other
                 * SIs are usable.
                 */

                for ( eRef2 = clAmsEntityListGetFirst(&otherSI->status.suList);
                      eRef2 != (ClAmsEntityRefT *) NULL;
                      eRef2 = clAmsEntityListGetNext(&otherSI->status.suList,eRef2))
                {
                    ClAmsSUT *otherSU = (ClAmsSUT *) eRef2->ptr;

                    if ( otherSU->status.readinessState != 
                         CL_AMS_READINESS_STATE_INSERVICE )
                    {
                        continue;
                    }

                    if ( otherSU->status.numActiveSIs || otherSU->status.numQuiescedSIs)
                    {
                        continue;
                    }
                    
                    /*
                     * Check if this standby SU is having a remove pending against other standby SIs
                     */
                    if ( clAmsEntityOpPending((ClAmsEntityT*)otherSU, 
                                              &otherSU->status.entity, 
                                              CL_AMS_ENTITY_OP_REMOVES_MPLUSN |
                                              CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
                    {
                        continue;
                    }

                    if ( clAmsPeSGColocationCheckFails(sg, activeSU, otherSU) )
                    {
                        continue;
                    }

                    if ( otherSU->status.numStandbySIs < 
                         sg->config.maxStandbySIsPerSU )
                    {
                        *su = otherSU;

                        return CL_OK;
                    }
                }
            }

            /*
             * If the logic falls to here, then it means that no SU was found that 
             * was a candidate for the 'least impact on failure' policy. This can 
             * also happen when the first standby assignment is made for an SI
             * previously assigned active.
             *
             * Go thru the assigned SU list and see if there are any open slots for 
             * standby assignment.
             */

            ClUint32T leastLoad;

            leastLoad = sg->config.maxStandbySIsPerSU;

            for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    continue;
                }

                if ( tmpSU->status.numActiveSIs || tmpSU->status.numQuiescedSIs)
                {
                    continue;
                }

                /*
                 * Check if this standby SU is having a remove pending against other standby SIs
                 */
                if ( clAmsEntityOpPending((ClAmsEntityT*)tmpSU, 
                                          &tmpSU->status.entity, 
                                          CL_AMS_ENTITY_OP_REMOVES_MPLUSN |
                                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
                {
                    continue;
                }

                if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
                {
                    continue;
                }

                if ( tmpSU->status.numStandbySIs < leastLoad )
                {
                    leastLoad = tmpSU->status.numStandbySIs;
                    *su = tmpSU;
                }
            }

            if ( *su )
                return CL_OK;

            /*
             * If SU was not found, check if a spare SU can be assigned.
             */

            if ( sg->status.numCurrStandbySUs < clAmsPeSGComputeMaxStandbySU(sg) )
            {
                for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
                      eRef != (ClAmsEntityRefT *) NULL;
                      eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
                {
                    ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                    if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
                    {
                        continue;
                    }
                    
                    if(tmpSU->status.numQuiescedSIs)
                    {
                        continue;
                    }

                    if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
                    {
                        *su = tmpSU;

                        return CL_OK;
                    }
                }
            }

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

        /*
         * This loading strategy picks the SU with the least "load". The
         * definition of load can be system specific and obtained through
         * a callout. The algorithm could also choose to compute the load
         * on a node (either as system load or load of all SIs on node)
         * when deciding which SU to select. Load is in the range of 0..100.
         */

        case CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU:
        {
            /*
             * primary goal is to ensure that the least number of standby
             * SUs are brought into service if a SU with active assignments
             * fails. Thus if two SIs are assigned as active to an SU, then
             * it is preferred that they are both assigned standby to the
             * same SU if possible. The logic below tries to ensure this.
             */

            /*
             * Step 1 is to find the SU to which this SI is assigned active
             */

            ClAmsSUT *activeSU = NULL;

            for ( eRef = clAmsEntityListGetFirst(&si->status.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&si->status.suList, eRef) )
            {
                ClAmsSISURefT *activeSURef = (ClAmsSISURefT *) eRef;

                if ( activeSURef->haState == CL_AMS_HA_STATE_ACTIVE 
                     ||
                     activeSURef->haState == CL_AMS_HA_STATE_QUIESCING)
                {
                    AMS_CHECK_SU ( activeSU = (ClAmsSUT *) eRef->ptr );

                    break;
                }
            }

            if(NULL == activeSU)
            {
                return CL_AMS_RC(CL_ERR_NOT_EXIST);
            }

            /*
             * Now if activeSU is found (and no reason why not), then step 2 is
             * to traverse the list of other SIs assigned to it and find out if
             * they are assigned as standbys to any SUs. These SUs are preferred
             * because of the 'least impact on failure policy'.
             */

            for ( eRef1 = clAmsEntityListGetFirst(&activeSU->status.siList);
                  eRef1 != (ClAmsEntityRefT *) NULL;
                  eRef1 = clAmsEntityListGetNext(&activeSU->status.siList, eRef1))
            {
                ClAmsSUSIRefT *otherSIRef = (ClAmsSUSIRefT *) eRef1;
                ClAmsSIT *otherSI = (ClAmsSIT *) eRef1->ptr;

                if ( otherSI == si )
                {
                    continue;
                }

                if ( otherSIRef->haState != CL_AMS_HA_STATE_ACTIVE )
                {
                    continue;
                }

                /*
                 * Check if any of the SUs that are already being used for other
                 * SIs are usable.
                 */

                for ( eRef2 = clAmsEntityListGetFirst(&otherSI->status.suList);
                      eRef2 != (ClAmsEntityRefT *) NULL;
                      eRef2 = clAmsEntityListGetNext(&otherSI->status.suList,eRef2))
                {
                    ClAmsSUT *otherSU = (ClAmsSUT *) eRef2->ptr;

                    if ( otherSU->status.readinessState != 
                         CL_AMS_READINESS_STATE_INSERVICE )
                    {
                        continue;
                    }

                    if ( otherSU->status.numActiveSIs || otherSU->status.numQuiescedSIs)
                    {
                        continue;
                    }

                    /*
                     * Check if this standby SU is having a remove pending against other standby SIs
                     */
                    if ( clAmsEntityOpPending((ClAmsEntityT*)otherSU, 
                                              &otherSU->status.entity, 
                                              CL_AMS_ENTITY_OP_REMOVES_MPLUSN |
                                              CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
                    {
                        continue;
                    }

                    if ( clAmsPeSGColocationCheckFails(sg, activeSU, otherSU) )
                    {
                        continue;
                    }

                    if ( otherSU->status.numStandbySIs < 
                         sg->config.maxStandbySIsPerSU )
                    {
                        *su = otherSU;

                        return CL_OK;
                    }
                }
            }

            /*
             * If the logic falls to here, then it means that no SU was found that 
             * was a candidate for the 'least impact on failure' policy. This can 
             * also happen when the first standby assignment is made for a SI.
             *
             * Now as per loading strategy, check if a spare SU can be assigned.
             */

            ClUint32T leastLoad;

            leastLoad = 100;

            if ( sg->status.numCurrStandbySUs < clAmsPeSGComputeMaxStandbySU(sg) )
            {
                for ( eRef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
                      eRef != (ClAmsEntityRefT *) NULL;
                      eRef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, eRef) )
                {
                    ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
                    ClUint32T suLoad;

                    if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
                    {
                        continue;
                    }

                    if(tmpSU->status.numQuiescedSIs)
                    {
                        continue;
                    }

                    if ( clAmsPeSUIsAssignable(tmpSU) == CL_OK )
                    {
                        suLoad = clAmsPeSUComputeLoad(tmpSU);

                        if ( suLoad < leastLoad )
                        {
                            leastLoad = suLoad;
                            *su = tmpSU;
                        }
                    }
                }
            }

            for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
                ClUint32T suLoad;

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
                {
                    continue;
                }

                if ( tmpSU->status.numActiveSIs || tmpSU->status.numQuiescedSIs)
                {
                    continue;
                }

                /*
                 * Check if this standby SU is having a remove pending against other standby SIs
                 */
                if ( clAmsEntityOpPending((ClAmsEntityT*)tmpSU, 
                                          &tmpSU->status.entity, 
                                          CL_AMS_ENTITY_OP_REMOVES_MPLUSN | 
                                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
                {
                    continue;
                }

                if ( clAmsPeSGColocationCheckFails(sg, activeSU, tmpSU) )
                {
                    continue;
                }

                suLoad = clAmsPeSUComputeLoad(tmpSU);

                if ( suLoad < leastLoad )
                {
                    leastLoad = suLoad;
                    *su = tmpSU;
                }
            }

            if ( *su )
                return CL_OK;

            return CL_AMS_RC(CL_ERR_NOT_EXIST);
        }

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

                tmpSU->status.numDelayAssignments = 0;

                if(tmpSU->status.suAssignmentTimer.count > 0)
                {
                    clAmsEntityTimerStop((ClAmsEntityT*)tmpSU,
                                         CL_AMS_SU_TIMER_ASSIGNMENT);
                }

                if ( tmpSU->status.numActiveSIs || tmpSU->status.numQuiescedSIs)
                {
                    continue;
                }

                /*
                 * Check if this standby SU is having a remove pending against other standby SIs
                 */
                if ( clAmsEntityOpPending((ClAmsEntityT*)tmpSU, 
                                          &tmpSU->status.entity, 
                                          CL_AMS_ENTITY_OP_REMOVES_MPLUSN |
                                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN) )
                {
                    continue;
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

            /*
             * Fall back to default strategy incase no SU is assignable.
             */
            return clAmsPeSGDefaultLoadingStrategyStandbyMPlusN(sg, su, si);
        }

        /*
         * This keeps the option open that a system designer may choose
         * any other scheme which is invoked here via a callout.
         */

        case CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED:
        {
            // Future
        }

        default:
        {
            AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }
}

/*
 * clAmsPeSGAssignSUMPlusN
 * -----------------------
 * This fn figures out which SIs to assign to which SUs in a SG for the M+N
 * redundancy model. Since 2N is a special case of M+N, this fn also works
 * for the 2N model with the right SG configuration.
 *
 * There are two distinct parts to this fn, finding out the SIs and then
 * finding out the SUs for active assignment and then for standby assignment.
 */

ClRcT
clAmsPeSGAssignSUMPlusN(
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

        ClAmsSIT *si=NULL;

        while ( 1 )
        {
            ClAmsSUT *su = NULL;

            rc1 = clAmsPeSGFindSIForActiveAssignment(sg, &si);

            if ( rc1 != CL_OK ) 
            {
                break;
            }
            clLogInfo("SG", "ASI",
                              "SI [%.*s] needs assignment...",
                              si->config.entity.name.length-1,
                              si->config.entity.name.value);

            rc2 = clAmsPeSGFindSUForActiveAssignmentMPlusN(sg, &su, si);

            if ( rc2 != CL_OK )
            {
                break;
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

            /*
             * Before assignment, check if there is an SU in the instantiated
             * or instantiable list that is of a higher rank. If yes, then
             * wait for a specific period before reassignment.
             */
            if(sg->config.autoAdjust)
            {
                clLogInfo("SG", "ADJUST",
                              "Evaluating SU [%.*s] for assignments...",
                              su->config.entity.name.length-1,
                              su->config.entity.name.value);
                
                if(clAmsPeSGCheckSUHigherRank(sg, su, si) == CL_OK)
                {
                    clLogInfo("SG", "ADJUST",
                              "Deferring SU [%.*s] active assignment as there are higher ranked "
                              "SUs",
                              su->config.entity.name.length-1,
                              su->config.entity.name.value);

                    ++su->status.numWaitAdjustments;

                    AMS_CALL(clAmsEntityTimerStart((ClAmsEntityT*)sg,
                                                   CL_AMS_SG_TIMER_ADJUST));
                    /*
                     * Skip assignment until the timer expires or realignment.
                     */
                    break;
                }
            }
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
            ClAmsHAStateT haState = CL_AMS_HA_STATE_STANDBY;

            rc1 = clAmsPeSGFindSIForStandbyAssignment(sg, &si, scannedSIList, numScannedSIs);

            if ( rc1 != CL_OK ) 
            {
                break;
            }

            rc2 = clAmsPeSGFindSUForStandbyAssignmentMPlusN(sg, &su, si);

            if ( rc2 != CL_OK )
            {
                break;
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
            
            /*
             * Auto adjust the SG based on this SUs rank.
             */
            clAmsPeSGRealignSU(sg, su, &haState);

            /*
             * back out for reevaluation after the old active
             */
            if(haState == CL_AMS_HA_STATE_ACTIVE)
                break;

            rc2 = clAmsPeSUAssignSI(su, si, haState);

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

/*
 * Check if the standby SIs from the SU can be assigned to the given SU in the sg.
 */

static ClBoolT clAmsPeCheckSpareStandbyAdjustMPlusN(ClAmsSUT *spareSU, ClAmsSUT *standbySU)
{
    ClAmsEntityRefT *siRef = NULL;
    ClAmsSGT *sg = NULL;

    if( !(sg = (ClAmsSGT*)standbySU->config.parentSG.ptr) )
        return CL_FALSE;

    if(sg->config.loadingStrategy != CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU)
        return CL_TRUE;

    if(clAmsPeSUIsAssignable(spareSU) != CL_OK)
        return CL_FALSE;

    for(siRef = clAmsEntityListGetFirst(&standbySU->status.siList);
        siRef != NULL;
        siRef = clAmsEntityListGetNext(&standbySU->status.siList, siRef))
    {
        ClAmsSIT *si = (ClAmsSIT*)siRef->ptr;
        ClAmsEntityRefT *suRef = NULL;
        ClAmsSUT *activeSU = NULL;
        if(!si) return CL_FALSE;
        for(suRef = clAmsEntityListGetFirst(&si->status.suList);
            suRef != NULL;
            suRef = clAmsEntityListGetNext(&si->status.suList, suRef))
        {
            ClAmsSISURefT *siSURef = (ClAmsSISURefT*)suRef;
            if(siSURef->haState == CL_AMS_HA_STATE_ACTIVE
               ||
               siSURef->haState == CL_AMS_HA_STATE_QUIESCING)
            {
                activeSU = (ClAmsSUT*)suRef->ptr;
                break;
            }
        }
        if(!activeSU) continue;
        if(clAmsPeSGColocationCheckFails(sg, spareSU, activeSU))
            return CL_FALSE;
        return CL_TRUE;
    }
    return CL_FALSE;
}

/*
 * Remove the other standby SIs for the SU thats going to become active
 * before switching over.
 */

ClRcT
clAmsPeSURemoveStandbyMPlusN(ClAmsSGT *sg, ClAmsSUT *su, ClUint32T switchoverMode, ClUint32T error, 
                             ClAmsSUT **activeSU, ClBoolT *reassignWork)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT *siRef = NULL;
    ClAmsSUT *maxStandbySU = NULL;
    ClAmsSIT **otherSIs = NULL;
    ClAmsSIT **activeSIs = NULL;
    ClUint32T numActiveSIs = 0;
    ClUint32T numOtherSIs = 0;
    ClUint32T i = 0;
    ClAmsEntityRemoveOpT removeOp = {{0}};
    ClAmsEntityRemoveOpT *pendingRemoveOp = NULL;
    
    if(!sg || !su || !activeSU || !reassignWork) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *activeSU = NULL;
    *reassignWork = CL_FALSE;

    if(sg->config.redundancyModel != CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N)
    {
        *reassignWork = CL_TRUE;
        return CL_OK;
    }

    for(siRef = clAmsEntityListGetFirst(&su->status.siList);
        siRef != NULL;
        siRef = clAmsEntityListGetNext(&su->status.siList, siRef))
    {
        ClAmsSIT *si = (ClAmsSIT*)siRef->ptr;
        ClAmsEntityRefT *suRef = NULL;
        if(!(numActiveSIs & 7))
        {
            activeSIs = clHeapRealloc(activeSIs,
                                      (numActiveSIs + 8) * sizeof(*activeSIs));
            CL_ASSERT(activeSIs != NULL);
        }
        activeSIs[numActiveSIs++] = si;
        /*
         * Find a standby SU with the highest rank.
         */
        for(suRef = clAmsEntityListGetFirst(&si->status.suList);
            suRef != NULL;
            suRef = clAmsEntityListGetNext(&si->status.suList, suRef))
        {
            ClAmsSUT *standbySU = (ClAmsSUT*)suRef->ptr;
            ClAmsSISURefT *siSURef = (ClAmsSISURefT*)suRef;

            if(standbySU == su) continue;

            if(clAmsPeSUIsInstantiable(standbySU) != CL_OK)
                continue;

            if(siSURef->haState != CL_AMS_HA_STATE_STANDBY)
                continue;

            /*
             * If there is a pending SI remove for the standby SU,
             * skip the process.
             */
            if(standbySU->status.entity.opStack.numOps > 0 
               && 
               clAmsEntityOpGet(&standbySU->config.entity, &standbySU->status.entity, 
                                CL_AMS_ENTITY_OP_REMOVE_MPLUSN, (void**)&pendingRemoveOp, NULL) == CL_OK)

            {
                clLogNotice("SI", "REPLAY", "Standby SU [%s] has [%d] pending ops. Mode [%#x]", 
                            standbySU->config.entity.name.value, standbySU->status.entity.opStack.numOps,
                            switchoverMode);
                *activeSU = standbySU;
                if( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_CONTROLLER) )
                {
                    clLogNotice("SI", "REPLAY", 
                                "Skipping SI remove replay operation during controller switchover phase for SU "
                                "[%s]", standbySU->config.entity.name.value);
                    clAmsEntityOpClear(&standbySU->config.entity, &standbySU->status.entity, 
                                       CL_AMS_ENTITY_OP_REMOVE_MPLUSN, NULL, NULL);
                    *reassignWork = CL_TRUE;
                }
                /*
                 * If the standby SU has a pending remove OP for another SU, then bail this SU out
                 */
                if(strncmp((const ClCharT*)pendingRemoveOp->entity.name.value,
                            (const ClCharT*)su->config.entity.name.value,
                            su->config.entity.name.length))
                {
                    clLogNotice("SI", "REPLAY", "Standby SU [%s] has pending removes against active SU [%s]. "
                                "Skipping reassign for SU [%s]", standbySU->config.entity.name.value,
                                pendingRemoveOp->entity.name.value, su->config.entity.name.value);
                    *activeSU = NULL;
                }
                goto out_free;
            }

            /*
             * If there is a pending CSI remove invocation against
             * the standby, skip it.
             */
            if(clAmsInvocationPendingForSU(standbySU, CL_AMS_CSI_RMV_CALLBACK))
                continue;

            if(!standbySU->config.rank)
            {
                if(!maxStandbySU)
                    maxStandbySU = standbySU;
            }
            else
            {
                if(!maxStandbySU || 
                   !maxStandbySU->config.rank ||
                   (standbySU->config.rank < maxStandbySU->config.rank))
                    maxStandbySU = standbySU;
            }
        }
    }

    if(!maxStandbySU)
    {
        *reassignWork = CL_TRUE;
        goto out_free;
    }

    *activeSU = maxStandbySU;

    /*
     * Find the other standby SIs for this SU.
     */
    for(siRef = clAmsEntityListGetFirst(&maxStandbySU->status.siList);
        siRef != NULL;
        siRef = clAmsEntityListGetNext(&maxStandbySU->status.siList, siRef))
    {
        ClAmsSIT *si = (ClAmsSIT*)siRef->ptr;
        for(i = 0; i < numActiveSIs && activeSIs[i] != si; ++i);
        if(i == numActiveSIs)
        {
            if( !(numOtherSIs & 7 ) )
            {
                otherSIs = clHeapRealloc(otherSIs, 
                                         (numOtherSIs + 8) * sizeof(*otherSIs));
                CL_ASSERT(otherSIs != NULL);
            }
            otherSIs[numOtherSIs++] = si;
        }
    }

    if(!numOtherSIs)
    {
        *reassignWork = CL_TRUE;
        goto out_free;
    }

    memcpy(&removeOp.entity, &su->config.entity , sizeof(removeOp.entity));
    removeOp.sisRemoved = numOtherSIs;
    removeOp.switchoverMode = switchoverMode;
    removeOp.error = error;

    clAmsEntityOpPush(&maxStandbySU->config.entity,
                      &maxStandbySU->status.entity,
                      (void*)&removeOp,
                      (ClUint32T)sizeof(removeOp),
                      CL_AMS_ENTITY_OP_REMOVE_MPLUSN);

    /*
     * Remove the other SI standby assignments
     */

    for(i = 0; i < numOtherSIs; ++i)
    {
        ClAmsSIT *si = otherSIs[i];
        clAmsPeSURemoveSI(maxStandbySU, si, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
    }

    out_free:

    if(activeSIs) clHeapFree(activeSIs);
    if(otherSIs)  clHeapFree(otherSIs);

    return rc;
}

ClRcT clAmsPeSUSIDependentsListMPlusN(ClAmsSUT *su, 
                                      ClAmsSUT **activeSU,
                                      ClListHeadT *dependentSIList)
{
    ClAmsEntityRefT *siRef = NULL;
    ClAmsEntityRefT *suRef = NULL;

    if(!su || !dependentSIList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    for(siRef = clAmsEntityListGetFirst(&su->status.siList);
        siRef != NULL;
        siRef = clAmsEntityListGetNext(&su->status.siList, siRef))
    {
        ClAmsSIT *targetSI = (ClAmsSIT*)siRef->ptr;
        ClAmsSUT *targetSU = NULL;
        ClAmsSIReassignEntryT *reassignEntry = NULL;

        /*
         * Check if the targetSIs dependencies are enabled.
         */
        if(!targetSI->config.siDependenciesList.numEntities) 
        {
            continue;
        }

        /*
         * This SI has dependencies. Check if it should wait for their
         * reassignments.
         */
        if(!clAmsPeCheckDependencySIInNode(targetSI, su, 0))
        {
            continue;
        }

        for(suRef = clAmsEntityListGetFirst(&targetSI->status.suList);
            suRef != NULL;
            suRef = clAmsEntityListGetNext(&targetSI->status.suList, suRef))
        {
            ClAmsSUT *standbySU = (ClAmsSUT*)suRef->ptr;
            if(!standbySU->status.numStandbySIs) continue;
            if(!targetSU) 
            {
                targetSU = standbySU;
            }
            else
            {
                if(standbySU->config.rank &&
                   ( (standbySU->config.rank < targetSU->config.rank)
                     ||
                     !targetSU->config.rank) )
                    targetSU = standbySU;
            }
        }
        if(!targetSU) continue;
        if(activeSU && !*activeSU) *activeSU = targetSU;
        /*
         * This SI is not active assignable. Mark this SI and the
         * SU to which it belongs for a pending REASSIGN.
         */
        reassignEntry = clHeapCalloc(1, sizeof(*reassignEntry));
        CL_ASSERT(reassignEntry != NULL);
        reassignEntry->si = targetSI;
        clListAddTail(&reassignEntry->list, dependentSIList);
        
        /*
         * Add a reassign op to the SI and SU.
         */
        clAmsPeAddReassignOp(targetSI, targetSU);
    }

    return CL_OK;
}

static ClAmsSUT *getLeastPreferredSU(ClAmsEntityListT* lst,ClAmsSUT* butBeforeThis)
{
    ClAmsEntityRefT *SURef = NULL;
    ClAmsSUT        *leastSU = NULL;    
    /*
     * The list is already ordered according to SU rank, so the first one is the preferred SU

     * Go through the assigned SU list from preferred (lowest) rank to the highest
     * to find the least preferred SU.
     *
     * The zero ranks end up at the end of the list (most likely) because the hash value is large
     */
    
    for(SURef = clAmsEntityListGetFirst(lst);
        SURef != NULL;
        SURef = clAmsEntityListGetNext(lst, SURef))
    {
        ClAmsSUT *SU = (ClAmsSUT*)SURef->ptr;
        if(SU == butBeforeThis) break;        
        if(SU->status.numQuiescedSIs) continue;
        /* Find the preferred SU */
        if ((!leastSU) || (SU->config.rank==0) || 
            (leastSU->config.rank 
             && 
             SU->config.rank > leastSU->config.rank))
            leastSU = SU;
    }
    return leastSU;            
}

ClRcT
clAmsPeSGAutoAdjustMPlusN(ClAmsSGT *sg)
{
    ClRcT rc = CL_OK;
    ClAmsSUT *leastSU = NULL;

    ClAmsSUT *lastStandbySU = NULL;
    ClAmsEntityRefT *SURef = NULL;
    
    ClAmsEntityRefT *eRef = NULL;
    ClUint32T numActiveSUs = 0;
    ClUint32T numStandbySUs = 0;
    ClUint32T numAdjustSUs = 0;
    ClUint32T suPendingInvocations = 0;

    CL_LIST_HEAD_DECLARE(suActiveAdjustList);
    CL_LIST_HEAD_DECLARE(suStandbyAdjustList);

    
    if(!sg->config.autoAdjust)
        return CL_OK;

    if(sg->status.adjustTimer.count)
        clAmsEntityTimerStop((ClAmsEntityT*)sg, CL_AMS_SG_TIMER_ADJUST);

    clLogInfo("SG", "ADJUST", "Adjusting SG [%.*s]",
              sg->config.entity.name.length-1,
              sg->config.entity.name.value);
    
    /*
     * SU ranks are arranged so the lowest number is the most preferred to be used.  This preference will
     * be referred to as the "lowest" rank.
 
     * First pass, instantiate one preferred SU from the instantiable list, if any.
     * and fail the least preferred.
     */
    if(sg->status.instantiableSUList.numEntities)
    {
        ClAmsSUT *instantiableSU = NULL;

        ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;
        eRef = clAmsEntityListGetFirst(&sg->status.instantiableSUList);
        CL_ASSERT(eRef != NULL);
        instantiableSU = (ClAmsSUT*)eRef->ptr;
        clAmsPeSUComputeAdminState(instantiableSU, &adminState);

        leastSU = getLeastPreferredSU(&sg->status.assignedSUList,NULL);
       
        if(instantiableSU->config.rank 
           && 
           adminState == CL_AMS_ADMIN_STATE_UNLOCKED
           &&
           clAmsPeSUIsInstantiable(instantiableSU) == CL_OK
           && 
           !instantiableSU->status.suProbationTimer.count)
        {
            
            if(leastSU) /* We found the active SU that is the lowest rank */
            {
                /* If the SU that we could instantiate is lower then our lowest, then start the new one up.
                   Every rank beats 0.
                */
                if (leastSU->config.rank == 0 
                    ||
                    instantiableSU->config.rank < leastSU->config.rank)
                {
                    clLogInfo("SG", "ADJUST", "Instantiating preferred (lower ranked) SU [%.*s]",
                              instantiableSU->config.entity.name.length-1,
                              instantiableSU->config.entity.name.value);
                    clAmsPeSUInstantiate(instantiableSU);

                    suPendingInvocations = clAmsInvocationsPendingForSU(leastSU);
                    if(suPendingInvocations)
                    {
                        clLogInfo("SG", "ADJUST", "Deferring the adjustment of SU [%.*s] as "
                                  "it has [%d] pending invocations", 
                                  leastSU->config.entity.name.length-1,
                                  leastSU->config.entity.name.value,
                                  suPendingInvocations
                                  );
                        /*
                         * Straight away bail out as we could end up relocating   
                         * SIs many times. Safe to try in a clean state.
                         */
                        goto out_defer;
                    }
                    clLogInfo("SG", "ADJUST", "Switching over less preferred (highest ranked) SU [%.*s]",
                              leastSU->config.entity.name.length-1,
                              leastSU->config.entity.name.value);
                    if(leastSU->status.numActiveSIs)
                        clAmsPeSUSwitchoverWorkActiveAndStandby(leastSU, NULL, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
                    else
                        clAmsPeSUSwitchoverWorkBySI(leastSU, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);  
                }
            }
        }
    }

    /*
     * Look for instantiated but unassigned SUs that have a higher rank.
     * and switching them over with more preferred standbys
     */
    leastSU = getLeastPreferredSU(&sg->status.assignedSUList,leastSU);

    clLogInfo("SG", "ADJUST", "Least preferred SU to start with [%s], inservice spares [%d]",
              leastSU ? (const ClCharT*)leastSU->config.entity.name.value:"None",
              sg->status.inserviceSpareSUList.numEntities);

    for(SURef = clAmsEntityListGetFirst(&sg->status.inserviceSpareSUList);
        ((leastSU) && (SURef != NULL));
        SURef = clAmsEntityListGetNext(&sg->status.inserviceSpareSUList, SURef))
    {
        ClAmsSUT *SU = (ClAmsSUT*)SURef->ptr;
        
        if(clAmsPeSUIsAssignable(SU) != CL_OK)
            continue;

        /*
         * Skip SUs in the probation window
         */
        if(SU->status.suProbationTimer.count > 0)
            continue;
        
        /* If the possible SU has a lower rank or it has a nonzero rank and the least rank is zero then we want to swap */
        if ((SU->config.rank) && ((SU->config.rank < leastSU->config.rank) || (!leastSU->config.rank)))
        {
            ClAmsSUAdjustListT *adjustEntry = NULL;
            /*
             * if the spare is trying to be swapped with a least pref. standby.
             * then check if adjustments would work in spares favor.
             */
            if(leastSU->status.numStandbySIs)
            {
                if(!clAmsPeCheckSpareStandbyAdjustMPlusN(SU, leastSU))
                {
                    clLogInfo("SG", "ADJUST", "Skipping higher ranked spare SU [%s] "
                              "since it cannot be assigned over lesser ranked [%s]",
                              SU->config.entity.name.value, leastSU->config.entity.name.value);
                    continue;
                }
            }
            adjustEntry = clHeapCalloc(1, sizeof(*adjustEntry));
            CL_ASSERT(adjustEntry != NULL);
            adjustEntry->su = leastSU;
            if(leastSU->status.numActiveSIs)
            {                
                clListAdd(&adjustEntry->list, &suActiveAdjustList);                
                ++numActiveSUs;
            }
            else
            {
                clListAddTail(&adjustEntry->list, &suStandbyAdjustList);
                ++numStandbySUs;
            }

            leastSU = getLeastPreferredSU(&sg->status.assignedSUList,leastSU);  /* Find the next least preferred */
        }
    }
 
    /*
     * Second pass: We scan in the normal way looking out for the least preferred actives.
     * and switching them over with more preferred standbys
     */

    /* But first see if I need to defer */
    for(eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef))
    {
        ClAmsSUT *su = (ClAmsSUT*)eRef->ptr;
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
            goto out_defer;
        }
    }
    

    /* Go through the assigned SU list from least preferred (highest) rank to the lowest */    
    for(eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef))
    {
        ClAmsSUT *su = (ClAmsSUT*)eRef->ptr;

        if(clAmsPeSUIsAssignable(su) != CL_OK) continue;

        /*
         * Skip SU in the probation time.
         */
        if(su->status.suProbationTimer.count) continue;

        /* If I saw a standby SU before I saw this one, then the standby should be active */
        if ((lastStandbySU)&&(su->status.numActiveSIs)) /* This will cause unranked Actives to always swap: || su->config.rank == 0) */
        {
            clLogInfo("SG", "ADJUST", "Active SU [%.*s] of rank [%d] should be swapped with Standby SU [%.*s] of rank [%d]",
                      su->config.entity.name.length-1, su->config.entity.name.value, su->config.rank,
                      lastStandbySU->config.entity.name.length-1, lastStandbySU->config.entity.name.value, lastStandbySU->config.rank
                      );

            /*
             * Add the lower ranked ones for active swap at the head.
             */
            ClAmsSUAdjustListT *adjustEntry = clHeapCalloc(1, sizeof(*adjustEntry));
            CL_ASSERT(adjustEntry != NULL);
            adjustEntry->su = su;
            clListAdd(&adjustEntry->list, &suActiveAdjustList);
            ++numActiveSUs;

            if(lastStandbySU->status.numStandbySIs)
            {
                adjustEntry = clHeapCalloc(1, sizeof(*adjustEntry));
                CL_ASSERT(adjustEntry != NULL);
                adjustEntry->su = lastStandbySU;
                clListAddTail(&adjustEntry->list, &suStandbyAdjustList);
                ++numStandbySUs;
            }
            lastStandbySU = su;

        }
        else if ((!lastStandbySU) && (su->config.rank)&&(su->status.numStandbySIs)) lastStandbySU = su;
        
#if 0        
        if(su->config.rank &&
           (su->status.numStandbySIs &&
            !su->status.numActiveSIs))
        {
            /*
             * Add the higher ranked ones for standby revert at the head.
             */
            ClAmsSUAdjustListT *adjustEntry = clHeapCalloc(1, sizeof(*adjustEntry));
            CL_ASSERT(adjustEntry != NULL);
            adjustEntry->su = su;
            clListAddTail(&adjustEntry->list, &suStandbyAdjustList);
            lastStandbySU = su;
            ++numStandbySUs;
        }
#endif        
    }
    
    //numAdjustSUs = CL_MIN(numActiveSUs, numStandbySUs);
    numAdjustSUs = numActiveSUs + numStandbySUs;
    
    
    if(numAdjustSUs)
    {
        ClUint32T i = 0;
        clLogInfo("SG", "ADJUST", "[%d] SUs to be adjusted", numActiveSUs + numStandbySUs);

        while(!CL_LIST_HEAD_EMPTY(&suStandbyAdjustList) &&
              i++ < numAdjustSUs)
        {
            ClListHeadT *head = suStandbyAdjustList.pNext;
            ClAmsSUAdjustListT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustListT, list);
            ClAmsSUT *su = adjustEntry->su;
            clLogInfo("SG", "ADJUST", "Removing standby assignments from SU [%.*s] of rank [%d]",
                      su->config.entity.name.length-1, su->config.entity.name.value, su->config.rank);
            clAmsPeSUSwitchoverWorkBySI(su, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
            clListDel(head);
            clHeapFree(adjustEntry);
        }

        i = 0;
        while(!CL_LIST_HEAD_EMPTY(&suActiveAdjustList) &&
              i++ < numAdjustSUs)
        {
            ClListHeadT *head = suActiveAdjustList.pNext;
            ClAmsSUAdjustListT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustListT, list);
            ClAmsSUT *su = adjustEntry->su;
            clLogInfo("SG", "ADJUST", "Switching over active SU [%.*s] with rank [%d]",
                      su->config.entity.name.length-1, su->config.entity.name.value,
                      su->config.rank);
            /*clAmsPeSUSwitchoverWorkActiveAndStandby(su, &suStandbyAdjustList, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);*/
            /* Because I've already emptied the standby adjust list */
            clAmsPeSUSwitchoverWorkActiveAndStandby(su, NULL, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
            clListDel(head);
            clHeapFree(adjustEntry);
        }
    }
    else
    {
        /*
         * There could be instantiated SUs in the SG list that are pending inservice moves.
         * So reschedule an adjust again.
         */
        if(sg->status.instantiatedSUList.numEntities > 0 )
        {
            clLogInfo("SG", "ADJUST", "SG [%s] has SUs pending instantiation. "
                      "Deferring SG adjustment by [%d] secs.",
                      sg->config.entity.name.value, 
                      CL_AMS_SG_ADJUST_DURATION/1000);
    
            goto out_defer;
        }
    }
    rc = CL_OK;

    if (0)
    {        
        out_defer:  /* Must clean up the lists even if we defer */

        clLogInfo("SG", "ADJUST", "Starting SG [%.*s] adjust timer of [%d] secs to readjust SG in a clean state",
                  sg->config.entity.name.length-1,
                  sg->config.entity.name.value,
                  CL_AMS_SG_ADJUST_DURATION/1000);

        AMS_CALL(clAmsEntityTimerStart((ClAmsEntityT*)sg,
                                       CL_AMS_SG_TIMER_ADJUST));
    }
    
    while(!CL_LIST_HEAD_EMPTY(&suActiveAdjustList))
    {
        ClListHeadT *head = suActiveAdjustList.pNext;
        ClAmsSUAdjustListT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustListT, list);
        clListDel(head);
        clHeapFree(adjustEntry);
    }

    while(!CL_LIST_HEAD_EMPTY(&suStandbyAdjustList))
    {
        ClListHeadT *head = suStandbyAdjustList.pNext;
        ClAmsSUAdjustListT *adjustEntry = CL_LIST_ENTRY(head, ClAmsSUAdjustListT, list);
        clListDel(head);
        clHeapFree(adjustEntry);
    }

    clLogInfo("SG", "ADJUST", "Done Adjusting SG [%.*s]",
              sg->config.entity.name.length-1,
              sg->config.entity.name.value);
   
    return rc;

}

ClRcT clAmsPeSISwapMPlusN(ClAmsSIT *si, ClAmsSGT *sg)
{
    ClAmsSUT *activeSU = NULL;
    ClAmsSUT *standbySU = NULL;
    ClAmsEntityRefT *eRef = NULL;
    ClAmsSIT **swapSIList = NULL;
    ClAmsSIT **swapSIListExtended = NULL;
    ClInt32T numSwapSIs = 0;
    ClInt32T numSwapExtendedSIs = 0;
    ClAmsEntitySwapActiveOpT swapActive = {0};
    ClInt32T i = 0;
    ClInt32T sisReassigned = 0;
    ClRcT rc = CL_OK;
    CL_LIST_HEAD_DECLARE(dependentSIList);

    for(eRef = clAmsEntityListGetFirst(&si->status.suList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&si->status.suList, eRef))
    {
        ClAmsSISURefT *siSURef = (ClAmsSISURefT*)eRef;
        if(siSURef->haState == CL_AMS_HA_STATE_ACTIVE 
           ||
           siSURef->haState == CL_AMS_HA_STATE_QUIESCING)
        {
            activeSU = (ClAmsSUT*)siSURef->entityRef.ptr;
        }
        else if(siSURef->haState == CL_AMS_HA_STATE_STANDBY
                && !standbySU)
        {
            standbySU = (ClAmsSUT*)siSURef->entityRef.ptr;
        }
        else 
        {
            /* do nothing*/
        }
    }

    if(activeSU == NULL || standbySU == NULL)
    {
        clLogError("SI", "OP-SWAP", 
                   "SI [%.*s] doesnt have active or standby SUs",
                   si->config.entity.name.length-1, si->config.entity.name.value);
        rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
        goto out;
    }

    if(activeSU->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING
       || 
       activeSU->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING
       ||
       activeSU->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING)
    {
        clLogError ("SI", "OP-SWAP", 
                    "Active SU [%.*s] has state [%s]. Returning try again for SI swap",
                    activeSU->config.entity.name.length-1, activeSU->config.entity.name.value,
                    CL_AMS_STRING_P_STATE(activeSU->status.presenceState));
        rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    if(standbySU->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING
       ||
       standbySU->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING
       ||
       standbySU->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING)
    {
        clLogError("SI", "OP-SWAP", 
                   "Standby SU [%.*s] has state [%s]. Returning try again for SI swap",
                   standbySU->config.entity.name.length-1, standbySU->config.entity.name.value,
                   CL_AMS_STRING_P_STATE(standbySU->status.presenceState));
        rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    /*
     * Now swap all the SIs assigned to the active SU. to standby
     * We first move all the active SIs to standby so that we don't have 2
     * actives at the same time as thats worse than having 2 standbys
     */
    for(eRef = clAmsEntityListGetFirst(&activeSU->status.siList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&activeSU->status.siList, eRef))
    {
        ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT*)eRef;
        ClAmsSIT *targetSI = (ClAmsSIT*)suSIRef->entityRef.ptr;
        if(suSIRef->haState != CL_AMS_HA_STATE_ACTIVE
           &&
           suSIRef->haState != CL_AMS_HA_STATE_QUIESCING)
        {
            clLogWarning("SI", "OP-SWAP", "Skipping swap of SI [%.*s] with ha state [%s]",
                         targetSI->config.entity.name.length-1,
                         targetSI->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(suSIRef->haState));
            continue;
        }

        /*
         * Do it in batches of 4 to reduce allocations.
         */
        if(!(numSwapSIs & 3 ))
        {
            swapSIList = clHeapRealloc(swapSIList,
                                       (ClUint32T)
                                       sizeof(*swapSIList)*(numSwapSIs + 4));
            CL_ASSERT(swapSIList != NULL);
        }

        swapSIList[numSwapSIs++] = targetSI;
    }

    swapSIListExtended = clHeapCalloc(sg->config.maxStandbySIsPerSU + numSwapSIs, 
                                      sizeof(*swapSIListExtended));
    CL_ASSERT(swapSIListExtended != NULL);

    /*
     * Now go through the standby assignments and swap the SI pairs.
     * Incase of standby assignments from other SIs not part of the active SU, then
     * move those SI standby to the old active. We do this first and then assign the swap
     * pairs to active. We do this in 2 separate passes
     */
    for(eRef = clAmsEntityListGetFirst(&standbySU->status.siList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&standbySU->status.siList, eRef))
    {
        ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT*)eRef;
        ClAmsSIT *targetSI = (ClAmsSIT*)suSIRef->entityRef.ptr;

        if(suSIRef->haState != CL_AMS_HA_STATE_STANDBY)
        {
            clLogWarning ("SI", "OP-SWAP",
                          "Skipping SI [%.*s] swap for standby SU [%.*s] as ha state isnt standby",
                          targetSI->config.entity.name.length-1,
                          targetSI->config.entity.name.value, 
                          standbySU->config.entity.name.length-1,
                          standbySU->config.entity.name.value);
            continue;
        }
        /*
         * Check if this SI is not in the swap list. and remove this from the standby
         * SU and move it to the new standby.
         */
        for(i = 0; i < numSwapSIs && swapSIList[i] != targetSI; ++i);
        
        if(i == numSwapSIs)
        {
            swapSIListExtended[numSwapExtendedSIs++] = targetSI;
        }

    }

    /*
     * First remove other standby CSIs.
     */
    for(i = 0; i < numSwapExtendedSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIListExtended[i];
        /* 
         * this SI isnt in the swap list. Move it to the new standby SU. (old active)
         */
        clLogInfo("SI", "OP-SWAP", "Removing SI [%.*s] from old standby SU [%.*s]",
                  targetSI->config.entity.name.length-1,
                  targetSI->config.entity.name.value,
                  standbySU->config.entity.name.length-1,
                  standbySU->config.entity.name.value);

        rc = clAmsPeSURemoveSI(standbySU, targetSI, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE | CL_AMS_ENTITY_SWITCHOVER_SWAP);
        if(rc != CL_OK)
        {
            clLogError("SI", "OP-SWAP",
                       "SI [%.*s] remove from old standby SU [%.*s] returned [%#x]",
                       targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value,
                       standbySU->config.entity.name.length-1,
                       standbySU->config.entity.name.value, rc);
            goto out_free;
        }
        
    }

    if(numSwapExtendedSIs > 0)
    {
        ClAmsEntitySwapRemoveOpT swapOp = {{0}};
        memcpy(&swapOp.entity, &activeSU->config.entity, sizeof(swapOp.entity));
        swapOp.sisRemoved = numSwapExtendedSIs;
        swapOp.otherSIs = clHeapCalloc(numSwapExtendedSIs, sizeof(ClAmsEntityT));
        CL_ASSERT(swapOp.otherSIs != NULL);
        swapOp.numOtherSIs = numSwapExtendedSIs;
        for(i = 0; i < numSwapExtendedSIs; ++i)
        {
            memcpy(swapOp.otherSIs + i, &swapSIListExtended[i]->config.entity, 
                   sizeof(ClAmsEntityT));
        }

        clAmsEntityOpPush(&standbySU->config.entity,
                          &standbySU->status.entity,
                          (void*)&swapOp,
                          (ClUint32T)sizeof(swapOp),
                          CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN);
        rc = CL_OK;
        goto out_free;
    }

    /*
     * Move active to standby.
     */
    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIList[i];
        clAmsPeSURemoveSI(activeSU, targetSI, 
                          CL_AMS_ENTITY_SWITCHOVER_FAST | CL_AMS_ENTITY_SWITCHOVER_SWAP);
    }

    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIList[i];
        clLogNotice("SI", "OP-SWAP", "SI swap - SI [%.*s] standby assignment to SU [%.*s]",
                    targetSI->config.entity.name.length-1,
                    targetSI->config.entity.name.value, 
                    activeSU->config.entity.name.length-1,
                    activeSU->config.entity.name.value);

        if( (rc = clAmsPeSUAssignSI(activeSU, targetSI, CL_AMS_HA_STATE_STANDBY)) != CL_OK)
        {
            clLogError("SI", "OP-SWAP", "SI swap - SI [%.*s] standby assignment to SU [%.*s] " \
                       "returned [%#x]", targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value, 
                       activeSU->config.entity.name.length-1,
                       activeSU->config.entity.name.value, rc);
            goto out_free;
        }
    }

    for(i = 0; i < numSwapExtendedSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIListExtended[i];

        clLogInfo("SI", "OP-SWAP",
                  "Assigning SI [%.*s] as standby to SU [%.*s]",
                  targetSI->config.entity.name.length-1,
                  targetSI->config.entity.name.value,
                  activeSU->config.entity.name.length-1,
                  activeSU->config.entity.name.value);

        rc = clAmsPeSUAssignSI(activeSU, targetSI, CL_AMS_HA_STATE_STANDBY);
        if(rc != CL_OK)
        {
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST
               ||
               CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)
            {
                /*
                 * This could be a fixed slot protection config.
                 * So ignore and continue.
                 */
                rc = CL_OK;
                continue;
            }
            clLogError("SI", "OP-SWAP",
                       "SI [%.*s] assign new standby SU [%.*s] returned [%#x]",
                       targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value,
                       activeSU->config.entity.name.length-1,
                       activeSU->config.entity.name.value, rc);
            goto out_free;
        }
    }

    swapActive.sisReassigned = numSwapSIs;
    clAmsEntityOpPush(&standbySU->config.entity,
                      &standbySU->status.entity,
                      (void*)&swapActive,
                      (ClUint32T)sizeof(swapActive),
                      CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN);
    
    clAmsPeSUSIDependentsListMPlusN(standbySU, NULL, &dependentSIList);

    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = (ClAmsSIT*)swapSIList[i];
        ClAmsSISURefT *siSURef = NULL;

        rc = clAmsEntityListFindEntityRef2(&targetSI->status.suList,
                                           &standbySU->config.entity,
                                           0,
                                           (ClAmsEntityRefT**)&siSURef);

        if(rc != CL_OK)
        {

            clLogWarning ("SI", "OP-SWAP", 
                          "SI [%.*s] doesnt have SU [%.*s] as standby",
                          targetSI->config.entity.name.length-1,
                          targetSI->config.entity.name.value,
                          standbySU->config.entity.name.length-1,
                          standbySU->config.entity.name.value);
            continue;
        }

        if(siSURef->haState != CL_AMS_HA_STATE_STANDBY)
        {
            clLogWarning("SI", "OP-SWAP",
                         "Skipping SI [%.*s] to SU [%.*s] as ha state isnt standby",
                         targetSI->config.entity.name.length-1,
                         targetSI->config.entity.name.value,
                         standbySU->config.entity.name.length-1,
                         standbySU->config.entity.name.value);
            continue;
        }

        if(clAmsPeSIReassignMatch(targetSI, &dependentSIList))
        {
            ++sisReassigned;
            continue;
        }

        clLogInfo("SI", "OP-SWAP",
                  "Swapping SI [%.*s] assigned active to SU [%.*s] "
                  "with standby SU [%.*s]\n",
                  targetSI->config.entity.name.length-1,
                  targetSI->config.entity.name.value,
                  activeSU->config.entity.name.length-1,
                  activeSU->config.entity.name.value,
                  standbySU->config.entity.name.length-1,
                  standbySU->config.entity.name.value);
    
        rc = clAmsPeSUAssignSI(standbySU, targetSI, CL_AMS_HA_STATE_ACTIVE);

        if(rc != CL_OK)
        {
            clLogError("SI", "OP-SWAP",
                       "SI [%.*s] active swap to SU [%.*s] returned [%#x]",
                       targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value,
                       standbySU->config.entity.name.length-1,
                       standbySU->config.entity.name.value, rc);
        }
        else
        {
            ++sisReassigned;
        }
    }

    clAmsPeSIReassignEntryListDelete(&dependentSIList);

    if(!sisReassigned || sisReassigned != numSwapSIs)
    {
        ClAmsEntitySwapActiveOpT *pSwapActive = NULL;
        rc = clAmsEntityOpGet(&standbySU->config.entity,
                              &standbySU->status.entity,
                              CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN,
                              (void**)&pSwapActive,
                              NULL);
        if(rc == CL_OK)
        {
            if(!sisReassigned)
            {
                clAmsEntityOpClear(&standbySU->config.entity,
                                   &standbySU->status.entity,
                                   CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN,
                                   NULL, NULL);
                if(pSwapActive) clHeapFree(pSwapActive);
            }
            else
            {
                if(pSwapActive) pSwapActive->sisReassigned = sisReassigned;
            }
        }
    }

    rc = CL_OK;
    
    out_free:
    if(swapSIList) clHeapFree(swapSIList);
    if(swapSIListExtended) clHeapFree(swapSIListExtended);

    out:
    return rc;
}

ClRcT
clAmsPeEntityOpSwapRemove(ClAmsEntityT *entity,
                          void *data,
                          ClUint32T dataSize,
                          ClBoolT recovery)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsEntitySwapRemoveOpT *swapOp = (ClAmsEntitySwapRemoveOpT*)data;
    ClRcT rc = CL_OK;
    ClInt32T i;
    ClAmsSUT *activeSU = NULL;
    ClAmsSUT *standbySU = NULL;
    ClAmsSIT **swapSIList = NULL;
    ClUint32T numSwapSIs = 0;
    ClAmsEntityRefT *eRef = NULL;
    ClAmsEntitySwapActiveOpT swapActive = {0};
    ClInt32T sisReassigned = 0;
    CL_LIST_HEAD_DECLARE(dependentSIList);
    
    if(!swapOp) return rc;

    if(recovery)
        goto out_free;

    if(--swapOp->sisRemoved > 0 ) return CL_AMS_RC(CL_ERR_TRY_AGAIN);

    if(swapOp->sisRemoved < 0 ) 
    {
        clLogWarning("SI", "OP_SWAP", "SU [%.*s] swap op sisRemoved is negative "
                     "implying incorrect standby SIs computed for the SU for removal",
                     entity->name.length-1, entity->name.value);
        swapOp->sisRemoved = 0;
    }
    memcpy(&entityRef.entity, &swapOp->entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                 &entityRef);

    if(rc != CL_OK)
    {
        clLogWarning("SI", "OP-SWAP", "Swap op active SU "
                     "[%s] not found", swapOp->entity.name.value);
        goto out_free;
    }
    activeSU = (ClAmsSUT*)entityRef.ptr;
    standbySU = (ClAmsSUT*)entity;
    /*
     * Now swap all the SIs assigned to the active SU. to standby
     * We first move all the active SIs to standby so that we don't have 2
     * actives at the same time as thats worse than having 2 standbys
     */
    for(eRef = clAmsEntityListGetFirst(&activeSU->status.siList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&activeSU->status.siList, eRef))
    {
        ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT*)eRef;
        ClAmsSIT *targetSI = (ClAmsSIT*)suSIRef->entityRef.ptr;
        if(suSIRef->haState != CL_AMS_HA_STATE_ACTIVE
           &&
           suSIRef->haState != CL_AMS_HA_STATE_QUIESCING)
        {
            clLogWarning("SI", "OP-SWAP", "Skipping swap of SI [%.*s] with ha state [%s]",
                         targetSI->config.entity.name.length-1,
                         targetSI->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(suSIRef->haState));
            continue;
        }

        /*
         * Do it in batches of 4 to reduce allocations.
         */
        if(!(numSwapSIs & 3 ))
        {
            swapSIList = clHeapRealloc(swapSIList,
                                       (ClUint32T)
                                       sizeof(*swapSIList)*(numSwapSIs + 4));
            CL_ASSERT(swapSIList != NULL);
        }

        swapSIList[numSwapSIs++] = targetSI;
    }

    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIList[i];
        clAmsPeSURemoveSI(activeSU, targetSI, 
                          CL_AMS_ENTITY_SWITCHOVER_FAST | CL_AMS_ENTITY_SWITCHOVER_SWAP);
    }

    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = swapSIList[i];
        clLogInfo("SI", "OP-SWAP", "SI swap - SI [%.*s] standby assignment to SU [%.*s]",
                  targetSI->config.entity.name.length-1,
                  targetSI->config.entity.name.value, 
                  activeSU->config.entity.name.length-1,
                  activeSU->config.entity.name.value);

        if( (rc = clAmsPeSUAssignSI(activeSU, targetSI, CL_AMS_HA_STATE_STANDBY)) != CL_OK)
        {
            clLogError("SI", "OP-SWAP", "SI swap - SI [%.*s] standby assignment to SU [%.*s] " \
                       "returned [%#x]", targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value, 
                       activeSU->config.entity.name.length-1,
                       activeSU->config.entity.name.value, rc);
            goto out_free;
        }
        
    }

    for(i = 0; i < swapOp->numOtherSIs; ++i)
    {
        ClAmsSIT *targetSI = NULL;

        memcpy(&entityRef.entity, swapOp->otherSIs+i,
               sizeof(entityRef.entity));

        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                     &entityRef);
        if(rc != CL_OK)
        {
            clLogError("SI", "OP-SWAP", "SI [%s] not found for standby assignment",
                       entityRef.entity.name.value);
            goto out_free;
        }
        targetSI = (ClAmsSIT*)entityRef.ptr;

        if( (rc = clAmsPeSUAssignSI(activeSU, targetSI, CL_AMS_HA_STATE_STANDBY)) != CL_OK)
        {
            clLogError("SI", "OP-SWAP", "SI swap - SI [%.*s] standby assignment to SU [%.*s] " \
                       "returned [%#x]", targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value, 
                       activeSU->config.entity.name.length-1,
                       activeSU->config.entity.name.value, rc);
            goto out_free;
        }
    }

    /*
     * Make a swap active OP.
     */
    swapActive.sisReassigned = numSwapSIs;
    clAmsEntityOpPush(&standbySU->config.entity,
                      &standbySU->status.entity,
                      (void*)&swapActive,
                      (ClUint32T)sizeof(swapActive),
                      CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN);

    clAmsPeSUSIDependentsListMPlusN(standbySU, NULL,  &dependentSIList);

    for(i = 0; i < numSwapSIs; ++i)
    {
        ClAmsSIT *targetSI = (ClAmsSIT*)swapSIList[i];
        ClAmsSISURefT *siSURef = NULL;

        rc = clAmsEntityListFindEntityRef2(&targetSI->status.suList,
                                           &standbySU->config.entity,
                                           0,
                                           (ClAmsEntityRefT**)&siSURef);

        if(rc != CL_OK)
        {

            clLogWarning ("SI", "OP-SWAP", "SI [%.*s] doesnt have SU [%.*s] as standby",
                          targetSI->config.entity.name.length-1,
                          targetSI->config.entity.name.value,
                          standbySU->config.entity.name.length-1,
                          standbySU->config.entity.name.value);
            continue;
        }

        if(siSURef->haState != CL_AMS_HA_STATE_STANDBY)
        {
            clLogWarning("SI", "OP-SWAP", "Skipping SI [%.*s] to SU [%.*s] as ha state isnt standby",
                         targetSI->config.entity.name.length-1,
                         targetSI->config.entity.name.value,
                         standbySU->config.entity.name.length-1,
                         standbySU->config.entity.name.value);
            continue;
        }

        if(clAmsPeSIReassignMatch(targetSI, &dependentSIList))
        {
            ++sisReassigned;
            continue;
        }

        clLogInfo("SI", "OP-SWAP", "Swapping SI [%.*s] assigned active to SU [%.*s] "
                  "with standby SU [%.*s]",
                  targetSI->config.entity.name.length-1,
                  targetSI->config.entity.name.value,
                  activeSU->config.entity.name.length-1,
                  activeSU->config.entity.name.value,
                  standbySU->config.entity.name.length-1,
                  standbySU->config.entity.name.value);
    
        rc = clAmsPeSUAssignSI(standbySU, targetSI, CL_AMS_HA_STATE_ACTIVE);

        if(rc != CL_OK)
        {
            clLogError("SI", "OP-SWAP",
                       "SI [%.*s] active swap to SU [%.*s] returned [%#x]",
                       targetSI->config.entity.name.length-1,
                       targetSI->config.entity.name.value,
                       standbySU->config.entity.name.length-1,
                       standbySU->config.entity.name.value, rc);
        }
        else
        {
            ++sisReassigned;
        }
    }

    clAmsPeSIReassignEntryListDelete(&dependentSIList);

    if(!sisReassigned || 
       sisReassigned != numSwapSIs)
    {
        /*
         * Update pending op.
         */
        ClAmsEntitySwapActiveOpT *pSwapActive = NULL;
        rc = clAmsEntityOpGet(&standbySU->config.entity,
                              &standbySU->status.entity,
                              CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN,
                              (void**)&pSwapActive,
                              NULL);
        if(rc == CL_OK) 
        {
            if(!sisReassigned)
            {
                clAmsEntityOpClear(&standbySU->config.entity,
                                   &standbySU->status.entity,
                                   CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN,
                                   NULL, NULL);
                if(pSwapActive) clHeapFree(pSwapActive);
            }
            else if(pSwapActive)
            {
                pSwapActive->sisReassigned = sisReassigned;
            }
        }
    }

    rc = CL_OK;

    out_free:

    clHeapFree(swapOp->otherSIs);

    if(swapSIList) clHeapFree(swapSIList);
    
    return rc;
}

ClRcT
clAmsPeEntityOpSwapActive(ClAmsEntityT *entity,
                          void *data,
                          ClUint32T dataSize,
                          ClBoolT recovery)
{
    ClAmsEntitySwapActiveOpT *swapActive = (ClAmsEntitySwapActiveOpT*)data;
    if(!entity || !swapActive) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(recovery) swapActive->sisReassigned = 1;
    if(--swapActive->sisReassigned > 0 ) return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    if(swapActive->sisReassigned < 0 )
    {
        clLogWarning("SI", "SWAP", "SIs reassigned for active transitioning SU [%s] is negative",
                     entity->name.value);
        swapActive->sisReassigned = 0;
    }
    return CL_OK;
}

ClRcT
clAmsPeEntityOpReduceRemove(ClAmsEntityT *entity, void *data, ClUint32T dataSize, ClBoolT recovery)
{
    ClAmsEntityReduceRemoveOpT *reduceOp = data;
    
    if(!data) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(recovery) reduceOp->sisRemoved = 1;

    if(--reduceOp->sisRemoved > 0 ) return CL_AMS_RC(CL_ERR_TRY_AGAIN);

    return CL_OK;
}

ClRcT
clAmsPeEntityOpActiveRemove(ClAmsEntityT *entity,
                            void *data,
                            ClUint32T dataSize,
                            ClBoolT recovery)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsSUT *failoverSU = NULL;
    ClAmsEntityRemoveOpT *activeRemoveOp = (ClAmsEntityRemoveOpT*)data;
    ClRcT rc = CL_OK;

    if(recovery) activeRemoveOp->sisRemoved = 1;
    if(--activeRemoveOp->sisRemoved > 0 ) return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    if(activeRemoveOp->sisRemoved < 0)
    {
        clLogWarning("SU", "ACTIVE_REMOVE", "SU [%s] has active remove op sis to be failed as negative",
                     entity->name.value);
        activeRemoveOp->sisRemoved = 0;
    }
    memcpy(&entityRef.entity, &activeRemoveOp->entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clLogWarning("SU", "ACTIVE_REMOVE", "SU [%s] pending CSI removes was not found",
                     activeRemoveOp->entity.name.value);
        return rc;
    }

    failoverSU = (ClAmsSUT*)entityRef.ptr;

    clLogInfo("AMS", "REPLAY", "SU [%s] replaying remove for SU [%s]",
              entity->name.value, failoverSU->config.entity.name.value);
    return clAmsPeSUSwitchoverRemoveReplay(failoverSU, activeRemoveOp->error, activeRemoveOp->switchoverMode);
}

ClRcT 
clAmsPeEntityOpRemove(ClAmsEntityT *entity,
                      void *data,
                      ClUint32T dataSize,
                      ClBoolT recovery)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsSUT *failoverSU = NULL;
    ClAmsEntityRemoveOpT *removeOp = (ClAmsEntityRemoveOpT*)data;
    ClRcT rc = CL_OK;

    if(!removeOp) return rc;
    /*
     * Check if all other SIs have been removed.
     */
    if(recovery) removeOp->sisRemoved = 1;
        
    if(--removeOp->sisRemoved > 0 ) return CL_AMS_RC(CL_ERR_TRY_AGAIN);

    if(removeOp->sisRemoved < 0 ) 
    {
        clLogWarning("SU", "OP_REMOVE", "SU [%.*s] remove op sisRemoved is negative "
                     "implying incorrect standby SIs computed for the SU for removal",
                     entity->name.length-1, entity->name.value);
        removeOp->sisRemoved = 0;
    }
    memcpy(&entityRef.entity, &removeOp->entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clLogWarning("SU", "REMOVE", "SU [%s] pending failover not found",
                     removeOp->entity.name.value);
        return rc;
    }
    failoverSU = (ClAmsSUT*)entityRef.ptr;
    return clAmsPeSUSwitchoverReplay(failoverSU, (ClAmsSUT*)entity, 
                                     removeOp->error, removeOp->switchoverMode);
}
