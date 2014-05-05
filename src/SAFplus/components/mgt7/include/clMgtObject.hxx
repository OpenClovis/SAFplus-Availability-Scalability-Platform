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
 *  \brief Header file of the ClMgtObject class which provides APIs to manage MGT objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTOBJECT_H_
#define CLMGTOBJECT_H_

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "clTransaction.hxx"
#include "clMgtMsg.hxx"

extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */

#include <clCommon.hxx>

namespace SAFplus
  {
  class MgtError:public Error
    {
    public:
      MgtError(const char* error): Error(error)
      {
      }
    
    };

  //  Can be thrown on marshal, demarshal, XMLize or deXMLize
  class SerializationError:public MgtError
    {
    public:
      SerializationError(const char* error):MgtError(error) 
      {
      }
    
    };
  

  class ClMgtObject;

  typedef std::map<std::string, std::vector<ClMgtObject*>* > ClMgtObjectMap;

/**
 * ClMgtObject class which provides APIs to manage a MGT object
 */
  class ClMgtObject
    {
  protected:
    /*
     * Store the child node
     */
    ClMgtObjectMap mChildren;

  public:
    std::string Name;
    std::vector<std::string> Keys;
    ClMgtObject *Parent;

  public:
    ClMgtObject(const char* name);
    virtual ~ClMgtObject();

    /**
     * \brief	Find the root of this management tree
     */
    ClMgtObject* root(void);

    /**
     * \brief	Find the child or grandchild recursively with this name
     */
    ClMgtObject* deepFind(const std::string &s);

    /**
     * \brief	Find the child or grandchild recursively with this name
     */
    ClMgtObject* find(const std::string &s);
 
    /**
     * \brief	Get child iterator beginning
     */
    ClMgtObjectMap::iterator begin(void) { return mChildren.begin(); }

    /**
     * \brief	Get child iterator end
     */
    ClMgtObjectMap::iterator end(void) { return mChildren.end(); }

    /**
     * \brief	Function to add a key
     * \param	key							Key of the list
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_ALREADY_EXIST		Key already exists
     */
    ClRcT addKey(std::string key);

    /**
     * \brief	Function to remove a key
     * \param	key							Key of the list
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_NOT_EXIST			List entry does not exist
     */
    ClRcT removeKey(std::string key);

    /**
     * \brief	Function to add a child name
     * \param	name						Name of the child
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_ALREADY_EXIST		Key already exists
     */
    ClRcT addChildName(std::string name);

    /**
     * \brief	Function to remove a child name
     * \param	name						Name of the child
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_NOT_EXIST			List entry does not exist
     */
    ClRcT removeChildName(std::string name);

    /**
     * \brief	Function to add a child object
     * \param	mgtObject				MGT object to be added
     * \param	objectName				MGT object name.  If not supplied, the mgtObject's name will be used
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	Module already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    ClRcT addChildObject(ClMgtObject *mgtObject, std::string const& objectName=*((std::string*)nullptr));
    ClRcT addChildObject(ClMgtObject *mgtObject, const char* objectName);

    /**
     * \brief	Function to remove a child object
     * \param	objectName				MGT object name
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT object does not exist
     */
    ClRcT removeChildObject(const std::string objectName, ClUint32T index = 0);

    /**
     * \brief	Function to get a child object
     * \param	objectName				MGT object name
     * \return	If the function succeeds, the return value is a MGT object
     * \return	If the function fails, the return value is NULL
     */
    ClMgtObject *getChildObject(const std::string objectName, ClUint32T index = 0);

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    /**
     * \brief	Virtual function called from netconf server to get object data
     */
    virtual void get(void **ppBuffer, ClUint64T *pBuffLen);
    virtual void toString(std::stringstream& xmlString);
    virtual std::string strValue() {return "";}

    /**
     * \brief	Virtual function called from netconf server to set object data
     */
    void set(void *pBuffer, ClUint64T buffLen);

    /**
     * \brief	Function to bind this MGT object to a specific manageability subtree within a particular module
     * \param	module					MGT module name
     * \param	route					XPath of the MGT object
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT module does not exist
     * \return	CL_ERR_ALREADY_EXIST	MGT object already exists
     */
    ClRcT bindNetconf(const std::string module, const std::string route);
    ClRcT bindSnmp(const std::string module, const std::string route);

    ClBoolT isKeysMatch(std::map<std::string, std::string> *keys);

    virtual std::vector<std::string> *getChildNames();

    /* persistent db to database */
    virtual ClRcT write();

    /* unmashall db to object */
    virtual ClRcT read();

    /* iterator db key and bind to object */
    virtual ClRcT iterator();

    void dumpXpath();

    std::string getFullXpath();

    virtual void load();

    // Debugging API only:
    void dbgDumpChildren();
    };


  inline void deXMLize(const std::string& obj,ClMgtObject* context, std::string& result) { result=obj; }
// True is 1, anything that begins with t,T,y,Y (for yes).  False is 0, anything that begins with f,F,n or N (for no)
  void deXMLize(const std::string& obj,ClMgtObject* context, bool& result); // throw(SerializationError);
  void deXMLize(const std::string& obj,ClMgtObject* context, ClBoolT& result); // throw(SerializationError);

  inline void deXMLize(const char* obj,ClMgtObject* context, std::string& result) { result=obj; }

  template<typename T> inline void deXMLize(const std::string& strVal,ClMgtObject* context, T& result) // throw(SerializationError)
    {
    std::stringstream ss;
    ss << strVal;
    // If you need to write a deXMLize for your custom type, you will
    // get  this inscrutiable error: cannot bind
    // std::basic_istream<char> lvalue to std::basic_istream<char>&& 
    ss >> result;
    }
  template<typename T> inline void deXMLize(const char* strVal,ClMgtObject* context, T& result) // throw(SerializationError)
    {
    std::stringstream ss;
    ss << strVal;
    // If you need to write a deXMLize for your custom type, you will
    // get  this inscrutiable error: cannot bind std::basic_istream<char> lvalue to std::basic_istream<char>&& 
    ss >> result;  
    }
  };
#endif /* CLMGTOBJECT_H_ */

/** \} */
