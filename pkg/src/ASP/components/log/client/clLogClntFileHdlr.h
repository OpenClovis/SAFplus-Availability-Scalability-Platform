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
#ifndef _CL_LOG_FILE_HDLR_CLIENT_H_
#define _CL_LOG_FILE_HDLR_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>    

#define  CL_LOG_CLNT_TIMESTAMP_OFFSET sizeof(ClUint8T) + \
                                      sizeof(ClLogSeverityT) + \
                                      sizeof(ClUint16T) + sizeof(ClUint32T) +\
                                      sizeof(ClUint16T) 
#define  CL_LOG_TIME_MAX_VALUE        0xFFFFFFFFFFFFFFFE
    
#define  CL_LOG_TIME_MIN_VALUE        0x0
typedef struct
{
    ClStringT  fileName;
    ClStringT  fileLocation;
}ClLogClntFileKeyT;

extern ClRcT
clLogClntFileKeyCreate(ClCharT           *fileName,
                       ClCharT           *fileLocation,
                       ClLogClntFileKeyT  **pFileKey);
extern void
clLogClntFileKeyDestroy(ClLogClntFileKeyT  *pFileKey);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_FILE_HDLR_CLIENT_H_ */
