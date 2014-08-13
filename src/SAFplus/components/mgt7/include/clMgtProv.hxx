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

#include <typeinfo>
#include <iostream>

extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */

#include "clMgtObject.hxx"
#include "clMgtDatabase.hxx"
#include "clLogApi.hxx"


namespace SAFplus
{
/**
 *  MgtProv class provides APIs to manage "provisioned" objects.  Provisioned objects are those that represent configuration that needs to be set by the operator.  This is in contrast to statistics objects which are not set by the operator.
 */
template <class T>
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
    virtual ~MgtProv();

    virtual void toString(std::stringstream& xmlString);
    virtual std::string strValue();


    /**
     * \brief   Define basic assignment operator
     */
    MgtProv<T>& operator = (const T& val)
    {
        value = val;
        setDb();
        return *this;
    }
    /**
     * \brief   Define basic access (type cast) operator
     */
    operator T& ()
    {
        getDb();
        return value;
    }
    /**
     * \brief   Define basic comparison
     */
    bool operator == (const T& val) { return value==val; }

  /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
    virtual void xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);
 
    virtual bool set(const T& value, SAFplus::Transaction& t=SAFplus::NO_TXN);

    /**
     * \brief   Define formal access operation
     */
    T& val()
    {
        getDb();
        return value;
    }

    inline friend std::ostream & operator<< (std::ostream &os, const MgtProv &b)
    {
        return os<<(T)b.value;
    }

    inline friend std::istream & operator>> (std::istream &is, const MgtProv &b)
    {
        return is>>(T)(b.value);
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

    /**
     *
     */
    virtual ClRcT write(ClMgtDatabase *db=NULL)
    {
      return setDb();
    }
    /**
     *
     */
    virtual ClRcT read(ClMgtDatabase *db=NULL)
    {
      return getDb();
    }


};

template <class T>
class ProvOperation : public SAFplus::TransactionOperation
{
protected:
    MgtProv<T> *mOwner;
    void         *mData;

public:
    ProvOperation()
    {
        mOwner = NULL;
        mData = NULL;
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

template <class T>
void ProvOperation<T>::setData(MgtProv<T> *owner, void *data, ClUint64T buffLen)
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
    deXMLize(valstr,mOwner,mOwner->value);

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
 * Implementation of MgtProv class
 * G++ compiler: template function declarations and implementations must appear in the same file.
 */

template <class T>
MgtProv<T>::MgtProv(const char* name) : MgtObject(name)
{
    mValIndex = -1;
}

template <class T>
MgtProv<T>::~MgtProv()
{}

template <class T>
void MgtProv<T>::toString(std::stringstream& xmlString)
{
    getDb();
    xmlString << "<";
    xmlString << name << ">";
    xmlString << value;
    xmlString << "</" << name << ">";
}

template <class T>
std::string MgtProv<T>::strValue()
{
    std::stringstream ss;
    ss <<  value ;
    return ss.str();
}

template <class T> void MgtProv<T>::xset(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
  if (!set((void *)pBuffer,buffLen,t)) throw SAFplus::TransactionException(t);
}

template <class T> bool MgtProv<T>::set(const T& val, SAFplus::Transaction& t)
  {
  if (&t == &SAFplus::NO_TXN) value = val;
  else
    {
    SimpleTxnOperation<T> *opt = new SimpleTxnOperation<T>(&value,val);
    t.addOperation(opt);
    }
  }

template <class T>
ClBoolT MgtProv<T>::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
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

    if (strcmp((char *)namestr, name.c_str()))
    {
        logDebug("MGT","PROV","Name [%s], XML [%s]",name.c_str(),(char *)namestr);
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
std::vector<std::string> *MgtProv<T>::getChildNames()
{
    return NULL;
}

template <class T>
ClRcT MgtProv<T>::setDb()
{
    std::string key = getFullXpath();

    std::stringstream ss;
    ss << value;

    ClMgtDatabase *db = ClMgtDatabase::getInstance();

    return db->setRecord(key, ss.str());
}

template <class T>
ClRcT MgtProv<T>::getDb()
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

    deXMLize(value,this,value);
    return rc;
}

};
#endif /* CLMGTPROV_HPP_ */

/** \} */

