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
 * ModuleName  : hal
 * File        : clHalError.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_HAL_ERROR_H_
#define _CL_HAL_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include "clCommonErrors.h"
/* DEFINES */
/*HAL efinitions for HAL */
#define CL_DEV_NOT_INSTALLED (CL_ERR_COMMON_MAX +1)

#define CL_HAL_SET_RC(ERR_ID)	(CL_RC(CL_CID_HAL, ERR_ID))

#define CL_IS_CONTINUE_ON_ERR(flags)  ((flags&CL_HAL_CONTINUE_ON_ERR)==CL_HAL_CONTINUE_ON_ERR)

/* FORWARD DECLARATION */

#ifdef __cplusplus
}
#endif
#endif	/*	 _CL_HAL_ERROR_H_ */

