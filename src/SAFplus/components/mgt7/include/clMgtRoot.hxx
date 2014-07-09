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
#ifdef MGT_ACCESS
#include <clSafplusMsgServer.hxx>
#include <clMsgApi.hxx>
#endif

#include "clCommon.hxx"

#define CL_NETCONF_BIND_TYPE 0
#define CL_SNMP_BIND_TYPE 1

#define CL_IOC_MGT_NETCONF_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 1)
#define CL_IOC_MGT_SNMP_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 2)

namespace SAFplus
{
/**
 * These message type are used from external,so it should be in SAFplus namespace
 */
  enum class MgtMsgType
  {
      CL_MGT_MSG_UNUSED,
      CL_MGT_MSG_EDIT,
      CL_MGT_MSG_GET,
      CL_MGT_MSG_RPC,
      CL_MGT_MSG_OID_SET,
      CL_MGT_MSG_OID_GET,
      CL_MGT_MSG_BIND, //netconf
      CL_MGT_MSG_OID_BIND, //snmp
      CL_MGT_MSG_BIND_RPC,
      CL_MGT_MSG_NOTIF
  };
  class MgtMsgProto
  {
    public:
      MgtMsgType     messageType;
      char                  data[1]; //Not really 1, it will be place on larger memory
      MgtMsgProto()
      {
        messageType = MgtMsgType::CL_MGT_MSG_UNUSED;
      }
  };
/**
 * MgtRoot class provides APIs to setup the MGT database
 */
class MgtRoot {
protected:
    MgtRoot();
    /*
     * Init singleton for context object
     */
    static MgtRoot *singletonInstance;

    /*
     * Store the list of MGT module
     */
    std::map<std::string, MgtModule*> mMgtModules;
public:
    virtual ~MgtRoot();

    /**
     * \brief	Function to create/get the singleton object of the MgtRoot class
     */
    static MgtRoot *getInstance();

    /**
     * \brief	Function to load MGT module to the system
     * \param	module					Pointer to MGT module
     * \param	moduleName				MGT module name
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	MGT module already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT loadMgtModule(MgtModule *module, const std::string moduleName);

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
    ClRcT bindMgtObject(ClUint8T bindType, MgtObject *object, const std::string module, const std::string route);

    /**
     * \brief   Function to bind a MGT object to a specific manageability subtree within a particular module
     * \param   module                  MGT module name
     * \param   route                   XPath of the MGT object
     * \return  CL_OK                   Everything is OK
     * \return  CL_ERR_NOT_EXIST        MGT module does not exist
     * \return  CL_ERR_ALREADY_EXIST    RPC already exists
     */
    ClRcT registerRpc(const std::string module, const std::string rpcName);
#ifdef MGT_ACCESS
    /**
     * Mgt message handlers
     */
    void clMgtMsgEditHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg);
    void clMgtMsgGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg);
    void clMgtMsgRpcHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg);
    void clMgtMsgOidSetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg);
    void clMgtMsgOidGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg);

    class MgtMessageHandler:public MsgHandler
    {
      public:
        MgtRoot* mRoot;
        MgtMessageHandler(SAFplus::MgtRoot *mroot=nullptr);
        void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    };
    static ClRcT sendMsg(ClIocAddressT dest, void* payload, uint payloadlen, MgtMsgType msgtype);
#endif
};
};
#endif /* CLMGTROOT_H_ */

/** \} */
