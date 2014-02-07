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
 *  \brief Header file of the ClMgtIndex class which provides APIs to manage MGT statistic objects
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTSTAT_HXX_
#define CLMGTSTAT_HXX_

#include "clMgtObject.hxx"

template <class T>
class ClMgtStat : public ClMgtObject
{
public:
    /**
     *  Value of the "ClMgtProv" object
     */
    T		Value;

public:
    ClMgtStat(const char* name);
    virtual ~ClMgtStat();

    virtual std::string toString();

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

};

/*
 * Implementation of ClMgtStat class
 * G++ compiler: template function declarations and implementations must appear in the same file.
 */

template <class T>
ClMgtStat<T>::ClMgtStat(const char* name) : ClMgtObject(name)
{}

template <class T>
ClMgtStat<T>::~ClMgtStat()
{}

template <class T>
std::string ClMgtStat<T>::toString()
{
    std::stringstream ss;
    ss << Value;
    return "<" + Name + ">" + ss.str() + "</" + Name + ">";
}

template <class T>
ClBoolT ClMgtStat<T>::validate( void *pBuffer, ClUint64T buffLen, ClTransaction& t)
{
    return CL_FALSE;
}

template <class T>
void ClMgtStat<T>::abort(ClTransaction& t)
{
}

template <class T>
void ClMgtStat<T>::set(ClTransaction& t)
{
    // Do nothing
}

#endif /* CLMGTSTAT_HXX_ */

/** \} */

