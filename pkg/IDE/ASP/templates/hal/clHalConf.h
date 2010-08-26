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
 * File        : clHalConf.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is a sample header for hal libraryconfiguration. 
 *
 *
 *****************************************************************************/

#ifndef _CL_HAL_CONF_H_
#define _CL_HAL_CONF_H_

#include <clCommon.h>
#include <clHalApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List of user specified operations for HAL.
 * The user can add his operations here depending on the requirement.   
 * In this example user has not added any operation to the default List defined 
 * in clHalApi.h
 */
#define USER_DEFINED_OP_ADDED       CL_FALSE

#if USER_DEFINED_OP_ADDED == CL_TRUE
    typedef enum
        {
            HAL_DEV_USER_DEFINED_OPERATION = CL_HAL_DEV_NUM_STD_OPERATIONS + 1
        }ClHalDevUserOperationT;
    #define HAL_DEV_NUM_OPERATIONS  (HAL_DEV_USER_DEFINED_OPERATION+1)
#else
    #define HAL_DEV_NUM_OPERATIONS (HAL_DEV_NUM_STD_OPERATIONS+1)
#endif
/**
 * The Device Id of the Ethernet Device Object , the name space of device Id is the
 * process.This implies that device Id needs to be unique in a process context but
 * same device ID could be used to address two different devices provided they are 
 * managed by different components/processes. 
 */
#define ETH_DEV_OBJECT_ID       1
    
#ifdef __cplusplus
}
#endif

#endif /* _CL_HAL_CONF_H_ */

