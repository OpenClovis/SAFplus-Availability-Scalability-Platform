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

#include <string>

#ifndef RENAMEPBFILE_HXX_
#define RENAMEPBFILE_HXX_

namespace SAFplus
  {
    namespace Rpc
      {

        /*
         *
         */
        class RenamePbFile
          {
          public:
            RenamePbFile(const std::string &, const std::string &);
            virtual ~RenamePbFile();
            void Replace(const std::string &infile, const std::string &outfile);
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* RENAMEPBFILE_HXX_ */
