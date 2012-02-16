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
 * ModuleName  : gms
 * File        : clGmsLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file instantiates the log message data structure for the GMS server
 * instance. 
 *****************************************************************************/
#include <clCommon.h>
#include "clGmsLog.h"



ClCharT clGmsLogMsg[9][256]= 
{
    "GMS Server Instance Started",
    "GMS Server Terminating",
    "GMS Server Bootup Failed",
    "New Group Created with Group id :%d",
    "Group with groupid:%d destroyed",
    "New Member [%d] Joined the Group [%d]",
    "Member [%d] Left the Group [%d]",
    "Leader changed from [%d] to [%d] of Group : [%d]",
    "Group [%d] reconfigured"
};
