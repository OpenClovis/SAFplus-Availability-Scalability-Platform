/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
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
#pragma once
#ifndef MGTFACTORY_HXX_
#define MGTFACTORY_HXX_

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <clMgtObject.hxx>
#include <MgtCreatorImpl.hxx>
namespace SAFplus
{
  class MgtFactory
  {
  private:
    MgtFactory();
    MgtFactory(const MgtFactory &) { }
    MgtFactory &operator=(const MgtFactory &) { return *this; }

  public:
    ~MgtFactory();
    static MgtFactory *getInstance()
    {
      static MgtFactory instance;
      return &instance;
    }
    static MgtObject* create(const std::string& xpath, const std::string& name);
    static void registerXpath(const std::string& xpath, IMgtCreator* creatorFn);

  private:
    static std::map<std::string, IMgtCreator* > &getObjectCreatorMap();
  };
};

#define MGT_REGISTER(classname) \
    private: \
    static const SAFplus::MgtCreatorImpl<classname> creator;

#define MGT_REGISTER_IMPL(classname, xpath) \
    const SAFplus::MgtCreatorImpl<classname> classname::creator(#xpath);


#endif /* MGTFACTORY_HXX_ */
