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
#include <clLogIpi.hxx>
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
    pluginFlags = 0;
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

    //? <cfg name="SAFPLUS_MGT_DB_PLUGIN">[OPTIONAL] Specifies the management database plugin</cfg>
    if ((&dbPlugin==nullptr)||(dbPlugin == ""))
      {
        char* dbPluginTmp = getenv("SAFPLUS_MGT_DB_PLUGIN");
        mDbDataHdl = clDbalObjCreate(dbPluginTmp);  // If NULL, we'll choose the default db
      }
    else
      {
        mDbDataHdl = clDbalObjCreate(dbPlugin.c_str());
      }
    assert(mDbDataHdl);

    /* Open the data DB */
    // Do not overwrite the flags passed by an user outsite
    //pluginFlags = 0;
    dbNameData.append(dbName).append(".db");
    unsigned int flags = (pluginFlags << 8) | CL_DB_APPEND;
    rc = mDbDataHdl->open(dbNameData.c_str(), dbNameData.c_str(), flags, maxKeySize, maxRecordSize);
    if (CL_OK != rc)
      {
    	logInfo("MGT", "DBR", "Opening database false");
        goto exitOnError1;
      }
    logInfo("MGT", "DBR", "Opening database [%s] Ok", dbNameData.c_str());
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

      logInfo("MGT", "DBR", "Write DB record [0x%" PRIx64 "%" PRIx64 "]: [%s] -> [%s] children [%d]", hashKey.num[0],hashKey.num[1], key.c_str(), value.c_str(),dbValue.child_size());

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
        logError("MGT","DBR","db is not initialized");
        return CL_ERR_NOT_INITIALIZED;
      }

    HashKey hashKey = getHashKeyFn(key);

    rc = mDbDataHdl->getRecord((ClDBKeyT) &hashKey, sizeof(hashKey), (ClDBRecordT*) &cvalue, &dataSize);
    if (CL_OK != rc)
      {
        logError("MGT","DBR","Record [%s] read failed rc [0x%x]", key.c_str(), rc);
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
    logInfo("MGT", "DBR", "Deleted record [0x%" PRIx64 "%" PRIx64 "]: [%s] rc [0x%x]", hashKey.num[0], hashKey.num[1], key.c_str(), rc);

    return rc;
  }

  ClRcT MgtDatabase::deleteAllRecordsContainKey(const std::string &keypart)
  {
     ClRcT rc = CL_ERR_NOT_EXIST;
     std::vector<std::string> childs;
     std::string root("/");
     iterate(root,childs);     
     for(std::vector<std::string>::const_iterator it=childs.cbegin();it!=childs.cend();it++)
     {
        const std::string& key = *it;
        if (key.find(keypart) != std::string::npos)
        {
           if ((rc = deleteRecord(key) != CL_OK))
           {
              logError("MGT","DELL.ALL","delete record with key [%s] failed, rc [0x%x]", key.c_str(), rc);
              break;
           }             
        }
     }      
     return rc;
  }


  //ClRcT loadDBRecords(DbalPlugin* db, DBRecord& dbRec
  // calling deleteAllReferencesToEntity("/safplusAmf/ServiceUnit[@name="su0"]","su0"); 
  ClRcT MgtDatabase::deleteAllReferencesToEntity(const std::string& xpathToDelete, const std::string &entityName)
  {
     ClRcT rc = CL_ERR_NOT_EXIST;
     size_t pos = std::string::npos;
     bool entityFound = false;
     std::vector<std::string> xpaths, childs, attrChilds, vec;
     std::string root("/"), val, attrVal;
     iterate(root,xpaths);
     for(std::vector<std::string>::const_iterator it=xpaths.cbegin();it!=xpaths.cend();it++)
     {
        const std::string& xpath = *it;
        if (((rc = getRecord(xpath, val, &childs)))!= CL_OK)
        {
           logError("MGT","DEL.REFS","get record with xpath [%s] fail rc=0x%x", xpath.c_str(), rc);
           return rc;
        }
        if ((pos=xpath.find(xpathToDelete)) == std::string::npos && val.compare(entityName)==0) // exists /safplusAmf/Node[@name="node0"]/serviceunits --> su0, so no childs. BUT there is a case: /safplusAmf/ServiceUnit[@name="su0"]/name --> su0
        {
           // So overwrite the xpath with empty
           val="";
           if (((rc = setRecord(xpath, val)))!= CL_OK)
           {
              logError("MGT","DEL.REFS","set record with xpath [%s] val [%s] fail rc=0x%x", xpath.c_str(), val.c_str(), rc);
              return rc;
           }
        }
        else if (pos!=std::string::npos && childs.size()>0) // exists safplusAmf/Node[@name="node0"]/serviceunits --> child: [1][2][3]...
        {           
           for(std::vector<std::string>::const_iterator it2=childs.cbegin();it2!=childs.cend();it2++)
           {              
              const std::string& idx = *it2;
              std::string attrXpath = xpath;              
              attrXpath.append(idx); // --> safplusAmf/Node[@name="node0"]/serviceunits[1]...    
              if (((rc = getRecord(attrXpath, attrVal, &vec)))!= CL_OK)
              {
                 logError("MGT","DEL.REFS","get record with xpath [%s] fail rc=0x%x", attrXpath.c_str(), rc);
                 return rc;
              }
              if (attrVal.compare(entityName) == 0)
              {
                 entityFound = true;
#if 0
                 if (child.size()==2) // only 2 attrs [1][2], so delete this record and set safplusAmf/Node[@name="node0"]/serviceunits su0 --> child:
                 {
                    if (((rc = deleteRecord(attrXpath)))!= CL_OK)
                    {
                       logError("MGT","DEL.REF","delete record with xpath [%s] fail rc=0x%x", xpath.c_str(), rc);
                       return rc;
                    }
                 }

                 else
                 {
                    //
                 }
#endif
              }
              else
              {
                 attrChilds.push_back(attrVal);
              }
           }
           if (entityFound)
           {
#if 0
              for (int i=0;i<childs.size();i++)
              {
                 std::stringstream attrXpath;
                 attrXpath << xpath
              }
#endif
              //reindex the list: 1. delete all attrs 2.
              for(std::vector<std::string>::const_iterator it2=childs.cbegin();it2!=childs.cend();it2++)
              {              
                 const std::string& idx = *it2;
                 std::string attrXpath = xpath;              
                 attrXpath.append(idx); // --> safplusAmf/Node[@name="node0"]/serviceunits[1]...  
                 if (((rc = deleteRecord(attrXpath)))!= CL_OK)
                 {
                    logError("MGT","DEL.REFS","delete record with xpath [%s] fail rc=0x%x", attrXpath.c_str(), rc);
                    return rc;
                 }
              }
              //2. reindex
              if (childs.size()==2)
              {
                 std::string& newVal = attrChilds[0];
                 if (((rc = setRecord(xpath, newVal)))!= CL_OK)
                 {
                    logError("MGT","DEL.REFS","set record with xpath [%s] val [%s] fail rc=0x%x", xpath.c_str(), newVal.c_str(), rc);
                    return rc;
                 }
              }
              else
              {
                 std::vector<std::string>vals;
                 for (int i=0;i<attrChilds.size();i++)                 
                 {
                    std::stringstream v;
                    v << "[" << i << "]";
                    vals.push_back(v.str());
                 }
                 if (((rc = setRecord(xpath, std::string(""),&vals)))!= CL_OK)
                 {
                    logError("MGT","DEL.REFS","set record with xpath [%s] val [] childs num [%d] rc=0x%x", xpath.c_str(), (int)vals.size(), rc);
                    return rc;
                 }
                 for (int i=0;i<attrChilds.size();i++)
                 {
                    std::string& newVal = attrChilds[i];
                    std::stringstream attrXpath;
                    attrXpath << xpath << "[" << i << "]";
                    if (((rc = setRecord(attrXpath.str(), newVal)))!= CL_OK)
                    {
                       logError("MGT","DEL.REFS","set record with xpath [%s] val [%s] fail rc=0x%x", xpath.c_str(), newVal.c_str(), rc);
                       return rc;
                    }
                 }
                 vals.clear();
              }
              entityFound = false;
           }
        }
        pos = std::string::npos;
        childs.clear();
        attrChilds.clear();
        vec.clear();
        val.clear();
        attrVal.clear();
     }
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
