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
 * ModuleName  : prov
 * File        : clProvApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Provision Library related API defination.
 *
 *
 *****************************************************************************/

#ifndef _CL_PROV_API_H_
#define _CL_PROV_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorNotifyApi.h>

/**
 *  \file 
 *  \brief Header file of Provision Library related APIs
 *  \ingroup prov_apis
 */

/**
 *  \addtogroup prov_apis
 *  \{
 */

/******************************** Provision Library APIs *************************/
/*********************************************************************************/
/* clProvInitialize                                                              */
/* clProvFinalize                                                                */
/* clProvVersionCheck                                                            */
/* clProvObjectCreate                                                            */
/* clProvObjectDelete                                                            */
/*                                                                               */
/*********************************************************************************/

/**
 * The structure is used to pass the job information while calling the
 * object implementer's callback functions. These functions include the 
 * validate, update and rollback callbacks used for processing the object 
 * modification request (create/set/delete) and the read callback for 
 * processing the get request on a transient attribute of a managed 
 * resource. 
 * The object implementer registers the callbacks which will be called 
 * in its context if any operation is done on its managed resources.
 */
    typedef struct ClProvTxnData
    {

/**
 * The argument provides the operation type of the job.
 * It can be one of the CL_COR_OP_SET, CL_COR_OP_CREATE,
 * CL_COR_OP_CREATE_AND_SET or CL_COR_OP_DELETE or 
 * CL_COR_OP_GET.
 */
           ClCorOpsT provCmd;

/**
 * It is pointer to the MOID of the managed resource on
 * which the operation is done. It is a void pointer, so 
 * a user need to cast it to ClCorMOIdPtrT pointer type 
 * before start using it. This is useful in the case of all
 * the request types that is CREATE, DELETE, GET and SET 
 * operations.
 */
        void* pMoId;
/**
 * This is pointer to the attribute path structure. This is
 * useful in the case of a SET and GET request. For the 
 * provisioning callback this paramter is always NULL. 
 */
        ClCorAttrPathPtrT attrPath;
/**
 * It is the type of the attribute that is for an array
 * attribute it is CL_COR_ARRAY_ATTR otherwise it is
 * CL_COR_SIMPLE_ATTR. This is useful in the case of a 
 * SET and GET operation. 
 */
        ClCorAttrTypeT    attrType;
/**
 * This is the actual data type of the attribute. It is
 * the data type modeled for the attribute. It can take
 * any value from the enum ClCorTypeT.  This is useful
 * in the case of a SET and GET request.
 */
        ClCorTypeT attrDataType;
/**
 * The identifier for the attribute. It is decimal value which
 * identifies an attribute among the list of attributes of the
 * managed resource class. This is useful in the case of a SET
 * and GET operation.
 */
        ClCorAttrIdT      attrId;
/**
 * It is a pointer to the address containing value of the attribute. 
 * For the SET the value of the attribute is present in pProvData, 
 * so the user need to dereference it for getting the value. For GET
 * operation on a transient attribute, the latest value should be 
 * copied to pProvData.
 * The size of the data to be copied to or from is given by the "size" 
 * field of this structure.
 */
        void             *pProvData;
/**
 * Size of the data. This is useful only in case of SET and GET job.
 * In case of SET operation, for getting the value of the attribute
 * only this element should be used. For a get operation, the value
 * copied to the pProvData should be equal to "size". 
 */
        ClUint32T         size;
/**
 * This is a non-zero value for a array attribute if an index is 
 * supplied while submitting the job. Otherwise it is always zero.
 * For a simple attribute it is always CL_COR_INVALID_ATTR_IDX.
 * This is useful in the case of a SET and GET operation.
 */
        ClUint32T         index;
     
        ClCorTxnJobIdT    jobId;

    } ClProvTxnDataT;

/**
 ************************************************
 *  \brief Transaction start callback function which will be 
 *  provided by the user.
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param txnHandle (in) Unique handle for the transaction. 
 *
 *  \retval
 *  none
 *
 *  \par Description:
 *  This callback function is used to let the user know that a transaction
 *  is getting started. This will be called before starting any of the 
 *  transaction requests. The 'txnHandle' contains the unique handle for the
 *  transaction. If the user doesn't want this callback to be called, then he
 *  can pass NULL as the value for this callback.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None.
 *  
 */
typedef void (*ClProvTxnStartCallbackT) (
                CL_IN ClHandleT txnHandle);

/**
 ************************************************
 *  \brief Transaction end callback function which will be 
 *  provided by the user.
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param txnHandle (in) Unique handle for the transaction. 
 *
 *  \retval
 *  none
 *
 *  \par Description:
 *  This callback function is used to let the user know that a transaction
 *  is getting ended. This will be called after all the transaction requests
 *  are completed. The 'txnHandle' contains the unique handle of the
 *  transaction. If the user doesn't want this callback to be called, then he
 *  can pass NULL as the value for this callback.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None.
 *  
 */
typedef void (*ClProvTxnEndCallbackT) (
                CL_IN ClHandleT txnHandle);

/**
 * This structure is used to get the start and end callbacks for a transaction
 * from the user. The start callback will be called before beginning a transaction
 * and the stop callback will be called after the transaction is completed. Both of
 * these are optional, if the user doesn't want these callbacks to be called, then 
 * he can pass NULL as the value.
 */
typedef struct
{
    /*
     * This is used to get the start callback from the user which will be called
     * before beginning a transaction.
     */
    ClProvTxnStartCallbackT fpProvTxnStart;

    /*
     * This is used to get the end callback from the user which will be called
     * after completing the transaction.
     */
    ClProvTxnEndCallbackT   fpProvTxnEnd; 
} ClProvTxnCallbacksT;

/**
 ************************************************
 *  \brief Initializes the Provision Manager library.
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param 
 *   None.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This API is used to initialize the Provision library for the component.
 *  It takes care of pre-provisioned information for a given resource(s),
 *  which are controlled by the invoking component.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
ClRcT   clProvInitialize();

/**
 ************************************************
 *  \brief Cleans up the Provision Manager library.
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param 
 *   None.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This API is used to clean up the Provision Manager library from the invoking component.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
ClRcT   clProvFinalize();

/**
 ************************************************
 *  \brief Verify the provision library version.
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param pVersion (in/out) This can be either an input or an output parameter.
 *  \c As an input parameter, version is a pointer to the required Prov version.
 *  \c Here, \e minorVersion is ignored and should be set to 0x00.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This API is used to verify the version which are supported by provision library.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
ClRcT   clProvVersionCheck(CL_INOUT ClVersionT* pVersion);

/**
 ************************************************
 *  \brief Create Prov MSO object
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param pMoId (in) MOID of given resource for which creating Prov MSO
 *  \param attrList (in) List of attributes to be initialized. This should contain
 *  \c the value for all the 'WRITE-INITIALIZED' attributes in that MO. 
 *  \c In case if there is no 'WRITE-INITIALIZED' attributes in given MO, we can pass NULL
 *  \c for this argument.
 *  \param pHandle (out) Pointer to the object handle. This will be updated after
 *  \c creation of the object. pHandle is used to delete Prov MSO
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This API is used to create Prov MSO from caller context. This function has to called from
 *  corresponding OI process context. This function takes attribute list, which needs to be filled
 *  if given Prov MSO has any WRITE-INITIALIZED attribute. If given Prov MSO is already there 
 *  it will return CL_OK. If given MO is not there then it creates the same along with Prov MSO.
 *  It also adds given Prov MSO in COR route list using clCorOIRegister function.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
ClRcT clProvObjectCreate(CL_IN ClCorMOIdPtrT pMoId, CL_IN ClCorAttributeValueListPtrT attrList, CL_OUT ClCorObjectHandleT* pHandle);

/**
 ************************************************
 *  \brief Delete Prov MSO object
 *
 *  \par Header File:
 *  clProvApi.h
 *
 *  \param handle (in) The object handle, which is return by clProvObjectCreate API.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This API is used to delete Prov MSO from caller context. If there are no MSOs other than Prov MSO
 *  for given MO, then this function deletes given MO also. If there are other MSOs other than prov MSO
 *  in given MO, then given MO will be deleted. 
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
ClRcT clProvObjectDelete(CL_IN ClCorObjectHandleT handle);

ClRcT clProvResourcesGet(ClCorMOIdListT** ppMoIdList);

/** \} */


#ifdef __cplusplus
}
#endif

#endif /* _CL_PROV_API_H_ */
