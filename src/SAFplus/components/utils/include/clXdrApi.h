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
/*********************************************************************
 * ModuleName  : utils
 * File        : clXdrApi.h
 ********************************************************************/

/*********************************************************************
 * Description :
 *     This file contains IDL APIs declarations. 
 ********************************************************************/

/******************************************************************************
 * Description :
 *
 * OpenClovis XDR Library API file.
 *
 ****************************************************************************/

/**
 * \file 
 * \brief Header file of XDR and IDL related APIs
 * \ingroup xdr_apis
 */


/**
 *  \addtogroup xdr_apis
 *  \{
 */


#ifndef _CL_XDR_API_H_
#define _CL_XDR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <clCommon.h>
#include <clBufferApi.h>
#include <clHandleApi.h>
#include <clXdrErrors.h>
#include <clIocApi.h>
#include <clEoApi.h>
#include <clMD5Api.h>
#include <clDifferenceVector.h>

#define CL_XDR_ENTER(x)
#define CL_XDR_EXIT(x)
#define CL_XDR_PRINT(x)

typedef ClRcT (*ClXdrMarshallFuncT) (void* pPyld,
                                     ClBufferHandleT msg,
                                     ClUint32T isDelete);

typedef ClRcT (*ClXdrUnmarshallFuncT) (ClBufferHandleT msg,
                                       void* pPyld);


/********************************************************************/
typedef struct ClIdlContextInfo
{
    ClBufferHandleT idlDeferMsg;
    ClBoolT         inProgress;
}ClIdlContextInfoT;

typedef struct ClIdlSyncInfo
{
    ClBufferHandleT idlRmdDeferMsg;
    ClHandleT         idlRmdDeferHdl;
}ClIdlSyncInfoT;

typedef ClHandleT ClIdlHandleT;

ClRcT clIdlSyncPrivateInfoSet(ClUint32T idlSyncKey, void *  pIdlCtxInfo);

ClRcT clIdlSyncPrivateInfoGet(ClUint32T idlSyncKey, void **  data);

ClRcT clIdlSyncResponseSend(ClIdlHandleT idlRmdHdl,ClBufferHandleT rmdMsgHdl,ClRcT retCode);

ClRcT clIdlSyncDefer(ClHandleDatabaseHandleT idlDbHdl, ClUint32T idlSyncKey, ClIdlHandleT *pIdlHdl);

ClRcT clXdrMarshallArray(void* array,
                         ClUint32T typeSize,
                         ClUint32T multiplicity,
                         ClXdrMarshallFuncT func,
                         ClBufferHandleT msg,
                         ClUint32T isDelete);

ClRcT clXdrUnmarshallArray(ClBufferHandleT msg,
                           void* array,
                           ClUint32T typeSize,
                           ClUint32T multiplicity,
                           ClXdrUnmarshallFuncT func);

ClRcT clXdrMarshallPtr(void* pointer,
                           ClUint32T typeSize,
                           ClUint32T multiplicity,
                           ClXdrMarshallFuncT func,
                           ClBufferHandleT msg,
                           ClUint32T isDelete);

ClRcT clXdrUnmarshallPtr(ClBufferHandleT msg,
                             void** pointer,
                             ClUint32T typeSize,
                             ClUint32T multiplicity,
                             ClXdrUnmarshallFuncT func);

/********************************************************************/

/**
 ************************************************
 *  \brief Marshall data of ClUint8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClUint8T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint8T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClUint8T
 */
ClRcT clXdrMarshallClUint8T(CL_IN    void*              pPyld,
                            CL_INOUT ClBufferHandleT    msg,
                            CL_IN    ClUint32T          isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClUint8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClUint8T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint8T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClUint8T()
 */
ClRcT clXdrUnmarshallClUint8T(ClBufferHandleT msg,
                              void* pPyld );

/**
 ************************************************
 *  \brief Marshall array of ClUint8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClUint8T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint8T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint8T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClUint8T()
 */

ClRcT clXdrMarshallArrayClUint8T(void* pPyld,
                                 ClUint32T count,
                                 ClBufferHandleT msg,
                                 ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClUint8T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint8T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClUint8T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint8T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClUint8T()
 */

ClRcT clXdrUnmarshallArrayClUint8T(ClBufferHandleT msg,
                                   void* pPyld,
                                   ClUint32T count);

/**
 ************************************************
 *  \brief Marshall pointer of ClUint8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClUint8T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint8T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint8T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClUint8T()
 */

ClRcT clXdrMarshallPtrClUint8T(void* pPyld,
                                   ClUint32T count,
                                   ClBufferHandleT msg,
                                   ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClUint8T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint8T pointer type and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type 
 *  ClUint8T* to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint8T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClUint8T()
 */

ClRcT clXdrUnmarshallPtrClUint8T(ClBufferHandleT msg,
                                 void** pPyld,
                                 ClUint32T multiplicity);
/**
 ************************************************
 *  \brief Marshall data of ClCharT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClCharT, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClCharT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClCharT
 */
ClRcT clXdrMarshallClCharT(void* pPyld,
                            ClBufferHandleT msg,
                            ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClCharT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClCharT and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClCharT type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClCharT
 */

ClRcT clXdrUnmarshallClCharT(ClBufferHandleT msg,
                              void* pPyld );
/**
 ************************************************
 *  \brief Marshall array of ClCharT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClCharT, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClCharT to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClCharT array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClCharT()
 */

ClRcT clXdrMarshallArrayClCharT(void* pPyld,
                                 ClUint32T count,
                                 ClBufferHandleT msg,
                                 ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClCharT array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClCharT array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClCharT in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClCharT array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClCharT()
 */

ClRcT clXdrUnmarshallArrayClCharT(ClBufferHandleT msg,
                                   void* pPyld,
                                   ClUint32T count);
/**
 ************************************************
 *  \brief Marshall pointer of ClCharT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClCharT, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClCharT to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClCharT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClCharT()
 */
ClRcT clXdrMarshallPtrClCharT(void* pPyld,
                                   ClUint32T count,
                                   ClBufferHandleT msg,
                                   ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClCharT pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is of type void** 
 *  and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClCharT*
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClCharT pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClCharT()
 */
ClRcT clXdrUnmarshallPtrClCharT(ClBufferHandleT msg,
                                void** pPyld,
                                ClUint32T multiplicity);

/**
 ************************************************
 *  \brief Marshall data of ClUint16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClUint16T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint16T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClUint16T()
 */
ClRcT clXdrMarshallClUint16T(void* pPyld,
                             ClBufferHandleT msg,
                             ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClUint16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClUint16T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint16T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClUint16T
 */

ClRcT clXdrUnmarshallClUint16T(ClBufferHandleT msg,
                               void* pPyld);
/**
 ************************************************
 *  \brief Marshall array of ClUint16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClUint16T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint16T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint16T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClUint16T()
 */

ClRcT clXdrMarshallArrayClUint16T(void* pPyld,
                                  ClUint32T count,
                                  ClBufferHandleT msg,
                                  ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClUint16T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint16T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClUint16T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint16T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClUint16T()
 */

ClRcT clXdrUnmarshallArrayClUint16T(ClBufferHandleT msg,
                                    void* pPyld,
                                    ClUint32T count);

/**
 ************************************************
 *  \brief Marshall pointer of ClUint16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClUint16T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint16T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint16T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClUint16T()
 */
ClRcT clXdrMarshallPtrClUint16T(void* pPyld,
                                    ClUint32T count,
                                    ClBufferHandleT msg,
                                    ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClUint16T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint16T pointer type and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type 
 *  ClUint16T* to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint16T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClUint16T()
 */
ClRcT clXdrUnmarshallPtrClUint16T(ClBufferHandleT msg,
                                  void** pPyld,
                                  ClUint32T multiplicity);

/**
 ************************************************
 *  \brief Marshall data of ClInt8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClInt8T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt8T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClInt8T
 */
ClRcT clXdrMarshallClInt8T(void* pPyld,
                           ClBufferHandleT msg,
                           ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClInt8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClInt8T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt8T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClInt8T()
 */

ClRcT clXdrUnmarshallClInt8T(ClBufferHandleT msg,
                             void* pPyld );
/**
 ************************************************
 *  \brief Marshall array of ClInt8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClInt8T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt8T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt8T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClInt8T()
 */

ClRcT clXdrMarshallArrayClInt8T(void* pPyld,
                                ClUint32T count,
                                ClBufferHandleT msg,
                                ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClInt8T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt8T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClInt8T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt8T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClInt8T()
 */

ClRcT clXdrUnmarshallArrayClInt8T(ClBufferHandleT msg,
                                  void* pPyld,
                                  ClUint32T count);
/**
 ************************************************
 *  \brief Marshall pointer of ClInt8T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClInt8T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt8T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt8T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClInt8T()
 */
ClRcT clXdrMarshallPtrClInt8T(void* pPyld,
                                  ClUint32T count,
                                  ClBufferHandleT msg,
                                  ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClInt8T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is of void** type and 
 *  the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of 
 *  type ClInt8T* to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt8T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClInt8T()
 */
ClRcT clXdrUnmarshallPtrClInt8T(ClBufferHandleT msg,
                                void** pPyld,
                                ClUint32T multiplicity);

/**
 ************************************************
 *  \brief Marshall data of ClInt16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClInt16T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt16T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClInt16T
 */
ClRcT clXdrMarshallClInt16T(void* pPyld,
                            ClBufferHandleT msg,
                            ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClInt16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClInt16T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt16T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClInt16T()
 */
ClRcT clXdrUnmarshallClInt16T(ClBufferHandleT msg,
                              void* pPyld );
/**
 ************************************************
 *  \brief Marshall array of ClInt16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClInt16T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt16T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt16T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClInt16T()
 */

ClRcT clXdrMarshallArrayClInt16T(void* pPyld,
                                 ClUint32T count,
                                 ClBufferHandleT msg,
                                 ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClInt16T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt16T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClInt16T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt16T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClInt16T()
 */

ClRcT clXdrUnmarshallArrayClInt16T(ClBufferHandleT msg,
                                   void* pPyld,
                                   ClUint32T count);

/**
 ************************************************
 *  \brief Marshall pointer of ClInt16T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClInt16T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt16T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt16T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClInt16T()
 */
ClRcT clXdrMarshallPtrClInt16T(void* pPyld,
                                   ClUint32T count,
                                   ClBufferHandleT msg,
                                   ClUint32T isDelete);
/**
 ************************************************
 *  \brief Unmarshall data of ClInt16T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is of void** type 
 *  and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type 
 *  ClInt16T* to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt16T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClInt16T()
 */
ClRcT clXdrUnmarshallPtrClInt16T(ClBufferHandleT msg,
                                 void** pPyld,
                                 ClUint32T multiplicity);

/**
 ************************************************
 *  \brief Marshall data of ClUint32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClUint32T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint32T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClUint32T()
 */
ClRcT clXdrMarshallClUint32T(void* pPyld,
                             ClBufferHandleT msg,
                             ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClUint32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClUint32T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint32T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClUint32T()
 */
ClRcT clXdrUnmarshallClUint32T(ClBufferHandleT msg,
                               void* pPyld);

/**
 ************************************************
 *  \brief Marshall array of ClUint32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClUint32T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint32T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint32T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClUint32T()
 */
#define clXdrMarshallArrayClUint32T(pPyld, count, msg, isDelete) \
clXdrMarshallArray((pPyld), sizeof(ClUint32T), \
                   (count), clXdrMarshallClUint32T, \
                   (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClUint32T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint32T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClUint32T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint32T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClUint32T()
 */

#define clXdrUnmarshallArrayClUint32T(msg,pPyld, count) \
clXdrUnmarshallArray((msg), \
                     (pPyld), sizeof(ClUint32T), \
                     (count), clXdrUnmarshallClUint32T)

/**
 ************************************************
 *  \brief Marshall pointer of ClUint32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClUint32T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint32T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint32T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClUint32T()
 */
#define clXdrMarshallPtrClUint32T(pPyld, count, msg, isDelete) \
clXdrMarshallPtr((pPyld), sizeof(ClUint32T), \
                 (count), clXdrMarshallClUint32T, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClUint32T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is of ClUint32T pointer type
 *  and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClUint32T
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint32T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClUint32T()
 */
#define clXdrUnmarshallPtrClUint32T(msg, pPyld, multiplicity) \
clXdrUnmarshallPtr((msg), \
                   (pPyld), sizeof(ClUint32T), \
                    multiplicity, clXdrUnmarshallClUint32T)

/**
 ************************************************
 *  \brief Marshall data of ClInt32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClInt32T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt32T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClInt32T
 */
ClRcT clXdrMarshallClInt32T(void* pPyld,
                            ClBufferHandleT msg,
                            ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClInt32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClInt32T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt32T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClInt32T()
 */
ClRcT clXdrUnmarshallClInt32T( ClBufferHandleT msg,
                               void* pPyld);

/**
 ************************************************
 *  \brief Marshall array of ClInt32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClInt32T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt32T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt32T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClInt32T()
 */
#define clXdrMarshallArrayClInt32T(pPyld, count, msg, isDelete) \
clXdrMarshallArray((pPyld), sizeof(ClInt32T), \
                   (count), clXdrMarshallClInt32T, \
                   (msg),(isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClInt32T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt32T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClInt32T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt32T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClInt32T()
 */

#define clXdrUnmarshallArrayClInt32T(msg, pPyld, count) \
clXdrUnmarshallArray((msg), \
                     (pPyld), sizeof(ClInt32T), \
                     (count), clXdrUnmarshallClInt32T)
/**
 ************************************************
 *  \brief Marshall pointer of ClInt32T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClInt32T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt32T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt32T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClInt32T()
 */
#define clXdrMarshallPtrClInt32T(pPyld, count, msg, isDelete) \
clXdrMarshallPtr((pPyld), sizeof(ClInt32T), \
                 (count), clXdrMarshallClInt32T, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClInt32T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt32T pointer type and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClInt32T
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt32T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClInt32T()
 */
#define clXdrUnmarshallPtrClInt32T(msg, pPyld, multiplicity) \
clXdrUnmarshallPtr((msg), \
                   (pPyld), sizeof(ClInt32T), \
                   multiplicity, clXdrUnmarshallClInt32T)

/**
 ************************************************
 *  \brief Marshall data of ClUint64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClUint64T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint64T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClUint64T()
 */
ClRcT clXdrMarshallClUint64T(void* pPyld,
                             ClBufferHandleT msg,
                             ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClUint64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClUint64T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint64T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClUint64T()
 */
ClRcT clXdrUnmarshallClUint64T(ClBufferHandleT msg,
                               void* pPyld);

/**
 ************************************************
 *  \brief Marshall array of ClUint64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClUint64T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClUint64T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint64T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClUint64T()
 */
#define clXdrMarshallArrayClUint64T(pPyld, count, msg, isDelete) \
clXdrMarshallArray((pPyld), sizeof(ClUint64T), \
                   (count), clXdrMarshallClUint64T, \
                   (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClUint64T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClUint64T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClUint64T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint64T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClUint64T()
 */

#define clXdrUnmarshallArrayClUint64T(msg,pPyld, count) \
clXdrUnmarshallArray((msg), \
                     (pPyld), sizeof(ClUint64T), \
                     (count), clXdrUnmarshallClUint64T)

/**
 ************************************************
 *  \brief Marshall pointer of ClUint64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClUint64T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param count (in)    This is the number of elements of type ClUint64T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClUint64T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClUint64T()
 */
#define clXdrMarshallPtrClUint64T(pPyld, count, msg, isDelete) \
clXdrMarshallPtr((pPyld), sizeof(ClUint64T), \
                 (count), clXdrMarshallClUint64T, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClUint64T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is of ClUint64T pointer type 
 *  and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClUint64T
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClUint64T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClUint64T()
 */
#define clXdrUnmarshallPtrClUint64T(msg, pPyld, multiplicity) \
clXdrUnmarshallPtr((msg), \
                   (pPyld), sizeof(ClUint64T), \
                    multiplicity,clXdrUnmarshallClUint64T)

/**
 ************************************************
 *  \brief Marshall data of ClInt64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClInt64T, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt64T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClInt64T
 */
ClRcT clXdrMarshallClInt64T(void* pPyld,
                            ClBufferHandleT msg,
                            ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClInt64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 * 
 *  \param pPyld (out) This parameter is the reference to the payload data 
 *  of type ClInt64T and the unmarshalled data is filled into this.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt64T type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClInt64T()
 */
ClRcT clXdrUnmarshallClInt64T( ClBufferHandleT msg,
                               void* pPyld);
/**
 ************************************************
 *  \brief Marshall array of ClInt64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the array of type ClInt64T, 
 *  which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt64T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt64T array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClInt64T()
 */
#define clXdrMarshallArrayClInt64T(pPyld, count, msg, isDelete) \
clXdrMarshallArray((pPyld), sizeof(ClInt64T), \
                   (count), clXdrMarshallClInt64T, \
                   (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClInt64T array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt64T array type and the unmarshalled data is filled into this.
 *
 *  \param count (in) This is the number of elements of type ClInt64T in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt64T array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClInt64T()
 */

#define clXdrUnmarshallArrayClInt64T(msg, pPyld, count) \
clXdrUnmarshallArray((msg), \
                     (pPyld), sizeof(ClInt64T), \
                     (count), clXdrUnmarshallClInt64T)

/**
 ************************************************
 *  \brief Marshall pointer of ClInt64T type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the pointer to the payload data 
 *  of type ClInt64T, which is to be marshalled.
 *
 *  \param count (in)    This is the number of elements of type ClInt64T to
 *  be marshalled and pointed by the payload data.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClInt64T type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClInt64T()
 */
#define clXdrMarshallPtrClInt64T(pPyld, count, msg, isDelete) \
clXdrMarshallPtr((pPyld), sizeof(ClInt64T), \
                 (count), clXdrMarshallClInt64T, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClInt64T pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the payload data 
 *  of ClInt64T pointer type and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClInt64T
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClInt64T pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClInt64T()
 */
#define clXdrUnmarshallPtrClInt64T(msg, pPyld, multiplicity) \
clXdrUnmarshallPtr((msg), \
                   (pPyld), sizeof(ClInt64T), \
                   multiplicity, clXdrUnmarshallClInt64T)

ClRcT clXdrMarshallClStringT(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete);

ClRcT clXdrUnmarshallClStringT( ClBufferHandleT msg, void* pPyld);

#define clXdrMarshallArrayClStringT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClStringT), (multiplicity), clXdrMarshallClStringT, (msg), (isDelete))

#define clXdrUnmarshallArrayClStringT(msg,pointer, multiplicity)        \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClStringT), (multiplicity), clXdrUnmarshallClStringT)

#define clXdrMarshallPointerClStringT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(ClStringT), (multiplicity), clXdrMarshallClStringT, (msg), (isDelete))

#define clXdrUnmarshallPointerClStringT(msg,pointer)                    \
clXdrUnmarshallPointer((msg),(pointer), sizeof(ClStringT), clXdrUnmarshallClStringT)

#define clXdrMarshallPtrClStringT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClStringT), (multiplicity), clXdrMarshallClStringT, (msg), (isDelete))

#define clXdrUnmarshallPtrClStringT(msg,pointer,multiplicity)           \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClStringT),multiplicity, clXdrUnmarshallClStringT)
    
/**
 ************************************************
 *  \brief Marshall data of ClNameT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClNameT, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClNameT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClNameT
 */
ClRcT  clXdrMarshallClNameT(void *pPyld, ClBufferHandleT msg, ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClNameT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pPyld (out) This parameter is the pointer of type void* 
 *  and the unmarshalled data is filled into this.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClNameT type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  pPyld i.e. pPyld contains the data.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClNameT()
 */
ClRcT  clXdrUnmarshallClNameT(ClBufferHandleT msg, void *pPyld);

/**
 ************************************************
 *  \brief Marshall array of ClNameT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pointer (in) This parameter is the array of type ClNameT, 
 *  which is to be marshalled.
 *
 *  \param multiplicity (in) This is the number of elements of type ClNameT to
 *  be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClNameT array type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallArrayClNameT()
 */
#define clXdrMarshallArrayClNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClNameT), \
                   (multiplicity), clXdrMarshallClNameT, \
                   (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClNameT array type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pointer (out) This parameter is pointer to the payload data 
 *  of ClNameT array type and the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClNameT in the
 *  array to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClNameT array type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pointer' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallArrayClNameT()
 */
#define clXdrUnmarshallArrayClNameT(msg, pointer, multiplicity) \
clXdrUnmarshallArray((msg), \
                     (pointer), sizeof(ClNameT), \
                     (multiplicity), clXdrUnmarshallClNameT)
/**
 ************************************************
 *  \brief Marshall pointer of ClNameT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pointer (in) This parameter is the pointer to the payload data 
 *  of type ClNameT, which is to be marshalled.
 *
 *  \param multiplicity (in) This is the number of elements of type ClNameT to
 *  be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClNameT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClNameT()
 */

#define clXdrMarshallPtrClNameT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClNameT), \
                 (multiplicity), clXdrMarshallClNameT, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClNameT pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pointer (out) This parameter is of ClNameT pointer type and 
 *  the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClNameT
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClNameT pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pointer' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClNameT()
 */
#define clXdrUnmarshallPtrClNameT(msg, pointer, multiplicity) \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClNameT), \
                    multiplicity, clXdrUnmarshallClNameT)
/**
 ************************************************
 *  \brief Marshall data of ClVersionT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClVersionT, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClVersionT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClVersionT()
 */
ClRcT clXdrMarshallClVersionT(void* pGenVar, ClBufferHandleT msg, ClUint32T isDelete);
/**
 ************************************************
 *  \brief Marshall pointer of ClVersionT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pointer (in) This parameter is the pointer to the payload data 
 *  of type ClVersionT, which is to be marshalled.
 *
 *  \param multiplicity (in) This is the number of elements of type ClVersionT to
 *  be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClVersionT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClVersionT()
 */

#define clXdrMarshallPtrClVersionT(pointer, multiplicity, msg, isDelete)    \
clXdrMarshallPtr((pointer), sizeof(ClVersionT), \
                 (multiplicity), clXdrMarshallClVersionT, \
                 (msg), (isDelete))

/**
 ************************************************
 *  \brief Unmarshall data of ClVersionT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pGenVar (out) This parameter is of type void*
 *  the unmarshalled data is filled into this.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClVersionT type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pGenVar' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClVersionT()
 */
ClRcT clXdrUnmarshallClVersionT(ClBufferHandleT msg , void* pGenVar);

/**
 ************************************************
 *  \brief Unmarshall data of ClVersionT pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pointer (out) This parameter is of ClVersionT pointer type and
 *  the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClVersionT
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClVersionT pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pointer' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClVersionT()
 */
#define clXdrUnmarshallPtrClVersionT(msg, pointer, multiplicity)  \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClVersionT), \
                    multiplicity, clXdrUnmarshallClVersionT)

/**
 ************************************************
 *  \brief Marshall data of ClHandleT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClHandleT, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClHandleT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClHandleT
 */
ClRcT clXdrMarshallClHandleT(void *pPyld, ClBufferHandleT msg, ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClHandleT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pBuf (out) This parameter is the pointer of type void* 
 *  and the unmarshalled data is filled into this.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClHandleT type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pBuf' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClHandleT()
 */
ClRcT clXdrUnmarshallClHandleT(ClBufferHandleT msg, void *pBuf);

/**
 ************************************************
 *  \brief Marshall pointer of ClHandleT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pointer (in) This parameter is the pointer to the payload data 
 *  of type ClHandleT, which is to be marshalled.
 *
 *  \param multiplicity (in) This is the number of elements of type ClHandleT to
 *  be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClHandleT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClHandleT()
 */
#define clXdrMarshallPtrClHandleT(pointer, multiplicity, msg, isDelete)    \
clXdrMarshallPtr((pointer), sizeof(ClHandleT), \
                 (multiplicity), clXdrMarshallClHandleT, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClHandleT pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pointer (out) This parameter is the pointer of ClHandleT type and 
 *  the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClHandleT
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClHandleT pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pointer' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClHandleT()
 */
#define clXdrUnmarshallPtrClHandleT(msg, pointer, multiplicity)  \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClHandleT), \
                   multiplicity, clXdrUnmarshallClHandleT)


/**
 ************************************************
 *  \brief Marshall data of ClWordT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pPyld (in)    This parameter is the reference to the payload data 
 *  of type ClWordT, which is to be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as 
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data,
 *  but in this function it should always be specified as 0 - Don't delete.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClWordT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network 
 *  using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallClWordT
 */
ClRcT clXdrMarshallClWordT(void *pPyld, ClBufferHandleT msg, ClUint32T isDelete);

/**
 ************************************************
 *  \brief Unmarshall data of ClWordT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pBuf (out) This parameter is the pointer of type void* 
 *  and the unmarshalled data is filled into this.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClWordT type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pBuf' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallClWordT()
 */
ClRcT clXdrUnmarshallClWordT(ClBufferHandleT msg, void *pBuf);


/**
 ************************************************
 *  \brief Marshall pointer of ClWordT type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param pointer (in) This parameter is the pointer to the payload data 
 *  of type ClWordT, which is to be marshalled.
 *
 *  \param multiplicity (in) This is the number of elements of type ClWordT to
 *  be marshalled.
 *
 *  \param msg   (inout) This is the handle to the buffer which is created 
 *  by the user. On success this message handle is updated and it
 *  contains this marshalled data along with any other data which was 
 *  marshalled using this handle earlier. This handle should be used as
 *  in-message-handle for making the rmd call.
 *
 *  \param isDelete (in) This parameter is used in deleting the payload data, 
 *  and it takes one of the following two values:
 *  0 - Don't Delete or 1 - Delete the payload data after marshalling.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to marshall the payload data of ClWordT type. The
 *  successful execution of this function adds the marshalled data into  
 *  message handle which should be used to transfer the data over network using rmd call.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrUnmarshallPtrClWordT()
 */
#define clXdrMarshallPtrClWordT(pointer, multiplicity, msg, isDelete)    \
clXdrMarshallPtr((pointer), sizeof(ClWordT), \
                 (multiplicity), clXdrMarshallClWordT, \
                 (msg), (isDelete))
/**
 ************************************************
 *  \brief Unmarshall data of ClWordT pointer type. 
 *
 *  \par Header File:
 *  clXdrApi.h
 *
 *  \param msg   (in) This handle is actually received in RMD receive 
 *  function as in-message-handle.
 *  On success this message handle is updated and it contains the 
 *  remaining data which were marshalled using this handle 
 *  and still not unmarshalled.  
 *
 *  \param pointer (out) This parameter is the pointer of ClWordT type and 
 *  the unmarshalled data is filled into this.
 *
 *  \param multiplicity (in) This is the number of elements of type ClWordT
 *  to be unmarshalled.
 * 
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing msg handle as 0(invalid).
 *
 *  \par Description:
 *  This function is used to unmarshall the payload data of ClWordT pointer type. The
 *  successful execution of this function unmarshalles the marshalled data into  
 *  'pointer' parameter.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clXdrMarshallPtrClWordT()
 */
#define clXdrUnmarshallPtrClWordT(msg, pointer, multiplicity)  \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClWordT), \
                   multiplicity, clXdrUnmarshallClWordT)

ClRcT clXdrMarshallClMD5T(void* pGenVar, ClBufferHandleT msg, ClUint32T isDelete);

ClRcT clXdrUnmarshallClMD5T(ClBufferHandleT msg , void* pGenVar);

#define clXdrMarshallPtrClMD5T(pointer, multiplicity, msg, isDelete)    \
clXdrMarshallPtr((pointer), sizeof(ClMD5T), \
                 (multiplicity), clXdrMarshallClMD5T, \
                 (msg), (isDelete))

#define clXdrUnmarshallPtrClMD5T(msg, pointer, multiplicity)  \
clXdrUnmarshallPtr((msg), \
                   (pointer), sizeof(ClMD5T), \
                   multiplicity, clXdrUnmarshallClMD5T)

#define clXdrMarshallArrayClMD5T(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClMD5T), (multiplicity), clXdrMarshallClMD5T, (msg), (isDelete))

#define clXdrUnmarshallArrayClMD5T(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClMD5T), (multiplicity), clXdrUnmarshallClMD5T)

ClRcT  clXdrMarshallClDifferenceVectorT(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallClDifferenceVectorT(ClBufferHandleT, void *);

#define clXdrMarshallArrayClDifferenceVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClDifferenceVectorT), (multiplicity), clXdrMarshallClDifferenceVectorT, (msg), (isDelete))

#define clXdrUnmarshallArrayClDifferenceVectorT(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClDifferenceVectorT), (multiplicity), clXdrUnmarshallClDifferenceVectorT)

#define clXdrMarshallPointerClDifferenceVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(ClDifferenceVectorT), (multiplicity), clXdrMarshallClDifferenceVectorT, (msg), (isDelete))

#define clXdrUnmarshallPointerClDifferenceVectorT(msg,pointer) \
clXdrUnmarshallPointer((msg),(pointer), sizeof(ClDifferenceVectorT), clXdrUnmarshallClDifferenceVectorT)

#define clXdrMarshallPtrClDifferenceVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClDifferenceVectorT), (multiplicity), clXdrMarshallClDifferenceVectorT, (msg), (isDelete))

#define clXdrUnmarshallPtrClDifferenceVectorT(msg,pointer,multiplicity) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClDifferenceVectorT),multiplicity, clXdrUnmarshallClDifferenceVectorT)

ClRcT  clXdrMarshallClDataVectorT(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallClDataVectorT(ClBufferHandleT, void *);

#define clXdrMarshallArrayClDataVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClDataVectorT), (multiplicity), clXdrMarshallClDataVectorT, (msg), (isDelete))

#define clXdrUnmarshallArrayClDataVectorT(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClDataVectorT), (multiplicity), clXdrUnmarshallClDataVectorT)

#define clXdrMarshallPointerClDataVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(ClDataVectorT), (multiplicity), clXdrMarshallClDataVectorT, (msg), (isDelete))

#define clXdrUnmarshallPointerClDataVectorT(msg,pointer) \
clXdrUnmarshallPointer((msg),(pointer), sizeof(ClDataVectorT), clXdrUnmarshallClDataVectorT)

#define clXdrMarshallPtrClDataVectorT(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClDataVectorT), (multiplicity), clXdrMarshallClDataVectorT, (msg), (isDelete))

#define clXdrUnmarshallPtrClDataVectorT(msg,pointer,multiplicity) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClDataVectorT),multiplicity, clXdrUnmarshallClDataVectorT)

ClRcT clXdrError(void* pPyld, ...);

ClRcT clIdlFree( void *pData);

#ifdef __cplusplus
}
#endif

#endif /*_CL_XDR_API_H_*/

/** \} */
