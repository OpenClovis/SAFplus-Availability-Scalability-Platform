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

#ifndef CLMGTACTION_HXX_
#define CLMGTACTION_HXX_

#include <clMgtRpc.hxx>

namespace SAFplus
{
  class MgtAction: public MgtRpc
  {
  public:
    /*
     * Store the path to object
     */
    MgtObject* objectParent;
    MgtAction(const char* name);
    virtual void toString(std::stringstream& xmlString)
    {
    }
    virtual void toString(std::stringstream& xmlString, int depth = SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts = SerializeNoOptions)
    {
    }
    virtual ~MgtAction();
    /**
     * Function to set object parent
     */
    void setObjectParent(MgtObject* parent);
  };
}
;

#endif /* CLMGTACTION_HXX_ */
