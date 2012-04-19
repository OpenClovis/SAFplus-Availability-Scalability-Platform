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
 * ModuleName  : amf
 * File        : clAmsCkpt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains AMS definitions relating to AMS checkpointing.
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_CKPT_H_
#define _CL_AMS_CKPT_H_ 

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/
#include <clCkptApi.h>
#include <clCkptExtApi.h>
#include <clAms.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>
#include <clAmsXdrHeaderFiles.h>

#define CL_AMS_CKPT_DB_NAME "AMS_CONFIG"

#define CL_AMS_CKPT_CTRL_DB_NAME "CKPTDB_AMS_CONFIG"

#define CL_AMS_CKPT_CONFIG_DS_ID 0x1

#define CL_AMS_CKPT_CONFIG_GRP_ID 0x0

#define CL_AMS_CKPT_CONFIG_ORDER_ID 0x0

typedef enum ClAmsCkptOperation
{
    CL_AMS_CKPT_OPERATION_START_GROUP = 1,
    CL_AMS_CKPT_OPERATION_END_GROUP,
    CL_AMS_CKPT_OPERATION_SET_CONFIG,
    CL_AMS_CKPT_OPERATION_SET_STATUS,
    CL_AMS_CKPT_OPERATION_SET_LIST,
    CL_AMS_CKPT_OPERATION_SET_TIMER,
    CL_AMS_CKPT_OPERATION_SET_PROXY_COMP,
    CL_AMS_CKPT_OPERATION_CSI_SET_NVP,
    CL_AMS_CKPT_OPERATION_SET_PGTRACK_CLIENT,
    CL_AMS_CKPT_OPERATION_SERVER_DATA,
    CL_AMS_CKPT_OPERATION_USER_DATA,
    CL_AMS_CKPT_OPERATION_SET_FAILOVER_HISTORY,
    CL_AMS_CKPT_OPERATION_ENTITY_DELETE,
    CL_AMS_CKPT_OPERATION_END,
    CL_AMS_CKPT_OPERATION_MAX,
}ClAmsCkptOperationT;

/*
 * These macros are used for selectively writing the AMS checkpoints
 */

#define CL_AMS_CKPT_WRITE_ALL 1

#define CL_AMS_CKPT_WRITE_DB  2

#define CL_AMS_CKPT_WRITE_INVOCATION 3

extern ClRcT clAmsCkptDBWrite(void);

extern ClRcT clAmsCkptDBRead(void);

extern ClRcT
clAmsCkptReadCurrentDBInvocationPair(ClAmsT *ams,
                                     ClUint32T *pDBInvocationPair);


/*
 * Function declaration for AMS checkpointing functionality
 */

extern ClRcT
clAmsCkptInitialize(
        CL_INOUT  ClAmsT  *ams, 
        CL_IN  ClCkptHdlT  ckptInitHandle, 
        CL_IN  ClUint32T  mode );

extern ClRcT
clAmsCkptWrite(
        CL_IN  ClAmsT  *ams,
        CL_IN  ClUint32T  mode );

extern ClRcT
clAmsCkptWriteSync(
        CL_IN  ClAmsT  *ams,
        CL_IN  ClUint32T  mode );

extern ClRcT
clAmsCkptRead ( 
        CL_INOUT  ClAmsT  *ams );

extern ClRcT   
clAmsReadXMLFile(
        CL_IN  ClCharT  *fileName,
        CL_OUT  ClCharT  **readData );

extern ClRcT   
clAmsWriteXMLFile(
        CL_IN  ClCharT  *fileName,
        CL_IN  ClCharT  *writeData,
        CL_IN  ClSizeT  dataSize );

extern ClRcT 
clAmsCkptFree(
        CL_IN  ClAmsT  *ams );

extern ClRcT
clAmsHotStandbyRegister(ClAmsT *ams);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_CKPT_H_ */
