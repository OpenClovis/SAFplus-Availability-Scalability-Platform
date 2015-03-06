/*
 * SAFplus also provides another way to get node or component notifications
 * through transport layer (tipc/udp for eg:) using the node address and port id
 *
 * A port id of 0 can be used to receive node notifications and a nodeAddress of
 * broadcast can be used to receive it from all nodes
 * 
 * It uses the clCpmNotificationCallbackInstall api to register for ioc/xport level events
 * using the nodeAddress and portid (iocaddress) as a filter. 
 * In case, the component eo port is known, then one can also register for component
 * notifications with a node address of CL_IOC_BROADCAST_ADDRESS (or all nodes)
 *
 * The event received is of type ClIocNotificationIdT defined in clIocApiExt.h header
 */

#include <clCpmApi.h>

static ClHandleT notificationHandle;
static void notificationCallback(ClIocNotificationIdT event, ClPtrT arg, ClIocAddressT *address);

/*
 * Registers an ioc notification callback with a specific node address and component eo port
 */
ClRcT iocNotificationRegister(ClIocPhysicalAddressT addr)
{
    return clCpmNotificationCallbackInstall(addr, notificationCallback, NULL, &notificationHandle);
}

/*
 * Registers a node notification for all nodes in cluster
 */
ClRcT iocNodeNotificationRegister(void)
{
    ClIocPhysicalAddressT addr = {.nodeAddress = CL_IOC_BROADCAST_ADDRESS,
                                  .portId = 0
    };
    return iocNotificationRegister(addr);
}

static void notificationCallback(ClIocNotificationIdT event, ClPtrT arg, ClIocAddressT *address)
{
    if(event == CL_IOC_NODE_LEAVE_NOTIFICATION)
    {
        clLogNotice("NODE", "EVENT", "Node leave for node [%d]",
                    address->iocPhyAddress.nodeAddress);
        if(address->iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
            /*
             * custom actions
             */
        }
    }
}

