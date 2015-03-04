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
 *  \brief Header file of the MgtModule class which provides APIs to manage MGT modules
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTMODULE_HXX_
#define CLMGTMODULE_HXX_

#include <map>
#include <string>
#include <clCommon.hxx>
#include "clMgtObject.hxx"
#include "clMgtNotify.hxx"
#include "clMgtRpc.hxx"

namespace SAFplus
{
  /**
   * MgtModule class provides APIs to manage a MGT modules
   */
  class MgtModule
  {
  private:
    /*
     * Store the list of MGT object
     */
    std::map<std::string, MgtObject*> mMgtObjects;

    /*
     * Store the list of MGT notify
     */
    std::map<std::string, MgtNotify*> mMgtNotifies;

    /*
     * Store the list of MGT addRpc
     */
    std::map<std::string, MgtRpc*> mMgtRpcs;

  public:
    std::string name;

  public:
    MgtModule(const char* name);
    virtual ~MgtModule();

    /**
     * \brief	Function to load a MGT module into the netconf server
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	Module already exists
     */
    ClRcT loadModule();

    /**
     * \brief	Virtual function to initialize non-config data structures
     */
    virtual void initialize();

    /**
     * \brief	Function to add a MGT object into the database
     * \param	mgtObject				MGT object to be added
     * \param	route					XPath of the MGT object
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	Module already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT addMgtObject(MgtObject *mgtObject, const std::string route);

    /**
     * \brief	Function to remove a MGT object
     * \param	route					XPath of the MGT object
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT object does not exist
     */
    ClRcT removeMgtObject(const std::string& route);

    /**
     * \brief	Function to get a MGT object from the database
     * \param	objectName				XPath of the MGT object
     * \return	If the function succeeds, the return value is a MGT object
     * \return	If the function fails, the return value is NULL
     */
    MgtObject *getMgtObject(const std::string& route);

    /**
     * \brief	Function to add a MGT MGT notification into the database
     * \param	mgtNotify				MGT notification to be added
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	MGT notification already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT addMgtNotify(MgtNotify *mgtNotify);

    /**
     * \brief	Function to remove a MGT object
     * \param	notifyName				Name of the MGT notification
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT notification does not exist
     */
    ClRcT removeMgtNotify(const std::string& notifyName);

    /**
     * \brief	Function to get a MGT notification from the database
     * \param	notifyName				Name of the MGT notification
     * \return	If the function succeeds, the return value is a MGT notification
     * \return	If the function fails, the return value is NULL
     */
    MgtNotify *getMgtNotify(const std::string& notifyName);

    /**
     * \brief	Function to add a MGT RPC into the database
     * \param	mgtRpc					MGT RPC to be added
     * \param	rpcName					Name of the MGT RPC
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	MGT RPC already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT addMgtRpc(MgtRpc *mgtRpc);

    /**
     * \brief	Function to remove a MGT object
     * \param	rpcName					Name of the MGT RPC
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT RPC does not exist
     */
    ClRcT removeMgtRpc(const std::string& rpcName);

    /**
     * \brief	Function to get a MGT RPC from the database
     * \param	rpcName					Name of the MGT RPC
     * \return	If the function succeeds, the return value is a MGT RPC
     * \return	If the function fails, the return value is NULL
     */
    MgtRpc *getMgtRpc(const std::string& rpcName);
  };

};

#endif /* CLMGTMODULE_HXX_ */

/** \} */
