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
 * ModuleName  : SystemTest                                                     
* File        : clCmResourceDeps.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * THis file contains the resource and its dependencies of the platform
 * hardware.
 *****************************************************************************/

#include <clCmApi.h>
#ifdef CL_USE_CHASSIS_MANAGER
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
#endif
