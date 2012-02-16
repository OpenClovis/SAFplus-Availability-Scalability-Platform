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
/*******************************************************************************
 * ModuleName  : gms
 * File        : saClm.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * Copyright (c) 2002-2005 MontaVista Software, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SA_CLM_H_
#define _SA_CLM_H_

# ifdef __cplusplus
extern "C" {
# endif

#include "saAis.h"

typedef SaUint64T SaClmHandleT;

typedef SaUint32T SaClmNodeIdT;

#define SA_CLM_LOCAL_NODE_ID 0xffffffff

#define SA_CLM_MAX_ADDRESS_LENGTH 64

typedef enum {
		SA_CLM_AF_INET = 1,
		SA_CLM_AF_INET6 = 2
} SaClmNodeAddressFamilyT;

typedef struct {
	SaClmNodeAddressFamilyT family;
	SaUint16T length;
	SaUint8T value[SA_CLM_MAX_ADDRESS_LENGTH];
} SaClmNodeAddressT;

typedef struct {
	SaClmNodeIdT nodeId;
	SaClmNodeAddressT nodeAddress;
	SaNameT nodeName;
	SaBoolT member;
	SaTimeT bootTimestamp __attribute__((__aligned__(8)));
	SaUint64T initialViewNumber __attribute__((__aligned__(8)));
} SaClmClusterNodeT __attribute__((__aligned__(8)));

typedef enum {
	SA_CLM_NODE_NO_CHANGE = 1,
	SA_CLM_NODE_JOINED = 2,
	SA_CLM_NODE_LEFT = 3,
	SA_CLM_NODE_RECONFIGURED = 4
} SaClmClusterChangesT;

typedef struct {
	SaClmClusterNodeT clusterNode;
	SaClmClusterChangesT clusterChange;
} SaClmClusterNotificationT;

typedef struct {
	SaUint64T viewNumber;
	SaUint32T numberOfItems;
	SaClmClusterNotificationT *notification;
} SaClmClusterNotificationBufferT;

typedef void (*SaClmClusterNodeGetCallbackT)(
	SaInvocationT invocation,
	const SaClmClusterNodeT *clusterNode,
	SaAisErrorT error);

typedef void (*SaClmClusterTrackCallbackT) (
	const SaClmClusterNotificationBufferT *notificationBuffer,
	SaUint32T numberOfMembers,
	SaAisErrorT error);

typedef struct {
	SaClmClusterNodeGetCallbackT saClmClusterNodeGetCallback;
	SaClmClusterTrackCallbackT saClmClusterTrackCallback;
} SaClmCallbacksT;


SaAisErrorT
saClmInitialize (
	SaClmHandleT *clmHandle,
	const SaClmCallbacksT *clmCallbacks,
	SaVersionT *version);


SaAisErrorT
saClmSelectionObjectGet (
		SaClmHandleT clmHandle,
		SaSelectionObjectT *selectionObject);

SaAisErrorT
saClmDispatch (
	SaClmHandleT clmHandle,
	SaDispatchFlagsT dispatchFlags);

SaAisErrorT
saClmFinalize (
	SaClmHandleT clmHandle);

SaAisErrorT
saClmClusterTrack (
	SaClmHandleT clmHandle,
	SaUint8T trackFlags,
	SaClmClusterNotificationBufferT *notificationBuffer);

SaAisErrorT
saClmClusterTrackStop (
	SaClmHandleT clmHandle);

SaAisErrorT
saClmClusterNodeGet (
	SaClmHandleT clmHandle,
	SaClmNodeIdT nodeId,
	SaTimeT timeout,
	SaClmClusterNodeT *clusterNode);

SaAisErrorT
saClmClusterNodeGetAsync (
	SaClmHandleT clmHandle,
	SaInvocationT invocation,
	SaClmNodeIdT nodeId);

#ifdef __cplusplus
}
#endif

#endif /* _SA_CLM_H_ */


