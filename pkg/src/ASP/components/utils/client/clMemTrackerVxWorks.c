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
 * Build: 4.1.0
 */
/*
  The memTracker module for memory tracking.
  Does mem logging of backtrace during allocs and frees.
  Cant use container hashing for cyclic deps.
  (container uses malloc and we would reenter incase of
  memtracking.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clMemTracker.h>
#include "clHash.h"

void clMemTrackerAdd
        (
        const char *id,void *pAddress,ClUint32T size,void *pPrivate,
        ClBoolT logFlag
        )
                            
{
    return;
}
        
void clMemTrackerDelete
        (
        const char *id,void *pAddress,ClUint32T size,ClBoolT logFlag
        )
                            
{
    return;
}

/*
  Tracker trace that should be mostly invoked on
  mem corruption.
*/

void clMemTrackerTrace
        (
        const char *id,void *pAddress,ClUint32T size
        )

{
    return;
}

/*
  Given a corrupted chunk,detect the underrun trace.
 */
ClRcT clMemTrackerGet(void *pAddress,ClUint32T *pSize,void **ppPrivate)
{
    return CL_OK;
}

void clMemTrackerDisplayLeaks(void)
{
    return;
}

/*Initialize the tracker subsystem*/
ClRcT clMemTrackerInitialize(void)
{
    return CL_OK;
}

