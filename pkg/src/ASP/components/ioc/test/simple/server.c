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
 * ModuleName  : ioc                                                           
 * $File: //depot/dev/main/Andromeda/Yamuna/ASP/components/ioc/test/unit-test/simple/simpleS.c $
 * $Author: amitg $
 * $Date: 2007/04/16 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *******************************************************************************/
/******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
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
#include <clIocIpi.h>
#include <clEoApi.h>
#define __IOC_PERF_SERVER__
#include "defs.h"




/******************************************************************************/
ClUint32T clAspLocalId = 0x1;
static ClIocCommPortHandleT iocCommPort;
static ClIocTLInfoT tlInfo;
static ClIocMcastUserInfoT mcastUserInfo;

#if 0
typedef void *( *taskStartFunction) ( void *);
#endif

ClUint8T clEoBasicLibs[1];
ClUint8T clEoClientLibs[1];
ClEoConfigT clEoConfig;

ClRcT recvLoop(void);
void serverCleanup(
    ClBufferHandleT * pUserMsg,
    ClIocCommPortHandleT iocCommPort
);

/******************************************************************************/


#ifdef VAR_ARGU_MACRO_SUPPORTED
#define TESTCODE_PRINTF(x,y...) printf("%s : %s : %d : "x,__FILE__,__FUNCTION__,__LINE__,y...)
#else
#define TESTCODE_PRINTF printf
#endif

static ClIocConfigT *gpClIocConfig;
static ClBoolT serverRunning=CL_TRUE;

int main(
    int argc, char **argv 
)
{
    ClRcT retCode = CL_OK;
    extern ClIocConfigT pAllConfig;
    ClIocPortT port;
    ClUint32T nodeRepFlag = 1;

    if(argc < 4)
    {
        clOsalPrintf("Usage : %s <ASP_NODEADDR> <PORT> <NODE_REP>\n", argv[0]);
        return -1;
    }

    sscanf(argv[1],"%u", &clAspLocalId);
    sscanf(argv[2],"%u", &port);
    sscanf(argv[3],"%u", &nodeRepFlag);
    
    retCode = clIocParseConfig(NULL,&gpClIocConfig);
    if(retCode != CL_OK)
    {
        clOsalPrintf("Error in ioc parse config.rc=0x%x\n",retCode);
        exit(1);
    }
    if( (retCode=clOsalInitialize(NULL)) != CL_OK   || 
        (retCode=clHeapInit()) != CL_OK ||
        (retCode=clTimerInitialize(NULL)) != CL_OK  || 
        (retCode=clBufferInitialize(NULL)) != CL_OK)
    {
        TESTCODE_PRINTF("Error : IOC initialization failed. error code = 0x%x\n", retCode);
        return (retCode);
    }

    pAllConfig.iocConfigInfo.isNodeRepresentative = nodeRepFlag;
    retCode = clIocLibInitialize(NULL);
    if (retCode != CL_OK)
    {
        TESTCODE_PRINTF("IOC  initialization failed with error code = 0x%x\n\n\n  ",
               retCode);
        return (retCode);
    }


    /*
     * create an IOC comm port 
     */
    TESTCODE_PRINTF("Info : server port Id 0x%x\n", port);
    retCode =
        clIocCommPortCreate(port, CL_IOC_UNRELIABLE_MESSAGING, &iocCommPort);
    if (retCode != CL_OK)
    {
        TESTCODE_PRINTF("Comm. port create failed and the return code =0x%x",
               retCode);
        return (-1);
    }
#ifdef COMMPORT_STRESS
    do
    {
        register ClInt32T i;
        ClIocPortT *pIds = clHeapCalloc(100,sizeof(*pIds));
        assert(pIds);
        for(i = 0; i < 100;++i)
        {
            retCode = clIocCommPortCreate(0,CL_IOC_UNRELIABLE_MESSAGING,pIds+i);
            assert(retCode==CL_OK);
        }
    }while(0);
#endif
    memset(&tlInfo,0,sizeof(tlInfo));
    memset(&mcastUserInfo,0,sizeof(mcastUserInfo));
    tlInfo.physicalAddr.nodeAddress = clAspLocalId;
    tlInfo.physicalAddr.portId = port;
    tlInfo.haState = CL_IOC_TL_ACTIVE;
    tlInfo.compId = SERVER_LOGICAL_ADDRESS;
    tlInfo.logicalAddr = CL_IOC_LOGICAL_ADDRESS_FORM(SERVER_LOGICAL_ADDRESS);
    retCode = clIocTransparencyRegister(&tlInfo);
    if(retCode != CL_OK)
    {
        clOsalPrintf("error in transparency register.rc=0x%x\n",retCode);
        return -1;
    }

    while(1)
    {
    ClRcT rc;
    ClIocNodeAddressT nodeAddr=0;
    sleep(3);
    rc = clIocMasterAddressGet(tlInfo.logicalAddr, port, &nodeAddr);
    if(rc != CL_OK) {
       clOsalPrintf("Failed to get the nodeAddress. error code [0x%x].\n", rc);
       continue;
    }
    clOsalPrintf("The logical addr %llx, nodeAddr %d\n", tlInfo.logicalAddr, nodeAddr);
    } 
    

#if 0
    mcastUserInfo.mcastAddr = CL_IOC_MULTICAST_ADDRESS_FORM(10, SERVER_PORT_ID);
    clOsalPrintf("The multicast Address is 0x%llx\n", mcastUserInfo.mcastAddr);
    mcastUserInfo.physicalAddr.nodeAddress = clAspLocalId;
    mcastUserInfo.physicalAddr.portId = port;
    retCode = clIocMulticastRegister(&mcastUserInfo);
    if(retCode != CL_OK)
    {
        clOsalPrintf("error in multicast register.rc=0x%x\n",retCode);
    }
    

    signal(SIGINT,sigintHandler);

    recvLoop();
#endif

    serverCleanup(NULL, iocCommPort);

    return 0;
}

/******************************************************************************/

void serverCleanup(
    ClBufferHandleT * pUserMsg,
    ClIocCommPortHandleT iocCommPort
)
{
    clIocCommPortReceiverUnblock(iocCommPort);
    clIocCommPortDelete(iocCommPort);
    if (pUserMsg)
        clBufferDelete(pUserMsg);
    clIocLibFinalize();
}

/******************************************************************************/

ClRcT recvLoop(
)
{
    ClRcT retCode = CL_OK;
    ClIocPhysicalAddressT destAddress;
    ClBufferHandleT userMsg;
    ClUint8T priority;
    ClUint8T protoType;
    ClUint32T length;
    static int cnt = 0;
    ClIocRecvOptionT recvOption = { 0 };
    ClIocRecvParamT recvParam = { 0 };


    while (serverRunning == CL_TRUE)
    {
        retCode = clBufferCreate(&userMsg);
        if (retCode != CL_OK)
        {
            TESTCODE_PRINTF("\n APP; Message can not be created ");
            clIocCommPortReceiverUnblock(iocCommPort);
            clIocCommPortDelete(iocCommPort);
            return (-1);
        }
        /*
         * set the length of the of the receive-message, but on the
         * receive side we can't know a-priori what the length would
         * be, so assume max-length
         */
        length = 5000000;
        recvOption.recvTimeout = 3000;

        retCode = clIocReceive(iocCommPort, &recvOption, userMsg, &recvParam);
        if (retCode != CL_OK)
        {
            clBufferDelete(&userMsg);
            continue;
        }
        
        priority = recvParam.priority;
        destAddress.nodeAddress = recvParam.srcAddr.iocPhyAddress.nodeAddress;
        destAddress.portId = recvParam.srcAddr.iocPhyAddress.portId;
        protoType = recvParam.protoType;
        length = recvParam.length;

        if(protoType == CL_IOC_PORT_NOTIFICATION_PROTO)
        {
            ClIocNotificationT notification;
            ClUint32T length = sizeof(notification);
            clBufferNBytesRead(userMsg, (ClUint8T*)&notification, &length);
            clOsalPrintf("SERVER : Got notification id=%d, instance=0x%x, port=0x%x\n",
                    notification.id,
                    notification.nodeAddress.iocPhyAddress.nodeAddress,
                    notification.nodeAddress.iocPhyAddress.portId);
            continue;
        }

        {
            ClUint8T pTempData[2000];
            ClUint32T len = 100;

            clBufferNBytesRead(userMsg, pTempData, &len);

            pTempData[100] = '\0';
            TESTCODE_PRINTF("******* %s\n", pTempData);
        }
        
        TESTCODE_PRINTF
            ("SERVER : %d Received data : length %d, protocol %d, priority %d, sender 0x%x:0x%x\n",
             ++cnt, length, protoType, priority, destAddress.nodeAddress,
             destAddress.portId);

    }
    return 0;
}
