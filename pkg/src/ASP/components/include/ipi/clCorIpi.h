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
 * ModuleName  : include
 * File        : clCorIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the typedefs, structures definitions and the ipi declartions.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_IPI_H_
#define _CL_COR_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include "xdrClCorAlarmConfiguredDataIDLT.h"
#include "xdrClCorAlarmResourceDataIDLT.h"
#include "xdrClCorAlarmProfileDataIDLT.h"


typedef enum ClCorDelayRequestOpt
{
    CL_COR_DELAY_INVALID_REQ = -1,
    CL_COR_DELAY_DM_UNPACK_REQ,             /* Give the time taken while doing dm data upack at standby COR (0) */
    CL_COR_DELAY_MOCLASSTREE_UNPACK_REQ,    /* Time taken while unpacking mo class tree data at standby COR (1) */
    CL_COR_DELAY_OBJTREE_UNPACK_REQ,        /* Time taken while unpacking the object tree at standy COR (2)     */
    CL_COR_DELAY_ROUTELIST_UNPACK_REQ,      /* Time taken while unpacking the route list at standby COR (3)     */
    CL_COR_DELAY_NITABLE_UNPACK_REQ,        /* Time taken while unpacking the ni table at standby COR (4)       */
    CL_COR_DELAY_SYNCUP_STANDBY_REQ,        /* Total time taken to syncup the data by standby COR (5)           */
    CL_COR_DELAY_DM_PACK_REQ,               /* Time taken to pack the dm data at active COR (6)                 */
    CL_COR_DELAY_MOCLASSTREE_PACK_REQ,      /* Time taken to pack the mo class tree at active COR (7)           */
    CL_COR_DELAY_OBJTREE_PACK_REQ,          /* Time taken to pack the object tree at active COR (8)             */
    CL_COR_DELAY_ROUTELIST_PACK_REQ,        /* Time taken to pack the route list at the active COR (9)          */
    CL_COR_DELAY_NITABLE_PACK_REQ,          /* Time taken to pack the name interface table at active COR (10)   */
    CL_COR_DELAY_SYNCUP_ACTIVE_REQ,         /* Total time taken to syncup the data at the active COR (11)       */
    CL_COR_DELAY_OBJTREE_DB_RESTORE_REQ,    /* Time taken to restore the object tree from the database (12)     */
    CL_COR_DELAY_ROUTELIST_DB_RESTORE_REQ,  /* Time taken to restore the route list from the database (13)      */
    CL_COR_DELAY_DB_RESTORE_REQ,            /* Total time taken to restore the object and route list from Db(14)*/
    CL_COR_DELAY_OBJ_DB_RECREATE_REQ,       /* Time taken to recreate the object db at the standby COR (15)     */
    CL_COR_DELAY_ROUTE_DB_RECREATE_REQ,     /* Time taken to recreate the route db at the standby COR (16)      */
    CL_COR_DELAY_DB_RECREATE_REQ,           /* Total time taken to recreate object and route db at slave COR    */
    CL_COR_DELAY_MAX_REQ
}ClCorDelayRequestOptT;


/**
 *  Get the time delays from COR
 * 
 *  Function to get the time delays in certain situations from cor active and standby.
 *  This function sends some information from active COR and in some situations ask
 *  standby COR to send the information back.
 * 
 * /par 
 *  opType : The parameter to determine the information which is needed.
 *  ptime : This parameter will be filled by the client after getting information from server.
 *  
 * /return 
 *  CL_COR_ERR_NULL_PTR : null pointer passed for pTime.
 *  CL_COR_ERR_INVALID_PARAM : invalid value of opType passed.
 */ 
ClRcT clCorOpProcessingTimeGet (CL_IN ClCorDelayRequestOptT opType, CL_OUT ClTimeT* pTime);

/**
 * Function used alarm to configure the Msos.
 */
ClRcT clCorAlarmInfoConfigure(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmInfo);

#ifdef __cplusplus
}
#endif

#endif /* _CL_COR_IPI_H_*/
