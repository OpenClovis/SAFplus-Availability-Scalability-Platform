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

#include "clMgtRoot.hxx"
#ifdef MGT_ACCESS
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCommon.hxx>
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

namespace SAFplus
{
  MgtRoot *MgtRoot::singletonInstance = 0;

  MgtRoot *MgtRoot::getInstance()
  {
    return (singletonInstance ? singletonInstance : (singletonInstance = new MgtRoot()));
  }

  MgtRoot::~MgtRoot()
  {
  }

  MgtRoot::MgtRoot()
  {
#ifdef MGT_ACCESS
    /*
     * Message server to communicate with snmp/netconf
     */
    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    SAFplus::MgtRoot::MgtMessageHandler mgtMessageHandler(this);
    mgtIocInstance->RegisterHandler(SAFplusI::CL_MGT_MSG_TYPE, &mgtMessageHandler, NULL);
#endif
  }

  ClRcT MgtRoot::loadMgtModule(MgtModule *module, std::string moduleName)
  {
    if (module == NULL)
    {
      return CL_ERR_NULL_POINTER;
    }

    /* Check if MGT module already exists in the database */
    if (mMgtModules.find(moduleName) != mMgtModules.end())
    {
      logDebug("MGT", "LOAD", "Module [%s] already exists!", moduleName.c_str());
      return CL_ERR_ALREADY_EXIST;
    }

    /* Insert MGT module into the database */
    mMgtModules.insert(pair<string, MgtModule *>(moduleName.c_str(), module));
    logDebug("MGT", "LOAD", "Module [%s] added successfully!", moduleName.c_str());

    return CL_OK;
  }

  ClRcT MgtRoot::unloadMgtModule(std::string moduleName)
  {
    /* Check if MGT module already exists in the database */
    if (mMgtModules.find(moduleName) == mMgtModules.end())
    {
      logDebug("MGT", "LOAD", "Module [%s] is not existing!", moduleName.c_str());
      return CL_ERR_NOT_EXIST;
    }

    /* Remove MGT module out off the database */
    mMgtModules.erase(moduleName);
    logDebug("MGT", "LOAD", "Module [%s] removed successful!",moduleName.c_str());

    return CL_OK;
  }

  MgtModule *MgtRoot::getMgtModule(const std::string moduleName)
  {
    map<string, MgtModule*>::iterator mapIndex = mMgtModules.find(moduleName);
    if (mapIndex != mMgtModules.end())
    {
      return static_cast<MgtModule *>((*mapIndex).second);
    }
    return NULL;
  }

  ClRcT MgtRoot::bindMgtObject(ClUint8T bindType, MgtObject *object,const std::string module, const std::string route)
  {
    ClRcT rc = CL_OK;

    if (object == NULL)
    {
      return CL_ERR_NULL_POINTER;
    }

    /* Check if MGT module already exists in the database */
    MgtModule *mgtModule = getMgtModule(module);
    if (mgtModule == NULL)
    {
      logDebug("MGT", "BIND", "Module [%s] does not exist!",module.c_str());
      return CL_ERR_NOT_EXIST;
    }

    rc = mgtModule->addMgtObject(object, route);
    if ((rc != CL_OK) && (rc != CL_ERR_ALREADY_EXIST))
    {
      logDebug("MGT", "BIND", "Binding module [%s], route [%s], returning rc[0x%x].", module.c_str(), route.c_str(), rc);
      return rc;
    }
#ifdef MGT_ACCESS
    /* Send bind data to the server */
    ClIocAddressT allNodeReps;
    if (bindType == CL_NETCONF_BIND_TYPE)
    {
      ClMgtMessageBindTypeT bindData = { { 0 } };
      strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
      strncat(bindData.route, route.c_str(), MGT_MAX_ATTR_STR_LEN - 1);

      allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
      allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_NETCONF_PORT;
      rc = MgtRoot::sendMsg(allNodeReps,(void *)&bindData,sizeof(bindData),MgtMsgType::CL_MGT_MSG_BIND);
    }
    else
    {
      ClMgtMsgOidBindTypeT bindData = { { 0 } };
      strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
      strncat(bindData.oid, route.c_str(), CL_MAX_NAME_LENGTH - 1);

      allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
      allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_SNMP_PORT;
      rc = MgtRoot::sendMsg(allNodeReps,(void *)&bindData,sizeof(bindData),MgtMsgType::CL_MGT_MSG_OID_BIND);
    }
#endif
    return rc;
  }
#ifdef MGT_ACCESS
  ClRcT MgtRoot::sendMsg(ClIocAddressT dest, void* payload, uint payloadlen, MgtMsgType msgtype)
  {
    ClRcT rc = CL_OK;
    uint msgLen = payloadlen + sizeof(MgtMsgProto) - 1;
    char msg[msgLen];
    MgtMsgProto *msgSending = (MgtMsgProto *)msg;
    msgSending->messageType = msgtype;
    memcpy(msgSending->data,payload,payloadlen);
    try
    {
      SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
      mgtIocInstance->SendMsg(dest, (void *)msgSending, msgLen, SAFplusI::CL_MGT_MSG_TYPE);
    }
    catch (SAFplus::Error &ex)
    {
      rc = ex.clError;
      logDebug("GMS","MSG","Failed to send");
    }
    return rc;
  }
#endif

  ClRcT MgtRoot::registerRpc(const std::string module,const std::string rpcName)
  {
    ClRcT rc = CL_OK;
#ifdef MGT_ACCESS
    ClMgtMessageBindTypeT bindData = { { 0 } };
    strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
    strncat(bindData.route, rpcName.c_str(), MGT_MAX_ATTR_STR_LEN - 1);

    /* Send bind data to the server */
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_NETCONF_PORT;
    rc = MgtRoot::sendMsg(allNodeReps,(void *)&bindData,sizeof(bindData),MgtMsgType::CL_MGT_MSG_BIND_RPC);
#endif
    return rc;
  }

#ifdef MGT_ACCESS
  void MgtRoot::clMgtMsgEditHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg)
  {
    ClRcT rc = CL_OK;

    logDebug("NETCONF", "COMP", "Receive Edit MSG");

    ClMgtMessageEditTypeT *editData = (ClMgtMessageEditTypeT *) pInMsg;
    ClCharT *data = editData->data;
    ClUint32T dataSize = strlen(data);

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(editData->module);
    if (!module)
      return;

    MgtObject * object = module->getMgtObject(editData->route);
    if (!object)
      return;

    SAFplus::Transaction t;

    if (object->set(data, dataSize, t) == CL_TRUE)
    {
      t.commit();
    }
    else
    {
      t.abort();
      rc = CL_ERR_INVALID_PARAMETER;
    }
#if 0
    // Send response
    *outMsgSize = sizeof(ClRcT);
    *ppOutMsg = malloc(*outMsgSize);
    memcpy(*ppOutMsg, &rc, *outMsgSize);
#endif
  }

  void MgtRoot::clMgtMsgGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg)
  {
    ClMgtMessageBindTypeT *bindData = (ClMgtMessageBindTypeT *) pInMsg;
    logDebug("NETCONF", "COMP",
               "Received Get request from [%d.%x] for module [%s] route [%s]",
               bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
               bindData->module, bindData->route);

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData->module);
    if (!module)
    {
      logError("NETCONF", "COMP",
                 "Received Get request from [%d.%x] for Non-existent module [%s] route [%s]",
                 bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
                 bindData->module, bindData->route);
      return;
    }

    MgtObject * object = module->getMgtObject(bindData->route);
    if (!object)
    {
      logError("NETCONF", "COMP",
                 "Received Get request from [%d.%x] for Non-existent route [%s] module [%s]",
                 bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
                 bindData->route, bindData->module);
      return;
    }
#if 0
    //send response
    object->get(ppOutMsg, outMsgSize);
#endif
  }

  void MgtRoot::clMgtMsgRpcHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg)
  {
    logDebug("NETCONF", "COMP", "Receive Rpc MSG");

    ClMgtMessageRpcTypeT *rpcData = (ClMgtMessageRpcTypeT *) pInMsg;
    ClCharT *data = rpcData->data;
    ClUint32T dataSize = strlen(data);
#if 0
    if (*ppOutMsg == NULL)
    {
      *ppOutMsg = (void *) calloc(MGT_MAX_DATA_LEN, sizeof(char));
    }
#endif
    MgtModule * module = MgtRoot::getInstance()->getMgtModule(rpcData->module);
    if (!module)
    {
#if 0
      snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
      *outMsgSize = strlen((char*) *ppOutMsg) + 1;
#endif
      return;
    }

    MgtRpc * rpc = module->getMgtRpc(rpcData->rpc);
    if (!rpc)
    {
#if 0
      snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
#endif
      goto out;
    }

    if (rpc->setInParams(data, dataSize) == CL_FALSE)
    {
#if 0
      snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
#endif
      goto out;
    }

    switch (rpcData->rpcType)
    {
      case CL_MGT_RPC_VALIDATE:
      {
#if 0
        if (rpc->validate())
        {
          strcpy((char*) *ppOutMsg, "<ok/>");
        }
        else
        {
          logDebug("MGT", "INI", "Validation error : %s",rpc->ErrorMsg.c_str());
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
#endif
        break;
      }
      case CL_MGT_RPC_INVOKE:
      {
#if 0
        if (rpc->invoke())
        {
          rpc->getOutParams(ppOutMsg, outMsgSize);
        }
        else
        {
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
#endif
        break;
      }
      case CL_MGT_RPC_POSTREPLY:
      {
#if 0
        if (rpc->postReply())
        {
          strcpy((char*) *ppOutMsg, "<ok/>");
        }
        else
        {
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
#endif
        break;
      }
      default:
      {
#if 0
        snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid operation</error>");
#endif
        break;
      }
    }
out:
  return;
#if 0
    *outMsgSize = strlen((char*) *ppOutMsg) + 1;
#endif

  }

  void clMgtMsgOidSetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg)
  {
    ClRcT rc = CL_OK;

    ClMgtMsgOidSetType *editData = (ClMgtMsgOidSetType *) pInMsg;

    logDebug("SNM", "COMP", "Receive 'edit' opcode, data [%s]",
               (ClCharT *)editData->data);

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(editData->module);

    if (!module)
      return;

    MgtObject * object = module->getMgtObject(editData->oid);
    if (!object)
      return;

    SAFplus::Transaction t;

    string strInMsg((ClCharT *)editData->data);

    if (strInMsg.compare(1, object->name.length(), object->name))
    {
      strInMsg = "<" + object->name + ">" + strInMsg + "</" + object->name
        + ">";
    }

    if (object->set((void *) strInMsg.c_str(), strInMsg.length(), t))
    {
      t.commit();
    }
    else
    {
      t.abort();
      rc = CL_ERR_INVALID_PARAMETER;
    }
#if 0
    //Send response
    *outMsgSize = sizeof(ClRcT);
    *ppOutMsg = malloc(*outMsgSize);
    memcpy(*ppOutMsg, &rc, *outMsgSize);
#endif
  }

  void clMgtMsgOidGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg)
  {
    logDebug("SNM", "COMP", "Receive 'get' opcode");

    ClMgtMsgOidBindTypeT *bindData = (ClMgtMsgOidBindTypeT *) pInMsg;

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData->module);
    if (!module)
      return;

    MgtObject * object = module->getMgtObject(bindData->oid);
    if (!object)
      return;
#if 0
    object->get(ppOutMsg, outMsgSize);
#endif
  }

  MgtRoot::MgtMessageHandler::MgtMessageHandler(MgtRoot *mroot):mRoot(mroot)
  {

  }
  void MgtRoot::MgtMessageHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
    MgtMsgProto *rxMsg = (MgtMsgProto *)msg;
    if(rxMsg == NULL)
    {
      logError("MGT","MSG","Received NULL message. Ignored");
      return;
    }
    switch(rxMsg->messageType)
    {
      case MgtMsgType::CL_MGT_MSG_EDIT:
        mRoot->clMgtMsgEditHandle(from.iocPhyAddress,rxMsg->data);
        break;
      case MgtMsgType::CL_MGT_MSG_GET:
        mRoot->clMgtMsgGetHandle(from.iocPhyAddress,rxMsg->data);
        break;
      case MgtMsgType::CL_MGT_MSG_OID_GET:
        mRoot->clMgtMsgOidGetHandle(from.iocPhyAddress,rxMsg->data);
        break;
      case MgtMsgType::CL_MGT_MSG_OID_SET:
        mRoot->clMgtMsgOidSetHandle(from.iocPhyAddress,rxMsg->data);
        break;
      case MgtMsgType::CL_MGT_MSG_RPC:
        mRoot->clMgtMsgRpcHandle(from.iocPhyAddress,rxMsg->data);
        break;
      default:
        break;
    }
  }
#endif
};
