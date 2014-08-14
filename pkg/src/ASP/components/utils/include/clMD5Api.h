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
 * Build: 5.0.0
 */
/*******************************************************************************
 * ModuleName  : utils
 * File        : clMD5Api.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * MD5 checksum library.
 *
 *****************************************************************************/

#ifndef _CL_MD5API_H_
#define _CL_MD5API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

typedef struct ClMD5
{
    ClUint8T md5sum[16];
} ClMD5T;

void clMD5Compute(ClUint8T *data, ClUint32T size, ClUint8T result[16]);

#ifdef __cplusplus
}
#endif

#endif /* _CL_MD5API_H_ */

