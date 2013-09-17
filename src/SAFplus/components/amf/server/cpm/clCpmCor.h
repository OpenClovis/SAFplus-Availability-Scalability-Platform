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

/**
 * This header file contains definitions of all the data structures
 * and programming interface used for CPM-COR interaction.
 */

#ifndef _CL_CPM_COR_H_
#define _CL_CPM_COR_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP include files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clAmsTypes.h>

extern ClRcT cpmCorNodeObjectCreate(SaNameT nodeMoIdName);

extern ClRcT cpmCorMoIdToMoIdNameGet(ClCorMOIdT *moId, SaNameT *moIdName);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_COR_H_ */
