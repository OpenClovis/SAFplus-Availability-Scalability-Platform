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
 * ModuleName  : ckpt                                                          
 * File        : clCkptExtApi.h
 *******************************************************************************/

/*******************************************************************************
* Description :                                                                
*
*    Checkpoint service provides more functionality than whatever described
*    in SAF.  This file contains APIs to access such functionality 
*
*
*****************************************************************************/

/**
 *  \file
 *  \brief Header file of Library based Checkpoint Service Related APIs
 *  \ingroup ckpt_apis_library
 */

/**
 ************************************
 *  \addtogroup ckpt_apis_library
 *  \{
 */

#ifndef _CL_CKPT_EXT_API_H_
#define _CL_CKPT_EXT_API_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clCkptApi.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>
#include <clDifferenceVector.h>

/**
 * Serialize Signature.
 */
typedef ClRcT (*ClCkptSerializeT)(

/**
 *  Data Set Id.
 */
                                  ClUint32T,

/**
 *  Address for encoded buffer.
 */
                                  ClAddrT *,

/**
 *  Length of the encoded buffer.
 */
                                  ClUint32T *,
/**
  *cookie.
*/                                  ClPtrT );


/**
 *  De-serialize Signature.
 */
typedef ClRcT (*ClCkptDeserializeT)(

/**
 * Data Set Id.
 */
                                   ClUint32T,

/**
 * Pointer to the encoded buffer.
 */
                                   ClAddrT ,

/**
 * Length of the buffer.
 */
                                   ClUint32T,
/**
 * cookie.
 */
                                   ClPtrT );

/*
 * Data set versioned callbacks for serializing and deserializing on-disk data.
 */
typedef struct ClCkptDataSetCallback
{
    ClVersionT version; 
    ClCkptSerializeT serialiser;
    ClCkptDeserializeT deSerialiser;
}ClCkptDataSetCallbackT;

typedef struct ClCkptDifferenceIOVectorElement
{
/**
 *  Identifier of the section.
 */
    ClCkptSectionIdT      sectionId;
    ClSizeT dataSize;
    ClOffsetT dataOffset;
    ClDifferenceVectorT *differenceVector;  /* the dataBuffer difference vector representation*/
}ClCkptDifferenceIOVectorElementT;

/**
 ************************************
 *  \brief Initializes the client.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param pCkptHdl (in/out) Handle to the client. This handle designates this particular
 *  initialization of the service.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER When the parameter \e pCkptHdl passed is a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This function is used to initialize the client and allocates resources to it. The function
 *  returns a handle that associates this particular initialization of the Checkpoint library.
 *  This handle must be passed as the first input parameter for all invocations
 *  of functions related to this library.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryFinalize()
 *
 */
extern ClRcT clCkptLibraryInitialize(
                                      CL_INOUT ClCkptSvcHdlT  *pCkptHdl );

extern ClRcT clCkptLibraryInitializeDB(
                                       CL_INOUT ClCkptSvcHdlT  *pCkptHdl,
                                       const ClCharT *dbName);

/**
 ************************************
 *  \brief  Destroys the client.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \par Description:
 *  This function is used to close the association with checkpoint service client.
 *  It must be invoked when the services are no longer required.
 *  This invocation frees all allocated resources.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryInitialize()
 *
 */
extern ClRcT clCkptLibraryFinalize( CL_IN ClCkptSvcHdlT ckptHdl);

/**
 ************************************
 *  \brief Creates the checkpoint.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This function is used to create the checkpoint. This function must be invoked before any
 *  further operations can be done with the checkpoint.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryCkptDelete
 *
 */
extern ClRcT clCkptLibraryCkptCreate( CL_IN ClCkptSvcHdlT  ckptHdl,
                                      CL_IN ClNameT       *pCkptName);


/**
 ************************************
 *  \brief Deletes the checkpoint.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This function is used to delete the checkpoint. This must be invoked when the checkpoint
 *  services are no longer required. This invocation frees all resources associated with the checkpoint.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryCkptCreate()
 *
 */


extern ClRcT clCkptLibraryCkptDelete(CL_IN ClCkptSvcHdlT ckptHdl,
                              CL_IN ClNameT     *pCkptName);


/**
 ************************************
 *  \brief Creates a data set.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId A unique identifier for identifying the data set.
 *  \param grpId An optional group Id to group different data sets together. [Currently not used]
 *  \param order When all the members of group are checkpointed the order in which this
 *  particular data set is checkpointed. [Currently not used]
 *  \param dsSerialiser Serialiser or encoder for the data.
 *  \param dsDeserialiser Deserialiser/decoder for the data.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This function is used to create a data set and allocate resources to it.
 *  This function should be invoked before performing any further operations
 *  on the dataset. The dataset information can then be read, or written into
 *  the database.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa
 *  clCkptLibraryCkptDataSetDelete(),
 *  clCkptLibraryCkptDataSetWrite(),
 *  clCkptLibraryCkptDataSetRead()
 *
 */
extern ClRcT clCkptLibraryCkptDataSetCreate( CL_IN ClCkptSvcHdlT    ckptHdl,
                                      CL_IN ClNameT          *pCkptName,
                                      CL_IN ClUint32T         dsId,
                                      CL_IN ClUint32T         grpId,
                                      CL_IN ClUint32T         order,
                                      CL_IN ClCkptSerializeT   dsSerialiser,
                                      CL_IN ClCkptDeserializeT dsDeserialiser);

extern ClRcT clCkptLibraryCkptDataSetVersionCreate( CL_IN ClCkptSvcHdlT    ckptHdl,
                                      CL_IN ClNameT          *pCkptName,
                                      CL_IN ClUint32T         dsId,
                                      CL_IN ClUint32T         grpId,
                                      CL_IN ClUint32T         order,
                                      CL_IN ClCkptDataSetCallbackT *pTable,
                                      CL_IN ClUint32T numTableEntries);
                                                    

/**
 ************************************
 *  \brief Deletes the dataset from the checkpoint.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NOT_EXIST The specified argument is not exist.
 *
 *  \par Description:
 *  This function is used to delete the dataset from the checkpoint.
 *  Before this function is invoked, the dataset must have been created
 *  using the clCkptLibraryCkptDataSetCreate() function.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa
 *  clCkptLibraryCkptDataSetCreate(),
 *  clCkptLibraryCkptDataSetWrite(),
 *  clCkptLibraryCkptDataSetRead()
 *
 */
extern ClRcT clCkptLibraryCkptDataSetDelete( CL_IN ClCkptSvcHdlT  ckptHdl,
                                      CL_IN ClNameT       *pCkptName,
                                      CL_IN ClUint32T      dsId );

/**
 ************************************
 *  \brief Writes the dataset information into the database.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be written.
 *  \param cookie User-data which will be opaquely passed to the serializer.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST The specified argument is not exist.
 *
 *  \par Description:
 *  This function is used to write the dataset information into the database.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa
 *  clCkptLibraryCkptDataSetCreate(),
 *  clCkptLibraryCkptDataSetDelete(),
 *  clCkptLibraryCkptDataSetRead()
 *
 */
extern ClRcT clCkptLibraryCkptDataSetWrite(CL_IN ClCkptSvcHdlT   ckptHdl,
                                    CL_IN ClNameT        *pCkptName,
                                    CL_IN ClUint32T       dsId,
                                    CL_IN ClPtrT       cookie );

extern ClRcT clCkptLibraryCkptDataSetVersionWrite(CL_IN ClCkptSvcHdlT   ckptHdl,
                                    CL_IN ClNameT        *pCkptName,
                                    CL_IN ClUint32T       dsId,
                                    CL_IN ClPtrT          cookie,
                                    CL_IN ClVersionT      *pVersion);

/**
 ************************************
 *  \brief Reads the dataset information into the database.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be written.
 *  \param cookie User-data which will be opaquely passed to the serializer.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST The specified  argument does not exist.
 *
 *  \par Description:
 *  This function is used to read the Dataset information from database.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa
 *  clCkptLibraryCkptDataSetCreate(),
 *  clCkptLibraryCkptDataSetDelete(),
 *  clCkptLibraryCkptDataSetRead()
 *
 */
extern ClRcT clCkptLibraryCkptDataSetRead( CL_IN ClCkptSvcHdlT      ckptHdl,
                                    CL_IN ClNameT           *pCkptName,
                                    CL_IN ClUint32T          dsId,
                                    CL_IN ClPtrT          cookie);

/**
 ************************************
 *  \brief Checks the existence of a checkpoint.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param pRetVal Return value to the user, returns \c CL_TRUE if it exists.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This function is used to check if the given checkpoint exists or not.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryDoesDatasetExist()
 *
 */
extern ClRcT clCkptLibraryDoesCkptExist(CL_IN  ClCkptSvcHdlT      ckptHdl,
                                        CL_IN  ClNameT           *pCkptName,
                                        CL_OUT ClBoolT           *pRetVal);

/**
 ************************************
 *  \brief Checks the existence of a Dataset in a given checkpoint.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset.
 *  \param pRetVal Return value to the user, returns CL_TRUE, if it exists.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This function is used to check whether the given Dataset exists.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryDoesCkptExist()
 *
 */
extern ClRcT clCkptLibraryDoesDatasetExist( CL_IN  ClCkptSvcHdlT    ckptHdl,
                                            CL_IN  ClNameT            *pCkptName,
                                            CL_IN  ClUint32T           dsId,
                                            CL_OUT ClBoolT            *pRetVal);
/**
 ************************************
 *  \brief Creates the element of the Dataset.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be written.
 *  \param elemSerialiser  Encoder of the element Data
 *  \param elemDeserialiser Decoder of the element Data.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST The specified argument is not exist.
 *
 *  \par Description:
 *  This function is used to create an element of the dataset. It must
 *  be invoked before any element of the dataset can be written.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryCkptElementWrite()
 *
 */
extern ClRcT clCkptLibraryCkptElementCreate(ClCkptSvcHdlT        ckptHdl,
                                     ClNameT              *pCkptName,
                                     ClUint32T            dsId,
                                     ClCkptSerializeT     elemSerialiser,
                                     ClCkptDeserializeT   elemDeserialiser);

extern ClRcT clCkptLibraryCkptElementVersionCreate(ClCkptSvcHdlT        ckptHdl,
                                     ClNameT              *pCkptName,
                                     ClUint32T            dsId,
                                     ClCkptDataSetCallbackT *pTable,
                                     ClUint32T numTableEntries);

/**
 ************************************
 *  \brief Writes the Element information into the database.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be written.
 *  \param elemName Identifier of the element.
 *  \param elemLen Length of the \e elemId.
 *  \param cookie User data. This is opaquely passed to the serializer.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing \e pCkptName as a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NOT_EXIST The specified argument does not exist.
 *
 *  \par Description:
 *  This function is used to write the Element information into the database.
 *  Before this function is invoked, the element must be created
 *  using the clCkptLibraryCkptElementCreate() function.
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryCkptElementCreate()
 *
 */
extern ClRcT clCkptLibraryCkptElementWrite (ClCkptSvcHdlT   ckptHdl,
                                    ClNameT        *pCkptName,
                                    ClUint32T       dsId,
                                    ClPtrT          elemId,
                                    ClUint32T       elemLen,
                                    ClPtrT       cookie);

extern ClRcT clCkptLibraryCkptElementVersionWrite (ClCkptSvcHdlT   ckptHdl,
                                    ClNameT        *pCkptName,
                                    ClUint32T       dsId,
                                    ClPtrT          elemId,
                                    ClUint32T       elemLen,
                                    ClPtrT       cookie,
                                    ClVersionT   *pVersion);

/**
 ************************************
 *  \brief Writes the Element information into the database.
 *
 *  \par Header File:
 *  clCkptExtApi.h
 *
 *  \param ckptHdl Handle to the client obtained from the clCkptLibraryInitialize()
 *  function, designating this particular initialization of the Checkpoint library.
 *  \param pCkptName Name of the Checkpoint.
 *  \param dsId Identifier of the dataset to be written.
 *  \param elemId Identifier of the element.
 *  \param elemLen Length of the \e elemId.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing \e pCkptName as a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NOT_EXIST The specified argument does not exist.
 *
 *  \par Description:
 *  This function is used to delete element data which was written already by
 *  API clCkptLibraryCkptElementDelete.
 *  Before this function is invoked, the element must be created and key
 *  should exist. 
 *
 *  \par Library File:
 *  clCkpt
 *
 *  \sa clCkptLibraryCkptElementCreate()
 *
 */
extern ClRcT clCkptLibraryCkptElementDelete (ClCkptSvcHdlT   ckptHdl,
                                    ClNameT        *pCkptName,
                                    ClUint32T       dsId,
                                    ClPtrT          elemId,
                                    ClUint32T       elemLen);

extern ClRcT clCkptReplicaChangeRegister(ClRcT (*pCkptRelicaChangeCallback)
                                         (const ClNameT *pCkptName, ClIocNodeAddressT replicaAddr));

extern ClRcT clCkptReplicaChangeDeregister(void);

ClRcT clCkptSectionOverwriteVector(ClCkptHdlT               ckptHdl,
                                   const ClCkptSectionIdT   *pSectionId,
                                   ClSizeT                  dataSize,
                                   ClDifferenceVectorT         *differenceVector);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_CKPT_EXT_API_H_*/

/**
 *  \}
 */
