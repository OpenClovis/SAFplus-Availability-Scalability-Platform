/*
 * Copyright (C) 2002-2013 OpenClovis Solutions Inc.  All Rights Reserved.
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
 *  \brief Header file of the MgtProvList class which provides APIs to manage "provisioned" objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTPROVLIST_HXX_
#define CLMGTPROVLIST_HXX_

#include <typeinfo>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <clMgtBaseCommon.hxx>
#include <clMgtObject.hxx>
#include <clMgtRoot.hxx>
using namespace std;
namespace SAFplus
{
  /*
   * Represents YANG leaf-list primitive
   */
  enum SORT
  {
    system = 0, user
  };

  template<class T>
  class MgtProvList: public MgtObject
  {
  public:
    /**
     *  value of the "ClMgtProvList" object
     */
    typedef std::vector<T> ContainerType;
    ContainerType value;
    SORT sortby;
    std::function<bool(const T&, const T&)> funcSortByUser;

  private:
    template<class U>
    ClRcT doRead(MgtDatabase *db, std::string parentXPath, const U & type) {
      MgtObject::read(db, parentXPath);
    }

    ClRcT doRead(MgtDatabase *db, std::string parentXPath, std::string & type) {
      std::string key;
      if(parentXPath.size() > 0)
      {
        key.assign(parentXPath);
        key.append(getFullXpath(false));
      }
      else
        key.assign(getFullXpath(true));
      if(db == nullptr) db = getDb();
      if(!db->isInitialized())
      {
        return CL_ERR_NOT_INITIALIZED;
      }

      std::vector<std::string> iter;
      db->iterate(key, iter);

      value.clear();
      for (std::vector<std::string>::iterator it=iter.begin(); it!=iter.end(); it++)
      {
        std::string val;
        if (db->getRecord(*it, val) == CL_OK)
        {
          if (val.length()>0)
          {
            value.push_back(val);
          }
        }
        else {
        }
      }
    }

    template<class U>
    ClRcT doSetObject(const std::string &val, const U & type) {
      MgtObject::setObj(val);
    }

    ClRcT doSetObject(const std::string &val, std::string & type) {
      std::vector<std::string> refs;
      boost::split(refs, val, boost::is_any_of(", "));
      value.clear();
      for(auto i = refs.begin(); i != refs.end(); i++)
      {
        if ((*i).length()>0)
        {
          value.push_back(*i);
        }
      }

      MgtRoot::getInstance()->addReference(this);
      updateReference();
      lastChange = beat++;
      return CL_OK;
    }

    template<class U>
    ClRcT doSetDb(std::string pxp, MgtDatabase *db, const U & type) {
    }

    ClRcT doSetDb(std::string pxp, MgtDatabase *db, std::string type)
    {
      if (!loadDb)  // Not a configuration item
        return CL_OK;

      if(db == nullptr)
        {
          db = getDb();
        }

      std::string basekey;
      if (pxp.size() > 0)
        {
        basekey = pxp;
        basekey.append("/");
        basekey.append(tag);
        }
      else if (dataXPath.size() > 0)
        basekey = dataXPath;
      else basekey = getFullXpath(true);

      // First, delete all existing records
      int idx;
      db->deleteRecord(basekey);
      int rc = CL_OK;
      for (idx=1; rc==CL_OK; idx++) // Keep deleting as long as there are records.  This assumes that the list records are not sparse!
        {
          std::string key = basekey;
          key.append("[");
          key.append(boost::lexical_cast<std::string>(idx));
          key.append("]");
          rc = db->deleteRecord(key);
        }

      // Now add the new records
      
      std::vector<std::string> children;
      if (value.size()==1)
      {       
        rc = db->setRecord(basekey, value[0], nullptr);
        return rc;
      }
      rc = CL_OK;
      /* TODO: Should be base on refs or value??? refs is configuration but 'value' is real assignment */
      idx = 1;
      for (typename ContainerType::iterator i = value.begin(); i != value.end(); ++i,++idx)
        {
          //typedef boost::array<char, 64> buf_t;
          std::string key = basekey;
          std::string childidx = "[";
          childidx.append(boost::lexical_cast<std::string>(idx));
          childidx.append("]");
          key.append(childidx);
          db->setRecord(key, *i, nullptr);
          children.push_back(childidx);
        } 
      rc = db->setRecord(basekey,"",&children);
      return rc;    
    }

  public:
    MgtProvList(const char* name);

    virtual ~MgtProvList();
    virtual ClRcT createObj(const std::string &val);
    virtual ClRcT deleteObj(const std::string &val);
    /**
     * API to value back to the leaf list
     */
    void pushBackValue(const std::string& strVal);
    /**
     * API to add child to the leaf list
     */
    virtual ClRcT addChildObject(const T&val);
    /**
     * API to remove child from the leaf list
     */
    virtual ClRcT removeChildObject(const T& val);
    /**
     * API to remove all childs from the leaf list
     */
    virtual void removeAllChildren();
    virtual void toString(std::stringstream& xmlString, int depth = SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts = SerializeNoOptions);

    std::string toStringItemAt(T &x);

    // Overload PointList stream insertion operator
    inline friend std::ostream & operator<<(std::ostream &os, const MgtProvList &b)
    {
      copy(b.value.begin(), b.value.end(), std::ostream_iterator < T > (b));
      return os;
    }

    // Overload PointList stream extraction operator
    inline friend std::istream & operator>>(std::istream &is, const MgtProvList &b)
    {
      copy(std::istream_iterator < T > (is), std::istream_iterator<T>(), std::back_inserter(b));
      return is;
    }

    virtual ClRcT read(MgtDatabase *db=nullptr, std::string parentXPath = "") {
      T type;
      return doRead(db, parentXPath, type);
    }

    virtual ClRcT setObj(const std::string &val) {
      T type;
      return doSetObject(val, type);
    }

        //? Function to write data to the database.
    virtual ClRcT write(MgtDatabase* db, std::string xpath = "")
    {
      return setDb(xpath, db);
    }

    //? Function to write changed data to the database.
    virtual ClRcT writeChanged(uint64_t firstBeat, uint64_t beat, MgtDatabase *db = nullptr, std::string xpath = "")
    {
      if ((lastChange > firstBeat)&&(lastChange <= beat))
          { 
          logDebug("MGT", "OBJ", "write [%s/%s] dataXpath [%s]", xpath.c_str(), tag.c_str(), dataXPath.c_str());
          return setDb(xpath, db);
          }
        return CL_OK;
    }

    ClRcT setDb(std::string pxp = "", MgtDatabase *db = nullptr) {
      T type;
      return doSetDb(pxp, db, type);
    }
  };

  template<class T>
  MgtProvList<T>::MgtProvList(const char* name) :
      MgtObject(name)
  {
    sortby = SORT::system;
  }

  template<class T>
  MgtProvList<T>::~MgtProvList()
  {
  }
  /**
   * API to create(add) child to the leaf list
   */
  template<class T>
  ClRcT MgtProvList<T>::createObj(const std::string &val)
  {
    ClRcT ret = CL_OK;
    logDebug("MGT", "PROVLIST", "clMgtProvList::createObj:begin");
    T tvalue;
    stringstream streamconvert(val);
    streamconvert >> tvalue;
    ret = addChildObject(tvalue);
    if (CL_OK != ret) return ret;
    std::string xpath(getFullXpath(true));
    MgtDatabase* db = getDb();
    if (db)
    {
      std::string pval;
      std::vector < std::string > childs;
      db->getRecord(xpath, pval, &childs);
      childs.push_back(val);
      db->setRecord(xpath, pval, &childs);
      logDebug("MGT", "PROVLIST", "Append child node [%s] into [%s]", val.c_str(), tag.c_str());
    }
    return ret;
  }
  /**
   * API to delete(remove) child from the leaf list
   */
  template<class T>
  ClRcT MgtProvList<T>::deleteObj(const std::string &val)
  {
    ClRcT ret = CL_OK;
    logDebug("MGT", "PROVLIST", "clMgtProvList::deleteObj");
    T tvalue;
    stringstream streamconvert(val);
    streamconvert >> tvalue;
    ret = removeChildObject(tvalue);
    if (ret != CL_OK) return ret;
    std::string xpath(getFullXpath(true));
    MgtDatabase* db = getDb();
    if (db)
    {
      std::string pval;
      std::vector < std::string > childs;
      ret = db->getRecord(xpath, pval, &childs);
      if (ret == CL_OK)
      {
        std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), val);
        if (delIter != childs.end())
        {
          childs.erase(delIter);
          ret = db->setRecord(xpath, pval, &childs);
          logDebug("MGT", "PROVLIST", "Remove child node [%s] out of [%s]", val.c_str(), tag.c_str());
        }
      }
    }
    return ret;
  }
  /**
   * API to value back to the leaf list
   */
  template<class T>
  void MgtProvList<T>::pushBackValue(const std::string& strVal)
  {
    addChildObject(strVal);
  }
  /**
   * API to add child to the leaf list
   */
  template<class T>
  ClRcT MgtProvList<T>::addChildObject(const T&val)
  {
    typename std::vector<T>::const_iterator retpos = std::find(value.begin(), value.end(), val);
    if (retpos == value.end())
    {
      value.push_back(val);
    } else
    {
      return CL_ERR_DUPLICATE;
    }
    return CL_OK;
  }
  /**
   * API to remove child from the leaf list
   */
  template<class T>
  ClRcT MgtProvList<T>::removeChildObject(const T& val)
  {
    typename std::vector<T>::iterator retpos = std::find(value.begin(), value.end(), val);
    if (retpos != value.end())
    {
      value.erase(retpos);
      return CL_OK;
    } else return CL_ERR_NOT_EXIST;

  }
  /**
   * API to remove all childs from the leaf list
   */
  template<class T>
  void MgtProvList<T>::removeAllChildren()
  {
    value.clear();
  }
  /**
   * API convert to String of the leaf list
   */
  template<class T>
  void MgtProvList<T>::toString(std::stringstream& xmlString, int depth, SerializationOptions opts)
  {

    if (SORT::system == sortby)
    {
      if (!std::is_sorted(value.begin(), value.end()))
      {
        std::sort(value.begin(), value.end());
      }
    } else
    {
      try
      {
        if (funcSortByUser)
        {
          if (!std::is_sorted(value.begin(), value.end(), funcSortByUser))
          {
            std::sort(value.begin(), value.end(), funcSortByUser);
          }
        } else logDebug("MGT", "PROVLIST", "function compare is not input!");
      }
      catch (std::bad_function_call &ex)
      {
        logDebug("MGT", "PROVLIST", "Exception:%s", ex.what());
      }
    }

    for (unsigned int i = 0; i < value.size(); i++)
    {
      xmlString << "<" << tag << ">" << toStringItemAt(value.at(i)) << "</" << tag << ">";
    }
  }
  template<class T>
  std::string MgtProvList<T>::toStringItemAt(T &x)
  {
    std::stringstream ss;
    ss << x;
    return ss.str();
  }
};
#endif /* CLMGTPROVLIST_HXX_ */
