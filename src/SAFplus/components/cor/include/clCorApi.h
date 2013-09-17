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
 * File        : clCorApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  This module contains Clovis Object Registry (COR) related definitions.
 *
 *
 *****************************************************************************/


/*********************************************************************************/
/******************************** COR IPIs ***************************************/
/*********************************************************************************/
/*                                                                               */
/* clCorClientInitialize                                                         */
/* clCorClientFinalize                                                           */
/* clCorClassCreate                                                              */
/* clCorClassDelete                                                              */
/* clCorClassAttributeCreate                                                     */
/* clCorClassAttributeArrayCreate                                                */
/* clCorClassAttributeValueSet                                                   */
/* clCorClassAttributeUserFlagsSet                                               */
/* clCorClassAttributeUserFlagsGet                                               */
/* clCorClassAttributeTypeGet                                                    */
/* clCorClassAssociationCreate                                                   */
/* clCorClassContainmentAttributeCreate                                          */
/* clCorClassAttributeDelete                                                     */
/* clCorClassNameSet                                                             */
/* clCorClassNameGet                                                             */
/* clCorClassTypeFromNameGet                                                     */
/* clCorClassAttributeNameGet                                                    */
/* clCorClassAttributeNameSet                                                    */
/* clCorMOClassCreate                                                            */
/* clCorMOClassDelete                                                            */
/* clCorMSOClassCreate                                                           */
/* clCorMSOClassExist                                                            */
/* clCorMSOClassDelete                                                           */
/* clCorMOClassExist                                                             */
/* clCorMOPathToClassIdGet                                                       */
/* clCorServiceRuleDisable                                                       */
/* clCorServiceRuleDeleteAll                                                     */
/* clCorServiceRuleStatusGet                                                     */
/* clCorMoIdToComponentAddressGet                                                */
/* clCorObjectFlagsSet   <-- !!! Not Supported !!!!--- >                         */
/* clCorObjectFlagsGet   <-- !!! Not Supported !!!!--- >                         */
/* clCorSubTreeDelete    <--- !!! Not Supported !!!!--- >                        */
/*********************************************************************************/
/******************************** COR APIs ***************************************/
/*********************************************************************************/
/* clCorObjectAttributeSet                                                       */
/* clCorObjectAttributeGet                                                       */
/* clCorObjectHandleGet                                                          */
/* clCorObjectCreate                                                             */
/* clCorObjectDelete                                                             */
/* clCorObjectWalk                                                               */
/* clCorObjectHandleToTypeGet                                                    */
/* clCorObjectHandleToMoIdGet                                                    */
/* clCorServiceAdd           <--- !!! Not Supported !!!!--- >                    */
/* clCorServiceRuleAdd                                                           */
/* clCorServiceRuleDelete                                                        */
/* clCorMoIdToLogicalSlotGet <--- !!! Not Supported !!!!--- >                    */
/* clCorLogicalSlotToMoIdGet <--- !!! Not Supported !!!!--- >                    */
/* clCorObjectAttributeWalk                                                      */
/* clCorVersionCheck                                                             */
/* clCorMoIdToNodeNameGet                                                        */
/* clCorNodeNameToMoIdGet                                                        */
/*                                                                               */
/*********************************************************************************/

/**
 *  \file
 *  \brief Header file of Clovis Object Registry (COR) related APIs
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */


#ifndef _CL_COR_API_H_
#define _CL_COR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clCorMetaData.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/* For constants and macros, refer to clCorMetaData.h header file */

/******************************************************************************
 *  Data Types
 *****************************************************************************/
/* For datatypes, refer to clCorMetaData.h header file */

/*****************************************************************************
 *  Internal IPI Functions
 *****************************************************************************/

ClRcT clCorClientInitialize(void);

ClRcT clCorClientFinalize(void);


/*****************************************************************************
 * -- IPI's for defining classes and relationships among them --- *
 *****************************************************************************/

extern ClRcT clCorClassCreate(ClCorClassTypeT classId, ClCorClassTypeT superClassId);
extern ClRcT clCorClassDelete(ClCorClassTypeT classId);
extern ClRcT clCorClassAttributeCreate(ClCorClassTypeT  classId, ClCorAttrIdT attrId, ClCorTypeT attrType);
extern ClRcT clCorClassAttributeArrayCreate(ClCorClassTypeT  classId, ClCorAttrIdT attrId, ClCorTypeT attrType, ClInt32T arraySize);
extern ClRcT clCorClassAttributeValueSet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClInt64T init, ClInt64T min, ClInt64T max);
extern ClRcT clCorClassAttributeUserFlagsSet(ClCorClassTypeT  classId, ClCorAttrIdT attrId, ClUint32T flags);
extern ClRcT clCorClassAttributeUserFlagsGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClUint32T* flags);
extern ClRcT clCorClassAttributeTypeGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClCorAttrTypeT  *pAttrType);
extern ClRcT clCorClassAssociationCreate(ClCorClassTypeT  classId, ClCorAttrIdT attrId, ClCorClassTypeT associatedClass, ClInt32T max);
extern ClRcT clCorClassContainmentAttributeCreate(ClCorClassTypeT  classId, ClCorAttrIdT attrId, ClCorClassTypeT containedClass, ClInt32T min, ClInt32T max);
extern ClRcT clCorClassAttributeDelete(ClCorClassTypeT classId, ClCorAttrIdT attrId);

/* -- Class related IPIs for debugging purposes only. --- */

extern ClRcT clCorClassNameSet(ClCorClassTypeT classId, char* name);
extern ClRcT clCorClassNameGet(ClCorClassTypeT classId, char *name, ClUint32T* size);
extern ClRcT clCorClassTypeFromNameGet(char *name, ClCorClassTypeT *classId);
extern ClRcT clCorClassAttributeNameGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, char *name, ClUint32T* size);
extern ClRcT clCorClassAttributeNameSet(ClCorClassTypeT classId, ClCorAttrIdT attrId, char *name);

/* -- IPI's for MOClass Tree manuplation (create,delete MOClasses) --- */

extern ClRcT clCorMOClassCreate(ClCorMOClassPathPtrT moPath, ClInt32T maxInstances);
extern ClRcT clCorMOClassDelete(ClCorMOClassPathPtrT moPath);
extern ClRcT clCorMSOClassCreate(ClCorMOClassPathPtrT moPath, ClCorServiceIdT svcId, ClCorClassTypeT classId);
extern ClRcT clCorMSOClassExist(ClCorMOClassPathPtrT moPath, ClCorServiceIdT svcId);
extern ClRcT clCorMSOClassDelete(ClCorMOClassPathPtrT moPath, ClCorServiceIdT svcId);
extern ClRcT clCorMOClassExist(ClCorMOClassPathPtrT moPath);
extern ClRcT clCorMOPathToClassIdGet(ClCorMOClassPathPtrT pPath, ClCorServiceIdT svcId, ClCorClassTypeT* pClassId);

/* This Object Tree Related Apis'--  not Supported  */
extern ClRcT clCorSubTreeDelete(ClCorMOIdT moId);
extern ClRcT clCorObjectFlagsSet(CL_IN ClCorMOIdPtrT moh, CL_IN ClCorObjFlagsT flags);
extern ClRcT   clCorObjectFlagsGet(CL_IN ClCorMOIdPtrT moh, CL_OUT ClCorObjFlagsT* pFlags);
/* MSP list related api. This is depricated now. */
extern ClRcT   clCorServiceAdd(ClCorServiceIdT id, char *mspName, ClCorCommInfoPtrT comm);

/** -- IPI's for Route List manipulation -- */
extern ClRcT clCorMoIdToComponentAddressGet(CL_IN ClCorMOIdPtrT moh, CL_OUT ClCorAddrT* addr);
extern ClRcT clCorServiceRuleDisable(ClCorMOIdPtrT moId, ClCorAddrT addr);
extern ClRcT clCorServiceRuleStatusGet(ClCorMOIdPtrT moId, ClCorAddrT addr, ClInt8T *status);
extern ClRcT clCorServiceRuleDeleteAll(ClCorServiceIdT srvcId, ClCorAddrT addr);
extern ClRcT clCorOIRegisterAndDisable(ClCorMOIdPtrT pMoId, ClCorAddrT addr);


/* -- IPI for getting the object Attribute information */
extern ClRcT clCorObjAttrInfoGet(CL_IN  ClCorObjectHandleT objH, CL_IN  ClCorAttrPathPtrT pAttrPath, CL_IN  ClCorAttrIdT attrId,
                    CL_OUT ClCorTypeT *attrType, CL_OUT ClCorAttrTypeT *arrDataType, CL_OUT ClUint32T *attrSize, 
                    CL_OUT ClUint32T *userFlags );
/* --- object manipulation APIs --- */

/**
 *  \brief Creates a COR object.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param txnSessionID (in/out) For a SIMPLE transaction containing one job, the value of the
 *                parameter must be CL_COR_SIMPLE_TXN. For a COMPLEX transaction containing
 *                multiple jobs, it must be initialized to zero and passed to the API for adding the first job.
 *                To add the subsequent jobs, the returned value from the previous API should be
 *                passed as the parameter.
 *  \param MoId (in) ID of the object to be created.
 *  \param handle (out) Pointer to the object handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR txnSessionId or moId is a NULL pointer.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *  \retval CL_COR_INST_ERR_INVALID_MOID MoId is invalid.
 *  \retval CL_COR_ERR_CLASS_NOT_PRESENT The specified Class is not present.
 *  \retval CL_COR_INST_ERR_NODE_NOT_FOUND Parent class is not present in the instance
 *   tree.
 *  \retval CL_COR_MO_TREE_ERR_NODE_NOT_FOUND moTree node is not present for the
 *  class.
 *  \retval CL_COR_INST_ERR_MAX_INSTANCE Maximum instance count for this class is
 *  reached.
 *  \retval CL_COR_INST_ERR_MO_ALREADY_PRESENT MO is present in the instance tree.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *  \retval CL_COR_INST_ERR_MSO_ALREADY_PRESENT MSO exists in the Object Tree.
 *  \retval CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT MSO Class is not present in the MO
 *  Tree.
 *
 *  \par Description:
 *  This API is used to create MO and MSO COR objects. The MoId, passed to this API must
 *  be valid (the MO class tree must be present for that MoId)
 *  \par
 *  To create an MO object, the service ID should be specified as CL_COR_INVALID_SVC_ID
 *  in the MoId. To create an MSO object, the service ID corresponding to the MSO should be
 *  specified in the MOID.
 *  \par
 *  If the creation of the object is part of a COMPLEX transaction, the object handle returned is
 *  valid only after the transaction is committed using the clCorTxnSessionCommit() API.
 *  For a COMPLEX transaction involving multiple jobs, the txnSessionId must be initialized
 *  to 0 and sent to the API to add the first job. This API returns a transaction session ID as the
 *  OUT parameter. This can be used for adding the subsequent jobs using
 *  clCorObjectAttributeSet()/clCorObjectDelete()/clCorObjectCreate() APIs. A COMPLEX transaction 
 *  can be committed by using the clCorTxnSessionCommit() API.
 *  \par
 *  Whenever the session-commit or the job addition to a complex transaction fails, the user is 
 *  supposed to call clCorTxnSessionFinalize API.
 *  \par
 *  If the creation of the object is part of a SIMPLE transaction, txnSessionId must be set to
 *  the value CL_COR_SIMPLE_TXN. A SIMPLE transaction can take only one job.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectAttributeGet, clCorObjectAttributeSet, clCorObjectDelete.
 *
 */

extern ClRcT clCorObjectCreate(CL_INOUT ClCorTxnSessionIdT *txnSessionId, CL_IN ClCorMOIdPtrT moId, CL_OUT ClCorObjectHandleT *handle);

/* New function to do get/set operation on all types of attributes.*/

/**
 *  \brief Sets the attribute of a COR object
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param txnSessionID (in/out) Transaction Session ID.
 *  \param phandle (in) Handle of the object whose attribute is being set.
 *  \param contAttrPath (in) Containment hierarchy path.
 *  \param attrID (in) ID of the attribute.
 *  \param index (in) Index of the attribute. It must be set to CL_COR_INVALID_ATTR_IDX for a
 *  SIMPLE attribute.
 *  \param value (in) Pointer to the value that is required to be set.
 *  \param size (in) Size of the value
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR txnSessionId or value is a NULL pointer.
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID The job ID is invalid .
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_INDEX Index is used for the SIMPLE attribute.
 *  \retval CL_COR_ERR_CLASS_ATTR_NOT_PRESENT The attribute ID is not present.
 *  \retval CL_COR_ERR_INVALID_SIZE For SIMPLE attributes, the parameter size is less than
 *  the size associated with the attribute attrId. For array attributes, this error is
 *  returned in one of the following cases:
 *  (1) the size is greater than the size of the associated array elements that are required to
 *  be updated;
 *  (2) the parameter size is less than the size of an individual array element;
 *  (3) the parameter size is not an integer. This integer value must be the multiple of
 *  the size of the individual array element.
 *  \retval CL_COR_ERR_OBJ_ATTR_INVALID_SET This error code is applicable to SIMPLE
 *  attributes, when the value is not within the range specified by min and max values
 *  associated with the attribute.
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_INDEX Invalid index for the attribute.
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_RELATION Size of the attribute is invalid.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *
 *  \par Description:
 *  This API is used to set the value of the attributes of an object . contAttrPath can contain
 *  a valid attribute path or NULL. The current implementation supports only a NULL value for
 *  this parameter.
 *  txnSessionId contains the ID for the transaction. For a SIMPLE transaction involving
 *  only one attribute set, this must be set to CL_COR_SIMPLE_TXN. For a COMPLEX
 *  transaction involving multiple attribute sets, this should be initialized to zero and sent to the
 *  API for adding the first job. The API updates txnSessionId and this updated value can be
 *  passed to add subsequent jobs to the transaction using clCorObjectAttributeSet()/
 *  clCorObjectCreate()/clCorObjectDelete() APIs. The transaction must be committed using 
 *  clCorTxnSessionCommit() API. If the session-commit or the job addition to the 
 *  complex transaction fails, then the user is supposed to call clCorTxnSessionFinalize() API.
 *  \par
 *  For SIMPLE attributes, the index must be set to -1 or CL_COR_INVALID_ATTR_IDX. 
 *  The size of the value must be equal to its actual size.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectCreate, clCorObjectAttributeGet, clCorObjectDelete.
 *
 */
extern ClRcT clCorObjectAttributeSet(CL_INOUT ClCorTxnSessionIdT *txnSessionId, CL_IN ClCorObjectHandleT pHandle, CL_IN ClCorAttrPathPtrT contAttrPath, CL_IN ClCorAttrIdT attrId, CL_IN ClUint32T index, CL_IN void *value, CL_IN ClUint32T size);

/**
 ***********************************************
 *  \brief Deletes a COR object.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param txnSessionID (in/out) Transaction Session ID.
 *  \param handle (in) Handle of the object to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR txnSessionId is a NULL pointer.
 *  \retval CL_COR_INVALID_SRVC_ID Service ID is incorrect.
 *  \retval CL_COR_INST_ERR_INVALID_MOID MoId is invalid.
 *  \retval CL_COR_INST_ERR_CHILD_MO_EXIST Child MO exists for the MO object node.
 *  \retval CL_COR_INST_ERR_MSO_EXIST MSO exists for the MO object node.
 *  \retval CL_COR_INST_ERR_NODE_NOT_FOUND Node not found in the object tree.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *  \retval CL_COR_MO_TREE_ERR_NODE_NOT_FOUND MO tree node not found.
 *
 *  \par Description:
 *  This API is used to delete a COR object. The parameter, handle, contains the handle of
 *  the object (obtained through clCorObjectHandleGet()) to be deleted.
 *  To delete an MSO object, the MoId containing the corresponding service ID must be sent to
 *  the clCorObjectHandleGet() API. This API returns a handle to the object that can be
 *  used to delete the MSO object.
 *  \par
 *  To delete the MO object, the service ID of the MoId must be set to
 *  CL_COR_INVALID_SVC_ID. The MSOs and child MOs must be deleted before deleting the
 *  parent MO. The MOs and MSOs can be deleted in a single transaction.
 *  txnSessionId contains the ID for the transaction. For a SIMPLE transaction involving
 *  only one object delete, this must be set to CL_COR_SIMPLE_TXN. For a COMPLEX
 *  transaction involving multiple CREATE, SET, and DELETE, this should be initialized to zero
 *  and sent to the API to add the first job. The API updates this transaction ID. The updated
 *  transaction ID must be sent to add subsequent jobs to the transaction using
 *  clCorObjectCreate()/clCorObjectAttributeSet()/clCorObjectDelete()
 *  APIs. The transaction should be committed by using the clCorTxnSessionCommit()
 *  API.
 *  \par
 *  If the session-commit or the job addition to the complex transaction fails, then the user is 
 *  supposed to call clCorTxnSessionFinalize.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa
 *  clCorObjectCreate.
 *
 */
extern ClRcT clCorObjectDelete(CL_INOUT ClCorTxnSessionIdT *txnSessionId, CL_IN ClCorObjectHandleT handle);

/**
 ***********************************************
 *  \brief Retrieves the value of an attribute belonging to an object.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pHandle (in) handle of the object whose attribute is being read.
 *  \param contAttrPath (in) Path of the containment hierarchy.
 *  \param attrID (in) ID of the attribute.
 *  \param index (in) Attribute index. It is set to CL_COR_INVALID_ATTR_IDX for a SIMPLE
 *  \param attribute.
 *  \param value (out) Pointer to the value. The attribute value is copied into this parameter.
 *  \param size (in/out) Size of the value.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR value or size is a NULL pointer.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *  \retval CL_COR_ERR_CLASS_ATTR_NOT_PRESENT Attribute ID passed is not present.
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_RELATION Size of the attribute is invalid.
 *  \retval CL_COR_ERR_INVALID_SIZE For SIMPLE attributes, the parameter size, must be equal
 *  to the size if the attribute. For array attributes, this error is returned in one of the
 *  following cases:
 *  (1) the  size is greater than the size of the associated array elements that must be
 *  updated;
 *  (2) the parameter size is less than size of a single array element;
 *  (3) the parameter size is not an integer. This integer must be a multiple of the size
 *  of the single array element.
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_INDEX index is used for SIMPLE attributes.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *
 *  \par Description:
 *  This API is used to GET (retrieve) the value of an attribute. pHandle is the handle to the
 *  object from which the attribute value is to be read. The object handle is returned when the
 *  object is created and can also be obtained by passing MOID to the API
 *  clCorObjectHandleGet().
 *  \par
 *  Parameter pAttrPath contains the containment path hierarchy where the attribute resides.
 *  This is currently not supported. So, it must be set to NULL.
 *  Parameter index contains the index of the attribute. For SIMPLE attributes, this can be set
 *  to -1 or CL_COR_INVALID_ATTR_IDX.
 *  The parameter value contains the address where the retrieved value should be copied.
 *  The parameter size must contain the size of the value to be retrieved from an attribute.
 *  
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectCreate, clCorObjectAttributeSet.
 *
 */
extern ClRcT clCorObjectAttributeGet(CL_IN ClCorObjectHandleT pHandle, CL_IN ClCorAttrPathPtrT contAttrPath, CL_IN ClCorAttrIdT attrId, CL_IN ClInt32T index, CL_OUT void *value, CL_INOUT ClUint32T *size);

/**
 *****************************************************************************
 *  \brief Creates and sets a COR object.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param txnSessionId (in/out) Transaction Session ID. For a simple transaction 
 *  \c which contains only one job, the value of the parameter will be CL_COR_SIMPLE_TXN. 
 *  \c For a complex transaction which contains more than one job, it is initialized to 
 *  \c zero and passed to the API for adding the first job. To add the subsequent jobs 
 *  \c the returned value from the previous function should be passed as the parameter. 
 *
 *  \param moId (in) MoId of the object to be created.
 *
 *  \param attrList (in) List of attributes to be initialized. This should contain 
 *  \c the value for all the 'Initialized' attributes in that MO.
 *
 *  \param pHandle (out) Pointer to the object handle. This will be updated after 
 *  \c creation of the object and its attributes set is done.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_COR_INST_ERR_INVALID_MOID  MoId passed is invalid.
 *  \retval CL_COR_ERR_CLASS_NOT_PRESENT Class is not present.
 *  \retval CL_COR_INST_ERR_PARENT_MO_NOT_EXIST Parent MO does not exist.
 *  \retval CL_COR_INST_ERR_MAX_INSTANCE Maximum Instance count for this object is reached.
 *  \retval CL_COR_INST_ERR_MO_ALREADY_PRESENT MO is already present in the instance tree.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *  \retval CL_COR_INST_ERR_MSO_ALREADY_PRESENT MSO is already present in the Object Tree.
 *  \retval CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT Class is not present in the MO Tree.
 *  \retval CL_COR_ERR_CLASS_ATTR_INVALID_INDEX If index is used in case of simple attribute 
 *  \c or index passed is invalid in case of array attribute.
 *  \retval CL_COR_ERR_CLASS_ATTR_NOT_PRESENT The attribute is not present.
 *  \retval CL_COR_ERR_INVALID_SIZE The size of the attribute specified is invalid.
 *  \retval CL_COR_ERR_OBJ_ATTR_INVALID_SET If the initial value specified is not within 
 *  \c the range specified by min and max values associated with the attribute.
 *  \retval CL_COR_ERR_ATTR_NOT_INITIALIZED If value is not provided for a 'Initialized' attribute.
 *
 *  \par Description:
 *  This function is used to create a COR object as well as set the value of the attributes. 
 *  The MoId passed to this function must be valid (the MO class tree must be present for that MoId). 
 *  For a MO object creation, the service ID should be specified as CL_COR_INVALID_SVC_ID in the MoId. 
 *  For a MSO object creation, the service ID corresponding to the MSO should be specified in the MOID. 
 *  The list of attributes information and the initial values are passed in the parameter attrList. 
 *  The specified parameter attrList should contain the values for all the 'Initialized' attributes 
 *  in that MO/MSo. 
 *  <br>
 *  If this API is used as part of a COMPLEX transaction, the object handle returned is valid only 
 *  after the transaction is committed by using the API clCorTxnSessionCommit. 
 *  <br>                                     
 *  For a SIMPLE transaction, the value CL_COR_SIMPLE_TXN should be passed to the parameter 'txnSessionId'. 
 *  The SIMPLE transaction will take only one job. For a COMPLEX transaction involving more than one job, 
 *  the txnSessionId should be initialized to 0 and send to the API to add the first job.  This API in turn, 
 *  will return a transaction session ID as the OUT parameter.  This can be used for adding the subsequent 
 *  jobs using the APIs  clCorObjectAttributeSet() / clCorObjectDelete() / clCorObjectCreate() /
 *  clCorObjectCreateAndSet().  This complex transaction can be committed by using the API clCorTxnSessionCommit().
 *  <br>
 *  If the session-commit or the job addition to the complex transaction fails, then the user is 
 *  supposed to call clCorTxnSessionFinalize.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectCreate, clCorObjectAttributeSet, clCorObjectDelete.
 *
 */
extern ClRcT clCorObjectCreateAndSet(CL_INOUT ClCorTxnSessionIdT* tid, CL_IN ClCorMOIdPtrT pMoId, CL_IN ClCorAttributeValueListPtrT attrList, CL_OUT ClCorObjectHandleT* pHandle);

/**
 ***********************************************
 *  \brief Retrieves the compressed MO handle corresponding to MOID.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoid (in) Pointer to the MOID.
 *  \param *objHandle (out) Pointer to object handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *  \retval CL_COR_INST_ERR_INVALID_MOID MoId is invalid.
 *  \retval CL_COR_ERR_INVALID_DEPTH The depth of the MoId is greater than the maximum
 *  depth that is configured.
 *  \retval CL_COR_UTILS_ERR_INVALID_KEY Failure to locate the node in the MO tree.
 *  \retval CL_COR_ERR_INVALID_PARAM An invalid parameter has been passed to the function.
 *  A parameter is not set correctly.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *
 *  \par Description:
 *  This API is used to get the handle of an object for a given MOID. The object handle returned
 *  is the compressed value of the MOID. This object handle can be used for performing SET,
 *  GET, and DELETE operations on the MO.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectHandleToMoIdGet, clCorObjectHandleToTypeGet.
 *
 */
extern ClRcT clCorObjectHandleGet(ClCorMOIdPtrT pMoId, ClCorObjectHandleT *objHandle);

extern void clCorObjectHandleFree(ClCorObjectHandleT* pObjH);

/* Function to get the size of the object handle. */
extern ClRcT clCorObjectHandleSizeGet(ClCorObjectHandleT objH, ClUint16T* pSize);

/**
 ************************************************
 *  \brief Walks through the object tree.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param moIdRoot (in) MOID from where the walk operation begins.
 *  \param moIdFilter (in) MOID with wild card entry. This filter is used to refine the
 *              scope of the search. For example, if the moIdRoot is /chassis:0/blade:1/,
 *              moIdFilter can have a value such as /chassis:0/blade:1/port:'*'/channel:'*'
 *  \param fp (in) The callback API invoked for every object found in COR.
 *  \param flags (in) Flags indicating walk related definitions. This parameter accepts
 *              the following values:
 *  \arg \c CL_COR_MO_WALK - The walk is performed on the MO objects of the object tree.
 *  \arg \c CL_COR_MSO_WALK - The walk is performed on the MSO nodes of the object tree.
 *  \arg \c CL_COR_MO_SUBTREE_WALK - The walk is performed on the MO objects of the object subtree.
 *  \param cookie (in)Pointer to user-defined data. This value is passed as a parameter to the
 *                 user callback API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR moIdRoot or moIdFilter is a NULL pointer.
 *  \retval CL_COR_SVC_ERR_INVALID_ID Service ID is invalid.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *
 *  \par Description:
 *  This function is used to perform a walk through the COR objects present in
 *  the object tree. The first parameter is the \e MOID of the root, from where
 *  the object walk begins. If the walk has to be performed on the entire tree,
 *  this parameter must be set to NULL.\e moIdFilter, contains filter for the
 *  search.This parameter can contain a wild card entry or a MOID. It must be set
 *  to NULL if a filter is not required for the object walk. \e fp is a callback
 *  API which has two parameters:data and cookie. \e data contains the handle to
 *  the object. \e cookie contains the cookie passed in the object
 *  walk API. \e flags indicate the type of object Walk that is required. The
 *  final parameter is the user argument cookie.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \par Note
 *  The cookie parameter can be used to pass the value used in the callback API. If moIdRoot
 *  and moIdFilter are NULL, all the MO/MSO objects are walked through irrespective of any
 *  specification in the flag value.
 *
 *  \sa clCorObjectAttributeWalk(), clCorMoIdAppend()
 *
 */

extern ClRcT clCorObjectWalk(CL_IN ClCorMOIdPtrT moIdRoot, CL_IN ClCorMOIdPtrT moIdFilter, CL_IN ClCorObjectWalkFunT fp,
                                       CL_IN ClCorObjWalkFlagsT flags, CL_IN void *cookie);

/*  -- Object handle manipulation APIs-- */
/**
 ************************************************
 *  \brief Returns the type of an object when its object handle is provided..
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param CL_IN  pHandle (in) Object handle.
 *  \param CL_OUT type (out) It can be set to CL_COR_OBJ_TYPE_MO or CL_COR_OBJ_TYPE_MSO.
 *
 *  \retval CL_OK The function is executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_UTILS_ERR_INVALID_KEY Invalid value of object Handle.
 *  \retval CL_COR_ERR_INVALID_PARAM An invalid parameter has been passed to the function.
 *   A parameter is not set correctly.
 *  \retval CL_COR_ERR_INVALID_DEPTH Invalid depth of the \e  moId.
 *  \retval CL_COR_ERR_CLASS_INVALID_PATH Qualifiers are invalid.
 *  \retval CL_COR_SVC_ERR_INVALID_ID Service ID of the \e MoId is invalid.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *
 *  \par Description:
 *  This function is used to retrieve the Object Type given an object handle.
 *  Type of an object signifies whether it is an MO or an MSO.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectHandleGet(), clCorObjectHandleToMoIdGet()
 */
extern ClRcT clCorObjectHandleToTypeGet(CL_IN ClCorObjectHandleT pHandle, CL_OUT ClCorObjTypesT* type);

extern ClRcT clCorObjectHandleServiceSet(ClCorObjectHandleT objH, ClCorServiceIdT svcId);

/**
 ************************************************
 *  \brief Returns the MOID corresponding to compressed MO handle. 
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param objHandle (in) Object handle.
 *  \param moId (out) MoId of the object.
 *  \param srvcId (out) Service Id of the object.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR moId or srvcId is a NULL pointer.
 *  \retval CL_COR_UTILS_ERR_INVALID_KEY Invalid value of object handle passed.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *
 *  \par Description:
 *  This API returns the \e MOId of a given object handle. The service ID is also filled along with the
 *  \e moId which is a valid value for an MSO.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorObjectHandleGet()
 *
 */
extern ClRcT clCorObjectHandleToMoIdGet(CL_IN ClCorObjectHandleT objHandle, CL_OUT ClCorMOIdPtrT moId, CL_OUT ClCorServiceIdT *srvcId);


extern ClRcT clCorMoIdToObjectHandleGet(ClCorMOIdPtrT pMoId, ClCorObjectHandleT* pObjH);

/**
 ************************************************
 *  \brief Adds a new route rule entry.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param CL_IN moh Handle of ClCorMOId.
 *  \param CL_IN addr COR Address. It contains card ID and EO ID.
 *
 *  \retval CL_OK The function is executed successfully.
 *  \retval CL_COR_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_MAX_DEPTH Routes for a given MoId has reached the maximum limit.
 *
 *  \par Description:
 *  This function is used to add a new rule. This rule table overrides all the entries and other
 *  routing information present in the COR. The route rule addition corresponds to adding
 *  \e moId and station list in the route table. By default all the routes are enabled.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \note
 *  For one \e moId there can be many stations <ioc address, port address> that can be added in the route list.
 *  The debug cli rmShow will show all the routelists added.
 *
 */
extern ClRcT clCorServiceRuleAdd(CL_IN ClCorMOIdPtrT moh, CL_IN ClCorAddrT addr);


/**
 ************************************************
 *  \brief Delete the station from the route list.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param CL_IN moh Handle of ClCorMOId.
 *  \param CL_IN addr COR Address. It contains slot id and EO id.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_COR_ERR_NO_MEM Failed to allocate memory.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *  \retval CL_COR_ERR_NULL_PTR MoId pointer passed is NULL.
 *  \retval CL_COR_ERR_ROUTE_NOT_PRESENT No Route list present for the given MOId.
 *
 *  \par Description:
 *  This API is used to delete the station address[addr] from the route list for a given MOId [moh]. 
 *  The station address is added by the component in the route list to participate in a transaction 
 *  incase of modification of the moId. Whenever a component goes down gracefully, it need to clean
 *  up its entry in the route list. For this the component needs to call this api in its EO finalize
 *  callback function. Once this station address
 *  is deleted 
 * 
 *  \par Library Name:
 *  libClCorClient.a
 * 
 *  \sa clCorServiceRuleAdd()
 *
 */
extern ClRcT   clCorServiceRuleDelete(CL_IN ClCorMOIdPtrT moh, CL_IN ClCorAddrT addr);


/* ---  Route Manager APIs --- */

/**
 ************************************************
 *  \brief Returns the logical slot. --- Not Supported in this release.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId MOId of the card.
 *  \param logicalSlot Logical slotId corresponding to the blade MOId.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_CARDTYPE_MISMATCH If card present is different than the one specified in MOId.
 *  \retval CL_COR_ERR_NOT_EXIST If an entry is not present for a given MOId.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing invalid parameters.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL Pointer.
 *
 *  \par Description:
 *  This function is used to return the logical slot for a given MOId.
 *
 *  \sa clCorLogicalSlotToMoIdGet()
 *
 */
ClRcT clCorMoIdToLogicalSlotGet(ClCorMOIdPtrT  pMoId, ClUint32T* logicalSlot);


/**
 ************************************************
 *  \brief Returns MOId given the Logical Slot. --- Not Supported in this release.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param logicalSlot Logical slotId corresponding to the blade MOId.
 *  \param  pMoId (out) MOId of the card.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NOT_EXIST If an entry is not present for a given MOId.
 *  \retval CL_COR_ERR_NULL_PTR On passing NULL pointer.
 *
 *  \par Description:
 *  This function is used to return \e MOId from the logical slot. The \e MOId reflects the current state of card. If a card is plugged
 *  in then the exact \e MOId is returned.If the card is not plugged in, an \e MOId with a wildcard card type is returned.
 *
 *  \sa clCorMoIdToLogicalSlotGet()
 *
 */

ClRcT clCorLogicalSlotToMoIdGet(ClUint32T logicalSlot, ClCorMOIdPtrT pMoId);

/**
 ************************************************
 *  \brief Walk is performed on the attributes of the object.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param objH (in) handle of the object
 *  \param pFilter (in)Pointer to the attribute filter. You have to provide the values for this structure.
 *  \param fp (in) User callback API called for every attribute found.
 *  \param cookie (in)The user data passed to the user callback API.
 *
 *  \retval CL_OK if the function executes successfully.
 *  \retval CL_COR_ERR_INVALID_PARAM A parameter is invalid.
 *  \retval CL_COR_ERR_NULL_PTR pFilter or cookie is a NULL pointer.
 *  \retval CL_COR_ERR_NO_MEM Failed to allocate memory.
 *  \retval CL_COR_ERR_NOT_SUPPORTED cmp_flag is not invalid.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Client version not supported.
 *  \retval CL_COR_UTILS_ERR_INVALID_KEY An invalid parameter has been passed to the
 *                                        function. A parameter is not set correctly.
 *  \retval CL_COR_INST_ERR_INVALID_MOID MoId is invalid.
 *
 *  \par pFilter Usage
 *  pFilter is used to apply filters to the walk criteria. It can have the following values:
 *   -# <tt> pFilter = NULL </tt> \n
 *      No filter is applied and it walks through all the attributes of object.
 *   -# <tt> pFilter->baseAttrWalk = CL_TRUE </tt> \n
 *      Walks through the native attributes of object pointed.
 *   -# <tt> pFilter->baseAttrWalk = CL_FALSE </tt> \n
 *      Does not walk through the native attributes.
 *   -# <tt> pFilter->contAttrWalk = CL_FALSE </tt> \n
 *      For the current implementation, this must be set to CL_FALSE.
 *   -# <tt> pFilter->pAttrPath = NULL </tt> \n
 *      This can be set to a valid attribute path or NULL. For the current
 *      implementation, this must be set to NULL.
 *   -# <tt> pFilter->cmpFlag</tt> \n
 *      This is used to compare the attrId with a specified value.
 *      Following are the various comparison flags.
 *      - CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO:
 *          The attributes whose value is equal to the specified value will be matched.
 *      - CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN:
 *          The attributes whose value is greater than the specified value will be matched.
 *      - CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS:
 *          The attributes whose value is greater than or equal to the specified value will be matched.
 *      - CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN:
 *          The attributes whose value is less than the specified value will be matched.
 *      - CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS:
 *          The attributes whose value is less than or equal to the specified value will be matched.
 *      .
 *   -# <tt> pFilter->attrWalkOption</tt> \n
 *      This can have one of the following values:
 *          - CL_COR_ATTR_WALK_ALL_ATTR: All the attributes of contained (and/or base object, depending
 *             on baseAttrWalk parameter) object are considered for walk, provided the pFilter->cmpFlag condition is true.
 *          - CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR: Only the attributes matching with the filter criteria are walked
 *            provided the pFilter->cmpFlag condition goes true.
 *
 *  \par Description:
 *  This API is used to perform walk on the attributes of an object, based on the filter. The filter contains the
 *  search criteria for the attribute walk inorder to narrow down the search. The function sends request to the COR
 *  server for attribute walk. The server then looks into its repository for the matched object. It packs the 
 *  information about the matched attribute or all the attributes of the object, based on the filter option. The
 *  attribute information include its identifier, size , type ( simpe or array), its data type, its latest value etc.
 *  For an object containing transient attributes, their latest value is fetched from the primary object implementer,
 *  if any, otherwise the default value is provided. The callback function specified in the function call is invoked 
 *  one by one for all the attributes in the client's context after unpacking the information sent by server.
 *  \par
 *  This API takes the object handle as the first parameter. The handle is obtained from the MOID using
 *  the clCorObjecthandleGet() API.
 *  The second parameter is the filter for the attribute walk. If the filter is NULL, the walk is
 *  performed on all attributes of the MO. The first element of the filter baseAttrWalk, must
 *  always be set to CL_TRUE.
 *  The parameter contAttrWalk must be set to CL_TRUE. pAttrPath must be set to
 *  NULL. The callback function should be specified when the object walk API is invoked. This
 *  callback function is called for every attribute found during the walk. The cookie (user-data)
 *  is passed as the parameter to the callback function.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \par Note
 *   -# The last parameter can be used to pass the value used by the callback API. If the first and
 *   second parameters are NULL, all the MO/MSO objects are walked through irrespective of
 *   any specification in the flag.
 *
 *  \sa clCorObjectHandleGet(), clCorMoIdCompare()
 *
 */

ClRcT
clCorObjectAttributeWalk(CL_IN ClCorObjectHandleT objH, CL_IN ClCorObjAttrWalkFilterT *pFilter,
                              CL_IN ClCorObjAttrWalkFuncT   fp, CL_IN void * cookie);


/**
 ************************************************
 *  \brief Verifies the version supported by COR.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param version Version supported.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED If version is not supported.
 *
 *  \par Description:
 *  This function is used to verify if the version is supported or not. If the version is not supported then the
 *  error is returned with the value of supported version filled in the out parameter.
 *
 *  \par Library File:
 *  ClCorClient
 *
 */
ClRcT
clCorVersionCheck( CL_INOUT ClVersionT *version);


/**
 ************************************************
 *  \brief This function can be used to get node name corresponding to a moId which is specified
 *  in the ASP configuration file.
 *
 *  \par Header File:
 *   clCorApi.h
 *
 *  \param pMoId MoId for which the node name is required.
 *  \param nodeName Node Name corresponding to the \e moId supplied.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NOT_EXIST If node name for the moId is not present.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *
 *  \par Description:
 *  The function can be used to get the node name corresponding to the moid, supplied in the
 *  config file of the ASP (clAmfConfig.xml).
 *
 *  \sa clCorNodeNameToMoIdGet()
 */
ClRcT
clCorMoIdToNodeNameGet(CL_IN ClCorMOIdPtrT pMoId, CL_OUT SaNameT* nodeName);

/**
 ************************************************
 *  \brief This function can be used to give the MoId corresponding to the node name supplied.
 *
 *  \par Header File:
 *   clCorApi.h
 *
 *  \param nodeName Node name for which the \e moId is desired.
 *  \param pMoId (out) \e moId is supplied by the user.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NOT_EXIST If \e moId for the node name is not present.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED version is not supported.
 *
 *  \par Description:
 *  This function is used to get the moId corresponding to the node name which is there in the
 *  config file of the ASP(clAmfConfig.xml).
 *
 *  \sa clCorMoIdToNodeNameGet()
 */
ClRcT
clCorNodeNameToMoIdGet(CL_IN SaNameT nodeName, CL_OUT ClCorMOIdPtrT  pMoId);

/**
 ************************************************
 *  \brief A component can register itself as an OI through this API.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId (in) This is pointer to MOID of the MO.
 *  \param pCompAddr (in) IOC address of OI.
 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NO_MEM Event library is not initialized.
 *  \retval CL_COR_ERR_TIMEOUT The server failed to send the response within the time limit.
 *  \retval CL_COR_ERR_NULL_PTR pMoid or pCompAddr is NULL.
 *
 *  \par Description:
 *  A component (specified by compAddr) can register itself as an OI for an MO (pointed by
 *  *pMoId) through this API. The MOID can be a qualified MOID or a wild card MOID. This
 *  API is synchronous in nature.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorOIUnregister (), clCorPrimaryOIGet (), clCorPrimaryOISet().
 *
 */
ClRcT clCorOIRegister ( CL_IN const ClCorMOIdPtrT pMoId, 
                        CL_IN const ClCorAddrPtrT pCompAaddr);

/**
 ************************************************
 *  \brief De-register the component acting as the OI.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId (in) This is pointer to MOID of the MO.
 *  \param pCompAddr (in) The component that needs to be de-registered as the OI. The MOID can
 *                  be a qualified MOID or a wild card MOID.
 *
 *  \retval CL_OK If the OI un-registration operation is successful.
 *  \retval CL_COR_ERR_OI_NOT_REGISTERED Component was not registered as OI for this MO given by pMoId.
 *  \retval CL_COR_ERR_NULL_PTR pCompAddr or pMoId is a NULL pointer.
 *  \retval CL_COR_ERR_TIMEOUT The server failed to send the response within the time limit.
 *
 *  \par Description:
 *  If this API is executed successfully, the component pointed by pCompAddr discontinues to
 *  be the OI. If this component is the primary OI, it discontinues to act as the primary OI.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorOIRegister().
 *
 */
ClRcT clCorOIUnregister ( CL_IN const ClCorMOIdPtrT pMoId, 
                          CL_IN const ClCorAddrPtrT pCompAddr);

/**
 ************************************************
 *  \brief Sets a component as the primary OI.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId (in) Pointer to MOID of the MO. It can be a qualified MOID or wild card MOID.
 *  \param pCompAddr (in) This is the pointer to the component address structure.
 *
 *  \retval CL_OK If the call is successful, the OI provided by the component address becomes the
 *              Primary OI for this MO.
 *  \retval CL_COR_ERR_OI_NOT_REGISTERED An OI can be a primary OI only if is registered as an
 *              OI. This error indicates that the OI is attempting to be a primary OI without registering
 *              as an OI.
 *  \retval CL_COR_ERR_ROUTE_PRESENT A Primary OI already exists for the MO pointed by pMoId.
 *  \retval CL_COR_ERR_NULL_PTR pMoid, pCompAddr, or pMoId is a NULL pointer.
 *  \retval CL_COR_ERR_TIMEOUT Server failed to send the response within the time limit.
 *
 *  \par Description:
 *  This API sets the OI specified by pCompAddr as a primaryOI. A component can become a
 *  primary OI only if it has registered itself as OI using clCorOIRegister() API. This is a
 *  synchronous API.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \note
 *  If a Primary OI set request is coming for a MoId, then COR will search to find any of the existing MoIds matches 
 *  with the incoming request either in exact match or in wild card match. If any match is found, then it will look 
 *  for the existence of the primary OI in that entry. If it has the primary OI and it is different from 
 *  the current OI being set, then it will throw error CL_COR_ERR_ROUTE_PRESENT. 
 *  \par
 *  If both existing primary OI and the current OI being set as the Primary OI are the same, then in this case
 *  it will check whether the MoId match is exact match. If it is exact match then will return CL_OK. Because the 
 *  same OI is again set as the Primary OI. If the MoId match is not an exact match, then it will throw error 
 *  CL_COR_ERR_ROUTE_PRESENT saying that the current OI being set as the primary OI is already registered as the 
 *  primary OI for a different MoId matching with the current MoId. 
 *  \par
 *  If both existing primary OI and the current OI being set as the Primary OI are different, then it will throw
 *  error CL_COR_ERR_ROUTE_PRESENT. This is to avoid the confusion of selecting one primary OI among these two 
 *  to forward the read requests on the MO.
 *  \par  
 *  If no primary OI already exists, then COR will go and set the primary OI flag for the exact MoId match. If the
 *  exactly matching MoId entry is not present, then COR will throw error CL_COR_ERR_OI_NOT_REGISTERED.
 *  \par
 *  By doing this way, COR is restricting that in a group of wild card matching MoIds, any one of the OI can 
 *  act as the Primary OI 
 *
 *  \sa clCorOIRegister(), clCorPrimaryOIGet(), clCorPrimaryOIClear().
 * 
 */

ClRcT clCorPrimaryOISet ( CL_IN const ClCorMOIdPtrT pMoId, 
                          CL_IN const ClCorAddrPtrT pCompAddr);

ClRcT clCorNIPrimaryOISet ( CL_IN const ClCharT *pResource);

/**
 ************************************************
 *  \brief De-register the component that is acting as the primary OI.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId (in) Pointer to MOID of the MO.
 *  \param pCompAddr (in) Pointer to the component address structure.
 *
 *  \retval CL_OK The API is successfull and the OI is not any more acting as a primary OI.
 *  \retval CL_COR_ERR_OI_NOT_REGISTERED This OI is not the Primary OI.
 *  \retval CL_COR_ERR_NULL_PTR pCompAddr or pMoId is NULL.
 *  \retval CL_COR_ERR_TIMEOUT Server failed to send the response within the time limit.
 * 
 *  \par Description:
 *  This API cancels the registration of the component as the primary OI. If this API is
 *  successfully executed, the OI pointed by pCompAddr discontinues to act as the primary OI
 *  for the MO (pointed by the pMoId). This is a synchronous call.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa  clCorOIRegister(), clCorPrimaryOIGet().
 *
 */
ClRcT clCorPrimaryOIClear ( CL_IN const ClCorMOIdPtrT pMoId, 
                            CL_IN const ClCorAddrPtrT pCompAddr);

ClRcT clCorNIPrimaryOIClear ( CL_IN const ClCharT *pResource);

/**
 ************************************************
 *  \brief Obtains the primary OI for a given MO.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pMoId (in) Pointer to MOID of the MO.
 *  \param pCompAddr (out) Pointer to the component address structure.
 *
 *  \retval CL_OK The call is successful and the address of the component is populated in the
 *              second parameter.
 *  \retval CL_COR_ERR_OI_NOT_REGISTERED There is no Primary OI registered for this MO.
 *  \retval CL_COR_ERR_NULL_PTR pCompAddr or pMoId is NULL.
 *  \retval CL_COR_ERR_TIMEOUT Server failed to send the response within the time limit.
 *
 *  \par Description:
 *   If this API is executed successfully, it obtains the primary OI for the MO pointed by pMoid.
 *   The component address of the primary OI is returned in pCompAddr.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorOIRegister(), clCorPrimaryOIGet(), clCorPrimaryOIClear().
 *
 */

ClRcT clCorPrimaryOIGet ( CL_IN const ClCorMOIdPtrT pMoId, 
                          CL_OUT ClCorAddrPtrT pCompAddr);

/**
 ************************************************
 *  \brief Creates a bundle and returns a unique handle identifying the bundle.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param pBundleHandle (out) This parameter identifies the bundle. This function
 *  will create the bundle and fill the bundle handle in pBundleHandle.
 *  \param pBundleConfig (in) The pBunldeConfig->bundleType element indicates 
 *  if the bundle is a transactional bundle or a non transactional bundle.
 *
 *  \retval  CL_OK The API executed successfully. The job was successfully queued in the bundle.
 *  \retval  CL_COR_ERR_NO_MEM Bundle initialization failed due to insufficient memory.
 *  \retval  CL_COR_ERR_BUNDLE_INIT_FAILURE Generic bundle initialization failure.
 *  \retval  CL_COR_ERR_BUNDLE_INVALID_CONFIG The transactional bundle configuration is
 *  currently not supported.
 *  \retval  CL_COR_ERR_NULL_PTR pBundleHandle or pBundleConfig is NULL.
 *
 *  \par Description:
 *   This API creates a bundle and returns a unique handle that identifies this bundle.
 *   clCorBundleObjectGet(), clCorBundleApply(), clCorBundleApplyAsync(),
 *   and clCorBundleFinalize() APIs use this handle to uniquely identify the bundle.
 *   The handle to the bundle is valid untill it is finalized. The pBundleConfig->bundleType
 *   gives the type of the bundle. In this release, only the CL_COR_BUNDLE_NON_TRANSACTIONAL
 *   is supported.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleApply, clCorBundleApplyAsync, clCorBundleFinalize.
 *
 */
ClRcT clCorBundleInitialize( CL_OUT ClCorBundleHandlePtrT pBundleHandle,
                             CL_IN ClCorBundleConfigPtrT  pBundleConfig);


/**
 ************************************************
 *  \brief Function to apply the bundle asynchronously.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param bundleHandle (in) This parameter identifies the bundle.
 *  \param funcPtr (in) The function pointer of type ClCorBundleCallbackPtrT which will 
 *  be executed once the response for a given bundle is received.
 *  \param userArg (in) The cookie or the user argument which will be passed to the callback
 *  function along with the bundle handle once the reponse for the bundle is received.
 *
 *  \retval CL_OK The API executed successfully. The bundle is applied successfully. 
 *  \retval CL_COR_ERR_NO_MEM  The bundle could not be applied due to insufficient memory.
 *  \retval CL_COR_ERR_NULL_PTR when the the pointer to the callback function specified by 
 *  <funcPtr>passed is NULL. Also if there is no information present for the given bundleHandle.
 *  \retval CL_COR_ERR_ZERO_JOBS_BUNDLE The bundle applied don't have any being added. 
 *  \retval CL_COR_ERR_INVALID_HANDLE The handle passed in invalid.
 *  \retval CL_COR_ERR_BUNDLE_IN_EXECUTION The bundle specified by <bundlehandle> is
 *  executing at the server while the same bundle is applied again.
 *  \retval CL_COR_ERR_BUNDLE_APPLY_FAILURE The bundle apply has been unsuccessfull.
 *
 *  \par Description:
 *  This function is used to apply the bundle asynchronously. The function submits the
 *  bundle and gets unblocked. The callback function should be provided with this function
 *  which will be executed after the bundle response from the server is received.
 *  
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleInitialize, clCorBundleApply, clCorBundleFinalize.
 *
 */

ClRcT clCorBundleApplyAsync(CL_IN ClCorBundleHandleT bundleHandle,
                         CL_IN ClCorBundleCallbackPtrT funcPtr,
                         CL_IN ClPtrT               userArg);

/**
 ***********************************************
 *  \brief Submits a bundle to the COR server for execution.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param bundleHandle (in) This parameter identifies the bundle.
 *
 *  \retval CL_OK The API executed successfully. The bundle is applied successfully. 
 *  \retval CL_COR_ERR_NO_MEM  The bundle could not be applied due to insufficient memory.
 *  \retval CL_COR_ERR_NULL_PTR If there is no information present for the given bundleHandle.
 *  \retval CL_COR_ERR_ZERO_JOBS_BUNDLE The bundle applied don't have any being added. 
 *  \retval CL_COR_ERR_INVALID_HANDLE The handle passed in invalid.
 *  \retval CL_COR_ERR_BUNDLE_IN_EXECUTION The bundle specified by <bundlehandle> is
 *  executing at the server while the same bundle is applied again.
 *  \retval CL_COR_ERR_BUNDLE_APPLY_FAILURE The bundle apply has been unsuccessful.
 *
 *  \par Description:
 *  The API operates synchronously. This API submits the bundle to the server for execution.
 *  The application blocks till a timeout or the bundle is successfully executed. After the bundle
 *  is executed successfully, the attribute value descriptor corresponding to Read-Jobs in the
 *  bundle can be accessed. The application can then free the attribute value descriptor. The
 *  bundle cannot be applied, if this API returns any errors described in the Return values
 *  section.
 *  
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleObjectGet, clCorBundleApplyAsync, clCorBundleFinalize.
 *
 */
ClRcT clCorBundleApply(CL_IN ClCorBundleHandleT bundleHandle);

    ClRcT clCorBundleApplySync(CL_IN ClCorBundleHandleT  bundleHandle);
    
    

/**
 *  \brief Finalizes the bundle.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param bundleHandle (in/out) This parameter identifies the bundle.
 *
 *  \retval CL_OK Bundle is finalized successfully.
 *  \retval CL_COR_ERR_NULL_PTR The information corresponding to the 
 *  bundleHandle is missing.
 *  \retval CL_COR_ERR_BUNDLE_IN_EXECUTION The bundle corresponding to the 
 *  bundleHandle is in progress.
 *  \retval CL_COR_ERR_BUNDLE_FINALIZE Failure while finalizing the bundle.
 *  
 *
 *  \par Description:
 *  This API finalizes the bundle and frees all the resources associated with the bundle.
 *  The bundleHandle then becomes invalid after this function is called.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleInitialize.
 *
 */
ClRcT clCorBundleFinalize(ClCorBundleHandleT bundleHandle);

ClRcT clCorBundleAttrValueSet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_IN   ClPtrT               *pValue);

/**
 ***********************************************
 *  \brief Populates a bundle with read-jobs.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param bundleHandle (in) This parameter identifies the bundle.
 *  \param objHandle (in) This parameter contains the description of the job that needs to be
 *  operated.
 *  \param pAttrDescList (in/out) This parameter describes the list of attributes on which read
 *  operation needs to be performed. ObjHandle and attrDescList together form a read-job.
 *
 *  \retval CL_OK The API executed successfully. The Job was successfully queued in the bundle.
 *  \retval CL_COR_ERR_NO_MEM This job is not added to the bundle. Bundle unsuccessful due
 *  to insufficient memory.
 *  \retval CL_COR_ERR_NULL_PTR pAttrDescList or objHandle is NULL or the data buffer
 *  corresponding to any of the attribute is NULL. This job is not added into the bundle.
 *  \retval CL_COR_ERR_INVALID_PARAM pAttrDescList->numOfDescriptor is NULL.
 *  \retval CL_COR_ERR_INVALID_HANDLE Invalid bundle handle. This job is not added to the
 *  bundle.
 *  \retval CL_COR_ERR_BUNDLE_IN_EXECUTION The bundle specified by <bundlehandle> is
 *  executing at the server while a new read-job is being added at the client side.
 *
 *  \par Description:
 *  This API populates a bundle with read-jobs. It can be called repeatedly to queue all the
 *  required read-jobs into a bundle. The API returns after queuing the jobs in the bundle. If
 *  there is a failure encountered (as indicated by the return value of this API), the jobs are not
 *  added into the bundle. The status and the data associated with each attribute (specified in
 *  attribute value descriptor) can be accessed only when the bundle execution is completed.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleInitialize, clCorBundleApply, clCorBundleApplyAsync, clCorBundleFinalize.
 *
 */
ClRcT clCorBundleObjectGet( CL_IN    ClCorBundleHandleT bundleHandle,
                            CL_IN    const ClCorObjectHandleT *pObjectHandle,
                            CL_INOUT ClCorAttrValueDescriptorListPtrT pAttrList);

ClRcT clCorNIAttrIdGet(ClCorClassTypeT classId, char *name,  ClCorAttrIdT  *attrId);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_API_H_ */
/** \} */

