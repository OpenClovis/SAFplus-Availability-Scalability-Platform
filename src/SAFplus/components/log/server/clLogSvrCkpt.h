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
#ifndef _CL_LOG_SVR_CKPT_H_
#define _CL_LOG_SVR_CKPT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <clBufferApi.h>
#include <clLogApi.h>
#include <clLogServer.h>


extern ClRcT
clLogSvrCkptGet(CL_IN  ClLogSvrEoDataT        *pSvrEoData,
                CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry, 
                CL_OUT ClBoolT                *pLogRestart);

extern ClRcT
clLogSvrCheckpointRead(void);

extern ClRcT
clLogSvrCkptDataSetDelete(ClUint32T         dsId);

extern ClRcT
clLogSvrStreamCheckpoint(CL_IN  ClLogSvrEoDataT     *pSvrEoData,
                         CL_IN  ClCntNodeHandleT    svrStreamNode);

#ifdef __cplusplus
}
#endif
#endif
