#include <iocApp.h>

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, "MAI", CL_LOG_CONTEXT_UNSPECIFIED,__VA_ARGS__)

ClRcT recvIocLoop(ClIocCommPortHandleT iocCommPortRev)
{
    ClRcT retCode = CL_OK;
    ClIocPhysicalAddressT destAddress;
    ClBufferHandleT userMsg;
    ClUint8T priority;
    ClUint8T protoType;
    ClUint32T length;
    //static int cnt = 0;
    ClIocRecvOptionT recvOption = { 0 };
    ClIocRecvParamT recvParam = { 0 };

    while (1)
    {
        retCode = clBufferCreate(&userMsg);
        if (retCode != CL_OK)
        {
            //TESTCODE_PRINTF("\n APP; Message can not be created ");
            clIocCommPortReceiverUnblock(iocCommPortRev);
            clIocCommPortDelete(iocCommPortRev);
            clprintf (CL_LOG_SEV_INFO,"SERVER : create buffer error");
            return (-1);
        }
        /*
         * set the length of the of the receive-message, but on the
         * receive side we can't know a-priori what the length would
         * be, so assume max-length
         */
        length = 5000000;
        recvOption.recvTimeout = 30000;

        retCode = clIocReceive(iocCommPortRev, &recvOption, userMsg, &recvParam);
        clprintf (CL_LOG_SEV_INFO,"SERVER : Got message");
        if (retCode != CL_OK)
        {
            clBufferDelete(&userMsg);
            clprintf (CL_LOG_SEV_INFO,"SERVER :rev error error = 0x%x.\n",retCode);
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
            clprintf (CL_LOG_SEV_INFO,"SERVER : Got notification id=%d, instance=0x%x, port=0x%x\n",
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
            clprintf (CL_LOG_SEV_INFO,"******* %s\n", pTempData);
        }

        clprintf (CL_LOG_SEV_INFO,"SERVER : Received data : length %d, protocol %d, priority %d, sender 0x%x:0x%x\n",
             length, protoType, priority, destAddress.nodeAddress,destAddress.portId);
    }
    return 0;
}

ClRcT sendIocLoop(ClIocCommPortHandleT iocCommPortSend,ClUint8T mSendProtoId,ClCharT c)
{
    ClRcT retCode = CL_OK;
    ClIocAddressT destAddress;
    ClUint8T *msg;
    int i = 0;
    ClBufferHandleT myMessage;
    ClIocSendOptionT sndOption;
    ClUint8T protoType = mSendProtoId;
    ClUint32T msgLen=1000;
    /*
     * create and fill the send-message with some data, in this case, the
     * letter 'i'
     */
    clprintf(CL_LOG_SEV_INFO,"CLIENT: begin sent ioc");
    retCode = clBufferCreate(&myMessage);
    if (retCode != CL_OK)
    {
    	clprintf(CL_LOG_SEV_INFO,"CLIENT: Message can not be created\n");
        clIocCommPortDelete(iocCommPortSend);
        return 0;
    }

    msg = (ClUint8T*)clHeapAllocate(msgLen);
    if(msg == NULL)
    {
        clBufferDelete(&myMessage);
        return -1;
    }

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = 0;
    int numTimes = 1000;
    for(i = 0; i < numTimes; i++)
    {

        memset(msg, c/*+clSispLocalId*/,msgLen);
        clBufferWriteOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);
        clBufferReadOffsetSet(myMessage,0,CL_BUFFER_SEEK_SET);
        assert(clBufferNBytesWrite(myMessage, msg, msgLen) == CL_OK);
        destAddress.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
        destAddress.iocPhyAddress.portId = 77;

        /*
         * send the message on the comm-port
         */
        sndOption.priority = (i % 4) + 1;
        retCode =clIocSend(iocCommPortSend, myMessage, protoType,&destAddress, &sndOption);
        if (retCode != CL_OK)
        {
        	clprintf(CL_LOG_SEV_INFO,"Error : CLIENT : %d commPort send : length %d. error = 0x%x.\n",
                 i+1, msgLen, retCode);
            i--;
        }
        clprintf(CL_LOG_SEV_INFO,"CLIENT: %d packet sent : length %d. 0x%x \n", i+1,msgLen,destAddress.iocPhyAddress.portId);
        sleep(10);

    }
    clHeapFree(msg);
    clBufferDelete(&myMessage);
    return 0;
}

void initializeIoc(ClUint8T sendProtoId, ClUint8T receiveProtoId)
{
    clprintf (CL_LOG_SEV_INFO, "Enter initializeIoc");
    /* Install Mgt Ioc Protocol */
    ClEoProtoDefT eoProtoDef =
    {
    	receiveProtoId,
        "Protocol to receive message from the caller",
        (ClEoProtoCallbackT) clMgtIocRequestHandle,
        NULL,
        CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE
    };

    clEoProtoSwitch(&eoProtoDef);
	clprintf (CL_LOG_SEV_INFO, "finish initial IOC");

}

ClRcT clMgtIocRequestHandle(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr)
{
	clprintf (CL_LOG_SEV_INFO, "receive message");
	if(eoRecvMsg)
		clBufferDelete(&eoRecvMsg);
	return CL_OK;
}

