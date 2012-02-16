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
#ifndef _CL_LOGVWFILTER_H
#define _CL_LOGVWFILTER_H

#include <clCommon.h>
#include <clCommonErrors.h>

#include "clLogVwHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    ClInt16T   fldIndex;
    ClInt64T    fldVal;
    ClCharT     *fldName;
    ClInt16T   compareType;
    ClBoolT     filterFlag;

}ClLogVwFilterInfoT;


extern ClRcT clLogVwGetFilterFlag(ClBoolT *flag);

extern ClRcT clLogVwCleanUpFilterInfo(void);

extern ClRcT clLogVwInitFilterInfo(const ClCharT *filterArg);

extern ClRcT clLogVwApplyFilter(ClLogVwRecHeaderT *recHeader, ClBoolT *result);

extern ClRcT clLogVwIsFiltered(ClLogVwRecHeaderT *recHeader, ClBoolT *result);

extern ClRcT clLogVwCheckRange(ClInt64T value, ClBoolT *result); 

extern ClRcT clLogVwSetFilterValue(ClCharT *name);

extern ClRcT clLogVwSetFldValueFromName(void);

extern ClRcT clLogVwGetTimeStampFromName(ClCharT *name, ClInt64T *timeStamp);


#ifdef __cplusplus
}
#endif

#endif
