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
#ifndef _CL_LOG_STREAM_HDLR_UTILS_H_
#define _CL_LOG_STREAM_HDLR_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogCommon.h>    
#include <clLogMaster.h>
#include <xdrClLogStreamAttrIDLT.h>

#define  CL_LOG_FILE_CNT_MAX    0XFFFFFFFE

#define  CL_LOG_MILLISEC_IN_SEC  1000    

extern ClRcT
clLogFileNameForStreamAttrGet(ClLogStreamAttrIDLT  *pStreamAttr,
                              ClCharT              **pFileName);

extern ClRcT
clLogFileOwnerFileNameGet(ClStringT  *pFileName,
                           ClStringT  *pFileLocation,
                           ClCharT    **ppFileName);

extern ClRcT
clLogFileOwnerTimeGet(ClCharT   *pStrTime,
                       ClInt32T  delta);

extern ClRcT
clLogFileOwnerSoftLinkCreate(ClCharT    *fileName,
                              ClCharT    *softLinkName, 
                              ClCharT    *pTimeStr,
                              ClUint32T  fileCount);
extern ClRcT
clLogFileOwnerSoftLinkDelete(ClLogFileKeyT  *pFileKey, 
                              ClUint32T      fileMaxCnt);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_COMMON_H_ */
