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
 * File        : clCkptExt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *   This file contains Checkpoint client related data structures
 *
 *
 *****************************************************************************/
#ifndef _CL_CKPT_EXT_H_
#define _CL_CKPT_EXT_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Type of the Sever Primary/Secondary */
typedef enum
{
    CKPT_SVR_TYPE_PRIMARY =0,
    CKPT_SVR_TYPE_SECONDARY,
    CKPT_SVR_TYPE_MAX
}ckptSvrType_e;
            
typedef struct ckptClnCb
{
    ClOsalMutexIdT         ckptSem;    /* Semaphore guarding the data structures */
    ClDBHandleT            ownDbHdl;   /* Db handle for checkpoint db */
    ClCntHandleT           ckptList;    /* checkpoints List */
} CkptClnCbT;

/* This structure represents Section related information */
typedef struct ckptDataSet
{
     ClUint32T            dsId;         /* identifier of dataset */
     ClAddrT              data;         /* Pointer to the data of this data set*/
     ClUint32T            size;         /* Size of the data set */
     ClUint32T            grpId;        /*group id */
     ClUint32T            order;        /*order which needs to be stored*/
     ClCkptDataSetCallbackT *pDataSetCallbackTable;
     ClUint32T             numDataSetTableEntries;
     ClCkptDataSetCallbackT *pElementCallbackTable;
     ClUint32T             numElementTableEntries;
}CkptDataSetT;

typedef struct ckptSvrInfo
{
    ClIocNodeAddressT   nodeAddr;   /* Node address of the server */
    ckptSvrType_e       svrType;   /* Type of the Sever Primary/Secondary */
}CkptSvrInfoT;
                                                                                                                             
/* Check point Object */
typedef struct clnCkpt
{
    ClNameT         *pCkptName;       /* Name of the checkpoint */
    ClDBHandleT     usrDbHdl;         /* Db handle for user checkpoint */
    ClCntHandleT    dataSetList;      /* Information about sections */
}ClnCkptT;

                                                                                                                             
typedef struct ckptKey
{
    ClUint32T     dsId;
    ClUint32T     keyLen;
    ClCharT       elemKey[1];
}CkptElemKeyT;


#ifdef __cplusplus
}
#endif
#endif  /* _CL_CKPT_EXT_H_*/


