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

#ifndef CL_MGT_MSG_H_
#define CL_MGT_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clSnmpDataDefinition.h>
#include <clSnmpDefs.h>

#define MGT_MAX_ATTR_STR_LEN (1024)
#define MGT_MAX_DATA_LEN (5*1024*1024)

#ifdef MGT_ACCESS
    typedef enum
    {
        CL_MGT_MSG_NONE = 0,
        CL_MGT_MSG_BIND,
        CL_MGT_MSG_BIND_RPC,
        CL_MGT_MSG_EDIT,
        CL_MGT_MSG_GET,
        CL_MGT_MSG_NOTIF,
        CL_MGT_MSG_RPC,
        CL_MGT_MSG_OID_BIND,
        CL_MGT_MSG_OID_GET,
        CL_MGT_MSG_OID_SET
    } ClMgtMsgT;

    typedef enum
    {
        CL_MGT_RPC_VALIDATE = 0, CL_MGT_RPC_INVOKE, CL_MGT_RPC_POSTREPLY
    } ClMgtRpcT;


    typedef struct ClMgtMessageBindType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT route[MGT_MAX_ATTR_STR_LEN];
        ClIocPhysicalAddressT iocAddress;
        bool valid(void)
        {
            if (module[0] == 0)
                return false;
            if (route[0] == 0)
                return false;
            if (iocAddress.nodeAddress > 4096)
                return false; // Sanity check
            return true;
        }
    } ClMgtMessageBindTypeT;

    typedef struct ClMgtMessageEditType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT route[MGT_MAX_ATTR_STR_LEN];
        ClInt8T editop;
        ClCharT data[1];
    } ClMgtMessageEditTypeT;

    typedef struct ClMgtMessageNotifyType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT notify[CL_MAX_NAME_LENGTH];
        ClCharT data[1];
    } ClMgtMessageNotifyTypeT;

    typedef struct ClMgtMessageRpcType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT rpc[CL_MAX_NAME_LENGTH];
        ClInt8T rpcType;
        ClCharT data[1];
    } ClMgtMessageRpcTypeT;

    /*
     * Snmp Operation Binding/Getting
     */
    typedef struct ClMgtMsgOidBindType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT oid[CL_MAX_NAME_LENGTH];
        ClIocPhysicalAddressT iocAddress;
    } ClMgtMsgOidBindTypeT;

    /*
     * Snmp Operation Get Data
     */
    typedef struct ClMgtMsgOidSetType
    {
        ClCharT module[CL_MAX_NAME_LENGTH];
        ClCharT oid[CL_MAX_NAME_LENGTH];
        ClInt8T opCode;
        ClCharT data[1];
    } ClMgtMsgOidSetTypeT;

    typedef void (*ClMsgHandlerCallbackT)(
            CL_IN ClIocPhysicalAddressT srcAddr,
            CL_IN void *pInMsg,
            CL_IN ClUint64T inMsgSize,
            CL_OUT void **ppOutMsg,
            CL_OUT ClUint64T *outMsgSize);

    typedef void (*ClMsgHandlerAsyncCallbackT)(
            CL_IN ClRcT rc,
            CL_IN void* pCookie);

    void clMgtMsgBindHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgBindRpcHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgEditHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgNotifyHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgRpcHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgOidSetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);
    void clMgtMsgOidGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
            ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize);

#endif

#ifdef __cplusplus
} /* end extern 'C' */
#endif

#endif /* CL_MGT_MGT_H_ */

