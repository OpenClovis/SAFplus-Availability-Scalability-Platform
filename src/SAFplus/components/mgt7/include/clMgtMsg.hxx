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

//For debugging
#define MGT_ACCESS

namespace SAFplus
  {
    /**
     * These message type are used from external,so it should be in SAFplus namespace
     */
//    enum class MgtMsgType
//      {
//      CL_MGT_MSG_UNUSED, CL_MGT_MSG_EDIT, CL_MGT_MSG_GET, CL_MGT_MSG_RPC, CL_MGT_MSG_OID_SET, CL_MGT_MSG_OID_GET, CL_MGT_MSG_BIND, //netconf
//      CL_MGT_MSG_OID_BIND, //snmp
//      CL_MGT_MSG_BIND_RPC,
//      CL_MGT_MSG_NOTIF
//      };
//    enum class MgtRpcMsgType
//      {
//      CL_MGT_RPC_VALIDATE, CL_MGT_RPC_INVOKE, CL_MGT_RPC_POSTREPLY
//      };
//    class MgtMsgProto
//      {
//      public:
//        MgtMsgType messageType;
//        char data[1]; //Not really 1, it will be place on larger memory
//        MgtMsgProto()
//          {
//            messageType = MgtMsgType::CL_MGT_MSG_UNUSED;
//          }
//      };
  }
;
extern "C"
  {

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clSnmpDataDefinition.h>
#include <clSnmpDefs.h>
//
#define MGT_MAX_ATTR_STR_LEN (1024)
#define MGT_MAX_DATA_LEN (5*1024*1024)
//
//    typedef struct ClMgtMessageBindType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT route[MGT_MAX_ATTR_STR_LEN];
//        ClIocPhysicalAddressT iocAddress;
//        bool valid(void)
//          {
//            if (module[0] == 0)
//              return false;
//            if (route[0] == 0)
//              return false;
//            if (iocAddress.nodeAddress > 4096)
//              return false; // Sanity check
//            return true;
//          }
//      } ClMgtMessageBindTypeT;
//
//    typedef struct ClMgtMessageEditType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT route[MGT_MAX_ATTR_STR_LEN];
//        ClInt8T editop;
//        ClCharT data[1];
//      } ClMgtMessageEditTypeT;
//
//    typedef struct ClMgtMessageNotifyType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT notify[CL_MAX_NAME_LENGTH];
//        ClCharT data[1];
//      } ClMgtMessageNotifyTypeT;
//
//    typedef struct ClMgtMessageRpcType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT rpc[CL_MAX_NAME_LENGTH];
//        SAFplus::MgtRpcMsgType rpcType;
//        ClCharT data[1];
//      } ClMgtMessageRpcTypeT;
//
//    /*
//     * Snmp Operation Binding/Getting
//     */
//    typedef struct ClMgtMsgOidBindType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT oid[CL_MAX_NAME_LENGTH];
//        ClIocPhysicalAddressT iocAddress;
//      } ClMgtMsgOidBindTypeT;
//
//    /*
//     * Snmp Operation Get Data
//     */
//    typedef struct ClMgtMsgOidSetType
//      {
//        ClCharT module[CL_MAX_NAME_LENGTH];
//        ClCharT oid[CL_MAX_NAME_LENGTH];
//        ClInt8T opCode;
//        ClCharT data[1];
//      } ClMgtMsgOidSetTypeT;

    typedef void (*ClMsgHandlerCallbackT)( CL_IN ClIocPhysicalAddressT srcAddr, CL_IN void *pInMsg, CL_IN ClUint64T inMsgSize,
        CL_OUT void **ppOutMsg, CL_OUT ClUint64T *outMsgSize);

    typedef void (*ClMsgHandlerAsyncCallbackT)( CL_IN ClRcT rc, CL_IN void* pCookie);

  } /* end extern 'C' */

#endif /* CL_MGT_MGT_H_ */

