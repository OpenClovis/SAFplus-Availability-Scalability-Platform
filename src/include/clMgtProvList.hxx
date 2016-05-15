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

#include "clMgtObject.hxx"

#include <typeinfo>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <vector>

extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */


namespace SAFplus
{
/*
 * Represents YANG leaf-list primitive
 */
template <class T>
class ProvListOperation;

template<class T>
class MgtProvList: public MgtObject
{
public:
    /**
     *  value of the "ClMgtProv" object
     */
    typedef std::vector<T> ContainerType;
    std::vector<T> value;
    ProvListOperation<T> *mOpt;

public:
    MgtProvList(const char* name);

    virtual ~MgtProvList();

  virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts=SerializeNoOptions);

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    /**
     * \brief   Virtual function to validate object data; throws transaction exception if fails
     */
    virtual void xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
  
    std::string toStringItemAt(T &x);

    // Overload PointList stream insertion operator
    inline friend std::ostream & operator<<(std::ostream &os, const MgtProvList &b)
    {
        copy(b.value.begin(), b.value.end(), std::ostream_iterator<T>(b));
        return os;
    }

    // Overload PointList stream extraction operator
    inline friend std::istream & operator>>(std::istream &is, const MgtProvList &b)
    {
        copy(std::istream_iterator<T>(is), std::istream_iterator<T>(), std::back_inserter(b));
        return is;
    }

    /**
     * \brief   Function to set data to database
     */
    ClRcT setDb(std::string pxp = "",MgtDatabase *db=nullptr);

    /**
     * \brief   Function to get data from database
     */
    ClRcT getDb(std::string pxp = "",MgtDatabase *db=nullptr);
    /**
     *
     */
    virtual ClRcT write(MgtDatabase *db=nullptr, std::string xpath = "")
    {
      return setDb(xpath,db);
    }

    virtual ClRcT writeChanged(uint64_t firstBeat, uint64_t beat,SAFplus::MgtDatabase *db, std::string xpath)
      {
        if ((lastChange > firstBeat)&&(lastChange <= beat)) return setDb(xpath, db);
        return CL_OK;
      }

    /**
     *
     */
    virtual ClRcT read(MgtDatabase *db=nullptr, std::string xpath = "")
    {
      return getDb(xpath,db);
    }
    void pushBackValue(const std::string& strVal);
};

template <class T>
class ProvListOperation : public SAFplus::TransactionOperation
{
protected:
    MgtProvList<T> *mOwner;
    std::vector<void *> mData;

public:
    ProvListOperation()
    {
        mOwner = nullptr;
    }
    void setOwner(MgtProvList<T> *owner);
    void addData(void *data, ClUint64T buffLen);
    virtual bool validate(SAFplus::Transaction& t);
    virtual void commit();
    virtual void abort();
};

template <class T>
void ProvListOperation<T>::setOwner(MgtProvList<T> *owner)
{
    mOwner = owner;
}

template <class T>
void ProvListOperation<T>::addData(void *data, ClUint64T buffLen)
{
    void *tempData = (void *) malloc (buffLen);

    if(!tempData) return;

    memcpy(tempData, data, buffLen);
    mData.push_back(tempData);
}

template <class T>
bool ProvListOperation<T>::validate(SAFplus::Transaction& t)
{
    if ((!mOwner) || (mData.size() == 0))
        return false;

    return true;
}

template <class T>
void ProvListOperation<T>::commit()
{
    if ((!mOwner) || (mData.size() == 0))
        return;

    mOwner->value.clear();

    for(unsigned int i = 0; i < mData.size(); i++)
    {
        mOwner->pushBackValue((char *)mData[i]);
        free(mData[i]);
    }

    mOwner->setDb();
    mOwner->mOpt = nullptr;
    mData.clear();
}

template <class T>
void ProvListOperation<T>::abort()
{
    if ((!mOwner) || (mData.size() == 0))
        return;

    for(unsigned int i = 0; i < mData.size(); i++)
    {
        free(mData[i]);
    }

    mOwner->mOpt = nullptr;
    mData.clear();
}

template<class T>
MgtProvList<T>::MgtProvList(const char* name) :
  MgtObject(name)
{
    mOpt = nullptr;
}

template<class T>
MgtProvList<T>::~MgtProvList()
{
}
template<class T>
std::string MgtProvList<T>::toStringItemAt(T &x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template<class T>
void MgtProvList<T>::toString(std::stringstream& xmlString, int depth, SerializationOptions opts)
{
    for (unsigned int i = 0; i < value.size(); i++)
    {
        xmlString << "<" << tag << ">" << toStringItemAt(value.at(i)) << "</" << tag << ">";
    }
}

template<class T> void MgtProvList<T>::xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
  if (!set(pBuffer,buffLen,t)) throw SAFplus::TransactionException(t);
}

template<class T>
ClBoolT MgtProvList<T>::set(const void *pBuffer, ClUint64T buffLen,
        SAFplus::Transaction& t)
{
    const xmlChar *valstr, *namestr;
    int ret;
    bool addOpt = false;

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, nullptr, nullptr, 0);

    if (reader != nullptr)
    {
        ret = xmlTextReaderRead(reader);
        while (ret)
        {
            namestr = xmlTextReaderConstName(reader);

            if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT
                    && !strcmp((const char*) namestr, tag.c_str()))
            {
                ret = xmlTextReaderRead(reader);
                if (ret && xmlTextReaderHasValue(reader)
                        && !xmlTextReaderIsEmptyElement(reader))
                {
                    valstr = xmlTextReaderConstValue(reader);
                    if (!mOpt)
                    {
                        mOpt = new ProvListOperation<T>;
                        mOpt->setOwner(this);
                        addOpt = true;
                    }

                    mOpt->addData((void *)valstr, strlen((char *)valstr) + 1);
                }
            }
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
    }

    if (!mOpt)
        return CL_FALSE;

    if (addOpt)
        t.addOperation(mOpt);

    return mOpt->validate(t);
}

template<class T>
void MgtProvList<T>::pushBackValue(const std::string& strVal)
{
    T val;

    deXMLize(strVal,this,val); 

    value.push_back(val);
}

#if 0
/*
 * List-leaf doesn't have children
 */
template <class T>
std::vector<std::string> *MgtProvList<T>::getChildNames()
{
    return nullptr;
}
#endif

template <class T>
ClRcT MgtProvList<T>::setDb(std::string pxp,MgtDatabase *db)
{
    ClRcT rc = CL_OK;

    //TODO: Temporary not support because of bug
#if 0
    if (!loadDb)
      return rc;

    std::string key;

    if (dataXPath.size() > 0)
    {
      key.assign(dataXPath);
    }
    else if(pxp.size() > 0)
    {
      key.assign(pxp);
      key.append(getFullXpath(false));
    }
    else
      key.assign(getFullXpath(true));

    if(db == nullptr)
    {
      db = MgtDatabase::getInstance();
    }
    if(!db->isInitialized())
    {
        return CL_ERR_NOT_INITIALIZED;
    }

    std::vector<std::string> iter = db->iterate(key);

    int updateCount = (iter.size() < value.size()) ? iter.size() : value.size();

    for (int i = 1; i <= updateCount; i++)
    {
        std::string itemkey = "";
        std::stringstream s;
        s << i;
        itemkey.append(key).append("[").append(s.str()).append("]");
        if (!db->setRecord(itemkey, toStringItemAt(value.at(i-1))))
        {
            db->insertRecord(itemkey, toStringItemAt(value.at(i-1)));
        }
    }

    if (iter.size() < value.size())
    {
        for (int i = iter.size() + 1; i <= value.size(); i++)
        {
            std::string itemkey = "";
            std::stringstream s;
            s << i;
            itemkey.append(key).append("[").append(s.str()).append("]");
            db->insertRecord(itemkey, toStringItemAt(value.at(i-1)));
        }
    }
    else
    {
        for (int i = value.size() + 1; i <= iter.size(); i++)
        {
            std::string itemkey = "";
            std::stringstream s;
            s << i;
            itemkey.append(key).append("[").append(s.str()).append("]");
            db->deleteRecord(itemkey);
        }
    }
#endif
    return rc;
}

template <class T>
ClRcT MgtProvList<T>::getDb(std::string pxp,MgtDatabase *db)
{
    ClRcT rc = CL_OK;

    if (!loadDb)
      return rc;

    std::string key;
    if(pxp.size() > 0)
    {
      key.assign(pxp);
      key.append(getFullXpath(false));
    }
    else
      key.assign(getFullXpath(true));

    if(db == nullptr)
    {
      db = MgtDatabase::getInstance();
    }
    if(!db->isInitialized())
    {
        return CL_ERR_NOT_INITIALIZED;
    }

    std::vector<std::string> iter;
    db->iterate(key, iter);

    value.clear();
    for (std::vector<std::string>::iterator it=iter.begin(); it!=iter.end(); it++)
    {
        std::string value;
        if (db->getRecord(*it, value))
        {
          this->pushBackValue(value);
        }
    }

    dataXPath.assign(key);
    loadDb = true;

    return rc;
}
};
#endif /* CLMGTPROVLIST_HXX_ */
