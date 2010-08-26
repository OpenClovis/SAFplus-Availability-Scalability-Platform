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
 * File        : clHalApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/



/*********************************************************************************/
/****************************** HAL APIs *****************************************/
/*********************************************************************************/
/*                                                                               */
/* pageHAL101 : clHalLibInitialize                                               */
/* pageHAL102 : clHalLibFinalize                                                 */
/*                                                                               */
/*********************************************************************************/



#ifndef _CL_HAL_API_H_
#define _CL_HAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 *  \page pageHAL Hardware Abstraction Layer (HAL)
 *
 *  \par Overview
 * 
 *  The Clovis Hardware Abstraction Layer (HAL) is designed to allow
 *  our software components to generalize access to all hardware platforms.
 *  HAL is capable of the following:
 *  \arg Abstracts various devices:  NP, DSP, ASIC, framer, LIU.
 *  \arg Abstracts uP interfaces: memory mapped, DMA, parallel and serial interfaces.
 *  \arg Provides unified system and Hardware interactions. 
 *  \arg Centralized hardware access. 
 *  \arg Allows easy cost: reduction for hardware device swaps. 
 *  \arg Provides a dynamic object container to send and receive data from all hardware. 
 *  \arg Virtual chip enables fully testing of control plane functionality before 
 *  hardware arrives as well as ongoing control plane regression testing. 
 *  \arg Provides debugging facilities in the simulation environment for 
 *  each hardware device driver.
 * 
 *  The HAL architecture is composed of the <em>HAL object</em> and its 
 *  <em>device objects</em>.  HAL object is an entity which encapsulates and 
 *  exposes the functionalities of one or more hardware devices to the 
 *  Clovis software objects, i.e. provisioning objects, alarm objects, etc. 
 *  A HAL object can have one or more device objects attached to it.  A 
 *  device object is a layer of software which interacts directly with the 
 *  hardware device, as such, it comprises of a device driver and HAL 
 *  primitives (i.e. fundamental hardware operations). Using the HAL object 
 *  APIs these hardware operations are possible:
 *  \arg initializes 
 *  \arg power control 
 *  \arg opens 
 *  \arg closes 
 *  \arg controls 
 *  \arg retrieves 
 *  \arg direct access to the hardware interface 
 *  \arg sends 
 *  \arg receives 
 *  \arg image downloads 
 *
 * \par Interaction with other components
 * The HAL can interact with any software components that require hardware 
 * access.
 *
 *
 */
#include <clCommon.h>
#include <clOmBaseClass.h>
#include <clCommonErrors.h>
#include <clHalObjectApi.h>
#include <clHalDevObjectApi.h>
#include <clHalError.h>

typedef enum
{

/**
 * Initialises the device.
 */
    CL_HAL_DEV_INIT=0,
/**
 * Returns the exclusive lock on the device.
 */
    CL_HAL_DEV_OPEN,
/**
 * Returns the release lock on the device.
 */
    CL_HAL_DEV_CLOSE,
/**
 * Reads from the device.
 */
    CL_HAL_DEV_READ,
/**
 * Writes to the device.
 */
    CL_HAL_DEV_WRITE, 
/**
 * Performs a cold boot on the device.
 */
    CL_HAL_DEV_COLD_BOOT,
/**
 * Performs a warm boot on the device.
 */
    CL_HAL_DEV_WARM_BOOT,
/**
 * To power off the device.
 */
    CL_HAL_DEV_PWR_OFF,
/**
 * Downloads the new image for the device.
 */
    CL_HAL_DEV_IMAGE_DN_LOAD, 
/**
 * Enum to access the devices directly ,bypassing the the HAL.
 */
    CL_HAL_DEV_DIRECT_ACCESS
}ClHalDevOperationT;

/**
 * You can extend the number of operations in a separate header file.
 */
#define CL_HAL_DEV_NUM_STD_OPERATIONS (CL_HAL_DEV_DIRECT_ACCESS) /*<Number of Std  operations */

typedef struct ClHalConf
{
/**
 * Total number of operations supported.
 */
    ClUint32T halDevNumOperations; 

/**
 * Number of Device Objects for the process.
 */
    ClUint32T halNumDevObject;

/**
 * Table of Device Objects for the process.
 */
    ClHalDevObjectT * pDevObjectTable ;

/**
 * Number of HAL Objects for the process.
 */
    ClUint32T halNumHalObject;

/**
 * Table of HAL Objects for the process.
 */
    ClHalObjectConfT * pHalObjConf;
}ClHalConfT;

/**
 * The symbol below will be populated by the application using a separate C
 * file, this symbol is used by HAL library for resource allocation.
 */
extern ClHalConfT appHalConfig;


/*****************************************************************************
 *  HAL APIs
 *****************************************************************************/


/**
 ************************************
 *  \page pageHAL101 clHalLibInitialize
 *
 *  \par Synopsis:
 *  Initializes the HAL.
 *
 *  \par Description:
 *  This API is used to initialize the Hardware Abstraction (HAL) library. 
 *  It must be invoked before calling any other HAL APIs.
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalLibInitialize();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NO_MEM: On Memory allocation failure.
 *  \retval CL_ERR_INVLD_STATE: If object state is invalid.
 *
 */
ClRcT clHalLibInitialize();



/**
 ************************************
 *  \page pageHAL102 clHalLibFinalize
 *
 *  \par Synopsis:
 *  Cleans up HAL.
 *
 *  \par Description:
 *  This API is used to clean up all the resources allocated by the HAL library. 
 *  It must be invoked when the component is terminating . This will free all
 *  the resources that HAL Library has allocated during initialization.  
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalLibFinalize();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: On passing invalid device handle.
 *  \retval CL_ERR_NULL_POINTER: If NULL pointer is passed.
 *  \retval CL_ERR_INVLD_STATE: If object state is invalid.
 *
 */
ClRcT clHalLibFinalize();



#ifdef __cplusplus
}
#endif 
#endif /* _CL_HAL_API_H_ */
