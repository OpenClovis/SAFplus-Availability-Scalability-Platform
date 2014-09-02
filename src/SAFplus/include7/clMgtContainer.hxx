/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
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
 *  \brief Header file of the ClMgtContainer class which represents a
 *  YANG container.
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#pragma once
#ifndef CLMGTCONTAINER_H_
#define CLMGTCONTAINER_H_

#include <clMgtObject.hxx>

namespace SAFplus
  {
  typedef std::map<std::string, MgtObject* > MgtObjectMap;

class MgtContainer:public MgtObject
  {
  protected:
  // Store the child nodes
  typedef MgtObjectMap Map;
    MgtObjectMap children;

  class HiddenIterator:public MgtIteratorBase
    {
  public:
    //MgtContainer* tainer;
    MgtObjectMap::iterator it;
    MgtObjectMap::iterator end;

    virtual bool next();
    virtual void del();

    };

  public:

  class Iterator:public MgtObjectMap::iterator
    {
    public:
    };

    MgtContainer():MgtObject("") {}
    MgtContainer(const char* name):MgtObject(name) {}
    virtual ~MgtContainer();


    virtual MgtObject::Iterator begin(void);
    // Override not needed, end is the same: virtual MgtObject::Iterator end(void);

    virtual ClRcT addChildObject(MgtObject *mgtObject, std::string const& objectName=*((std::string*)nullptr));
    virtual ClRcT addChildObject(MgtObject *mgtObject, const char* objectName);
    virtual ClRcT removeChildObject(const std::string& objectName);

    virtual MgtObject* find(const std::string &name);
    virtual MgtObject* deepFind(const std::string &name);
    virtual MgtObject* deepMatch(const std::string &nameSpec);

    virtual MgtObject::Iterator multiFind(const std::string &nameSpec);
    virtual MgtObject::Iterator multiMatch(const std::string &nameSpec);

    virtual void toString(std::stringstream& xmlString);
    virtual std::string strValue() {return "";}

    // Settings objects
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    //virtual void get(void **ppBuffer, ClUint64T *pBuffLen);
    virtual ClRcT write(ClMgtDatabase *db=NULL);
    virtual ClRcT write(std::string parentXPath,ClMgtDatabase *db=NULL);
    virtual ClRcT read(ClMgtDatabase *db=NULL);
    virtual ClRcT read(std::string parentXPath,ClMgtDatabase *db=NULL);
  
  };

}

#endif
