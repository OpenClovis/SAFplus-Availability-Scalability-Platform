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

#include "clTransaction.hxx"

#ifdef __cplusplus
extern "C" {
#endif
#include <clDebugApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

namespace SAFplus
{

Transaction::Transaction()
{
    mHandle = Handle::create();
}

Transaction::~Transaction()
{

}

void Transaction::commit()
{
    for(unsigned int i = 0; i < mOperations.size(); i++)
    {
        mOperations[i]->commit();
        delete mOperations[i];
    }
    mOperations.clear();
}

void Transaction::abort()
{
    for(unsigned int i = 0; i < mOperations.size(); i++)
    {
        mOperations[i]->abort();
        delete mOperations[i];
    }
    mOperations.clear();
}

ClRcT Transaction::addOperation(TransactionOperation *operation)
{
    ClRcT rc = CL_OK;

    if (!operation)
    {
        rc = CL_ERR_NULL_POINTER;
        return rc;
    }

    mOperations.push_back(operation);

    return rc;
}

Transaction NO_TXN;

};
