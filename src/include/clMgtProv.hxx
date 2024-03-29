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
 *  \brief Header file of the MgtProv class which provides APIs to manage "provisioned" objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTPROV_HPP_
#define CLMGTPROV_HPP_

#include <typeinfo>
#include <iostream>

#include <clMgtBaseCommon.hxx>
#include "clMgtObject.hxx"
#include "clMgtDatabase.hxx"
#include "clLogIpi.hxx"

namespace SAFplus
{
  /**
   *  MgtProv class provides APIs to manage "provisioned" objects.  Provisioned objects are those that represent configuration that needs to be set by the operator.  This is in contrast to statistics objects which are not set by the operator.
   */
  template<class T>
    class MgtProv : public MgtObject
    {
    protected:
      /* This variable is used to index the value in Transaction object.*/
      ClInt32T mValIndex;

    public:
      /**
       *  Value of the "MgtProv" object
       */
      T value;

    public:
      MgtProv(const char* name);
      MgtProv(const char* name, const T& initialValue);
      virtual ~MgtProv();

      virtual void toString(std::stringstream& xmlString,int depth=SAFplusI::MgtToStringRecursionDepth,SerializationOptions opts=SerializeNoOptions);
      virtual std::string strValue();

      /**
       * \brief   Define basic assignment operator
       */
      MgtProv<T>& operator =(const T& val)
      {
        if (!(val == value))
          {
          value = val;
          lastChange = beat++;
          MgtObject *r = root();
          r->headRev = r->headRev + 1;
          // will be periodically flushed: setDb();
          }
        return *this;
      }
      /**
       * \brief   Define basic access (type cast) operator
       */
      operator T&()
      {
        return value;
      }
      /**
       * \brief   Define basic comparison
       */
      bool operator ==(const T& val)
      {
        return value == val;
      }

      /**
       * \brief   Virtual function to validate object data
       */
      virtual bool set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
      virtual void xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

      virtual bool set(const T& value, SAFplus::Transaction& t = SAFplus::NO_TXN);

      /**
       * \brief   Define formal access operation
       */
      T& val()
      {
        return value;
      }

      inline friend std::ostream &operator<<(std::ostream &os, const MgtProv &b)
      {
        return os << (T) b.value;
      }

      inline friend std::istream &operator>>(std::istream &is, const MgtProv &b)
      {
        return is >> (T) (b.value);
      }

      virtual std::vector<std::string> *getChildNames();

      // Private function to write data to database, use <ref>write</ref>
      ClRcT setDb(std::string pxp = "", MgtDatabase *db = nullptr);

      /**
       * \brief   Function to get data from database
       */
      ClRcT getDb(std::string pxp = "", MgtDatabase *db = nullptr);

      //? Function to write data to the database.
      virtual ClRcT write(MgtDatabase *db = nullptr, std::string xpath = "")
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
      virtual ClRcT read(MgtDatabase *db = nullptr, std::string xpath = "")
      {
        return getDb(xpath, db);
      }

      virtual ClRcT setObj(const std::string &value);
    };

  template<class T>
    class ProvOperation : public SAFplus::TransactionOperation
    {
    protected:
      MgtProv<T> *mOwner;
      void *mData;

    public:
      ProvOperation()
      {
        mOwner = nullptr;
        mData = nullptr;
      }
      void setData(MgtProv<T> *owner, void *data, ClUint64T buffLen);
      virtual bool validate(SAFplus::Transaction& t);
      virtual void commit();
      virtual void abort();
    };

  /*
   * Implementation of ProvOperation class
   * G++ compiler: template function declarations and implementations must appear in the same file.
   */

  template<class T>
    void ProvOperation<T>::setData(MgtProv<T> *owner, void *data, ClUint64T buffLen)
    {
      mOwner = owner;
      assert(mData == nullptr);
      mData = (void *) malloc(buffLen);

      if (!mData)
        return;

      memcpy(mData, data, buffLen);
    }

  template<class T>
    bool ProvOperation<T>::validate(SAFplus::Transaction& t)
    {
      if ((!mOwner) || (!mData))
        return false;

      return true;
    }

  template<class T>
    void ProvOperation<T>::commit()
    {
      if ((!mOwner) || (!mData))
        return;

      char *valstr = (char *) mData;
      deXMLize(valstr, mOwner, mOwner->value);

      mOwner->setDb();
      free(mData);
      mData = nullptr;
    }

  template<class T>
    void ProvOperation<T>::abort()
    {
      if ((!mOwner) || (!mData))
        return;

      free(mData);
      mData = nullptr;
    }
  /*
   * Implementation of MgtProv class
   * G++ compiler: template function declarations and implementations must appear in the same file.
   */

  template<class T>
  MgtProv<T>::MgtProv(const char* name) : MgtObject(name),value()
    {
      mValIndex = -1;
    }

  template<class T>
  MgtProv<T>::MgtProv(const char* name, const T& initialValue) : MgtObject(name),value(initialValue)
    {
      mValIndex = -1;
    }

  template<class T>
    MgtProv<T>::~MgtProv()
    {
    }

  template<class T>
  void MgtProv<T>::toString(std::stringstream& xmlString, int depth,SerializationOptions opts)
    {
      xmlString << "<" << tag;
      if (opts & MgtObject::SerializeNameAttribute)
        xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
      if (opts & MgtObject::SerializePathAttribute)
        xmlString << " path=" << "\"" << getFullXpath() << "\"";
      xmlString << ">";
      xmlString << value;
      xmlString << "</" << tag << ">";
    }

  template<class T>
    std::string MgtProv<T>::strValue()
    {
      std::stringstream ss;
      ss << value;
      return ss.str();
    }

  template<class T>
    void MgtProv<T>::xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
      if (!set((void *) pBuffer, buffLen, t))
        throw SAFplus::TransactionException(t);
    }

  template<class T>
    bool MgtProv<T>::set(const T& val, SAFplus::Transaction& t)
    {
      if (&t == &SAFplus::NO_TXN)
        {
          if (!(val == this->value))  // This awkward predicate formulation allows T to only implement ==
          {
          value = val;
          lastChange = beat++;
          MgtObject *r = root();
          r->headRev = r->headRev + 1;
          }
        }
      else
        {
          MgtObject *r = root();
          SimpleTxnOperation<T> *opt = new SimpleTxnOperation<T>(&value, val);
          ClUint32T newHeadRev = r->headRev + 1;
          SimpleTxnOperation<ClUint32T> *opt2 =
              new SimpleTxnOperation<ClUint32T>(&(r->headRev), newHeadRev);
          t.addOperation(opt);
          t.addOperation(opt2);
        }
      return true;
    }

  template<class T>
    bool MgtProv<T>::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
      xmlChar *valstr, *namestr;
      int ret, nodetyp, depth;
      std::stringstream ss;
#ifndef SAFplus7
      logDebug("MGT", "OBJ", "Validate [%.*s]", (int) buffLen, (const char*)pBuffer);
#endif
      xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, nullptr, nullptr, 0);
      if (!reader)
        {
          return false;
        }

      ret = xmlTextReaderRead(reader);
      if (ret <= 0)
        {
          xmlFreeTextReader(reader);
          return false;
        }

      namestr = (xmlChar *) xmlTextReaderConstName(reader);

      if (strcmp((char *) namestr, tag.c_str()))
        {
#ifndef SAFplus7
          logDebug("MGT","PROV","Name [%s], XML [%s]",tag.c_str(),(char *)namestr);
#endif
          xmlFreeTextReader(reader);
          return false;
        }

      ret = xmlTextReaderRead(reader);
      if (ret <= 0)
        {
          xmlFreeTextReader(reader);
          return false;
        }

      depth = xmlTextReaderDepth(reader);
      nodetyp = xmlTextReaderNodeType(reader);
      valstr = (xmlChar *) xmlTextReaderValue(reader);

      if ((nodetyp != XML_TEXT_NODE) || (depth != 1))
        {
          xmlFreeTextReader(reader);
          return false;
        }

      ProvOperation<T> *opt = new ProvOperation<T>;
      opt->setData(this, (void *) valstr, strlen((char *) valstr) + 1);
      MgtObject *r = root();
      ClUint32T newHeadRev = r->headRev + 1;
      SimpleTxnOperation<ClUint32T> *opt2 = new SimpleTxnOperation<ClUint32T>(&(r->headRev), newHeadRev);
      t.addOperation(opt);
      t.addOperation(opt2);

      xmlFree(valstr);
      xmlFreeTextReader(reader);
      return (opt->validate(t) && opt2->validate(t));
    }

  /*
   * Leaf doesn't have children
   */
  template<class T>
    std::vector<std::string> *MgtProv<T>::getChildNames()
    {
      return nullptr;
    }

  template<class T>
    ClRcT MgtProv<T>::setDb(std::string pxp, MgtDatabase *db)
    {
      if ((!loadDb)&&(!replicated)) // Not a configuration item
        return CL_OK;
     
      std::string key;
      if (dataXPath.size() > 0)
        {
          key = dataXPath;
        }
      else if(pxp.size() > 0)
        {
          key.assign(pxp);
          key.append(getFullXpath(false));
        }
      else key = getFullXpath(true);

      std::stringstream ss;
      ss << value;
      if(!db) db=MgtObject::getDb();
      //this function will implement later
      if(db == nullptr)
      {
    	  return CL_OK;
      }
      assert(db);

      return db->setRecord(key, ss.str());
    }

  template<class T>
    ClRcT MgtProv<T>::getDb(std::string pxp, MgtDatabase *db)
    {
      if ((!loadDb)&&(!replicated))
        return CL_OK;
      if (!db) db = MgtObject::getDb();
      assert(db);
      std::string key;
      if (pxp.size() > 0)
        {
          key.assign(pxp);
          key.append(getFullXpath(false));
        }
      else key = getFullXpath();

      dataXPath.assign(key);
      loadDb = true;

      std::string val;
      ClRcT rc = db->getRecord(key, val);
      if (CL_OK != rc)
        {
          return rc;
        }
      deXMLize(val, this, value);
      //lastChange = beat++;
      return rc;
    }

  template<class T>
    ClRcT MgtProv<T>::setObj(const std::string &val)
    {
      if (!settable)
        {
        logDebug("MGT","PROV","attempt to set a non-config object [%s]",tag.c_str());
        return CL_ERR_BAD_OPERATION;
        }

      deXMLize(val, this, value);
      lastChange = beat++;

      MgtObject *r = root();
      r->headRev = r->headRev + 1;

      return CL_OK;
    }

}
;
#endif /* CLMGTPROV_HPP_ */

/** \} */

