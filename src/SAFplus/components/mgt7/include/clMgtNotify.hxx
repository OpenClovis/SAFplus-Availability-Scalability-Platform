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
 *  \brief Header file of the ClMgtNotify class which provides APIs to manage Yang notifications
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */

#ifndef CLMGTNOTIFY_HXX_
#define CLMGTNOTIFY_HXX_

#include <map>
#include <string>
#include "clMgtObject.hxx"
#include "clDbg.hxx"

extern "C"
{
#include <clCommon.h>
} /* end extern 'C' */

namespace SAFplus
{
/**
 *  MgtNotify class provides APIs to manage Yang notifications
 */
class MgtNotify : public MgtObject
{
private:
    /*
     * Store the list of notification leaf
     */
    std::map<std::string, std::string> mLeafList;

public:
    std::string name;
    std::string Module;

public:
    MgtNotify(const char* name);
    virtual ~MgtNotify();
    virtual void toString(std::stringstream& xmlString)
    {

    }
    /**
     * Function add a leaf to the notification
     */
    void addLeaf(std::string leaf, std::string defaultValue);

    /**
     * Function set value to the leaf
     */
    void setLeaf(std::string leaf, std::string value);

    /**
     * Function get value from the leaf
     */
    void getLeaf(std::string leaf, std::string *value);

    /**
     * Function to send notification to the netconf server
     */
    void sendNotification(SAFplus::Handle hdl);

    virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
      clDbgNotImplemented("MgtNotify::set");
    }

};
};

#endif /* CLMGTNOTIFY_HXX_ */
