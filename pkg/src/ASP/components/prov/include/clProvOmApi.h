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
 * ModuleName  : prov
 * File        : clProvOmApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Provision OM class defination.
 *
 *
 *****************************************************************************/

#ifndef _CL_PROV_OM_API_H_
#define _CL_PROV_OM_API_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <clCommon.h>
#include <clOmObjectManage.h>
#include <clOmCommonClassTypes.h>
#include <clOmBaseClass.h>
#include <clProvApi.h>

/**
 * \file 
 * \brief Header file of Provision OM Class Definition and Callbacks
 * \ingroup prov_apis
 */

/**
 *  \addtogroup prov_apis
 *  \{
 */


/******************************** Provision OM Callbacks *************************/
/*********************************************************************************/
/* clProvValidate                                                                */
/* clProvUpdate                                                                  */
/* clProvRollback                                                                */
/* clProvRead                                                                    */
/*                                                                               */
/*********************************************************************************/
/**
 * Provision Library base class. 
 */
CL_OM_BEGIN_CLASS(CL_OM_BASE_CLASS,CL_OM_PROV_CLASS)
/**
 ************************************************
 *  \brief Validating provision attribute change request
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis (in) Ignore this argument, this is obsolete.
 *  \param txnHandle (in) Unique transaction identification.
 *  \param pProvTxn (in) Pointer to the structure contains necessary information to 
 *  \c identify which attribute in given Prov MSO is changed, its value and its property
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called for change in any attribute. In this function need to have
 *  necessary logic to verify attribute change can be accepected by given OI. If not it can return non CL_OK.
 *  If the return value is non CL_OK then clProvRollback function will be get called. If return value is 
 *  CL_OK then clProvUpdate function will be get called. This is the first function called in the two phase 
 *  transaction for given change in attribute.
 *  If more than one attribute is change for given Prov MSO, then this function will called for all those 
 *  attributes one by one. If validation for all the attribute is success then clProvUpdate function 
 *  will be called. For example given transation 5 attributes are changed. For first three attributes
 *  validation sucessed and 4th attribute is failed. Then clProvRollback function will be called only for
 *  first 3 three attributes.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  This function is being deprecated, if clProvObjectValidate() callback function has been defined 
 *  in the Object Implementer, then that will be called instead of this to group validate 
 *  all the requests in an object.
 *
 *  \sa clProvObjectValidate()
 *
 */
ClRcT(*clProvValidate)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxn);

/**
 ************************************************
 *  \brief Update provision attribute change request
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis (in) Ignore this argument, this is obsolete.
 *  \param txnHandle (in) Unique transaction identification.
 *  \param pProvTxn (in) Pointer to the structure contains necessary information to 
 *  \c identify which attribute in given Prov MSO is changed, its value and its property
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called for change in any attribute. In this function needs to 
 *  have a logic to take any action for attribute change. This function will called after clProvValidate 
 *  function is called for all the attributes which are changed in given transation. 
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  This function is being deprecated, if clProvObjectUpdate() callback function has been defined, 
 *  then that function will be called instead of this to group update all the requests in an object.
 *
 *  \sa clProvObjectUpdate()
 *
 */
ClRcT(*clProvUpdate)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxn);

/**
 ************************************************
 *  \brief Rollback provision attribute change request
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis (in) Ignore this argument, this is obsolete.
 *  \param txnHandle (in) Unique transaction identification.
 *  \param pProvTxn (in) Pointer to the structure contains necessary information to 
 *  \c identify which attribute in given Prov MSO is changed, its value and its property
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called when clProvValidate function returns non CL_OK.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  This function is being deprecated, if clProvObjectRollback() has been defined, 
 *  then that function will be called instead of this to group rollback all the changes in an object.
 *
 *  \sa clProvObjectRollback()
 */
ClRcT(*clProvRollback)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxn);

/**
 ************************************************
 *  \brief Get attribute value 
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis (in) Ignore this argument, this is obsolete.
 *  \param txnHandle (in) Unique transaction identification.
 *  \param pProvTxn (in) Pointer to the structure contains necessary information to 
 *  \c identify which attribute in given Prov MSO is changed, its value and its property
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called when get request for Run-Time attribute of Prov MSO.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  This function is being deprecated, if clProvObjectRead() callback function has been defined, 
 *  then that function will be called instead of this to read all the attributes values in an object.
 *
 *  \sa clProvObjectRead()
 *
 */
ClRcT(*clProvRead)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxn);

/**
 ************************************************
 *  \brief Object operations start callback function which will be
 *  provided by the user.
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pMoId (in) This contains the MoId of the object which is
 *  involved in the transaction.
 *  \param txnHandle (in) Unique transaction identifier.
 *
 *  \retval
 *  None
 *
 *  \par Description:
 *  This callback function is used to let the user know that 
 *  the transactional operations are getting started on the object. This
 *  function will be called before starting any of the transaction requests
 *  on the object. The 'txnHandle' contains the unique handle for the 
 *  transaction. If the user doesn't want this callback to be called, 
 *  then he can pass NULL as the value for this callback.
 *  
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
void (*clProvObjectStart) (ClCorMOIdPtrT pMoId, ClHandleT txnHandle);


/**
 ************************************************
 *  \brief Object operations end callback function which will be
 *  provided by the user.
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pMoId (in) This contains the MoId of the object which is
 *  involved in the transaction.
 *  \param txnHandle (in) Unique transaction identifier.
 *
 *  \retval
 *  None
 *
 *  \par Description:
 *  This callback function is used to let the user know that 
 *  the transactional operations are getting ended on the object. This
 *  function will be called after all the transaction requests are completed
 *  on the object. The 'txnHandle' contains the unique handle for 
 *  the transaction. If the user doesn't want this callback to be called, 
 *  then he can pass NULL as the value for this callback.
 *  
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 */
void (*clProvObjectEnd) (ClCorMOIdPtrT pMoId, ClHandleT txnHandle);

/**
 ************************************************
 *  \brief Group validate proivision attributes change requests.
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis            (in) : This is a pointer to the Prov class.
 *  \param txnHandle        (in) : Unique handle for the request.
 *  \param pProvTxnDataList (in) : This is pointer to the list of structures containing the
 *                                 information about the set/create/delete jobs.
 *                                 The different fields of the structure are explained
 *                                 as part of the definition of ClProvTxnDataT structure
 *                                 in the API reference guide.
 *  \param txnDataEntries   (in) : No. of jobs in pProvTxnDataList.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called to group validate all the attributes changes of an object. 
 *  In this function the user need to have necessary logic to verify the attributes changes and 
 *  can return non CL_OK if the validation fails.
 *  If the return value is non CL_OK then clProvObjectRollback() function will get called otherwise 
 *  clProvObjectUpdate() function will get called. 
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 *  \sa clProvObjectRollback(), clProvObjectUpdate()
 */
ClRcT(*clProvObjectValidate)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, 
                             ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries);

/**
 ************************************************
 *  \brief Group update proivision attributes changes.
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis            (in) : This is a pointer to the Prov class.
 *  \param txnHandle        (in) : Unique handle for the request.
 *  \param pProvTxnDataList (in) : This is pointer to the list of structures containing the
 *                                 information about the set/create/delete jobs.
 *                                 The different fields of the structure are explained
 *                                 as part of the definition of ClProvTxnDataT structure
 *                                 in the API reference guide.
 *  \param txnDataEntries   (in) : No. of jobs in pProvTxnDataList.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called only if the validation was successful so that the user 
 *  can group update all the attributes changes of an object. 
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 *  \sa clProvObjectValidate(), clProvObjectRollback()
 */
ClRcT(*clProvObjectUpdate)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, 
                           ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries);

/**
 ************************************************
 *  \brief Group rollback proivision attributes changes.
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis            (in) : This is a pointer to the Prov class.
 *  \param txnHandle        (in) : Unique handle for the request.
 *  \param pProvTxnDataList (in) : This is pointer to the list of structures containing the
 *                                 information about the set/create/delete jobs.
 *                                 The different fields of the structure are explained
 *                                 as part of the definition of ClProvTxnDataT structure
 *                                 in the API reference guide.
 *  \param txnDataEntries   (in) : No. of jobs in pProvTxnDataList.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called if the validation has failed, so that the user 
 *  can rollback all the changes done in the validation for all the requests.
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 *
 *  \sa clProvObjectValidate(), clProvObjectUpdate()
 */
ClRcT(*clProvObjectRollback)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, 
                             ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries);

/**
 ************************************************
 *  \brief Get all the attributes values. 
 *
 *  \par Header File:
 *  clProvOmApi.h
 *
 *  \param pThis            (in)     : This is a pointer to the Prov class.
 *  \param txnHandle        (in)     : Unique handle for the request.
 *  \param pProvTxnDataList (in/out) : This is pointer to the list of structures containing the
 *                                     information about the set/create/delete jobs.
 *                                     The different fields of the structure are explained
 *                                     as part of the definition of ClProvTxnDataT structure
 *                                     in the API reference guide.
 *  \param txnDataEntries   (in)     : No. of jobs in pProvTxnDataList.
 *
 *  \retval CL_OK The function is executed successfully.
 *
 *  \par Description:
 *  This callback function will be called to read all the transient attributes values 
 *  from the Primary Object Implementer (POI).
 *
 *  \par Library File:
 *  ClProv
 *
 *  \note
 *  None. 
 */
ClRcT(*clProvObjectRead)(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, 
                         ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries);

CL_OM_END


/** \} */


#ifdef __cplusplus
}
#endif

#endif /* _CL_PROV_OM_API_H_ */

