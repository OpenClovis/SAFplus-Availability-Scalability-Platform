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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "clCommon.h"
#include "clCommonErrors.h"
#include "clOsalApi.h"
#include "clOsalErrors.h"
#include "clDebugApi.h"

#define DATA_TAP_MAGIC_NUMBER (0x1234)
#define INST_BUF_SIZE 4096
#define CL_INST_VERSION (0x1)

typedef struct InstrumentedId {
    SaNameT compName;
    SaNameT nodeName;
} InstrumentedId;

static InstrumentedId *myInstId = 0;

static ClOsalMutexIdT id_mutex ;

static int sock = -1;
static struct sockaddr_in  instaddr;

static ClRcT
initSocket()
{
    char               *ipaddr;
    in_addr_t           addr_val;
    char               *port;
    uint16_t            port_num;

    CL_FUNC_ENTER();

    //
    // We'll want to get the address where we'll send instrumentation
    // information out of the environment.  Initialize the sockaddr
    // so after we create the socket we can connect it to the address.
    if ((ipaddr = getenv("CL_LOGGING_ADDR")) == 0)
    {
        clOsalPrintf("No value for CL_LOGGING_ADDR\n");
        CL_FUNC_EXIT();
        return CL_RC(0, CL_ERR_INVALID_PARAMETER);
    }
    if ((port = getenv("CL_LOGGING_PORT")) == 0)
    {
        clOsalPrintf("No value for CL_LOGGING_PORT\n");
        CL_FUNC_EXIT();
        return CL_RC(0, CL_ERR_INVALID_PARAMETER);
    }
    addr_val = inet_addr(ipaddr);
    port_num = atoi(port);
    memset(&instaddr, 0, sizeof instaddr);
    instaddr.sin_port = htons(port_num);
    instaddr.sin_addr.s_addr = addr_val;

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        clOsalPrintf("Failed to open socket");
        return CL_OSAL_ERR_OS_ERROR;
    }
//    if (connect(sock, &instaddr, sizeof addr) != 0)
//    {
//        clOsalPrintf("Failed to connect socket to %s/%s\n", ipaddr, port);
//        close(sock);
//        sock = -1;
//        CL_FUNC_EXIT();
//        return CL_OSAL_ERR_OS_ERROR;
//    }
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
clInstInitId()
{
    ClRcT           rc          = CL_OK;
    InstrumentedId *tmpPtr      = 0;

    CL_FUNC_ENTER();
    // check whether the id has been initialized
    if (myInstId != 0)
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }

    if (sock == -1)
    {
        rc = initSocket();
        if (rc != CL_OK)
        {
            clOsalPrintf("failed to initialize instrumentation socket\n");
            return rc;
        }
    }


    // Nope, not initialized, so try to initialize it here.
    // First, try to lock a mutex so that only one thread
    // will be creating the id.
    // Then, try to get our component name, our local node name
    // and then allocate the id and set it from the component
    // and local node name.
    // Finally, unlock and delete the mutex.
    if (!id_mutex)
    {
        if ((rc = clOsalMutexCreateAndLock(&id_mutex)) != CL_OK)
        {
            clOsalPrintf("Failed [0x%x] to create and lock mutex\n", rc);
            CL_FUNC_EXIT();
            return rc;
        }
    }
    else
    {
        // Some other thread must have come through here and set
        // the mutex before us.  We're done.
        CL_FUNC_EXIT();
        return CL_OK;
    }

    if ((tmpPtr = clHeapCalloc(1, sizeof (InstrumentedId))) == 0)
    {
        clOsalPrintf("failed to allocate InstrumentedID\n");
        rc = CL_RC(0, CL_ERR_NO_MEMORY);
        goto breakdown;
    }
    memset(&tmpPtr->compName, 0, sizeof tmpPtr->compName);
    memset(&tmpPtr->nodeName, 0, sizeof tmpPtr->nodeName);
    rc = CL_OK;
    myInstId = tmpPtr;
    tmpPtr = 0;

breakdown:
    clOsalMutexUnlock(id_mutex);
    clOsalMutexDelete(id_mutex);
    id_mutex = 0;

    CL_FUNC_EXIT();
    return rc;
}

ClRcT
clInstSetIdentity(const SaNameT *nodeName, const SaNameT *compName)
{
    CL_FUNC_ENTER();
    if (myInstId == 0)
    {
        return CL_RC(0, CL_ERR_NOT_INITIALIZED);
    }
    if (myInstId->nodeName.length == 0)
    {
        memcpy(&myInstId->nodeName, nodeName, sizeof *nodeName);
    }
    if (myInstId->compName.length == 0)
    {
        memcpy(&myInstId->compName, compName, sizeof *compName);
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
clInstSendMessage(int msgType, const char *msg)
{
    char        msgBuf[INST_BUF_SIZE];
    ClRcT       rc = CL_OK;
    int         space;
    char       *ptr = msgBuf;

    CL_FUNC_ENTER();
    if ((rc = clInstInitId()) != CL_OK)
    {
        clOsalPrintf("Failed [0x%x] to initialize Instrumentation id\n", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    //
    // Fill in the msgBuf: msgType,nodeName,compName,msg
    // Check that we have room for the message, formatting it
    // as we go.  Finally, send it over the socket which should
    // already been connected in clInstInitId
    sprintf(msgBuf, "%d", CL_INST_VERSION);
    space = sizeof msgBuf - 1;
    sprintf(msgBuf+1, "%d,", msgType);
    space = sizeof msgBuf - 2;
    ptr += strlen(ptr);
    space = (sizeof msgBuf) - (ptr - msgBuf);
    if (myInstId->nodeName.length == 0)
    {
        strcpy(ptr, "unknown_node,");
    }
    else
    {
        if ((myInstId->nodeName.length + 1) >= space)
        {
            clOsalPrintf("nodeName (%s) too long for remaining space (%d)\n",
                        myInstId->nodeName.value, space);
        }
        else
        {
            strcpy(ptr, myInstId->nodeName.value);
            strcat(ptr, ",");
        }
    }
    ptr += strlen(ptr);
    space = (sizeof msgBuf) - (ptr - msgBuf);

    if (myInstId->compName.length == 0)
    {
        strcat(ptr, "unknown_component,");
    }
    else
    {
        if ((myInstId->compName.length + 1) >= space)
        {
            clOsalPrintf("compName (%s) too long for remaining space (%d)\n",
                        myInstId->compName.value, space);
            return CL_RC(0, CL_ERR_BUFFER_OVERRUN);
        }
        else
        {
            strcpy(ptr, myInstId->compName.value);
            strcat(ptr, ",");
        }
    }
    ptr += strlen(ptr);
    space = (sizeof msgBuf) - (ptr - msgBuf);

    // check that there's enough room for the message in our buffer
    // and if there is then copy it in.
    if (strlen(msg) >= space)
    {
        strncpy(ptr, msg, space - 4);   // room for "..."
        ptr += (space - 4);
        strcpy(ptr, "...");
    }
    else
    {
        strcpy(ptr, msg);
    }
    ptr += strlen(ptr);
    space = (sizeof msgBuf) - (ptr - msgBuf);

    if (sendto(sock, msgBuf, ptr - msgBuf, 0, &instaddr, sizeof instaddr) == -1)
    {
        clOsalPrintf("Failed to send message, sock = %d, errno = %d\n", sock, errno);
        return CL_OSAL_ERR_OS_ERROR;
    }
    CL_FUNC_EXIT();
    return rc;
}

static struct DataTapStatics {
    struct sockaddr_in  tapaddr;
    struct DataTapPackage {
        ClUint8T  version;
        ClUint32T magic;
        ClUint32T flags;
        ClUint32T data;
    } package;
    int inited;
    int broked;
    int sock;
} gDataTapObj;

ClRcT
clDataTapInit(ClUint32T initFlags, ClUint32T appNum)
{
    char *env = 0;
    char buf[200];
    in_addr_t           addr_val;

    if (gDataTapObj.inited)
    {
        return CL_ERR_INITIALIZED;
    }
    // Should we have some consistency checking on the initFlags?
    // Probably, but I don't know yet what checking to do.
    gDataTapObj.package.version = CL_INST_VERSION;
    gDataTapObj.package.magic = htonl(DATA_TAP_MAGIC_NUMBER);
    gDataTapObj.package.flags = htonl(initFlags);
    gDataTapObj.package.data = 0;
    gDataTapObj.broked = 0;
    gDataTapObj.inited = 1;
    memset(&gDataTapObj.tapaddr, 0, sizeof gDataTapObj.tapaddr);

    // Get the port number from the environment
    sprintf(buf, "CSA%d_DEST_PORT", appNum);
    env = getenv(buf);
    if (env == 0)
    {
        clOsalPrintf("NO value for %s environment variable\n", buf);
        gDataTapObj.broked = 1;
        return CL_ERR_INVALID_PARAMETER;
    }
    gDataTapObj.tapaddr.sin_port = htons(atoi(env));
    printf("sin_port = %u (%u)\n", gDataTapObj.tapaddr.sin_port, (unsigned)atoi(env));

    // Now get the IP address fromthe environment
    sprintf(buf, "CSA%d_DEST_ADDR", appNum);
    env = getenv(buf);
    if (env == 0)
    {
        clOsalPrintf("NO value for %s environment variable\n", buf);
        gDataTapObj.broked = 1;
        return CL_ERR_INVALID_PARAMETER;
    }

    addr_val = inet_addr(env);
    gDataTapObj.tapaddr.sin_addr.s_addr = addr_val;

    gDataTapObj.sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (gDataTapObj.sock == -1)
    {
        clOsalPrintf("Failed to open socket");
        return CL_OSAL_ERR_OS_ERROR;
    }

    return CL_OK;
}

ClRcT clDataTapSend(ClUint32T data)
{
    struct DataTapPackage package = gDataTapObj.package;
    package.data = htonl(data);
    
    if (!gDataTapObj.inited)
    {
        clOsalPrintf("Data Tap uninitialized\n");
        return CL_ERR_INITIALIZED;
    }
    if (gDataTapObj.broked)
    {
        return CL_ERR_INVALID_PARAMETER;
    }
    if (sendto(gDataTapObj.sock, &package, sizeof package, 0, &gDataTapObj.tapaddr, sizeof gDataTapObj.tapaddr) < 0)
    {
        clOsalPrintf("Failed to send message, sock = %d, errno = %d\n", sock, errno);
        return CL_OSAL_ERR_OS_ERROR;
    }
    return CL_OK;
}
