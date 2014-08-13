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
#include <clMgtObject.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCommon.hxx>

#include "MgtMsg.pb.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;
using namespace Mgt::Msg;

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
#ifdef MGT_ACCESS
  MgtRoot::MgtRoot():mgtMessageHandler()
  {
    /*
     * Message server to communicate with snmp/netconf
     */
    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    mgtMessageHandler.init(this);
    mgtIocInstance->RegisterHandler(SAFplusI::CL_MGT_MSG_TYPE, &mgtMessageHandler, NULL);
  }
#else
  MgtRoot::MgtRoot()
  {

  }
#endif
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

  ClRcT MgtRoot::bindMgtObject(Handle handle, MgtObject *object,const std::string module, const std::string route)
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
    string strBind;
    MsgBind bindData;
    MsgMgt mgtMsgReq;
    Mgt::Msg::Handle *hdl = bindData.mutable_handle();

    hdl->set_id0(handle.id[0]);
    hdl->set_id1(handle.id[1]);

    bindData.set_module(module);
    bindData.set_route(route);

    bindData.SerializeToString(&strBind);

    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND);
    mgtMsgReq.set_bind(strBind);

    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS; //GAS: using broadcast CL_IOC_BROADCAST_ADDRESS = 0xffffffff
    allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;

    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;

    try
    {
      string output;
      mgtMsgReq.SerializeToString(&output);
      int size = output.size();
      mgtIocInstance->SendMsg(allNodeReps, (void *)output.c_str(), size, SAFplusI::CL_MGT_MSG_TYPE);
    }
    catch (Error &e)
    {
      assert(0);
    }

#endif

    return rc;
  }

#ifdef MGT_ACCESS
  ClRcT MgtRoot::sendReplyMsg(ClIocAddressT dest, void* payload, uint payloadlen)
  {
    ClRcT rc = CL_OK;
    try
    {
      SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
      mgtIocInstance->SendMsg(dest, (void *)payload, payloadlen, CL_IOC_SAF_MSG_REPLY_PROTO);
    }
    catch (SAFplus::Error &ex)
    {
      rc = ex.clError;
      logDebug("GMS","MSG","Failed to send reply");
    }
    return rc;
  }
#endif

  ClRcT MgtRoot::registerRpc(SAFplus::Handle handle,const std::string module,const std::string rpcName)
  {
    ClRcT rc = CL_OK;
#ifdef MGT_ACCESS
    Mgt::Msg::MsgBind bindData;
    MsgMgt mgtMsgReq;
    string strBind;


    Mgt::Msg::Handle *hdl = bindData.mutable_handle();
    hdl->set_id0(handle.id[0]);
    hdl->set_id1(handle.id[1]);
    bindData.set_module(module.c_str(),CL_MAX_NAME_LENGTH - 1);
    bindData.set_route(rpcName.c_str(),MGT_MAX_ATTR_STR_LEN - 1);
    bindData.SerializeToString(&strBind);
    /* Send bind data to the server */
    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND);
    mgtMsgReq.set_bind(strBind);

    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = 0x1;//GAS: using broadcast CL_IOC_BROADCAST_ADDRESS = 0xffffffff
    allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;
    try
    {
      string output;
      SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
      mgtMsgReq.SerializeToString(&output);
      int size = output.size();
      mgtIocInstance->SendMsg(allNodeReps, (void *)output.c_str(), size, SAFplusI::CL_MGT_MSG_TYPE);
    }
    catch (SAFplus::Error &ex)
    {
      rc = ex.clError;
      logDebug("ROOT","RPC","Failed to send rpc registration");
    }
#endif
    return rc;
  }

#ifdef MGT_ACCESS
  void MgtRoot::clMgtMsgEditHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
    ClRcT rc = CL_OK;
    SAFplus::Transaction t;

    Mgt::Msg::MsgBind bindData;
    bindData.ParseFromString(reqMsg.bind());
    if(reqMsg.data_size() <= 0)
    {
      logError("NETCONF","COMP","Received empty set data");
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }
    Mgt::Msg::MsgSetGet setData;
    setData.ParseFromString(reqMsg.data(0));

    logDebug("NETCONF","COMP","Received setData [%s]",setData.data().c_str());
    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
    {
      logDebug("NETCONF","COMP","Can't found module %s",bindData.module().c_str());
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }

    MgtObject * object = module->getMgtObject(bindData.route().c_str());
    if (!object)
    {
      logDebug("NETCONF","COMP","Can't found object %s",bindData.route().c_str());
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }
    string strInMsg((ClCharT *)setData.data().c_str());
    // Add root element, it should be similar to netconf message
    // OID: <adminStatus>true</adminStatus>
    // NETCONF: <interfaces><adminStatus>true</adminStatus><ename>eth0</ename></interface>
    if (setData.data().compare(1, object->name.length(), object->name))
    {
      setData.set_data("<" + object->name + ">" + strInMsg + "</" + object->name  + ">");
    }
    if (object->set((void *)setData.data().c_str(),setData.data().size(), t) == CL_TRUE)
    {
      t.commit();
    }
    else
    {
      t.abort();
      rc = CL_ERR_INVALID_PARAMETER;
    }

    MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
  }

  void MgtRoot::clMgtMsgGetHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
    Mgt::Msg::MsgBind bindData;
    bindData.ParseFromString(reqMsg.bind());

    logDebug("NETCONF", "COMP",
               "Received Get request from [%d.%x] for module [%s] route [%s]",
               srcAddr.iocPhyAddress.nodeAddress, srcAddr.iocPhyAddress.portId,
               bindData.module().c_str(), bindData.route().c_str());
    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
    {
      logError("NETCONF", "COMP",
                 "Received Get request from [%d.%x] for Non-existent module [%s] route [%s]",
                 srcAddr.iocPhyAddress.nodeAddress, srcAddr.iocPhyAddress.portId,
                 bindData.module().c_str(), bindData.route().c_str());
      return;
    }

    MgtObject * object = module->getMgtObject(bindData.route());
    if (!object)
    {
      logError("NETCONF", "COMP",
                 "Received Get request from [%d.%x] for Non-existent route [%s] module [%s]",
                 srcAddr.iocPhyAddress.nodeAddress, srcAddr.iocPhyAddress.portId,
                 bindData.route().c_str(), bindData.module().c_str());
      return;
    }
    logDebug("NETCONF","COMP","Found object %s",object->name.c_str());
    string outBuff;
    ClUint64T outMsgSize = 0;
    object->get(&outBuff, &outMsgSize);
    // Send response
    logDebug("NETCONF","COMP","Send reply [data=%s; size=%d] for get request",(char *)outBuff.c_str(),outMsgSize);
    MgtRoot::sendReplyMsg(srcAddr,(void *)outBuff.c_str(),outMsgSize);
  }

  void MgtRoot::clMgtMsgRpcHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
#if 0
    Mgt::Msg::MsgBind bindData;
    bindData.ParseFromString(reqMsg.bind());
    Mgt::Msg::MsgRpc rpcData;
    rpcData.ParseFromString(reqMsg.data(0));

    void *ppOutMsg = (void *) rpcData.mutable_data()->c_str();
    ClUint64T outMsgSize = 0;

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
    {
      snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
      outMsgSize = strlen((char*) ppOutMsg) + 1; //add 1 for null byte
      MgtRoot::sendReplyMsg(srcAddr,(void *)ppOutMsg,outMsgSize);
      free(ppOutMsg);
      return;
    }

    MgtRpc * rpc = module->getMgtRpc(bindData.route());
    if (!rpc)
    {
      snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
      outMsgSize = strlen((char*) ppOutMsg) + 1; //add 1 for null byte
      MgtRoot::sendReplyMsg(srcAddr,(void *)ppOutMsg,outMsgSize);
      free(ppOutMsg);
      return;
    }

    if (rpc->setInParams((void *)rpcData.data().c_str(), rpcData.data().size()) == CL_FALSE)
    {
      snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid parameter</error>");
      outMsgSize = strlen((char*) ppOutMsg) + 1; //add 1 for null byte
      MgtRoot::sendReplyMsg(srcAddr,(void *)ppOutMsg,outMsgSize);
      free(ppOutMsg);
      return;
    }

    switch (rpcData.rpctype())
    {
      case Mgt::Msg::MsgRpc::CL_MGT_RPC_VALIDATE:
      {
        if (rpc->validate())
        {
          strcpy((char*) ppOutMsg, "<ok/>");
        }
        else
        {
          logDebug("MGT", "INI", "Validation error : %s",rpc->ErrorMsg.c_str());
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
      }
      case Mgt::Msg::MsgRpc::CL_MGT_RPC_INVOKE:
      {
        if (rpc->invoke())
        {
          rpc->getOutParams((void **)&ppOutMsg, &outMsgSize);
        }
        else
        {
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
      }
      case Mgt::Msg::MsgRpc::CL_MGT_RPC_POSTREPLY:
      {
        if (rpc->postReply())
        {
          strcpy((char*) ppOutMsg, "<ok/>");
        }
        else
        {
          if (rpc->ErrorMsg.length() == 0)
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Unknown error</error>");
          else
            snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
      }
      default:
      {
        snprintf((char*) ppOutMsg, MGT_MAX_DATA_LEN,"<error>Invalid operation</error>");
        break;
      }
    }
out:
    outMsgSize = strlen((char*) ppOutMsg) + 1; //add 1 for null byte
    MgtRoot::sendReplyMsg(srcAddr,(void *)ppOutMsg,outMsgSize);
#endif
  }

  void MgtRoot::clMgtMsgOidSetHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
    ClRcT rc = CL_OK;
    Mgt::Msg::MsgBind bindData;
    bindData.ParseFromString(reqMsg.bind());
    if(reqMsg.data_size() <= 0)
    {
      logError("SNM","COMP","Data is empty");
      return;
    }
    Mgt::Msg::MsgSetGet setData;
    setData.ParseFromString(reqMsg.data(0));

    logDebug("SNM", "COMP", "Receive 'edit' opcode for oid [%s] data [%s]",bindData.route().c_str(), setData.data().c_str());

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());

    if (!module)
    {
      logDebug("SNM","COMP","Module %s not found",bindData.module().c_str());
      return;
    }

    MgtObject * object = module->getMgtObject(bindData.route());
    if (!object)
    {
      logDebug("SNM","COMP","Object %s not found",bindData.route().c_str());
      return;
    }


    SAFplus::Transaction t;

    string strInMsg((ClCharT *)setData.data().c_str());

    // Add root element, it should be similar to netconf message
    // OID: <adminStatus>true</adminStatus>
    // NETCONF: <interfaces><adminStatus>true</adminStatus><ename>eth0</ename></interface>
    if (setData.data().compare(1, object->name.length(), object->name))
    {
      setData.set_data("<" + object->name + ">" + strInMsg + "</" + object->name  + ">");
    }
    logDebug("SNM","COMP","New data: %s",setData.data().c_str());

    if (object->set((void *) setData.data().c_str(), setData.data().size(), t))
    {
      t.commit();
    }
    else
    {
      t.abort();
      rc = CL_ERR_INVALID_PARAMETER;
    }
    MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
  }

  void MgtRoot::clMgtMsgOidGetHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
     logDebug("SNM", "COMP", "Receive 'get' opcode");
     Mgt::Msg::MsgBind bindData;
     bindData.ParseFromString(reqMsg.bind());

    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
      return;

    MgtObject * object = module->getMgtObject(bindData.route());
    if (!object)
      return;
    logDebug("SNM","COMP","Found object %s ",object->name.c_str());
    string ppOutMsg;
    ClUint64T outMsgSize = 0;
    object->get(&ppOutMsg, &outMsgSize);
    logDebug("SNM","COMP","Send data %s size: %d",ppOutMsg.c_str(),outMsgSize);
    // Send action response
    MgtRoot::sendReplyMsg(srcAddr,(void *)ppOutMsg.c_str(),outMsgSize);
  }

  MgtRoot::MgtMessageHandler::MgtMessageHandler()
  {

  }
  void MgtRoot::MgtMessageHandler::init(MgtRoot *mroot)
  {
    mRoot = mroot;
  }
  void MgtRoot::MgtMessageHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
    Mgt::Msg::MsgMgt mgtMsgReq;
    mgtMsgReq.ParseFromArray(msg, msglen);
    Mgt::Msg::MsgBind bindData;
    bindData.ParseFromString(mgtMsgReq.bind());
    const char *route = bindData.route().c_str();
    switch(mgtMsgReq.type())
    {
      case Mgt::Msg::MsgMgt::CL_MGT_MSG_SET:
        if(route[0] == '/') //netconf
        {
          logDebug("MGT","MSG","Received NETCONF setting request");
          mRoot->clMgtMsgEditHandle(from,mgtMsgReq);
        }
        else //snmp
        {
          logDebug("MGT","MSG","Received SNMP setting request");
          mRoot->clMgtMsgOidSetHandle(from,mgtMsgReq);
        }
        break;
      case Mgt::Msg::MsgMgt::CL_MGT_MSG_GET:
        if(route[0] == '/')
        {
          logDebug("MGT","MSG","Received NETCONF getting request");
          mRoot->clMgtMsgGetHandle(from,mgtMsgReq);
        }
        else
        {
          logDebug("MGT","MSG","Received SNMP getting request");
          mRoot->clMgtMsgOidGetHandle(from,mgtMsgReq);
        }
        break;
      case Mgt::Msg::MsgMgt::CL_MGT_MSG_RPC:
        logDebug("MGT","MSG","Received NETCONF rpc request");
        mRoot->clMgtMsgRpcHandle(from,mgtMsgReq);
        break;
      default:
        break;
    }
  }
#endif
};
