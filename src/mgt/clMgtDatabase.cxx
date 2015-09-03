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

#include <clCommon.hxx>
#include "clMgtDatabase.hxx"
#include "clLogApi.hxx"
#include "MgtMsg.pb.hxx"

namespace SAFplus
{
  static __inline__ ClUint32T getHashKeyFn(const ClCharT *keyStr)
  {
    ClUint32T cksum = SAFplus::computeCrc32((ClUint8T*) keyStr, (ClUint32T) strlen(keyStr));
    return cksum & DBAL_DB_KEY_MASK;
  }

  MgtDatabase *MgtDatabase::singletonInstance = 0;

  MgtDatabase * MgtDatabase::getInstance()
  {
    return (singletonInstance ? singletonInstance : (singletonInstance = new MgtDatabase()));
  }

  void MgtDatabase::DestroyInstance()
  {
    delete singletonInstance;
    singletonInstance = 0;
  }

  MgtDatabase::~MgtDatabase()
  {
    finalizeDB();
  }

  MgtDatabase::MgtDatabase()
  {
    mInitialized = CL_FALSE;
  }

  ClRcT MgtDatabase::initializeDB(const std::string &dbName, ClUint32T maxKeySize, ClUint32T maxRecordSize)
  {
    ClRcT rc = CL_OK;
    ClDBHandleT dbDataHdl = 0; /* Database handle*/

    std::string dbNameData = "";
    std::string dbNameIdx = "";

    /*Initialize dbal if not initialized*/
    rc = clDbalLibInitialize();
    if (CL_OK != rc)
      {
        logDebug("MGT", "DBAL", "Dbal lib initialized failed [%x]", rc);
        return rc;
      }

    /* Open the data DB */
    dbNameData.append(dbName).append(".db");
    rc = clDbalOpen(dbNameData.c_str(), dbNameData.c_str(), CL_DB_APPEND, maxKeySize, maxRecordSize, &dbDataHdl);
    if (CL_OK != rc)
      {
        goto exitOnError1;
      }

    mDbDataHdl = dbDataHdl;
    mInitialized = CL_TRUE;
    return rc;

    exitOnError1: clDbalLibFinalize();
    return rc;
  }

  ClRcT MgtDatabase::finalizeDB()
  {
    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    /* Close the data DB */
    clDbalClose(mDbDataHdl);
    mDbDataHdl = 0;

    /*Finalize dbal */
    clDbalLibFinalize();
    return CL_OK;
  }

  ClRcT MgtDatabase::setRecordByDbValue(const std::string &key, const Mgt::Msg::MsgMgtDb &dbValue)
  {
    ClRcT rc = CL_OK;
    ClUint32T hashKey = getHashKeyFn(key.c_str());
    std::string strVal;

    // Marshall data
    dbValue.SerializeToString(&strVal);
    rc = clDbalRecordReplace(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());
    return rc;
  }

  ClRcT MgtDatabase::setRecord(const std::string &key, const std::string &value)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    //get record before replace
    Mgt::Msg::MsgMgtDb dbValue;
    getRecordDbValue(key, dbValue);

    dbValue.set_value(value);
    dbValue.set_xpath(key);

    rc = setRecordByDbValue(key, dbValue);
    return rc;
  }

  ClRcT MgtDatabase::getRecordDbValue(const std::string &key, Mgt::Msg::MsgMgtDb &dbValue)
  {
    ClRcT rc = CL_OK;
    ClCharT *cvalue;
    ClUint32T dataSize = 0;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    ClUint32T hashKey = getHashKeyFn(key.c_str());

    rc = clDbalRecordGet(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT*) &cvalue, &dataSize);
    if (CL_OK != rc)
      {
        return rc;
      }

    std::string strVal;
    strVal.assign(cvalue, dataSize);

    // De-marshall data
    dbValue.ParseFromString(strVal);
    SAFplusHeapFree(cvalue);
    return rc;
  }

  ClRcT MgtDatabase::getRecord(const std::string &key, std::string &value)
  {
    ClRcT rc = CL_OK;

    Mgt::Msg::MsgMgtDb dbValue;
    rc = getRecordDbValue(key, dbValue);
    if (CL_OK != rc)
      {
        return rc;
      }

    ClUint32T hashKey = getHashKeyFn(key.c_str());
    value.assign(dbValue.value());

    logInfo("MGT", "DBR", "Record [0x%x]: [%s] -> [%s]", hashKey, key.c_str(), value.c_str());
    return rc;
  }

  ClRcT MgtDatabase::insertByDbValue(const std::string &key,
      const Mgt::Msg::MsgMgtDb &dbValue)
  {
    ClRcT rc = CL_OK;

    ClUint32T hashKey = getHashKeyFn(key.c_str());
    std::string strVal;
    // Marshall data
    dbValue.SerializeToString(&strVal);
    /*
     * Insert into data table
     */
    rc = clDbalRecordInsert(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());
    return rc;
  }

  void MgtDatabase::insertToParentRecord(const std::string &key)
  {
    ClRcT rc = CL_OK;
    std::string childName;
    std::string parentKey;
    std::size_t found = key.find_last_of("/");
    if (found == std::string::npos || found == 0)
      {
        return;
      }

    if (key[key.length() - 1] == ']')
      {
        found = key.find_last_of("[");
        if (found != std::string::npos)
          {
            childName = key.substr(found);
            parentKey = key.substr(0, found);
          }
      }
    else
      {
        childName = key.substr(found + 1);
        parentKey = key.substr(0, found);
      }

    if (std::find(listKey.begin(), listKey.end(), parentKey)!= listKey.end())
      {
        Mgt::Msg::MsgMgtDb dbValue;
        rc = getRecordDbValue(parentKey, dbValue);
        if (rc == CL_OK)
          {
            //check if this key already exists
            ClBoolT found;
            for (int i = 0; i < dbValue.children_size(); ++i)
              {
                if(childName == dbValue.children(i))
                  {
                    found = true;
                    break;
                  }
              }
            if(!found)
              {
                dbValue.add_children(childName);
                rc = setRecordByDbValue(parentKey, dbValue);
              }
          }
        //					else
        //						{
        //							//remove the key from listKey
        //							auto newEnd = std::remove(listKey.begin(), listKey.end(), parentKey);
        //							listKey.erase(newEnd);
        //						}
      }
    else
      {
        Mgt::Msg::MsgMgtDb dbValue;
        dbValue.set_xpath(parentKey);
        dbValue.set_value("");
        dbValue.add_children(childName);

        rc = insertByDbValue(parentKey, dbValue);
        if (CL_OK == rc)
          {
            listKey.push_back(parentKey);
          }
      }

    insertToParentRecord(parentKey);
  }

  ClRcT MgtDatabase::insertRecord(const std::string &key, const std::string &value)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    //insert this key to parent
    insertToParentRecord(key);
    Mgt::Msg::MsgMgtDb dbValue;
    dbValue.set_value(value);
    dbValue.set_xpath(key);

    /*
     * Insert into data table
     */
    rc = insertByDbValue(key, dbValue);
    return rc;
  }

  ClRcT MgtDatabase::deleteRecord(const std::string &key)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    ClUint32T hashKey = getHashKeyFn(key.c_str());

    rc = clDbalRecordDelete(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey));

    return rc;
  }

  void MgtDatabase::getXpathList(const std::string &xpath, std::vector<std::string> &xpathList)
  {
    ClRcT rc = CL_OK;
    Mgt::Msg::MsgMgtDb dbValue;
    rc = getRecordDbValue(xpath, dbValue);
    if (CL_OK != rc)
      {
        return;
      }

    if (!dbValue.value().empty())
      {
        xpathList.push_back(xpath);
      }
    for (int i = 0; i < dbValue.children_size(); ++i)
      {
        std::string childXpath = xpath;
        std::string child = dbValue.children(i);
        if (child[child.length() - 1] == ']')
          {
            childXpath.append(dbValue.children(i));
          }
        else
          {
            childXpath.append(std::string("/").append(child));
          }
        getXpathList(childXpath, xpathList);
      }
  }

  std::vector<std::string> MgtDatabase::iterate(const std::string &xpath, bool keyOnly)
  {
    ClRcT rc = CL_OK;
    std::vector<std::string> iter;
    std::vector<std::string> xpathList;
    std::string path = xpath;

    //check to remove the last "/" character
    if(path[path.length() - 1] == '/')
      {
        path.substr(0, path.length() - 2);
      }

    getXpathList(path, xpathList);
    if(xpathList.size() > 0)
      {
        for (auto it = xpathList.begin(); it != xpathList.end(); ++it)
          {
            logInfo("MGT", "DBR", "Iterate : %s ", (*it).c_str());
            if (keyOnly)
              {
                // /a/b[@key"1"]/@key
                std::size_t found = (*it).find_last_of("/@");
                if ((found != std::string::npos) && (found != 0) && (*it)[found - 2] == ']')
                  {
                    iter.push_back((*it));
                  }
              }
            else
              {
                iter.push_back((*it));
              }
          }
      }

    return iter;
  }

  ClBoolT MgtDatabase::isInitialized()
  {
    return mInitialized;
  }
};
