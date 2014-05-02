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
 *  \brief Header file of the ClMgtProv class which provides APIs to manage "provisioned" objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTPROV_HPP_
#define CLMGTPROV_HPP_

#include "clMgtObject.hxx"
#include "clMgtDatabase.hxx"
#include "clLogApi.h"

#include <typeinfo>
#include <iostream>

namespace SAFplus
{
/**
 *  ClMgtProv class provides APIs to manage "provisioned" objects.  Provisioned objects are those that represent configuration that needs to be set by the operator.  This is in contrast to statistics objects which are not set by the operator.
 */
template <class T>
class ClMgtProv : public ClMgtObject
{
protected:
    /* This variable is used to index the value in Transaction object.*/
    ClInt32T mValIndex;

public:
    /**
     *  Value of the "ClMgtProv" object
     */
    T		Value;

public:
    ClMgtProv(const char* name);
    virtual ~ClMgtProv();

    virtual void toString(std::stringstream& xmlString);
    virtual std::string strValue();

    /**
     * \brief   Define basic assignment operator
     */
    ClMgtProv<T>& operator = (const T& val)
    {
        Value = val;
        setDb();
        return *this;
    }
    /**
     * \brief   Define basic access (type cast) operator
     */
    operator T& ()
    {
        getDb();
        return Value;
    }
    /**
     * \brief   Define basic comparison
     */
    bool operator == (const T& val) { return Value==val; }

  /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
    virtual void xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    /**
     * \brief   Define formal access operation
     */
    T& value()
    {
        getDb();
        return Value;
    }

    inline friend std::ostream & operator<< (std::ostream &os, const ClMgtProv &b)
    {
        return os<<(T)b.Value;
    }

    inline friend std::istream & operator>> (std::istream &is, const ClMgtProv &b)
    {
        return is>>(T)(b.Value);
    }

    virtual std::vector<std::string> *getChildNames();

    /**
     * \brief   Function to set data to database
     */
    ClRcT setDb();

    /**
     * \brief   Function to get data from database
     */
    ClRcT getDb();

};

template <class T>
class ProvOperation : public SAFplus::TransactionOperation
{
protected:
    ClMgtProv<T> *mOwner;
    void         *mData;

public:
    ProvOperation()
    {
        mOwner = NULL;
        mData = NULL;
    }
    void setData(ClMgtProv<T> *owner, void *data, ClUint64T buffLen);
    virtual bool validate(SAFplus::Transaction& t);
    virtual void commit();
    virtual void abort();
};

/*
 * Implementation of ProvOperation class
 * G++ compiler: template function declarations and implementations must appear in the same file.
 */

template <class T>
void ProvOperation<T>::setData(ClMgtProv<T> *owner, void *data, ClUint64T buffLen)
{
    mOwner = owner;
    mData = (void *) malloc (buffLen);

    if(!mData) return;

    memcpy(mData, data, buffLen);
}

template <class T>
bool ProvOperation<T>::validate(SAFplus::Transaction& t)
{
    if ((!mOwner) || (!mData))
        return false;

    return true;
}

template <class T>
void ProvOperation<T>::commit()
{
    if ((!mOwner) || (!mData))
        return;

    std::stringstream ss;

    char *valstr = (char *) mData;

    if (((typeid(T) == typeid(bool)) || (typeid(T) == typeid(ClBoolT))) && (!strcmp((char*)valstr, "true")))
    {
        ss << "1";
        ss >> mOwner->Value;
    }
    else
    {
        ss << valstr;
        ss >> mOwner->Value;
    }

    mOwner->setDb();
    free(mData);
    mData = NULL;
}

template <class T>
void ProvOperation<T>::abort()
{
    if ((!mOwner) || (!mData))
        return;

    free(mData);
    mData = NULL;
}
/*
 * Implementation of ClMgtProv class
 * G++ compiler: template function declarations and implementations must appear in the same file.
 */

template <class T>
ClMgtProv<T>::ClMgtProv(const char* name) : ClMgtObject(name)
{
    mValIndex = -1;
}

template <class T>
ClMgtProv<T>::~ClMgtProv()
{}

template <class T>
void ClMgtProv<T>::toString(std::stringstream& xmlString)
{
    getDb();
    xmlString << "<";
    xmlString << Name << ">";
    xmlString << Value;
    xmlString << "</" << Name << ">";
}

template <class T>
std::string ClMgtProv<T>::strValue()
{
    std::stringstream ss;
    ss <<  Value ;
    return ss.str();
}

template <class T> void ClMgtProv<T>::xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
  if (!set(pBuffer,buffLen,t)) throw SAFplus::TransactionException(t);
}

template <class T>
ClBoolT ClMgtProv<T>::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
    xmlChar        *valstr, *namestr;
    int             ret, nodetyp, depth;
    std::stringstream ss;
#ifndef SAFplus7
    clLogDebug("MGT", "OBJ", "Validate [%.*s] ", (int) buffLen, (const char*)pBuffer);
#endif
    xmlTextReaderPtr reader = xmlReaderForMemory((const char*)pBuffer, buffLen, NULL,NULL, 0);
    if(!reader)
    {
        return CL_FALSE;
    }

    ret = xmlTextReaderRead(reader);
    if (ret <= 0)
    {
        xmlFreeTextReader(reader);
        return CL_FALSE;
    }

    namestr = (xmlChar *)xmlTextReaderConstName(reader);

    if (strcmp((char *)namestr, Name.c_str()))
    {
        xmlFreeTextReader(reader);
        return CL_FALSE;
    }

    ret = xmlTextReaderRead(reader);
    if (ret <= 0)
    {
        xmlFreeTextReader(reader);
        return CL_FALSE;
    }

    depth = xmlTextReaderDepth(reader);
    nodetyp = xmlTextReaderNodeType(reader);
    valstr = (xmlChar *)xmlTextReaderValue(reader);

    if ((nodetyp != XML_TEXT_NODE) || (depth != 1))
    {
        xmlFreeTextReader(reader);
        return CL_FALSE;
    }

    ProvOperation<T> *opt = new ProvOperation<T>;
    opt->setData(this, (void *)valstr, strlen((char *)valstr) + 1);
    t.addOperation(opt);

    xmlFree(valstr);
    xmlFreeTextReader(reader);
    return opt->validate(t);
}

/*
 * Leaf doesn't have children
 */
template <class T>
std::vector<std::string> *ClMgtProv<T>::getChildNames()
{
    return NULL;
}

template <class T>
ClRcT ClMgtProv<T>::setDb()
{
    std::string key = getFullXpath();

    std::stringstream ss;
    ss << Value;

    ClMgtDatabase *db = ClMgtDatabase::getInstance();

    return db->setRecord(key, ss.str());
}

template <class T>
ClRcT ClMgtProv<T>::getDb()
{
    ClRcT rc = CL_OK;
    std::string key = getFullXpath();
    std::string value;

    ClMgtDatabase *db = ClMgtDatabase::getInstance();

    rc = db->getRecord(key, value);
    if (CL_OK != rc)
    {
        return rc;
    }

    std::stringstream ss;

    if (((typeid(T) == typeid(bool)) || (typeid(T) == typeid(ClBoolT))) && (!value.compare("true")))
    {
        ss << "1";
        ss >> Value;
    }
    else
    {
        ss << value;
        ss >> Value;
    }

    return rc;
}

};
#endif /* CLMGTPROV_HPP_ */

/** \} */

