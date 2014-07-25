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

#include <clIocApiExt.h>
#include <clMgtMsg.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
#include "clMgtMsgHandler.hxx"

using namespace std;
using namespace SAFplus;

ClBoolT gIsNodeRepresentative = CL_TRUE;

int main(void)
  {
    ClRcT rc = CL_OK;

    //GAS: initialize expose by a explicit method
    SAFplus::ASP_NODEADDR = 0x1;

    logInitialize();
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    utilsInitialize();

    /*
     * initialize SAFplus libraries
     */
    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc =
        clBufferInitialize(NULL)) != CL_OK)
      {

      }

    clIocLibInitialize(NULL);

    //Mgt server listening
    MgtMsgHandler msghandle;
    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    mgtIocInstance->init(SAFplusI::MGT_IOC_PORT, 25, 10);
    mgtIocInstance->registerHandler(SAFplusI::CL_MGT_MSG_TYPE,&msghandle,NULL);
    mgtIocInstance->Start();

    //Loop forever
    while (1)
      {
        sleep(1);
      }
  }

