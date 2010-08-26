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
 * ModuleName  : PM 
 * File        : clPMApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains PM API prototypes and structures.
 *
 *
 *****************************************************************************/

#ifndef _CL_PM_API_H_
#define _CL_PM_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clOmObjectManage.h>
#include <clOmCommonClassTypes.h>
#include <clOmBaseClass.h>
#include <clCorApi.h>
#include <clCorNotifyApi.h>
#include <clAlarmDefinitions.h>
#include <clAlarmUtils.h>

/**
 * \file 
 * \brief Header file of PM API prototypes and structures. 
 * \ingroup pm_apis
 */

/**
 *  \addtogroup pm_apis
 *  \{
 */

typedef struct
{
    ClAlarmProbableCauseT probableCause;
    ClAlarmSpecificProblemT specificProblem;
    ClAlarmUtilPayLoadListPtrT pAlarmPayload;
}ClPMAlarmDataT;

typedef ClPMAlarmDataT* ClPMAlarmDataPtrT;

/**
 * This is used to pass PM and PM reset attributes 
 * information to the Primary OI.
 */
typedef struct
{
    /**
     * Attribute Id of PM or PM reset attribute.
     */
    ClCorAttrIdT attrId;

    /**
     * Attribute Path. This is always NULL and will 
     * be supported in the future release.
     */
    ClCorAttrPathPtrT pAttrPath;

    /**
     * Type of the attribtute. The value of this will be 
     * CL_COR_ARRAY_ATTR for array attribute and 
     * CL_COR_SIMPLE_ATTR for non-array attribute.
     */
    ClCorAttrTypeT attrType;

    /**
     * This contains the actual datatype of the attribute.
     */
    ClCorTypeT attrDataType;

    /**
     * This contains the PM reset attribute's value if it is 
     * reset from north-bound, or the Primary OI should copy the 
     * value of PM attribute from its local cache into this 
     * if it is read from north-bound.
     */
    void* pPMData;

    /**
     * Size of the attribute.
     */
    ClUint32T size;

    /**
     * Index of the attribute. This will be CL_COR_INVALID_ATTR_IDX
     * for non-array attribute.
     */
    ClInt32T index;

    ClUint32T alarmCount;

    ClPMAlarmDataPtrT pAlarmData;

} ClPMAttrDataT;


/**
 * Pointer type for ClPMAttrDataT
 */
typedef ClPMAttrDataT* ClPMAttrDataPtrT;

/**
 * This is used to pass the PM object's values to the 
 * Primary OI. This either contains the list of PM attributes info
 * whose values has to be read or the PM reset attribute's info and 
 * value.
 */
typedef struct 
{
    /**
     * The PM Object Identifier.
     */
    ClCorMOIdPtrT pMoId;

    /**
     * This contains the no. of attributes passed.
     */
    ClUint32T attrCount;

    /**
     * This either contains the PM attributes' info or PM reset 
     * attributes' info and value.
     */
    ClPMAttrDataPtrT pAttrData;

} ClPMObjectDataT;

/**
 * Pointer type for ClPMObjectDataPtrT.
 */
typedef ClPMObjectDataT* ClPMObjectDataPtrT;

/**
 *  \brief Callback used to read the value of PM attributes. 
 *
 *  \par Header File:
 *  clPMApi.h
 *
 *  \param txnHandle   : The unique Transaction Handle for the READ operation.
 *  \param pObjectData : This contains the PM attributes info and
 *                       user is supposed to fill in the values.
 *
 *  \retval CL_OK The callback function successfully executed.
 *
 *  \par Description:
 *  This callback function is used read the value of PM attributes 
 *  from the Primary OI when READ operation is performed on the 
 *  PM attributes from north-bound. The user is supposed to 
 *  walk the attributes in pObjectData and fill in the values.
 *
 *  \par Library File:
 *  ClPMClient
 *
 *  \sa ClPMObjectResetCallbackT 
 *
 */

typedef ClRcT (*ClPMObjectReadCallbackT)
                (CL_IN ClHandleT txnHandle, CL_INOUT ClPMObjectDataPtrT pObjectData);

/**
 *  \brief Callback used to reset the value of PM attributes. 
 *
 *  \par Header File:
 *  clPMApi.h
 *
 *  \param txnHandle   : The unique Transaction Handle for the RESET operation.
 *  \param pObjectData : This contains the PM reset attribute's info and the reset value
 *                       passed from north-bound.
 *
 *  \retval CL_OK The callback function successfully executed.
 *
 *  \par Description:
 *  This callback function is used to reset the value of PM attributes.
 *  This will be called when the PM reset attribute is SET from north-bound.
 *  The user is supposed to reset all its local cache PM values upon 
 *  receiving this callback.
 *
 *  \par Library File:
 *  ClPMClient
 *
 *  \sa ClPMObjectReadCallbackT 
 *
 */
typedef ClRcT (*ClPMObjectResetCallbackT)
                (CL_IN ClHandleT txnHandle, CL_INOUT ClPMObjectDataPtrT pObjectData);


/**
 * This stores address of the PM callback functions implemented by Primary OI.
 */
typedef struct 
{
    ClPMObjectReadCallbackT fpPMObjectRead;
    ClPMObjectResetCallbackT fpPMObjectReset;
} ClPMCallbacksT;


/**
 *  \brief Starts the PM operation on the list of MO Ids passed. 
 *
 *  \par Header File:
 *  clPMApi.h
 *
 *  \param pMoIdList : List of MoIds on which PM operation has to be started. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEM Memory allocation failed.
 *  \retval CL_ERR_INVALID_HANDLE PM library is not initialized successfully.
 *
 *  \par Description:
 *  This API is used start the PM operation on the list MOs specified in pMoIdList.
 *  The PM operation includes fetching the PM attributes values from the Primary OI 
 *  in the periodic interval configured, update the value of cached PM attributes into 
 *  COR, check the value of the attribute for threshold crossing and raise/clear 
 *  the alarm configured for that threshold limit.
 *
 *  \par Library File:
 *  ClPMClient
 *
 *  \sa clPMStop 
 *
 */
ClRcT clPMStart(ClCorMOIdListPtrT pMoIdList);


/**
 *  \brief Stops the PM operation on the list of MO Ids passed. 
 *
 *  \par Header File:
 *  clPMApi.h
 *
 *  \param pMoIdList : List of MoIds on which PM operation has to be stopped. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEM Memory allocation failed.
 *  \retval CL_ERR_INVALID_HANDLE PM library is not initialized successfully.
 *  \retval CL_ERR_NOT_EXIST PM library is not initialized successfully.
 *  \retval CL_ERR_NULL_POINTER PM MoId list is empty, the library is not initialized successfully or
 *                              the specified moId(s) are not in the PM operation list.
 *
 *  \par Description:
 *  This API is used to stop the PM operation on the list of MO Ids passed.
 *
 *  \par Library File:
 *  ClPMClient
 *
 *  \sa clPMStart
 *
 */
ClRcT clPMStop(ClCorMOIdListPtrT pMoIdList);

ClRcT clPMResourcesGet(ClCorMOIdListT** ppMoIdList);

ClRcT clPMClientDebugRegister(ClHandleT* pDebugHandle);
ClRcT clPMClientDebugUnregister(ClHandleT debugHandle);

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* _CL_PM_API_H_ */
