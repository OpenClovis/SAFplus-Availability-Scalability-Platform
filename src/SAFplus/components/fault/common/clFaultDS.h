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
 * ModuleName  : fault
 * File        : clFaultDS.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides the structure of the fault record
 *				and fault event
 *
 *
 *****************************************************************************/
						

#ifndef _CL_FAULT_D_S_H_
#define _CL_FAULT_D_S_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/*
 * Index numbers for various 
 * fault categories
 */

#define CL_FM_CAT_COM_X     	0
#define CL_FM_CAT_QOS_X     	1
#define CL_FM_CAT_PROC_X        2
#define CL_FM_CAT_EQUIP_X       3
#define CL_FM_CAT_ENV_X     	4

/*
 * Index numbers for various 
 * fault severities 
 */
#define CL_FM_SEV_CR_X          0
#define CL_FM_SEV_MJ_X          1
#define CL_FM_SEV_MN_X          2
#define CL_FM_SEV_WR_X          3
#define CL_FM_SEV_IN_X          4
#define CL_FM_SEV_CL_X          5
	
/*
 * Internal values for various 
 * fault categories
 */
#define CL_FM_CAT_COM_INTERNAL			0x80	
#define CL_FM_CAT_QOS_INTERNAL			0x40	
#define CL_FM_CAT_PROC_INTERNAL			0x20	
#define CL_FM_CAT_EQUIP_INTERNAL		0x10	
#define CL_FM_CAT_ENV_INTERNAL			0x08	

/*
 * Internal values for various 
 * fault severities
 */
#define CL_FM_SEV_CR_INTERNAL			0x20 	
#define CL_FM_SEV_MJ_INTERNAL			0x10
#define CL_FM_SEV_MN_INTERNAL			0x08	
#define CL_FM_SEV_WR_INTERNAL			0x04 	
#define CL_FM_SEV_IN_INTERNAL			0x02	
#define CL_FM_SEV_CL_INTERNAL			0x01
	
#ifdef __cplusplus
}
#endif

#endif		/* _CL_FAULT_D_S_H_ */
