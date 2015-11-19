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

#define DBAL_DB_KEY_BITS (32ULL)
#define DBAL_DB_KEY_SIZE (1ULL << DBAL_DB_KEY_BITS)
#define DBAL_DB_KEY_MASK (DBAL_DB_KEY_SIZE - 1ULL)

#define MGT_DB_MAX_NUMBER_RECORD 1024
#define MGT_DB_MAX_SIZE_RECORD 1024

namespace SAFplus
{

  class MgtDatabase
  {
  protected:
    MgtDatabase();

    static MgtDatabase *singletonInstance;

    ClBoolT mInitialized;
    DbalPlugin* mDbDataHdl;

  private:
    void loadDb(std::vector<std::string> &result);
    ClRcT write2DB(const std::string &key, const std::string &value, std::vector<std::string> *child = nullptr, bool overwrite = false);

  public:
    virtual ~MgtDatabase();

    /**
     * \brief	Function to create/get the singleton object of the ClMgtDatabase class
     */
    static MgtDatabase *getInstance();

    static void DestroyInstance(); // Constructor for singleton

    /**
     * \brief	Function to initialize Database Abstraction Layer and open a mgt database
     */
    ClRcT initializeDB(const std::string &dbName, ClUint32T maxKeySize = MGT_DB_MAX_NUMBER_RECORD, ClUint32T maxRecordSize = MGT_DB_MAX_SIZE_RECORD);

    /**
     * \brief	Function to finalize DBAL and close mgt database
     */
    ClRcT finalizeDB();

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

    /**
     * \brief   Function to return iterators match with xpath
     */
    void iterate(const std::string &xpath, std::vector<std::string> &result);

  };
}
;

#endif /* CLMGTDATABASE_HXX_ */

/** \} */

