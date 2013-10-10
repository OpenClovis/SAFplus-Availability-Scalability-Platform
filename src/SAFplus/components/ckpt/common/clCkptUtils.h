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
#define CL_LOG_SP(...) __VA_ARGS__
 
#define CKPT_ERR_CHECK(libCode , logLvl, message, rc)\
{\
    if(rc != CL_OK)\
    {\
        char __tempstr[256];   \
        snprintf(__tempstr,256,CL_LOG_SP message); \
        clCkptLogError(logLvl, rc, libCode);\
        clLog(logLvl,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,__tempstr);\
        goto exitOnError;\
    }\
}\

#define CKPT_ERR_CHECK_BEFORE_HDL_CHK(libCode , logLvl, message, rc)\
{\
    if(rc != CL_OK)\
    {\
        char __tempstr[256];   \
        snprintf(__tempstr,256,CL_LOG_SP message); \
        clCkptLogError(logLvl, rc, libCode);\
        clLog(logLvl,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,__tempstr);\
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
