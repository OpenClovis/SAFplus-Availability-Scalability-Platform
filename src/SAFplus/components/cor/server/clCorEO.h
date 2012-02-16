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
 * File        : clCorEO.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains EO use example related definitions 
 *
 *
 *****************************************************************************/

#ifndef _COR_EO_H_
#define _COR_EO_H_

# ifdef __cplusplus
extern "C"
{
# endif

/* INCLUDES */
#include <clCommon.h>
#include <clEoApi.h>
/* #include <clQueueApi.h> */
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clOampRtApi.h>

/* need a better way .. */

typedef struct corEO
{
	int someData;
	int state;
	ClQueueT someQ;
	ClOsalMutexIdT sem;
	ClEoExecutionObjT* EOId;
    int myUpTime;

} COREoData_t;

extern ClOampRtResourceArrayT *corComponentResourceGet(ClNameT *compName);

# ifdef __cplusplus
}
# endif

#endif /* _COR_EO_H_ */
