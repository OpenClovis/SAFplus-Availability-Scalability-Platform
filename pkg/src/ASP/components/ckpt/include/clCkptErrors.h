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

/**
 * \file 
 * \brief Header file of Ckpt Service defined Errors
 * \ingroup ckpt_apis_server
 */

/**
 * \addtogroup ckpt_apis_server
 * \{
 */

#ifndef _CL_CKPT_ERRORS_H_
#define _CL_CKPT_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * INCLUDES 
     */
#include <clCommon.h>
#include <clCommonErrors.h>

/**
  * While iterating the sections on the checkpoint, if no more sections are available  
  * this error will be returned.
  */
#define CL_CKPT_ERR_NO_SECTIONS          0x101
  

/*
 * Macros for checkpointing related errors.
 */
#define CKPT_RC(ERR_ID)       (CL_RC(CL_CID_CKPT, ERR_ID))

#define CL_CKPT_ERR_NO_MEMORY         CKPT_RC(CL_ERR_NO_MEMORY)
#define CL_CKPT_ERR_INVALID_PARAMETER CKPT_RC(CL_ERR_INVALID_PARAMETER)
#define CL_CKPT_ERR_NULL_POINTER      CKPT_RC(CL_ERR_NULL_POINTER)
#define CL_CKPT_ERR_NOT_EXIST         CKPT_RC(CL_ERR_NOT_EXIST)
#define CL_CKPT_ERR_INVALID_HANDLE    CKPT_RC(CL_ERR_INVALID_HANDLE)    
#define CL_CKPT_ERR_INVALID_BUFFER    CKPT_RC(CL_ERR_INVALID_BUFFER)    
#define CL_CKPT_ERR_NOT_IMPLEMENTED   CKPT_RC(CL_ERR_NOT_IMPLEMENTED)   
#define CL_CKPT_ERR_DUPLICATE         CKPT_RC(CL_ERR_DUPLICATE)         
#define CL_CKPT_ERR_OUT_OF_RANGE      CKPT_RC(CL_ERR_OUT_OF_RANGE)      
#define CL_CKPT_ERR_NO_RESOURCE       CKPT_RC(CL_ERR_NO_RESOURCE)       
#define CL_CKPT_ERR_INITIALIZED       CKPT_RC(CL_ERR_INITIALIZED)       
#define CL_CKPT_ERR_BUFFER_OVERRUN    CKPT_RC(CL_ERR_BUFFER_OVERRUN)    
#define CL_CKPT_ERR_NOT_INITIALIZED   CKPT_RC(CL_ERR_NOT_INITIALIZED)   
#define CL_CKPT_ERR_VERSION_MISMATCH  CKPT_RC(CL_ERR_VERSION_MISMATCH)  
#define CL_CKPT_ERR_ALREADY_EXIST     CKPT_RC(CL_ERR_ALREADY_EXIST)     
#define CL_CKPT_ERR_UNSPECIFIED       CKPT_RC(CL_ERR_UNSPECIFIED)       
#define CL_CKPT_ERR_INVALID_STATE     CKPT_RC(CL_ERR_INVALID_STATE)     
#define CL_CKPT_ERR_DOESNT_EXIST      CKPT_RC(CL_ERR_DOESNT_EXIST)      
#define CL_CKPT_ERR_TIMEOUT           CKPT_RC(CL_ERR_TIMEOUT)           
#define CL_CKPT_ERR_INUSE             CKPT_RC(CL_ERR_INUSE)             
#define CL_CKPT_ERR_TRY_AGAIN         CKPT_RC(CL_ERR_TRY_AGAIN)         
#define CL_CKPT_ERR_NO_CALLBACK       CKPT_RC(CL_ERR_NO_CALLBACK)       
#define CL_CKPT_ERR_MUTEX_ERROR       CKPT_RC(CL_ERR_MUTEX_ERROR)       
#define CL_CKPT_ERR_OP_NOT_PERMITTED  CKPT_RC(CL_ERR_OP_NOT_PERMITTED)       
#define CL_CKPT_ERR_NO_SPACE          CKPT_RC(CL_ERR_NO_SPACE)     
#define CL_CKPT_ERR_BAD_FLAG          CKPT_RC(CL_ERR_BAD_FLAG)      
#define CL_CKPT_ERR_BAD_OPERATION     CKPT_RC(CL_ERR_BAD_OPERATION)      

    
#ifdef __cplusplus
}
#endif

#endif  /* _CL_CKPT_ERRORS_H_ */

/** 
 * \} 
 */
