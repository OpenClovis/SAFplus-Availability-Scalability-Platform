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
#ifndef __CL_IOC_GENERAL_H__
#define __CL_IOC_GENERAL_H__

#include <clIocIpi.h>
#include <clTimerApi.h>
#include <clIocApi.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clCommon.h>
#include <clIocTransportApi.h>


# define IOC_MSG_QUEUED                       (CL_IOC_ERR_MAX+1)

# define IOC_MORE_FRAG    (1<<0)
# define IOC_LAST_FRAG    (1<<1)

typedef struct
{
    ClUint32T reassemblyQsize;
} ClIocUserObjectT;


typedef struct
{
    ClUint32T fragId;
    ClIocPhysicalAddressT sendAddr;
    ClIocPhysicalAddressT destAddr;
} ClIocReassemblyKeyT;


typedef struct
{
    ClIocReassemblyKeyT reassemblyKey;
    ClTimerHandleT timerID; /* associated timer ID */
    ClCntHandleT fragList;
    ClUint32T totalLength;
    ClUint32T expectedLength;
    ClUint32T lastFragSeen;
    ClUint32T expectedFragOffset;
    ClUint32T mayDiscard;
#ifdef CL_IOC_COMPRESSION
    ClTimeT   pktSendTime;
#endif
} ClIocReassemblyNodeT;


typedef struct
{
    ClIocAddressT address;
    void *pAddressList;
    ClUint32T numEntries;
    ClIocUserOpTypeT mapping;
} ClIocUserEntriesT;


typedef struct
{
    ClIocNodeAddressT destAddr;
    ClUint16T prefixLen;
    ClIocNodeAddressT nextHop;
    ClUint16T metrics;
    ClUint8T routeFlags;
    ClUint8T version;
    ClUint32T linkIndex;
    ClUint8T status;
    ClUint8T xportIndex;
    ClUint8T entryType;
    ClCharT xportName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
} ClIocRouteTableShowT;


typedef enum
{   
    CL_IOC_TL_FROM_PEERS,
    CL_IOC_TL_FROM_USER
} ClIocTLSourceT;


typedef struct 
{
    ClIocNodeAddressT iocAddress;   /* ioc address */
    ClUint8T linkName[CL_IOC_MAX_XPORT_NAME_LENGTH];     /* Name of the link */
    ClUint8T transportAddress[CL_IOC_MAX_XPORT_NAME_LENGTH]; /* transport address */
    ClUint32T addressSize;  /* size of transport address in bytes */
    ClUint8T linkType;      /* link id for the transport */
    ClUint8T status;
    ClUint8T entryType;
    ClUint8T retryCount;
} ClIocArpTableEntryT;

typedef struct
{
    ClIocArpTableEntryT arpEntry;
    ClCharT xportName[CL_IOC_MAX_XPORT_NAME_LENGTH];
} ClIocArpTableShowT;



#endif
