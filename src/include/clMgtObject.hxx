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
 *  \brief Header file of the MgtObject class which provides APIs to manage MGT objects
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
#include "clMgtDatabase.hxx"

#include <clCommon.hxx>

namespace SAFplus
  {
  class MgtDatabase;
  class MgtObject;

  class MgtError:public Error
    {
    public:
      MgtError(const char* error): Error(error)
      {
      }
    
    };

  // this management node is not a leaf but you tried to assign it data
  class NoDataError:public Error
    {
    public:
      NoDataError(const char* error): Error(error)
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

  // This is the hidden virtual iterator underneath MgtObject iterators.
  class MgtIteratorBase
    {
  public:
    std::pair<std::string, MgtObject*> current;
    int refs;
    MgtIteratorBase(): current("",nullptr),refs(1) {};
    bool operator++()
      {
      return next();
      }

    virtual ~MgtIteratorBase() 
      {
      }
    virtual bool next();
    virtual void del();
    };

extern MgtIteratorBase mgtIterEnd;

/**
 * MgtObject class which provides APIs to manage a MGT object
 */
  class MgtObject
    {
  public:
    std::string tag;
    std::string listTag;
    std::string dataXPath;
    bool loadDb;
    bool config;  // True if this object is configuration (available in the database).  False if it is statistics or status
    MgtObject *parent;
    ClUint32T headRev; // Revision to check before sending
  public:
    MgtObject(const char* name);
    virtual ~MgtObject();

    class Iterator
      {
    public:  // Do not use
      Iterator():b(&mgtIterEnd) {}  // b must ALWAYS be valid
      Iterator(const Iterator& i);
      Iterator& operator=(const Iterator& i);
      friend class MgtObject;
      MgtIteratorBase* b;
    public:

      bool operator !=(const Iterator& e) const;

      inline bool operator++(int)
        {
        if (b) return b->next();
        else return false;
        }
      inline bool operator++()
        {
        if (b) return b->next();
        else return false;
        }

      ~Iterator()
        {
        if (b) 
          { 
          b->refs--; 
          if (b->refs==0) b->del();
          }
        b = nullptr;
        }

      const std::pair<std::string, MgtObject* >* operator ->() const
        {
        return &b->current;
        }
      const std::pair<std::string, MgtObject* >& operator *() const
        {
        return b->current;
        }
      };

    /**  TODO: Should be unnecessary but code gen is using it
     * \brief	Function to add a key
     * \param	key							Key of the list
     * \return	CL_OK						Everything is OK
     * \return	CL_ERR_ALREADY_EXIST		Key already exists
     */
    ClRcT addKey(std::string key) { return  CL_OK; }

    /**
     * \brief	Function to add a child object
     * \param	mgtObject				MGT object to be added
     * \param	objectName				MGT object name.  If not supplied, the mgtObject's name will be used
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_ALREADY_EXIST	Module already exists
     * \return	CL_ERR_NULL_POINTER		Input parameter is a NULL pointer
     */
    virtual ClRcT addChildObject(MgtObject *mgtObject, const std::string& objectName=*((std::string*)nullptr));
    virtual ClRcT addChildObject(MgtObject *mgtObject, const char* objectName);

    /**
     * \brief	Function to remove a child object
     * \param	objectName				MGT object name
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT object does not exist
     */
    virtual ClRcT removeChildObject(const std::string& objectName);
    virtual void removeAllChildren();

    /**
     * \brief	Find the root of this management tree
     */
    MgtObject* root(void);

    /**
     * \brief	Find the child or grandchild recursively with this name
     */
    virtual MgtObject* deepFind(const std::string &name);
    virtual MgtObject* deepMatch(const std::string &nameSpec);

    /**
     * \brief	Find the first child with this name
     * \param	name				MGT object name
     * \return	If the function succeeds, the return value is a MGT object
     * \return	If the function fails, the return value is NULL
     */
    virtual MgtObject* find(const std::string &name);

    /**
     * \brief	Function to get a child object
     * \param	objectName				MGT object name
     * \return	If the function succeeds, the return value is a MGT object
     * \return	If the function fails, the return value is NULL
     */
      MgtObject *getChildObject(const std::string& objectName) 
      {
      return find(objectName);
      }

    
      //? fills result with all management objects that match the provided path.
      virtual void resolvePath(const char* path, std::vector<MgtObject*>* result);
      //? returns true if this object matches the passed specification
      virtual bool match(const std::string& nameSpec);

    /**
     * \brief	Find children whos name fits the name specification
     * \param	nameSpec The name specification: use directory-style wildcards. TODO: should it be XPATH style?
     * \return	If the function succeeds, the return value is an iterator of all compliant mgt objects.
     */

    virtual MgtObject::Iterator multiFind(const std::string &nameSpec);
    virtual MgtObject::Iterator multiMatch(const std::string &nameSpec);
 
    /**
     * \brief	Get child iterator beginning
     */
    virtual MgtObject::Iterator begin(void);

    /**
     * \brief	Get child iterator end
     */
    virtual MgtObject::Iterator end(void);

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
      return CL_TRUE;
    }

    enum SerializationOptions
      {
        SerializeNoOptions=0,
        SerializeNameAttribute=1, // Add name="foo" to tag, for example <tag> becomes <tag name="foo">
        SerializePathAttribute=2, // Add path="/full/route/to/object/foo" to tag, for example <tag> becomes <tag name="/SAFplusAmf/Component/c0">
        SerializeListKeyAttribute=4, // Add path="/full/route/to/object/foo" to tag, for example <tag> becomes <tag name="/SAFplusAmf/Component/c0">
        SerializeFormatted=8,     // Pretty print 
        SerializeOnePath=16
      };

    /**
     * \brief	Virtual function called from netconf server to get object data
     */
    virtual void get(std::string *data);
    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth,SerializationOptions opts=SerializeNoOptions)=0;
    virtual std::string strValue() {return "";}


        /** \brief Returns true if the name matches the namespec.. */
    virtual bool match( const std::string &name, const std::string &spec);

    /**
     * \brief	Virtual function called from netconf server to set object data
     */
    void set(void *pBuffer, ClUint64T buffLen);

    /**
     * \brief	Function to bind this MGT object to a specific managability subtree within a particular module
     * \param	module					MGT module name
     * \param	route					XPath of the MGT object
     * \return	CL_OK					Everything is OK
     * \return	CL_ERR_NOT_EXIST		MGT module does not exist
     * \return	CL_ERR_ALREADY_EXIST	MGT object already exists
     */
    ClRcT bind(Handle handle, const std::string& module, const std::string& route);

    /** \brief persist to database. 
     *  \param db The database to access. by default it uses the
     *  globally defined database. 
     */
    virtual ClRcT write(MgtDatabase *db=nullptr, std::string parentXPath = "");
    /** \brief Load object from database. 
     *  \param db The database to access. by default it uses the
     *  globally defined database. 
     */
    virtual ClRcT read(MgtDatabase *db=nullptr, std::string parentXPath = "");

    /* iterator db key and bind to object */
    // not implemented virtual ClRcT iterator();

    void dumpXpath(unsigned int depth = 0);

    std::string getFullXpath(bool includeParent = true);

    virtual MgtObject *findMgtObject(const std::string &xpath, std::size_t idx);
    virtual ClRcT setObj(const std::string &value);
    virtual ClRcT createObj(const std::string &value);
    virtual ClRcT deleteObj(const std::string &value);
    virtual ClRcT setChildObj(const std::string &childName, const std::string &value);
    virtual ClRcT setChildObj(const std::map<std::string,std::string> &keyList);

    // Debugging API only:
    void dbgDumpChildren();
    void dbgDump();

    virtual MgtObject* lookUpMgtObject(const std::string & classType, const std::string &ref);
    virtual void updateReference(void);
    };


  inline void deXMLize(const std::string& obj,MgtObject* context, std::string& result) { result=obj; }
// True is 1, anything that begins with t,T,y,Y (for yes).  False is 0, anything that begins with f,F,n or N (for no)
  void deXMLize(const std::string& obj,MgtObject* context, bool& result); // throw(SerializationError);
    // ClBoolT is a short so this breaks normal numbers: void deXMLize(const std::string& obj,MgtObject* context, ClBoolT& result); // throw(SerializationError);

  inline void deXMLize(const char* obj,MgtObject* context, std::string& result) { result=obj; }
  inline void deXMLize(const char* obj,MgtObject* context, int& result) { result=atoi(obj); }
  template<typename T> inline void deXMLize(const std::string& strVal,MgtObject* context, T& result) // throw(SerializationError)
    {
    std::stringstream ss;
    ss << strVal;
    // If you need to write a deXMLize for your custom type, you will
    // get  this inscrutiable error: cannot bind
    // std::basic_istream<char> lvalue to std::basic_istream<char>&& 
    ss >> result;
    }
  template<typename T> inline void deXMLize(const char* strVal,MgtObject* context, T& result) // throw(SerializationError)
    {
    std::stringstream ss;
    ss << strVal;
    // If you need to write a deXMLize for your custom type, you will
    // get  this inscrutiable error: cannot bind std::basic_istream<char> lvalue to std::basic_istream<char>&& 
    ss >> result;  
    }
  template<typename T> inline void deXMLize(const char* strVal,MgtObject* context, bool& result) // throw(SerializationError)
  {
    std::stringstream ss;
    ss << strVal;
    deXMLize(ss.str(),context,result);

  }
  };

#endif /* CLMGTOBJECT_H_ */

/** \} */
