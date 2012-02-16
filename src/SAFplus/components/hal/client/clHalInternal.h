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
 * File        : clHalInternal.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * HAL provides the API's which abstracts the underlying    
 * Hardware Devices making the calls by application objects like         
 * provisioning, alarm and performance manager decoupled from the H/w    
 * This file is the configuration file for HAL                           
 *************************************************************************/

#ifndef _HAL_INT_H_
 #define _HAL_INT_H_
#ifdef __cplusplus
 extern "C" {
#endif
#include <clHalApi.h>
#include "clCntApi.h"

typedef enum 
{
    HAL_DEV_INITIALISED,
    HAL_DEV_NOT_PROVISIONED,
}HalDevState_e ;

typedef struct HalDeviceObject
{
    ClUint32T deviceId ;
    /*To keep track of how many objects have registered with this device*/
    ClUint32T refCount ; 
    void *pdevCapability ;
    ClUint32T devCapLen ;
    ClUint32T state ;
    ClUint32T maxRspTime ;
    ClUint32T bootUpPriority; 
    ClfpDevOperationT *pdevOperations;
}HalDeviceObjectT;

typedef struct HalDevBoot{
    ClUint32T devHandle;
    ClUint32T devBootUpPriority;
}HalDevBootT;

 typedef struct HalDeviceObjTable
{
    HalDeviceObjectT **pphalDeviceObj;
    ClUint32T nextIndex ; 
}HalDeviceObjTableT ;

typedef struct HalObject 
{
    ClUint32T omId; 
    ClCorMOIdPtrT     moId;
    ClUint32T maxRspTime ;
    ClUint32T numDevObj ; 
    ClCntHandleT hTableRefDevObject;    
}HalObjectT;
/* Internal APIs of Dev Object used by Hal Object */
ClRcT halDevObjCapLenGet  (ClHalDeviceObjectH hDevObjHandle ,
                              ClUint32T *pDevCapLen );

ClRcT halDevObjCapGet(ClHalDeviceObjectH hDevObjHandle ,
                          ClUint32T devCapLen,
                          void * pDevCap);

ClRcT halDevObjMaxRspTimeGet(ClHalDeviceObjectH hDevObjHandle ,
                              ClUint32T *pMaxRspTime); 

ClRcT halDevObjRefCountIncr(ClHalDeviceObjectH hDevObjHandle);

ClRcT halDevObjRefCountDecr(ClHalDeviceObjectH hDevObjHandle);

ClRcT halDevObjIdGet (ClHalDeviceObjectH hDevObjHandle,
                       ClUint32T *pDeviceId);

ClRcT halDevObjOperationExecute(ClHalDeviceObjectH hDevObjHandle, 
                                 ClUint32T omId,
                                 ClCorMOIdPtrT moId,
                                 ClUint32T operation,
                                 ClUint32T subOperation,
                                 void *pUserData, ClUint32T usrDataLen);

/**
* This API creates the Hal Device Object. 
*
* @param - deviceID - Identifier to deviceType provided by Ref App. 
* @param - pdevCapability- Pointer to Device Capability. 
* @param - devCapLen - Length of Device Capability Buffer. 
* @param - maxRspTime - Maximum response time for any operation.
* @param - bootUpPriority - Boot up priority used by boot mgr.
* @param - phalDevObject - Pointer to handle of Device Object returned by the 
*                          API.
* 
* @return - ClRcT 
*/
ClRcT halDevObjCreate(ClUint32T deviceID,
                       void *pdevCapability, 
                       ClUint32T devCapLen,
                       ClUint32T maxRspTime,
                       ClUint32T bootUpPriority,
                       ClHalDeviceObjectH *const phalDevObj);

   
/**
* This API creates the Hal Device Object and also registers handlers for 
* operations to be performed.
*
* @param - phalDevObject - structre containing all the fileds of  devobject 
* @param - phalDevObjHandle - Pointer to handle of Device Object returned by
*                             the API.
* @return - ClRcT 
*/
ClRcT halDevObjCreateHandlersReg(ClHalDevObjectT * phalDevObject,
                                  ClHalDeviceObjectH *const phalDevObjHandle);

/**
* This API takes takes the deviceID and returns device Handle.
*
* @param - deviceID - Identifier to deviceType provided by Ref App. 
* @param - phalDevObject - Pointer to handle of Device Object returned by  
*               API.
* @return - ClRcT 
*/
ClRcT halDevObjHandleGet(ClUint32T deviceID ,
                          ClHalDeviceObjectH *phalDevObjHandle);
/**
* This API takes takes the device Handle and returns deviceID  .
*
* @param - hDevObjHandle- Handle of Device Object returned by the
*                         halDevObjCreate API 
* @param - pDeviceID - Identifier to deviceType returned by the API. 
* @return - ClRcT 
*/
ClRcT halDevObjIdGet(ClHalDeviceObjectH hDevObjHandle,
                      ClUint32T *pDeviceId);
/**
* This API deletes the Device Object.
*
* @param - hDevObjHandle - Handle to the Device Object.
* 
* @return ClRcT
*/
ClRcT halDevObjDelete( ClHalDeviceObjectH hDevObjHandle );

/**
* This API registers the Function for the operation with the  Device Object.
*
* @param - halDevObj - Handle to the Device Object.
* @param - operation - Operation to be performed on device ,
*                       this id is from halConf.h
* @param - fpOperation - Function which will called by the device Object for 
*                        the specified operation.
* @return ClRcT.
*/
ClRcT halDevObjHandlerRegister(ClHalDeviceObjectH hDevObj,
                                ClUint32T operation, 
                                ClfpDevOperationT fpOperation);

/**
* This API Creates the Device Objects from the Configuration Information
* for the blade . 
*
* @return - ClRcT.
*/
ClRcT halDevObjTableCreate();
                                
/**
* This API created the HAL object corresponding to OM Id.   
*
* It returns the Hal Object Handle which is used for all interactions
* with HAL . 
*
*@param - omId  - To identify the MSO .  
*@param - moId  -   
*@param - phalObjHandle- Handle to HAL object returned. 
*
*@return -ClRcT
**/
ClRcT halObjCreate (ClUint32T omId ,
                    ClCorMOIdPtrT moId ,
                    ClHalObjectHandleT *const phalObjHandle);

/**
* This API is used to add refrences of the device objects to the Hal Object.
* Device objects will be acted upon by operations done to the Hal Object .
*
*@param - hHalObjHandle- Handle to Hal Object. 
*@param - deviceId - Unique identifier to identify the device . 
*@param - deviceAccessPriority - Order in which device will be accessed. This
*         parameter is of importance while initializing/powering down 
*         the Hal Object. 
*
*@return -ClRcT
*/
ClRcT halObjDevObjectInstall(ClHalObjectHandleT hHalObjHandle,
                              ClUint32T deviceId,
                              ClUint32T deviceAccessPriority);

/**
* This API is used to remove refrences of the device objects to the Hal Object.t
*
*@param - hHalObjHandle- Handle to Hal Object. 
*@param - deviceId  Unique identifier to identify the device . 
*
*@return -ClRcT
*/
ClRcT halObjDevObjectUnInstall(ClHalObjectHandleT hHalObjHandle, 
                                ClUint32T deviceId);


#ifdef __cplusplus
}
#endif
#endif 

