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
#ifndef _CL_LOGVWPARSER_H
#define _CL_LOGVWPARSER_H

#include <clCommon.h>
#include <clCommonErrors.h>

#include "clLogVwHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

    
typedef struct
{
    ClCharT *msgIdPtr;
    ClCharT *stringPtr;
}ClLogVwTlvHashMapT;

typedef struct 
{
    ClLogVwRecHeaderT        *recHeader;
    ClLogVwTlvHashMapT       *tlvHash;
    ClBoolT             isDataColOn;
    ClUint16T           headerSize;
    ClLogVwMaxRecSizeT  recordSize;

    FILE                *outPtr;

    struct hsearch_data *tlvHashTb;

    ClBoolT             isTlvFilePresent;
}ClLogVwParserT;

extern ClRcT clLogVwParseOneRecord(ClLogVwByteT *bytes);

extern ClRcT clLogVwInitParserData(void);

extern ClRcT clLogVwCleanUpParserData(void);

extern ClRcT clLogVwDestroyTlvMap(void);

extern ClRcT clLogVwCreateTlvMap(ClCharT *tlvFileName);

extern ClRcT clLogVwSelDataColumn(ClBoolT flag);

extern  ClRcT clLogVwSetParserHeaderSize(ClUint16T size);

extern  ClRcT clLogVwSetParserRecSize(ClLogVwMaxRecSizeT size);

extern ClRcT clLogVwSetParserOutPtr(FILE *fp);
#ifdef __cplusplus
}
#endif

#endif
