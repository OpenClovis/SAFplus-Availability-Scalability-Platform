/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/Yamuna/ASP/integration-test/event/common/clEvtTestCommon.h $
 * $Author: amitg $
 * $Date: 2006/10/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef EVT_TEST_COMMON_H
#define EVT_TEST_COMMON_H
#include <clCommon.h>

#define EVENT_TEST_CHANNEL "EVENT_TEST_CHANNEL"

#define EVENT_TEST_EVENT_TYPE 0x100
ClNameT gEvtTestChannelName = { 
        sizeof(EVENT_TEST_CHANNEL)-1,
        EVENT_TEST_CHANNEL
        };
#endif /* EVT_TEST_COMMON_H */

#define clOsalMalloc clHeapAllocate
#define clOsalFree clHeapFree
