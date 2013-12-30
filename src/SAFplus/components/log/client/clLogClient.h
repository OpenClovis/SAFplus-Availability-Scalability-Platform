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
 * ModuleName  : log
 * File        : clLogClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_LOG_CLIENT_H_
#define _CL_LOG_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmapApi.h>
#include <clLogApi.h>

#define CL_LOG_CLIENT_VERSION    {'B', 0x01, 0x01}

ClRcT
clLogClntSSOResponseProcess(ClHandleT  hLog,
                            SaNameT    *pStreamName,
                            SaNameT    *pNodeName,
                            ClStringT  *pShmName,
                            ClUint32T  shmSize,
                            ClHandleT  *phStream,
                            ClUint32T  recSize);
ClRcT
clLogClntHandlerRegisterParamValidate(ClLogHandleT              hLog, 
                                      SaNameT                   *pStreamName, 
                                      ClLogStreamScopeT         streamScope,
                                      SaNameT                   *pNodeName, 
                                      ClLogStreamHandlerFlagsT  handlerFlags,
                                      ClLogStreamHandleT        *phStream);

ClRcT clLogClntFilterSetCb(ClBitmapHandleT  hBitmap,
                           ClUint32T        bitNum, 
                           void             *pCookie);
ClRcT
clLogClientFilterSetNotify(SaNameT            streamName,
                           ClLogStreamScopeT  streamScope,
                           SaNameT            nodeName,
                           ClLogFilterT       filter);

ClRcT
clLogClntStdStreamOpen(ClLogHandleT hLog);

ClRcT
clLogClntStdStreamClose(ClUint32T  nStream);

ClRcT    
clLogSeverityFilterGet(ClLogStreamHandleT    hStream,
                       ClLogSeverityFilterT  *pSeverity);
ClRcT clLogClientFilterSetNotify_4_0_0(
                           /* Suppressing coverity warning for pass by value with below comment */
                           // coverity[pass_by_value]
                           SaNameT            streamName,
                           ClLogStreamScopeT  streamScope,
                           SaNameT            nodeName,
                           ClLogFilterT       filter);

    
#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_CLIENT_H_*/
