#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clTimeServer.h>

#ifdef VXWORKS_BUILD
#define CL_SOCKLEN_T int
#else
#define CL_SOCKLEN_T socklen_t
#endif

typedef struct ClTimeServerResponse
{
    ClUint8T  version;
    ClUint32T secLow;
    ClUint32T secHigh;
    ClUint32T usecLow;
    ClUint32T usecHigh;
}ClTimeServerResponseT;

static ClBoolT gClTimeServerRunning = CL_FALSE;
static ClOsalMutexT gClTimeServerMutex;
static ClOsalCondT  gClTimeServerCond;
static void timeServerResponseMarshall(ClTimeValT *t, ClUint8T *buffer, ClUint32T *bufLen)
{
    ClUint32T len = 0;
    ClUint32T secLow = htonl(t->tvSec & 0xffffffffU);
    ClUint32T secHigh = htonl( (t->tvSec >> 32) & 0xffffffffU);
    ClUint32T usecLow = htonl (t->tvUsec & 0xffffffffU);
    ClUint32T usecHigh = htonl( (t->tvUsec >> 32) & 0xffffffffU);

    buffer[len++] = CL_TIME_SERVER_VERSION & 0xff;
    memcpy(&buffer[len], &secLow, sizeof(ClUint32T));
    len += sizeof(ClUint32T);

    memcpy(&buffer[len], &secHigh, sizeof(ClUint32T));
    len += sizeof(ClUint32T);

    memcpy(&buffer[len], &usecLow, sizeof(ClUint32T));
    len += sizeof(ClUint32T);
    
    memcpy(&buffer[len], &usecHigh, sizeof(ClUint32T));
    len += sizeof(ClUint32T);

    *bufLen = len;
}

static ClRcT timeServerResponseUnmarshall(ClUint8T *buffer, ClUint32T bufLen, ClTimeValT *t)
{
    ClTimeServerResponseT response = {0};

    response.version = buffer[0] & 0xff;

    if(response.version != CL_TIME_SERVER_VERSION)
        return CL_ERR_VERSION_MISMATCH;

    /*
     * Version 1 - expected response.
     */
    if(bufLen != (sizeof(ClUint8T) + 4*sizeof(ClUint32T)))
        return CL_ERR_NO_SPACE;

    bufLen = 1;
    memcpy(&response.secLow, &buffer[bufLen], sizeof(ClUint32T));
    bufLen += sizeof(ClUint32T);

    memcpy(&response.secHigh, &buffer[bufLen], sizeof(ClUint32T));
    bufLen += sizeof(ClUint32T);

    memcpy(&response.usecLow, &buffer[bufLen], sizeof(ClUint32T));
    bufLen += sizeof(ClUint32T);

    memcpy(&response.usecHigh, &buffer[bufLen], sizeof(ClUint32T));
    bufLen += sizeof(ClUint32T);

    response.secLow = ntohl(response.secLow);
    response.secHigh = ntohl(response.secHigh);
    response.usecLow = ntohl(response.usecLow);
    response.usecHigh = ntohl(response.usecHigh);
    
    t->tvSec = response.secHigh;
    t->tvUsec = response.usecHigh;
    t->tvSec = ( (t->tvSec << 32) | (response.secLow) );
    t->tvUsec = ( (t->tvUsec << 32) | (response.usecLow) );
    return CL_OK;
}

ClRcT clTimeServerGet(const ClCharT *host, ClTimeValT *t)
{
    struct sockaddr_in serverAddr;
    struct sockaddr_in cliAddr;
    struct hostent *h;
    ClInt32T fd = -1;
    ClInt32T bytes = 0;
    ClUint8T req = 1;
    ClUint8T reqBuf[2] = {(ClUint8T)CL_TIME_SERVER_VERSION, req};
    ClUint8T responseBuf[sizeof(ClTimeServerResponseT)*2] = {0};
    ClRcT rc = CL_ERR_UNSPECIFIED;
    CL_SOCKLEN_T addrsize = sizeof(struct sockaddr_in);
    if(clParseEnvBoolean("CL_TIME_SERVER") == CL_FALSE) return CL_ERR_NOT_INITIALIZED;

    if(!host) host = getenv("CL_TIME_SERVER_IP");

    if(!host || !t) return CL_ERR_INVALID_PARAMETER;
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&cliAddr, 0, sizeof(cliAddr));
    h = gethostbyname(host);
    if(!h) return -1;
    memcpy(&serverAddr.sin_addr.s_addr, h->h_addr, h->h_length);
    serverAddr.sin_family = h->h_addrtype;
    serverAddr.sin_port = htons(CL_TIME_SERVER_PORT);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    CL_ASSERT(fd >= 0);
    
    cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliAddr.sin_port = htons(0);
    cliAddr.sin_family = AF_INET;

    if(bind(fd, (struct sockaddr*)&cliAddr, sizeof(cliAddr)) < 0)
    {
        perror("bind:");
        goto out;
    }

    bytes = sendto(fd, (char *)reqBuf, sizeof(reqBuf), 0, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in));
    if(bytes != sizeof(reqBuf))
    {
        perror("sendto:");
        clLogError("TIME", "CLIENT", "Send to time server returned [%s]",
                   strerror(errno));
        goto out;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));

    retry:
    bytes = recvfrom(fd, (char*)responseBuf, sizeof(responseBuf), 0,(struct sockaddr *) &serverAddr, &addrsize);
    if(bytes <= 0 )
    {
        if (errno == EINTR) goto retry;
        else
        {
            clLogError("TIME", "CLIENT", "Recv from time server returned [%s]",
                       strerror(errno));
            goto out;
        }
    }

    rc = timeServerResponseUnmarshall(responseBuf, bytes, t);
    if(rc != CL_OK)
    {
        clLogError("TIME", "CLIENT", "Time server response unmarshall for [%d] bytes returned [%#x]", 
                   bytes, rc);
        goto out;
    }

    clLogInfo("TIME", "APP", "Server returned time [%lld.%lld] usecs",
              t->tvSec, t->tvUsec);

    out:
    if(fd >= 0)
        close(fd);
    return rc;
}

static void *timeServer(void *arg)
{
    struct sockaddr_in serverAddr;
    ClInt32T fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0 )
    {
        perror("socket:");
        clOsalMutexLock(&gClTimeServerMutex);
        clOsalCondSignal(&gClTimeServerCond);
        clOsalMutexUnlock(&gClTimeServerMutex);
        return NULL;
    }
    serverAddr.sin_addr.s_addr =  htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CL_TIME_SERVER_PORT);
    if(bind(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind:");
        close(fd);
        clOsalMutexLock(&gClTimeServerMutex);
        clOsalCondSignal(&gClTimeServerCond);
        clOsalMutexUnlock(&gClTimeServerMutex);
        return NULL;
    }
    
    clOsalMutexLock(&gClTimeServerMutex);
    clOsalCondSignal(&gClTimeServerCond);
    clOsalMutexUnlock(&gClTimeServerMutex);

    while(gClTimeServerRunning)
    {
        ClTimeT sTime = 0;
        ClUint8T requestBuf[2];
        ClInt32T bytes;
        struct sockaddr_in client_addr;
        CL_SOCKLEN_T addrsize = 0;
        
        retry:
        memset(&client_addr, 0, sizeof(client_addr));
        addrsize =  sizeof(struct sockaddr_in);
        bytes = recvfrom(fd, (char*)requestBuf, sizeof(requestBuf), 0, (struct sockaddr *)&client_addr, &addrsize);
        if(bytes <= 0)
        {
            if(errno == EINTR) goto retry;
            else if(bytes == 0) goto retry;
            else  break;
        }

        if(requestBuf[0] != CL_TIME_SERVER_VERSION)
        {
            clLogWarning("TIME", "SERVER", "Time server got request with version [%d]. Ignoring now",
                         requestBuf[0]);
            continue;
        }

        if(requestBuf[1] != 1) 
        {
            clLogCritical("TIME", "SERVER", "Invalid request [%#x]", requestBuf[1]);
            continue;
        }
        sTime = clOsalStopWatchTimeGet();
        if(sTime)
        {
            ClUint8T responseBuf[sizeof(ClTimeServerResponseT)*2] = {0};
            ClUint32T responseLen = 0;
            ClTimeValT cTV = {0};
            cTV.tvSec = 0;
            cTV.tvUsec = sTime;
            timeServerResponseMarshall(&cTV, responseBuf, &responseLen);
            bytes = sendto(fd, (char *)responseBuf, responseLen, 0, (struct sockaddr *) &client_addr, addrsize);
            if(bytes != (ClInt32T) responseLen)
            {
                perror("Sendto:");
                clLogCritical("TIME", "SERVER", "Time server error responding to client "\
                              "[%s - %d]", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
        }
    }
    clLogInfo("TIME", "SERVER", "Exiting");
    return NULL;
}

void clTimeServerInitialize(void)
{
    if((clParseEnvBoolean("CL_TIME_SERVER") == CL_TRUE) && !gClTimeServerRunning)
    {
        ClRcT rc = CL_OK;
        ClTimerTimeOutT delay = {0};
        rc = clOsalMutexInit(&gClTimeServerMutex);
        CL_ASSERT(rc == CL_OK);
        clOsalCondInit(&gClTimeServerCond);
        CL_ASSERT(rc == CL_OK);
        gClTimeServerRunning = CL_TRUE;
        clOsalMutexLock(&gClTimeServerMutex);
        rc = clOsalTaskCreateDetached("TIME-SERVER", CL_OSAL_SCHED_OTHER, 0, 0, timeServer, NULL);
        CL_ASSERT(rc == CL_OK);
        clOsalCondWait(&gClTimeServerCond, &gClTimeServerMutex, delay);
        clOsalMutexUnlock(&gClTimeServerMutex);
    }
    return;
}

void clTimeServerFinalize(void)
{
    gClTimeServerRunning = CL_FALSE;
}
