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
    typedef std::vector<T> ContainerType;
    std::vector<T> value;

public:
    MgtIdentifierList(const char* name);

    virtual ~MgtIdentifierList();

    virtual void toString(std::stringstream& xmlString);

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    /**
     * \brief   Virtual function to validate object data; throws transaction exception if fails
     */
    std::string toStringItemAt(T x);

    void pushBackValue(T strVal);
};

template<class T>
MgtIdentifierList<T>::MgtIdentifierList(const char* name) :
  MgtObject(name)
{
}

template<class T>
MgtIdentifierList<T>::~MgtIdentifierList()
{
}
template<class T>
std::string MgtIdentifierList<T>::toStringItemAt(T x)
{
    std::stringstream ss;

    MgtObject *obj = dynamic_cast<MgtObject *>(x);
    if (obj)
        ss << x->getFullXpath();

    return ss.str();
}

template<class T>
void MgtIdentifierList<T>::toString(std::stringstream& xmlString)
{
    for (unsigned int i = 0; i < value.size(); i++)
    {
        xmlString << "<" << tag << ">" << toStringItemAt(value.at(i)) << "</" << tag << ">";
    }
}

template<class T>
ClBoolT MgtIdentifierList<T>::set(const void *pBuffer, ClUint64T buffLen,
        SAFplus::Transaction& t)
{
    return CL_FALSE;
}

template<class T>
void MgtIdentifierList<T>::pushBackValue(T val)
{
    value.push_back(val);
}

};
#endif /* CLMGTIDENTIFIERLIST_HXX_ */
