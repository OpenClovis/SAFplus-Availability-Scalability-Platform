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
    mapParentKey.clear();
    mapKeyValue.clear();
  }

  ClRcT MgtDatabase::initializeDB(const std::string &dbName, ClUint32T maxKeySize, ClUint32T maxRecordSize)
  {
    ClRcT rc = CL_OK;
    ClDBHandleT dbDataHdl = 0; /* Database handle*/

    std::string dbNameData = "";
    std::string dbNameIdx = "";

    /*Check if DB already exists */
    std::map<std::string, ClDBHandleT>::iterator dbh;
    rc = checkIfDBOpened(dbName, dbh);
    if (rc == CL_OK)
      {
        logInfo("MGT", "DBAL", "DB Name [%s] was already initialized", dbName.c_str());
        return rc;
      }

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
        logDebug("MGT", "DBAL", "Open Dbal [%s] failed -> finializedDB", dbName.c_str());
        goto exitOnError1;
      }

    databaseList.insert(std::make_pair(dbName, dbDataHdl));
    mDbDataHdl = dbDataHdl;
    mDbName = dbName;
    mInitialized = CL_TRUE;
    return rc;

    exitOnError1: finalizeDB();
    return rc;
  }

  ClRcT MgtDatabase::finalizeDB()
  {
    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    /* Close all the data DB */
    for (auto it = databaseList.begin(); it != databaseList.end(); ++it)
      {
        clDbalClose(it->second);
      }
    mDbName = "";
    mDbDataHdl = 0;
    databaseList.clear();
    mapParentKey.clear();
    mapKeyValue.clear();

    /*Finalize dbal */
    clDbalLibFinalize();
    return CL_OK;
  }

  ClRcT MgtDatabase::checkIfDBOpened(const std::string &dbName, std::map<std::string, ClDBHandleT>::iterator &dbh)
  {
    ClRcT rc = CL_OK;
    dbh = databaseList.find(dbName);
    if (dbh == databaseList.end())
      {
        //not opened
        rc = !CL_OK;
      }
    return rc;
  }

  ClRcT MgtDatabase::switchDB(const std::string &dbName)
  {
    ClRcT rc = CL_OK;
    if (dbName == mDbName) return rc;

    std::map<std::string, ClDBHandleT>::iterator dbh;
    rc = checkIfDBOpened(dbName, dbh);
    if (rc != CL_OK)
      {
        return rc;
      }

    logInfo("MGT", "DBR", "Switch from %s to %s ", mDbName.c_str(), dbName.c_str());
    mDbName = dbName;
    mDbDataHdl = dbh->second;
    mapKeyValue.clear();
    mapParentKey.clear();

    return rc;
  }

  void MgtDatabase::closeDB()
  {
    closeDB(mDbName);
  }

  void MgtDatabase::closeDB(const std::string &dbName)
  {
    std::map<std::string, ClDBHandleT>::iterator dbh;
    ClRcT rc = checkIfDBOpened(dbName, dbh);
    if (rc != CL_OK)
      {
        logInfo("MGT", "DBAL", "DB Name [%s] was not opened yet", dbName.c_str());
        return;
      }

    if (dbName == mDbName)
      {
        mapKeyValue.clear();
        mapParentKey.clear();
      }
    clDbalClose(dbh->second);
    databaseList.erase(dbName);
  }

  ClRcT MgtDatabase::setRecord(const std::string &key, const std::string &value)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    Mgt::Msg::MsgMgtDb dbValue;
    dbValue.set_value(value);
    dbValue.set_xpath(key);

    ClUint32T hashKey = getHashKeyFn(key.c_str());
    std::string strVal;

    // Marshall data
    dbValue.SerializeToString(&strVal);
    rc = clDbalRecordReplace(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());
    if (rc == CL_OK && dataLoaded())
      {
        mapKeyValue[key] = value;
      }
    return rc;
  }

  ClRcT MgtDatabase::getRecord(const std::string &key, std::string &value)
  {
    ClRcT rc = CL_OK;

    Mgt::Msg::MsgMgtDb dbValue;
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
    if (CL_OK != rc)
      {
        return rc;
      }

    value.assign(dbValue.value());
    logInfo("MGT", "DBR", "Record [%s] -> [%s]", key.c_str(), value.c_str());
    return rc;
  }

  ClRcT MgtDatabase::insertRecord(const std::string &key, const std::string &value)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    ClUint32T hashKey = getHashKeyFn(key.c_str());
    Mgt::Msg::MsgMgtDb dbValue;
    dbValue.set_value(value);
    dbValue.set_xpath(key);
    std::string strVal;

    // Marshall data
    dbValue.SerializeToString(&strVal);
    /*
     * Insert into data table
     */
    rc = clDbalRecordInsert(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());

    if (rc == CL_OK && dataLoaded())
      {
        mapKeyValue[key] = value;
        insertToParentKey(key);
      }

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
    if (rc == CL_OK && dataLoaded())
      {
        mapKeyValue.erase(key);
      }

    return rc;
  }

  ClBoolT MgtDatabase::dataLoaded()
  {
    return (mapKeyValue.size() > 0);
  }

  void MgtDatabase::insertToParentKey(const std::string &key)
  {
    std::string childName;
    std::string parentKey;
    std::size_t found = key.find_last_of("/");
    if (found == std::string::npos || key.length() <= 1)
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
        parentKey = (found != 0) ? key.substr(0, found) : key.substr(0, found + 1);
      }

    if (mapParentKey.find(parentKey) != mapParentKey.end())
      {
        std::vector<std::string> listKey = mapParentKey[parentKey];
        if(std::find(listKey.begin(), listKey.end(), childName) == listKey.end())
          {
            listKey.push_back(childName);
            mapParentKey[parentKey] = listKey;
          }
      }
    else
      {
        std::vector<std::string> listKey;
        listKey.push_back(childName);
        mapParentKey[parentKey] = listKey;
      }
    insertToParentKey(parentKey);
  }

  void MgtDatabase::insertToIterator(const std::string &xpath, std::vector<std::string> &iter, bool keyOnly)
  {
    if (keyOnly)
      {
        std::size_t found = (xpath).find_last_of("/@");
        if ((found != std::string::npos) && (found != 0) && (xpath)[found - 2] == ']')
          {
            iter.push_back((xpath));
          }
      }
    else
      {
        iter.push_back(xpath);
      }
  }

  void MgtDatabase::loadData()
  {
    ClUint32T keySize = 0;
    ClUint32T dataSize = 0;
    ClUint32T nextKeySize = 0;
    ClUint32T *recKey = nullptr;
    ClUint32T *nextKey = nullptr;
    ClCharT *recData = nullptr;
    ClRcT rc = CL_OK;

    /*
     * Iterators key value
     */
    rc = clDbalFirstRecordGet(mDbDataHdl, (ClDBKeyT*) &recKey, &keySize, (ClDBRecordT*) &recData, &dataSize);
    if (rc != CL_OK)
      {
        return;
      }

    while (1)
      {
        Mgt::Msg::MsgMgtDb dbValue;
        std::string strVal;
        strVal.append(recData, dataSize);

        // Free memory
        SAFplusHeapFree(recData);

        // De-marshall data
        dbValue.ParseFromString(strVal);

        mapKeyValue[dbValue.xpath()] = dbValue.value();
        insertToParentKey(dbValue.xpath());

        logInfo("MGT", "LST", "Read [%s]", dbValue.xpath().c_str());

        if ((rc = clDbalNextRecordGet(mDbDataHdl, (ClDBKeyT) recKey, keySize, (ClDBKeyT*) &nextKey, &nextKeySize, (ClDBRecordT*) &recData, &dataSize)) != CL_OK)
          {
            rc = CL_OK;
            break;
          }
        recKey = nextKey;
        keySize = nextKeySize;
      }
  }

  void MgtDatabase::iterateParentKey(const std::string &xpath, std::vector<std::string> &iter, bool keyOnly)
  {
    if (mapKeyValue.find(xpath) != mapKeyValue.end())
      {
        insertToIterator(xpath, iter, keyOnly);
        return;
      }

    std::vector<std::string> childList = mapParentKey[xpath];
    for (auto it = childList.begin(); it != childList.end(); ++it)
      {
        std::string path = xpath;
        std::string childName = *it;
        if (childName[childName.length() - 1] == ']' || path[path.length() - 1] == '/')
          {
            path.append(childName);
          }
        else
          {
            path.append(std::string("/").append(childName));
          }

        if (mapKeyValue.find(path) != mapKeyValue.end())
          {
            insertToIterator(path, iter, keyOnly);
          }
        else
          {
            iterateParentKey(path, iter, keyOnly);
          }
      }
  }

  std::vector<std::string> MgtDatabase::iterate(const std::string &xpath, bool keyOnly)
  {
    std::vector<std::string> iter;
    std::string path = xpath;
    if (!dataLoaded())
      {
        loadData();
      }
    iterateParentKey(xpath, iter, keyOnly);

    return iter;
  }

  ClBoolT MgtDatabase::isInitialized()
  {
    return mInitialized;
  }
};
