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
 * ModuleName  : om
 * File        : clOmCommonClassTypes.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Add brief description of module
 *
 *
 *****************************************************************************/
#ifndef _CL_OM_COMMON_CLASS_TYPES_H
#define _CL_OM_COMMON_CLASS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

#define	CL_OM_INVALID_CLASS	0
typedef ClInt32T	ClOmClassTypeT;

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
#define CL_OM_CMN_CLASS_TYPE_START		1

/* Object type enumerations for the common OM classes */
#define CL_OM_BASE_CLASS_TYPE			(CL_OM_CMN_CLASS_TYPE_START+0)	/*  */
#define	CL_OM_PROV_CLASS_TYPE		(CL_OM_CMN_CLASS_TYPE_START+1)	/*   */
#define CL_OM_CM_CLASS_TYPE			(CL_OM_CMN_CLASS_TYPE_START+2)	/*   */
#define CL_OM_ALARM_CLASS_TYPE		(CL_OM_CMN_CLASS_TYPE_START+3)	/*   */
#define CL_OM_FM_CLASS_TYPE			(CL_OM_CMN_CLASS_TYPE_START+4)	/*   */
/* 	
	Needed always, MAXIMUM NUMBER OF COMMON OM CLASS TYPES.
	Note: PLEASE ADD ENTRIES ABOVE THIS LINE ONLY, and DO NOT
	forget to increment!!!
*/
#define CL_OM_CMN_CLASS_TYPE_END	(CL_OM_CMN_CLASS_TYPE_START+7)	/*  */

#ifdef __cplusplus
}
#endif

#endif /* _CL_OM_COMMON_CLASS_TYPES_H_ */
