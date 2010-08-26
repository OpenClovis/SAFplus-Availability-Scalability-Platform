#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <saAmf.h>
#include <saMsg.h>

#define DEFAULT_MSG_TEST_QUEUES (100)
#define DEFAULT_MSG_TEST_DELAY  (1000000) /* in usecs*/
#define COMPS_PER_NODE (0x5)
#define TOTAL_NODES (0x5)
#define TOTAL_COMPS (COMPS_PER_NODE * TOTAL_NODES)
#define __ALIGN(v, a)    ( ( (v)+(a)-1)/(a) * (a) )
#define MSG_QUEUES_PER_COMP (__ALIGN(numMsgQueues, TOTAL_COMPS)/TOTAL_COMPS)
/*
 * Make a queue matrix.
 */
#define NR_ROWS (TOTAL_NODES)
#define NR_COLS (COMPS_PER_NODE)
#define MSG_QUEUES_PER_ROW (MSG_QUEUES_PER_COMP * NR_COLS)
#define MSG_QUEUES_PER_NODE MSG_QUEUES_PER_ROW
#define MSG_TEST_QUEUE_PREFIX "msgTestQueue"
/*
 * The test msg is allocated on the stack. So be careful with the value below
 */
#define MSG_TEST_DATA_SIZE_MAX (1<<16)
#define MSG_TEST_DATA_SIZE_MASK (MSG_TEST_DATA_SIZE_MAX-1)
#define MSG_TEST_PRIORITY_QUEUE_SIZE (50<<20U) /*virtually a large priority queue*/

#define testLog(sev, area, context, ...) do {                       \
    const char *pArea = area;                                       \
    const char *pContext = context;                                 \
    if(!pArea) pArea = "Unknown";                                   \
    if(!pContext) pContext = "Unknown";                             \
    doLog(sev, pArea, pContext, __FILE__, __LINE__, __VA_ARGS__);   \
}while(0)

#define testLogError(area, context, ...) testLog("ERROR", area, context, __VA_ARGS__)
#define testLogNotice(area, context, ...) testLog("NOTICE", area, context, __VA_ARGS__)

static int numMsgQueues = DEFAULT_MSG_TEST_QUEUES;
static SaMsgQueueHandleT *msgQueues;
static SaMsgHandleT *msgHandles;
static SaMsgHandleT gMsgHandle;
static int queueMatrix[NR_ROWS+1][NR_COLS+1];
static int myRow, myCol;
static SaNameT myCompName;

static pthread_mutex_t msgTestMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  msgTestCond = PTHREAD_COND_INITIALIZER;
static int msgTestDone = 0;
static int msgTestQueueDone = 0;
static int msgTestDispatchers;
static int msgTestDelay = DEFAULT_MSG_TEST_DELAY; /* in usecs*/

static void doLog(const char *sev, const char *area, const char *context, const char *file, int line, const char *fmt, ...)
{
    char buf[600];
    va_list ptr;
    int bytes = 0;
    time_t t = 0;
    struct tm tm = {0};
    time(&t);
    localtime_r((const time_t*)&t, &tm);
    bytes = snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s.%s.%s.%s) ", tm.tm_hour, tm.tm_min, tm.tm_sec, 
                     (char*)myCompName.value, area, context, sev);
    va_start(ptr, fmt);
    vsnprintf(buf+bytes, sizeof(buf)-bytes, fmt, ptr);
    va_end(ptr);
    fprintf(stdout, "%s\n", buf);
    fflush(stdout);
}

static void *msgTestDispatch(void *arg)
{
    SaMsgHandleT msgHandle = *(SaMsgHandleT*)arg;
    do 
    {
        saMsgDispatch(msgHandle, SA_DISPATCH_BLOCKING);
    } while(!msgTestDone);
    pthread_mutex_lock(&msgTestMutex);
    if(!--msgTestDispatchers)
        pthread_cond_signal(&msgTestCond);
    pthread_mutex_unlock(&msgTestMutex);
    return NULL;
}

static void msgTestDispatchTask(SaMsgHandleT *msgHandle)
{
    pthread_t tid;
    pthread_attr_t attr;
    assert(pthread_attr_init(&attr)==0);
    assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)==0);
    assert(pthread_create(&tid, &attr, msgTestDispatch, (void*)msgHandle) == 0);
}

static void msgTestQueueReceivedCallback(SaMsgQueueHandleT queueHandle)
{
    SaMsgSenderIdT senderId = 0;
    SaNameT queue = {0};
    SaVersionT version = {'B', 1, 1};
    char recvData[MSG_TEST_DATA_SIZE_MAX] = {0};
    SaMsgMessageT   getMessage1 = {0, version, sizeof(recvData), &queue, recvData, SA_MSG_MESSAGE_HIGHEST_PRIORITY};
    SaAisErrorT rc ;

    testLogNotice("MSG", "RCV", "Received message for queue handle [%#llx]", queueHandle);
    rc = saMsgMessageGet(queueHandle, &getMessage1, NULL, &senderId, SA_TIME_ONE_MINUTE);
    if(rc != SA_AIS_OK)
    {
        testLogError("MSG", "RCV", "Message received failed with [%#x]", rc);
        if(rc != SA_AIS_ERR_TIMEOUT)
            assert(0);
    }
    else
    {
#if 0 /*uncomment to enable sendrecv*/
        char replyData[2] = "X";
#endif
        char verifyData[MSG_TEST_DATA_SIZE_MAX] = {0};
        void *data ;
        SaSizeT size;
        data = getMessage1.data;
        size = getMessage1.size;
        assert(data);
        strncpy(verifyData, "hello", (int)getMessage1.size-1);
        if(!memcmp(data, verifyData, (int)size))
            testLogNotice("MSG", "RCV", "Message get success from destination [%.*s] with size [%d]", 
                    queue.length, queue.value, (int)size);
        else
        {
            testLogError("MSG", "RCV", "Message get from destination [%.*s] of len [%lld] with data [%.*s]", 
                    queue.length, (char*)queue.value, size, (int)size, (char*)data);
            assert(0);
        }
#if 0 /*uncomment to enable sendrecv*/
        getMessage1.data = replyData;
        getMessage1.size = 1;
        rc = saMsgMessageReply(gMsgHandle, &getMessage1, &senderId, SA_TIME_MAX);
        if(rc != SA_AIS_OK)
        {
            testLogError("MSG", "REP-SEND", "Message reply failed with [%#x] for destination [%.*s]", rc,
                         queue.length, queue.value);
        }
#endif
    }
}

/*
 * Call it from main or app initialize. after saAmfComponentNameGet.
 */
int msgTestInitialize(const SaNameT *compName)
{
    int err = -1;
    register int i;
    register int j;
    char *p = NULL;
    char *queues = getenv("MSG_TEST_QUEUES");
    char *queueDelay = getenv("MSG_TEST_DELAY"); /* in milliseconds*/
    int row = 0;
    int col = 0;
    SaMsgCallbacksT callback = { .saMsgMessageReceivedCallback = msgTestQueueReceivedCallback };
    SaVersionT msgVersion = {'B', 0x2, 0x1};
    SaAisErrorT rc = SA_AIS_OK;
    if(queues)
    {
        char *p = NULL;
        numMsgQueues = (int)strtol(queues, &p, 10);
        if(*p)
            numMsgQueues = DEFAULT_MSG_TEST_QUEUES;
        if(numMsgQueues <= 0)
            numMsgQueues = DEFAULT_MSG_TEST_QUEUES;
    }
    if(queueDelay)
    {
        char *p = NULL;
        msgTestDelay = (int)strtol(queueDelay, &p, 10);
        if(*p)
            msgTestDelay = DEFAULT_MSG_TEST_DELAY;
        else
            msgTestDelay *= 1000; /*convert to usecs*/

        if(msgTestDelay <= 0)
            msgTestDelay = DEFAULT_MSG_TEST_DELAY;
    }
    memcpy(&myCompName, compName, sizeof(myCompName));
    myCompName.value[myCompName.length]= 0;
    p = (char*)myCompName.value;
    while(*p && !isdigit(*p)) ++p;
    if(!*p) 
        goto out;
    row = (int)strtol(p, &p, 10);
    if(!*p) 
        goto out;
    while(*p && !isdigit(*p)) ++p;
    if(!*p)
        goto out;
    col = (int)strtol(p, &p, 10);
    if(*p)
        goto out;
    testLogNotice("MSG", "TST", "Initializing msg test with comp [%.*s], row [%d], col [%d]",
                  myCompName.length, (char*)myCompName.value, row, col);
    assert(row <= NR_ROWS);
    assert(col < NR_COLS);
    myRow = row;
    myCol = col;
    /*
     * uninitialize first row which is goin to be unused.
     */
    for(i = 0; i <= NR_ROWS; ++i)
    {
        for(j = 0; j < NR_COLS; ++j)
        {
            if(!i)
                queueMatrix[i][j] = -1;
            else
                queueMatrix[i][j] = (i-1)*MSG_QUEUES_PER_NODE + j*MSG_QUEUES_PER_COMP;
        }
    }
    /*
     * Our own queue set
     */
    msgHandles = calloc(MSG_QUEUES_PER_COMP, sizeof(*msgHandles));
    msgQueues = calloc(MSG_QUEUES_PER_COMP, sizeof(*msgQueues));
    assert(msgHandles && msgQueues);
    rc = saMsgInitialize(&gMsgHandle, NULL, &msgVersion);
    if(rc != SA_AIS_OK)
    {
        testLogError("MSG", "TST", "Msg initialize failed with [%#x]", rc);
        goto out;
    }
    /*
     * Create a queue dispatcher thread for each queue. thats owned.
     */
    for(i = 0; i < MSG_QUEUES_PER_COMP; ++i)
    {
        rc = saMsgInitialize(msgHandles+i, &callback, &msgVersion);
        if(rc != SA_AIS_OK)
        {
            testLogError("MSG", "TST", "Msg initialize failed for queue index [%d] with [%#x]",
                    i, rc);
            for(j = i; --j >= 0; )
            {
                rc = saMsgFinalize(msgHandles[j]);
                if(rc != SA_AIS_OK)
                {
                    testLogError("MSG", "TST", "Msg finalize failed for queue index [%d] with [%#x]",
                                 j, rc);
                }
            }
            goto out;
        }
    }

    /*
     * Once all msg handles are successfully initialized, we kick off the per queue dispatcher thread.
     */
    pthread_mutex_lock(&msgTestMutex);
    for(i = 0; i < MSG_QUEUES_PER_COMP; ++i)
        msgTestDispatchTask(msgHandles+i);

    msgTestDispatchers = MSG_QUEUES_PER_COMP; 
    pthread_mutex_unlock(&msgTestMutex);

    err = 0;

    out:
    return err;
}

static void msgTestQueueDelete(int queueIndex, int numQueues)
{
    register int i;
    SaNameT queueName = {0};
    SaAisErrorT rc = SA_AIS_OK;
    for(i = 0; i < numQueues; ++i)
    {
        if(msgQueues[i])
        {
            snprintf((char*)queueName.value, sizeof(queueName.value), "%s_%d",
                     MSG_TEST_QUEUE_PREFIX, queueIndex+i);
            queueName.length = strlen((const char*)queueName.value);
            rc = saMsgQueueUnlink(gMsgHandle, &queueName);
            if(rc != SA_AIS_OK)
            {
                testLogError("MSG", "TST", "Msg queue unlink for queue [%.*s] returned with [%#x]",
                             queueName.length, (char*)queueName.value, rc);
            }
            rc = saMsgQueueClose(msgQueues[i]);
            msgQueues[i] = 0;
            if(rc != SA_AIS_OK)
            {
                testLogError("MSG", "TST", "Msg queue close for queue [%.*s] returned with [%#x]",
                             queueName.length, (char*)queueName.value, rc);
            }
        }
    }
    for(i = 0; i < numQueues; ++i)
    {
        if(msgHandles[i])
        {
            saMsgFinalize(msgHandles[i]);
            msgHandles[i] = 0;
        }
    }
    if(gMsgHandle)
    {
        saMsgFinalize(gMsgHandle);
        gMsgHandle = 0;
    }
}

static SaAisErrorT msgTestQueueCreate(int queueIndex, int numQueues)
{
    SaNameT msgQueueName = {0};
    SaMsgQueueCreationAttributesT attributes = {0};
    SaMsgQueueStatusT msgQueueStatus = {0};
    SaMsgQueueOpenFlagsT flags = SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK;
    SaAisErrorT rc = SA_AIS_OK;
    register int i;
    attributes.size[SA_MSG_MESSAGE_HIGHEST_PRIORITY] = MSG_TEST_PRIORITY_QUEUE_SIZE;
    /* attributes.creationFlags = SA_MSG_QUEUE_PERSISTENT;*/
    for(i = 0; i < numQueues; ++i)
    {
        SaMsgQueueHandleT queueHandle = 0;
        snprintf((char*)msgQueueName.value, sizeof(msgQueueName.value), "%s_%d", 
                 MSG_TEST_QUEUE_PREFIX, queueIndex + i);
        msgQueueName.length = strlen((const char*)msgQueueName.value);
        rc = saMsgQueueOpen(msgHandles[i], &msgQueueName, &attributes, flags, SA_TIME_MAX,
                            &queueHandle);
        testLogNotice("MSG", "TST", "Msg queue open for [%.*s] returned with [%#x]",
                      msgQueueName.length, (char*)msgQueueName.value, rc);
        if(rc != SA_AIS_OK && rc != SA_AIS_ERR_EXIST)
            return rc;
        if(rc == SA_AIS_OK)
            msgQueues[i] = queueHandle;
        sleep(1);
        /*  
         * Confirm that status get on the queue succeeds.
         */
        rc = saMsgQueueStatusGet(msgHandles[i], &msgQueueName, &msgQueueStatus);
        assert(rc == SA_AIS_OK);
    }
    return rc;
}

static SaAisErrorT msgTestQueueSendReceive(int queueIndex, int numQueues)
{
    SaVersionT version = {'B', 2, 1};
    char messageData[MSG_TEST_DATA_SIZE_MAX] = "hello";
    SaMsgHandleT msgHandle = gMsgHandle;
    SaNameT queue = {0};
    SaAisErrorT rc = 0;
    SaMsgMessageT   sendMessage = {0, version, sizeof(messageData), &queue, messageData, SA_MSG_MESSAGE_HIGHEST_PRIORITY};
#if 0 /*uncomment to enable sendreceive*/
    char getData1[2] = {0};
    char verifyData1[2] = "X";
    SaMsgMessageT   getMessage1 = {0, version, 1, &queue, getData1, SA_MSG_MESSAGE_HIGHEST_PRIORITY};
#endif
    SaNameT destQueueName = {0};
    SaMsgQueueStatusT destQueueStatus = {0};
    int dataLen,i;

    testLogNotice("MSG", "SND", "Msg queue test send for queue index [%d], num queues [%d]",
                  queueIndex, numQueues);
    for(i = 0; i < numQueues; ++i)
    {
        snprintf((char*)destQueueName.value, sizeof(destQueueName.value), "%s_%d", MSG_TEST_QUEUE_PREFIX, 
                 queueIndex + i);
        destQueueName.length = strlen((const char*)destQueueName.value);
        rc = saMsgQueueStatusGet(msgHandle, &destQueueName, &destQueueStatus);
        /*
         * Return if anyone in this group is not ready yet.
         */
        if(rc != SA_AIS_OK)
            return rc;
    }

    dataLen = strlen(messageData);
    for(i = 0; i < numQueues; ++i)
    {
        SaMsgMessageT *msg = &sendMessage;
        int dataSize = (random() & MSG_TEST_DATA_SIZE_MASK) + dataLen + 10;
        snprintf((char*)queue.value, sizeof(queue.value), "%s_%d", MSG_TEST_QUEUE_PREFIX, queueIndex+i);
        queue.length = strlen((const char*)queue.value);
        if(dataSize > MSG_TEST_DATA_SIZE_MASK)
            dataSize = MSG_TEST_DATA_SIZE_MASK; /* hit max limit*/
        msg->size = dataSize;
        testLogNotice("MSG", "SND", "Sending and receiving message to destination [%s] of data size [%d]", 
                      (char*)queue.value, dataSize);
#if 0 /*uncomment to enable sendrecv*/
        rc = saMsgMessageSendReceive(msgHandle,
                                     &queue,
                                     msg,
                                     &getMessage1,
                                     NULL,
                                     SA_TIME_ONE_MINUTE);

        if(rc != SA_AIS_OK)
        {
            testLogError("MSG", "SND", "Message sendrecv for destination [%s] failed with [%#x]",
                         (char*)queue.value, rc);
            /*
             * if(rc != SA_AIS_ERR_NOT_EXIST) assert(0);
             */
        }
        else
        {
            assert((void*)getMessage1.data == (void*)getData1);
            if(getMessage1.size == 1 && !memcmp(getData1, verifyData1, 1))
                testLogNotice("MSG", "SND", "Message sendrecv success to destination [%.*s]", 
                              queue.length, (char*)queue.value);
            else
            {
                testLogError("MSG", "SND", "Message sendrecv received [%d] bytes of data [%.*s]",
                             (int)getMessage1.size, (int)getMessage1.size, getData1);
                assert(0);
            }
        }
#else
        rc = saMsgMessageSend(msgHandle,
                              &queue,
                              msg,
                              SA_TIME_ONE_MINUTE);
        if(rc != SA_AIS_OK)
        {
            testLogError("MSG", "SND", "Message send for destination [%s] failed with [%#x]",
                         (char*)queue.value, rc);
            /*
             * if(rc != SA_AIS_ERR_NOT_EXIST) assert(0);
             */
        }
#endif
    }
    return SA_AIS_OK;
}

static void *msgTestTask(void *unused)
{
    SaAisErrorT rc;
    register int i;
    register int j;
    rc = msgTestQueueCreate(queueMatrix[myRow][myCol], MSG_QUEUES_PER_COMP);
    if(rc != SA_AIS_OK)
        goto out;
    /*
     * Now we are all set to blast the sends. to all the queues in the queue matrix.
     */
    while(!msgTestQueueDone && !msgTestDone)
    {
        for(i = 1; i <= NR_ROWS; ++i)
        {
            for(j = 0; j < NR_COLS; ++j)
            {
                /*
                 * Skip self sends. 
                 * Uncomment j == myCol to enable skipping only self sends rather than node sends.
                 */
                if(i == myRow /*&& j == myCol*/)
                    continue;
                if(queueMatrix[i][j] < 0) 
                    continue;
                if(msgTestQueueSendReceive(queueMatrix[i][j], MSG_QUEUES_PER_COMP) != SA_AIS_OK)
                    sleep(1);
            }
            usleep(msgTestDelay); /*delay a bit after a burst send*/
        }
        usleep(msgTestDelay); /*delay a bit after a burst send on all*/
    }

    out:
    pthread_mutex_lock(&msgTestMutex);
    --msgTestDispatchers;
    pthread_cond_signal(&msgTestCond);
    pthread_mutex_unlock(&msgTestMutex);
    return NULL;
}

/*
 * Start the msg tests for this comp. 
 * Call it from csiset callback on getting active
 */
int msgTestStart(void)
{
    int err;
    pthread_attr_t attr;
    pthread_t tid;
    assert(pthread_attr_init(&attr)==0);
    assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
    msgTestDone = 0;
    msgTestQueueDone = 0;
    pthread_mutex_lock(&msgTestMutex);
    err = pthread_create(&tid, &attr, msgTestTask, NULL);
    if(err == 0)
    {
        ++msgTestDispatchers;
    }
    pthread_mutex_unlock(&msgTestMutex);
    return err;
}

/*
 * Call it during app finalize/terminate to cleanup msg queues
 */
int msgTestStop(void)
{
    pthread_mutex_lock(&msgTestMutex);
    if(!msgTestDone)
    {
        msgTestDone = 1;
        msgTestQueueDone = 1;
        while(msgTestDispatchers > 0)
        {
            pthread_cond_wait(&msgTestCond, &msgTestMutex);
        }
        pthread_mutex_unlock(&msgTestMutex);
        msgTestQueueDelete(queueMatrix[myRow][myCol], MSG_QUEUES_PER_COMP);
        pthread_mutex_lock(&msgTestMutex);
    }
    pthread_mutex_unlock(&msgTestMutex);
    return 0;
}

/*
 * Call it from csiset during quiesced/quiescing to stop the queue sendrecv tests.
 */
int msgTestQueueStop(void)
{
    pthread_mutex_lock(&msgTestMutex);
    if(msgTestDispatchers > 0 && !msgTestQueueDone)
    {
        msgTestQueueDone = 1;
        pthread_cond_wait(&msgTestCond, &msgTestMutex);
    }
    pthread_mutex_unlock(&msgTestMutex);
    return 0;
}
