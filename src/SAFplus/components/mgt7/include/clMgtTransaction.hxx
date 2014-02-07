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

#ifndef CLMGTTRANSACTION_HXX_
#define CLMGTTRANSACTION_HXX_

#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <clCommon.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

typedef struct ClTransactionData
{
	ClUint64T len;
    ClCharT *data;
} ClTransactionDataT;

using namespace std;

class ClTransaction {
protected:
    std::vector<ClTransactionDataT*> mData;
public:
    ClTransaction();
    virtual ~ClTransaction();
    void add(void *pBuffer, ClUint64T buffLen);
    void* get(ClUint32T index);
    void remove(ClUint32T index);
    ClUint32T getSize();
    void clean();
};

#endif /* CLMGTTRANSACTION_HXX_ */
