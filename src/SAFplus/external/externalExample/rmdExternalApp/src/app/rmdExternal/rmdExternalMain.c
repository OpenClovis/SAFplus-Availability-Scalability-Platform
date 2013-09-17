#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clHeapApi.h>
#ifdef USE_EO
#include <clEoApi.h>
#endif
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocParseConfig.h>
#include "rmdExternalDefs.h"
#include <clIocLogicalAddresses.h>
#include "alarm_clock_EOServer.h"
#include "alarm_clock_EOClient.h"
#include "alarm_clock_EOAlarmClockopServer.h"

#define __LOGICAL_ADDRESS(a) CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_STATIC_LOGICAL_ADDRESS_START + (a))
#define __RPC_SERVER_ADDRESS __LOGICAL_ADDRESS(4)

extern ClRcT clRmdLibInitialize(ClPtrT pConfig);
extern ClBoolT gIsNodeRepresentative;
static ClIocConfigT *gpClIocConfig;
extern ClInt32T clAspLocalId;
extern ClHeapConfigT *pHeapConfigUser;
static ClHeapConfigT heapConfig;
static ClEoMemConfigT memConfig;
static ClRcT clMemInitialize(void)
{
    ClRcT rc;

    if((rc = clMemStatsInitialize(&memConfig)) != CL_OK)
    {
        return rc;
    }
    pHeapConfigUser =  &heapConfig;
    if((rc = clHeapInit()) != CL_OK)
    {
        return rc;
    }
    return CL_OK;
}

ClRcT
Initialize ( ClInt32T ioc_address_local )
{
    ClRcT rc = CL_OK;
    
	clAspLocalId = ioc_address_local;

    rc = clIocParseConfig(NULL, &gpClIocConfig);
    if(rc != CL_OK)
    {
        clOsalPrintf("Error : Failed to parse clIocConfig.xml file. error code = 0x%x\n",rc);
        exit(1);
    }

    if ((rc = clOsalInitialize(NULL)) != CL_OK)
    {
        printf("Error: OSAL initialization failed\n");
        return rc;
    }
    if ((rc = clMemInitialize()) != CL_OK)
    {
        printf("Error: Heap initialization failed\n");
        return rc;
    }
    if ((rc = clTimerInitialize(NULL)) != CL_OK)
    {
        printf("Error: Timer initialization failed\n");
        return rc;
    }
    if ((rc = clBufferInitialize(NULL)) != CL_OK)
    {
        printf("Error: Buffer initialization failed\n");
        return rc;
    }
    return rc;
}

ClRcT RmdServerTableInitialize(ClIocLogicalAddressT addr, ClIocPortT port)
{
    ClRcT clrc = CL_OK;
    ClIocTLInfoT tlInfo = {0};
    printf("Enter RmdInitialize\n");
    if ( (clrc = clAlarm_clock_EOClientInstall()) != CL_OK)  // Actually ignore the name, it starts the server
    {
        printf ("RPC server initialization failed. error: 0x%x", clrc);
    }

    if ( (clrc = clAlarm_clock_EOClientTableRegister(port) ) != CL_OK)
    {
        printf ("RPC server client initialization failed. error: 0x%x", clrc);
    }

    tlInfo.physicalAddr.nodeAddress = addr;
    printf ("RPC server nodeAddress: %d \n", tlInfo.physicalAddr.nodeAddress);
    tlInfo.physicalAddr.portId = port;
    tlInfo.haState = CL_IOC_TL_ACTIVE;
    tlInfo.logicalAddr = addr;
    printf ("RPC server logicalAddr: %d \n", (int)addr);
    printf ("RPC server portId: %d \n", (int)port);
    if ( (clrc = clIocTransparencyRegister(&tlInfo) ) != CL_OK )
    {
        printf("RPC server transparency layer init failed with [%#x]", clrc);
    }
    printf("finish Initialize");
    return clrc;
}

int
main(int argc, char **argv)
{
    int ioc_port = DEF_IOC_PORT; /* IOC port number, default is DEF_IOC_PORT */
    int ioc_address_local = 4;
    extern ClIocConfigT pAllConfig;
    printf("start rmd external \n");
    int socket_type;
    char a=10;
    printf("value of aA %c", a);
    printf("value of aA %d", (int)a);
    ClRcT rc = CL_OK;
    socket_type = CL_IOC_RELIABLE_MESSAGING;
    heapConfig.mode = CL_HEAP_NATIVE_MODE;
    memConfig.memLimit = 0;
    rc = Initialize( ioc_address_local );
    if (rc != CL_OK)
    {
        printf("Error: failed to Initialize ASP libraries\n");
        exit(1);
    }
    pAllConfig.iocConfigInfo.isNodeRepresentative = CL_TRUE;
    gIsNodeRepresentative = CL_TRUE;
    if ((rc = clIocLibInitialize(NULL)) != CL_OK)
    {
        printf("Error: IOC initialization failed with rc = 0x%x\n", rc);
        exit(1);
    }
    if ((rc = clRmdLibInitialize(NULL)) != CL_OK)
    {
        printf("Error: RMD initialization failed with rc = 0x%x\n", rc);
        exit(1);
    }
    printf("Info: start rmd server\n");
    rc = rmdSeverInit();
    if(rc != CL_OK)
    {
        printf("Info: start rmd server ok\n");
    }
    else
    {
        RmdServerTableInitialize(__RPC_SERVER_ADDRESS,ioc_port);
    }
  
    printf("Info: start rmd server ok\n");

    do
    {
        sleep(20);
        clLogNotice("CALLBACK", "TASKS","running ....");
    }while(1);

}
    
