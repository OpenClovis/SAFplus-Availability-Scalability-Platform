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

#include "clMgtTransaction.hxx"

#ifdef __cplusplus
extern "C" {
#endif
#include <clDebugApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

ClTransaction::ClTransaction() {
}

ClTransaction::~ClTransaction() {
	// TODO Auto-generated destructor stub
}

void ClTransaction::add(void *pBuffer, ClUint64T buffLen)
{
    ClTransactionDataT *transaction = new (ClTransactionDataT);
    transaction->len = buffLen;
    transaction->data = new char[buffLen];
    memcpy(transaction->data, pBuffer, buffLen);
    mData.push_back(transaction);
}

void* ClTransaction::get(ClUint32T index)
{
    if (index >= mData.size())
    {
        return NULL;
    }

    ClTransactionDataT *transaction = mData[index];
    return transaction->data;
}

void ClTransaction::remove(ClUint32T index)
{
    ClTransactionDataT *transaction = mData[index];
    delete(transaction->data);
    delete(transaction);
    mData.erase(mData.begin() + index);
}

ClUint32T ClTransaction::getSize()
{
    return mData.size();
}

void ClTransaction::clean()
{
    ClInt32T i;

    for(i = mData.size() - 1; i>=0; i--)
    {
        remove(i);
    }
}
