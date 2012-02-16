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
 * File        : clAmsDebug.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the function call types used by the client library to
 * call the server and for the server to call its peer. 
 *****************************************************************************/

#ifndef _CL_AMS_DEBUG_H_
#define _CL_AMS_DEBUG_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define AMS_DEBUG               1
#define AMS_CLIENT_DEBUG        1
#define AMS_SERVER_DEBUG        1
//#define AMS_EMULATE_RMD_CALLS   1

#ifdef  __cplusplus
}
#endif

#endif							/* _CL_AMS_DEBUG_H_ */
