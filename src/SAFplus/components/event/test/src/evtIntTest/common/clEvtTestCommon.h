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
