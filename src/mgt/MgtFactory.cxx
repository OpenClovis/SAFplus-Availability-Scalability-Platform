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

#include "MgtFactory.hxx"

using namespace std;
namespace SAFplus
{
void MgtFactory::registerXpath(const std::string& xpath, IMgtCreator* creatorFn)
{
    getObjectCreatorMap()[xpath] = creatorFn;
}

MgtObject* MgtFactory::create(const std::string& xpath)
{
    std::map<std::string, IMgtCreator*>::iterator i;

    i = getObjectCreatorMap().find(xpath);

    if (i != getObjectCreatorMap().end())
    {
        return i->second->create();
    }
    else
    {
        return (MgtObject*) nullptr;
    }
}

MgtFactory::MgtFactory()
{
    // TODO Auto-generated constructor stub

}

MgtFactory::~MgtFactory()
{
    // TODO Auto-generated destructor stub
}

std::map<std::string, IMgtCreator*>& MgtFactory::getObjectCreatorMap()
{
    static std::map<std::string, IMgtCreator*> mgtObjectCreatorMap;
    return mgtObjectCreatorMap;
}
};
