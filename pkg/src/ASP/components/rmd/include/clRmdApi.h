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
 * ModuleName  : rmd
 * File        : clRmdApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header defines the RMD Interface.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of RMD Interface
 *  \ingroup rmd_apis
 */

/**
 *  \addtogroup rmd_apis
 *  \{
 */

#ifndef _CL_RMD_API_H_
# define _CL_RMD_API_H_

# ifdef __cplusplus
extern "C"
{
# endif


/*****************************************************************************/


# include <clBufferApi.h>
# include <clIocApi.h>
# include <clRmdErrors.h>


    /*
     * RMD Flags
     */

/**
 * 0x0001 If you want to make a synchronous call, set this bit to 0. When this bit is set to 1, it is an asynchronous call.
 */
# define CL_RMD_CALL_ASYNC               (1<<0)

/**
 * 0x0002 If you do not require to check the reply, set this bit to 0.
 */
# define CL_RMD_CALL_NEED_REPLY          (1<<1)

/**
 * 0x0004 If you want to perform Atmost Once Semantics, you are required to set this bit to 1.
 */
# define CL_RMD_CALL_ATMOST_ONCE         (1<<2)

/**
 * 0x0008 If you want to make a non-optimized RMD call, you are required to set this bit to 1.
 */
# define CL_RMD_CALL_DO_NOT_OPTIMIZE     (1<<3)

/**
 * 0x0010 If you want RMD to use and free its input message, you are required to set this bit to 1.
 */
# define CL_RMD_CALL_NON_PERSISTENT      (1<<4)

/**
 * 0x0020 If you want to use the services of the same EO instance as in previous call,
 * you are required to set this bit.
 */
# define CL_RMD_CALL_IN_SESSION          (1<<5)

    /*
     * Default Values
     */

/*
 * Default header version of the RMD.
 */
#define CL_RMD_HEADER_VERSION     1
/**
 * Default priority of the RMD call.
 */
# define CL_RMD_DEFAULT_PRIORITY   CL_IOC_DEFAULT_PRIORITY

/**
 * Default timeout for the RMD call - 10 seconds
 * QNX has a traffic shaper leaky bucket. So have higher timeouts.
 */

#ifdef QNX_BUILD

# define CL_RMD_DEFAULT_TIMEOUT    50000

#else

# define CL_RMD_DEFAULT_TIMEOUT    10000

#endif

/**
 * Default retries for the RMD call.
 */
# define CL_RMD_DEFAULT_RETRIES    5

/**
 * Default transport handle for the RMD call.
 */
# define CL_RMD_DEFAULT_TRANSPORT_HANDLE  0

/**
 * Default values for the options.
 */
#define CL_RMD_DEFAULT_OPTIONS { \
                                    CL_RMD_DEFAULT_TIMEOUT, \
                                    CL_RMD_DEFAULT_RETRIES, \
                                    CL_RMD_DEFAULT_PRIORITY,  \
                                    CL_RMD_DEFAULT_TRANSPORT_HANDLE, \
                               }

#define CL_RMD_DEFAULT_OPTIONS_SET(options) \
        (options).timeout = CL_RMD_DEFAULT_TIMEOUT, \
        (options).retries = CL_RMD_DEFAULT_RETRIES, \
        (options).priority = CL_RMD_DEFAULT_PRIORITY, \
        (options).transportHandle = CL_RMD_DEFAULT_LINK_HANDLE
/**
 * When this is used, RMD waits forever for the call to complete and does not issue timeouts.
 */
# define CL_RMD_TIMEOUT_FOREVER    -1

/*
 * Check for typical rmd unreachable errors
 */
#define CL_RMD_UNREACHABLE_CHECK(ret)                           \
    (CL_GET_ERROR_CODE((ret)) == CL_IOC_ERR_COMP_UNREACHABLE || \
     CL_GET_ERROR_CODE((ret)) == CL_IOC_ERR_HOST_UNREACHABLE)

#define CL_RMD_TIMEOUT_UNREACHABLE_CHECK(ret)                           \
    ( CL_GET_ERROR_CODE((ret))== CL_ERR_TIMEOUT || CL_RMD_UNREACHABLE_CHECK(ret) ) 

#define CL_RMD_VERSION_ERROR(rc)  ( (rc) == CL_RC(CL_CID_EO, CL_ERR_DOESNT_EXIST) \
                                    || (rc) == CL_RC(CL_CID_EO, CL_ERR_VERSION_MISMATCH) )
        
/*
 * Type definitions
 */


/**
 * \brief Callback function pointer for the async RMD call.
 *
 * \param retCode The return code of the remote function or RMD return code. 
 * \param pCookie The value that you passed when you made the RMD call.
 * \param inMsgHdl Input passed when making the call.
 * \param outMsgHdl This is the output from the remote server.  You must "free" this pointer and ONLY this pointer.
 * 
 * \par Description:
 * Callback function pointer for the asynchronous call. You need to free the output message only.
 *
 * \sa clRmdWithMsg()
 */
    typedef void (*ClRmdAsyncCallbackT) (
                                            ClRcT retCode,
                                            ClPtrT pCookie,
                                            ClBufferHandleT inMsgHdl,
                                            ClBufferHandleT outMsgHdl);


    typedef ClPtrT ClRmdObjHandleT;

/**
 * This is a structure to pass optional parameters.
 */
    typedef struct ClRmdOptions
    {

/**
 * Timeout value per try in miliseconds. Also timeout must be the
 * expected time of execution of the remote function. It should be in order
 * of the minimum resolution timerlib provides. The values -1 or 0 means timeout forever.
 *
 */
        ClUint32T timeout;

/**
 * Number of times you can retry after the first call in case of
 * timeout. You can set the maximum number of retries.
 */
        ClUint32T retries;

/**
 * Priority value for the call. It will be passed on to the IOC without modification.
 */
        ClUint8T priority;

/**
 * The Transport Handle obtained via clIocBind() to make the RMD call via
 * the specified transport.
 */
        ClIocToBindHandleT transportHandle;

    } ClRmdOptionsT;

/**
 * This structure ClRmdAsyncOptionsT contains additional asynchronous call parameters. It is used to pass these parameters.
 */
    typedef struct ClRmdAsyncOptions
    {
/**
 * User's cookie.
 */
        ClPtrT pCookie;
/**
 * User's callback.
 */
        ClRmdAsyncCallbackT fpCallback;

    } ClRmdAsyncOptionsT;



/**
 ************************************
 *  \brief Invokes a Remote Function Call when the parameters are passed as messages.
 *
 *  \par Header File:
 *  clRmdApi.h
 *
 *  \param remoteObjAddr Address of the destination Object.
 *  \param funcId Function Id to be executed.
 *  \param inMsgHdl Created and freed by the caller except when CL_RMD_CALL_NON_PERSISTENT
 *  is set. In this case, RMD frees the message. NULL indicates no value is passed
 *  to the remote end by you.
 *  \param outMsgHdl [out] Created and freed by the caller. If it is NULL and
 *  \e CL_RMD_CALL_NEED_REPLY flag is set, CL_RMD_RC(CL_ERR_INVALID_PARAM) will be returned.
 *  \param flags It informs RMD to decide the type of call. You must specify the flag to indicate
 *  whether the call is Synchronous or Asynchronous, or whether Reply and Atmost Once semantics
 *  are required or not.
 *  \param options Optional parameter that can be passed like- priority, timeout,
 *  cookie, retries, and callback function. \n
 *  If it is NULL, default values be assumed.
 *  \param pAsyncOptions This is to be passed only if the call made is asynchronous. In this
 *  parameter, optional parameters, like cookies and callback functions can be passed.
 *
 *  \retval CL_OK The API executed successfully
 *  \par 
 *
 *  \retval CL_ERR_NO_MEMORY If system memory is no longer available
 *  \retval CL_ERR_TIMEOUT If reply is not received in specified time or an invalid IOC port id is passed
 *  \retval CL_ERR_NULL_PTR On passing a NULL pointer
 *  \retval CL_INVALID_PARAMETER On passing invalid parameters.
 *  \retval CL_ERR_NOT_INITIALIZED If RMD Library is not initialized.
 *  \retval CL_ERR_NOT_IMPLEMENTED If call type is not supported.
 *  It also returns some OS defined error codes.
 *  \par 
 *  Error Codes listed above are generated by RMD, therefore the  return
 *  value contains the RMD Component Identifier.
 *
 *  \retval CL_EO_ERR_FUNC_NOT_REGISTERED If the requested function is not registered.
 *  \retval CL_EO_ERR_EO_SUSPENDED If the remote EO is in suspended state.
 *  \par 
 *  Error codes listed above are generated by EO, therefore the  return
 *  value contains the EO Component Identifier.
 *
 *  \retval CL_IOC_ERR_RECV_UNBLOCKED If the receiver is unblocked.
 *  \retval CL_IOC_ERR_HOST_UNREACHABLE If an invalid node address is passed.
 *  \retval CL_IOC_ERR_COMP_UNREACHABLE If an invalid commport address is passed.
 *  \retval CL_ERR_NOT_EXIST If an invalid logical address is passed.
 *  \par 
 *  Error codes listed above are generated by IOC, therefore the  return
 *  value contains the IOC Component Identifier.
 *
 *  \par Description:
 *  This API is used to invoke a Remote Function Call. You must pass the the the following parameters.
 *  \arg destination address, that is, the remote ObjectId where the function is exposed,
 *  \arg the \e functionId to invoke
 *  \arg input parameter in a message
 *  \arg output message to receive the reply
 *  \arg RMD specific flags 
 *  \arg RMD options 
 *  \arg RMD asynchronous calls options 
 *  \par
 *  While making an asynchronous call, you need to pass options like- priority, timeout value,
 *  retries, cookie, and the callback function in a structure ClRmdAsyncOptions. But all the parameters
 *  are not mandatory for each and every call. As in case of synchronous call, cookie and callback
 *  function are not required to pass. 
 *  \par
 *  The remote function is identifed by the remote object address and the
 *  Function ID.
 *
 *  \par Library File:
 *  libClRmd
 *
 */

    ClRcT clRmdWithMsg(CL_IN ClIocAddressT remoteObjAddr,   /* remote Object
                                                             * addr */
                       CL_IN ClUint32T funcId,  /* Function ID to invoke */
                       CL_IN ClBufferHandleT inMsgHdl,   /* Input
                                                                 * Message */
                       CL_OUT ClBufferHandleT outMsgHdl, /* Output
                                                                 * Message */
                       CL_IN ClUint32T flags,   /* Flags */
                       CL_IN ClRmdOptionsT *pOptions,   /* Optional Parameters
                                                         * for RMD Call */
                       CL_IN ClRmdAsyncOptionsT *pAsyncOptions);    /* Optional
                                                                     * Parameters
                                                                     * for
                                                                     * Async
                                                                     * RMD Call
                                                                     */
 

    ClRcT clRmdWithMsgVer(CL_IN ClIocAddressT remoteObjAddr,   /* remote Object
                                                             * addr */
                          CL_IN ClVersionT *version,
                          CL_IN ClUint32T funcId,  /* Function ID to invoke */
                          CL_IN ClBufferHandleT inMsgHdl,   /* Input
                                                                 * Message */
                          CL_OUT ClBufferHandleT outMsgHdl, /* Output
                                                                 * Message */
                          CL_IN ClUint32T flags,   /* Flags */
                          CL_IN ClRmdOptionsT *pOptions,   /* Optional Parameters
                                                         * for RMD Call */
                          CL_IN ClRmdAsyncOptionsT *pAsyncOptions);    /* Optional
                                                                     * Parameters
                                                                     * for
                                                                     * Async
                                                                     * RMD Call
                                                                     */



# ifdef __cplusplus
}
# endif


#endif                          /* _CL_RMD_API_H_ */

/** \} */

