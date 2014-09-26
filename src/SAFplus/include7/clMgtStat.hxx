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
#pragma once
#ifndef CLMGTSTAT_HXX_
#define CLMGTSTAT_HXX_

#include "clMgtProv.hxx"

namespace SAFplus
{
  template <class T>
  class ClMgtStat : public ClMgtProv<T>
  {
  public:
    ClMgtStat(const char* name);
    virtual ~ClMgtStat();

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

  };

  /*
   * Implementation of ClMgtStat class
   * G++ compiler: template function declarations and implementations must appear in the same file.
   */

  template <class T>
  ClMgtStat<T>::ClMgtStat(const char* name) : ClMgtProv<T>(name)
  {}

  template <class T>
  ClMgtStat<T>::~ClMgtStat()
  {}

  template <class T>
  ClBoolT ClMgtStat<T>::set( void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
  {
    return CL_FALSE;
  }
}
#endif /* CLMGTSTAT_HXX_ */

/** \} */

