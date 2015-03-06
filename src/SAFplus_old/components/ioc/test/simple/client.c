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
 * ModuleName  : ioc                                                           
 * $File: //depot/dev/main/Andromeda/Yamuna/ASP/components/ioc/test/unit-test/simple/simpleC.c $
 * $Author: karthick $
 * $Date: 2007/04/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 ******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <clCntApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clBufferApi.h>
#include <clTimerApi.h>
#include <clIocErrors.h>
#include <clIocManagementApi.h>
#include <clOsalApi.h>
#include <clIocTransportApi.h>
#include <clIocParseConfig.h>
#include <clEoApi.h>
#define __IOC_PERF_CLIENT__
#include "defs.h"



/******************************************************************************/
ClUint32T clAspLocalId = 0x1;
ClUint32T msgLen=1000;
ClUint32T numTimes=10;
ClIocCommPortHandleT iocCommPort;

#if 0
typedef void *( *taskStartFunction) ( void *);
#endif
ClUint8T clEoBasicLibs[1];
ClUint8T clEoClientLibs[1];
ClEoConfigT clEoConfig;

static ClRcT LoopFinish(void);
static ClRcT sendLoop(void);
static void clientCleanup(
    ClBufferHandleT * pUserMsg1,
    ClBufferHandleT * pUserMsg2,
    ClIocCommPortHandleT iocCommPort
);
static ClCharT *progName;
static ClIocConfigT *gpClIocConfig;
ClRcT clEoProgNameGet(ClCharT *pName,ClUint32T size)
{
  snprintf(pName,size,"%s",progName);
  return CL_OK;
}

int main(
    int argc,
    char **argv
)
{

    ClRcT retCode = CL_OK;
    ClInt32T i = 0;
#if 0
    extern ClIocConfigT pAllConfig;
    ClOsalTaskIdT taskId = 0;
#endif

    if (argc != 1 && argc != 3 && argc != 5)
    {
        goto usage;
    }
    progName = argv[0];
    for(i=1 ; i < argc; i=i+2)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 's' : msgLen = strtoul(argv[i+1], NULL, 10);
                           break;
                case 'n' : numTimes = strtoul(argv[i+1], NULL, 10);
                           break;
                default :
                           goto usage;
            }
        } 
        else
        {
            goto usage;
        }
    }

    retCode = clIocParseConfig(NULL,&gpClIocConfig);
    if(retCode != CL_OK)
    {
        clOsalPrintf("Error in ioc parse config.rc=0x%x\n",retCode);
        exit(1);
    }

    /*
     * initialize IOC which includes SISP components 
     */
    if((retCode=clOsalInitialize(NULL))!= CL_OK || 
       (retCode=clHeapInit())!=CL_OK ||
       (retCode=clTimerInitialize(NULL)) != CL_OK || 
       (retCode=clBufferInitialize(NULL)) != CL_OK)
    {
        printf("\nIOC  initialization failed with error code = 0x%x\n\n\n  ",
               retCode);
        return (retCode);
    }
    
#if 0
    pAllConfig.iocConfigInfo.isNodeRepresentative = CL_TRUE;
#endif
    retCode = clIocLibInitialize(NULL);
    if (retCode != CL_OK)
    {
        printf("\nIOC  initialization failed with error code = 0x%x\n\n\n  ",
               retCode);
        return (retCode);
    }

#if 0
    while(1)
    {
        sleep(3);
        clIocArpTablePrint();
        clOsalPrintf("\n\n\n");
    }
#endif

    /*
     * create an IOC comm port 
     */
#if 0
    {
        ClIocCommPortHandleT iocCommPort[1200] = {0};


    for(i = 0 ; i < 1050; i++)
    {
        retCode =
            clIocCommPortCreate(0, CL_IOC_UNRELIABLE_MESSAGING, &iocCommPort[i]);
        if (retCode != CL_OK)
        {
            printf("\nError : %d Comm. port create failed. error= 0x%x***********************************\n", i, retCode);
        } else
        {
            printf("\nInfo : %d Comm. port 0x%x \n", i, (int)iocCommPort[i]);
        }
    }

    printf("===============================================================================\n");

    for(i = 0 ; i < 1050; i++)
    {
        retCode = 
            clIocCommPortDelete(iocCommPort[i]);
        if(retCode == CL_OK)
            printf("%d Commport deletion successfull\n", i);
        else
            printf("%d Commport deletion failed++++++++++++++++++++++++++++\n", i);

    }
    while(1);
    }
#else
    retCode = clIocCommPortCreate(0, CL_IOC_UNRELIABLE_MESSAGING, &iocCommPort);
    if(retCode != CL_OK)
    {
        printf("Error : Failed to create a commport. rc 0x%x\n", retCode);
        return -1;
    }
#endif

#if 0
    for (i = 0; i < 100; i++)
    {
        clOsalTaskCreate("Send Loop", CL_OSAL_SCHED_OTHER, 1, 0,
                         (taskStartFunction) sendLoop, NULL, &taskId);
    }
#else
    sendLoop();
#endif
    
    
    clientCleanup(NULL, NULL, iocCommPort);
    return (0);

usage :
    printf("USAGE: %s -s <packet-size-in-bytes> -n <number-of-times>\n", argv[0]);
    return (-1);
    
}

/******************************************************************************/
static void clientCleanup(
    ClBufferHandleT * pUserMsg1,
    ClBufferHandleT * pUserMsg2,
    ClIocCommPortHandleT liocCommPort
)
{
    LoopFinish(); /* Just for the sake of wandering packets */
    
    clIocCommPortReceiverUnblock(liocCommPort);
    clIocCommPortDelete(liocCommPort);
    clIocLibFinalize();
    clBufferFinalize();
    clTimerFinalize();
    clHeapExit();
    clOsalFinalize();
}


static ClRcT sendLoop(
)
{
    ClRcT retCode = CL_OK;
    ClIocAddressT destAddress;
    ClUint8T *msg;
    ClInt32T i = 0;
    ClBufferHandleT myMessage;
    ClIocSendOptionT sndOption;
    /*
     * create and fill the send-message with some data, in this case, the
     * letter 'i' 
     */
    retCode = clBufferCreate(&myMessage);
    if (retCode != CL_OK)
    {
        printf("CLIENT: Message can not be created\n");
        clIocCommPortDelete(iocCommPort);
        return 0;
    }

    msg = clHeapAllocate(msgLen);
    if(msg == NULL)
    {
        clBufferDelete(&myMessage);
        return -1;
    }
    
    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = 0;

    for(i = 0; i < numTimes; i++)
    {
        ClCharT c = 'M';
        
        memset(msg, c/*+clSispLocalId*/,msgLen);

        clBufferWriteOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);
        clBufferReadOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);

        assert(clBufferNBytesWrite(myMessage, msg, msgLen) == CL_OK);

        destAddress.iocMulticastAddress = CL_IOC_MULTICAST_ADDRESS_FORM(10, SERVER_PORT_ID);

        /*
         * send the message on the comm-port 
         */
        sndOption.priority = (i % 4) + 1;
        
        retCode =
            clIocSend(iocCommPort, myMessage, IOC_PERF_PROTOCOL_ID,
                       &destAddress, &sndOption);
        if (retCode != CL_OK)
        {
            printf("Error : CLIENT : %d commPort send : length %d. error = 0x%x.\n",
                 i+1, msgLen, retCode);
            i--;
        }

        printf("CLIENT: %d packet sent : length %d.\n", i+1,msgLen);
        sleep(1);

    }
    clHeapFree(msg);
    clBufferDelete(&myMessage);
    return 0;
}

static ClRcT LoopFinish(
)
{
    return CL_OK;
    ClRcT retCode = CL_OK;
    ClBufferHandleT userMsg;
    ClIocRecvOptionT recvOption = { 0 };
    ClIocRecvParamT recvParam = { 0 };

    while (1)
    {
        retCode = clBufferCreate(&userMsg);
        if (retCode != CL_OK)
        {
            printf("\n APP; Message can not be created ");
            return 1;
        }
        recvOption.recvTimeout = 1000;
        /*
         * receive the message on the comm-port 
         */
        retCode = clIocReceive(iocCommPort, &recvOption, userMsg, &recvParam);

        clBufferDelete(&userMsg);
        if (retCode != CL_OK)
            return 1;
    }
    return 0;
}

/******************************************************************************/
/******************************************************************************/
