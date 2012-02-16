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
 * ModuleName  : prov
 * File        : clProvLogMsgIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Provision log related strings
 *
 *
 *****************************************************************************/
#ifndef _CL_PROV_LOG_MSG_IPI_H_
#define _CL_PROV_LOG_MSG_IPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "clProvMainIpi.h"

    ClCharT*                gProvLogMsgDb[] =
    {
        "Not able to get the MOID from OM handle RC : [0x%x]  ", /* 0 */
        "Not able to set the rule status RC : [0x%x]  ", /* 1 */
        "MOID alloca failed RC : [0x%x] ", /* 2 */
        "MOID Initalize failed RC : [0x%x] ", /* 3 */
        "Error while getting MO class RC : [0x%x]   ", /* 4 */
        "Error while creating OM object RC : [0x%x]  ", /* 5 */
        "Error while adding OM handle into containner RC : [0x%x]", /* 6 */
        "Error while inserting MAP from MOID to om ID RC : [0x%x]  ", /* 7 */
        "Error while getting OM handle RC : [0x%x] ", /* 8 */
        "MOID to OM get failed RC : [0x%x] ", /* 9 */
        "Failed to set attribute @ validate phase  RC : [0x%x] ", /* 10 */
        "Failed to set attribute @ update phase  RC : [0x%x] ", /* 11 */
        "Failed to set attribute @ prevalidate phase  RC : [0x%x] ", /* 12 */
        "Failed to get MOID from obj handle RC : [0x%x] ", /* 13 */
        "Failed to prepare the OM Object RC : [0x%x] ", /* 14 */
        "Error while crating container RC : [0x%x] ", /* 15 */
        "Error while getting OM class type RC : [0x%x]  ", /* 16 */
        "Not able to get EO object RC : [0x%x] ", /* 17 */
        "Error while creating Object RC : [0x%x] ", /* 18 */
        "Service Set failed RC : [0x%x] ", /* 19 */
        "Failed to finalize PROV OM RC : [0x%x] ", /* 20 */
        "Unable to get node MOID RC : [0x%x] ", /* 21 */
        "Not able to get port information RC : [0x%x] ", /* 22 */
        "Error while getting component handle RC : [0x%x] ", /* 23 */
        "Error while getting component name RC : [0x%x] ", /* 24 */
        "Error while getting resource information RC : [0x%x] ", /* 25 */
        "Error while getting node MOID RC : [0x%x] ", /* 26 */
        "Error while converting from nodeMOID to name of the MOID RC : [0x%x] ", /* 27 */
        "Error while adding PM service to cor RC : [0x%x] ", /* 28 */
        "Failed to Initialize PROV OM RC : [0x%x] ", /* 29 */
        "Error while getting MoId from string MoId RC : [0x%x] ", /* 30 */
        "Provision Init done", /* 31 */
        "Provision Cleanup done", /* 32 */
        "Failed to create object @ update phase  RC : [0x%x] ", /* 33 */
        "Failed to get OM object reference from moId RC : [%0xx] ", /* 34 */
        "Failed while getting the parameters of the attribute job. rc[0x%x]", /* 35 */
        "Failed to rollback object creation.  rc : [0x%x]", /* 36 */
        "Failed to rollback SET operation. rc : [0x%x]", /*37 */
        "Failed to validate CREATE/CREATE AND SET operation. rc : [0x%x]", /* 38 */
        "Failed to validate DELETE operation. rc : [0x%x]", /* 39 */
        "Failed to rollback DELETE operation. rc : [0x%x]", /* 40 */
        "Failed to update DELETE operation. rc : [0x%x]" /* 41 */
    };


#ifdef __cplusplus
}
#endif

#endif /* _CL_PROV_LOG_MSG_IPI_H_ */

