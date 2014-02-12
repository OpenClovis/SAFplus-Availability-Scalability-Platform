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

#ifndef MGTFACTORY_HXX_
#define MGTFACTORY_HXX_

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <clMgtObject.hxx>
#include <MgtCreatorImpl.hxx>

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
        static ClMgtObject* create(const std::string& xpath);
        static void registerXpath(const std::string& xpath, IMgtCreator* creatorFn);

    private:
        static std::map<std::string, IMgtCreator* > &getObjectCreatorMap();
};

#define REGISTER(classname) \
    private: \
    static const MgtCreatorImpl<classname> creator;

#define REGISTERIMPL(classname, xpath) \
    const MgtCreatorImpl<classname> classname::creator(#xpath);


#endif /* MGTFACTORY_HXX_ */
