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
 * File        : clCkptUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains  common utilities used by checkpoint client and server
*
*
*****************************************************************************/
#ifndef _CL_CKPT_UTILS_H_
#define _CL_CKPT_UTILS_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommonErrors.h>
#include <clCkptErrors.h>    
#include <clLogApi.h>

/* Routine to Log a checkpoint error */
void clCkptLogError(ClUint32T   logLvl, 
                    ClRcT       retCode, 
                    ClUint32T   libCode);
                    
/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/
                    
#define CL_CKPT_SVR            0                 /* Logger is Ckpt server */
#define CL_CKPT_LIB            1                 /* Logger is Ckpt Library */
#define CL_LOG_CKPT_SVR_NAME   "clAspCkptSvr"    /* Ckpt server name */
#define CL_LOG_CKPT_LIB_NAME   "clAspCkptLib"    /* Ckpt library name */

#define CL_CKPT_NANOS_IN_MILI    (CL_CKPT_NANO_TO_MICRO * CL_CKPT_NANO_TO_MICRO)
#define CL_CKPT_MILIS_IN_SEC      CL_CKPT_NANO_TO_MICRO
#define CL_CKPT_NANOS_IN_SEC     (CL_CKPT_NANOS_IN_MILI * CL_CKPT_MILIS_IN_SEC)


/* 
 * Macro for ckpt Server availability check.
 */
#define CL_CKPT_SVR_EXISTENCE_CHECK \
    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) \
		return CL_CKPT_ERR_TRY_AGAIN; \
    

/* 
 * Macros for error checking.
 */
 
#define CKPT_ERR_CHECK(libCode , logLvl, message, rc)\
{\
    if(rc != CL_OK)\
    {\
        clCkptLogError(logLvl, rc, libCode);\
        CL_DEBUG_PRINT(logLvl, message);\
        goto exitOnError;\
    }\
}\

#define CKPT_ERR_CHECK_BEFORE_HDL_CHK(libCode , logLvl, message, rc)\
{\
    if(rc != CL_OK)\
    {\
        clCkptLogError(logLvl, rc, libCode);\
        CL_DEBUG_PRINT(logLvl, message);\
        goto exitOnErrorBeforeHdlCheckout;\
    }\
}



/*
 * NULL check related macros.
 */
 
#define CKPT_NULL_CHECK(x) \
if(x == NULL)\
{ \
    return CL_CKPT_ERR_NULL_POINTER;\
}\

#define CKPT_NULL_CHECK_HDL(x)\
if(x == 0)\
{\
    return CL_CKPT_ERR_INVALID_HANDLE;\
}\


#define CKPT_COMP_NAME  "ckptSvr" /* Ckpt component name */

#define CKPT_LOCK      clOsalMutexLock    /* Locking */
#define CKPT_UNLOCK    clOsalMutexUnlock  /* Unlocking */

#ifdef __cplusplus
}
#endif

#endif							/* _CL_CKPT_UTILS_H_ */ 
