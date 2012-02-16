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
 * ModuleName  : name
 * File        : clNameLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains NS specific log messages.
 *****************************************************************************/
#include "clCommon.h"

ClCharT  *clNameLogMsg[] =
{
    "NS Context Creation failed, rc=0x%x",
    "Not permitted to delete default NS contexts",
    "NS Context deletion failed, rc=0x%x",
    "NS Registration failed, rc=0x%x",
    "NS Query Database failed, rc=0x%x",
    "NS Display request failed, rc=0x%x",
    "NS Service Deregistration failed, rc=0x%x",
    "NS Component Deregistration failed, rc=0x%x",
    "NS syncup failed, rc=0x%x",
    "NS Initialization  failed, rc=0x%x",
    "NS Entry Cleanup failed, rc=0x%x",
};

