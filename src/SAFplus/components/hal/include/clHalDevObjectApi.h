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
 * ModuleName  : hal
 * File        : clHalDevObjectApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/



/*********************************************************************************/
/****************************** HAL APIs *****************************************/
/*********************************************************************************/
/*                                                                               */
/* pageHAL201 : clHalDoStateSet                                                  */
/* pageHAL202 : clHalDoStateGet                                                  */
/* pageHAL203 : clHalDoInitialize                                                */
/*                                                                               */
/*********************************************************************************/


#ifndef _CL_HAL_DEV_OBJECT_API_H_
#define _CL_HAL_DEV_OBJECT_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <clCommon.h>

/**
 * Function prototype of Operations which will act upon actual devices. 
 */
typedef ClRcT (*ClfpDevOperationT)(

/**
 * Identifier of the MSO. It is a combination of the class Id and instance Id.
 */ 
                                 CL_IN ClUint32T omId, 
/**
 * Id of the MO.
 */
                                 CL_IN ClCorMOIdPtrT moId,
/**
 * Sub-operation to further qualify the operation.
 */
                                 CL_IN ClUint32T subOperation,
/**
 * Pointer to the user-data.
 */
                                 CL_IN void *pUserData,
/**
 * Length of the user-data.
 */
                                 CL_IN ClUint32T usrDataLen);


/** 
 * Handle of the Device Object.
 */
typedef ClUint32T ClHalDeviceObjectH ;

typedef struct ClHalDevObject
{

/**
 * Identifier to \e deviceType provided by Reference Application.
 */
    ClUint32T deviceId ; 

/**
 * Pointer to Device Capability.
 */ 
    void * pDevCapability ;

/**
 * Length of Device Capability Buffer.
 */
    ClUint32T devCapLen ;

/**
 *  Maximum response time, in milliseconds, for any operation.
 */ 
    ClUint32T maxRspTime ;

/**
 *  Boot up priority used by Boot Manager.
 */ 
    ClUint32T bootUpPriority;

/**
 *  Array of device operations of size \c HAL_DEV_NUM_OPERATIONS.
 */
    ClfpDevOperationT * pDevOperations;
                                    
}ClHalDevObjectT;


/**
 ************************************
 *  \page pageHAL201 clHalDoStateSet
 *
 *  \par Synopsis:
 *  Sets the Hardware state of the Device Object.
 *
 *  \par Description:
 *  This API is used to set the Hardware state of the Device Object . Clovis
 *  does not define any state model for the DO . The state machine for the DO can
 *  be designed by application as per it's requirement. 
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalDoStateSet(
 *                          CL_IN ClHalDeviceObjectH devObjHandle,
 *                          CL_IN ClUint32T state);
 *  \endcode
 *
 *
 *  \param hDevObjHandle: Handle of the Device Object.
 *  \param state: Hardware state of the Device Object.
 *
 *  \par Return values:
 *  None
 *
 */
ClRcT clHalDoStateSet(CL_IN ClHalDeviceObjectH devObjHandle,
                      CL_IN ClUint32T state);

/**
 ************************************
 *  \page pageHAL202 clHalDoStateGet
 *
 *  \par Synopsis:
 *  Retrieves the hardware state of the DO.
 *
 *  \par Description:
 *  This API is used to retrieve the Hardware state of the Device Object (DO).
 *  The state machine for the DO can  be designed by application as per it's requirement. 
 *
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalDoStateGet(
 *                          CL_IN ClHalDeviceObjectH devObjHandle,
 *                          CL_OUT ClUint32T * pState);
 *  \endcode
 *
 *  \param devObjHandle: Handle of the Device Oject.
 *  \param pState: Hardware State of Device Object returned by the API.
 *
 *  \retval CL_OK: The API successfully executed.
 *  \retval CL_ERR_INVLD_HANDLE: On passing an invalid handle.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *
 */
ClRcT clHalDoStateGet(CL_IN ClHalDeviceObjectH devObjHandle,
                         CL_OUT ClUint32T * pState);


/**
 ************************************
 *  \page pageHAL203 clHalDoInitialize
 *
 *  \par Synopsis:
 *  Initializes the DO for a HAL Object.
 *
 *  \par Description:
 *  This API is used to initialise the Device Object for a given component. It calls the
 *  initialization API of the device driver. 
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalDoInitialize(
 *                          CL_IN void * pUserData,
 *                          CL_IN ClUint32T usrDataLen, 
 *                          CL_IN ClUint32T flags);
 *  \endcode
 *
 *  \param pUserData: User-data.
 *  \param usrDataLen: Length of user-data.
 *  \param flags: Flags. \c CL_HAL_CONTINUE_ON_ERR will make the API continue operations
 *  on other devices even if an error is encountered.
 *
 *  \retval CL_OK: The API successfully executed.
 *  \retval CL_ERR_NO_MEM: On memory allocation failure.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_ERR_INVLD_PARAM: On passing an invalid parameter.
 *
 */
ClRcT clHalDoInitialize(CL_IN void * pUserData,
                            CL_IN ClUint32T usrDataLen, 
                            CL_IN ClUint32T flags);


#ifdef __cplusplus
}
#endif

#endif /* _CL_HAL_DEV_OBJECT_API_H_ */ 

