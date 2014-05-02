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

#ifndef CLMGTRPC_HXX_
#define CLMGTRPC_HXX_

#include <map>
#include <string>
#include "clMgtMsg.hxx"
#include "clMgtObject.hxx"
#include "clCommon.hxx"

namespace SAFplus
{

  class ClMgtRpc
  {
  public:
    std::string Name;
    std::string Module;
    std::string ErrorMsg;

    /*
     * Store the list of input parameters
     */
    ClMgtObject mInParams;

    /*
     * Store the list of output parameters
     */
    ClMgtObject mOutParams;

  public:
    ClMgtRpc(const char* name);
    virtual ~ClMgtRpc();

    /**
     * Function to add input parameter
     */
    void addInParam(std::string param, ClMgtObject *mgtObject);

    /**
     * Function to add output parameter
     */
    void addOutParam(std::string param, ClMgtObject *mgtObject);

    /**
     * Function to set value to an input parameter
     */
    ClBoolT setInParams(void *pBuffer, ClUint64T buffLen);


    void getOutParams(void **ppBuffer, ClUint64T *pBuffLen);


    virtual ClBoolT validate() = 0;
    virtual ClBoolT invoke() = 0;
    virtual ClBoolT postReply() = 0;

    /**
     * \brief   Function to register a Rpc to the server
     * \return  CL_OK                   Everything is OK
     * \return  CL_ERR_NOT_EXIST        MGT module does not exist
     * \return  CL_ERR_ALREADY_EXIST    Rpc already exists
     */
    ClRcT registerRpc();
  };
};

#endif /* CLMGTRPC_HXX_ */
