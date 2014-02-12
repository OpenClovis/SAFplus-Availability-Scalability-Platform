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

#include "clMgtDatabase.hxx"

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clCksmApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

#define DBAL_DB_KEY_BITS (0x16)
#define DBAL_DB_KEY_SIZE (1 << DBAL_DB_KEY_BITS)
#define DBAL_DB_KEY_MASK (DBAL_DB_KEY_SIZE - 1)

static __inline__ ClUint32T getHashKeyFn(const ClCharT *keyStr)
{
    ClUint32T cksum = 0;
    clCksm32bitCompute((ClUint8T*)keyStr, (ClUint32T)strlen(keyStr), &cksum);
    return cksum & DBAL_DB_KEY_MASK;
}

ClMgtDatabase *ClMgtDatabase::singletonInstance = 0;

ClMgtDatabase *ClMgtDatabase::getInstance()
{
    return (singletonInstance ? singletonInstance : (singletonInstance =
                                        new ClMgtDatabase()));
}

ClMgtDatabase::~ClMgtDatabase()
{

}

ClMgtDatabase::ClMgtDatabase()
{
    mInitialized = CL_FALSE;
}

ClRcT ClMgtDatabase::initializeDB(const std::string &dbName, ClUint32T maxKeySize, ClUint32T maxRecordSize)
{
    ClRcT rc = CL_OK;
    ClDBHandleT dbDataHdl = 0; /* Database handle*/

    std::string dbNameData = "";

    /*Initialize dbal if not initialized*/
    rc = clDbalLibInitialize();
    if (CL_OK != rc)
    {
        return rc;
    }

    /* Open the data DB */
    dbNameData.append(dbName).append(".db");
    rc = clDbalOpen(dbNameData.c_str(), dbNameData.c_str(), CL_DB_APPEND, maxKeySize, maxRecordSize, &dbDataHdl);
    if (CL_OK != rc)
    {
        goto exitOnError;
    }

    mDbDataHdl = dbDataHdl;
    mInitialized = CL_TRUE;
    return rc;

exitOnError:
    clDbalLibFinalize();
    return rc;
}

ClRcT ClMgtDatabase::finalizeDB()
{
    if(!mInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }

    /* Close the data DB */
    clDbalClose (mDbDataHdl);
    mDbDataHdl = 0;

    /*Finalize dbal */
    clDbalLibFinalize();
    return CL_OK;
}

ClRcT ClMgtDatabase::setRecord(const std::string &key, const std::string &value)
{
    ClRcT rc = CL_OK;
    ClUint32T dataSize = 0;

    if(!mInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }

    ClUint32T hashKey = getHashKeyFn(key.c_str());

    rc = clDbalRecordReplace(mDbDataHdl, (ClDBKeyT)&hashKey, sizeof(hashKey), (ClDBRecordT) value.c_str(), value.length());

    return rc;
}

ClRcT ClMgtDatabase::getRecord(const std::string &key, std::string &value)
{
    ClRcT rc = CL_OK;
    ClCharT *cvalue;
    ClUint32T dataSize = 0;

    if(!mInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }

    ClUint32T hashKey = getHashKeyFn(key.c_str());

    rc = clDbalRecordGet(mDbDataHdl, (ClDBKeyT)&hashKey, sizeof(hashKey), (ClDBRecordT*)&cvalue, &dataSize);
    if (CL_OK != rc)
    {
        return rc;
    }

    value.clear();
    value.append(cvalue, dataSize);

    return rc;
}

