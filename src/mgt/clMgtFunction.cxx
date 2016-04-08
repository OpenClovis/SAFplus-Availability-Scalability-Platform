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
 *
 */
#include <algorithm>
#include <clSafplusMsgServer.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <MgtMsg.pb.hxx>



//TODO Temporary using clCommonErrors6.h
#include <clCommonErrors6.h>

#include <clMgtFunction.hxx>
#include <clMgtRoot.hxx>

using namespace Mgt::Msg;

namespace SAFplus
{
// Mgt Checkpoint
  Checkpoint* mgtCheckpoint = 0;

  Checkpoint* getMgtCheckpoint()
  {
    assert(mgtCheckpoint);  // You should have called mgtAccessInitialize
    return mgtCheckpoint;
  }

  void mgtAccessInitialize(void)
  {
    if (!mgtCheckpoint)
    {
      mgtCheckpoint = new Checkpoint(MGT_CKPT, Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024 * 1024, SAFplusI::CkptDefaultRows);
      mgtCheckpoint->name = "safplusMgt";
    }
  }


  SAFplus::Handle getMgtHandle(const std::string& pathSpec, ClRcT &errCode)
  {
    std::string xpath = pathSpec;  // I need to make a copy so I can modify it
    if (pathSpec[0] == '{')
      {
        xpath = pathSpec.substr(pathSpec.find('}')+1);
      }

    size_t lastSlash = std::string::npos;
    while(1)
      {
        try
          {
            logDebug("MGT", "LKP", "Trying [%s]", xpath.c_str());
            const SAFplus::Buffer& b = mgtCheckpoint->read(xpath);
            if (&b)
              {
                assert(b.len() == sizeof(SAFplus::Handle));
                SAFplus::Handle hdl = *((const SAFplus::Handle*) b.data);
                logDebug("MGT", "LKP", "Resolved [%s] to [%" PRIx64 ",%" PRIx64 "]", xpath.c_str(),hdl.id[0],hdl.id[1]);
                //std::string strRout = pathSpec.substr(lastSlash+1);  // request the "rest" of the string (everything that did not match the binding)
                return hdl;
              }
          }
        catch (SAFplus::Error& e)
          {
          }
        lastSlash = xpath.find_last_of("/[");
        if (lastSlash == std::string::npos) break;
        xpath = xpath.substr(0,lastSlash);
      }
    return INVALID_HDL;
  }

  void lookupObjects(const std::string& pathSpec, std::vector<MgtObject*>* matches)
  {
    MgtRoot *mgtRoot = MgtRoot::getInstance();

    // If there are non xpath directives preceding the XPATH then strip and process them
    // TODO: add directives
    std::string xpath;
    if (pathSpec[0] == '{')
      {
        xpath = pathSpec.substr(pathSpec.find('}')+1);
      }
    else
      {
        xpath = pathSpec;
      }

    mgtRoot->resolvePath(xpath.c_str()+1,matches);  // +1 to drop the preceding /

  }

  SAFplus::MsgReply *mgtRpcRequest(SAFplus::Handle src, MsgMgt_MgtMsgType reqType, const std::string& pathSpec, const std::string& value = "", Mgt::Msg::MsgRpc::MgtRpcType rpcType=Mgt::Msg::MsgRpc::CL_MGT_RPC_UNDEFINE)
  {
    SAFplus::MsgReply *msgReply = NULL;
    uint retryDuration = SAFplusI::MsgSafplusSendReplyRetryInterval;
    std::string request;
    if (pathSpec[0]=='{')
      {
        if ((pathSpec[1] == 'b')||(pathSpec[1] == 'p')) // Commands a break (to gdb) or pause on the server side processing this RPC, so we'll wait forever before retrying
          {
            retryDuration = 1000*1000*1000;
          }
      }
    MsgMgt mgtMsgReq;
    MsgRpc rpcMsgReq;
    if(reqType==Mgt::Msg::MsgMgt::CL_MGT_MSG_RPC)
    {
        std::string data = value;
        rpcMsgReq.set_rpctype(rpcType);
        rpcMsgReq.set_data(data);
//        rpcMsgReq.set_bind(pathSpec)
        rpcMsgReq.SerializeToString(&request);
    }
    else
    {
        mgtMsgReq.set_type(reqType);
        mgtMsgReq.set_bind(pathSpec);
        mgtMsgReq.add_data(value);
        mgtMsgReq.SerializeToString(&request);
        logError("MGT", "REV", "send MGT [%d] [%s] [%s]",reqType,pathSpec.c_str(),value.c_str());
    }
    SAFplus::SafplusMsgServer* mgtIocInstance = &SAFplus::safplusMsgServer;
    try
      {
        msgReply = mgtIocInstance->sendReply(src, (void *) request.c_str(), request.size(), SAFplusI::CL_MGT_MSG_TYPE, NULL, retryDuration);
        if (!msgReply)
        {
            logError("MGT", "REV", "No REPLY");
        }
      }
    catch (SAFplus::Error &ex)
      {
        logError("MGT", "REQ", "RPC [%d] failure for xpath [%s]", reqType, pathSpec.c_str());
      }
    return msgReply;
  }

  std::string mgtGet(SAFplus::Handle src, const std::string& pathSpec)
  {
    std::string output = "";
    MsgGeneral rxMsg;
    logError("MGT", "REV", "mgtget call");
    SAFplus::MsgReply *msgReply = mgtRpcRequest(src, Mgt::Msg::MsgMgt::CL_MGT_MSG_XGET, pathSpec);
    if (msgReply != NULL)
    {
      rxMsg.ParseFromArray(msgReply->buffer, strlen(msgReply->buffer));
      if (rxMsg.data_size() == 1) //Received data
        {
          output.assign(rxMsg.data(0));
        }
      else
        {
          logError("MGT", "REV", "Invalid message with data size %d", rxMsg.data_size());
        }
    }
    return output;
  }

  std::string mgtGet(const std::string& pathSpec)
  {
    std::string output = "";
    std::vector<MgtObject*> matches;

    std::string suffixLoc = pathSpec.substr(pathSpec.find('/') + 1);
    if (suffixLoc.empty())
      {
        MgtRoot *mgtRoot = MgtRoot::getInstance();

        // 1. 'get' all children of root
        for (MgtObject::Iterator it = mgtRoot->begin(); it != mgtRoot->end(); it++)
        {
            std::string objxml;
            it->second->get(&objxml);
            output.append(objxml);
        }

        // 2. Forward to retrieve from handle
        // 2.1 Collect handle from checkpoint
        std::vector<SAFplus::Handle> processHdl;
        for (Checkpoint::Iterator it = mgtCheckpoint->begin(); it != mgtCheckpoint->end(); ++it)
          {
            SAFplus::Buffer *b = (*it).second.get();
            if (b)
              {
                assert(b->len() == sizeof(SAFplus::Handle));
                SAFplus::Handle hdl = *((const SAFplus::Handle*) b->data);
                processHdl.push_back(hdl);
              }
          }

        // 2.2 Issue 'get' all children for each handle (also ignore duplicate handle)
        std::vector<SAFplus::Handle>::iterator it;
        it = std::unique(processHdl.begin(), processHdl.end());
        processHdl.resize(std::distance(processHdl.begin(), it));
        for (it = processHdl.begin(); it != processHdl.end(); ++it)
          {
            std::string objxml;
            SAFplus::Handle hdl = *it;
            objxml = mgtGet(hdl, pathSpec);
            output.append(objxml);
          }

        return output;
      }

    lookupObjects(pathSpec, &matches);

    if (matches.size())
      {
        for(std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            std::string objxml;
            (*i)->get(&objxml);
            output.append(objxml);
          }
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        ClRcT errCode = CL_OK;
        Handle hdl = getMgtHandle(pathSpec, errCode);
        if (INVALID_HDL != hdl)
        {
          output = mgtGet(hdl, pathSpec);  // Pass the {} directives through to the server side
        }
        else
          {
            logError("MGT", "REV", "Route [%s] has no implementer", pathSpec.c_str());
            // TODO: throw exception
          }
      }
    return output;
  }

  ClRcT mgtSet(SAFplus::Handle src, const std::string& pathSpec, const std::string& value)
  {
    ClRcT ret = CL_OK;
    logError("MGT", "REV", "mgtSet call");
    SAFplus::MsgReply *msgReply = mgtRpcRequest(src, Mgt::Msg::MsgMgt::CL_MGT_MSG_XSET, pathSpec, value);

    if (msgReply == NULL)
      {
        ret = CL_ERR_IGNORE_REQUEST;
      }
    else
      {
        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
      }
    return ret;
  }

  ClRcT mgtSet(const std::string& pathSpec, const std::string& value)
  {
    ClRcT ret = CL_OK;
    std::vector<MgtObject*> matches;

    lookupObjects(pathSpec, &matches);

    if (matches.size())
      {
        for(std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            ret = (*i)->setObj(value);
          }
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(pathSpec, ret);
        if (ret == CL_OK && INVALID_HDL != hdl)
        {
          ret = mgtSet(hdl, pathSpec, value);
        }
        else
          {
            ret = CL_OK;
            logError("MGT", "REV", "Route [%s] has no implementer", pathSpec.c_str());
            // TODO: throw exception
          }
      }

    return ret;
  }


  ClRcT mgtRpc(SAFplus::Handle src,Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec,const std::string& request)
  {
    ClRcT ret = CL_OK;
    SAFplus::MsgReply *msgReply = mgtRpcRequest(src, Mgt::Msg::MsgMgt::CL_MGT_MSG_RPC, pathSpec, request,mgtRpcType);
    if (msgReply == NULL)
      {
        ret = CL_ERR_IGNORE_REQUEST;
      }
    else
      {
        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
      }
    return ret;
  }

  ClRcT mgtRpc(Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec, const std::string& request)
  {
    ClRcT ret = CL_OK;
    std::string xpath = "{n}/";
    xpath.append(pathSpec);
    std::vector<MgtObject*> matches;
    lookupObjects(xpath, &matches);
    if (matches.size())
      {
        for(std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            MgtRpc *rpc = dynamic_cast<MgtRpc*> (*i);
            rpc->setInParams((void*)request.c_str(),request.length());
            switch (mgtRpcType)
            {
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_VALIDATE:
                // set parameter
                ret = rpc->validate();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_INVOKE:
                //set input parram
                ret = rpc->invoke();
                break;
              case Mgt::Msg::MsgRpc::CL_MGT_RPC_POSTREPLY:
                //set input parram
                ret = rpc->postReply();
                break;
              default:
                break;
            }
          }
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(xpath, ret);
        if (ret == CL_OK && INVALID_HDL != hdl)
        {
          ret = mgtRpc(hdl,mgtRpcType, pathSpec, request);
        }
        else
          {
            ret = CL_OK;
            logError("MGT", "REV", "Route [%s] has no implementer", pathSpec.c_str());
            // TODO: throw exception
          }
      }
    return ret;
  }


  ClRcT mgtCreate(SAFplus::Handle src, const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    SAFplus::MsgReply *msgReply = mgtRpcRequest(src, Mgt::Msg::MsgMgt::CL_MGT_MSG_CREATE, pathSpec);

    if (msgReply == NULL)
      {
        ret = CL_ERR_IGNORE_REQUEST;
      }
    else
      {
        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
      }
    return ret;
  }

  ClRcT mgtCreate(const std::string& pathSpec)
  {
      ClRcT ret = CL_OK;
      std::vector<MgtObject*> matches;

      std::size_t idx = pathSpec.find_last_of("/");

      if (idx == std::string::npos)
        {
          // Invalid xpath
          return CL_ERR_INVALID_PARAMETER;
        }

      std::string path = pathSpec.substr(0, idx);
      std::string value = pathSpec.substr(idx + 1);

      lookupObjects(path, &matches);

      if (matches.size())
        {
          for(std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
            {
              ret = (*i)->createObj(value);
            }
        }
      else  // Object Implementer not found. Broadcast message to get data
        {
          Handle hdl = getMgtHandle(pathSpec, ret);
          if (ret == CL_OK && INVALID_HDL != hdl)
          {
            ret = mgtCreate(hdl, pathSpec);
          }
          else
            {
              ret = CL_OK;
              logError("MGT", "REV", "Route [%s] has no implementer", pathSpec.c_str());
              // TODO: throw exception
            }
        }
    return ret;
  }

  ClRcT mgtDelete(SAFplus::Handle src, const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    SAFplus::MsgReply *msgReply = mgtRpcRequest(src, Mgt::Msg::MsgMgt::CL_MGT_MSG_DELETE, pathSpec);

    if (msgReply == NULL)
      {
        ret = CL_ERR_IGNORE_REQUEST;
      }
    else
      {
        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
      }
    return ret;
  }

  ClRcT mgtDelete(const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;
    std::vector<MgtObject*> matches;
    std::size_t idx = pathSpec.find_last_of("/");

    if (idx == std::string::npos)
      {
        // Invalid xpath
        return CL_ERR_INVALID_PARAMETER;
      }

    std::string path = pathSpec.substr(0, idx);
    std::string value = pathSpec.substr(idx + 1);

    lookupObjects(path, &matches);

    if (matches.size())
      {
        for(std::vector<MgtObject*>::iterator i = matches.begin(); i != matches.end(); i++)
          {
            ret = (*i)->deleteObj(value);
          }
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(pathSpec, ret);
        if (ret == CL_OK && INVALID_HDL != hdl)
        {
          ret = mgtDelete(hdl, pathSpec);
        }
        else
          {
            ret = CL_OK;
            logError("MGT", "REV", "Route [%s] has no implementer", pathSpec.c_str());
            // TODO: throw exception
          }
      }
    return ret;
  }
}


extern "C" void dbgDumpMgtBindings()
{
  // 2. Forward to retrieve from handle
  // 2.1 Collect handle from checkpoint
  std::vector<SAFplus::Handle> processHdl;
  for (SAFplus::Checkpoint::Iterator it = SAFplus::mgtCheckpoint->begin(); it != SAFplus::mgtCheckpoint->end(); ++it)
    {
      SAFplus::Buffer *b = (*it).second.get();

      if (b)
	{
	  assert(b->len() == sizeof(SAFplus::Handle));
	  SAFplus::Handle hdl = *((const SAFplus::Handle*) b->data);
	  printf("%s -> [%" PRIx64 ":%" PRIx64 "] node: %d, port: %d\n",((char*) (*it).first->data),hdl.id[0],hdl.id[1],hdl.getNode(),hdl.getPort());
	}
    }
}
