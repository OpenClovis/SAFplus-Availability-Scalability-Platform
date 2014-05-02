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

#include "clMgtModule.hxx"
#include "clMgtRoot.hxx"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

#ifdef SAFplus7
#define clLog(...)
#endif

namespace SAFplus
{
  ClMgtModule::ClMgtModule(const char* name)
  {
    Name.assign(name);
  }

  ClMgtModule::~ClMgtModule()
  {
  }

  void ClMgtModule::initialize()
  {
  }

  ClRcT ClMgtModule::loadModule()
  {
    return ClMgtRoot::getInstance()->loadMgtModule(this, this->Name);
  }

  ClRcT ClMgtModule::addMgtObject(ClMgtObject *mgtObject, const std::string route)
  {
    ClRcT rc = CL_OK;

    if (mgtObject == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT object already exists in the database */
    if (mMgtObjects.find(route) != mMgtObjects.end())
      {
        clLogDebug("MGT", "ROUTE", "Route [%s] already exists!", route.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT object into the database */
    mMgtObjects.insert(pair<string, ClMgtObject *> (route.c_str(), mgtObject));
    clLogDebug("MGT", "ROUTE", "Route [%s] added successfully!", route.c_str());

    return rc;
  }

  ClRcT ClMgtModule::removeMgtObject(std::string route)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtObjects.find(route) == mMgtObjects.end())
      {
        clLogDebug("MGT", "ROUTE", "Routing [%s] does not exist!", route.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtObjects.erase(route);
    clLogDebug("MGT", "ROUTE", "Routing [%s] removed successful!", route.c_str());

    return rc;
  }

  ClMgtObject *ClMgtModule::getMgtObject(const std::string route)
  {
    map<string, ClMgtObject*>::iterator mapIndex = mMgtObjects.find(route);
    if (mapIndex != mMgtObjects.end())
      {
        return static_cast<ClMgtObject *>((*mapIndex).second);
      }
    return NULL;
  }

  ClRcT ClMgtModule::addMgtNotify(ClMgtNotify *mgtNotify)
  {
    ClRcT rc = CL_OK;

    if (mgtNotify == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT notification already exists in the database */
    if (mMgtNotifies.find(mgtNotify->Name) != mMgtNotifies.end())
      {
        clLogDebug("MGT", "NOT", "Notify [%s] is already existing!", mgtNotify->Name.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT notification into the database */
    mMgtNotifies.insert(pair<string, ClMgtNotify *> (mgtNotify->Name.c_str(), mgtNotify));
    mgtNotify->Module.assign(this->Name);

    clLogDebug("MGT", "NOT", "Notify [%s] added successful!", mgtNotify->Name.c_str());

    return rc;
  }

  ClRcT ClMgtModule::removeMgtNotify(std::string notifyName)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtNotifies.find(notifyName) == mMgtNotifies.end())
      {
        clLogDebug("MGT", "NOT", "Notify [%s] does not exist!", notifyName.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtNotifies.erase(notifyName);
    clLogDebug("MGT", "NOT", "Notify [%s] removed successful!", notifyName.c_str());

    return rc;
  }

  ClMgtNotify *ClMgtModule::getMgtNotify(const std::string notifyName)
  {
    map<string, ClMgtNotify*>::iterator mapIndex = mMgtNotifies.find(notifyName);
    if (mapIndex != mMgtNotifies.end())
      {
        return static_cast<ClMgtNotify *>((*mapIndex).second);
      }
    return NULL;
  }

  ClRcT ClMgtModule::addMgtRpc(ClMgtRpc *mgtRpc)
  {
    ClRcT rc = CL_OK;

    if (mgtRpc == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT RPC already exists in the database */
    if (mMgtRpcs.find(mgtRpc->Name) != mMgtRpcs.end())
      {
        clLogDebug("MGT", "RPC", "RPC [%s] is already existing!", mgtRpc->Name.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT RPC into the database */
    mMgtRpcs.insert(pair<string, ClMgtRpc *> (mgtRpc->Name.c_str(), mgtRpc));
    mgtRpc->Module.assign(this->Name);
    clLogDebug("MGT", "RPC", "RPC [%s] added successful!", mgtRpc->Name.c_str());

    return rc;
  }

  ClRcT ClMgtModule::removeMgtRpc(std::string rpcName)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtRpcs.find(rpcName) == mMgtRpcs.end())
      {
        clLogDebug("MGT", "RPC", "RPC [%s] does not exist!", rpcName.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtRpcs.erase(rpcName);
    clLogDebug("MGT", "RPC", "RPC [%s] removed successful!", rpcName.c_str());

    return rc;
  }

  ClMgtRpc *ClMgtModule::getMgtRpc(const std::string rpcName)
  {
    map<string, ClMgtRpc*>::iterator mapIndex = mMgtRpcs.find(rpcName);
    if (mapIndex != mMgtRpcs.end())
      {
        return static_cast<ClMgtRpc *>((*mapIndex).second);
      }
    return NULL;
  }
}
