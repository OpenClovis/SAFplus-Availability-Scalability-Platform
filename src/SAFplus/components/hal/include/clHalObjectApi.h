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
 * File        : clHalObjectApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/



/*********************************************************************************/
/*************************** HAL Object APIs *************************************/
/*********************************************************************************/
/*                                                                               */
/*  pageHAL301  : clHalObjectDoPriorityGet                                       */
/*  pageHAL302  : clHalObjectDoPrioritySet                                       */
/*  pageHAL303  : clHalObjectNumDoGet                                            */
/*  pageHAL304  : clHalObjectOperate                                             */
/*  pageHAL305  : clHalObjectMaxRespTimeGet                                      */
/*  pageHAL306  : clHalObjectCapabilityLenGet                                    */
/*  pageHAL307  : clHalObjectCapabilityGet                                       */
/*                                                                               */
/*********************************************************************************/


#ifndef _CL_HAL_OBJECT_API_H_
#define _CL_HAL_OBJECT_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clOmApi.h>



/**
 * Enum to specify the access order of devices in case of a group access. 
 */
typedef enum {

/**
 * To access the devices in the increasing order.
 */
    CL_HAL_ACCESS_ORDER_INCR,

/**
 * To access the devices in the decreasing order.
 */
    CL_HAL_ACCESS_ORDER_DECR 
}ClHalAccessOrderT;

/**
 * Handle of the HAL Object.
 */
typedef ClPtrT ClHalObjectHandleT ;

/**
 * Flags to continue HAL operation if error is encountered on any device object.
 */
#define CL_HAL_CONTINUE_ON_ERR 0x0001

typedef struct  ClHalDevObjectInfo 
{

/**
 * Unique id of the device. 
 */
    ClUint32T deviceId;

/**
 * Access Priority of Device within HAL Object.
 */
    ClUint32T deviceAccessPriority;
                                        
}ClHalDevObjectInfoT;

/**
 * \e ClHalObjConfT is a structure for HAL Object Configuration, done in halCOnf.c file. 
 */
typedef struct ClHalObjectConf{

/**
 * Class Id of the OM class.
 */
    ClUint16T omClassId;                   
/**
 * String Representation of the MO.
 */ 
    ClUint8T strMOId[CL_MAX_NAME_LENGTH];   
/**
 * Pointer to the Device Object information.
 */
    ClHalDevObjectInfoT *pDevObjInfo;        

/**
 * Number of Device Objects associated with the HAL Object.
 */
    ClUint16T devObjects;                  
}ClHalObjectConfT ;

/**
 ************************************
 *  \page pagHAL301 clHalObjectDoPriorityGet
 *
 *  \par Synopsis:
 *  Returns the access priority of DO.
 *
 *  \par Description:
 *  This API is used to return the access priority of the Device Object (DO) in the
 *  HAL Object. Any operation on HAL Object translates to action on assosciated
 *  Device Objects. In case if there are multiple Device Objects assosciated
 *  with a HAL Object , access priority determines the order in which the
 *  operation will be performed on DO. 
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectDoPriorityGet(
 * 				CL_IN ClHalObjectHandleT halObjHandle, 
 * 				CL_IN ClUint32T deviceId,
 *                              CL_OUT ClUint32T *pDevAccessPriority);
 *  \endcode
 *
 *  \param halObjHandle: Handle of the HAL Object. 
 *  \param deviceId: Unique ID of the device . 
 *  \param pDevAccessPriority: Priority assosciated with the DO. 
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST: If the device is not found.
 *  \retval CL_DEV_NOT_INSTALLED: If the device is not installed.
 *
 */
ClRcT clHalObjectDoPriorityGet(CL_IN ClHalObjectHandleT halObjHandle, 
                            CL_IN ClUint32T deviceId,
                            CL_OUT ClUint32T *pDevAccessPriority);


/**
 ************************************
 *  \page pagHAL302 clHalObjectDoPrioritySet
 *
 *  \par Synopsis:
 *  Sets the access priority of DO.
 *
 *  \par Description:
 *  This API is used to set the access priority of the device object in the
 *  HAL Object.Any operation on HAL Object translates to action on assosciated
 *  Device Objects. In case if there are multiple Device Objects assosciated
 *  with a HAL Object , access priority determines the order in which the
 *  operation will be performed on DO. 
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectDoPrioritySet(
 * 				CL_IN ClHalObjectHandleT halObjHandle, 
 * 				CL_IN ClUint32T deviceId,
 *                              CL_IN ClUint32T devAccessPriority);
 *  \endcode
 *
 *  \param halObjHandle: Handle of the HAL Object. 
 *  \param deviceId: Unique id of the device. 
 *  \param pDevAccessPriority: Priority assosciated with the DO. 
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST: If the device is not found.
 *  \retval CL_DEV_NOT_INSTALLED: If the device is not installed.
 *
 */

ClRcT clHalObjectDoPrioritySet(CL_IN ClHalObjectHandleT halObjHandle, 
                               CL_IN ClUint32T deviceId,
                               CL_IN ClUint32T devAccessPriority);


/**
 ************************************
 *  \page pagHAL303 clHalObjectNumDoGet
 *
 *  \par Synopsis:
 *  Retrieves the number of DO.
 *
 *  \par Description:
 *  A Hal Object could be assosciated with multiple Device
 *  Objects . This API API is used to retrieve the number of Device Objects
 *  associated with the HAL Object
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectNumDoGet(
 * 			 	 CL_IN ClHalObjectHandleT halObjHandle,
 * 				 CL_OUT ClUint32T * pNumDevObject);
 *  \endcode
 *
 *  \param halObjHandle: Handle of the HAL Object. 
 *  \param pNumDevObject: Number of devices returned by the API. 
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *
 */
ClRcT clHalObjectNumDoGet(CL_IN ClHalObjectHandleT halObjHandle,
                          CL_OUT ClUint32T * pNumDevObject);



/**
 ************************************
 *  \page pagHAL304 clHalObjectOperate
 *
 *  \par Synopsis:
 *  Invokes the operation on the HAL Object.
 *
 *  \par Description:
 *  This API is used to invoke the actual operation on the Hal Object.Any 
 *  operation on HAL Object translates to action on assosciated
 *  Device Objects
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectOperate(
 * 			 	 CL_IN ClHalObjectHandleT halObjHandle,
 * 				 CL_IN ClUint32T operation,
 *                               CL_IN ClUint32T suboperation,
 *                               CL_IN ClHalAccessOrderT accessOrder,
 *                               CL_IN void  * const pUserData, 
 *                               CL_IN ClUint32T usrdataLen,
 *                               CL_IN ClUint32T flag,
 *                               CL_IN ClUint32T userflag);  
 *  \endcode
 *
 *  \param halObjHandle: Identifies the devices.
 *  \param operation: HAL OperationId. Received from the halConf.h file.
 *  \param suboperation: Sub-operation to qualify the operation. 
 *  \param accessOrder: Increasing or decreasing order of access priority.
 *  \param pUserData: Pointer to the user-data.
 *  \param usrdataLen: Length of the user-data. 
 *  \param flag: Flags, like \c CL_HAL_CONTINUE_ON_ERR.    
 *  \param userflag: User flag.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_ERR_INVLD_PARAM: On passing invalid parameters.
 */
ClRcT clHalObjectOperate(CL_IN ClHalObjectHandleT halObjHandle,
                       CL_IN ClUint32T operation,
                       CL_IN ClUint32T suboperation,
                       CL_IN ClHalAccessOrderT accessOrder,
                       CL_IN void  * const pUserData, 
                       CL_IN ClUint32T usrdataLen,
                       CL_IN ClUint32T flag,
                       CL_IN ClUint32T userflag);  

/**
 ************************************
 *  \page pageHAL305 clHalObjectMaxRespTimeGet
 *
 *  \par Synopsis:
 *  Retrieves the maximum device response time.
 *
 *  \par Description:
 *  A Hal Object could be assosciated with multiple device objects. Each device
 *  object will have maximum response time .This API is used to retrieve sum of 
 *  device response time for the assosciated device objects.
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectMaxRespTimeGet(
 * 			 	 CL_IN ClHalObjectHandleT halObjHandle,
 * 				 CL_OUT ClUint32T * const phalDevMaxRespTime);
 *  \endcode
 *
 *  \param halObjHandle: Identifies the HAL Object.
 *  \param phalDevMaxRespTime: Pointer to the maximum response time.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *
 */

ClRcT clHalObjectMaxRespTimeGet(CL_IN ClHalObjectHandleT halObjHandle,
                                CL_OUT ClUint32T * const phalDevMaxRespTime);    

/**
 ************************************
 *  \page pageHAL306 clHalObjectCapabilityLenGet
 *
 *  \par Synopsis:
 *  Retrieves the length of HAL Object Capability structure.
 *
 *  \par Description:
 *  A Hal Object could be assosciated with multiple device objects. Each device
 *  object can have certain capabilty expressed in char string .This API is used
 *  to retrieve sum of the size of buffer for the assosciated device objects.
 *
 *  This API is used to retrieve the length of HAL Object Capability structure.
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectCapabilityLenGet(
 * 			 	CL_IN ClHalObjectHandleT halObjHandle,
 *                              CL_OUT ClUint32T  * const pHalObjCapLen);
 *  \endcode
 *
 *  \param halObjHandle: Identifies the HAL Object.
 *  \param pHalObjCapLen: Pointer to the length of capability structure.
 * 
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *
 */

ClRcT clHalObjectCapabilityLenGet(CL_IN ClHalObjectHandleT halObjHandle,
                          CL_OUT ClUint32T  * const pHalObjCapLen);    


/**
 ************************************
 *  \page pageHAL307 clHalObjectCapabilityGet
 *
 *  \par Synopsis:
 *  Retrieves the device capability for the HAL Object.
 *
 *  \par Description:
 *  A Hal Object could be assosciated with multiple device objects. Each device
 *  object can have certain capabilty expressed in char string .This API is used
 *  to retrieve strcat of the buffer for the assosciated device objects.
 *
 *  This API is used to return the device capability for the HAL Object.
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectCapabilityGet(
 * 			 	 CL_IN ClHalObjectHandleT halObjHandle, 
 * 				 CL_IN ClUint32T halObjCapLen, 
 *                               CL_OUT void * pHalObjCap);
 *  \endcode
 *
 *  \param halObjHandle: Identifies the HAL Object.
 *  \param devCapLen: Length of buffer passed. 
 *  \param phalObjCap: Pointer to the Device Capability Structure.
 * 
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *
 */
ClRcT clHalObjectCapabilityGet( CL_IN ClHalObjectHandleT halObjHandle, 
                            CL_IN ClUint32T halObjCapLen, 
                            CL_OUT void * pHalObjCap);    

/**
 ************************************
 *  \page pageHAL308 clHalObjectCreate
 *
 *  \par Synopsis:
 *  Creates a HAL Objects. 
 *
 *  \par Description:
 *  This API is used to create HAL Objects from the configuration file. 
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectCreate (
 * 			 	CL_IN ClUint32T omHandle, 
 * 				CL_IN ClCorMOIdPtrT hMOId,
 *                              CL_IN ClHalObjectHandleT * phHalObj);
 *  \endcode
 *
 *  \param omHandle: Identifies the OM Object to which this HAL object will be attached. 
 *  Only the Class ID part of this parameter is used.              
 *  \param hMOId: Handle of the MO to which HAL Object is attached.
 *  \param phHalObj: Handle of the HAL object returned by the API .
 * 
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEM: On memory allocation failure.
 *
 */
ClRcT clHalObjectCreate (CL_IN ClUint32T omHandle, 
                                 CL_IN ClCorMOIdPtrT hMOId,
                                 CL_IN ClHalObjectHandleT * phHalObj);
/**
 ************************************
 *  \page pageHAL309 clHalObjectDelete
 *
 *  \par Synopsis:
 *  Deletes the HAL object.
 *
 *  \par Description:
 *  This API is used to delete the HAL object corresponding to the HAL Object handle.
 *  This handle is the one returned by the clHalObjectCreate API.   
 *
 *  \par Syntax:
 *  \code 	ClRcT clHalObjectDelete (
 * 			 	CL_IN ClHalObjectHandleT halObjHandle);
 *  \endcode
 *
 *  \param halObjHandle: Handle of HAL object to be deleted. 
 * 
 *  \retval CL_OK: The API executed successfully.
 *
 */
ClRcT clHalObjectDelete (CL_IN ClHalObjectHandleT halObjHandle);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_HAL_OBJECT_API_H_ */



