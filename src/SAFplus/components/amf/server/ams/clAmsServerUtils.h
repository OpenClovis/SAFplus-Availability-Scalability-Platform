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
 * ModuleName  : amf
 * File        : clAmsServerUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains utility definitions required by AMS.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/


#ifndef _CL_AMS_SERVER_UTILS_H_
#define _CL_AMS_SERVER_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clAmsErrors.h>
#include <clAms.h>
#include <clAmsMgmtCommon.h>
#include <clAmsModify.h>

/******************************************************************************
 * Debug Defines
 *****************************************************************************/

/* object management functions */

extern ClAmsEntityRefT* clAmsAllocEntityRef();
extern void clAmsFreeEntityRef(ClAmsEntityRefT* ref);
extern ClAmsEntityRefT* clAmsCreateEntityRef(ClAmsEntityT* ent);
    
/* Log formatting */    
    
extern char *clAmsFormatMsg (const char *fmt, ...);    
extern void clAmsLogMsgServer( const ClUint32T level, char *buffer, const ClCharT* file, ClUint32T line );
extern ClAmsT   gAms;


#define AMS_SERVER

#define AMS_LOG_COUNT_STRING   clAmsFormatMsg ("AMF (%04d.%05d) ",      \
                                    gAms.ops.currentOp,                 \
                                    AMS_LOG_INCR(gAms.logCount))

#define AMS_LOG AMS_SERVER_LOG                                          \

/******************************************************************************
 * Common Error Checking Defines
 *****************************************************************************/

#define AMS_CALL(fn)                                                    \
do {                                                                    \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    returnCode = (fn);                                                  \
                                                                        \
    if (CL_GET_ERROR_CODE(returnCode) == CL_ERR_NO_OP)                  \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_TRACE, ("Function [%s] returned NoOp\n", #fn)); \
        return returnCode;                                              \
    } \
    if (returnCode != CL_OK) clDbgCodeError(CL_LOG_SEV_ERROR, ("Fn [%s] returned [0x%x]\n", #fn, returnCode) ); \
                                                                        \
    if (returnCode != CL_OK)                                            \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Fn [%s] returned [0x%x]\n",               \
             __FUNCTION__, __LINE__, #fn, returnCode));                 \
        return returnCode;                                              \
    }                                                                   \
} while (0)

#define AMS_LOG_ERR(fn)                                                  \
do {                                                                    \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    returnCode = (fn);                                                  \
    if (CL_GET_ERROR_CODE(returnCode) == CL_ERR_NO_OP)                  \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_DEBUG, ("Function [%s] returned NoOp", #fn)); \
    }                                                                   \
                                                                        \
    if (returnCode != CL_OK) clDbgCodeError(CL_LOG_SEV_WARNING, ("Fn [%s] returned [0x%x]", #fn, returnCode) ); \
                                                                        \
    if (returnCode != CL_OK)                                            \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR, ("ALERT [%s:%d] : Fn [%s] returned [0x%x]\n", __FUNCTION__, __LINE__, #fn, returnCode));                 \
    }                                                                   \
} while (0)

    
#define AMS_CHECKPTR_AND_UNLOCK(x,mutex)                                \
{                                                                       \
    if ( (x) != CL_FALSE )                                              \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expression (%s) is True. Null Pointer\n", \
             __FUNCTION__, __LINE__, #x));                              \
        AMS_CALL ( clOsalMutexUnlock (mutex));                          \
        rc = CL_ERR_NULL_POINTER;                                       \
        goto exitfn;                                                    \
    }                                                                   \
}

#define AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(fn,mutex)   \
{                                                       \
    rc = (fn);                                          \
                                                        \
    if ( (rc) != CL_OK )                                \
    {                                                   \
        AMS_CALL ( clOsalMutexUnlock(mutex));           \
        goto exitfn;                                    \
    }                                                   \
}

#define AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(serviceState, mutex)        do { \
    if ( (serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&              \
            (serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )       \
    {                                                                   \
        clOsalMutexUnlock(mutex);                                       \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                                  \
                ("AMS server is not functioning, dropping the request\n")); \
        rc = CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);                  \
        goto exitfn;                                                    \
    }                                                                   \
} while(0)

#define AMS_CHECK_SERVICE_STATE_LOCKED(serviceState)                           do { \
    clOsalMutexLock(gAms.mutex);                                    \
    if ( (serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&          \
         (serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )      \
    {                                                               \
        clOsalMutexUnlock(gAms.mutex);                              \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                              \
                       ("AMS server is not functioning, dropping the request\n")); \
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);            \
    }                                                               \
    clOsalMutexUnlock(gAms.mutex);                                  \
}while(0)

#define AMS_ENTITY_OK(x,y) ((x)&&(((x)->config.entity.type) == (y) ))
    
#define AMS_CHECK_ENTITY(x,y)                                do {       \
    AMS_CHECKPTR ( !(x) );                                              \
                                                                        \
    if ( ((x)->config.entity.type) != (y) )                             \
    {                                                                   \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expecting entity type %d, got type %d\n", \
             __FUNCTION__, __LINE__, (y), (x)->config.entity.type));    \
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);                    \
    }                                                                   \
    clAmsMarkEntityDirty((ClAmsEntityT*)(x));                           \
} while(0)

#define AMS_CHECK_ENTITY_AND_UNLOCK(x, y, mutex)                   do { \
    AMS_CHECKPTR_AND_UNLOCK( !(x), mutex );                             \
                                                                        \
    if ( ((x)->config.entity.type) != (y) )                             \
    {                                                                   \
        clOsalMutexUnlock(mutex);                                       \
        AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expecting entity type %d, got type %d\n", \
             __FUNCTION__, __LINE__, (y), (x)->config.entity.type));    \
        rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);                      \
        goto exitfn;                                                    \
    }                                                                   \
    clAmsMarkEntityDirty((ClAmsEntityT*)(x));                           \
} while(0)

#define AMS_ENTITY_LOG(ENTITY, DEBUGFLAG, LEVEL, MSG) do { clLogMsgWrite(CL_LOG_HANDLE_SYS, LEVEL, 0, "SVR", "ENT", __FILE__, __LINE__,  AMS_STRIP_PARENS MSG ); } while(0)
    
#if 0
{                                                                       \
    if ( LEVEL == CL_LOG_SEV_ERROR )                                      \
    {                                                                   \
        clAmsLogMsgServer( LEVEL, clAmsFormatMsg MSG,__FILE__,__LINE__ );                 \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if ( ( gAms.debugFlags & (DEBUGFLAG) ) ||                       \
             ( ((ClAmsEntityT *) ENTITY)->debugFlags & (DEBUGFLAG) ) )  \
        {                                                               \
            clAmsLogMsgServer ( LEVEL, clAmsFormatMsg MSG,__FILE__,__LINE__);            \
        }                                                               \
    }                                                                   \
}
#endif

#define AMS_SERVER_LOG(LEVEL, MSG)  do { clLogMsgWrite(CL_LOG_HANDLE_SYS, LEVEL, 0, "SVR", "---", __FILE__, __LINE__,  AMS_STRIP_PARENS MSG ); } while(0)

#if 0
{                                                                       \
    if ( LEVEL == CL_LOG_SEV_ERROR )                                      \
    {                                                                   \
        clAmsLogMsgServer ( LEVEL, clAmsFormatMsg MSG,__FILE__,__LINE__);                \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if ( ( gAms.debugFlags&CL_AMS_MGMT_SUB_AREA_MSG ) != CL_FALSE ) \
        {                                                               \
            clAmsLogMsgServer ( LEVEL, clAmsFormatMsg MSG,__FILE__,__LINE__);            \
        }                                                               \
    }                                                                   \
}
#endif

#define AMS_CALL_CKPT_WRITE(fn)                                         \
{                                                                       \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    if ( ( gAms.serviceState == CL_AMS_SERVICE_STATE_RUNNING ) ||       \
            ( gAms.serviceState == CL_AMS_SERVICE_STATE_STARTINGUP ) || \
            ( gAms.serviceState == CL_AMS_SERVICE_STATE_SHUTTINGDOWN )) \
    {                                                                   \
                                                                        \
        AMS_CALL ( clOsalMutexLock (&gAms.ckptMutex));                  \
        returnCode = (fn);                                              \
        AMS_CALL ( clOsalMutexUnlock (&gAms.ckptMutex));                \
        if ( returnCode != CL_OK )                                      \
        {                                                               \
            AMS_SERVER_LOG(CL_LOG_SEV_ERROR,                              \
                    ("ALERT [%s:%d] : Fn [%s] returned [0x%x]\n",       \
                     __FUNCTION__, __LINE__, #fn, returnCode));         \
        }                                                               \
    }                                                                   \
}


#define AMS_FUNC_ENTER( arg )                                           \
{                                                                       \
    if ( ( gAms.debugFlags&CL_AMS_MGMT_SUB_AREA_FN_CALL ) != CL_FALSE ) \
    {                                                                   \
        clAmsLogMsgServer ( CL_LOG_SEV_TRACE, AMS_LOG_COUNT_STRING,__FILE__,__LINE__);      \
        clAmsLogMsgServer ( CL_LOG_SEV_TRACE,                             \
            clAmsFormatMsg ("Fn [%s:%d] : ", __FUNCTION__, __LINE__),__FILE__,__LINE__);  \
        clAmsLogMsgServer ( CL_LOG_SEV_TRACE, clAmsFormatMsg arg,__FILE__,__LINE__);       \
    }                                                                   \
}

#define AMS_CALL_PUBLISH_NTF(fn)                                        \
{                                                                       \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    returnCode = (fn);                                                  \
                                                                        \
    if ( returnCode != CL_OK )                                          \
    {                                                                   \
        AMS_LOG(CL_LOG_SEV_ERROR,                                         \
                ("ALERT [%s:%d] : Fn [%s] returned [0x%x]\n",           \
                 __FUNCTION__, __LINE__, #fn, returnCode));             \
    }                                                                   \
}
#define AMS_CHECK_NODE(x)   AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_NODE) 
#define AMS_CHECK_APP(x)    AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_APP) 
#define AMS_CHECK_SG(x)     AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_SG) 
#define AMS_CHECK_SU(x)     AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_SU) 
#define AMS_CHECK_SI(x)     AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_SI) 
#define AMS_CHECK_COMP(x)   AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_COMP) 
#define AMS_CHECK_CSI(x)    AMS_CHECK_ENTITY(x,CL_AMS_ENTITY_TYPE_CSI) 

#define CL_LOG_AREA_AMS "AMS"

#define CL_LOG_CONTEXT_AMS_FAULT_COMP "FLT-COMP"

#define CL_LOG_CONTEXT_AMS_FAULT_SU   "FLT-SU"

#define CL_LOG_CONTEXT_AMS_FAULT_NODE "FLT-NODE"

#define CL_LOG_CONTEXT_AMS_RESTART_NODE "RESTART"

#include <clAmsUtils.h>

    
#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_SERVER_UTILS_H_ */
