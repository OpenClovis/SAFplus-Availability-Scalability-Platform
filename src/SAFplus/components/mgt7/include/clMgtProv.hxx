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

#ifndef CLMGTPROV_HPP_
#define CLMGTPROV_HPP_

#include "clMgtObject.hxx"
#include "clLogApi.h"

#include <typeinfo>
#include <iostream>
/**
 *  ClMgtProv class provides APIs to manage "provisioned" objects
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

    virtual std::string toString();

    /**
     * \brief   Define basic assignment operator
     */
    ClMgtProv<T>& operator = (const T& val) { Value = val; return *this;}
    /**
     * \brief   Define basic access (type cast) operator
     */
    operator T& () { return Value; }
    /**
     * \brief   Define basic comparison
     */
    bool operator == (const T& val) { return Value==val; }

  /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT validate(void *pBuffer, ClUint64T buffLen, ClTransaction& t);

    /**
     * \brief   Virtual function to abort object modification
     */
    virtual void abort(ClTransaction& t);

    /**
     * \brief	Virtual function called from netconf server to set object data
     */
    virtual void set(ClTransaction& t);

  /**
     * \brief   Define formal access operation
     */
    T& value() { return Value; }

    inline friend ostream & operator<< (ostream &os, const ClMgtProv &b)
    {
        return os<<(T)b.Value;
    }

    inline friend istream & operator>> (istream &is, const ClMgtProv &b)
    {
        return is>>(T)(b.Value);
    }

    virtual vector<string> *getChildNames();
};

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
std::string ClMgtProv<T>::toString()
{
    std::stringstream ss;
    ss << Value;
    return "<" + Name + ">" + ss.str() + "</" + Name + ">";
}

template <class T>
ClBoolT ClMgtProv<T>::validate(void *pBuffer, ClUint64T buffLen, ClTransaction& t)
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

    mValIndex = t.getSize();
    t.add((void *)valstr, strlen((char *)valstr) + 1);
    xmlFreeTextReader(reader);
    return CL_TRUE;
}

template <class T>
void ClMgtProv<T>::abort(ClTransaction& t)
{
    mValIndex = -1;
}

template <class T>
void ClMgtProv<T>::set(ClTransaction& t)
{
    std::stringstream ss;

    if (mValIndex == -1)
        return;

    char *valstr = (char *) t.get(mValIndex);

    if (((typeid(T) == typeid(bool)) || (typeid(T) == typeid(ClBoolT))) && (!strcmp((char*)valstr, "true")))
    {
        ss << "1";
        ss >> Value;
    }
    else
    {
        ss << valstr;
        ss >> Value;
    }

    mValIndex = -1;
}

/*
 * Leaf doesn't have children
 */
template <class T>
vector<string> *ClMgtProv<T>::getChildNames()
{
    return NULL;
}

#endif /* CLMGTPROV_HPP_ */

/** \} */

