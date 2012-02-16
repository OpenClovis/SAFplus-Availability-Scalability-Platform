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
#ifndef _CL_LOGVWMAPPING_H
#define _CL_LOGVWMAPPING_H

#include <clCommon.h>
#include <clCommonErrors.h>

#include "clLogVwHeader.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CL_LOGVW_COMP_NAME_MAX_LEN    100
#define CL_LOGVW_STREAM_NAME_MAX_LEN  100
#define CL_LOGVW_MAX_MAP_COUNT        500

#define CL_LOGVW_NEXT_ENTRY_END       0x0

#define CL_LOGVW_COMP_HASH_TB_SIZE   1000

#define CL_LOGVW_STREAM_HASH_TB_SIZE 1000

typedef struct
{
    ClCharT     *compNamePtr;
    ClCharT     *compIdPtr;
    ClCharT     *streamNamePtr;
    ClCharT     *streamIdPtr;
    ClCharT     *compNameStartPtr;
    ClCharT     *compIdStartPtr;
    ClCharT     *streamNameStartPtr;
    ClCharT     *streamIdStartPtr;
    ClUint16T   nextCompIdIndex;
    ClUint16T   nextStreamIdIndex;
    ClInt32T    mFd;
    ClCharT endianess;
    struct hsearch_data *compHashTb;
    struct hsearch_data *streamHashTb;

}ClLogVwIdToNameMapT;


extern ClRcT clLogVwInitNameMap(ClCharT *logicalFileName);

extern ClRcT clLogVwGetCompNameFromId(ClLogVwCmpIdT compId, ClCharT **compName);

extern ClRcT clLogVwGetStreamNameFromId(ClLogVwStreamIdT streamId, 
                                        ClCharT **streamName);

extern ClRcT clLogVwAddEntryToCompMap(void);

extern ClRcT clLogVwAddEntryToStreamMap(void);

extern ClRcT clLogVwCleanUpNameMap(void); 

extern ClRcT clLogVwGetStreamIdFromName(ClCharT *name, ClLogVwStreamIdT *streamId);

extern ClRcT clLogVwGetCompIdFromName(ClCharT *name, ClCompIdT *cmpId);

#ifdef __cplusplus
}
#endif


#endif
