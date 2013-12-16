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
 *//*
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
#include <clIocApi.h>
#include <saAis.h>
#include <saMsg.h>
#include <clMsgApiExt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_MSG_DEFAULT_DELAY            {.tsSec = 0, .tsMilliSec = 500 }
#define CL_MSG_DEFAULT_RETRIES          (5)


#define CL_MSG_RC(err)          CL_RC(CL_CID_MSG, err)

extern ClBoolT gClMsgInit;
extern ClIocNodeAddressT gLocalAddress;
extern ClIocPortT gLocalPortId;

#define CL_MSG_SA_RET(err)  (SaAisErrorT)err

#define CL_MSG_INIT_CHECK(ret) \
    do { \
         if(gClMsgInit == CL_FALSE) \
         {                                                                   \
             ret = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);                   \
             clLogError("MSG", "ERR", "Message client or server is not yet initialized. Please try after some time. error code [0x%x].", ret); \
         }                                                                   \
    } while(0)

ClRcT clMsgIovecToMessageCopy(SaMsgMessageT **ppNewMsg, ClMsgMessageIovecT *pMessage, ClUint32T index);
ClRcT clMsgIovecToIovecCopy(ClMsgMessageIovecT **ppNewMsg, ClMsgMessageIovecT *pMessage);
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
      
typedef enum {
    CL_MSG_DATA_ADD = 1,
    CL_MSG_DATA_DEL,
    CL_MSG_DATA_UPD
}ClMsgSyncActionT;

typedef enum {
    CL_MSG_SEND = 1,
    CL_MSG_REPLY_SEND = 2,
    CL_MSG_SEND_RECV = 3,
} ClMsgMessageSendTypeT;

#ifdef __cplusplus
}
#endif

#endif
