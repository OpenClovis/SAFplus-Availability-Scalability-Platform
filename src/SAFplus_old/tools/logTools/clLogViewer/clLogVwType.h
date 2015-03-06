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
#ifndef _CL_LOGVWTYPE_H
#define _CL_LOGVWTYPE_H

#include <clCommon.h>
#include <clLogApi.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef     ClUint8T        ClLogVwFlagT;
typedef     ClLogSeverityT  ClLogVwSeverityT;
typedef     ClUint16T       ClLogVwStreamIdT;
typedef     ClUint32T       ClLogVwCmpIdT;
typedef     ClUint16T       ClLogVwServiceIdT;
typedef     ClInt64T        ClLogVwTimeStampT;
typedef     ClUint16T       ClLogVwMsgIdT;
typedef     ClCharT         ClLogVwByteT;

typedef     ClUint32T       ClLogVwRecNumT;
typedef     ClUint32T       ClLogVwMaxFileRotatedT;
typedef     ClUint32T       ClLogVwMaxRecSizeT;

#ifdef __cplusplus
}
#endif

#endif
