/*
 * 
 *   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
 * 
 *   The source code for this program is not published or otherwise divested
 *   of its trade secrets, irrespective of what has been deposited with  the
 *   U.S. Copyright office.
 * 
 *   No part of the source code  for this  program may  be use,  reproduced,
 *   modified, transmitted, transcribed, stored  in a retrieval  system,  or
 *   translated, in any form or by  any  means,  without  the prior  written
 *   permission of OpenClovis Inc
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : SystemTest                                                     
* File        : clCmResourceDeps.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * THis file contains the resource and its dependencies of the platform
 * hardware.
 *****************************************************************************/

#include <clCmApi.h>
#include <SaHpi.h>

/* Following is the list of entities affected by sensor assertion on fan */
SaHpiEntityPathT impactedEntitiesByFan[]=
{
    {{{SAHPI_ENT_SYSTEM_BOARD,1},{SAHPI_ENT_PHYSICAL_SLOT,1},{SAHPI_ENT_SYSTEM_CHASSIS,7}}},
    {{{SAHPI_ENT_SYSTEM_BOARD,2},{SAHPI_ENT_PHYSICAL_SLOT,2},{SAHPI_ENT_SYSTEM_CHASSIS,7}}},
    {{{SAHPI_ENT_UNSPECIFIED,0}}}
};

ClCmEventCorrelatorT clCmEventCorrelationConfigTable [] = 
{
    {
        {{{SAHPI_ENT_FAN,1},{SAHPI_ENT_SYSTEM_CHASSIS,7}}},
        impactedEntitiesByFan
    },
    {
        {{{SAHPI_ENT_UNSPECIFIED,0}}},
        (SaHpiEntityPathT *)NULL
    }
};
ClRcT systemBoardCallBack(SaHpiSessionIdT sessionid , 
                          SaHpiEventT *pEvent , 
                          SaHpiRptEntryT  *pRptentry, 
                          SaHpiRdrT *pRdr)
{

    return CL_OK;
}
ClCmUserPolicyT cmUserPolicy[]=
{
    {
        {{{SAHPI_ENT_SYSTEM_BOARD,1},{SAHPI_ENT_PHYSICAL_SLOT,1},{SAHPI_ENT_SYSTEM_CHASSIS,7}}},
        systemBoardCallBack
    },
    {
        {{{SAHPI_ENT_SYSTEM_BOARD,2},{SAHPI_ENT_PHYSICAL_SLOT,2},{SAHPI_ENT_SYSTEM_CHASSIS,7}}},
        systemBoardCallBack
    },
    {
        {{{SAHPI_ENT_UNSPECIFIED,0}}},
        (ClCmEventCallBackT)NULL
    }
};
