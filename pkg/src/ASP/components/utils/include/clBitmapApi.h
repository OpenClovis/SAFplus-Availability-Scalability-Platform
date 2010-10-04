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
 * ModuleName  : utils
 * File        : clBitmapApi.h
 *******************************************************************************/

/******************************************************************************
 * Description :
 *
 * OpenClovis Bitmap Library API file.
 *
 ****************************************************************************/

/**
 * \file 
 * \brief Header file of Bitmap related APIs
 * \ingroup bitmap_apis
 */


/**
 *  \addtogroup bitmap_apis
 *  \{
 */

#ifndef _CL_BITMAP_API_H_
#define _CL_BITMAP_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmap.h>


/*****************************************************************************
 *  Bitmap Data types 
 *****************************************************************************/
/**
  * Callback function type, which is executed during the walk 
  * on a bitmap
  */
typedef ClRcT (*ClBitmapWalkCbT) 
               (ClBitmapHandleT hBitmap, ClUint32T bitNum, 
                void *pCookie) ;

/*****************************************************************************
 *  Bitmap functions
 *****************************************************************************/

/**
 ************************************************
 *  \brief Create a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param phBitmap (out) This parameter is pointer to the bitmap handle. This
 *  handle is used in subsequent calls to query bitmap information and to perform
 *  operations on the bitmap.
 *
 *  \param bitNum (in) The length (number of bits) of bitmap. Since bit number
 *  starts from index 0, passing bitnum value as 0 is valid.
 *
 *  \retval CL_OK The function executed successfully, the returned handle is valid.
 *
 *  \retval CL_ERR_NULL_POINTER On passing null pointer in phBitmap.
 *  \retval CL_ERR_NO_MEMORY Memory allocation failure.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This function is used to create a Bitmap of specified length. Initially all the bits of
 *  Bitmap are set to zero. Bitmap bit number starts from index 0.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapDestroy()
 */

ClRcT
clBitmapCreate(CL_OUT ClBitmapHandleT  *phBitmap, 
               CL_IN  ClUint32T        bitNum);

/**
 ************************************************
 *  \brief Destroy a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \par Description:
 *  This function is used to destroy an already created bitmap. This basically frees
 *  up the resources allocated to the bitmap.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapCreate()
 */


ClRcT
clBitmapDestroy(CL_IN ClBitmapHandleT  hBitmap);

/**
 ************************************************
 *  \brief Set a bit of the bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param bitNum (in) The bit number which is to be set.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \par Description:
 *  This function is used to set the specified bit to 1. If the specified bit
 *  number is greater than the length of the bitmap then bitmap length is extended
 *  to the bitnum and then that bit is set. In the later case the bit map
 *  attribute - [number of bits in the bitmap] is updated to the new value.
 *
 *  \note
 *  If the specified bit is already set, CL_OK is returned.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapBitClear(), clBitmapNextClearBitSetNGet()
 */

ClRcT 
clBitmapBitSet(CL_IN ClBitmapHandleT  hBitmap,
               CL_IN ClUint32T        bitNum);

ClRcT 
clBitmapAllBitsSet(ClBitmapHandleT  hBitmap);

/**
 ************************************************
 *  \brief Clear a bit of the bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param bitNum (in) The bit number which is to be cleared.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid bitnum (bit number
 *  greater than the length of bitmap).
 *
 *  \par Description:
 *  This function is used to clear (set to 0)the specified bit. If the specified bit
 *  number is greater than the length of the bitmap then error is returned. 
 *
 *  \note
 *  If the specified bit is already clear, CL_OK is returned.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapBitSet()
 */
ClRcT 
clBitmapBitClear(CL_IN ClBitmapHandleT  hBitmap,
                 CL_IN ClUint32T        bitNum);

ClRcT 
clBitmapAllBitsClear(ClBitmapHandleT  hBitmap);

/**
 ************************************************
 *  \brief Find the status of a bit (Set or Clear) in a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param bitNum (in) The bit number whose status is to be checked.
 *
 *  \param pRetCode (out) This parameter contains the return code. 
 *   It may have following values:
 *  \arg \e CL_OK On success. \n
 *  \arg \e CL_ERR_INVALID_HANDLE On passing invalid bitmap handle. \n
 *  \arg \e CL_ERR_INVALID_PARAMETER On passing invalid bitnum (bit number
 *  greater than the length of the bitmap).
 *
 *  \retval CL_BM_BIT_UNDEF On passing invalid bit number.
 *
 *  \retval CL_BM_BIT_CLEAR If specified bit is clear (0).
 *
 *  \retval CL_BM_BIT_SET If specified bit is set (1).
 *
 *  \par Description:
 *  This function is used to find the status of specified bit. If the specified bit
 *  number is greater than the length of the bitmap then error is returned. 
 *
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapNumBitsSet(), clBitmapLen()
 */

ClInt32T 
clBitmapIsBitSet(CL_IN  ClBitmapHandleT  hBitmap,
                 CL_IN  ClUint32T        bitNum,
                 CL_OUT ClRcT            *pRetCode); 

/**
 ************************************************
 *  \brief Perform unlocked walk on a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param fpUserSetBitWalkCb (in) Pointer to the function which would be
 *  called for each set bit (1) of bitmap.
 *
 *  \param pCookie (in) Cookie information to be passed in the callback function.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_NULL_POINTER On passing NULL function pointer
 *
 *  \par Description:
 *  This function is used to perform unlocked walk on the bitmap. It calls the
 *  registered callback function for every set bit of bitmap, passing the bit
 *  number and the cookie information along with it. This walk is unlocked in
 *  the sense that the lock on bitmap is released while executing the callback
 *  function i.e. other threads/processes are allowed to perform any operation on
 *  the bitmap during the callback execution period. After execution of the
 *  callback, it again gets the lock on bitmap to find the next set bit and
 *  then it releases the lock during callback execution. This continues till
 *  callback is executed for every set bit in the bitmap.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapWalk()
 */
ClRcT 
clBitmapWalkUnlocked(CL_IN ClBitmapHandleT  hBitmap, 
                     CL_IN ClBitmapWalkCbT  fpUserSetBitWalkCb, 
                     CL_IN void             *pCookie);

/**
 ************************************************
 *  \brief Perform locked walk on a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param fpUserSetBitWalkCb (in) Pointer to the function which would be
 *  called for each set bit (1) of bitmap.
 *
 *  \param pCookie (in) Cookie information to be passed in the callback function.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_NULL_POINTER On passing NULL function pointer
 *
 *  \par Description:
 *  This function is used to perform locked walk on the bitmap. It calls the
 *  registered callback function for every set bit of bitmap, passing the bit
 *  number and the cookie information along with it. This walk is locked in
 *  the sense that the lock on bitmap is released only when it completes the
 *  walk i.e. no other thread/process is allowed to perform any operation on
 *  the bitmap during the walk period.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapWalkUnlocked()
 */
ClRcT 
clBitmapWalk(CL_IN ClBitmapHandleT  hBitmap, 
             CL_IN ClBitmapWalkCbT  fpUserSetBitWalkCb, 
             CL_IN void             *pCookie);

/**
 ************************************************
 *  \brief Get the length of a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \retval 0 On passing invalid handle.
 *
 *  \retval len length of the bitmap on success.
 *
 *  \par Description:
 *  This function is used to find the length of the bitmap. It returns 0 on
 *  failure.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapIsBitSet(), clBitmapNumBitsSet()
 */
ClUint32T 
clBitmapLen(CL_IN ClBitmapHandleT  hBitmap);

/**
 ************************************************
 *  \brief Number of bits set in a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in) This parameter is bitmap handle.
 *
 *  \param pNumBits (out) On success this parameter is set to the number of
 *  bits set in the bitmap.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \par Description:
 *  This function is used to find the number of bits set in the bitmap. 
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapIsBitSet(), clBitmapLen()
 */
ClRcT
clBitmapNumBitsSet(CL_IN  ClBitmapHandleT  hBitmap,
                   CL_OUT ClUint32T        *pNumBits);

/**
 ************************************************
 *  \brief Set the next clear bit and get that bit number.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap (in)  This parameter is bitmap handle.
 *
 *  \param length  (in)  This parameter is length of bitmap.
 *
 *  \param pBitSet (out) On success this parameter is set to the bit number
 *  which this function sets.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_NULL_POINTER On passing pBitSet(parameter) as NULL.
 *
 *  \par Description:
 *  This function is used to set the next clear bit of the bitmap i.e. the bit
 *  next to the last set bit. It sets the value of pBitSet parameter to this
 *  bit number on success.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapBitSet()
 */
ClRcT
clBitmapNextClearBitSetNGet(CL_IN  ClBitmapHandleT  hBitmap,
                            CL_IN  ClUint32T        length, 
                            CL_OUT ClUint32T        *pBitSet);

/**
 ************************************************
 *  \brief Get the buffer corresponding to a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap        (in)  This parameter is bitmap handle.
 *
 *  \param pListLen       (out) On success this parameter is set to the length
 *  of the buffer (allocated by this function)
 *
 *  \param ppPositionList (out) On success this parameter contains the
 *  reference to the buffer (this functiion allocates memory to the buffer and 
 *  fills the bitmap info into it.)
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to get the bitmap info(which bit is set and which
 *  one is not) into a buffer. Bit in buffer is set if the corresponding bit
 *  in bitmap is set. The memory for the buffer is allocated in this function, 
 *  this memory is freeed by the user using clHeapFree(). 
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapBuffer2BitmapGet()
 */

ClRcT
clBitmap2BufferGet(CL_IN  ClBitmapHandleT  hBitmap,
                   CL_OUT ClUint32T        *pListLen,
                   CL_OUT ClUint8T         **ppPositionList);

/**
 ************************************************
 *  \brief Get the bitmap corresponding to a bitmap buffer.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param listLen        (in) This parameter contains the length of the
 *  bitmap buffer
 *
 *  \param ppPositionList (in)  This parameter contains the bitmap buffer 
 *  
 *  \param phBitmap       (out) On success it contains pointer to the newly
 *  created bitmap handle.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to get the bitmap corresponding to a bitmap buffer. 
 *  It creates a new bitmap and bit in bitmap is set if the corresponding bit
 *  in bitmap buffer is set.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmap2BufferGet()
 */

ClRcT
clBitmapBuffer2BitmapGet(CL_IN  ClUint32T        listLen, 
                         CL_IN  ClUint8T         *pPositionList,
                         CL_OUT ClBitmapHandleT  *phBitmap);

/**
 ************************************************
 *  \brief Get the position list corresponding to a bitmap.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param hBitmap        (in)  This parameter is bitmap handle.
 *
 *  \param pListLen       (out) On success this parameter is set to the length
 *  of the buffer (allocated by this function)
 *
 *  \param ppPositionList (out) On success this parameter contains the
 *  reference to the position list (this functiion allocates memory to the 
 *  position list and fills the bitmap info into it.)
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE On passing invalid bitmap handle.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to get the bitmap info (only which bit is set) into 
 *  a position list. An element of position list identifies that which bit is
 *  set. Memory for position list is allocated in this function, and this 
 *  memory is freeed by the user using clHeapFree(). 
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapPositionList2BitmapGet()
 */


ClRcT
clBitmap2PositionListGet(CL_IN  ClBitmapHandleT  hBitmap,
                         CL_OUT ClUint32T        *pListLen,
                         CL_OUT ClUint32T        **ppPositionList);

/**
 ************************************************
 *  \brief Get the bitmap corresponding to a bitmap position list.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param listLen        (in)  This parameter contains the length of position
 *  list.
 *
 *  \param ppPositionList (in)  This parameter contains the bitmap position
 *  list
 *  
 *  \param phBitmap       (out) On success it contains pointer to the newly
 *  created bitmap handle.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to get the bitmap corresponding to a bitmap position
 *  list. It creates a new bitmap and bit in bitmap is set according to the
 *  values of the list elements. 
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmap2PositionListGet()
 */
ClRcT
clBitmapPositionList2BitmapGet(CL_IN  ClUint32T        listLen, 
                               CL_IN  ClUint32T        *pPositionList,
                               CL_OUT ClBitmapHandleT  *phBitmap);

/**
 ************************************************
 *  \brief Set the bitmap bits corresponding to the bits in buffer.
 *
 *  \par Header File:
 *  clBitmapApi.h
 *
 *  \param listLen        (in) This parameter contains the length buffer in
 *  bytes.
 *
 *  \param ppPositionList (in) This parameter contains the buffer
 *  
 *  \param phBitmap       (in) It contains already created bitmap handle.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \retval CL_ERR_NULL_POINTER On passing any of the pointer parameter as NULL.
 *
 *  \par Description:
 *  This function is used to set the corresponding bits of an already created 
 *  bitmap. Corresponding to each set bit in the buffer, the bitmap bit is set,
 *  but other bits of bitmap remain unchanged.
 *
 *  \par Library File:
 *  ClUtil
 *
 *  \sa clBitmapBuffer2BitmapGet()
 */


ClRcT
clBitmapBufferBitsCopy(CL_IN ClUint32T        listLen, 
                       CL_IN ClUint8T         *pPositionList,
                       CL_IN ClBitmapHandleT  hBitmap);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_BITMAP_API_H_ */

/** \} */
