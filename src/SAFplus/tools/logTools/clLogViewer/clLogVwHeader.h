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
#ifndef _CL_LOGVWHEADER_H
#define _CL_LOGVWHEADER_H

#include <clCommon.h>
#include <clCommonErrors.h>

#include "clLogVwType.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    ClLogVwFlagT endianess;
    ClLogVwSeverityT severity;
    ClLogVwStreamIdT streamId;
    ClLogVwCmpIdT compId;
    ClLogVwServiceIdT serviceId;
    ClLogVwTimeStampT timeStamp;
    ClLogVwMsgIdT msgId;
}ClLogVwRecHeaderT;

typedef struct
{
   ClCharT      *timeStampString;
   
   ClUint16T    endianessIndex;
   ClUint16T    severityIndex ;
   ClUint16T    streamIdIndex ;
   ClUint16T    compIdIndex;
   ClUint16T    serviceIdIndex;
   ClUint16T    timeStampIndex;
   ClUint16T    msgIdIndex;

   ClBoolT      selColumnsFlag;
   ClBoolT      isSeverityOn;
   ClBoolT      isStreamIdOn;
   ClBoolT      isCompIdOn;
   ClBoolT      isServiceIdOn;
   ClBoolT      isTimeStampOn;
   ClBoolT      isMsgIdOn;

   FILE         *outPtr;
   
}ClLogVwHeaderT;


extern ClRcT clLogVwInitHeader(void);

extern ClRcT clLogVwInitRecHeader(ClLogVwByteT *bytes, ClLogVwRecHeaderT *recHeader);

extern ClRcT clLogVwDispLogRecHeader(ClLogVwRecHeaderT *recHeader);

extern ClRcT clLogVwCleanUpHeaderData(void);

extern ClRcT clLogVwDispCompName(ClLogVwCmpIdT compId);

extern ClRcT clLogVwDispStreamName(ClLogVwStreamIdT streamId);

extern ClRcT clLogVwDispSeverityName(ClLogVwSeverityT severity);

extern ClRcT clLogVwGetSeverityIdFromName(ClCharT *name, ClLogVwSeverityT *sevId);

extern ClRcT clLogVwSetHeaderColumn(ClCharT *columnInfoString);

extern ClRcT clLogVwSelectColumns(ClCharT *columnInfoString);

extern ClRcT clLogVwGetSelColumnsFlag(ClBoolT *flag);

extern ClRcT clLogVwResetRecCounter(void);

extern ClRcT clLogVwSetHeaderOutPtr(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif 
