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
 * ModuleName  : name
 * File        : clNameCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains functions and datastructures common
 * to both Name Service Client and Name Service Server.                   
 *
 *
 *****************************************************************************/
                                                                                                                             

                                                                                                                             
#ifndef _CL_NAME_COMMON_H_
#define _CL_NAME_COMMON_H_
                                                                                                                             
#ifdef __cplusplus
extern "C" {
#endif

#include "clNameApi.h"
#include "clRmdApi.h"

#define CL_NS_DFLT_TIMEOUT 5000
#define CL_NS_DFLT_RETRIES  3
#define CL_NS_VERSION_NO 0x0100

/* Name Service RMD FN# */
                                                                                                                             
#define CL_NS_REGISTER                 CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1)
#define CL_NS_COMPONENT_DEREGISTER     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2)
#define CL_NS_SERVICE_DEREGISTER       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,3)
#define CL_NS_CONTEXT_CREATE           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,4)
#define CL_NS_CONTEXT_DELETE           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,5)
#define CL_NS_QUERY                    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,6)
#define CL_NS_NACK                     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,7)
#define CL_NS_DB_ENTRIES_PACK          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,8)

#define NAME_FUNC_ID(n)                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,n)

/* Macro to send RMD */
#define CL_NS_CALL_RMD(addr, fcnId, pInBuf, pOpBuf, rc)\
do  \
{ \
  ClRmdOptionsT      rmdOptions = CL_RMD_DEFAULT_OPTIONS;\
  ClIocAddressT      address;\
  ClUint32T          rmdFlags = pOpBuf ? CL_RMD_CALL_NEED_REPLY :0;\
  address.iocPhyAddress.nodeAddress = addr; \
  address.iocPhyAddress.portId      = CL_IOC_NAME_PORT; \
  rmdOptions.timeout = CL_NS_DFLT_TIMEOUT; \
  rmdOptions.retries = CL_NS_DFLT_RETRIES; \
  rmdOptions.priority = 2; \
  rmdFlags = rmdFlags | CL_RMD_CALL_ATMOST_ONCE; \
  rc = clRmdWithMsg(address, \
                    fcnId, (ClBufferHandleT)pInBuf,\
                    (ClBufferHandleT)pOpBuf, rmdFlags, &rmdOptions, NULL);\
} \
while(0)

/* Source og the request */
typedef enum
{
    CL_NS_MASTER,    /* Source is master(NS/M) */
    CL_NS_LOCAL,     /* Source is peer(NS/L) */
    CL_NS_CLIENT,    /* Source is client library */
}ClNameSvcSourceT;
                           
/* Type of queries that can be carried out */                                
typedef enum
{
    CL_NS_QUERY_OBJREF,          /* Query the object reference */
    CL_NS_QUERY_MAPPING,         /* Query object mapping */
    CL_NS_QUERY_ALL_MAPPINGS,    /* Query all the mappings associated with
                                    a service */
    CL_NS_QUERY_ATTRIBUTE,       /* Attribute level query */
    CL_NS_LIST_NAMES,            /* List all the service names in a given
                                    context */
    CL_NS_LIST_BINDINGS,         /* List all the bindings in a given context */
}ClNameSvcOpsT;

                                                                                                                            
/* Structure containing the details of request to be serviced
  by NS Server Side module */ 
typedef struct 
{
    ClUint32T            version;      /* Version */
    ClUint32T            dsIdCnt;
    ClUint32T            dsId;
    ClNameT              name;         /* Service name */
    ClUint64T            objReference; /* Object Reference */
    ClNameSvcSourceT     source;       /* Source of the packet */
    ClUint32T            compId;       /* Id of the component providing the service */
    ClNameSvcPriorityT   priority;     /* Priority of the component providing the service */
    ClUint32T            contextId;    /* Context Id in which to operate */
    ClNameSvcOpsT        op;               /* Operation to be performed */
    ClNameSvcContextT    contextType;      /* GLOBAL or NODE LOCAL context */
    ClUint32T            contextMapCookie; /* cookie */
    ClNameSvcAttrSearchT attrQuery;    /* Attribute level query related info */   
    ClUint32T            attrCount;    /* No. of attributes */
    ClNameSvcAttrEntryT  attr[1];      /* List of attributes */
} ClNameSvcInfoT;
                                                                                                                            
typedef ClNameSvcInfoT* ClNameSvcInfoPtrT;

#ifdef __cplusplus
}
#endif
                                                                                                                            
                                                                                                                            
#endif /* _CL_NAME_COMMON_H_ */

