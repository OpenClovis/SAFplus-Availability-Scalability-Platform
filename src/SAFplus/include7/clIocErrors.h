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
 * ModuleName  : ioc                                                           
 * File        : clIocErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  IOC error codes are defined in this file.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Ioc Error Codes
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */




#ifndef _CL_IOC_ERRORS_H_
# define _CL_IOC_ERRORS_H_

# ifdef __cplusplus
extern "C"
{
# endif

    /*
     * INCLUDES 
     */
# include <clCommon6.h>
# include <clCommonErrors6.h>

    /*
     * DEFINES 
     */
    /*
     * Intelligent Object Communication -specific error IDs 
     */
/**
 * IOC or Transport is not initialized.
 */
#define CL_IOC_ERR_INIT_FAILED                       0x100 /* 0x50100 */

/**
 * Registering the communication port with IOC has failed.
 */
#define CL_IOC_ERR_COMMPORT_REG_FAIL                 0x101 /* 0x50101 */

/**
 * The operation could not succeed. Can be tried again.
 */
#define CL_IOC_ERR_TRY_AGAIN                         0x102 /* 0x50102 */

/**
 * Invalid mode of communication port.
 */
#define CL_IOC_ERR_COMMPORT_INVALID_MODE             0x103 /* 0x50103 */

/**
 * No more entries can be added to TL.
 */
#define CL_IOC_ERR_TL_LIMIT_EXCEEDED                 0x104 /* 0x50104 */

/**
 * Entry already exists.
 */
#define CL_IOC_ERR_TL_DUPLICATE_ENTRY                0x105 /* 0x50105 */

/**
 * No active instance present.
 */
#define CL_IOC_ERR_TL_ACTIVE_INST_NOT_PRESENT            0x106 /* 0x50106 */

/**
 * Not a valid session.
 */
#define CL_IOC_ERR_INVALID_SESSION                   0x107 /* 0x50107 */

/**
 * The requested component/commport is not reachable.
 */
#define CL_IOC_ERR_COMP_UNREACHABLE                  0x108 /* 0x50108 */

/**
 * Transport is not registered.
 */
#define CL_IOC_ERR_XPORT_NOT_REGISTERED              0x109 /* 0x50109 */

/**
 * Transport is already registered. 
 */
#define CL_IOC_ERR_XPORT_ALREADY_REGISTERED          0x10a /* 0x5010a */

/**
 * Link is not registered.
 */
#define CL_IOC_ERR_XPORT_LINK_NOT_REGISTERED         0x10b /* 0x5010b */

/**
 * Link couldnot be deleted.
 */
#define CL_IOC_ERR_XPORT_LINK_NOT_DELETED            0x10c /* 0x5010c */

/**
 * The requested host is not reachable.
 */
#define CL_IOC_ERR_HOST_UNREACHABLE                  0x10d /* 0x5010d */

/**
 * The communications is blocked, performing the requested operation.
 */
#define CL_IOC_ERR_COMMPORT_BLOCKED                  0x10e /* 0x5010e */

/**
 * Route entry does not exist.
 */
#define CL_IOC_ERR_ROUTE_NOT_EXIST                   0x10f /* 0x5010f */

/**
 * The passed protocol type is IOC internal protocol.
 */
#define CL_IOC_ERR_PROTO_IN_USE_WITH_IOC             0x110 /* 0x50110 */

/**
 * Flow control is in XOFF state.
 */
#define CL_IOC_ERR_FLOW_XOFF_STATE                   0x111 /* 0x50111 */

/**
 * Receive call did not succeed, blocked receive call is unblocked.
 */
#define CL_IOC_ERR_RECV_UNBLOCKED                    0x112 /* 0x50112 */

/**
 * Invalid type of message passed.
 */
#define CL_IOC_ERR_INVALID_MSG_OPTION                0x113 /* 0x50113 */

/**
 * Maximum number of links allowed per transport has already reached.
 */
#define CL_IOC_ERR_XPORT_LINK_LIMIT_EXCEEDED         0x114 /* 0x50114 */

/**
 * Node is already registered.
 */
#define CL_IOC_ERR_NODE_EXISTS                       0x115 /* 0x50115 */

/**
 * IOC maximum of error code.
 */
#define CL_IOC_ERR_MAX                               0x116 /* 0x50116 */

/**
 * Error macro definitions for IOC.
 */
#define CL_IOC_RC(ERROR_CODE)                   CL_RC(CL_CID_IOC, (ERROR_CODE))


# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_ERRORS_H_ */

/** \} */
