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
#include "./alarm_clock_EOServer.h"
#include "rmdExternalDefs.h"
#include "alarm_clock_EOClient.h"
#include "alarm_clock_EOAlarmClockopServer.h"
#include "alarm_clock_EOAlarmClockopClient.h"


#define __LOGICAL_ADDRESS(a) CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_STATIC_LOGICAL_ADDRESS_START + (a))
#define __RPC_SERVER_ADDRESS __LOGICAL_ADDRESS(4)
#define  LOCAL_ADDRESS 4
#define __RPC_SC_SERVER_ADDRESS 1
extern ClRcT clRmdLibInitialize(ClPtrT pConfig);
extern ClRcT clAlarm_clock_EOClientInstall(void);
ClIdlHandleT clockServerHandle = 0;
void SendRmdMsg();
void RmdConnect(void);

ClRcT eoTableInitialize(ClIocLogicalAddressT addr, ClIocPortT port)
{
    ClRcT clrc = CL_OK;
    ClIocTLInfoT tlInfo = {0};
    if ( (clrc = clAlarm_clock_EOClientInstall()) != CL_OK)  // Actually ignore the name, it starts the server
    {
        printf ("RPC server initialization failed. error: 0x%x", clrc);
    }

    if ( (clrc = clAlarm_clock_EOClientTableRegister(port) ) != CL_OK)
    {
        printf ("RPC server client initialization failed. error: 0x%x", clrc);
    }

    tlInfo.physicalAddr.nodeAddress = addr;
    tlInfo.physicalAddr.portId = port;
    tlInfo.haState = CL_IOC_TL_ACTIVE;
    tlInfo.logicalAddr = addr;
    if ( (clrc = clIocTransparencyRegister(&tlInfo) ) != CL_OK )
    {
        printf("RPC server transparency layer init failed with [%#x]", clrc);
    }
    return clrc;
}

#include <clNodeCache.h>

const char* Cap2Str(ClUint32T cap)
{
    if(CL_NODE_CACHE_LEADER_CAPABILITY(cap)) return "leader";
    if (CL_NODE_CACHE_SC_CAPABILITY(cap)) return "controller";
    if (CL_NODE_CACHE_PL_CAPABILITY(cap)) return "payload";
    return "unknown";
}
void nodeCachePoll(void)
{
    unsigned int done = CL_FALSE;
    ClNodeCacheMemberT nodes[64];
    ClUint32T numNodes;
    ClIocNodeAddressT leader;
    ClTimerTimeOutT delay;
    delay.tsSec = 1;
    delay.tsMilliSec = 0;  

    while(1)  
    {
        numNodes = 64;

        printf("Nodes in the cluster:\n");
        clNodeCacheViewGet(nodes,&numNodes);
        for (int i = 0; i<numNodes;i++)
        {
            printf("%30s: Address: %d,  Version: %x  Capability: %s\n",nodes[i].name,nodes[i].address,nodes[i].version,Cap2Str(nodes[i].capability));
        }      
        
        clOsalTaskDelay(delay);        
    }
    
    
}


int main(int argc, char **argv)
{
    int ioc_port = DEF_IOC_PORT; 
    int ioc_address_local = LOCAL_ADDRESS;
    ClRcT rc = CL_OK;
    rc = clExtInitialize(ioc_address_local);
    sleep(7);  // wait for node cache sync
    // This call loops forever printing the current cluster membership state -- for debugging and illustrative purposes: nodeCachePoll();
    
    if (rc != CL_OK)
    {
        printf("Error: failed to Initialize External libraries\n");
        exit(1);
    }
    if(rc != CL_OK)
    {
        printf("Info: failed to start rmd server\n");
        return rc;
    }
    else
    {
        printf("Info: initial Eo Table \n");
        eoTableInitialize(__RPC_SERVER_ADDRESS,ioc_port);
    }
    printf("Info: start rmd server ok\n");
    SendRmdMsg();
}
    
void RmdConnect(void)
{
    ClRcT              rc = CL_OK;
    ClIdlHandleObjT    idlObj        = CL_IDL_HANDLE_INVALID_VALUE;

    ClIdlAddressT      serverCompAddr = {0};
    printf("Create connection to SCComponent\n");
    serverCompAddr.addressType = CL_IDL_ADDRESSTYPE_IOC;
    serverCompAddr.address.iocAddress.iocPhyAddress.nodeAddress = 1;
    /*
     * idl related information.
     */
    idlObj.address = serverCompAddr;
    idlObj.flags   = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout = 5000;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = 3;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = 128;

    if(!clockServerHandle)
    {
        rc = clIdlHandleInitialize(&idlObj, &clockServerHandle);
    }

    if(rc != CL_OK)
    {
        printf("Cannot initialize the RMD server handle");
    }

}

void SendRmdMsg()
{    
    ClRcT clrc;
    while(1)
    { 
        printf("Sending Rmd message to SCcomponent\n");
        acTimeT_4_0_0 time;
        if (clockServerHandle == 0)
            RmdConnect();  
        if ((clrc=GetTimeClientSync_4_0_0(clockServerHandle, &time))==CL_OK)
        {
	    printf("Get Time RPC [ From SCComponent ]: [%02d:%02d:%02d] \n",
		        	    time.hour,time.minute,time.second);
        }
        else
        {
            printf("Get Time RPC [ From SCComponent ] failed %x", clrc);
        }
        ClTimerTimeOutT delay;
        delay.tsSec = 0;
        delay.tsMilliSec = 700;  
    }
}
