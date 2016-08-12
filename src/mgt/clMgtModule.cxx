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
#include <clCommonErrors6.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;


namespace SAFplus
{
  MgtModule::MgtModule(const char* nam)
  {
    // name.assign(nam);
    tag.assign(nam);
    mgtDb = nullptr;
  }

  MgtModule::~MgtModule()
  {
  }

  void MgtModule::initialize()
  {
  }

  ClRcT MgtModule::bind(Handle handle)
  {
    return MgtRoot::getInstance()->loadMgtModule(handle, this, this->tag);
  }

  ClRcT MgtModule::bind(Handle handle, MgtObject* obj)
  {
    // Binding from its children
    if (this == obj)
    {
      std::map<std::string, MgtObject*>::iterator iter;
      std::map<std::string, MgtObject*>::iterator endd = children.end();
      for (iter = children.begin(); iter != endd; ++iter)
      {
        MgtObject *childObj = iter->second;
        MgtRoot::getInstance()->bind(handle, childObj);
      }
    }
    else
    {
      MgtRoot::getInstance()->bind(handle, obj);
    }
  }

  ClRcT MgtModule::registerRpc(Handle handle, MgtRpc* obj)
  {
      MgtRoot::getInstance()->registerRpc(handle, obj);
  }

  ClRcT MgtModule::addMgtObject(MgtObject *mgtObject, const std::string& route)
  {
    ClRcT rc = CL_OK;

    if (mgtObject == nullptr)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT object already exists in the database */
    if (children.find(route) != children.end())
      {
        logDebug("MGT", "ROUTE", "Route [%s] already exists!", route.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    if (route[0] == '/')
      {
      logWarning("MGT", "ROUTE", "Route [%s] has a preceding /", route.c_str());

      }

    /* Insert MGT object into the database */
    children.insert(pair<string, MgtObject *> (route.c_str(), mgtObject));
    logDebug("MGT", "ROUTE", "Route [%s] added successfully!", route.c_str());

    return rc;
  }

  ClRcT MgtModule::removeMgtObject(const std::string& route)
  {
    ClRcT rc = CL_OK;

    /* Check if MGT module already exists in the database */
    if (children.find(route) == children.end())
      {
        logDebug("MGT", "ROUTE", "Routing [%s] does not exist!", route.c_str());
        return CL_ERR_NOT_EXIST;
      }

    /* Remove MGT module out off the database */
    children.erase(route);
    logDebug("MGT", "ROUTE", "Routing [%s] removed successful!", route.c_str());

    return rc;
  }

  MgtObject *MgtModule::getMgtObject(const std::string& route)
  {
    map<string, MgtObject*>::iterator mapIndex = children.find(route);
    if (mapIndex != children.end())
      {
        return static_cast<MgtObject *>((*mapIndex).second);
      }
    return nullptr;
  }

  void MgtModule::dbgDump()
  {
    std::stringstream dumpStrStream;
    
    std::map<std::string, MgtObject*>::iterator iter;
    std::map<std::string, MgtObject*>::iterator endd = children.end();
    for (iter = children.begin(); iter != endd; iter++)
      {
        dumpStrStream << iter->first << " ";
      }

    printf("%s\n", dumpStrStream.str().c_str());
    logDebug("MGT", "DUMP", "%s", dumpStrStream.str().c_str());
  }

  void MgtModule::dbgDumpChildren()
  {
    std::stringstream dumpStrStream;
    std::map<std::string, MgtObject*>::iterator iter;
    std::map<std::string, MgtObject*>::iterator endd = children.end();
    for (iter = children.begin(); iter != endd; iter++)
      {
        MgtObject* obj = iter->second;
        obj->toString(dumpStrStream);
      }
    printf("%s\n", dumpStrStream.str().c_str());
    logDebug("MGT", "DUMP", "%s", dumpStrStream.str().c_str());
  }

MgtDatabase* MgtModule::getDb(void)
{
  return mgtDb;
}


#if 0
  void MgtModule::resolvePath(const char* path, std::vector<MgtObject*>* result)
  {
    std::string xpath(path);
    size_t idx = xpath.find_first_of("/[", 0);
    std::string child = xpath.substr(0, idx);

    map<string, MgtObject*>::iterator objref = children.find(child);
    if (objref == children.end()) return;
    MgtObject *object = static_cast<MgtObject *>((*objref).second);
    if (idx == string::npos) // this was the end of the string
      {
        result->push_back(object);
        return;
      }
 
    if (xpath[idx] == '/')
      {
        std::string rest = xpath.substr(idx + 1);
        object->resolvePath(rest.c_str(), result);
      }
    else if (xpath[idx] == '[')
      {
        std::string rest = xpath.substr(idx);
        object->resolvePath(rest.c_str(), result);
      }
    else  // Its an array or other complex entity
      {
        clDbgNotImplemented("complex access at module level");
      }

  }
#endif

  MgtObject *MgtModule::findMgtObject(const std::string& xpath)
  {
  size_t idx = xpath.find_first_of("/[", 0);
  std::string child = xpath.substr(0, idx);

  map<string, MgtObject*>::iterator objref = children.find(child);
  if (objref == children.end()) return nullptr;
  MgtObject *object = static_cast<MgtObject *>((*objref).second);
  if (idx == string::npos) // this was the end of the string
    return object;
 
  if (xpath[idx] == '/')
    {
    std::string rest = xpath.substr(idx + 1);
    std::vector<MgtObject*> matches;
    object->resolvePath(rest.c_str(), &matches);
    int temp = matches.size();
    if (temp == 0) return nullptr;
    if (temp == 1) return matches[0];
    else
      {
      clDbgNotImplemented("complex access at module level");
      }
    }
  else  // Its an array or other complex entity
    {
      clDbgNotImplemented("complex access at module level");
      return nullptr;
    }

#if 0
    for (map<string, MgtObject*>::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject *object = static_cast<MgtObject *>((*it).second);
        std::string objXpath = object->getFullXpath();

        if (xpath.find(objXpath) == 0)
          {
            if (xpath.length() == objXpath.length())
              {
                return object;
              }
            else
              {
                if (xpath[objXpath.length()] != '/')
                  continue;

                MgtObject *findObj = object->findMgtObject(xpath, objXpath.length());

                if (findObj)
                  return findObj;

              }
          }
      }
#endif
    return nullptr;
  }

  ClRcT MgtModule::addMgtNotify(MgtNotify *mgtNotify)
  {
    ClRcT rc = CL_OK;

    if (mgtNotify == nullptr)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT notification already exists in the database */
    if (mMgtNotifies.find(mgtNotify->tag) != mMgtNotifies.end())
      {
        logDebug("MGT", "NOT", "Notify [%s] is already existing!", mgtNotify->tag.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT notification into the database */
    mMgtNotifies.insert(pair<string, MgtNotify *> (mgtNotify->tag.c_str(), mgtNotify));
    mgtNotify->Module.assign(this->tag);

    logDebug("MGT", "NOT", "Notify [%s] added successful!", mgtNotify->tag.c_str());

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
    return nullptr;
  }

  ClRcT MgtModule::addMgtRpc(MgtRpc *mgtRpc)
  {
    ClRcT rc = CL_OK;

    if (mgtRpc == nullptr)
      {
        return CL_ERR_NULL_POINTER;
      }

    /* Check if MGT RPC already exists in the database */
    if (mMgtRpcs.find(mgtRpc->tag) != mMgtRpcs.end())
      {
        logDebug("MGT", "RPC", "RPC [%s] is already existing!", mgtRpc->tag.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    /* Insert MGT RPC into the database */
    mMgtRpcs.insert(pair<string, MgtRpc *> (mgtRpc->tag.c_str(), mgtRpc));
    mgtRpc->Module.assign(this->tag);
    logDebug("MGT", "RPC", "RPC [%s] added successful!", mgtRpc->tag.c_str());

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
    return nullptr;
  }
}
