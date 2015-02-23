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
 * ModuleName  : buffer
 * File        : clBufferApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the buffer management
 * library.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Buffer Management related APIs
 *  \ingroup buffer_apis
 */

/**
 *  \addtogroup buffer_apis
 *  \{
 */


#ifndef _CL_BUFFER_API_H_
#define _CL_BUFFER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon6.h>
//#include <clRuleApi.h>
#include <clMemStats6.h>
#include <clPoolIpi6.h>

/*****************************************************************************/

typedef enum {
/**
 * This is to set the read or write offset pointer to a position mentioned
 * from the BEGINING of the Buffer message.
 */
    CL_BUFFER_SEEK_SET = 0,
/**
 * This is to set the read or write offset pointer to a position mentioned
 * from the CURRENT position of the pointer of the Buffer message.
 */
    CL_BUFFER_SEEK_CUR,
/**
 * This is to set the read or write offset pointer to a position mentioned
 * from the END of the Buffer message.
 */
    CL_BUFFER_SEEK_END,
 /* Add any new seek types before this.*/
    CL_BUFFER_SEEK_MAX
} ClBufferSeekTypeT;


/**
 * The type of the handle for the buffer messages.
 */
typedef ClPtrT ClBufferHandleT;

struct iovec;

/**
 * The config mode of the Buffer. When configured in native mode,
 * malloc is used for fetching the memory for the configured buffer pools
 * instead of the clovis pool implementation.
 */
typedef enum ClBufferMode
{
    /**
     * In this mode the buffer library uses the malloc and free for allocating and
     * deallocating memory for buffers.
     */
    CL_BUFFER_NATIVE_MODE,
  
    /**
     * This mode makes buffer library uses Clovis implementation of Pool for
     * allocating and deallocating memory for buffers.
     */
    CL_BUFFER_PREALLOCATED_MODE,


    CL_BUFFER_MAX_MODE,
}ClBufferModeT;

/**
 * The type of the buffer configuration info.
 */
typedef struct {
    /**
     * The Number pools used for buffer library.
     */
    ClUint32T numPools;
    /**
     * Pool configuration of all the pools used by buffer library.
     */
    ClPoolConfigT *pPoolConfig;
    /**
     * Mode of usage of pools
     */
    ClBoolT lazy;
    /**
     * Mode configured for Buffer fof fetching the memory.
     * i.e \e CL_BUFFER_NATIVE_MODE, or \e CL_BUFFER_PREALLOCATED_MODE.
     */
    ClBufferModeT mode;

} ClBufferPoolConfigT;


/*****************************************************************************/



/************************* BUFF MGMT APIs ************************************/


/**
 ************************************
 *  \brief Initializes the Buffer Management library.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \par Parameters:
 *  \param pConfig This should contain the pool configuration to be used by
 *  the Buffer library. If NULL is passed then the default pool configuration
 *  will be used.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_INITIALIZED If the Buffer Management library is already initialized.
 *
 *  \par Description:
 *  This API is used to initialize the Buffer Management library. It must be
 *  called before calling any other Buffer library APIs. Since Buffer
 *  library uses OpenClovis OSAL APIs, the OSAL library should be initialized
 *  before intializing the Buffer library.
 *
 *  \par Library Files:
 *  ClBuffer
 *
 *  \sa clBufferFinalize()
 *
 */


ClRcT clBufferInitialize(const ClBufferPoolConfigT *pConfig);
/*****************************************************************************/


/**
 ************************************
 *  \brief Cleans up the Buffer Management library.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \par Parameters:
 *   None
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is should be called when an application decides of not using the
 *  buffer management library any more. The application must have invoked 
 *  clBufferInitialize() before it invokes this function. It is called typically
 *  when an application calling this function is shutting down and wants to
 *  releave all the resources used by it.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferInitialize()
 *
 */

ClRcT clBufferFinalize (void);
/*****************************************************************************/


/**
 ************************************
 *  \brief Creates a new message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param pMessageHandle (out) A pointer to the handle of type \e ClBufferHandleT
 *  designating the buffer for a message created.
 *
 *  \retval CL_OK The API successfully created a buffer for a message.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to create a buffer for a message. It allocates a buffer
 *  of 2 kilobytes size, even if a write operation on the buffer is to be
 *  performed at a later stage in the application. The write operation
 *  on the buffer handle provided by this API, the buffer management library
 *  will use this pre-allocated buffer.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreateAndAllocate(), clBufferDelete(), clBufferClear()
 */
ClRcT clBufferCreate (ClBufferHandleT *pMessageHandle);

/*****************************************************************************/

/**
 ************************************
 *  \brief Creates buffers for a requested size of a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param size Initial size of a message buffer in bytes.
 *  \param pMessageHandle A pointer to the buffer message of type
 *  \e ClBufferHandleT designating the message buffer created.
 *
 *  \retval CL_OK The buffers for a message are successfully created.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to create buffers to hold a message of size mentioned by
 *  \e size. It allocates the buffers even if the write operation is to be
 *  performed at a later stage in application. As a write is performed onto
 *  this buffer handle returned by this API, the buffer management library
 *  will use these pre-allocated buffers.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferDelete(), clBufferClear(), clBufferCreate()
 *
 */
ClRcT clBufferCreateAndAllocate (ClUint32T size, ClBufferHandleT *pMessageHandle);

/*****************************************************************************/


/**
 ************************************
 *  \brief Deletes the Buffers.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param pMessageHandle Pointer to the message handle, which is to be deleted.
 *
 *  \retval CL_OK The Buffers are deleted successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to delete message buffer(s) designated by the handle
 *  obtained through \e clBufferCreate() API. Successful invocation of this
 *  API will make the handle invalid.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferClear(), clBufferCreateAndAllocate(), clBufferCreate()
 *
 */
ClRcT clBufferDelete (ClBufferHandleT *pMessageHandle);

/*****************************************************************************/

/**
 ************************************
 *  \brief Deletes the content of the buffer message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param pMessageHandle Pointer to the message handle returned by \e clBufferCreate() API.
 *
 *  \retval CL_OK The buffer message is cleared successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to delete message buffer(s) designated by the handle
 *  obtained through \e clBufferCreate() API.
 *  The handle is still available for re-use.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferDelete(), clBufferCreateAndAllocate(), clBufferCreate()
 *
 */
ClRcT clBufferClear (ClBufferHandleT messageHandle);

/*****************************************************************************/

/**
 ************************************
 *  \brief Returns the length of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *  \param pMessageLength (out) Pointer to a variable of type \e ClUint32T,
 *  in which the total length of the message is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to retrieve the total length of the message. There is no
 *  impact on the read and write offset of the message even on subsequent
 *  invocation of this API.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferNBytesWrite()
 *
 */
ClRcT clBufferLengthGet (ClBufferHandleT messageHandle, ClUint32T *pMessageLength);

/*****************************************************************************/

/**
 ************************************
 *  \brief Recompute the message length & set internal variable
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to recompute the total message length, to be used if another entity (IOC)
 *  fools with the buffers.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *
 */
ClRcT clBufferLengthCalc (ClBufferHandleT bufferHandle);


/**
 ************************************
 *  \brief Reads the specified number of bytes of data from a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *  \param pByteBuffer Pointer to a stream of memory of type \e ClUint8T*, in which *pNumberOfBytesToRead of the read data is returned. Memory allocation and deallocation for this parameter must be done by the caller.
 *  \param pNumberOfBytesToRead Pointer to variable of type \e ClUint32T, which contains number of bytes of data to be read. And the number of bytes of data that has been successfully read is returned in this parameter.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to read \e *pNumberOfBytesToRead number of bytes of data
 *  from a buffer message pointed by \e messageHandle, starting from the
 *  where the last read has stopped at in that buffer message. If the buffer
 *  doesnt contain \e *pNumberOfBytesToRead number of bytes of data then
 *  \e *pNumberOfBytesToRead parameter will be updated with actually read number of
 *  bytes. The current read offset(or pointer), is automatically updated by
 *  \e *pNumberOfBytesToRead bytes after every read, so that the next read
 *  operation starts from the point where this read has left at. 
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferClear(), clBufferNBytesWrite(), clBufferCreate()
 *
 */
ClRcT clBufferNBytesRead (ClBufferHandleT messageHandle, ClUint8T *pByteBuffer, ClUint32T* pNumberOfBytesToRead);

/*****************************************************************************/


/**
 ************************************
 *  \brief Writes the specified number bytes of data from a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API
 *  into which n bytes of data is to be written.
 *  \param pByteBuffer Pointer to the byte buffer from which \e numberOfBytesToWrite bytes are to be written into the buffer message.
 *  \param numberOfBytesToWrite Number of bytes to be written.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to write n bytes of data into the message referred by the
 *  message handle, starting from the current write offset. While writing data
 *  to a buffer message if the buffer gets full then a new buffer will be
 *  allocated automatically and then the write continues into that buffer. The
 *  write offset is automatically updated by \e numberOfBytesToWrite bytes
 *  after every write, so that the next write can continue from the point
 *  where this write ended at.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferClear(), clBufferNBytesRead(), clBufferCreate()
 *
 */

ClRcT clBufferNBytesWrite (ClBufferHandleT messageHandle, ClUint8T *pByteBuffer, ClUint32T numberOfBytesToWrite);

/*****************************************************************************/


/**
 ************************************
 *  \brief Computes a 16-bit checksum on a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API for which the checksum is to be computed.
 *  \param startOffset Offset from the beginning of the message, from which the data for computing the checksum begins.
 *  \param length Number of bytes of data for which the checksum is to be computed.
 *  \param pChecksum (out) Pointer to variable of type \e ClUint32T in which the computed checksum is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to compute a 16-bit checksum on a message from
 *  \e startOffset for \e length number of bytes in the message. This checksum
 *  can be used to check the validity of the buffer message after passing it 
 *  through a network.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferChecksum32Compute()
 *
 */

ClRcT clBufferChecksum16Compute(ClBufferHandleT messageHandle,ClUint32T startOffset,ClUint32T length, ClUint16T* pChecksum);
/****************************************************************************/


/**
 ************************************
 *  \brief Computes a 32-bit checksum on a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API for which the checksum is to be computed.
 *  \param startOffset Offset from the beginning of the message, from which the data for computing the checksum begins.
 *  \param length Number of bytes of data for which the checksum is to be computed.
 *  \param pChecksum (out) Pointer to variable of type \e ClUint32T in which the computed checksum is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to compute a 32-bit checksum on a message from
 *  \e startOffset for \e length number of bytes in the message. This checksum
 *  can be used to check the validity of the buffer message after passing it 
 *  through a network.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferChecksum16Compute()
 *
 */

ClRcT
clBufferChecksum32Compute(ClBufferHandleT messageHandle, ClUint32T startOffset, ClUint32T length, ClUint32T* pChecksum);

/*****************************************************************************/


/**
 ************************************
 *  \brief Prepends specified number of bytes at the begining of message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *  \param pByteBuffer Pointer to a variable of type \e ClUint8T, in which \e numberOfBytesToRead bytes of data is returned.
 *  \param numberOfBytesToWrite Number of bytes to be written.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to prepend \e numberOfBytesToWrite number of bytes at
 *  the beginning of the buffer message. The write offset of the buffer
 *  message will be updated and is set to \e numberOfBytesToWrite, i.e. the
 *  next write-operation performed will begin the write at
 *  \e numberOfBytesToWrite offset from the starting of the message.
 *  Note that the current write offset is lost. Hence, it is advisable to
 *  preserve the write offset before invoking this API.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferNBytesWrite(), clBufferConcatenate(), clBufferDelete()
 *
 */

ClRcT
clBufferDataPrepend (ClBufferHandleT messageHandle, ClUint8T *pByteBuffer, ClUint32T numberOfBytesToWrite);

/*****************************************************************************/



/**
 ************************************
 *  \brief Concatenates source message to destination message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param destination This is buffer message handle to which the \e pSource buffer message will be concatinated. The handle of the concatenated message will be the same \e destination buffer message handle .
 *  \param pSource (in/out) Pointer to buffer message handle of the message, which is to be concatenated with \e destination buffer message.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to concatenate the \e pSource buffer message to the end
 *  of \e destination message. The \e pSource message handle will become
 *  not-usable after this concatination operation and the \e pSource message
 *  will be deleted. The write offset of the resultant \e destination
 *  message will be set to end of the message and the read offset of the to
 *  start of the message.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferDataAppend(), 
 *      clBufferDelete(), clBufferDataPrepend()
 *
 */

ClRcT
clBufferConcatenate (ClBufferHandleT destination, ClBufferHandleT *pSource);

/*****************************************************************************/


/**
 ************************************
 *  \brief Returns current read offset of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *  \param pReadOffset Pointer to a variable of type \e ClUint32T, in which the current read offset is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to retrieve the current read offset of the buffer message.
 *  This offset is updated on invoking the clBufferNBytesRead() API or it can
 *  be set using clBufferReadOffsetSet().
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferWriteOffsetGet()
 *  clBufferDelete(), clBufferReadOffsetSet()
 *  clBufferWriteOffsetSet(), clBufferTrailerTrim()
 *  clBufferHeaderTrim(), clBufferNBytesRead()
 *
 */

ClRcT
clBufferReadOffsetGet (ClBufferHandleT messageHandle, ClUint32T *pReadOffset);

/*****************************************************************************/


/**
 ************************************
 *  \brief Returns current write offset of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate API
 *  \param pWriteOffset Pointer to a variable of type \e ClUint32T, in which the current write offset is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to retrieve the current write offset of the message. This
 *  offset is updated on invoking the clBufferNBytesWrite() API or
 *  directly through clBufferWriteOffsetSet().
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferReadOffsetGet(),
 *  clBufferDelete(), clBufferReadOffsetSet(),
 *  clBufferWriteOffsetSet(), clBufferTrailerTrim(),
 *  clBufferHeaderTrim()
 *
 */

ClRcT
clBufferWriteOffsetGet (ClBufferHandleT messageHandle, ClUint32T *pWriteOffset);

/*****************************************************************************/


/**
 ************************************
 *  \brief Sets current read offset of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by clBufferCreate() API
 *  \param seekType This parameter accepts the following values:
 *  \arg \c CL_BUFFER_SEEK_SET: The read offset should be set to a position from the beginning of the message.
 *  \arg \c CL_BUFFER_SEEK_CUR: The read offset should be set to a position from its current location.
 *  \arg \c CL_BUFFER_SEEK_END: The read offset should be set to a position from the the end of the message.
 *  \param newReadOffset The offset at which the current read offset is to be set.
 *  If \e seekType is set to CL_BUFFER_SEEK_SET then the value of
 *  \e newReadOffset should be 0 or greater, for CL_BUFFER_SEEK_CUR it can be
 *  a positive of negative number and for CL_BUFFER_END it should be 0 or a negative number.
 *
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *
 *  \par Description:
 *  This API is used to set the read offset of the buffer message pointed by 
 *  the \e messageHandle. This is used, when an application wants to read the
 *  data from the buffer message from a specific point onwards. This
 *  offset is also automatically updated on invoking the clBufferNBytesRead().
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferWriteOffsetGet(), clBufferDelete(),
 *  clBufferWriteOffsetSet(), clBufferTrailerTrim(),
 *  clBufferHeaderTrim(), clBufferNBytesRead()
 *
 */

ClRcT
clBufferReadOffsetSet (ClBufferHandleT messageHandle, ClInt32T newReadOffset, ClBufferSeekTypeT seekType);

/*****************************************************************************/


/**
 ************************************
 *  \brief Sets current write offset of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by clBufferCreate() API.
 *  \param seekType This parameter can accept the following values:
 *  \arg \c CL_BUFFER_SEEK_SET: The write offset should be set to a position from the beginning of the message.
 *  \arg \c CL_BUFFER_SEEK_CUR: The write offset should be set to a position from its current location.
 *  \arg \c CL_BUFFER_SEEK_END: The write offset should be set to a position from the the end of the message.
 *  \param newWriteOffset The offset at which the current write offset is to be set.
 *  If \e seekType is set to CL_BUFFER_SEEK_SET then the value of
 *  \e newWriteOffset should be 0 or greater, for CL_BUFFER_SEEK_CUR it can be
 *  a positive of negative number and for CL_BUFFER_END it should be 0 or a negative number.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER If an invalid parameter is passed.
 *
 *  \par Description:
 *  This API is used to set the current write offset of the buffer message
 *  pointed by \e messageHandle. The application wanting to write some data
 *  to the buffer message starting at a perticular point in the buffer
 *  message should set the buffer message's current write offset to that
 *  point and then do the write operation. This offset is also
 *  automatically updated on invoking the clBufferNBytesWrite().
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferWriteOffsetGet(), clBufferDelete(),
 *  clBufferReadOffsetSet(), clBufferTrailerTrim(),
 *  clBufferHeaderTrim(), clBufferNBytesRead()
 *
 */

ClRcT
clBufferWriteOffsetSet (ClBufferHandleT messageHandle, ClInt32T newWriteOffset, ClBufferSeekTypeT seekType);

/*****************************************************************************/

/**
 *******************************************************************************
 *  \brief Trims the start of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API.
 *  \param numberOfBytes Number of bytes to delete from the start of the message.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *
 *  \par Description:
 *  This API is used to delete a specific number of bytes from the start of the
 *  message pointed by \e messageHandle. If the read/write offset is in the
 *  region being deleted, the read/write offset will be set to 0, i.e, the
 *  beginning of the message, after performing the trim operation.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferHeaderTrim(), clBufferDelete()
 *
 */

ClRcT
clBufferHeaderTrim (ClBufferHandleT messageHandle, ClUint32T numberOfBytes);

/*****************************************************************************/


/**
 *******************************************************************************
 *  \brief Trims the tail of the message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by clBufferCreate() API.
 *  \param numberOfBytes Number of bytes to delete from the tail of the message.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *
 *  \par Description:
 *  This API is used to delete a specific number of bytes from tail of the
 *  message pointed by \e messageHandle. If the read/write offset is in the
 *  region being deleted, the read/write offset will be set to length of the
 *  buffer message, i.e, the end of the message, after performing the
 *  trim operation.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferHeaderTrim(), clBufferDelete()
 *
 */


ClRcT
clBufferTrailerTrim (ClBufferHandleT messageHandle, ClUint32T numberOfBytes);

/*****************************************************************************/



/**
 *******************************************************************************
 *  \brief Copies specific number of bytes from one message to another.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param sourceMessage Handle to the message returned by \e clBufferCreate() API, from which data is to be copied.
 *  \param sourceMessageOffset Offset with respect to beginning of \e sourceMessage, from where the copying is to begin.
 *  \param destinationMessage Handle to the message returned by \e clBufferCreate() API, into which data is being copied.
 *  \param numberOfBytes Number of bytes to be copied from \e sourceMessage to \e destinationMessage.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to copy the \e numberOfBytes bytes starting at
 *  \e sourceMessageOffset offset in \e sourceMessage. The data will be copied
 *  in \e destinationMessage starting at write offset of the message.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferReadOffsetGet(), clBufferDelete(),
 *  clBufferWriteOffsetGet(), clBufferToBufferCopy(), 
 *  clBufferDuplicate()
 *
 */


ClRcT
clBufferToBufferCopy(ClBufferHandleT sourceMessage, ClUint32T sourceMessageOffset,
                      ClBufferHandleT destinationMessage, ClUint32T numberOfBytes);

/*****************************************************************************/


/**
 *******************************************************************************
 *  \brief Duplicates a message.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to the message returned by \e clBufferCreate() API, which is to be duplicated.
 *  \param pDuplicatedMessage (out) Pointer to variable of type \e ClBufferHandleT, in which handle of the duplicate
 *  message is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to make duplicate copy of \e messageHandle. The newly
 *  created buffer message handle will be returned in \e pDuplicateMessage.
 *  The read and write offsets of the newly created buffer message are set
 *  to the values same as that of the \e messageHandle.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferDelete(),
 *  clBufferToBufferCopy()
 *
 */


ClRcT
clBufferDuplicate (ClBufferHandleT messageHandle, ClBufferHandleT *pDuplicatedMessage);

/*
 * Same as duplicate but just shares the chain with the source and copies only the first chain,
 * so any metadata prepended would be local. However a write to the cloned buffer would result in a COW
 * or copy on write of the cloned destination buffer from the parent.
*/
ClRcT
clBufferClone (ClBufferHandleT source, ClBufferHandleT *pClone);

/*
 * Stich a heap allocated chunk to the buffer chain without duplication.
 */
ClRcT
clBufferAppendHeap (ClBufferHandleT source, ClUint8T *buffer, ClUint32T size);

/*****************************************************************************/

/**
 *******************************************************************************
 *  \brief Flattens message into a single buffer.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param messageHandle Handle to message returned by \e clBufferCreate() API.
 *  \param ppFlattenBuffer (out) The flatten buffer would be returned in this location.
 *  This must be a valid pointer and cannot be NULL.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to copy the message from a buffer message into a single flat buffer stream. The buffer library will allocate the memory for the flat buffer stream and this has to be freed by the caller of this API.
 *
 *  \par Library Files:
 *  ClBuffer
 *
 *  \sa clBufferCreate(), clBufferDelete()
 *
 */

ClRcT
clBufferFlatten(ClBufferHandleT messageHandle,
                   ClUint8T** ppFlattenBuffer);

/*****************************************************************************/


/**
 *******************************************************************************
 *  \brief Copies message from user-space to kernel-space.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \note
 *  This API is available only inside the kernel. It cannot be invoked from the user-space.
 *
 *  \param userMessageHandle Handle to message returned by \e clBufferCreate() API, which is to be copied into kernel
 *  space.
 *  \param pKernelMessageHandle (out) Pointer to variable of type \e ClBufferHandleT, in which the handle to
 *  message in the kernel space is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to copy a message from user-space to kernel-space. The kernel
 *  space message will be created by the Buffer Management Library and the handle to
 *  created message will be returned. Write offset would be set to the end and read offset
 *  would point to the beginning of the kernel space message.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa
 *  clBufferCreate(), clBufferDelete(), clBufferKernelToUserCopy()
 *
 */

ClRcT
clBufferUserToKernelCopy(ClBufferHandleT userMessageHandle,
                      ClBufferHandleT* pKernelMessageHandle);


/*****************************************************************************/



/**
 *******************************************************************************
 *  \brief Copies message from kernel-space to user-space.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \note
 *  This API is available only inside the kernel. It cannot be invoked from the user-space.
 *
 *  \param kernelMessageHandle Handle to kernel-space message returned by \e clBufferCreate()
 *  API, from which data is to be copied into user-space message.
 *
 *  \param userMessageHandle Handle to user-space message returned by \e clBufferCreate()
 *  API,into which data is to be copied from kernel-space message.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to copy a message from kernel-space to user-space. The user-space
 *  message must be created in user space. The contents of the kernel-space
 *  message will be copied into the user-space message.Write offset would be set to the end
 *  and read offset would point to the beginning of the user-space message.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferDelete(), clBufferUserToKernelCopy()
 *
 */

ClRcT
clBufferKernelToUserCopy(ClBufferHandleT kernelMessageHandle,
                    ClBufferHandleT userMessageHandle);
/*****************************************************************************/

/**
 *******************************************************************************
 *  \brief Frees up the unused pools of all sizes.
 *
 *  \par Header File:
 *  clBufferApi.h
 *
 *  \param pShrinkOptions This is pointer to the shrik option parameter of
 *  type ClPoolShrinkOptionsT. The values that can be passed are 
 *  \arg CL_POOL_SHRINK_DEFAULT: This option will free half of total unused pools.
 *  \arg CL_POOL_SHRINK_ONE: This option will free only one pool of all the unused pools.
 *  \arg CL_POOL_SHRINK_ALL: This option will free all the unused pools.
 *  
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If Buffer Management library is not initialized.
 *
 *  \par Description:
 *  This API is used to free the unused pools. The new pools of a size get
 *  allocated when there is shortage of space for creating new buffer messages.
 *  And after using the buffer messages they will be freed using
 *  clBufferDelete(), but the pool remains. So these pools are required to be
 *  freed up at times.
 *
 *  \par Library Files:
 *	 ClBuffer
 *
 *  \sa clBufferCreate(), clBufferDelete()
 *
 */
ClRcT
clBufferShrink(ClPoolShrinkOptionsT *pShrinkOptions);
/*****************************************************************************/


/** \brief Dumps the buffer for debugging, etc
 */
    ClRcT clDbgBufferPrint(ClBufferHandleT buffer);
    
    
ClRcT clBufferStatsGet(ClMemStatsT *pBufferStats);

/*****************************************************************************/

ClRcT clBufferPoolStatsGet(ClUint32T numPools,ClUint32T *pPoolSize,ClPoolStatsT *pBufferPoolStats);

/*****************************************************************************/

ClRcT clBufferVectorize(ClBufferHandleT buffer,struct iovec **ppIOVector,ClInt32T *pNumVectors);

/*****************************************************************************/
    /*The below definitions are deprecated and should no longer be used*/
    
typedef ClBufferHandleT ClBufferMessageHandleT CL_DEPRECATED;

ClRcT 
clBufferMessageCreate(ClBufferHandleT *pMessageHandle) CL_DEPRECATED;

ClRcT
clBufferMessageCreateAndAllocate(ClUint32T size, ClBufferHandleT *pMessageHandle) CL_DEPRECATED;


ClRcT
clBufferMessageDelete(ClBufferHandleT *pMessageHandle) CL_DEPRECATED;

ClRcT
clBufferMessageClear(ClBufferHandleT messageHandle) CL_DEPRECATED;

ClRcT
clBufferMessageLengthGet(ClBufferHandleT messageHandle, ClUint32T *pMessageLength) CL_DEPRECATED;

ClRcT
clBufferMessageNBytesRead(ClBufferHandleT messageHandle, 
                          ClUint8T *pByteBuffer, 
                          ClUint32T* pNumberOfBytesToRead) CL_DEPRECATED;

ClRcT
clBufferMessageNBytesWrite(ClBufferHandleT messageHandle, 
                           ClUint8T *pByteBuffer, 
                           ClUint32T numberOfBytesToWrite) CL_DEPRECATED;

ClRcT
clBufferMessageDataPrepend(ClBufferHandleT messageHandle, 
                           ClUint8T *pByteBuffer, 
                           ClUint32T numberOfBytesToWrite) CL_DEPRECATED;

ClRcT
clBufferMessageConcatenate(ClBufferHandleT destination, ClBufferHandleT *pSource) CL_DEPRECATED;

ClRcT
clBufferMessageReadOffsetGet(ClBufferHandleT messageHandle,
                             ClUint32T *pReadOffset) CL_DEPRECATED;

ClRcT
clBufferMessageWriteOffsetGet(ClBufferHandleT messageHandle,
                              ClUint32T *pWriteOffset) CL_DEPRECATED;

ClRcT
clBufferMessageReadOffsetSet(ClBufferHandleT messageHandle,
                             ClInt32T newReadOffset,
                             ClBufferSeekTypeT seekType) CL_DEPRECATED;

ClRcT
clBufferMessageWriteOffsetSet(ClBufferHandleT messageHandle,
                              ClInt32T newWriteOffset,
                              ClBufferSeekTypeT seekType) CL_DEPRECATED;

ClRcT
clBufferMessageHeaderTrim(ClBufferHandleT messageHandle,
                          ClUint32T numberOfBytes) CL_DEPRECATED;

ClRcT
clBufferMessageTrailerTrim(ClBufferHandleT messageHandle,
                           ClUint32T numberOfBytes) CL_DEPRECATED;

ClRcT
clBufferMessageToMessageCopy(ClBufferHandleT source,
                             ClUint32T sourceOffset,
                             ClBufferHandleT destination,
                             ClUint32T numberOfBytes) CL_DEPRECATED;

ClRcT
clBufferMessageDuplicate(ClBufferHandleT source,
                         ClBufferHandleT *pDuplicate) CL_DEPRECATED;
#ifdef __cplusplus
}
#endif

#endif /* _CL_BUFFER_API_H_ */


/** \} */


