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
 *  \brief Header file of the ClMgtProvList class which provides APIs to manage "provisioned" objects
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

/*
 *
 */
template<class T>
class ClMgtProvList: public ClMgtObject
{
protected:
    /* This variable is used to index the value in Transaction object.*/
	vector<ClInt32T> mValIndexes;

public:
    /**
     *  Value of the "ClMgtProv" object
     */
    vector<T> Value;

public:
    ClMgtProvList(const char* name);

    virtual ~ClMgtProvList();

    virtual std::string toString();

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT
    validate(void *pBuffer, ClUint64T buffLen, ClTransaction& t);

    /**
     * \brief   Virtual function to abort object modification
     */
    virtual void
    abort(ClTransaction& t);

    /**
     * \brief   Virtual function called from netconf server to set object data
     */
    virtual void
    set(ClTransaction& t = NO_TRANSACTION);

    std::string toStringItemAt(T &x);

    // Overload PointList stream insertion operator
    inline friend ostream & operator<<(ostream &os, const ClMgtProvList &b)
    {
        copy(b.Value.begin(), b.Value.end(), ostream_iterator<T>(b));
        return os;
    }

    // Overload PointList stream extraction operator
    inline friend istream & operator>>(istream &is, const ClMgtProvList &b)
    {
        copy(istream_iterator<T>(is), istream_iterator<T>(), back_inserter(b));
        return is;
    }

    virtual vector<string> *getChildNames();

};
template<class T>
ClMgtProvList<T>::ClMgtProvList(const char* name) :
        ClMgtObject(name)
{
    mValIndexes.clear();
}

template<class T>
ClMgtProvList<T>::~ClMgtProvList()
{
}
template<class T>
std::string ClMgtProvList<T>::toStringItemAt(T &x)
{
    std::stringstream ss;
    ss << x;
    return "<" + Name + ">" + ss.str() + "</" + Name + ">";
}

template<class T>
std::string ClMgtProvList<T>::toString()
{
    std::string strOut;
    for (unsigned int i = 0; i < Value.size(); i++)
    {
        strOut += toStringItemAt(Value.at(i));
    }
    return strOut;
}

template<class T>
ClBoolT ClMgtProvList<T>::validate(void *pBuffer, ClUint64T buffLen,
        ClTransaction& t)
{
    const xmlChar *valstr, *namestr;
    int ret;
    ClInt32T valIndex;

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen,
            NULL, NULL, 0);

    if (reader != NULL)
    {
        ret = xmlTextReaderRead(reader);
        while (ret)
        {
            namestr = xmlTextReaderConstName(reader);

            if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT
                    && !strcmp((const char*) namestr, Name.c_str()))
            {
                ret = xmlTextReaderRead(reader);
                if (ret && xmlTextReaderHasValue(reader)
                        && !xmlTextReaderIsEmptyElement(reader))
                {
                    valstr = xmlTextReaderConstValue(reader);
                    valIndex = t.getSize();
                    t.add((void *)valstr, strlen((char *)valstr) + 1);
                    mValIndexes.push_back(valIndex);
                }
            }
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
    }
    return CL_TRUE;
}

template<class T>
void ClMgtProvList<T>::abort(ClTransaction& t)
{
    mValIndexes.clear();
}

template<class T>
void ClMgtProvList<T>::set(ClTransaction& t)
{
    ClInt32T valIndex;

    for (unsigned int i = 0; i < mValIndexes.size(); i++)
    {
        valIndex = mValIndexes[i];
        char *valstr = (char *) t.get(valIndex);
        T value;
        stringstream ss;

        if (((typeid(T) == typeid(bool)) || (typeid(T) == typeid(ClBoolT))) && (!strcmp((char*)valstr, "true")))
        {
            ss << "1";
            ss >> value;
        }
        else
        {
           ss << valstr;
           ss >> value;
        }

        Value.push_back(value);
    }

    mValIndexes.clear();
}
/*
 * List-leaf doesn't have children
 */
template <class T>
vector<string> *ClMgtProvList<T>::getChildNames()
{
    return NULL;
}

#endif /* CLMGTPROVLIST_HXX_ */
