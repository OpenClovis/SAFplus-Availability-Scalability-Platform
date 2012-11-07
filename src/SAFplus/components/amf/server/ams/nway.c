#include <nway.h>
#include <custom.h>

/*
 * Try distributing least sis per SU.
*/

static ClRcT
clAmsPeSGDefaultLoadingStrategyActiveNway(ClAmsSGT *sg, 
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

        if (tmpSU->status.numQuiescedSIs)
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
clAmsPeSGDefaultLoadingStrategyStandbyNway(ClAmsSGT *sg,
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

            if(otherSU == activeSU)
            {
                continue;
            }

            if ( otherSU->status.readinessState != 
                 CL_AMS_READINESS_STATE_INSERVICE )
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

        if(tmpSU == activeSU)
        {
            continue;
        }

        if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
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
 * clAmsPeSGFindSUForActiveAssignmentNway
 * ----------------------------------------
 * This fn finds the next preferred SU that can be given an active SI
 * assignment. The selection of SU is based on the loading strategy
 * as well as the redundancy model. This fn only handles the Nway
 * model.
 *
 * Note, support for multiple loading strategies is an enhancement over
 * SAF. The loading strategy implicitly mentioned in the AMF B.1.1 spec
 * is least_si_per_su.
 */
ClRcT
clAmsPeSGFindSUForActiveAssignmentNway(
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
            return clAmsPeSGDefaultLoadingStrategyActiveNway(sg, su, si);
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

                if ( tmpSU->status.numQuiescedSIs )
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

                if (tmpSU->status.numQuiescedSIs)
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

                if (tmpSU->status.numQuiescedSIs)
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

            return clAmsPeSGDefaultLoadingStrategyActiveNway(sg, su, si);
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
            AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);

        }
    }
}

/*
 * clAmsPeSGFindSUForStandbyAssignmentNway
 * -----------------------------------------
 * This fn finds the next preferred SU that can be given a standby SI
 * assignment. The selection of SU is based on the loading strategy as 
 * well as the redundancy model and the assignments of other SI assigned 
 * to a SU. This fn only handles the Nway model.
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
clAmsPeSGFindSUForStandbyAssignmentNway(
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
            return clAmsPeSGDefaultLoadingStrategyStandbyNway(sg, su, si);
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

                    if(otherSU == activeSU)
                    {
                        continue;
                    }

                    if ( otherSU->status.readinessState != 
                         CL_AMS_READINESS_STATE_INSERVICE )
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
                
                if(tmpSU == activeSU)
                {
                    continue;
                }

                if ( tmpSU->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
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

                    if(otherSU == activeSU)
                    {
                        continue;
                    }

                    if ( otherSU->status.readinessState != 
                         CL_AMS_READINESS_STATE_INSERVICE )
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
            return clAmsPeSGDefaultLoadingStrategyStandbyNway(sg, su, si);
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
            AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Loading strategy [%d] for SG [%s] is not supported. Exiting..\n",
                     sg->config.loadingStrategy,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }
}

static ClRcT
clAmsPeSIIsActiveAssignableNway(CL_IN ClAmsSIT *si)
{
    return clAmsPeSIIsActiveAssignableCustom(si);
}

static ClRcT
clAmsPeSGFindSIForActiveAssignmentNway(
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
    if(!sg->config.autoAdjust && sg->config.loadingStrategy == CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE)
    {
        for (entityRef = clAmsEntityListGetFirst(&sg->config.siList);
             entityRef !=  NULL;
             entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef)) 
        {
            ClAmsEntityRefT *suRef;
            ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
            AMS_CHECK_SI(si);
            if(lookAfter == si) continue;
            if(clAmsPeSIIsActiveAssignableNway(si) != CL_OK)
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
                if(su->status.numQuiescedSIs)
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

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.siList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef) )
    {
        ClAmsSIT *si = (ClAmsSIT *)entityRef->ptr;
        ClRcT rc2;

        AMS_CHECK_SI ( si );

        /* If the caller wants us to start at a particular spot, then keep going until we find that spot
           Note that this causes an iteration over all SIs to be O(N^2) so it would be better to return the
           entityRef if this code needs to run efficiently
         */
        if (lookAfter) 
        {
            if (si == lookAfter) lookAfter= NULL;
            continue;
        }
        
        
        rc2 = clAmsPeSIIsActiveAssignableNway(si);

        if ( rc2 == CL_OK )
        {
            *targetSI = si;
            return CL_OK;
        }

        if ( rc2 != CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE) )
        {
            *targetSI = NULL;
            return rc2;
        }
    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * clAmsPeSGAssignSUNway
 * -----------------------
 * This fn figures out which SIs to assign to which SUs in a SG for the Nway
 * redundancy model. 
 *
 * There are two distinct parts to this fn, finding out the SIs and then
 * finding out the SUs for active assignment and then for standby assignment.
 */

ClRcT
clAmsPeSGAssignSUNway(
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

            rc1 = clAmsPeSGFindSIForActiveAssignmentNway(sg, &si);

            if ( rc1 != CL_OK ) 
            {
                break;
            }
            clLogInfo("SG", "ASI",
                              "SI [%.*s] needs assignment...",
                              si->config.entity.name.length-1,
                              si->config.entity.name.value);

            rc2 = clAmsPeSGFindSUForActiveAssignmentNway(sg, &su, si);

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

            rc2 = clAmsPeSGFindSUForStandbyAssignmentNway(sg, &su, si);

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
