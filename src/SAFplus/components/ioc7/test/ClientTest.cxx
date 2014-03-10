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
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clIocApi.h>
#include "clSafplusMsgServer.hxx"

using namespace std;
using namespace SAFplus;

//Auto scanning
#define IOC_PORT 0
#define IOC_PORT_SERVER 65

int
main(void)
{
    ClIocAddressT iocDest;

    ClRcT rc = CL_OK;

    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK)
    {
        cout<<"ERROR:"<<CL_GET_ERROR_CODE(rc)<<endl;
        exit(0);
    }

    // Port communication
    SafplusMsgServer msgClient(IOC_PORT);

    iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    iocDest.iocPhyAddress.portId = IOC_PORT_SERVER;
    char helloMsg[] = "Hello world!";

    msgClient.SendMsg(iocDest, helloMsg, sizeof(helloMsg), CL_IOC_PROTO_CTL);
}

