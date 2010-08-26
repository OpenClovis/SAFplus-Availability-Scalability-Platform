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
 * ModuleName  : amf
 * File        : clAmsDefaultConfig.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains definitions for AMS default configuration.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_DEFAULT_CONFIG_H_
#define _CL_AMS_DEFAULT_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clAmsMgmtCommon.h>

/*
* Defaults for debug flags that control the verbosity of output. Can be set
* statically at compile time as well as dynamically at run time via the AMS
* management API and the ASP debug CLI.
*/

#define CL_AMS_DEBUG_FLAGS_DEFAULT      CL_AMS_DEBUG_FLAGS_GACODE

#define CL_AMS_DEBUG_FLAGS_GACODE       ( CL_AMS_MGMT_SUB_AREA_MSG )
#define CL_AMS_DEBUG_FLAGS_BETACODE     ( CL_AMS_MGMT_SUB_AREA_MSG | \
                                         CL_AMS_MGMT_SUB_AREA_STATE_CHANGE )
#define CL_AMS_DEBUG_FLAGS_ALPHACODE    ( CL_AMS_MGMT_SUB_AREA_MSG | \
                                         CL_AMS_MGMT_SUB_AREA_STATE_CHANGE | \
                                         CL_AMS_MGMT_SUB_AREA_TIMER )
#define CL_AMS_DEBUG_FLAGS_ALL          0xFF


#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_DEFAULT_CONFIG_H_ */
