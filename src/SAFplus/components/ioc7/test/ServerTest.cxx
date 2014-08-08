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
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include <clIocProtocols.h>
#include "clSafplusMsgServer.hxx"
#include "MsgHandlerProtocols.hxx"

using namespace SAFplus;

#define IOC_PORT_SERVER 65

ClBoolT gIsNodeRepresentative = CL_TRUE;

int
main(void)
{
    MsgHandlerProtocols handler;

    ClRcT rc = CL_OK;

    SAFplus::ASP_NODEADDR = 0x2;

    safplusInitialize(SAFplus::LibDep::LOG | SAFplus::LibDep::UTILS | SAFplus::LibDep::OSAL | SAFplus::LibDep::HEAP | SAFplus::LibDep::TIMER | SAFplus::LibDep::BUFFER | SAFplus::LibDep::IOC);

    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    //Msg server listening
    SAFplus::SafplusMsgServer safplusMsgServer(IOC_PORT_SERVER, 10, 10);

    // Handle IOC Heartbeat protocol
    safplusMsgServer.RegisterHandler(CL_IOC_PROTO_HB, &handler, NULL);

    // Handle IOC Control Protocol
    safplusMsgServer.RegisterHandler(CL_IOC_PROTO_CTL, &handler, NULL);

    safplusMsgServer.Start();

    // Loop forever
    while(1);

}

