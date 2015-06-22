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
#ifdef __cplusplus
extern "C"
{
#endif
//#include <clCommonErrors.h>
//#include <clDebugApi.h>
//#include <clCksmApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

#define DBAL_DB_KEY_BITS (32ULL)
#define DBAL_DB_KEY_SIZE (1ULL << DBAL_DB_KEY_BITS)
#define DBAL_DB_KEY_MASK (DBAL_DB_KEY_SIZE - 1ULL)

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

  MgtDatabase::~MgtDatabase()
  {

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

    /*
     * Open the index DB
     */
    dbNameIdx.append(dbName).append(".idx");
    rc = clDbalOpen(dbNameIdx.c_str(), dbNameIdx.c_str(), CL_DB_APPEND, maxKeySize, maxRecordSize, &dbDataHdl);
    if (CL_OK != rc)
      {
        goto exitOnError2;
      }

    mDbIterHdl = dbDataHdl;

    mInitialized = CL_TRUE;
    return rc;

    exitOnError2: clDbalClose(mDbDataHdl);
    mDbDataHdl = 0;
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

    /* Close the index DB */
    clDbalClose(mDbIterHdl);
    mDbIterHdl = 0;

    /*Finalize dbal */
    clDbalLibFinalize();
    return CL_OK;
  }

  ClRcT MgtDatabase::setRecord(const std::string &key, const std::string &value)
  {
    ClRcT rc = CL_OK;

    if (!mInitialized)
      {
        return CL_ERR_NOT_INITIALIZED;
      }

    ClUint32T hashKey = getHashKeyFn(key.c_str());

    rc = clDbalRecordReplace(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) value.c_str(), value.length());

    return rc;
  }

  ClRcT MgtDatabase::getRecord(const std::string &key, std::string &value)
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

    value.clear();
    value.append(cvalue, dataSize);
    logInfo("MGT", "DBR", "Record [0x%x]: [%s] -> [%s]", hashKey, key.c_str(), value.c_str());
    SAFplusHeapFree(cvalue);
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

    /*
     * Insert into idx table
     */
    rc = clDbalRecordInsert(mDbIterHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) key.c_str(), key.length());
    if (rc != CL_OK)
      {
        return rc;
      }

    /*
     * Insert into data table
     */
    rc = clDbalRecordInsert(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT) value.c_str(), value.length());
    if (rc != CL_OK)
      {
        clDbalRecordDelete(mDbIterHdl, (ClDBKeyT) &hashKey, sizeof(hashKey));
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

    rc = clDbalRecordDelete(mDbIterHdl, (ClDBKeyT) &hashKey, sizeof(hashKey));

    rc = clDbalRecordDelete(mDbDataHdl, (ClDBKeyT) &hashKey, sizeof(hashKey));

    return rc;
  }

  std::vector<std::string> MgtDatabase::iterate(const std::string &xpath, bool keyOnly)
  {
    if (listXpath.size() == 0)
      {
        updateLists();
      }

    std::vector<std::string> iter;

    if (keyOnly)
      {
        for (std::vector<std::string>::iterator it = listKey.begin() ; it != listKey.end(); ++it)
          {
            if (!(*it).compare(0, xpath.length(), xpath))
              iter.push_back((*it));
          }
      }
    else
      {
        for (std::vector<std::string>::iterator it = listXpath.begin() ; it != listXpath.end(); ++it)
          {
            if (!(*it).compare(0, xpath.length(), xpath))
              iter.push_back((*it));
          }
      }

    return iter;
  }

  void MgtDatabase::updateLists()
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
    rc = clDbalFirstRecordGet(mDbIterHdl, (ClDBKeyT*) &recKey, &keySize, (ClDBRecordT*) &recData, &dataSize);
    if (rc != CL_OK)
      {
        return;
      }

    while (1)
      {
        std::string value(recData, dataSize);
        listXpath.push_back(value);
        logInfo("MGT", "LST", "Read [%s]", value.c_str());
        //Free memory
        SAFplusHeapFree(recData);

        std::size_t found = value.find_last_of("@");
        if ((found != std::string::npos) && (found != 0))
          if (value[found - 1] == ']')
            {
              listKey.push_back(value);
            }

        if ((rc = clDbalNextRecordGet(mDbIterHdl, (ClDBKeyT) recKey, keySize, (ClDBKeyT*) &nextKey, &nextKeySize, (ClDBRecordT*) &recData, &dataSize)) != CL_OK)
          {
            rc = CL_OK;
            break;
          }
        recKey = nextKey;
        keySize = nextKeySize;
      }
  }

  ClBoolT MgtDatabase::isInitialized()
  {
    return mInitialized;
  }
};
