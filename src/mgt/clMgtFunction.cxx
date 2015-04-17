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
  Group *MgtFunction::mgtGroup = 0;

  Group *MgtFunction::getGroup()
  {
    if (!mgtGroup)
      {
        mgtGroup = new Group();
        mgtGroup->init(MGT_GROUP);
      }

    return mgtGroup;
  }

  void MgtFunction::registerEntity(Handle handle)
  {
    Group *group = MgtFunction::getGroup();
    group->registerEntity(handle, 0, NULL, 0, 0);
  }

  void MgtFunction::deregisterEntity(Handle handle)
  {
    Group *group = MgtFunction::getGroup();
    group->deregister(handle);
  }

  std::string MgtFunction::mgtGet(SAFplus::Handle src, const std::string& pathSpec)
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

  std::string MgtFunction::mgtGet(const std::string& pathSpec)
  {
    std::string output = "";
    ClUint64T outMsgSize = 0;

    MgtRoot *mgtRoot = MgtRoot::getInstance();
    MgtObject *object = mgtRoot->findMgtObject(pathSpec);

    if (object)
      {
        object->get(&output, &outMsgSize);
      }
    else  // Object Implementer not found. Broadcast message to get data
      {
        Group *group = MgtFunction::getGroup();

        for (Group::Iterator it = group->begin();it != group->end(); it++)
          {
            Handle hdl = it->first;
            output = mgtGet(hdl, pathSpec);
            if (output.length() != 0)
              return output;
          }
      }
    return output;
  }

  ClRcT MgtFunction::mgtSet(SAFplus::Handle src, const std::string& pathSpec, const std::string& value)
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

  ClRcT MgtFunction::mgtSet(const std::string& pathSpec, const std::string& value)
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
        Group *group = MgtFunction::getGroup();

        for (Group::Iterator it = group->begin();it != group->end(); it++)
          {
            Handle hdl = it->first;
            ret = mgtSet(hdl, pathSpec, value);
            if ((ret == CL_OK) || (ret != CL_ERR_IGNORE_REQUEST))
              return ret;
          }
      }
    return ret;
  }

  ClRcT MgtFunction::mgtCreate(SAFplus::Handle src, const std::string& pathSpec)
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

  ClRcT MgtFunction::mgtCreate(const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MgtRoot *mgtRoot = MgtRoot::getInstance();

    int idx = pathSpec.find_last_of("/");

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
        Group *group = MgtFunction::getGroup();

        for (Group::Iterator it = group->begin();it != group->end(); it++)
          {
            Handle hdl = it->first;
            ret = mgtCreate(hdl, pathSpec);
            if ((ret == CL_OK) || (ret != CL_ERR_IGNORE_REQUEST))
              return ret;
          }
      }
    return ret;
  }

  ClRcT MgtFunction::mgtDelete(SAFplus::Handle src, const std::string& pathSpec)
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

  ClRcT MgtFunction::mgtDelete(const std::string& pathSpec)
  {
    ClRcT ret = CL_OK;

    MgtRoot *mgtRoot = MgtRoot::getInstance();

    int idx = pathSpec.find_last_of("/");

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
        Group *group = MgtFunction::getGroup();

        for (Group::Iterator it = group->begin();it != group->end(); it++)
          {
            Handle hdl = it->first;
            ret = mgtDelete(hdl, pathSpec);
            if ((ret == CL_OK) || (ret != CL_ERR_IGNORE_REQUEST))
              return ret;
          }
      }
    return ret;
  }
}
