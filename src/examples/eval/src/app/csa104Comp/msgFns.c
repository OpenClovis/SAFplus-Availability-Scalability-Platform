#include <assert.h>
#include "msgFns.h"

SaMsgHandleT            msgLibraryHandle;
SaMsgQueueHandleT       msgQueueHandle;

void msgInitialize(void)
{
    SaAisErrorT             Rc;
    SaMsgCallbacksT         MsgCallbacks = {
                                .saMsgQueueOpenCallback           = (SaMsgQueueOpenCallbackT) 0,
                                .saMsgQueueGroupTrackCallback     = (SaMsgQueueGroupTrackCallbackT) 0,
                                .saMsgMessageDeliveredCallback    = (SaMsgMessageDeliveredCallbackT) 0,
                                .saMsgMessageReceivedCallback     = (SaMsgMessageReceivedCallbackT) 0,
                            };
    SaVersionT              Version = {
                                .majorVersion = 1,
                                .minorVersion = 1,
                                .releaseCode  = 'B',
                            };

	Rc = saMsgInitialize (& msgLibraryHandle, & MsgCallbacks, & Version);
	if ( SA_AIS_OK != Rc)
    {
		clprintf ( CL_LOG_SEV_ERROR, "Init failed [0x%X]", Rc);
        assert(0);
    }
}


SaAisErrorT msgOpen(const char* queuename,int bytesPerPriority)
{
	SaAisErrorT             rc;
    SaNameT                 saQueueName;
    SaMsgQueueCreationAttributesT   CreationAttributes;
    
    saQueueName.length=strlen(queuename);
    memcpy(saQueueName.value,queuename,saQueueName.length+1);

    SaMsgQueueOpenFlagsT    OpenFlags = SA_MSG_QUEUE_CREATE;
    
    CreationAttributes.creationFlags = 0;
    for (int i=0;i<SA_MSG_MESSAGE_LOWEST_PRIORITY;i++)
        CreationAttributes.size[i] = bytesPerPriority;
    CreationAttributes.retentionTime = 0;

    rc = saMsgQueueOpen (msgLibraryHandle, &saQueueName, & CreationAttributes, OpenFlags, SA_TIME_MAX, &msgQueueHandle);
    if (SA_AIS_OK != rc)
    {
		clprintf ( CL_LOG_SEV_ERROR, "Msg QueueOpen failed [0x%X]\n\r", rc);
    }
    return rc;
 }

SaAisErrorT msgSend(const char* queuename, void* buffer, int length)
{
    SaAisErrorT             rc;
    SaNameT                 saQueueName;
    SaMsgMessageT           message;

    /* Load the SAF string */
    saQueueName.length=strlen(queuename);
    memcpy(saQueueName.value,queuename,saQueueName.length+1);

    /* Load the SAF message structure */
    message.type = 0;
    message.version.releaseCode = 0;
    message.version.majorVersion=0;
    message.version.minorVersion=0;
    message.senderName = 0;  /* You could put a SaNameT* in here if you wanted to pass a reply queue (for example) */
    message.size = length;
    message.data = buffer;
    message.priority = SA_MSG_MESSAGE_HIGHEST_PRIORITY;
    rc = saMsgMessageSend (msgLibraryHandle, &saQueueName, &message, SA_TIME_MAX);
    if (SA_AIS_OK != rc)
    {
        /* Error 0xC here means that the queue has not yet been created.
         That is, the receiver is not yet listening. */
		clprintf ( CL_LOG_SEV_ERROR, "Msg saMsgMessageSend to queue [%s] failed [0x%X]", saQueueName.value,rc );
    }
    return rc;
}


void* msgReceiverLoop(void * notused)
{
    SaAisErrorT             rc;
    SaNameT                 SenderName;
    char                    Data[1024];
    SaMsgSenderIdT          SenderId;
    SaTimeT                 SendTime;
    
    while(1)
    {
        
        SaMsgMessageT           message = {
            .size       = 1024,
            .senderName = &SenderName,
            .data       = Data,
        };
    
        rc = saMsgMessageGet (msgQueueHandle, &message, & SendTime, & SenderId, SA_TIME_MAX);
        if (SA_AIS_OK != rc)
        {
            clprintf ( CL_LOG_SEV_ERROR, "Msg saMsgMessageGet failed [0x%X]\n\r", rc );
            break;
        }
        
        if (message.senderName->length)
        {
            clprintf ( CL_LOG_SEV_INFO, "Sender Name   : %s\n", message.senderName->value);
        }
        clprintf ( CL_LOG_SEV_INFO, "Received Message  : %s\n", (char *)message.data);
        
    }
    
    rc = saMsgQueueClose (msgQueueHandle);
    if (SA_AIS_OK != rc)
    {
		clprintf ( CL_LOG_SEV_ERROR, "Msg Queue Close failed [0x%X]\n\r", rc );
    }
    return 0;
 
}
