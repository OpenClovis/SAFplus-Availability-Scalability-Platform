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
 * ModuleName  : amf
 * File        : clAmsEventServerApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the server side code for the event API.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clAmsEventServerApi.h>
#include <clAmsPolicyEngine.h>
#include <clAmsServerUtils.h>

extern ClAmsT gAms;
#include <clFaultDefinitions.h>

/******************************************************************************
 * Global data structures
 *****************************************************************************/

/******************************************************************************
 * AMS Methods
 *****************************************************************************/

ClRcT
clAmsGetFaultReport(
        CL_IN  const SaNameT  *compName,
        CL_IN  ClAmsLocalRecoveryT  recommendedRecovery,
        CL_IN  ClUint64T instantiateCookie)
{

    ClRcT  rc = CL_OK;
    ClUint32T  escalation = 0;
    ClAmsEntityRefT  entityRef = {{CL_AMS_ENTITY_TYPE_ENTITY},0,0};

    AMS_FUNC_ENTER (("\n"));

    if(!compName) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    /*
     * First find the name reported by the fault API in the component database
     * If the name is not found in the component database it means that its a node
     * level fault escalation. In this case find the node in the node database.
     */

    memcpy (&entityRef.entity.name, compName, sizeof (SaNameT));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP; 
    
    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    if ( (rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],&entityRef)) == CL_OK )
    {
        ClUint64T currentInstantiateCookie = 0;
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        clLogInfo("COMP", "FAILURE", "Processing fault for component [%s], instantiate Cookie [%lld]", comp->config.entity.name.value, instantiateCookie);
        
        currentInstantiateCookie = comp->status.instantiateCookie;
        if(instantiateCookie && instantiateCookie < currentInstantiateCookie)
        {
            clLogInfo("COMP", "FAILURE", 
                      "Ignoring fault for component [%s], instantiation identifier [%lld] "
                      "as component has already been recovered with instantiation identifier [%lld]",
                      comp->config.entity.name.value, instantiateCookie,
                      comp->status.instantiateCookie);
            goto exitfn;
        }
        AMS_CHECK_RC_ERROR ( clAmsPeEntityFaultReport(
                    entityRef.ptr,
                    &recommendedRecovery,
                    &escalation) );

    }

    else
    {

        if ( CL_GET_ERROR_CODE (rc) != CL_ERR_NOT_EXIST )
        {
            clLogWarning("COMP", "FAILURE", "Unable to find component [%s:%d] in AMS database", 
                         compName->value, compName->length);
            goto exitfn;
        }

         /*
          * See if its node level fault escalation
          */

         entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;

         AMS_CHECK_RC_ERROR( clAmsEntityDbFindEntity(
                     &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                     &entityRef) ); 
         
         AMS_CHECK_RC_ERROR( clAmsPeEntityFaultReport(
                     entityRef.ptr,
                     &recommendedRecovery,
                     &escalation) );

     }

exitfn:

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
    return CL_AMS_RC (rc);

}
