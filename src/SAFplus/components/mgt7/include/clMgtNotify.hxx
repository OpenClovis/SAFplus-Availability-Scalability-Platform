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

#ifdef __cplusplus
extern "C" {
#endif
#include <clCommon.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

/**
 *  ClMgtNotify class provides APIs to manage Yang notifications
 */
class ClMgtNotify {
private:
    /*
     * Store the list of notification leaf
     */
    std::map<std::string, std::string> mLeafList;

public:
    std::string Name;
    std::string Module;

public:
    ClMgtNotify(const char* name);
    virtual ~ClMgtNotify();

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
    void sendNotification();
};

#endif /* CLMGTNOTIFY_HXX_ */
