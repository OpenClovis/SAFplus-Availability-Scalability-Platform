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
 *  \brief Header file of the MgtIdentifierList class which provides APIs to manage "instance identifier" objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTIDENTIFIERLIST_HXX_
#define CLMGTIDENTIFIERLIST_HXX_

#include "clMgtObject.hxx"
#include "clMgtRoot.hxx"

#include <typeinfo>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <vector>

namespace SAFplus
{
/*
 * Represents YANG leaf-list primitive
 */

template<class T>
class MgtIdentifierList: public MgtObject
{
public:
    class Elem
    {
    public:
      std::string ref;
      bool        validValue;
      T           value;
      Elem():ref(),validValue(false),value() {}
      Elem(const std::string& r):ref(r),validValue(false),value() {}
      Elem(const T& v):ref(),validValue(true),value(v) {}
    };
    typedef std::vector<Elem> Container;
    Container value;

    class iterator:public SAFplus::MgtIteratorBase
    {
      typename Container::iterator i;
      typename Container::iterator end;
      virtual bool next()
      {
        do { 
           ++i; 
        } while ((i!=end)&&(i->validValue==false));
        if (i != end)
          {
          current.first = i->ref;
          current.second = i->value;
          }
        else
          {
            current.first = "";
            current.second = nullptr;
          }
        return i != end;
      }
    public:
      iterator(const typename Container::iterator& ini,const typename Container::iterator& e):i(ini),end(e) 
        { 
          if ((i != end)&&(!i->validValue)) 
            next(); 
          if (i != end)
            {
            current.first = i->ref;
            current.second = i->value;
            }
          else
            {
            current.first = "";
            current.second = nullptr;
            }
        }
      iterator():i() {}
    public:
      bool operator!= (const iterator& other) const { return i != other.i; } 
      bool operator++() { return next(); }
      bool operator++(int) { return next(); }
      T operator*() { return i->value; }

    virtual void del()
    {
      delete this;
    }
  };


      //? Get child iterator beginning
  virtual MgtObject::Iterator begin(void)
  {
    MgtObject::Iterator it;
    it.b = new iterator(value.begin(), value.end());
    return it;
  }


  iterator listBegin() { return iterator(value.begin(),value.end()); }
  iterator listEnd() { return iterator(value.end(),value.end()); }

public:
    MgtIdentifierList(const char* name);

    virtual ~MgtIdentifierList();

    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts=SerializeNoOptions);

  //? Virtual function to assign object data
    virtual bool set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    virtual ClRcT setObj(const std::string &value);

      // Private function to write data to database, use write
  ClRcT setDb(std::string pxp = "", MgtDatabase *db = nullptr)
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
       rc = db->setRecord(basekey, value[0].ref, nullptr);
       return rc;
    }
    rc = CL_OK;
    /* TODO: Should be base on refs or value??? refs is configuration but 'value' is real assignment */
    idx = 1;
    for (typename Container::iterator i = value.begin(); i != value.end(); ++i,++idx)
      {
        //typedef boost::array<char, 64> buf_t;
        std::string key = basekey;
        std::string childidx = "[";
        childidx.append(boost::lexical_cast<std::string>(idx));
        childidx.append("]");
        key.append(childidx);
        db->setRecord(key, i->ref, nullptr);
        children.push_back(childidx);
      } 
    rc = db->setRecord(basekey,"",&children);
    return rc;    
  }

    /**
     * \brief   Virtual function to validate object data; throws transaction exception if fails
     */
    std::string toStringItemAt(int idx);

    void pushBackValue(T strVal);

    void push_back(T val) { pushBackValue(val); }

    typename Container::iterator find(const T& item)
    {
      typename Container::iterator it;
      for (it=value.begin(); it!=value.end(); it++)
      {
        if (item == it->value) return it;
      }
      return it;
    }

    bool contains(const T& item)
    {
      if (find(item) == value.end()) return false;
      return true;
    }

    //? Remove all elements
    void clear() 
    { 
      // TODO: should we also clear refs?
      value.clear(); 
    }

    /*? Remove the first element that == item, returns true if an item was erased */
    bool erase(const T& item)
    {
      typename Container::iterator it;
      for (it=value.begin(); it!=value.end(); it++)
      {
        // TODO: should we also remove the equivalent index in refs?
        if (item == it->value) 
          {
          value.erase(it);
          return true;
          }
      }
      return false;
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

      //? Reads data from the database into this object
    virtual ClRcT read(MgtDatabase *db, std::string xpt = "")
    {
      std::string key;
      if(xpt.size() > 0)
      {
        key.assign(xpt);
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
        value.push_back(Elem(val));
        }
      }
      MgtRoot *mgtRoot = MgtRoot::getInstance();
      mgtRoot->addReference(this);
      dataXPath.assign(key);
      loadDb = true;

      return CL_OK;
    }

    void updateReference()
    {
      MgtObject *objRoot = dynamic_cast<MgtObject *>(this->root());

      for ( typename Container::iterator it = value.begin(); it != value.end(); it++)
        {
        if (!it->validValue)
          {
          std::string ref = it->ref;
          int firstSlash = ref.find('/');
          if (firstSlash != std::string::npos)
            {
              std::vector<MgtObject*> result;
              if (firstSlash == 0)  // skip the first slash
                objRoot->resolvePath(ref.c_str()+1,&result);
              else
                objRoot->resolvePath(ref.c_str(),&result);
              for (std::vector<MgtObject*>::iterator i = result.begin(); i != result.end(); ++i)
                {
                  MgtObject *obj = *i;
                  if (obj) 
                    {
                    it->value = dynamic_cast<T> (obj);
                    it->validValue = true;
                    break;
                    } 
                }
            }
          else
            {
            MgtObject *obj = objRoot->lookUpMgtObject("", ref);
            if (obj)  
              { //value.push_back((T)obj);
              it->value = dynamic_cast<T> (obj);
              it->validValue = true;
              }
            }
          }
        }
    }
};

      //? Push the valid contents of this Identifier list into a vector.  Skips entities that have not been resolved.
template<class T> std::vector<T>& operator << (std::vector<T>& lhs, const MgtIdentifierList<T>& rhs)
  {
      MgtIdentifierList<T>& rhsnc = (MgtIdentifierList<T>&) rhs;
    for (typename MgtIdentifierList<T>::iterator it = rhsnc.listBegin(); rhsnc.listEnd() != it; ++it)
      {
      lhs.push_back(*it);
      }
      
    return lhs;
  }

template<class T>
MgtIdentifierList<T>::MgtIdentifierList(const char* name) :
  MgtObject(name)
{
}

template<class T>
MgtIdentifierList<T>::~MgtIdentifierList()
{
  MgtRoot *mgtRoot = MgtRoot::getInstance();
  mgtRoot->removeReference(this);
}

template<class T>
std::string MgtIdentifierList<T>::toStringItemAt(int idx)
{
    std::stringstream ss;
    Elem& e = value[idx];
    if (e.validValue)
      {
      MgtObject *obj = dynamic_cast<MgtObject *>(e.value);
      if (obj)
        ss << obj->getFullXpath();
      else
        ss << "?" << e.ref;
      }
    else
      {
      ss << "?" << e.ref;
      }

    return ss.str();
}

template<class T>
void MgtIdentifierList<T>::toString(std::stringstream& xmlString, int depth, SerializationOptions opts)
{
  xmlString << '<' << tag;
  if (opts & MgtObject::SerializeNameAttribute)
    xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
  if (opts & MgtObject::SerializePathAttribute)
    xmlString << " path=" << "\"" << getFullXpath(true) << "\"";
  xmlString << '>';                

  int len = value.size();
  for (unsigned int i = 0; i < len; i++)
    {
      //MgtObject *obj = dynamic_cast<MgtObject *>(value[i].value);
      if (value[i].validValue && value[i].value)
      {
         xmlString << toStringItemAt(i);
         if (i+1<len && value[i+1].validValue && value[i+1].value) 
           xmlString << ", ";
      }
       
    }

  xmlString << "</" << tag << ">";
}

template<class T> bool MgtIdentifierList<T>::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
    return false;
}

template<class T> ClRcT MgtIdentifierList<T>::setObj(const std::string &val)
{
  std::vector<std::string> refs;
  boost::split(refs, val, boost::is_any_of(", "));
  value.clear();
  for(auto i = refs.begin(); i != refs.end(); i++)
    value.push_back(Elem(*i));
  
  MgtRoot::getInstance()->addReference(this);
  updateReference();
  lastChange = beat++;
  return CL_OK;
}

template<class T>
void MgtIdentifierList<T>::pushBackValue(T val)
{
  value.push_back(Elem(val));
}

};
#endif /* CLMGTIDENTIFIERLIST_HXX_ */
