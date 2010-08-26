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
 * ModuleName  : ckpt                                                          
 * File        : clCkptLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint service specific log messages
*
*
*****************************************************************************/
#ifndef _CL_CKPT_LOG_H_
#define _CL_CKPT_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>


/*
 * Pointer to Log message array.
 */
 
extern ClCharT     *clCkptLogMsg[];


/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/


#define CL_CKPT_LOG_1_CKPT_CREATED		clCkptLogMsg[0]
#define CL_CKPT_LOG_1_CKPT_CLOSED	    clCkptLogMsg[1]
#define CL_CKPT_LOG_1_CKPT_DELETED		clCkptLogMsg[2]
#define CL_CKPT_LOG_1_SEC_CREATED  		clCkptLogMsg[3]
#define CL_CKPT_LOG_1_SEC_DELETED		clCkptLogMsg[4]
#define CL_CKPT_LOG_1_CKPT_WRITTEN		clCkptLogMsg[5]
#define CL_CKPT_LOG_1_SEC_OVERWRITTEN	clCkptLogMsg[6]
#define CL_CKPT_LOG_1_CKPT_READ	     	clCkptLogMsg[7]
#define CL_CKPT_LOG_6_VERSION_NACK	    clCkptLogMsg[8]
#define CL_CKPT_LOG_1_HDL_FAILURE	    clCkptLogMsg[9]

#ifdef __cplusplus
}
#endif


#endif /* _CL_CKPT_LOG_H_ */
