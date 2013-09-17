#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocManagementApi.h>
#include <clIocErrors.h>
#include <clIocTransportApi.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

#include "iocExternalDefs.h"
#include "iocExternalSender.h"

# define CL_IOC_BROADCAST_ADDRESS  0xffffffff
#define CL_BLOCKING_RECV_TIMEOUT 1000

void
externalIocSenderStart(int socket_type,
        int  ioc_address_local,
        int  ioc_address_dest,
        int  ioc_port_dest,
        int  mode)
{
    ClRcT 					rc = CL_OK;
    ClIocCommPortHandleT    iocCommPort;

    fprintf(stderr, "external IOC Application is started as sender:\n");
    fprintf(stderr, "    local_address         = %d\n", ioc_address_local);
    fprintf(stderr, "    dest_address          = %d\n", ioc_address_dest);
    fprintf(stderr, "    dest_port             = %d\n", ioc_port_dest);
    fprintf(stderr, "    mode                  = %d\n", mode);
 

    rc = clIocCommPortCreate( CL_IOC_RESERVED_PORTS+10, 
            socket_type, 
            &iocCommPort);
    if (rc != CL_OK)
    {
        fprintf(stderr, "Error [%#x]. Failed to create sender port [%d]\n", 
		rc, CL_IOC_RESERVED_PORTS+10);
        exit(1);
    }

    sleep(4); /* To give some time to IOC to propagate the port info */
    printf("send multiple address\n");
    ClRcT retCode = CL_OK;
    ClIocAddressT destAddress;
    ClUint8T *msg;
    int i = 0;
    ClUint32T msgLen=1000;
    ClBufferHandleT myMessage;
    ClIocSendOptionT sndOption;
    ClUint8T protoType = CL_IOC_USER_PROTO_START + 1;
    /*clIocLibFinalize
     * create and fill the send-message with some data, in this case, the
     * letter 'i'
     */
    printf("CLIENT: begin sent ioc");
    retCode = clBufferCreate(&myMessage);
    if (retCode != CL_OK)
    {
    	printf("CLIENT: Message can not be created\n");
        clIocCommPortDelete(iocCommPort);
        return ;
    }

    msg = (ClUint8T*)clHeapAllocate(msgLen);
    if(msg == NULL)
    {
        clBufferDelete(&myMessage);
        return ;
    }

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = 0;
    int numTimes= 100;
    for(i = 0; i < numTimes; i++)
    {
        ClCharT c = 'k';

        memset(msg, c/*+clSispLocalId*/,msgLen);

        clBufferWriteOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);
        clBufferReadOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);
        if (clBufferNBytesWrite(myMessage, msg, msgLen) != CL_OK)
        {
            printf("Error : CLIENT : clBufferNBytesWrite fail");
            return;
        }
        destAddress.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
        destAddress.iocPhyAddress.portId = 77;

        /*
         * send the message on the comm-port
         */
        sndOption.priority = (i % 4) + 1;
        retCode =clIocSend(iocCommPort, myMessage, protoType,&destAddress, &sndOption);
        if (retCode != CL_OK)
        {
        	printf("Error : CLIENT : %d commPort send : length %d. error = 0x%x.\n",
                 i+1, msgLen, retCode);
            i--;
        }
        printf("CLIENT: %d packet sent : length %d. 0x%x \n", i+1,msgLen,destAddress.iocPhyAddress.portId);
        sleep(10);

    }
    clHeapFree(msg);
    clBufferDelete(&myMessage);
    return;
}



