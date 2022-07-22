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
 *  \brief Header file of MgtDatabase class which provides APIs to access mgt database
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTDATABASE_HXX_
#define CLMGTDATABASE_HXX_

#include <string>
#include <vector>
#include <clDbalBase.hxx>
extern "C"
{
//#include <clCommon.h>
//#include <clDbalBase.hxx>
} /* end extern 'C' */

//#define DBAL_DB_KEY_BITS (32ULL)
//#define DBAL_DB_KEY_SIZE (1ULL << DBAL_DB_KEY_BITS)
//#define DBAL_DB_KEY_MASK (DBAL_DB_KEY_SIZE - 1ULL)

#define MGT_DB_MAX_NUMBER_RECORD 1024
#define MGT_DB_MAX_SIZE_RECORD 1024

namespace SAFplus
{
  // Additional flags to be sent when opening the underlying database
  // extern unsigned int dbalPluginFlags;

  class MgtDatabase
  {
  protected:

    //static MgtDatabase *singletonInstance;

    ClBoolT mInitialized;
    DbalPlugin* mDbDataHdl;

  private:
    void loadDb(std::vector<std::string> &result, bool includeMatadata = false);
    ClRcT write2DB(const std::string &key, const std::string &value, std::vector<std::string> *child = nullptr, bool overwrite = false);

  public:
    unsigned int pluginFlags;
    MgtDatabase();
    virtual ~MgtDatabase();

    DbalPlugin* getPlugin() { return mDbDataHdl; }
    /**
     * \brief	Function to create/get the singleton object of the ClMgtDatabase class
     */
    //static MgtDatabase *getInstance();

    //static void DestroyInstance(); // Constructor for singleton

    /**
     * \brief	Function to initialize Database Abstraction Layer and open a mgt database
     */
    ClRcT initialize(const std::string &dbName, const std::string &dbPlugin=*((std::string*) nullptr), ClUint32T maxKeySize = MGT_DB_MAX_NUMBER_RECORD, ClUint32T maxRecordSize = MGT_DB_MAX_SIZE_RECORD);

    /**
     * \brief	Function to finalize DBAL and close mgt database
     */
    ClRcT finalize();

    /**
     * \brief	Function to check if the DB is initiated or not
     */
    ClBoolT isInitialized();

    /**
     * \brief	Function to set record to Db
     */
    ClRcT setRecord(const std::string &key, const std::string &value, std::vector<std::string> *child = nullptr);

    /**
     * \brief Function to insert record to Db
     */
    ClRcT insertRecord(const std::string &key, const std::string &value, std::vector<std::string> *child = nullptr);

    /**
     * \brief	Function to get record from Db
     */
    ClRcT getRecord(const std::string &key, std::string &value, std::vector<std::string> *child = nullptr);

    /**
     * \brief	Function to delete record out of Db
     */
    ClRcT deleteRecord(const std::string &key);
    ClRcT deleteAllRecordsContainKey(const std::string &keypart);
    ClRcT deleteAllReferencesToEntity(const std::string& xpathToDelete, const std::string &entityName, const char* entityListName);

    /**
     * \brief   Function to return iterators match with xpath
     */
    void iterate(const std::string &xpath, std::vector<std::string> &result, bool includeMatadata = false);

  };
}
;

#endif /* CLMGTDATABASE_HXX_ */

/** \} */

