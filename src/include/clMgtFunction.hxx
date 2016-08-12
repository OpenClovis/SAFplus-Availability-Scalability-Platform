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

#pragma once
#ifndef CLMGTFUNCTION_HXX_
#define CLMGTFUNCTION_HXX_
//? <section name="Management">

#include <string>
#include <clHandleApi.hxx>
#include <clCkptApi.hxx> // to use checkpoint
#include <MgtMsg.pb.hxx>
namespace SAFplus
{
     extern Checkpoint* mgtCheckpoint;
     Checkpoint* getMgtCheckpoint();
     SAFplus::Handle getMgtHandle(const std::string& pathSpec, ClRcT &errCode);

     //? Call this before any other management APIs (or call safplusInitialize with the MGT flag)
     void mgtAccessInitialize();

     //? Get information from the management subsystem -- this call will look up the path in the Management checkpoint and forward the request to the appropriately bound process handle
     std::string mgtGet(const std::string& pathSpec);
     //? Get information from the management subsystem -- this call will send the request directly to the process specified by the supplied handle
     std::string mgtGet(SAFplus::Handle src, const std::string& pathSpec);

     //? Get information from the management subsystem -- removes the XML and casts
     uint64_t mgtGetUint(const std::string& pathSpec);
     //? Get information from the management subsystem -- removes the XML and casts
     int64_t mgtGetInt(const std::string& pathSpec);
     //? Get information from the management subsystem -- removes the XML and casts
     double   mgtGetNum(const std::string& pathSpec);
     //? Get information from the management subsystem -- removes the XML and casts
     std::string mgtGetString(const std::string& pathSpec);

     ClRcT mgtRpc(SAFplus::Handle src,Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec,const std::string& request);
     ClRcT mgtRpc(Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec, const std::string& request);
     //? Make a management remote procedure call (RPC).  
     ClRcT mgtRpc(Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec, const std::string& attribute);
     ClRcT mgtRpc(SAFplus::Handle src,Mgt::Msg::MsgRpc::MgtRpcType mgtRpcType,const std::string& pathSpec, const std::string& attribute);

     //? Set a management entity to a specific value -- this call will send the request directly to the process specified by the supplied handle
     ClRcT mgtSet(SAFplus::Handle src, const std::string& pathSpec, const std::string& value);
     //? Set a management entity to a specific value -- this call will look up the path in the Management checkpoint and forward the request to the appropriately bound process handle
     ClRcT mgtSet(const std::string& pathSpec, const std::string& value);

     //? Create a new management entity (if allowed). For example, you may create new elements in YANG lists.  The entity's fields will be created with default values (or zero).  You may then use <ref>mgtSet()</ref> to set the field values.  This call will send the request directly to the process specified by the supplied handle.
     ClRcT mgtCreate(SAFplus::Handle src, const std::string& pathSpec);
     //? Create a new management entity (if allowed). For example, you may create new elements in YANG lists.  The entity's fields will be created with default values (or zero).  You may then use <ref>mgtSet()</ref> to set the field values.  This call will look up the path in the Management checkpoint and forward the request to the appropriately bound process handle.
     ClRcT mgtCreate(const std::string& pathSpec);

     //? Delete a new management entity (if allowed). For example, you may create new elements in YANG lists.  This call will send the request directly to the process specified by the supplied handle
     ClRcT mgtDelete(SAFplus::Handle src, const std::string& pathSpec);
     //? Delete a new management entity (if allowed). For example, you may create new elements in YANG lists.  This call will look up the path in the Management checkpoint and forward the request to the appropriately bound process handle.
     ClRcT mgtDelete(const std::string& pathSpec);

}
//? </section>

#endif /* CLMGT_HXX_ */
