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

#include "MgtMsg.pb.hxx"

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
  MgtRoot::MgtRoot():mgtMessageHandler()
  {
    /*
     * Message server to communicate with snmp/netconf
     */
    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    mgtMessageHandler.init(this);
    mgtIocInstance->RegisterHandler(SAFplusI::CL_MGT_MSG_TYPE, &mgtMessageHandler, NULL);
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

    return rc;
  }

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

  ClRcT MgtRoot::registerRpc(SAFplus::Handle handle,const std::string module,const std::string rpcName)
  {
    ClRcT rc = CL_OK;
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
    return rc;
  }

  void MgtRoot::clMgtMsgEditHandle(ClIocAddress srcAddr, Mgt::Msg::MsgMgt reqMsg)
  {
    ClRcT rc = CL_OK;
    ClBoolT rc1 = CL_FALSE;
    SAFplus::Transaction t;

    Mgt::Msg::MsgBind bindData;
    Mgt::Msg::MsgSetGet setData;
    bindData.ParseFromString(reqMsg.bind());

    logDebug("MGT","SET","Received setting message for module %s and route %s",bindData.module().c_str(),bindData.route().c_str());
    if(reqMsg.data_size() <= 0)
    {
      logError("MGT","SET","Received empty set data");
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }
    setData.ParseFromString(reqMsg.data(0));
    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
    {
      logDebug("MGT","SET","Can't found module %s",bindData.module().c_str());
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }

    MgtObject * object = module->getMgtObject(bindData.route().c_str());
    if (!object)
    {
      logDebug("MGT","SET","Can't found object %s",bindData.route().c_str());
      rc = CL_ERR_INVALID_PARAMETER;
      MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
      return;
    }
    string strInMsg((ClCharT *)setData.data().c_str());
    // Add root element, it should be similar to netconf message
    // OID: <adminStatus>true</adminStatus>
    // NETCONF: <interfaces><adminStatus>true</adminStatus><ename>eth0</ename></interface>
    if (setData.data().compare(1, object->tag.length(), object->tag))
    {
      setData.set_data("<" + object->tag + ">" + strInMsg + "</" + object->tag  + ">");
    }
    rc1 = object->set((void *)setData.data().c_str(),setData.data().size(), t);
    if (rc1 == CL_TRUE)
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
    MsgGeneral rplMesg;
    string outBuff,strRplMesg;
    ClUint64T outMsgSize = 0;
    Mgt::Msg::MsgBind bindData;

    bindData.ParseFromString(reqMsg.bind());
    logDebug("MGT","GET","Received getting message for module %s and route %s",bindData.module().c_str(),bindData.route().c_str());
    MgtModule * module = MgtRoot::getInstance()->getMgtModule(bindData.module());
    if (!module)
    {
      logError("MGT", "GET",
                 "Received getting request from [%d.%x] for Non-existent module [%s] route [%s]",
                 srcAddr.iocPhyAddress.nodeAddress, srcAddr.iocPhyAddress.portId,
                 bindData.module().c_str(), bindData.route().c_str());
      return;
    }

    MgtObject * object = module->getMgtObject(bindData.route());
    if (!object)
    {
      logError("MGT", "GET",
                 "Received getting request from [%d.%x] for Non-existent route [%s] module [%s]",
                 srcAddr.iocPhyAddress.nodeAddress, srcAddr.iocPhyAddress.portId,
                 bindData.route().c_str(), bindData.module().c_str());
      return;
    }
    /* Improvement: Compare revision to limit data sending */
    if(reqMsg.data_size() > 0)
    {
      std::string strRev = reqMsg.data(0);
      ClUint32T rxrev = std::stoi(strRev);
      logDebug("MGT","GET","Checking if data has updated [%x-%x]",rxrev,object->root()->headRev);
      if(rxrev >= object->root()->headRev && object->root()->headRev !=0 )
      {
        rplMesg.add_data(strRev);
        rplMesg.SerializeToString(&strRplMesg);
        MgtRoot::sendReplyMsg(srcAddr,(void *)strRplMesg.c_str(),strRplMesg.size());
        return;
      }
      // If revision of that route on mgt server is out of date, also send lastest data
    }
    std::string strRev = std::to_string(object->root()->headRev);
    object->get(&outBuff, &outMsgSize);
    rplMesg.add_data(strRev);
    rplMesg.add_data(outBuff.c_str(),outMsgSize);
    rplMesg.SerializeToString(&strRplMesg);
    MgtRoot::sendReplyMsg(srcAddr,(void *)strRplMesg.c_str(),strRplMesg.size());
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
    switch(mgtMsgReq.type())
    {
      case Mgt::Msg::MsgMgt::CL_MGT_MSG_SET:
        mRoot->clMgtMsgEditHandle(from,mgtMsgReq);
        break;
      case Mgt::Msg::MsgMgt::CL_MGT_MSG_GET:
        mRoot->clMgtMsgGetHandle(from,mgtMsgReq);
        break;
      default:
        break;
    }
  }
};
