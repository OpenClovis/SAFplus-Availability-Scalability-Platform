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
   if(!mgtCheckpoint)
     {
        mgtCheckpoint = new Checkpoint(MGT_CKPT, Checkpoint::SHARED | Checkpoint::REPLICATED ,
              1024*1024,
              SAFplusI::CkptDefaultRows);
        mgtCheckpoint->name = "safplusMgt";
     }
  }

  SAFplus::Handle getMgtHandle(const std::string& pathSpec, ClRcT &errCode)
  {
    std::string xpath = pathSpec;  // I need to make a copy so I can modify it
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

                std::string strRout = pathSpec.substr(lastSlash+1);  // request the "rest" of the string (everything that did not match the binding)
                return hdl;
              }
          }
        catch (SAFplus::Error& e)
          {
          }
        lastSlash = xpath.rfind("/");
        if (lastSlash == std::string::npos) break;
        xpath = xpath.substr(0,lastSlash);
      }
    return INVALID_HDL;
#if 0
    errCode = CL_OK;
    std::string strPath = "";
    SAFplus::Checkpoint::Iterator iterMgtChkp = getMgtCheckpoint()->end();
    for (iterMgtChkp = getMgtCheckpoint()->begin();
          iterMgtChkp != getMgtCheckpoint()->end();
          iterMgtChkp++)
      {
        SAFplus::Checkpoint::KeyValuePair& item = *iterMgtChkp;
        strPath.assign((char*) (*item.first).data);
        int find = strPath.find_last_of("/");
        if (find != std::string::npos)
          {
            std::string route = strPath.substr(find);
            if (pathSpec.find(route) != std::string::npos)
              {
                Handle &Hdl = *((Handle*) (*item.second).data);
                return Hdl;
              }
          }
      }
    errCode = CL_ERR_INVALID_PARAMETER;
    return *((Handle*) NULL);
#endif
  }

  std::string mgtGet(SAFplus::Handle src, const std::string& pathSpec)
  {
    std::string output = "";
    MsgMgt mgtMsgReq;
    std::string request;
    std::string strRev = "";

    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_XGET);
    mgtMsgReq.set_bind(pathSpec);
    //Send mine data revision
    mgtMsgReq.add_data(strRev);
    mgtMsgReq.SerializeToString(&request);

    SAFplus::SafplusMsgServer* mgtIocInstance = &SAFplus::safplusMsgServer;
    try
      {
        SAFplus::MsgReply *msgReply = mgtIocInstance->sendReply(src, (void *) request.c_str(), request.size(), SAFplusI::CL_MGT_MSG_TYPE);
        MsgGeneral rxMsg;
        if (!msgReply)
          {
            logError("MGT", "REV", "No REPLY");
            return output;
          }

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
    catch (SAFplus::Error &ex)
      {
        logError("MGT", "REV", "Failure while retrieving data for route [%s]", pathSpec.c_str());
      }
    return output;
  }

  std::string mgtGet(const std::string& pathSpec)
  {
    std::string output = "";

    MgtRoot *mgtRoot = MgtRoot::getInstance();
    std::vector<MgtObject*> matches;
    mgtRoot->resolvePath(pathSpec.c_str()+1,&matches);

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
        std::string xpath;
        if (pathSpec[0] == '{')
          {
            xpath = pathSpec.substr(pathSpec.find('}')+1);
          }
        else
          {
            xpath = pathSpec;
          }
        Handle hdl = getMgtHandle(xpath   , errCode);
        if (INVALID_HDL != hdl)
        {
          output = mgtGet(hdl, pathSpec);
        }
      }
    return output;
  }

  ClRcT mgtSet(SAFplus::Handle src, const std::string& pathSpec, const std::string& value)
  {
    ClRcT ret = CL_OK;

    MsgMgt mgtMsgReq;
    std::string request;

    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_XSET);
    mgtMsgReq.set_bind(pathSpec);
    mgtMsgReq.add_data(value);
    mgtMsgReq.SerializeToString(&request);

    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    try
      {
        MsgReply *msgReply = mgtIocInstance->sendReply(src, (void *) request.c_str(), request.size(), SAFplusI::CL_MGT_MSG_TYPE);

        if (!msgReply)
          return CL_ERR_IGNORE_REQUEST;

        if (msgReply->len == 0)
          return CL_ERR_IGNORE_REQUEST;

        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
        return ret;
      }
    catch (SAFplus::Error &ex)
      {
        ret = CL_ERR_FAILED_OPERATION;
      }

    return ret;
  }

  ClRcT mgtSet(const std::string& pathSpec, const std::string& value)
  {
    ClRcT ret = CL_OK;

    MgtRoot *mgtRoot = MgtRoot::getInstance();
    MgtObject *object = mgtRoot->findMgtObject(pathSpec);

    if (object)
      {
        ret = object->setObj(value);
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(pathSpec, ret);
        if((ret == CL_OK) && (NULL != &hdl))
          {
            ret = mgtSet(hdl, pathSpec, value);
            if ((ret == CL_OK) || (ret != CL_ERR_IGNORE_REQUEST))
              return ret;
          }
      }
    return ret;
  }

  ClRcT mgtCreate(SAFplus::Handle src, const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MsgMgt mgtMsgReq;
    std::string request;

    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_CREATE);
    mgtMsgReq.set_bind(pathSpec);
    mgtMsgReq.add_data("");
    mgtMsgReq.SerializeToString(&request);

    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    try
      {
        MsgReply *msgReply = mgtIocInstance->sendReply(src, (void *) request.c_str(), request.size(), SAFplusI::CL_MGT_MSG_TYPE);

        if (!msgReply)
          return CL_ERR_IGNORE_REQUEST;

        if (msgReply->len == 0)
          return CL_ERR_IGNORE_REQUEST;

        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
        return ret;
      }
    catch (SAFplus::Error &ex)
      {
        ret = CL_ERR_FAILED_OPERATION;
      }

    return ret;
  }

  ClRcT mgtCreate(const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MgtRoot *mgtRoot = MgtRoot::getInstance();

    std::size_t idx = pathSpec.find_last_of("/");

    if (idx == std::string::npos)
      {
        // Invalid xpath
        return CL_ERR_INVALID_PARAMETER;
      }

    std::string xpath = pathSpec.substr(0, idx);
    std::string value = pathSpec.substr(idx + 1);

    MgtObject *object = mgtRoot->findMgtObject(xpath);

    if (object)
      {
        ret = object->createObj(value);
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(pathSpec, ret);
        if((ret == CL_OK) && (NULL != &hdl))
          {
            ret = mgtCreate(hdl, pathSpec);
            if ((ret == CL_OK) || (ret != CL_ERR_IGNORE_REQUEST))
              return ret;
          }
      }
    return ret;
  }

  ClRcT mgtDelete(SAFplus::Handle src, const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MsgMgt mgtMsgReq;
    std::string request;

    mgtMsgReq.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_DELETE);
    mgtMsgReq.set_bind(pathSpec);
    mgtMsgReq.add_data("");
    mgtMsgReq.SerializeToString(&request);

    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    try
      {
        MsgReply *msgReply = mgtIocInstance->sendReply(src, (void *) request.c_str(), request.size(), SAFplusI::CL_MGT_MSG_TYPE);

        if (!msgReply)
          return CL_ERR_IGNORE_REQUEST;

        if (msgReply->len == 0)
          return CL_ERR_IGNORE_REQUEST;

        memcpy(&ret, msgReply->buffer, sizeof(ClRcT));
        return ret;
      }
    catch (SAFplus::Error &ex)
      {
        ret = CL_ERR_FAILED_OPERATION;
      }

    return ret;
  }

  ClRcT mgtDelete(const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MgtRoot *mgtRoot = MgtRoot::getInstance();

    std::size_t idx = pathSpec.find_last_of("/");

    if (idx == std::string::npos)
      {
        // Invalid xpath
        return CL_ERR_INVALID_PARAMETER;
      }

    std::string xpath = pathSpec.substr(0, idx);
    std::string value = pathSpec.substr(idx + 1);

    MgtObject *object = mgtRoot->findMgtObject(xpath);

    if (object)
      {
        ret = object->deleteObj(value);
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Handle hdl = getMgtHandle(pathSpec, ret);
        if((ret == CL_OK) && (NULL != &hdl))
         {
           ret = mgtDelete(hdl, pathSpec);
         }
      }
    return ret;
  }
}
