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

#ifndef MGTENUMTYPE_HXX_
#define MGTENUMTYPE_HXX_

#include <string>
#include <map>

namespace SAFplus
{
    template <typename EnumMgr, typename Enum>
    struct MgtEnumType {
        typedef std::pair<Enum, std::string> pair_t;
        typedef std::map<Enum, std::string> map_t;
        typedef typename map_t::const_iterator map_t_iter;
        static std::string toString(const Enum en);
        static Enum toEnum(const std::string &en);
        static const char* c_str(const Enum en);
    };

    template <typename EnumMgr, typename Enum>
    std::string MgtEnumType<EnumMgr, Enum>::toString(const Enum en) {
        map_t_iter it = EnumMgr::en2str_map.find(en);
        return (it != EnumMgr::en2str_map.end()) ? it->second : "";
    }

    template <typename EnumMgr, typename Enum>
    Enum MgtEnumType<EnumMgr, Enum>::toEnum(const std::string &en) {
        map_t_iter it = EnumMgr::en2str_map.begin();
        while (it != EnumMgr::en2str_map.end())
          {
            if (!en.compare(it->second))
                return it->first;
            it++;
          }
        return (Enum)0;
    }

    template <typename EnumMgr, typename Enum>
    const char* MgtEnumType<EnumMgr, Enum>::c_str(const Enum en) {
        return static_cast<const char*>(EnumMgr::en2str_map.find(en)->second.c_str());
    }
} /* namespace SAFplus */
#endif /* MGTENUMTYPE_HXX_ */
