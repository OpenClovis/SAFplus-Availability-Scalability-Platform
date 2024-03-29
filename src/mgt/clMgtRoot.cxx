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
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCommon.hxx>
#include <inttypes.h>
#include <csignal>
#include <boost/algorithm/string.hpp>

#include "MgtMsg.pb.hxx"

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors6.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;
using namespace Mgt::Msg;

namespace SAFplus
{
  extern Checkpoint* mgtCheckpoint;
  MgtRoot *MgtRoot::singletonInstance = 0;

  MgtRoot *MgtRoot::getInstance()
  {
    return (singletonInstance ? singletonInstance : (singletonInstance = new MgtRoot()));
  }

  void MgtRoot::DestroyInstance()
  {
    delete singletonInstance;
    singletonInstance = 0;
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
    mgtIocInstance->RegisterHandler(SAFplusI::CL_MGT_MSG_TYPE, &mgtMessageHandler, nullptr);
    // Init Mgt Checkpoint
    mgtAccessInitialize();
  }

  void MgtRoot::bind(Handle handle,MgtObject* obj)
  {
    addChildObject(obj,obj->tag); 
    std::string path = obj->getFullXpath();
    // Add to Mgt Checkpoint
    try
    {
      // for key
      //std::string strXPath = "/";
      //strXPath.append(path);

      // for data
      char handleData[sizeof(Buffer) - 1 + sizeof(Handle)];
      Buffer* val = new (handleData) Buffer(sizeof(Handle));
      *((Handle*) val->data) = handle;

      // Add to checkpoint
      mgtCheckpoint->write(path.c_str(), *val);
    }
    catch (Error &e)
      {
        assert(0);
      }
  }

  void MgtRoot::registerRpc(Handle handle,MgtRpc* obj)
  {
    addChildObject(obj, obj->tag);
    std::string path = obj->getFullXpath();
    try
    {
      char handleData[sizeof(Buffer) - 1 + sizeof(Handle)];
      Buffer* val = new (handleData) Buffer(sizeof(Handle));
      *((Handle*) val->data) = handle;
      // Add to checkpoint
      mgtCheckpoint->write(path.c_str(), *val);
    }
    catch (Error &e)
      {
        assert(0);
      }
    logInfo("MGT", "LOAD", "Bound xpath [%s] to local object", path.c_str());
  }

  void dbgDumpMgtCheckpoint()
  {
    if (!mgtCheckpoint) 
      {
        printf("management checkpoint not available\n");
        return;
      }
    for (Checkpoint::Iterator i=mgtCheckpoint->begin();i!=mgtCheckpoint->end();i++)
      {
        SAFplus::Checkpoint::KeyValuePair& item = *i;
        char* key = ((char*) (*item.first).data);
        SAFplus::Handle hdl = *((const SAFplus::Handle*) (*item.second).data);
        printf("key [%s], value [%" PRIx64 ",%" PRIx64 ", cluster: %d node: %d port: %d obj: %" PRIx64 "] , change [%d]\n",key, hdl.id[0],hdl.id[1], hdl.getCluster(),hdl.getNode(),hdl.getPort(),hdl.getIndex(), item.second->changeNum());
      }
  }

  ClRcT MgtRoot::loadMgtModule(Handle handle, MgtModule *module, std::string moduleName)
  {
    assert(0);
#if 0
    if (module == nullptr)
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

    // Add to Mgt Checkpoint
    try
    {
      // for key
      std::string strXPath = "/";
      strXPath.append(moduleName);

      // for data
      char handleData[sizeof(Buffer) - 1 + sizeof(Handle)];
      Buffer* val = new (handleData) Buffer(sizeof(Handle));
      *((Handle*) val->data) = handle;

      // Add to checkpoint
      mgtCheckpoint->write(strXPath.c_str(), *val);
    }
    catch (Error &e)
    {
      assert(0);
    }
#endif
    return CL_OK;
  }

  ClRcT MgtRoot::unloadMgtModule(std::string moduleName)
  {
    assert(0);
#if 0
    /* Check if MGT module already exists in the database */
    if (mMgtModules.find(moduleName) == mMgtModules.end())
    {
      logDebug("MGT", "LOAD", "Module [%s] is not existing!", moduleName.c_str());
      return CL_ERR_NOT_EXIST;
    }

    /* Remove MGT module out off the database */
    mMgtModules.erase(moduleName);
    logDebug("MGT", "LOAD", "Module [%s] removed successful!",moduleName.c_str());
#endif
    return CL_OK;
  }

  MgtModule *MgtRoot::getMgtModule(const std::string moduleName)
  {
    assert(0);
#if 0
    map<string, MgtModule*>::iterator mapIndex = mMgtModules.find(moduleName);
    if (mapIndex != mMgtModules.end())
    {
      return static_cast<MgtModule *>((*mapIndex).second);
    }
#endif
    return nullptr;
  }

  ClRcT MgtRoot::bindMgtObject(Handle handle, MgtObject *object,const std::string& module, const std::string& route)
  {
    assert(0);
#if 0
    ClRcT rc = CL_OK;

    if (object == nullptr)
    {
      return CL_ERR_NULL_POINTER;
    }

    /* Check if MGT module already exists in the database */
    MgtModule *mgtModule = getMgtModule(module);
    if (mgtModule == nullptr)
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

#if 0 // Is this necessary now that we use the checkpoint?
    /* Send bind data to the server */
    //ClIocAddressT allNodeReps;
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

    //allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS; //GAS: using broadcast CL_IOC_BROADCAST_ADDRESS = 0xffffffff
    //allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;

    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;

    try
    {
      string output;
      mgtMsgReq.SerializeToString(&output);
      int size = output.size();
      mgtIocInstance->SendMsg(getProcessHandle(SAFplusI::MGT_IOC_PORT,Handle::AllNodes), (void *)output.c_str(), size, SAFplusI::CL_MGT_MSG_TYPE);
    }
    catch (Error &e)
    {
      assert(0);
    }
#endif

    try
      {
// Add to Mgt Checkpoint
      // for key
      std::string strXPath = "/";
      strXPath.append(module);
      if (!route.empty())
        {
        strXPath.append("/");
        strXPath.append(route);
        }

      // for data
      char handleData[sizeof(Buffer)-1+sizeof(Handle)];
      Buffer* val = new(handleData) Buffer(sizeof(Handle));
      *((Handle*)val->data) = handle;
      // Add to checkpoint
      mgtCheckpoint->write(strXPath.c_str(), *val);
// end
    }
    catch (Error &e)
    {
      assert(0);
    }

    return rc;
#endif
  }

  ClRcT MgtRoot::sendReplyMsg(Handle dest, void* payload, uint payloadlen)
  {
    ClRcT rc = CL_OK;
    try
    {
      SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
      logDebug("MGT","MSG","Sent reply length [%d]", payloadlen);
      mgtIocInstance->SendMsg(dest, (void *)payload, payloadlen, SAFplusI::CL_IOC_SAF_MSG_REPLY_PROTO);
    }
    catch (SAFplus::Error &ex)
    {
      rc = ex.clError;
      logDebug("MGT","MSG","Failed to send reply");
    }
    return rc;
  }

//  ClRcT MgtRoot::registerRpc(SAFplus::Handle handle,const std::string module,const std::string rpcName)
//  {
//    ClRcT rc = CL_OK;
//#if 0 //TODO: stored this handle into checkpoint
//    Mgt::Msg::MsgBind bindData;
//    MsgMgt mgtMsgReq;
//    string strBind;
//
//
//    Mgt::Msg::Handle *hdl = bindData.mutable_handle();
//    hdl->set_id0(handle.id[0]);
//    hdl->set_id1(handle.id[1]);
//    bindData.set_module(module.c_str(),CL_MAX_NAME_LENGTH - 1);
//    bindData.set_route(rpcName.c_str(),MGT_MAX_ATTR_STR_LEN - 1);
//    bindData.SerializeToString(&strBind);
//    /* Send bind data to the server */
//    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND);
//    mgtMsgReq.set_bind(strBind);
//
//    // TODO: Who do I send this message to?  I think it should go to the active Mgt server?  Or broadcast to both Mgt servers?
//    //ClIocAddressT allNodeReps;
//    //allNodeReps.iocPhyAddress.nodeAddress = 0x1;//GAS: using broadcast CL_IOC_BROADCAST_ADDRESS = 0xffffffff
//    //allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;
//    try
//    {
//      string output;
//      SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
//      mgtMsgReq.SerializeToString(&output);
//      int size = output.size();
//      mgtIocInstance->SendMsg(getProcessHandle(SAFplusI::MGT_IOC_PORT,Handle::AllNodes), (void *)output.c_str(), size, SAFplusI::CL_MGT_MSG_TYPE);
//    }
//    catch (SAFplus::Error &ex)
//    {
//      rc = ex.clError;
//      logDebug("ROOT","RPC","Failed to send rpc registration");
//    }
//#endif
//    return rc;
//  }

#if 0
  void MgtRoot::resolvePath(const char* path, std::vector<MgtObject*>* result)
  {
    size_t idx;
    
    std::string xpath(path);
    idx = xpath.find("/");


    std::string moduleName = xpath.substr(0, idx);

    MgtModule * module = getMgtModule(moduleName);
    if (module)
      {
      if (idx == std::string::npos)
      {
        module->resolvePath("", result);
      }
      else
        module->resolvePath(path+idx+1, result);
      }
  }
#endif

#if 0
  MgtObject *MgtRoot::findMgtObject(const std::string &xpath)
  {
    size_t idx;

    if (xpath[0] != '/')
      {
        // Invalid xpath; we ARE the root so the xpath must start with /
        return nullptr;
      }

    idx = xpath.find("/", 1);

    if (idx == std::string::npos)
      {
        // Invalid xpath
        return nullptr;
      }

    std::string moduleName = xpath.substr(1, idx -1);

    MgtModule * module = getMgtModule(moduleName);

    if (!module)
      {
        // Module not found
        for(map<string, MgtModule*>::iterator iter = mMgtModules.begin(); iter != mMgtModules.end(); iter++)
          {
            module = static_cast<MgtModule *>((*iter).second);
            std::string modXpath;
            modXpath.assign("/");
            modXpath.append((*iter).first);
            modXpath.append(xpath);

            MgtObject *object = module->findMgtObject(modXpath);
            if (object)
              return object;
          }

        return nullptr;
      }

    std::string rest = xpath.substr(idx+1);
    return module->findMgtObject(rest);
  }
#endif

  void MgtRoot::clMgtMsgXGetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {
  std::vector<MgtObject*> matches;
  std::string path = reqMsg.bind();
  std::string strRplMesg;
  MsgGeneral rplMesg;
  std::string cmds;
  unsigned int depth=64;  // To stop accidental infinite loops, let's set the default depth to something large but not crazy
  MgtObject::SerializationOptions seropts =  (MgtObject::SerializationOptions) (MgtObject::SerializeListKeyAttribute | MgtObject::SerializePathAttribute | MgtObject::SerializeOnePath);

  rplMesg.clear_data();

  if (path[0] == '{')  // Debugging requests
    {
      int end = path.find_first_of("}");
      cmds = path.substr(1,end-1);
      vector<string> strs;
      boost::split(strs,cmds,boost::is_any_of(","));
      for (auto cmd: strs)
        {
          if (cmd[0] == 'n')  // NETCONF compatible XML
            {
              seropts = MgtObject::SerializeNoOptions;
            }
          if (cmd[0] == 'b')
            {
              std::raise(SIGINT);
            }
          else if (cmd[0] == 'p')
            {
              clDbgPause();
            }
          else if (cmd[0] == 'd')
            {
              depth = std::stoi(cmd.substr(2)); // format is "d=#" so get the integer at offset 2
            }

        }
      // TODO: parse the non-xml requests (depth, pause thread, break thread, log custom string)
      cmds.append(" ");  // Right now I just assume everything in there is a custom logging string
      path = path.substr(end+1);
    }
  if (path[0] == '/')  // Only absolute paths are allowed over the RPC API since there is no context
    {
      std::string suffixLoc = path.substr(1);
      if (suffixLoc.empty())
        {
          // Get top container/list/leaf
          depth = 1;

          // Retrieve all children of root
          for (MgtObject::Iterator it = begin(); it != end(); ++it)
          {
              matches.push_back(it->second);
          }
        }
      else
        {
          resolvePath(path.c_str()+1, &matches);
        }

      std::stringstream outBuff;
      for (std::vector<MgtObject*>::iterator i=matches.begin(); i != matches.end(); i++)
        {
          MgtObject *object = *i;
          object->toString(outBuff, depth, seropts);
          
        }
      const std::string& s = outBuff.str();
      rplMesg.add_data(s.c_str(), s.length() + 1);
    }

  rplMesg.SerializeToString(&strRplMesg);
  logDebug("MGT","XGET","Replying to request [%s] %sfrom [%" PRIx64 ",%" PRIx64 "] with msg of size [%lu]",path.c_str(),cmds.c_str(),srcAddr.id[0], srcAddr.id[1], (long unsigned int) strRplMesg.size());
  logDebug("MGT","XGET","Contents [%.400s...]",rplMesg.data(0).c_str());

  // TODO: in the case of a broadcast to figure out the owner we don't want to reply, but in the case
  // of an explicit query, we must reply even if 0
  if (strRplMesg.size()>0)
    {
      logDebug("MGT","XGET","Replying to request [%s] %sfrom [%" PRIx64 ",%" PRIx64 "] with msg of size [%lu]",path.c_str(),cmds.c_str(),srcAddr.id[0], srcAddr.id[1], (long unsigned int) strRplMesg.size());
      logDebug("MGT","XGET","Contents [%.400s...]",rplMesg.data(0).c_str());
      MgtRoot::sendReplyMsg(srcAddr,(void *)strRplMesg.c_str(),strRplMesg.size());
    }
  }

  void MgtRoot::clMgtMsgXSetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {
    ClRcT rc = CL_OK;
    std::vector<MgtObject*> matches;
    std::string path = reqMsg.bind();
    std::string cmds;
    std::string value;

    if (path[0] == '{')  // Debugging requests
    {
      int end = path.find_first_of("}");
      cmds = path.substr(1,end-1);
      vector<string> strs;
      boost::split(strs,cmds,boost::is_any_of(","));
      for (auto cmd: strs)
        {
          if (cmd[0] == 'b')
            {
              std::raise(SIGINT);
            }
          else if (cmd[0] == 'p')
            {
              clDbgPause();
            }
        }
      // TODO: parse the non-xml requests (depth, pause thread, break thread, log custom string)
      cmds.append(" ");  // Right now I just assume everything in there is a custom logging string
      path = path.substr(end+1);
    }

    if (path[0] == '/')
      {
        resolvePath(path.c_str() + 1, &matches);
        value = reqMsg.data(0);
        if (matches.size())
          {
            for (std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
              {
                rc |= (*i)->setObj(value);
              }
          }
        else
          {
            rc = CL_ERR_NOT_EXIST;
          }
      }
    logDebug("MGT","SET","Object [%s] update [%s] complete [0x%x]", path.c_str(),value.c_str(), rc);
    MgtRoot::sendReplyMsg(srcAddr,(void *)&rc,sizeof(ClRcT));
  }

  ClRcT MgtRoot::clMgtMsgCreateHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {
    ClRcT rc = CL_OK;
    std::string xpath = reqMsg.bind();
    std::size_t idx = xpath.find_last_of("/");
    std::vector<MgtObject*> matches;
    std::string cmds;

    if (xpath[0] == '{')  // Debugging requests
    {
      int end = xpath.find_first_of("}");
      cmds = xpath.substr(1,end-1);
      vector<string> strs;
      boost::split(strs,cmds,boost::is_any_of(","));
      for (auto cmd: strs)
        {
          if (cmd[0] == 'b')
            {
              std::raise(SIGINT);
            }
          else if (cmd[0] == 'p')
            {
              clDbgPause();
            }
        }
      // TODO: parse the non-xml requests (depth, pause thread, break thread, log custom string)
      cmds.append(" ");  // Right now I just assume everything in there is a custom logging string
      xpath = xpath.substr(end+1);
    }

    if (idx != std::string::npos)
    {
      std::string path = xpath.substr(0, idx);
      std::string value = xpath.substr(idx + 1);

      resolvePath(path.c_str() + 1, &matches);

      if (matches.size())
      {
        for (std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            rc = (*i)->createObj(value);
            logDebug("MGT","CRET","Object [%s] created", xpath.c_str());
            updateReference();  // Since a new object is created, update refs that might be pointed to it
            MgtRoot::sendReplyMsg(srcAddr, (void *) &rc, sizeof(ClRcT));
            /*
             * Not allow multiple objects
             */
            return rc;
          }
      }
      else
        {
          rc = CL_ERR_NOT_EXIST;
        }
    }
    logDebug("MGT","CRET","Creating object [%s] failed, errorCode [0x%x]", xpath.c_str(), rc);
    MgtRoot::sendReplyMsg(srcAddr, (void *) &rc, sizeof(ClRcT));
    return rc;
  }

  void MgtRoot::clMgtMsgRPCHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgRpc& reqMsg)
  {
    std::vector<MgtObject*> matches;
    std::string path, cmds;
    std::string attrs = "";
    ClRcT rcRet = CL_OK;
    std::string data = reqMsg.data();
    path = reqMsg.bind();
    if (path[0] == '{')  // Debugging requests
    {
      int end = path.find_first_of("}");
      cmds = path.substr(1,end-1);
      vector<string> strs;
      boost::split(strs, cmds, boost::is_any_of(","));
      for (auto cmd: strs)
      {
        if (cmd[0] == 'b')
        {
          std::raise(SIGINT);
        }
        else if (cmd[0] == 'p')
        {
          clDbgPause();
        }
      }
      // TODO: parse the non-xml requests (depth, pause thread, break thread, log custom string)
      cmds.append(" ");  // Right now I just assume everything in there is a custom logging string
      path = path.substr(end+1);
    }
    //Todo Remove this hard code
    if (path[0] == '/')
    {
      resolvePath(path.c_str() + 1, &matches);
      if (matches.size())
      {
        for (std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
        {
          MgtRpc *rpc = dynamic_cast<MgtRpc*> (*i);
          if (rpc)
          {
            if(data!="")
            {
              rpc->setInParams((void*)data.c_str(),data.length());
            }
            switch (reqMsg.rpctype())
            {
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_VALIDATE:
                rcRet = rpc->validate();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_INVOKE:
                rcRet = rpc->invoke();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_POSTREPLY:
                rcRet = rpc->postReply();
                break;
              default:
                break;
            }
            if (rcRet==CL_OK)
            {
              std::string output="";
              std::string objxml;
              rpc->getOutParams(&objxml);
              output.append(objxml);
              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d]", output.c_str(),output.size()); 
              return;
            }
            else
            {
              std::string output="false";
              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d]", output.c_str(),output.size()); 
              return;
            }
          }
          else
          {
              std::string output="false";
              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d] ", output.c_str(),output.size()); 
              return;
          }
        }
      }
    }
  }
  void MgtRoot::clMgtMsgActionHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgRpc& reqMsg)
  {
    std::vector<MgtObject*> matches,parentMatches;
    std::string path, cmds;
    std::string attrs = "";
    ClRcT rcRet = CL_OK;
    std::string data = reqMsg.data();
    std::string strObjectPath = reqMsg.objectpath();
    path = reqMsg.bind();
    //Todo Remove this hard code
    if (path[0] == '/')
    {
      resolvePath(path.c_str() + 1, &matches);
      if (matches.size())
      {
        for (std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
        {
          MgtAction *rpc = dynamic_cast<MgtAction*> (*i);
          if (rpc)
          {
            if(data!="")
            {
              rpc->setInParams((void*)data.c_str(),data.length());
            }
            resolvePath(strObjectPath.c_str() + 1, &parentMatches);
            if(parentMatches.size() > 0)
            {
              logDebug("MGT", "REV", "set Parent Object:%s",strObjectPath.c_str());
              rpc->setObjectParent(parentMatches[0]);
            }
            else
            {
              logDebug("MGT", "REV", "path Parent Object %s is NULL",strObjectPath.c_str());
            }
            switch (reqMsg.rpctype())
            {
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_VALIDATE:
                rcRet = rpc->validate();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_INVOKE:
                rcRet = rpc->invoke();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_POSTREPLY:
                rcRet = rpc->postReply();
                break;
              default:
                break;
            }
            if (rcRet==CL_OK)
            {
              std::string output="";
              std::string objxml;
              rpc->getOutParams(&objxml);
              output.append(objxml);
              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d]", output.c_str(),output.size());
              return;
            }
            else
            {
              std::string output="false";

              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d]", output.c_str(),output.size());
              return;
            }
          }
          else
          {
              std::string output="false";
              MgtRoot::sendReplyMsg(srcAddr, (void *) output.c_str(), output.size());
              logDebug("MGT", "REV", "Return value is [%s] , size [%d] ", output.c_str(),output.size());
              return;
          }
        }
      }
    }
  }
  void MgtRoot::clMgtMsgDeleteHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {
    ClRcT rc = CL_OK;
    std::string xpath = reqMsg.bind();
    std::size_t idx = xpath.find_last_of("/");
    std::vector<MgtObject*> matches;
    std::string cmds;

    if (xpath[0] == '{')  // Debugging requests
    {
      int end = xpath.find_first_of("}");
      cmds = xpath.substr(1,end-1);
      vector<string> strs;
      boost::split(strs,cmds,boost::is_any_of(","));
      for (auto cmd: strs)
        {
          if (cmd[0] == 'b')
            {
              std::raise(SIGINT);
            }
          else if (cmd[0] == 'p')
            {
              clDbgPause();
            }
        }
      // TODO: parse the non-xml requests (depth, pause thread, break thread, log custom string)
      cmds.append(" ");  // Right now I just assume everything in there is a custom logging string
      xpath = xpath.substr(end+1);
    }

    if (idx != std::string::npos)
    {
      std::string path = xpath.substr(0, idx);
      std::string value = xpath.substr(idx + 1);

      resolvePath(path.c_str() + 1, &matches);
      if (matches.size())
      {
        for (std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            rc = (*i)->deleteObj(value);
            logDebug("MGT","DEL","Object [%s] got deleted",xpath.c_str());
            MgtRoot::sendReplyMsg(srcAddr, (void *) &rc, sizeof(ClRcT));
            /*
             * Not allow multiple objects
             */
            return;
          }
      }
      else
        {
          rc = CL_ERR_NOT_EXIST;
        }
    }
    logDebug("MGT","DEL","Deleting object [%s] got failure, errorCode [0x%x]", xpath.c_str(), rc);
    MgtRoot::sendReplyMsg(srcAddr, (void *) &rc, sizeof(ClRcT));
  }

  MgtRoot::MgtMessageHandler::MgtMessageHandler()
  {

  }
  void MgtRoot::MgtMessageHandler::init(MgtRoot *mroot)
  {
    mRoot = mroot;
  }
  void MgtRoot::MgtMessageHandler::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
    //Check message is rpc ???
    logDebug("MGT","MSG","receive new  message ");
    Mgt::Msg::MsgMgt mgtMsgReq;
    Mgt::Msg::MsgRpc mgtRpcReq;
    if(mgtMsgReq.ParseFromArray(msg, msglen))
    {
      logDebug("MGT","MSG","process MGT message [%d]",mgtMsgReq.type());
      ClRcT rc = CL_ERR_COMMON_MAX;
      switch(mgtMsgReq.type())
      {
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_XGET:
          mRoot->clMgtMsgXGetHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_XSET:
          mRoot->clMgtMsgXSetHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_CREATE:
          rc = mRoot->clMgtMsgCreateHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_DELETE:
          mRoot->clMgtMsgDeleteHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_REST_PUT:
          mRoot->clMgtMsgRestPutHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_REST_POST:
          mRoot->clMgtMsgRestPostHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_REST_PATCH:
          mRoot->clMgtMsgRestPatchHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_REST_GET:
          mRoot->clMgtMsgRestGetHandler(from,mgtMsgReq);
          break;
        case Mgt::Msg::MsgMgt::CL_MGT_MSG_REST_DELETE:
          mRoot->clMgtMsgRestDeleteHandler(from,mgtMsgReq);
          break;
        default:
          break;
      }
      ClRcT *rcOut = static_cast<ClRcT *>(cookie);
      *rcOut = rc;
    }
    else
    {
      mgtRpcReq.ParseFromArray(msg, msglen);
      if(mgtRpcReq.isrpc())
      {
         logDebug("MGT","MSG","process RPC message");
         mRoot->clMgtMsgRPCHandler(from,mgtRpcReq);
      }
      else
      {
         logDebug("MGT","MSG","process Action message");
         mRoot->clMgtMsgActionHandler(from,mgtRpcReq);
      }
    }
  }

  void MgtRoot::addReference(MgtObject* mgtObject)
  {
    if (std::find(this->mgtReferenceList.begin(),this->mgtReferenceList.end(), mgtObject) == this->mgtReferenceList.end())
      this->mgtReferenceList.push_back(mgtObject);
  }

  void MgtRoot::removeReference(MgtObject* mgtObject)
  {
    // remove all elements in the list whose value is mgtObject
    this->mgtReferenceList.erase(std::remove(this->mgtReferenceList.begin(), this->mgtReferenceList.end(), mgtObject), this->mgtReferenceList.end());
  }

  void MgtRoot::updateReference(void)
  {
    for(std::vector<MgtObject*>::iterator it = this->mgtReferenceList.begin(); it != this->mgtReferenceList.end(); ++it)
      {
        MgtObject* mgtObject = *it;
        printf("updating %s\n", mgtObject->tag.c_str());
        mgtObject->updateReference();
      }
  }

  void MgtRoot::clMgtMsgRestGetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {

  }
  void MgtRoot::clMgtMsgRestDeleteHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {

  }
  void MgtRoot::clMgtMsgRestPutHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {

  }
  void MgtRoot::clMgtMsgRestPatchHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {

  }
  void MgtRoot::clMgtMsgRestPostHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg)
  {

  }


};
