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

/**
 *  \file
 *  \brief Header file of MgtRoot class which provides APIs to setup the MGT database
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTROOT_H_
#define CLMGTROOT_H_

#include <map>
#include <string>

#include "clMgtModule.hxx"
#include "clMgtObject.hxx"
#include <clSafplusMsgServer.hxx>
#include <clMsgApi.hxx>
#include "clCommon.hxx"
#include <clCkptApi.hxx> // to use checkpoint
namespace Mgt
  {
    namespace Msg
      {
        class MsgMgt;
        class MsgRpc;
      }
  }

namespace SAFplus
{

/**
 * MgtRoot class provides APIs to setup the MGT database
 */
  class MgtRoot:public MgtContainer {
protected:
    MgtRoot();
    /*
     * Init singleton for context object
     */
    static MgtRoot *singletonInstance;

    /*
     * Store the list of MGT module
     */
    //std::map<std::string, MgtModule*> mMgtModules;

    /*
     * Mgt ref list
     */
    std::vector<MgtObject*> mgtReferenceList;


public:
    virtual ~MgtRoot();

    /**
     * \brief	Function to create/get the singleton object of the MgtRoot class
     */
    static MgtRoot *getInstance();

    static void DestroyInstance(); // Constructor for singleton

    /**
     * \brief	Function to load MGT module to the system
     * \param	module					Pointer to MGT module
     * \param	moduleName				MGT module name
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	MGT module already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT loadMgtModule(Handle handle, MgtModule *module, const std::string moduleName);

    /**
     * \brief	Function to unload MGT module from the system
     * \param	moduleName				MGT module name
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT module does not exist
     */
    ClRcT unloadMgtModule(const std::string moduleName);

    /**
     * \brief	Function to get a MGT module from the database
     * \param	moduleName				MGT module name
     * \return	If the function succeeds, the return value is a MGT module
     * \return	If the function fails, the return value is NULL
     */
    MgtModule *getMgtModule(const std::string moduleName);

    /**
     * \brief	Function to bind a MGT object to a specific manageability subtree within a particular module
     * \param	object					Pointer to MGT object
     * \param	module					MGT module name
     * \param	route					XPath of the MGT object
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT module does not exist
     * \return	CL_ERR_ALREADY_EXIST	MGT object already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT bindMgtObject(Handle handle, SAFplus::MgtObject *object, const std::string& module, const std::string& route);
    void bind(Handle handle,MgtObject* obj);
    void registerRpc(Handle handle,MgtRpc* obj);



    /**
     * \brief   Function to bind a MGT object to a specific manageability subtree within a particular module
     * \param   module                  MGT module name
     * \param   route                   XPath of the MGT object
     * \return  CL_OK                   Everything is OK
     * \return  CL_ERR_NOT_EXIST        MGT module does not exist
     * \return  CL_ERR_ALREADY_EXIST    RPC already exists
     */
//    ClRcT registerRpc(SAFplus::Handle handle,const std::string module, const std::string rpcName);
    /**
     * Mgt message handlers
     */
#if 0 // Obsoletely, don't use anymore
    void clMgtMsgEditHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
    void clMgtMsgGetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
#endif
    void clMgtMsgXGetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
    void clMgtMsgXSetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
    ClRcT clMgtMsgCreateHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
    void clMgtMsgDeleteHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& mgtMsgReq);
    void clMgtMsgRPCHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgRpc& reqMsg);
    void clMgtMsgActionHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgRpc& reqMsg);
    void clMgtMsgRestGetHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg);
    void clMgtMsgRestDeleteHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg);
    void clMgtMsgRestPutHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg);
    void clMgtMsgRestPatchHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg);
    void clMgtMsgRestPostHandler(SAFplus::Handle srcAddr, Mgt::Msg::MsgMgt& reqMsg);
    class MgtMessageHandler:public SAFplus::MsgHandler
    {
      public:
        MgtRoot* mRoot;
        MgtMessageHandler();
        void init(SAFplus::MgtRoot *mroot=nullptr);
        void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    };
    MgtMessageHandler mgtMessageHandler;

//TODO:
//    static ClRcT sendMsg(SAFplus::Handle dest, void* payload, uint payloadlen, MgtMsgType msgtype,void* reply = NULL);
    static ClRcT sendReplyMsg(SAFplus::Handle dest, void* payload, uint payloadlen);


    //MgtObject *findMgtObject(const std::string &xpath);  // DEPRECATED
    //void resolvePath(const char* path, std::vector<MgtObject*>* result);

    //? Add an entity that implements the "updateReference" virtual function to the list of entities that need to be updated when the mgt tree structure changes.  This functionality allows lazy resolution of MgtIdentifier and MgtIdentifierList.  By allowing lazy resolution, the system does not require that management changes be made in a dependency order.
    void addReference(MgtObject* mgtObject);
    //? Remove an entity from the update list.  This functionality allows lazy resolution of MgtIdentifier and MgtIdentifierList.
    void removeReference(MgtObject* mgtObject);

    //? Called to trigger a call to "updateReference" to every element in the update list.  This functionality allows lazy resolution of MgtIdentifier and MgtIdentifierList.
    void updateReference(void);
};
};
#endif /* CLMGTROOT_H_ */

/** \} */
