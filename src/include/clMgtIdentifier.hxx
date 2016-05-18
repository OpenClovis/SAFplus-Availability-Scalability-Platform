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
 *  \brief Header file of the MgtIdentifier class which provides APIs to manage MGT "instance identifier" objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTIDENTIFIER_HXX_
#define CLMGTIDENTIFIER_HXX_

#include "clMgtProv.hxx"
#include "clMgtRoot.hxx"
namespace SAFplus
{
  template <class T>
  class MgtIdentifier : public MgtObject
  {
  public:
      /**
       *  Value of the "MgtIdentifier" object
       */
      T value;

      std::string ref;

  public:
    MgtIdentifier(const char* name);
    virtual ~MgtIdentifier();

    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts=SerializeNoOptions);
    virtual std::string strValue();

    /**
     * \brief   Define basic assignment operator
     */
    MgtIdentifier<T>& operator = (const T& val)
    {
        value = val;
        return *this;
    }
    /**
     * \brief   Define basic access (type cast) operator
     */
    operator T& ()
    {
      if (value==NULL) updateReference();  // was not loaded the first time, try to reresolve.  TODO: consider the efficiency of this 
      return value;
    }
    /**
     * \brief   Define basic comparison
     */
    bool operator == (const T& val) { return value==val; }

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
    virtual ClRcT setObj(const std::string &value);

    // Private function to write data to database, use write
    ClRcT setDb(std::string pxp = "", MgtDatabase *db = nullptr)
    {
      ClRcT ret = CL_OK;
  
      /* DB notation:
       *  [/safplusAmf/Component[@name="c1"]/serviceUnit] -> value : [su1]
       */
      if(db == nullptr)
        {
          db = MgtDatabase::getInstance();
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

      /* TODO: Should be base on 'ref' or 'value'??? ref is configuration but 'value' is real assignment */
      db->setRecord(basekey, ref);

      return ret;
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

    virtual ClRcT read(MgtDatabase *db, std::string xpt = "")
    {
      std::string key = getFullXpath();
      if (xpt.size() > 0)
        {
          key = getFullXpath(false);
          xpt.append(key);
          key = xpt;
        }

      if (db == nullptr)
        {
          db = MgtDatabase::getInstance();
        }

      dataXPath.assign(key);
      loadDb = true;

      std::string val;

      if (CL_OK == db->getRecord(key, val))
        {
          ref = val;
          MgtRoot *mgtRoot = MgtRoot::getInstance();
          mgtRoot->addReference(this);
        }

      return CL_OK;
    }

    //? Resolve this reference to a pointer if it is not yet resolved.  This can happen because this object may be loaded from the database before the object it points to is created.
    void updateReference()
    {
      MgtObject *objRoot = dynamic_cast<MgtObject *>(this->root());
      MgtObject *obj = NULL;
      if(ref.length() > 0)
        {
          if (ref.find('/') != std::string::npos)
            {
              std::vector<MgtObject*> result;
              objRoot->resolvePath(ref.c_str(),&result);
              for (std::vector<MgtObject*>::iterator i = result.begin(); i != result.end(); ++i)
                {
                  obj = *i;
                  if (obj) break;
                }
            }
          else
            {
          //MgtObject *obj = root->lookUpMgtObject(typeid(T).name(), ref);
            obj = objRoot->lookUpMgtObject("", ref);
            }

          if (obj) value = (T)obj;
          else
            {
              logWarning("MGT", "READ", "Object [%s] contains unresolved management tree reference [%s]", getFullXpath(true).c_str(), ref.c_str());
            }

        }
    }

  };

  /*
   * Implementation of MgtIdentifier class
   * G++ compiler: template function declarations and implementations must appear in the same file.
   */

  template <class T>
  MgtIdentifier<T>::MgtIdentifier(const char* name) : MgtObject(name)
  {
    value = nullptr;
  }

  template <class T>
  MgtIdentifier<T>::~MgtIdentifier()
  {}

  template <class T>
  ClBoolT MgtIdentifier<T>::set( void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
  {
    return CL_FALSE;
  }

template<class T> ClRcT MgtIdentifier<T>::setObj(const std::string &value)
{
  ref = value;
  updateReference();
  lastChange = beat++;
  return CL_OK;
}

  template <class T>
  void MgtIdentifier<T>::toString(std::stringstream& xmlString, int depth, SerializationOptions opts)
  {
      if (value == nullptr)
          return;

      xmlString << "<";
      xmlString << tag << ">";
      MgtObject *obj = dynamic_cast<MgtObject *>(value);
      if (obj)
          xmlString << value->getFullXpath();
      xmlString << "</" << tag << ">";
  }

  template <class T>
  std::string MgtIdentifier<T>::strValue()
  {
      std::stringstream ss;

      if (value == nullptr)
          return ss.str();

      MgtObject *obj = dynamic_cast<MgtObject *>(value);
      if (obj)
          ss << value->getFullXpath();
      return ss.str();
  }
}
#endif /* CLMGTIDENTIFIER_HXX_ */

/** \} */

