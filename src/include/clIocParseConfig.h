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
#ifndef _CL_IOC_PARSE_CONFIG_H_
#define _CL_IOC_PARSE_CONFIG_H_

#ifdef __cplusplus

extern "C" { 

#endif

#include <clCommon6.h>
#include <clIocApi.h>
#include <clIocUdpTransportApi.h>

    typedef struct ClIocConfig
    {
        ClIocLibConfigT iocConfigInfo;
        /*Transport info*/
        ClUint32T numOfXports;
        ClIocUserTransportConfigT *transport;
        ClEoActionInfoT wmActions[CL_IOC_QUEUE_MAX];
    } ClIocConfigT;

    extern ClRcT clIocParseConfig(const ClCharT *pNodeName,ClIocConfigT **ppConfig);

#ifdef __cplusplus
}
#endif

#endif
