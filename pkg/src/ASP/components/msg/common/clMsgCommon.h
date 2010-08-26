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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description : This file contains the data-structures and function declarations
 *               of clMsgCommon.c functions. needed by others. 
 *****************************************************************************/


#ifndef __CL_MSG_COMMON_H__
#define __CL_MSG_COMMON_H__

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <saAis.h>
#include <saMsg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_MSG_DEFAULT_DELAY            {.tsSec = 0, .tsMilliSec = 500 }
#define CL_MSG_DEFAULT_RETRIES          (5)


#define CL_MSG_RC(err)          CL_RC(CL_CID_MSG, err)

ClRcT clMsgMessageToMessageCopy(SaMsgMessageT **ppNewMsg, SaMsgMessageT *pMessage);
void clMsgMessageFree(SaMsgMessageT *pMessage);


/* NOTE : ClTimerTimeOutT's tsSec is ClUint32T and it can at max hold CL_MAX_TIMEOUT seconds. */
#define CL_MAX_TIMEOUT ((SaTimeT)((ClUint64T)(~(ClUint32T)0))) 

#define CL_MSG_SEND_TIME_GET(currentTime) \
    do{ \
        ClTimerTimeOutT tempTime; \
        clOsalTimeOfDayGet(&tempTime); \
        currentTime = ((SaTimeT)(tempTime.tsSec)*1000 + tempTime.tsMilliSec) * 1000 * 1000; \
    } while(0)


static __inline__ void clMsgTimeConvert(ClTimerTimeOutT *pClTime, SaTimeT saTime)
{
    SaTimeT tempSaTime = saTime/(1000*1000);
    if(saTime == SA_TIME_MAX || tempSaTime/1000 >= CL_MAX_TIMEOUT)
    {
        pClTime->tsSec = 0;
        pClTime->tsMilliSec = 0;
    }
    else
    {
        pClTime->tsSec = tempSaTime/1000;
        pClTime->tsMilliSec = tempSaTime % 1000;
    }
}
        
#ifdef __cplusplus
}
#endif

#endif
