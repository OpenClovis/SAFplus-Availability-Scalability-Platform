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
                            ClNameT    *pStreamName,
                            ClNameT    *pNodeName,
                            ClStringT  *pShmName,
                            ClUint32T  shmSize,
                            ClHandleT  *phStream);
ClRcT
clLogClntHandlerRegisterParamValidate(ClLogHandleT              hLog, 
                                      ClNameT                   *pStreamName, 
                                      ClLogStreamScopeT         streamScope,
                                      ClNameT                   *pNodeName, 
                                      ClLogStreamHandlerFlagsT  handlerFlags,
                                      ClLogStreamHandleT        *phStream);

ClRcT clLogClntFilterSetCb(ClBitmapHandleT  hBitmap,
                           ClUint32T        bitNum, 
                           void             *pCookie);
ClRcT
clLogClientFilterSetNotify(ClNameT            streamName,
                           ClLogStreamScopeT  streamScope,
                           ClNameT            nodeName,
                           ClLogFilterT       filter);

ClRcT
clLogClntStdStreamOpen(ClLogHandleT hLog);

ClRcT
clLogClntStdStreamClose(ClUint32T  nStream);

ClRcT    
clLogSeverityFilterGet(ClLogStreamHandleT    hStream,
                       ClLogSeverityFilterT  *pSeverity);
    
#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_CLIENT_H_*/
