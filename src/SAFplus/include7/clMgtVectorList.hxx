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
 *  \brief Header file of MgtVectorList class which provides APIs to manage Yang lists
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTLIST_HXX_
#define CLMGTLIST_HXX_

#include <string>
#include <vector>

#include "clMgtObject.hxx"
#include "clMgtMsg.hxx"


extern "C"
{
#include <clCommon.h>
} /* end extern 'C' */

namespace SAFplus
{

  /**
   *  MgtVectorList class provides APIs to manage Yang lists
   */
  class MgtVectorList : public MgtObject
  {
  protected:
    /*
     * Store the list entries
     */
    std::vector<MgtObject*> mEntries;
    //std::vector<std::string> mKeys;
    ClBoolT isEntryExist(MgtObject* entry);
    //?MgtObject* findEntryByKeys(std::map<std::string, std::string> *keys);

  public:
    MgtVectorList(const char* name);
    virtual ~MgtVectorList();
    //ClBoolT isEntryExist(MgtObject* entry);

#if 0
    /**
     * \brief	Function to add a key
     * \param	key							Key of the list
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_ALREADY_EXIST		Key already exists
     */
    ClRcT addKey(std::string key);

    /**
     * \brief	Function to remove a list entry
     * \param	key							Key of the list
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_NOT_EXIST			List entry does not exist
     */
    ClRcT removeKey(std::string key);
    void removeAllKeys();

    /**
     * \brief	Function to add a list entry to a list
     * \param	entry						Pointer to the list entry
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_NULL_POINTER			Input parameter is a NULL pointer
     * \return	CL_ERR_INVALID_PARAMETER	Input parameter is invalid
     * \return	CL_ERR_ALREADY_EXIST		List entry already exists
     */
    ClRcT addEntry(MgtObject* entry);

#endif
    virtual ClRcT removeChildObject(const std::string& objectName);
    void removeAllChildren();

    /**
     * \brief	Function to get a list entry from the database
     * \param	index						Index of the entry
     * \return	If the function succeeds, the return value is a MGT container
     * \return	If the function fails, the return value is NULL
     */
    MgtObject* getEntry(ClUint32T index);

    /**
     * \brief	Function to remove a list entry
     * \param	index						Index of the entry
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_NOT_EXIST			List entry does not exist
     */
    ClRcT removeEntry(ClUint32T index);

    virtual ClRcT addChildObject(MgtObject *mgtObject, std::string const& objectName=*((std::string*)nullptr));
  

    /**
     * \brief	Function called from netconf server to get data of the list
     */
    virtual void toString(std::stringstream& xmlString);

    ClUint32T getEntrySize();
    ClUint32T getKeySize();
    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

  };

};

#endif /* CLMGTLIST_HXX_ */

/** \} */

