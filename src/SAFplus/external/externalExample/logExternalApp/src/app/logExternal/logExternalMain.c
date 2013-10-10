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
#include <clEoApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocParseConfig.h>
#include <clIocLogicalAddresses.h>
#include "rmdExternalDefs.h"
#include "alarmClockLog.h"
#include "clRmdIpi.h"


#define __LOGICAL_ADDRESS(a) CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_STATIC_LOGICAL_ADDRESS_START + (a))
#define __RPC_SERVER_ADDRESS __LOGICAL_ADDRESS(4)
#define LOCAL_ADDRESS 4
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

int
main(int argc, char **argv)
{

    ClEoConfigT eoConfig =
    {
        CL_OSAL_THREAD_PRI_MEDIUM,    /* EO Thread Priority                       */
        2,                            /* No of EO thread needed                   */
        0,                            /* Required Ioc Port                        */
        (CL_EO_USER_CLIENT_ID_START + 0), 
        CL_EO_USE_THREAD_FOR_APP,     /* Thread Model                             */
        NULL,                         /* Application Initialize Callback          */
        NULL,                         /* Application Terminate Callback           */
        NULL,                         /* Application State Change Callback        */
        NULL                          /* Application Health Check Callback        */
    };    
    int ioc_address_local = LOCAL_ADDRESS;
    extern ClIocConfigT pAllConfig;
    int socket_type;
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

    rc = rmdSeverInit(eoConfig);

    if(rc != CL_OK)
    {
        printf("Info: start rmd server ok\n");
    }
    else
    {
        sleep(10);
        rc = alarmClockLogInitialize();
        if(rc != CL_OK)
        {
             printf("alarmClockLogInitialize FAILED\n");
        }
    }
    do
    {
        sleep(20);
        printf("Info : running ....");
    }while(1);

}
    
