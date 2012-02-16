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
 * ModuleName  : name
 * File        : clNameEventIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains Name Service Server Side module 
 * functionality related publishing of events whenever there is a  
 * change in NS DB.                                                      
 *
 *
 *****************************************************************************/


#ifndef _CL_NAME_EVENT_IPI_H_
#define _CL_NAME_EVENT_IPI_H_
                                                                                                                             
#ifdef __cplusplus
extern "C" {
#endif
#include "clCommon.h"
#include "clEoApi.h"
#include "clEventApi.h"


#define CL_NAME_PUB_NAME       "NAME_SVC_PUB_NAME"

                                                                                                                             
void nameSvcEventFinalize();
ClRcT nameSvcEventInitialize();


#ifdef __cplusplus
}
#endif
                                                                                                                             
#endif /* _CL_NAME_EVENT_IPI_H_ */

