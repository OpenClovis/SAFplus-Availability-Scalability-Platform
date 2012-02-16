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
 * ModuleName  : cor
 * File        : clCorMoIdToSlotTableClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * MOId to Logical slot live table - Client APIs.
 *****************************************************************************/
#include <clCorClient.h>
#include <clCorErrors.h>
 
ClRcT clCorMoIdToLogicalSlotGet(ClCorMOIdPtrT pMoId, ClUint32T* logicalSlot)
{
	return CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
}

ClRcT clCorLogicalSlotToMoIdGet(ClUint32T logicalSlot, ClCorMOIdPtrT  pMoId)
{
	return CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
}

