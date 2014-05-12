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
 *  \brief Header file of ClMgtList class which provides APIs to manage Yang lists
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

#include <boost/container/map.hpp>

extern "C"
  {
#include <clCommon.h>
  } /* end extern 'C' */

namespace SAFplus
  {

  /**
   *  MgtList class provides APIs to manage Yang lists
   */
  class MgtList : public MgtObject
    {
  protected:
    typedef boost::container::map<std::string, MgtObject* > Map;
    /*
     * Store the list entries
     */
    Map children;
    //std::vector<std::string> mKeys;
    ClBoolT isEntryExist(MgtObject* entry);
    //?MgtObject* findEntryByKeys(std::map<std::string, std::string> *keys);
 
    class HiddenIterator:public MgtIteratorBase
      {
    public:
      //MgtContainer* tainer;
      Map::iterator it;
      Map::iterator end;

      virtual bool next();
      virtual void del();
      };

  public:
    MgtList(const char* name);
    virtual ~MgtList();

    virtual ClRcT addChildObject(MgtObject *mgtObject, std::string const& objectName=*((std::string*)nullptr));
    virtual ClRcT removeChildObject(const std::string& objectName);
    void removeAllChildren();
 
    MgtObject::Iterator begin(void);


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

