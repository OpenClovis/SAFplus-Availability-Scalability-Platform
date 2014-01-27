/*
 * AMF test case hooker.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <signal.h>

#include <clCpmApi.h>
#include <clSafUtils.h>
#include <saAmf.h>
#include <clEoIpi.h>
#include <clLogApi.h>
#include <clParserApi.h>
#include <clOsalApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmfTestHooks.h>

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  "AMF", "TST",                         \
                                  __VA_ARGS__)

#define CL_AMF_TEST_HOOK(sym) clAmfTest##sym##Callback
#define CL_TEST_HA_STATE_LEN   1
#define CL_TEST_MSG_LENGTH_LEN 2
#define CL_TEST_INFO_TYPE_LEN  1    
#define CL_TEST_MAX_NAME_LEN 256

typedef enum
{
    CL_TEST_COMP_NAME_TYPE=1,
    CL_TEST_SI_NAME_TYPE=2,
    CL_TEST_CSI_NAME_TYPE=3,
    CL_TEST_HA_STATE_TYPE=4,
}ClTstInfoTypeT;

static SaAmfCallbacksT saveAmfCallbacks;

static ClAmsEntityHandleT gAmsMgmtHandle;

static int gSockFd = -1;

static ClBoolT gClTestSetupInitialized = CL_FALSE;
    
/*Functions*/
static ClRcT clTestSetupClientForMsgSend(void);

static ClRcT clTestSendInfo(SaNameT compName,
                            SaNameT siName,
                            SaNameT csiName,
                            SaAmfHAStateT haState);
    
static ClRcT clTestSIEntityNameGet(SaNameT csiName, SaNameT* siName);


/*
 * Read server configuration
 */
static ClRcT
clTestReadSvrConfig(ClCharT **pServerIp, ClUint32T *pPort)
{
    ClCharT *port = NULL;

    /* Default values */
    *pPort = 2727; 
    *pServerIp=(ClCharT*) "localhost";
    
    if(NULL == (port = getenv("CL_AMF_TEST_SERVER_PORT")))
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to get the Server's port.");
        return CL_ERR_NULL_POINTER;
    }

    *pPort = atol(port);

    if(NULL == (*pServerIp = getenv("CL_AMF_TEST_SERVER_IP")))
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to get the Server's ip address.");
        return CL_ERR_NULL_POINTER;
    }

    clprintf(CL_LOG_SEV_TRACE, "Connecting to the Test Server IP : %s, Port : %u", *pServerIp, *pPort);

    return CL_OK;
}


/**
 * Creates the socket and connects
 * to the test server. 
 */
static ClRcT
clTestSetupClientForMsgSend(void)
{
    int sockfd = 0;
    int retCode = 0;
    ClRcT rc = 0;    
    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;
    ClCharT *serverIp = NULL;
    ClUint32T serverPort = 0;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        clprintf(CL_LOG_SEV_CRITICAL, "ERROR opening socket, system error [%d]", errno);
        return CL_ERR_LIBRARY;
    }
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    rc = clTestReadSvrConfig(&serverIp, &serverPort);
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_CRITICAL, "Server configuration read failed, rc [0x%x]", rc);
        goto setup_failed;
    }
    else
    {
        server = gethostbyname((char *)serverIp);
        if(NULL == server)
        {
            perror("Failed to get host");            
            clprintf(CL_LOG_SEV_CRITICAL, "ERROR, no such host, system error [%d]", errno);
            goto setup_failed;
        }
        serv_addr.sin_port = htons(serverPort);
    }
    
    serv_addr.sin_family = AF_INET;
    memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr,
          server->h_length);

    clprintf(CL_LOG_SEV_INFO, " =============================================================");
    clprintf(CL_LOG_SEV_INFO, " Connecting to server ip:%s, port:%d ...",
             (char *)serverIp , serverPort);
    clprintf(CL_LOG_SEV_INFO, " =============================================================");

    retCode = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(retCode < 0)
    {
        perror("Failed to connect to server");
        clprintf(CL_LOG_SEV_CRITICAL, "ERROR connecting to server, system error [%d]", errno);
        goto setup_failed;
    }

    gSockFd = sockfd;

    return CL_OK;

setup_failed:
    clprintf(CL_LOG_SEV_ERROR, "Client setup for Msg send failed.");
    return CL_ERR_LIBRARY;
}

/*Send message over socket*/
static int
clTestSend(ClUint32T nbToBeWritten, ClCharT *buf)
{

    if( (ClUint32T)send(gSockFd, buf, nbToBeWritten, 0) != nbToBeWritten )
    {
        perror("");
        clprintf(CL_LOG_SEV_ERROR, "Error in send()");
        return -1;
    }
    return 0;
}

#if 0
/*Send message over socket*/
static int
clTestSendOld(ClUint32T nbToBeWritten, ClCharT *buf)
{

    int nbWritten = 0;
    int maxFdP = 1;
    int err = 0;
    struct timeval timeout = {0};

    fd_set writeSet;

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; /*10 mili-second */

    for(;;)
    {	
        FD_ZERO(&writeSet);	
        FD_SET(gSockFd, &writeSet);
        maxFdP = gSockFd + 1;

        err = select(maxFdP, NULL, &writeSet,  NULL, &timeout);
        if(err == 0 )  
        {
            continue;
        }
        else
        if(err < 0 && (errno == EBADF || errno == EPIPE))
        {
            perror("");
            clprintf(CL_LOG_SEV_ERROR, "Error in select() on socket fd");
            return -1;
        }

        if(FD_ISSET(gSockFd, &writeSet))
        {/* Socket is writable */
            if( (nbWritten = send(gSockFd, buf, nbToBeWritten, 0))
                == nbToBeWritten )
            {
                return 0;
            }
            else
            {
                perror("");
                clprintf(CL_LOG_SEV_ERROR, "Error in send()");
                return -1;
            }
        }
    }

    return 0;
}
#endif

static ClRcT
               // coverity[pass_by_value]
clTestSendInfo(SaNameT compName,
               // coverity[pass_by_value]
               SaNameT siName,
               // coverity[pass_by_value]
               SaNameT csiName,
               SaAmfHAStateT haState)
{
    ClCharT amfHaState = (ClCharT) haState;
    ClCharT infoType = 0;
    ClCharT *msg = NULL;
    ClCharT *buff = NULL;    
    ClUint32T msgLen = 0;
    ClUint16T length = 0;

    /*Strip trailing \0 from length*/
    if('\0' == csiName.value[csiName.length - 1])
    {
        csiName.length -= 1;
    }
    
    msgLen = (4 * (CL_TEST_INFO_TYPE_LEN + CL_TEST_MSG_LENGTH_LEN))
             + compName.length + siName.length + csiName.length
             + CL_TEST_HA_STATE_LEN ;
    
             
    buff = (ClCharT*) clHeapAllocate(msgLen);
    if(NULL == buff)
    {
        clprintf(CL_LOG_SEV_CRITICAL, "Memory allocation failure.");
        return CL_ERR_NO_MEMORY;
    }
    /*Storing for future reference*/
    msg = buff;
    
    /*Preparing msg for compName*/
    /********************************************/
    infoType = (ClCharT)CL_TEST_COMP_NAME_TYPE;
    memcpy(buff, &infoType, CL_TEST_INFO_TYPE_LEN);
    buff += CL_TEST_INFO_TYPE_LEN;

    length = (ClUint16T) compName.length;
    length = htons(length);    
    memcpy(buff, &length, CL_TEST_MSG_LENGTH_LEN);
    buff += CL_TEST_MSG_LENGTH_LEN;

    memcpy(buff, &compName.value, compName.length);
    buff += compName.length;
    /********************************************/
    
    /*Preparing msg for siName*/
    /********************************************/
    infoType = (ClCharT)CL_TEST_SI_NAME_TYPE;
    memcpy(buff, &infoType, CL_TEST_INFO_TYPE_LEN);
    buff += CL_TEST_INFO_TYPE_LEN;

    length = (ClUint16T) siName.length;
    length = htons(length);
    memcpy(buff, &length, CL_TEST_MSG_LENGTH_LEN);
    buff += CL_TEST_MSG_LENGTH_LEN;
    
    memcpy(buff, &siName.value, siName.length);
    buff += siName.length;
    /********************************************/

    /*Preparing msg for csiName*/
    /********************************************/
    infoType = (ClCharT)CL_TEST_CSI_NAME_TYPE;
    memcpy(buff, &infoType, CL_TEST_INFO_TYPE_LEN);
    buff += CL_TEST_INFO_TYPE_LEN;

    length = (ClUint16T) csiName.length;
    length = htons(length);
    memcpy(buff, &length, CL_TEST_MSG_LENGTH_LEN);
    buff += CL_TEST_MSG_LENGTH_LEN;
    
    memcpy(buff, &csiName.value, csiName.length);
    buff += csiName.length;
    /********************************************/    
    

    /*Preparing msg for haState*/
    /********************************************/
    infoType = (ClCharT)CL_TEST_HA_STATE_TYPE;
    memcpy(buff, &infoType, CL_TEST_INFO_TYPE_LEN);
    buff += CL_TEST_INFO_TYPE_LEN;

    length = htons(1);
    memcpy(buff, &length, CL_TEST_MSG_LENGTH_LEN);
    buff += CL_TEST_MSG_LENGTH_LEN;

    memcpy(buff, &amfHaState, CL_TEST_HA_STATE_LEN);
    buff += CL_TEST_HA_STATE_LEN;
    /********************************************/    

    if(-1 == clTestSend(msgLen, msg))
    {
        clHeapFree(msg);
        clprintf(CL_LOG_SEV_CRITICAL, "Info send failed.");
        return CL_ERR_LIBRARY;
    }
    clHeapFree(msg);
    
    clprintf(CL_LOG_SEV_NOTICE, "Sent compName[%.*s] siName[%.*s] csiName[%.*s] haState[%hhd]",
             compName.length, compName.value,
             siName.length, siName.value,
             csiName.length, csiName.value,
             amfHaState);
    
    return CL_OK;
}

                                   // coverity[pass_by_value]
static ClRcT clTestSIEntityNameGet(SaNameT csiName, 
                                   SaNameT* siName)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity;
    ClAmsCSIConfigT* pCsiConfig = NULL;

    memset(&entity, 0, sizeof(entity));
    entity.type = CL_AMS_ENTITY_TYPE_CSI;

    saNameSet(&entity.name, (ClCharT *) csiName.value);
    ++entity.name.length; /* add for the extra null byte*/

    rc = clAmsMgmtEntityGetConfig(gAmsMgmtHandle, &entity, (ClAmsEntityConfigT **) &pCsiConfig);
    if (CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to get the SI for the CSI [%s]", csiName.value);
        return rc;
    }
    
    strncpy((ClCharT*) (*siName).value, (const ClCharT*)pCsiConfig->parentSI.entity.name.value, CL_MAX_NAME_LENGTH-1);
    (*siName).length = (pCsiConfig->parentSI.entity.name.length -1);

    clHeapFree(pCsiConfig);

    return CL_OK;
}

static ClRcT clTestSendCSISet(SaInvocationT       invocation,
                              const SaNameT       *compName,
                              SaAmfHAStateT       haState,
                              SaAmfCSIDescriptorT *csiDescriptor)
{
    ClRcT rc = CL_OK;
    SaNameT siName = {0};

    rc = clTestSIEntityNameGet(csiDescriptor->csiName, &siName);
    if (rc != CL_OK)
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to get the SI name. rc [0x%x]", rc);
        return rc;
    }

    clprintf(CL_LOG_SEV_TRACE, "SI name : [%s]", siName.value);

    rc = clTestSendInfo(*compName,
                        siName,
                        csiDescriptor->csiName,
                        haState);
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Error while sending info to server.");
    }

    return rc;
}

static ClRcT clTestSendCSIRemove(SaInvocationT  invocation,
                                 const SaNameT  *compName,
                                 const SaNameT  *csiName,
                                 SaAmfCSIFlagsT csiFlags)
{
    ClRcT rc = CL_OK;
    SaNameT siName = {0};

    rc = clTestSIEntityNameGet(*csiName, &siName);
    if (rc != CL_OK)
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to get the SI name. rc [0x%x]", rc);
        return rc;
    }

    clprintf(CL_LOG_SEV_TRACE, "SI name : [%s]", siName.value);

     rc = clTestSendInfo(*compName,
                         siName,
                         *csiName,
                         (SaAmfHAStateT)0);
     if(CL_OK != rc)
     {
         clprintf(CL_LOG_SEV_ERROR, "Error while sending info to server.");
     }
     return rc;
}       

static ClRcT clTestSetupInitialize(void)
{
    ClRcT rc = CL_OK;
    ClVersionT          clversion = {'B', 0x1, 0x1 };

    if(gClTestSetupInitialized) return CL_OK;

    rc = clTestSetupClientForMsgSend();
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_CRITICAL, "Failed to setup client for info send.");
        goto errorexit;
    }
   
    rc = clAmsMgmtInitialize(&gAmsMgmtHandle, NULL, &clversion);
    if (CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Failed to initialize the AMS Management.");
        goto errorexit;
    }

    /*Ignore SIGPIPE signal*/
    signal(SIGPIPE,SIG_IGN);

    gClTestSetupInitialized = CL_TRUE;

    errorexit:
    return rc;
}

static ClRcT clTestSetupFinalize(void)
{
    if(!gClTestSetupInitialized) return CL_OK;
    gClTestSetupInitialized = CL_FALSE;
    if(gSockFd > 0) close(gSockFd);
    if(gAmsMgmtHandle)
        clAmsMgmtFinalize(gAmsMgmtHandle);
    return CL_OK;
}

static void CL_AMF_TEST_HOOK(CompHealthcheck)(SaInvocationT invocation,
                                              const SaNameT *compName,
                                              SaAmfHealthcheckKeyT *healthcheckKey)
{
    if(saveAmfCallbacks.saAmfHealthcheckCallback)
        saveAmfCallbacks.saAmfHealthcheckCallback(invocation, compName, healthcheckKey);
}

static void CL_AMF_TEST_HOOK(CompTerminate)(SaInvocationT invocation,
                                            const SaNameT *compName)
{
    if(saveAmfCallbacks.saAmfComponentTerminateCallback)
        saveAmfCallbacks.saAmfComponentTerminateCallback(invocation, compName);
    clTestSetupFinalize();
}

static void CL_AMF_TEST_HOOK(CompCSISet)(SaInvocationT invocation,
                                         const SaNameT *compName,
                                         SaAmfHAStateT haState,
                                         // coverity[pass_by_value]
                                         SaAmfCSIDescriptorT csiDescriptor)
{
    if(saveAmfCallbacks.saAmfCSISetCallback)
    {
        clTestSendCSISet(invocation, compName, haState, &csiDescriptor);
        saveAmfCallbacks.saAmfCSISetCallback(invocation, compName, haState, csiDescriptor);
    }
}

static void CL_AMF_TEST_HOOK(CompCSIRemove)(SaInvocationT invocation,
                                            const SaNameT *compName,
                                            const SaNameT *csiName,
                                            SaAmfCSIFlagsT csiFlags)
{
    if(saveAmfCallbacks.saAmfCSIRemoveCallback)
    {
        clTestSendCSIRemove(invocation, compName, csiName, csiFlags);
        saveAmfCallbacks.saAmfCSIRemoveCallback(invocation, compName, csiName, csiFlags);
    }
}

static void CL_AMF_TEST_HOOK(ProxiedCompInstantiate)(SaInvocationT invocation,
                                                      const SaNameT *proxiedCompName)
{
    if(saveAmfCallbacks.saAmfProxiedComponentInstantiateCallback)
        saveAmfCallbacks.saAmfProxiedComponentInstantiateCallback(invocation,
                                                                  proxiedCompName);
}

static void CL_AMF_TEST_HOOK(ProxiedCompCleanup)(SaInvocationT invocation,
                                                 const SaNameT *proxiedCompName)
{
    if(saveAmfCallbacks.saAmfProxiedComponentCleanupCallback)
        saveAmfCallbacks.saAmfProxiedComponentCleanupCallback(invocation,
                                                              proxiedCompName);
}

static void CL_AMF_TEST_HOOK(CompPGTrack)(const SaNameT *csiName,
                                          SaAmfProtectionGroupNotificationBufferT *notificationBuffer,
                                          SaUint32T numberOfMembers,
                                          SaAisErrorT error)
{
    if(saveAmfCallbacks.saAmfProtectionGroupTrackCallback)
        saveAmfCallbacks.saAmfProtectionGroupTrackCallback(csiName, notificationBuffer,
                                                           numberOfMembers, error);
}

SaAisErrorT saAmfInitializeTestHook(SaAmfHandleT *amfHandle,
                                    const SaAmfCallbacksT *amfCallbacks,
                                    SaVersionT *version)
{
    ClRcT rc;
    SaAmfCallbacksT testAmfCallbacks = {0};

    rc = clASPInitialize();
    if(CL_OK != rc)
    {
        return clClovisToSafError(rc);
    }
    rc = clTestSetupInitialize();
    if(rc != CL_OK) 
    {
        clprintf(CL_LOG_SEV_CRITICAL, "AMF test setup initialize failed with [%#x]. "
                 "Disabling AMF test hooks", rc);
        return clClovisToSafError(rc);
    }

    if(amfCallbacks)
    {
        memcpy(&saveAmfCallbacks, amfCallbacks, sizeof(saveAmfCallbacks));
        testAmfCallbacks.saAmfHealthcheckCallback = CL_AMF_TEST_HOOK(CompHealthcheck);
        testAmfCallbacks.saAmfComponentTerminateCallback = CL_AMF_TEST_HOOK(CompTerminate);
        testAmfCallbacks.saAmfCSISetCallback = CL_AMF_TEST_HOOK(CompCSISet);
        testAmfCallbacks.saAmfCSIRemoveCallback = CL_AMF_TEST_HOOK(CompCSIRemove);
        testAmfCallbacks.saAmfProtectionGroupTrackCallback = CL_AMF_TEST_HOOK(CompPGTrack);
        testAmfCallbacks.saAmfProxiedComponentInstantiateCallback = CL_AMF_TEST_HOOK(ProxiedCompInstantiate);
        testAmfCallbacks.saAmfProxiedComponentCleanupCallback = CL_AMF_TEST_HOOK(ProxiedCompCleanup);
        amfCallbacks = (const SaAmfCallbacksT*)&testAmfCallbacks;
    }

    rc = clCpmClientInitialize((ClCpmHandleT *)amfHandle,
                               (ClCpmCallbacksT *)amfCallbacks,
                               (ClVersionT *)version);
    
    if(rc == CL_OK)
    {
        ClSelectionObjectT dispatchFd = 0;
        rc = clCpmSelectionObjectGet(*amfHandle, &dispatchFd);
    }

    return clClovisToSafError(rc);
}

