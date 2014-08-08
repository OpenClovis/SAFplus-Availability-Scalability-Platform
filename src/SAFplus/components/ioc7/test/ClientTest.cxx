/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
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
#include <iostream>
#include <sstream>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clIocApi.h>
#include "clSafplusMsgServer.hxx"
#include "ClientTest.hxx"

using namespace std;
using namespace SAFplus;

//Auto scanning
#define IOC_PORT 0
#define IOC_PORT_SERVER 65

ClBoolT gIsNodeRepresentative = CL_TRUE;

int main(void)
  {
    ClIocAddressT iocDest;

    ClRcT rc = CL_OK;

    SAFplus::ASP_NODEADDR = 0x1;

    safplusInitialize(SAFplus::LibDep::LOG | SAFplus::LibDep::UTILS | SAFplus::LibDep::OSAL | SAFplus::LibDep::HEAP | SAFplus::LibDep::TIMER | SAFplus::LibDep::BUFFER | SAFplus::LibDep::IOC);
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;  // 2
    iocDest.iocPhyAddress.portId = IOC_PORT_SERVER;
    char helloMsg[] = "Hello world ";

    /*
     * ??? msgClient or safplusMsgServer
     */
    SafplusMsgServer msgClient(IOC_PORT);

    /* Loop receive on loop */
    msgClient.Start();
    int i = 0;
    while (i++ < 50)
      {
        logInfo("CLT","TST","Send msg # %d", i);
        MsgReply *msgReply = msgClient.sendReply(iocDest, (void *) helloMsg, strlen(helloMsg), CL_IOC_PROTO_CTL);
        logInfo("CLT","TST","Received [%s]", (char*) msgReply->buffer);
        sleep(1);
      }
    //TODO: crashed if memory is not enough
    char helloMsg1[2000000] = {0};
    memset(helloMsg1, 'a', sizeof(helloMsg1) - 1);

    logInfo("CLT","TST","Send msg with size # %ld", strlen(helloMsg1));
    while (i++ < 100)
    {
        logInfo("CLT","TST","Send msg # %d", i);
        MsgReply *msgReply = msgClient.sendReply(iocDest, (void *) helloMsg1, strlen(helloMsg1), CL_IOC_PROTO_CTL);
        logInfo("CLT","TST","Received [%s]", (char*) msgReply->buffer);
        sleep(1);
    }  

  }

