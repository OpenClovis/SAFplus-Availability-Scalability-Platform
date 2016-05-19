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
#include <clMgtDatabase.hxx>
#include <clLogApi.hxx>
#include <clMgtApi.hxx>

#define MURMURKEY 0xc107deed

namespace SAFplus
{

  typedef struct 
  {
    uint64_t num[2];
  } HashKey;

#if 0
  static __inline__ ClUint32T getHashKeyFn(const ClCharT *keyStr)
  {
    ClUint32T cksum = SAFplus::computeCrc32((ClUint8T*) keyStr, (ClUint32T) strlen(keyStr));
    return cksum & DBAL_DB_KEY_MASK;
  }
#endif
  static __inline__ HashKey getHashKeyFn(const std::string& s)
  {
    HashKey k;
    murmurHash3_128 ( s.c_str(), s.length(), MURMURKEY, (void*) &k );
    return k;
  }


  //MgtDatabase *MgtDatabase::singletonInstance = 0;
#if 0
  MgtDatabase * MgtDatabase::getInstance()
  {
    return (singletonInstance ? singletonInstance : (singletonInstance = new MgtDatabase()));
  }

  void MgtDatabase::DestroyInstance()
  {
    delete singletonInstance;
    singletonInstance = 0;
  }
#endif

  MgtDatabase::~MgtDatabase()
  {
    finalize();
  }

  MgtDatabase::MgtDatabase()
  {
    mInitialized = CL_FALSE;
  }

  ClRcT MgtDatabase::initialize(const std::string &dbName, const std::string &dbPlugin, ClUint32T maxKeySize, ClUint32T maxRecordSize)
  {
    ClRcT rc = CL_OK;
    //ClDBHandleT dbDataHdl = 0; /* Database handle*/

#if 0  // always should be initialized in safplusInitialize function
    /*Initialize dbal if not initialized*/
    rc = clDbalLibInitialize();
    if (CL_OK != rc)
      {
        logDebug("MGT", "DBAL", "Dbal lib initialized failed [%x]", rc);
        return rc;
      }
#endif

    std::string dbNameData = "";
    std::string dbNameIdx = "";

    //? <cfg name="SAFPLUS_MGT_DB">[OPTIONAL] Specifies the management database plugin</cfg>
    if ((&dbPlugin==nullptr)||(dbPlugin == ""))
      {
        char* db = getenv("SAFPLUS_MGT_DB");
        mDbDataHdl = clDbalObjCreate(db);  // If NULL, we'll choose the default db
      }
    else
      {
        mDbDataHdl = clDbalObjCreate(dbPlugin.c_str());
      }
    assert(mDbDataHdl);

    /* Open the data DB */
    dbNameData.append(dbName).append(".db");
    unsigned int flags = (pluginFlags << 8) | CL_DB_APPEND;
    rc = mDbDataHdl->open(dbNameData.c_str(), dbNameData.c_str(), flags, maxKeySize, maxRecordSize);
    if (CL_OK != rc)
      {
        goto exitOnError1;
      }

    //mDbDataHdl = dbDataHdl;
    mInitialized = CL_TRUE;
    return rc;

    exitOnError1: //clDbalLibFinalize();
    return rc;
  }

  ClRcT MgtDatabase::finalize()
  {
    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    /* Close the data DB */
    //clDbalClose(mDbDataHdl);
    delete mDbDataHdl;
    mDbDataHdl=NULL;

    /*Finalize dbal */
    //clDbalLibFinalize();
    return CL_OK;
  }

  ClRcT MgtDatabase::write2DB(const std::string &key, const std::string &value, std::vector<std::string> *child, bool overwrite)
    {
      ClRcT rc = CL_OK;

      HashKey hashKey = getHashKeyFn(key);

      Mgt::Msg::MsgMgtDb dbValue;
      dbValue.set_value(value);
      dbValue.set_xpath(key);

      // Store metadata for child xpath
      if (child != nullptr)
        {
          for (std::vector<std::string>::iterator i = child->begin(); i != child->end(); i++)
           {
              dbValue.add_child(*i);
           }
        }

      std::string strVal;
      // Marshall data
      dbValue.SerializeToString(&strVal);

      logInfo("MGT", "DBR", "Write DB record [0x%" PRIx64 "%" PRIx64 "]: [%s] -> [%s]", hashKey.num[0],hashKey.num[1], key.c_str(), value.c_str());

      if (overwrite)
        {
          /*
           * Update value
           */
          rc = mDbDataHdl->replaceRecord((ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());
        }
      else
        {
          /*
           * Insert into data table
           */
          rc = mDbDataHdl->insertRecord((ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) strVal.c_str(), strVal.size());
        }
      return rc;
    }

  ClRcT MgtDatabase::insertRecord(const std::string &key, const std::string &value, std::vector<std::string> *child)
  {
    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }
    return write2DB(key, value, child);
  }

  ClRcT MgtDatabase::setRecord(const std::string &key, const std::string &value, std::vector<std::string> *child)
  {
    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }
    return write2DB(key, value, child, true);
  }

  ClRcT MgtDatabase::getRecord(const std::string &key, std::string &value, std::vector<std::string> *child)
  {
    ClRcT rc = CL_OK;
    ClCharT *cvalue;
    ClUint32T dataSize = 0;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    HashKey hashKey = getHashKeyFn(key);

    rc = mDbDataHdl->getRecord((ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT*) &cvalue, &dataSize);
    if (CL_OK != rc)
      {
        return rc;
      }

    Mgt::Msg::MsgMgtDb dbValue;
    std::string strVal;
    strVal.append(cvalue, dataSize);

    // De-marshall data
    dbValue.ParseFromString(strVal);
    value.assign(dbValue.value());

    if (child != nullptr && dbValue.child_size() > 0)
    {
      for (int j = 0; j < dbValue.child_size(); j++)
        {
          child->push_back(dbValue.child(j));
        }
    }

    logInfo("MGT", "DBR", "Record [0x%" PRIx64 "%" PRIx64 "]: [%s] -> [%s]", hashKey.num[0], hashKey.num[1], key.c_str(), value.c_str());
    SAFplusHeapFree(cvalue);
    return rc;
  }

  ClRcT MgtDatabase::deleteRecord(const std::string &key)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    HashKey hashKey = getHashKeyFn(key);

    rc = mDbDataHdl->deleteRecord((ClDBKeyT) &hashKey, sizeof(hashKey));

    return rc;
  }

  void MgtDatabase::loadDb(std::vector<std::string> &result)
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
    rc = mDbDataHdl->getFirstRecord((ClDBKeyT*) &recKey, &keySize, (ClDBRecordT*) &recData, &dataSize);
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

        // Ignore metadata key xpath
        if (dbValue.child_size() > 0)
          {
            logDebug("MGT", "LOAD", "Ignore metadata key [%s]", dbValue.xpath().c_str());
          }
        else
          {
            result.push_back(dbValue.xpath());
            logDebug("MGT", "LOAD", "Read [%s]", dbValue.xpath().c_str());
          }

        if ((rc = mDbDataHdl->getNextRecord((ClDBKeyT) recKey, keySize, (ClDBKeyT*) &nextKey, &nextKeySize, (ClDBRecordT*) &recData, &dataSize)) != CL_OK)
          {
            rc = CL_OK;
            break;
          }
        recKey = nextKey;
        keySize = nextKeySize;
      }
  }

  void MgtDatabase::iterate(const std::string &xpath, std::vector<std::string> &result )
    {
      if (xpath[0] == '/' and xpath.length() == 1) // Get whole db
        {
          return loadDb(result);
        }

      // Iterator through child (depth = 1)
      std::string value;
      std::vector<std::string> child;

      logDebug("MGT", "ITER", "Trying [%s] ", xpath.c_str());
      ClRcT rc = getRecord(xpath, value, &child);
      if (rc == CL_OK)
        {
          if (child.size() > 0)
            {
              // Get all its child (depth = 1)
              for (std::vector<std::string>::iterator i = child.begin(); i != child.end(); i++)
                {
                  std::string childxpath(xpath);
                  if ((*i)[0] == '[') // Key value
                    childxpath.append(*i);
                  else
                    childxpath.append("/").append(*i);

                  logDebug("MGT", "ITER", "Lookup child [%s] ", childxpath.c_str());
                  result.push_back(childxpath);
                }
            }
          else
            {
              result.push_back(xpath);
            }
        }
    }

  ClBoolT MgtDatabase::isInitialized()
  {
    return mInitialized;
  }
};
