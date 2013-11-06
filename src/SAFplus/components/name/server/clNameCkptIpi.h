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
 * ModuleName  : name
 * File        : clNameCkptIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains Name Service checkpointing related data
 * structures and defines
 *
 *
 *****************************************************************************/
                                                                                                                             


#ifndef _CL_NAME_CKPT_IPI_H_
#define _CL_NAME_CKPT_IPI_H_ 
                                                                                                                             
#ifdef __cplusplus
extern "C" {
#endif
#include "clCommon.h"
#include "clEoApi.h"
#include "clCkptApi.h"
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clLogUtilApi.h>
#include "clCkptExtApi.h"
#include "clNameIpi.h"
#include "xdrClNameSvcInfoIDLT.h"
                                                                                                                             
#define CL_NS_CKPT_COUNT            1    /* No. of Check Points intended - CKPT Names */
#define CL_NS_CKPT_NAME             "Cl_Name_Point"
#define CL_NS_CKPT_DATASET_COUNT    2
                                                                                                                             
/* Data Set IDs */
#define CL_NS_CKPT_GLOBAL     0x1
#define CL_NS_CKPT_LOCAL      0x2
                                                                                                                             
#define CL_NS_FROM_SERVER     0x3

#define CL_NS_MASTER 0
#define CL_NS_LOCAL  1

#define CL_NAME_DEBUG_TRACE(arg) \
    CL_DEBUG_PRINT(CL_LOG_SEV_TRACE, arg);

#define CL_NAME_CONTEXT_GBL_DSID   1    
#define CL_NS_PER_CTX_DSID         1    
#define CL_NS_RESERVED_DSID        2
typedef enum
{
    CL_NAME_SVC_CTX_INFO     = 1,
    CL_NAME_SVC_ENTRY        = 2,
    CL_NAME_SVC_BINDING_DATA = 3,
    CL_NAME_SVC_COMP_INFO    = 4
      
}ClNsEntryTypeT;    

typedef ClUint32T      ClNameSvcCompInfoT ; 

typedef struct
{
    ClUint32T   key;
    ClBoolT     isDelete;
}ClNSCkptT;    

typedef struct
{
    ClNsEntryTypeT     type;
    ClNameSvcInfoIDLT  nsInfo;
}ClNsEntryPackT;

extern ClRcT
clNameSvcCkptInit(void);
extern ClRcT
clNameSvcCkptCreate(void);
extern ClRcT
clNameCkptCtxInfoWrite(void);
extern ClRcT
clNameSvcPerCtxDataSetCreate(ClUint32T  contexId,
                             ClUint32T  dsIdCnt);

extern ClRcT
clNameCkptCtxAllDSDelete(ClUint32T  contexId,
                         ClUint32T  dsIdCnt,
                         ClUint8T   *freeDsIdMap,
                         ClUint32T  freeMapSize);
extern ClRcT
clNameSvcPerCtxDataSetIdGet(ClNameSvcContextInfoT *pCtxData,
                            ClUint32T             *pDsIdCnt);
extern ClRcT
clNameSvcPerCtxDataSetIdPut(ClUint32T contextId, ClUint32T dsId);

extern ClRcT
clNameSvcPerCtxInfoWrite(ClUint32T              contexId,
                         ClNameSvcContextInfoT  *pCtxData); 
extern ClRcT
clNameSvcBindingDataWrite(ClUint32T              contexId,
                          ClNameSvcContextInfoT  *pCtxData,
                          ClNameSvcBindingT      *pBindData);
extern ClRcT
clNameSvcBindingDetailsWrite(ClUint32T                 contexId,
                             ClNameSvcContextInfoT     *pCtxData,
                             ClNameSvcBindingT         *pBindData,
                             ClNameSvcBindingDetailsT  *pBindDetail);
extern ClRcT
clNameSvcCompInfoWrite(ClUint32T                 contexId,
                       ClNameSvcContextInfoT     *pCtxData,
                       ClNameSvcBindingT         *pBindData,
                       ClNameSvcBindingDetailsT  *pBindDetail);

extern ClRcT
clNameSvcEntrySerializer(ClUint32T  dsId,
                         ClAddrT    *pBuffer,
                         ClUint32T  *pSize,
                         ClPtrT cookie);
extern ClRcT
clNameContextCkptNamePack(ClBufferHandleT  inMsg);
extern ClRcT
clNameNSTableWalk(ClCntKeyHandleT   key,
                  ClCntDataHandleT  data,
                  ClCntArgHandleT   arg,
                  ClUint32T         dataSize);
extern ClRcT
clNameContextCkptSerializer(ClUint32T  dsId,
                            ClAddrT    *pBuffer,
                            ClUint32T  *pSize,
                            ClPtrT cookie);
extern ClRcT
clNameContextCkptDeserializer(ClUint32T   dsId,
                              ClAddrT     pBuffer,
                              ClUint32T   size,
                              ClPtrT cookie);
extern ClRcT
clNameContextCkptNameUnpack(ClBufferHandleT  inMsg);
extern ClRcT
clNameSvcCtxRecreate(ClUint32T  key);
extern ClRcT
clNameSvcEntryRecreate(ClUint32T              key,
                       ClNameSvcContextInfoT  *pCtxData);
extern ClRcT
clNameSvcPerCtxInfoUpdate(ClNameSvcContextInfoT  *pCtxData,
                          ClNameSvcInfoIDLT      *pData);
extern ClRcT
clNameSvcBindingEntryRecreate(ClNameSvcContextInfoT  *pCtxData,
                              ClNsEntryPackT         nsEntryInfo);
extern ClRcT
clNameSvcCompInfoAdd(ClNameSvcContextInfoT  *pCtxData,
                     ClNameSvcInfoIDLT      *pData);
extern ClRcT
clNameSvcBindingDetailEntryCreate(ClNameSvcContextInfoT *pCtxData,
                                  ClNameSvcInfoIDLT     *pData);
extern ClRcT
clNameSvcBindingEntryCreate(ClNameSvcContextInfoT  *pCtxData,
                            ClNameSvcInfoIDLT      *pData);
extern ClRcT
clNameSvcEntryDeserializer(ClUint32T  dsId,
                           ClAddrT    pBuffer,
                           ClUint32T  size,
                           ClPtrT cookie);
extern ClRcT
clNameSvcCkptRead(void);
extern ClRcT
clNameSvcPerCtxCkptCreate(ClUint32T  key,
                          ClBoolT    isDelete);
extern ClRcT
clNameSvcDataSetDelete(ClUint32T  contexId,
                       ClUint32T  dsId);
extern ClRcT
clNameSvcPerCtxSerializer(ClUint32T  dsId,
                          ClAddrT    *pBuffer,
                          ClUint32T  *pSize,
                          ClPtrT cookie);
extern ClRcT
clNameSvcPerCtxDeserializer(ClUint32T  dsId,
                            ClAddrT    pBuffer,
                            ClUint32T  size,
                            ClPtrT cookie);
extern ClRcT
clNameSvcCkptFinalize(void);
#ifdef __cplusplus
}
#endif
                                                                                                                             
#endif /* _CL_NAME_CKPT_IPI_H_ */

