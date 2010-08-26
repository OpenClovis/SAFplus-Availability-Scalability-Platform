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
 * ModuleName  : ckpt                                                          
 * File        : clCkptPeer.h
 *******************************************************************************/

/*****************************************************************************
 * Description :                                                                
 *
 *   This file contains definitions for inter server communication
 *
 *
 *****************************************************************************/
#ifndef _CL_CKPT_PEER_H_
#define _CL_CKPT_PEER_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCkptSvr.h>
#include <clCkptDs.h>
#include <clDebugApi.h>
#include <clCkptCommon.h>
#include <xdrCkptInfoT.h>
#include <xdrCkptPrimInfoT.h>
#include <xdrCkptDPInfoT.h>
#include <xdrCkptCPInfoT.h>
#include <ipi/clRmdIpi.h>
                                                                                                                             
/**====================================**/
/**     E N U M S                      **/
/**====================================**/
                                                                                                                             
typedef enum
{
    CKPT_PEER_INFO,
    CKPT_CKPT_INFO,
    CKPT_CPLANE_INFO,
    CKPT_DPLANE_INFO,
    CKPT_SECTION_INFO,
    CKPT_ENTRY_ADD,
    CKPT_NAME_INFO,
    CKPT_SECID_INFO,
    CKPT_SEC_INFO,
    CKPT_ENTRY_DEL,
    CKPT_SEC_ADD,
    CKPT_SEC_DEL,
    CKPT_SEC_OVERWRITE,
    CKPT_PRIM_INFO,
    CKPT_RESTART_INFO,
}CkptCheckFlagT;


                                                                                                                             
/**====================================**/
/**     S T R U C T U R E S            **/
/**====================================**/
                                                                                                                             

/*
 * Cookie passed in defer sync related calls.
 */
 
typedef struct ClCkptCb
{
    ClRmdResponseContextHandleT rmdHdl;  /* defer sync related hdl */
    ClBoolT                     delFlag; /* Whether call is coming from user 
                                            or timer callback */
    ClUint32T                   cbCount; /* No. of completed calls */
    ClRcT                       retCode;
}CkptCbInfoT;



/*
 * Cookie carrying info that would help to decide when to signal the 
 * client about the completion of the call.
 */
 
typedef struct ClCkptMutx
{
    ClOsalCondIdT   condVar;
    ClRcT           *pRetCode; /* Return value of the call */
    ClUint32T       cbCount;   /* No. of completed calls */
}CkptMutxInfoT; 

                                                                                                                             
/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/

#define CL_CKPT_NANO_TO_MICRO 1000  /* 1 micro = 1000 nano */
#define CL_CKPT_MICRO_TO_MILLI 1000  
#define CL_CKPT_NANO_TO_MILLI (CL_CKPT_NANO_TO_MICRO * CL_CKPT_MICRO_TO_MILLI)

#define CL_CKPT_NODE_AVAIL   1    /* Ckpt server running on that node */
#define CL_CKPT_NODE_UNAVAIL 0    /* Ckpt server not running on that node */

#define CL_CKPT_CREDENTIAL_POSITIVE 1  /* Node can store replicas */
#define CL_CKPT_CREDENTIAL_NEGATIVE 0  /* Node cannot store replicas */

#define CKPT_SVR_TO_SVR_RMD(fcnId, inMsgHdl,outMsg, remAddr, rc)\
do\
{\
    ClRmdOptionsT     rmdOptions = {0};\
    ClUint32T         rmdFlags = CL_RMD_CALL_ASYNC;\
    ClIocAddressT     destAddr;\
    rmdOptions.timeout = CKPT_RMD_DFLT_TIMEOUT;\
    rmdOptions.retries = CKPT_RMD_DFLT_RETRIES;\
    rmdOptions.priority = 0;\
    destAddr.iocPhyAddress.nodeAddress = remAddr; \
    destAddr.iocPhyAddress.portId = CL_IOC_CKPT_PORT;\
    if(outMsg != 0)\
         rmdFlags |= CL_RMD_CALL_NEED_REPLY;\
    rmdOptions.transportHandle = 0;\
    rc = clRmdWithMsg(destAddr,fcnId,\
            (ClBufferHandleT)inMsgHdl,(ClBufferHandleT)outMsg, rmdFlags, &rmdOptions, NULL);\
}\
while(0);

#define CKPT_SVR_SVR_RMD(fcnId, inMsgHdl,outMsg, remAddr, rc)\
do\
{\
    ClRmdOptionsT     rmdOptions ={0};\
    ClUint32T         rmdFlags = 0;\
    ClIocAddressT     dAddr;\
    rmdOptions.timeout = CKPT_RMD_DFLT_TIMEOUT;\
    rmdOptions.retries = CKPT_RMD_DFLT_RETRIES;\
    rmdOptions.priority = 0;\
    dAddr.iocPhyAddress.nodeAddress = remAddr; \
    dAddr.iocPhyAddress.portId = CL_IOC_CKPT_PORT;\
    if(outMsg != 0)\
        rmdFlags = CL_RMD_CALL_NEED_REPLY;\
    rc = clRmdWithMsg(dAddr, fcnId,\
            inMsgHdl,outMsg, rmdFlags, &rmdOptions, NULL);\
}\
while(0);

#define CKPT_SVR_SVR_RMD_CALLBACK(fcnId,inMsg,remAddr,rc,callback,cookie)\
do\
{\
    ClIocAddressT           destAddr;\
    ClRmdOptionsT           rmdOptions= {0};\
    ClUint32T               rmdFlags;\
    ClRmdAsyncOptionsT      asyncOptions;\
    ClBufferHandleT  outMsg = 0;\
    rmdOptions.timeout = CKPT_RMD_DFLT_TIMEOUT;\
    rmdOptions.retries = CKPT_RMD_DFLT_RETRIES;\
    rmdOptions.priority = 0;\
    destAddr.iocPhyAddress.nodeAddress = remAddr; \
    destAddr.iocPhyAddress.portId = CL_IOC_CKPT_PORT;\
    rmdFlags = CL_RMD_CALL_ASYNC|CL_RMD_CALL_NEED_REPLY;\
    asyncOptions.pCookie = cookie ;\
    asyncOptions.fpCallback = callback;\
    clBufferCreate(&outMsg);\
    rc = clRmdWithMsg( destAddr,fcnId,inMsg,outMsg,\
                       rmdFlags,&rmdOptions,&asyncOptions);\
}\
while(0);

#define  CL_CKPT_UNREACHABEL_ERR_CHK(rc, addr)\
do\
{\
    if( CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || \
            CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE )\
    {\
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,\
                "Destination address [%d] is not reachable rc [0x %x], continuing"\
                "to update other replicas...", addr, rc); \
        rc = CL_OK ;\
    }\
    else if( CL_OK != rc )\
    {\
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, \
           "Failed to update replica address [%d] for ckpt[%.*s] with rc[0x %x]",\
                addr, pCkpt->ckptName.length, \
                pCkpt->ckptName.value, rc);\
    }\
}while(0)

#ifdef __cplusplus
}
#endif
#endif /* _CL_CKPT_PEER_H_ */
