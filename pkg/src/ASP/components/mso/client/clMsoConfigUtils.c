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
 * ModuleName  : mso 
 * File        : clMsoConfigUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the Mso configuration library implementation. 
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>

/* ASP Includes */
#include <clCommon.h>
#include <clVersionApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clOmApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorMetaData.h>
#include <clCorTxnApi.h>
#include <clDebugApi.h>
#include <clOampRtApi.h>
#include <clCorErrors.h>
#include <clProvApi.h>

#include <clMsoConfig.h>
#include "clMsoConfigUtils.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCoveApi.h"
#endif

static ClRcT _clMsoConfigResourceArrayGet(ClOampRtResourceArrayT* pResourcesArray)
{    
    ClCpmHandleT    cpmHandle                       = 0;
    ClNameT         compName                        = { 0 };
    ClParserPtrT    top                             = NULL;
    ClCharT         fileName[CL_MAX_NAME_LENGTH]    = { 0 };
    ClCharT         *pFileName = fileName;
    ClCharT*        pAspPath                        = NULL;
    ClCharT*        pNodeName                       = NULL;
    ClRcT           rc                              = CL_OK;

    pAspPath = getenv( "ASP_CONFIG" );
    pNodeName = getenv( "ASP_NODENAME" );

    if( NULL == pNodeName || NULL == pAspPath)
    {
        clLogError("MSO", "UTIL", "ASP_CONFIG or ASP_NODENAME env variable is not exported.");
        return CL_MSO_RC(CL_ERR_NULL_POINTER);
    }
    
    if(gClOIConfig.oiDBReload)
    {
        if(!gClOIConfig.pOIRouteFile)
        {
            clLogError("MSO", "UTIL", "OI db reload flag is set but no OI route config file is specified");
            return CL_MSO_RC(CL_ERR_NULL_POINTER);
        }
        pFileName = (ClCharT*)gClOIConfig.pOIRouteFile;
        if(gClOIConfig.pOIRoutePath)
            pAspPath = (ClCharT*)gClOIConfig.pOIRoutePath;
        
    }
    else
    {
        snprintf( fileName, sizeof(fileName), "%s_%s", pNodeName, "rt.xml" );
    }

    top = clParserOpenFile( pAspPath, pFileName );
    if ( top == NULL )
    {
        clLogError("MSO", "UTIL", "Failed to open the route file : [%s] at path [%s]", 
                   pFileName, pAspPath);
        return CL_MSO_RC(CL_ERR_NULL_POINTER);
    }

    /*
     *  1. Get the cpm handle
     *  2. Get the name of the component which invokes this function.
     *  3. Get the resource information which are controled by given component.
     */

    cpmHandle = 0;

    rc = clCpmComponentNameGet( cpmHandle, &compName );
    if (rc != CL_OK)
    {
        clLogError("MSO", "UTIL", "Failed to get component name. rc [0x%x]", rc);
        clParserFree(top);
        return rc;
    }

    rc = clOampRtResourceInfoGet( top, &compName, pResourcesArray );
    if (rc != CL_OK)
    {
        clLogError("MSO", "UTIL", "Failed to get resource info from xml file. rc [0x%x]", rc);
        clParserFree(top);
        return rc;
    }

    clParserFree( top );

    return CL_OK;
}

ClRcT
clMsoConfigResourceListGet(ClCorMOIdListT** ppMoIdList, ClCorServiceIdT svcId)
{
    ClOampRtResourceArrayT resourcesArray = {0};
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClCorMOIdT moId = {{{0}}};
    ClCorMOClassPathT moPath = {{0}};

    rc = _clMsoConfigResourceArrayGet(&resourcesArray);
    if (rc != CL_OK)
    {
        clLogError("MSO", "UTIL", "Failed to get the resource array. rc [0x%x]", rc);
        return rc;
    }

    *ppMoIdList = clHeapAllocate(sizeof(ClCorMOIdListT) + resourcesArray.noOfResources * 
                        sizeof(ClCorMOIdT));
    memset(*ppMoIdList, 0, sizeof(ClCorMOIdListT) + resourcesArray.noOfResources * sizeof(ClCorMOIdT));

    for (i=0; i < resourcesArray.noOfResources; i++)
    {
        rc = clCorMoIdNameToMoIdGet(&resourcesArray.pResources[i].resourceName, &moId);
        if (rc != CL_OK)
        {
            clLogError("MSO", "UTIL", "Failed to get moid from moid name. rc [0x%x]", rc);
            clHeapFree(resourcesArray.pResources);
            clHeapFree(*ppMoIdList);
            return rc;
        }

        rc = clCorMoIdToMoClassPathGet(&moId, &moPath);
        if (rc != CL_OK)
        {
            clLogError("MSO", "UTIL", "Failed to get mo class path from moId. rc [0x%x]", rc);
            clHeapFree(resourcesArray.pResources);
            clHeapFree(*ppMoIdList);
            return rc;
        }

        rc = clCorMSOClassExist(&moPath, svcId);
        if (rc != CL_OK)
        {
            continue;
        }

        rc = clCorMoIdServiceSet(&moId, svcId);
        if (rc != CL_OK)
        {
            clLogError("MSO", "UTIL", "Failed to set service-id in the moId. rc [0x%x]", rc);
            clHeapFree(resourcesArray.pResources);
            clHeapFree(*ppMoIdList);
            return rc;
        }

        memcpy(&(*ppMoIdList)->moId[(*ppMoIdList)->moIdCnt], &moId, sizeof(ClCorMOIdT));

        (*ppMoIdList)->moIdCnt++;
    }

    clHeapFree(resourcesArray.pResources);

    return CL_OK;
}

/* This function used to get the resource information from the rt.xml.
 *  1. Get the necessary environment variables to identify the rt.xml
 *  2. Pass the information about the rt.xml to OAMP parser utility function.
 *  3. Get the resource information.
 */
ClRcT
clMsoConfigResourceInfoGet( ClCorAddrT* pCompAddr,
                            ClOampRtResourceArrayT* pResourcesArray )
{
    ClRcT           rc                              = CL_OK;

    pCompAddr->nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet( &pCompAddr->portId );
    if (rc != CL_OK)
    {
        clLogError("MSO", "UTIL", "Failed to get the EO port. rc [0x%x]", rc);
        return rc;
    }

    rc = _clMsoConfigResourceArrayGet(pResourcesArray);
    if (rc != CL_OK)
    {
        clLogError("MSO", "UTIL", 
                "Failed to get the resources array for the application [0x%x:0x%x].",
                pCompAddr->nodeAddress, pCompAddr->portId);
        return rc;
    }

    return CL_OK;
}
