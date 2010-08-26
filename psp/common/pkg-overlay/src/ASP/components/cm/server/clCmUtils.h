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
/******************************************************************************
 * Description :
 * Header file for clCmUtils.c
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_CM_UTILS_H_
#define _CL_CM_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <SaHpi.h>
#include <oh_utils.h>

#define DISPLAY_BUFFER_LEN 2560

extern void clCmDisplayProgressBar(ClInt16T *button);
extern char* clCmSensorRange(SaHpiSensorRangeFlagsT flags);
extern char* clCmEpath2String(SaHpiEntityPathT *epath);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CM_UTILS_H_ */
