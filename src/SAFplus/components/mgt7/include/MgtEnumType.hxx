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
#include <vector>

namespace SAFplus
{
    template <typename EnumMgr, typename Enum>
    struct MgtEnumType {
        typedef std::pair<Enum, std::string> pair_t;
        typedef std::vector<pair_t> vec_t;
        typedef typename vec_t::const_iterator vec_t_iter;
        static std::string toString(const Enum en);
        static Enum toEnum(const std::string &en);
    };

    template <typename EnumMgr, typename Enum>
    std::string MgtEnumType<EnumMgr, Enum>::toString(const Enum en) {
        for (vec_t_iter it = EnumMgr::en2str_vec.begin(); it < EnumMgr::en2str_vec.end(); it++) {
            if (en == it->first)
                return it->second;
        }
        return "";
    }

    template <typename EnumMgr, typename Enum>
    Enum MgtEnumType<EnumMgr, Enum>::toEnum(const std::string &en) {
        for (vec_t_iter it = EnumMgr::en2str_vec.begin(); it < EnumMgr::en2str_vec.end(); it++) {
            if (en == it->second)
                return it->first;
        }
        return (Enum)0;
    }

} /* namespace SAFplus */
#endif /* MGTENUMTYPE_HXX_ */
