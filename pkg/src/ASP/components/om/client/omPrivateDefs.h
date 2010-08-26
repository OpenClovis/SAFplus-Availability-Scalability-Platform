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
 * File        : omPrivateDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the definitions for the Object Model's               
 * private methods.                                                          
 *                                                                           
 * FILES_INCLUDED:                                                           
 *                                                                           
 * clCommon.h                                                           
 *                                                                           
 *****************************************************************************/

#ifndef _OM_PRIVATE_DEFS_H_
# define _OM_PRIVATE_DEFS_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <clCommon.h>
#include <clOmApi.h>

extern ClRcT omAddObj(ClOmClassTypeT classId, ClUint32T numInstances, ClHandleT *handle, void *pObjPtr);

extern ClRcT omRemoveObj(ClHandleT handle, ClUint32T numInstances);

extern ClRcT omValidateHandle(ClHandleT handle);

extern ClRcT omClassTypeValidate(ClOmClassTypeT classType);

extern ClUint32T omNumOfCmnClassEntriesGet(void);

extern ClOmClassControlBlockT * omCmnClassTblGet(void);

# ifdef __cplusplus
}
# endif

#endif                          /* _OM_PRIVATE_DEFS_H_ */
