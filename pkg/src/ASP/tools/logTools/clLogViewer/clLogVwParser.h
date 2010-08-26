/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
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
