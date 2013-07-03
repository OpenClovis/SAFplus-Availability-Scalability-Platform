#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocManagementApi.h>
#include <clIocErrors.h>
#include <clIocTransportApi.h>
#include <clIocUdpTransportApi.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

#include "iocExternalDefs.h"
#include "iocExternalReceiver.h"

void
externalIocReceiverStart(int socket_type, int ioc_address_local, int ioc_port_local, int mode)
{
    ClRcT rc = CL_OK;
    ClIocCommPortHandleT    iocCommPort;
    printf("external Ioc Application is started as receiver: addr=%d, port=%d, mode=%d\n",
            ioc_address_local, ioc_port_local, mode);
    /* Create receiver port and get ready for receiving messages */
    rc = clIocCommPortCreate((ClUint32T)ioc_port_local,
                             socket_type,
                             &iocCommPort);
    if (rc != CL_OK)
    {
        printf("Error: failed to create receiver port\n");
        exit(1);
    }

    rc = clIocPortNotification(ioc_port_local, CL_IOC_NOTIFICATION_DISABLE);
    if(rc != CL_OK)
    {
        printf("Error : failed to disable the port notifications from IOC. error code [0x%x].", rc);
        exit(1);
    }
    ClRcT retCode = CL_OK;
    ClIocPhysicalAddressT destAddress;
    ClBufferHandleT userMsg;
    ClUint8T priority;
    ClUint8T protoType;
    ClUint32T length;
    ClIocRecvOptionT recvOption = { 0 };
    ClIocRecvParamT recvParam = { 0 };

    while (1)
    {
        retCode = clBufferCreate(&userMsg);
        if (retCode != CL_OK)
        {
            //TESTCODE_PRINTF("\n APP; Message can not be created ");
            clIocCommPortReceiverUnblock(iocCommPort);
            clIocCommPortDelete(iocCommPort);
            printf("SERVER : create buffer error");
            return ; 
        }
        /*
         * set the length of the of the receive-message, but on the
         * receive side we can't know a-priori what the length would
         * be, so assume max-length
         */
        length = 5000000;
        recvOption.recvTimeout = 30000;

        retCode = clIocReceive(iocCommPort, &recvOption, userMsg, &recvParam);
        printf("SERVER : Got message");
        if (retCode != CL_OK)
        {
            clBufferDelete(&userMsg);
            printf ("SERVER :rev error error = 0x%x.\n",retCode);
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
            printf("SERVER : Got notification id=%d, instance=0x%x, port=0x%x\n",
                    notification.id,
                    notification.nodeAddress.iocPhyAddress.nodeAddress,
                    notification.nodeAddress.iocPhyAddress.portId);
            continue;
        }
        else
        {
            ClUint8T pTempData[2000];
            ClUint32T len = 100;
            clBufferNBytesRead(userMsg, pTempData, &len);
            pTempData[100] = '\0';
            printf("******* %s\n", pTempData);
        }

        printf("SERVER : Received data : length %d, protocol %d, priority %d, sender 0x%x:0x%x\n",
             length, protoType, priority, destAddress.nodeAddress,destAddress.portId);
    }
    return ;
    

}



