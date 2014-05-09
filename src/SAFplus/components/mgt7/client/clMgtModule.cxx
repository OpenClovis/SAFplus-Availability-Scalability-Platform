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

#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;


namespace SAFplus
{
  MgtModule::MgtModule(const char* nam)
  {
    name.assign(nam);
  }

  MgtModule::~MgtModule()
  {
  }

  void MgtModule::initialize()
  {
  }

  ClRcT MgtModule::loadModule()
  {
    return MgtRoot::getInstance()->loadMgtModule(this, this->name);
  }

  ClRcT MgtModule::addMgtObject(MgtObject *mgtObject, const std::string route)
  {
    ClRcT rc = CL_OK;

    if (mgtObject == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT object already exists in the database */
    if (mMgtObjects.find(route) != mMgtObjects.end())
      {
        logDebug("MGT", "ROUTE", "Route [%s] already exists!", route.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT object into the database */
    mMgtObjects.insert(pair<string, MgtObject *> (route.c_str(), mgtObject));
    logDebug("MGT", "ROUTE", "Route [%s] added successfully!", route.c_str());

    return rc;
  }

  ClRcT MgtModule::removeMgtObject(const std::string& route)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtObjects.find(route) == mMgtObjects.end())
      {
        logDebug("MGT", "ROUTE", "Routing [%s] does not exist!", route.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtObjects.erase(route);
    logDebug("MGT", "ROUTE", "Routing [%s] removed successful!", route.c_str());

    return rc;
  }

  MgtObject *MgtModule::getMgtObject(const std::string& route)
  {
    map<string, MgtObject*>::iterator mapIndex = mMgtObjects.find(route);
    if (mapIndex != mMgtObjects.end())
      {
        return static_cast<MgtObject *>((*mapIndex).second);
      }
    return NULL;
  }

  ClRcT MgtModule::addMgtNotify(MgtNotify *mgtNotify)
  {
    ClRcT rc = CL_OK;

    if (mgtNotify == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT notification already exists in the database */
    if (mMgtNotifies.find(mgtNotify->name) != mMgtNotifies.end())
      {
        logDebug("MGT", "NOT", "Notify [%s] is already existing!", mgtNotify->name.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT notification into the database */
    mMgtNotifies.insert(pair<string, MgtNotify *> (mgtNotify->name.c_str(), mgtNotify));
    mgtNotify->Module.assign(this->name);

    logDebug("MGT", "NOT", "Notify [%s] added successful!", mgtNotify->name.c_str());

    return rc;
  }

  ClRcT MgtModule::removeMgtNotify(const std::string& notifyName)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtNotifies.find(notifyName) == mMgtNotifies.end())
      {
        logDebug("MGT", "NOT", "Notify [%s] does not exist!", notifyName.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtNotifies.erase(notifyName);
    logDebug("MGT", "NOT", "Notify [%s] removed successful!", notifyName.c_str());

    return rc;
  }

  MgtNotify *MgtModule::getMgtNotify(const std::string& notifyName)
  {
    map<string, MgtNotify*>::iterator mapIndex = mMgtNotifies.find(notifyName);
    if (mapIndex != mMgtNotifies.end())
      {
        return static_cast<MgtNotify *>((*mapIndex).second);
      }
    return NULL;
  }

  ClRcT MgtModule::addMgtRpc(MgtRpc *mgtRpc)
  {
    ClRcT rc = CL_OK;

    if (mgtRpc == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT RPC already exists in the database */
    if (mMgtRpcs.find(mgtRpc->name) != mMgtRpcs.end())
      {
        logDebug("MGT", "RPC", "RPC [%s] is already existing!", mgtRpc->name.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT RPC into the database */
    mMgtRpcs.insert(pair<string, MgtRpc *> (mgtRpc->name.c_str(), mgtRpc));
    mgtRpc->Module.assign(this->name);
    logDebug("MGT", "RPC", "RPC [%s] added successful!", mgtRpc->name.c_str());

    return rc;
  }

  ClRcT MgtModule::removeMgtRpc(const std::string& rpcName)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (mMgtRpcs.find(rpcName) == mMgtRpcs.end())
      {
        logDebug("MGT", "RPC", "RPC [%s] does not exist!", rpcName.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    mMgtRpcs.erase(rpcName);
    logDebug("MGT", "RPC", "RPC [%s] removed successful!", rpcName.c_str());

    return rc;
  }

  MgtRpc *MgtModule::getMgtRpc(const std::string& rpcName)
  {
    map<string, MgtRpc*>::iterator mapIndex = mMgtRpcs.find(rpcName);
    if (mapIndex != mMgtRpcs.end())
      {
        return static_cast<MgtRpc *>((*mapIndex).second);
      }
    return NULL;
  }
}
