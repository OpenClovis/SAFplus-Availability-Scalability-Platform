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
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorSync.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the definitions for COR synchronization
 *
 *
 *****************************************************************************/


#ifndef _CL_COR_SYNC_H_
#define _CL_COR_SYNC_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include "clDebugApi.h"
#include "clIocApi.h"

/* Default buffer size.*/
#define COR_DM_DATA_BLOCK_SIZE          64*1024
/* @todo : Use of message buffers is required for Object tree
	since we do not know for sure how much size of objTree
	is going to be */
#define COR_OBJTREE_BUF_SZ              20*64*1024   /* 1 MB */
#define COR_LIST_BLOCK_SIZE             1*1024


/* MACROS */
#define MAX_COR_COMM_RETRIES	5
#define MAX_COR_COMM_TIMEOUT	1000
#define CL_COR_SYNC_UP_RETRY_TIME 100

#define clCorServerToServerVersionValidate(version, rc) \
		do\
		{ \
			if((rc = clVersionVerify(&gCorServerToServerVersionDb, (&version) )) != CL_OK) \
			{ \
			    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to validate the server version .")); \
			} \
		}while(0) 

typedef enum {
	COR_INSTANCE_INVALID,
	COR_INSTANCE_MASTER,
	COR_INSTANCE_SLAVE
}ClCorInstanceTypeT;

/* Type of information that is synced up */
typedef enum {
       COR_SYNC_HELLO_REQUEST,
       COR_SYNC_HELLO_REQUEST_REPLY,
       COR_SYNC_CONFIG_DATA_REQUEST,
       COR_SYNC_CONFIG_DATA_REQUEST_REPLY,
       COR_SYNC_DM_DATA_REQUEST,
       COR_SYNC_DM_DATA_REQUEST_REPLY,
       COR_SYNC_COR_LIST_REQUEST,
       COR_SYNC_COR_LIST_REQUEST_REPLY,
       COR_SYNC_MOTREE_REQUEST,
       COR_SYNC_MOTREE_REQUEST_REPLY,
       COR_SYNC_OBJECT_TREE_REQUEST,
       COR_SYNC_OBJECT_TREE_REQUEST_REPLY,
       COR_SYNC_RM_DATA_REQUEST,
       COR_SYNC_RM_DATA_REQUEST_REPLY,
       COR_SYNC_NI_TABLE_REQUEST,
       COR_SYNC_NI_TABLE_REQUEST_REPLY,
       COR_SYNC_SLOT_TO_MOID_MAP_TABLE,
       COR_SYNC_SLOT_TO_MOID_MAP_TABLE_REPLY,
       COR_SYNC_DATA_END
} corSyncEvts_e;

typedef struct corSyncPkt {
	ClVersionT			version;
	corSyncEvts_e           opcode;
	ClUint16T 		cksum;
	ClIocNodeAddressT        src; 
	ClUint32T 		dlen;
	ClUint8T		*data;
} corSyncPkt_t;

/* Macro definitions. */

#define CONSTRUCT_COR_COMM_PKT(pkt, opCode)\
	do\
		{\
		CL_COR_VERSION_SET(pkt->version);  \
		pkt->src = clIocLocalAddressGet(); \
		pkt->opcode = opCode;\
		/*  pkt->cksum = (ClUint16T)pkt.src + (ClUint16T)pkt.dest + (ClUint16T)opCode;*/\
		pkt->dlen = 0;\
		}\
	while(0)


/*@todo : Check how we can get the peer address. It may not be obtained by clCpmMasterAddressGet*/
#define SEND_PKT_TO_MASTER(inMessageHandle, clCorXdrMarshallFP, outMessageHandle, pkt, pktLen, rc)\
do\
{\
   			ClRmdOptionsT    rmdOptions = CL_RMD_DEFAULT_OPTIONS;\
   			ClUint32T          rmdFlags = CL_RMD_CALL_NEED_REPLY ;\
			ClIocAddressT destAddr;\
			 rc = clBufferCreate(&inMessageHandle);\
			 if(CL_OK != rc)\
			 	{\
			 	clHeapFree(pkt);\
			 	return rc;\
			 	}\
			 rc = clBufferCreate(&outMessageHandle);\
			 if(CL_OK != rc)\
			 	{\
                clBufferDelete(&inMessageHandle); \
			 	clHeapFree(pkt);\
			 	return rc;\
			 	}\
			 rmdOptions.timeout = CL_RMD_TIMEOUT_FOREVER;\
 			 rmdOptions.retries = 0;\
			 rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;\
			 CL_CPM_MASTER_ADDRESS_GET(&destAddr.iocPhyAddress.nodeAddress);\
			 if(CL_OK != rc)\
			 	{\
			 	clHeapFree(pkt);\
                clBufferDelete(&inMessageHandle); \
                clBufferDelete(&outMessageHandle); \
			 	return rc;\
			 	}\
			destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;\
			rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)(pkt, inMessageHandle, 0); \
			if(CL_OK != rc)\
			 	{\
			 	clHeapFree(pkt);\
                clBufferDelete(&inMessageHandle); \
                clBufferDelete(&outMessageHandle); \
			 	return rc;\
			 	}\
			rc = clRmdWithMsg(destAddr, COR_EO_SYNCH_RQST_OP, inMessageHandle,\
                        						outMessageHandle, rmdFlags, &rmdOptions, NULL);\
}\
while(0)

/* Function prototype for COR slave synchronization */
ClRcT synchronizeWithMaster();


# ifdef __cplusplus
}
# endif

#endif							/* _CL_COR_SYNC_H_ */
