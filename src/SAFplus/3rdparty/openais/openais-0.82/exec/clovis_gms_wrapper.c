/*
 *
 * This is the interface between clovis code for Group Membership Service
 * And the totem. This wrapper is implemented as an add-on service on top
 * of totem protocol provided by openais. This service stays parellel to
 * any other services. Below diagram explains the high level architecture
 * for openais.
 *
 *      ----------------------------------------------------
 *      |     AMF and more "CLIENT" service libraries      |
 *      ----------------------------------------------------
 *                         | Client-Server |                   
 *                         |      IPC      |
 *                         |   Interface   |
 *      ----------------------------------------------------
 *      |                  openais Executive               |
 *      |                                                  |
 *      |      +---------+ +--------+ +---------+          |
 *      |      | Object  | |  AIS   | | Service |          |
 *      |      | Datbase | | Config | | Handler |          |
 *      |      | Service | | Parser | | Manager |          |
 *      |      +---------+ +--------+ +---------+          |
 *      |                                                  |
 *      |  +-------+ +-------+ +-------+        +=======+  |
 *      |  |  AMF  | |  EVS  | |  CLM  |       ||  GMS  || |
 *      |  |Service| |Service| |Service| * * * ||Wrapper|| |
 *      |  +-------+ +-------+ +-------+       ||Service|| |
 *      |                                       +=======+  |
 *      |                  +---------+                     |
 *      |                  |  Sync   |                     |
 *      |                  | Service |                     |
 *      |                  +---------+                     |
 *      |                  +---------+                     |
 *      |                  |   VSF   |                     |
 *      |                  | Service |                     |
 *      |                  +---------+                     |
 *      |  +--------------------------------+ +--------+   |
 *      |  |                 Totem          | | Timers |   |
 *      |  |                 Stack          | |  API   |   |
 *      |  +--------------------------------+ +--------+   |
 *      |                 +-----------+                    |
 *      |                 |   Poll    |                    |
 *      |                 | Interface |                    |
 *      |                 +-----------+                    |
 *      ----------------------------------------------------
 *
 * As shown above, the GMS Wrapper service sits as a parellel service
 * to any other services provided by openais. Please see http://openais.org
 * for more details on the architecture. Also refer to clovis_modifications.txt
 * file under this folder to see the list of changes done in openais
 * to implement this service
 * 
 */
/* ------------------------------------------------------------------------
 * System header files below
 *------------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#if defined(OPENAIS_LINUX)
#include <sys/sysinfo.h>
#endif
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
#include <sys/sysctl.h>
#endif
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ------------------------------------------------------------------------
 * Openais specific header files below
 *------------------------------------------------------------------------*/
#include "totem.h"
#include "../include/mar_gen.h"
#include "../include/list.h"
#include "../include/queue.h"
#include "../lcr/lcr_comp.h"
#include "totempg.h"
#include "main.h"
#include "ipc.h"
#include "mempool.h"
#include "service.h"
#include "logsys.h"


/* ------------------------------------------------------------------------
 * Clovis specific header files below
 *------------------------------------------------------------------------*/
#include "../../../../components/include/clCommon.h"
#include "../../../../components/debug/include/clDebugApi.h"
#include "../../../../components/gms/include/clClmApi.h"
#include "../../../../components/gms/common/clGmsCommon.h"
#include "../../../../components/cnt/include/clCntApi.h"
#include "../../../../components/gms/server/clGms.h"
#include "../../../../components/gms/server/clGmsView.h"
#include "../../../../components/osal/include/clOsalApi.h"
#include "../../../../components/gms/server/clGmsEngine.h"
#include "../../../../components/gms/server/clGmsMsg.h"
#include "../../../../components/gms/include/clGmsErrors.h"
#include "../../../../components/utils/include/clVersionApi.h"

/* ------------------------------------------------------------------------
 * Macros
 *------------------------------------------------------------------------*/
#define SERVICE_ID_EXTRACT(a) ((a) & 0x0000FFFF)
LOGSYS_DECLARE_SUBSYS ("GMS", LOG_INFO);

/* ------------------------------------------------------------------------
 * Clovis data structures as extern declarations
 *------------------------------------------------------------------------*/
extern  ClUint32T           gmsOpenAisEnable;
extern  ClGmsNodeT          gmsGlobalInfo;
extern  void                _clGmsGetThisNodeInfo(ClGmsClusterMemberT  *);
extern  ClRcT               _clGmsCallClusterMemberEjectCallBack( ClGmsMemberEjectReasonT );
extern  ClGmsCsSectionT     joinCs;
extern  ClBoolT             ringVersionCheckPassed;
        ClVersionT          ringVersion;
        ClGmsNodeAddressT   myAddress;
extern  struct totem_ip_address my_ip;

/* ------------------------------------------------------------------------
 * Openais specific Function declarations
 *------------------------------------------------------------------------*/

/* NOTE: The functions and data structures with "lib" string
 * attached, are basically intended to provide a client library
 * service for this registered service. In our case, however, we
 * dont provide any client library through openais, and hence
 * dont need to give those functions here */


/* gms_confchg_fn will be called on every configuration change in the totem */
static void gms_confchg_fn (
        enum totem_configuration_type configuration_type,
        unsigned int *member_list, int member_list_entries,
        unsigned int *left_list, int left_list_entries,
        unsigned int *joined_list, int joined_list_entries,
        struct memb_ring_id *ring_id);


/* Start the synchronization. This is when this node will start
 * the formation of a singleton ring where only this is the member
 * of the ring. This happens soon after the boot up. Then on joining
 * and leaving of other nodes in the cluster would result in formation
 * of new rings each time. */
static void gms_sync_init (void);


/* sync_process is called to process synchronization messages */
static int gms_sync_process (void);


/* sync_activate is called to activate the current service synchronization. */
static void gms_sync_activate (void);


/* sync_abort is called to abort the current service synchronization. */
static void gms_sync_abort (void);


/* exec_init_fn is a function used to initialize the executive service.
 * This is only called once. */
static int gms_exec_init_fn (struct objdb_iface_ver0 *objdb);


/* exec_dump_fn is called during the finalize of openais */
static void gms_exec_dump_fn (void);


/* Upon reception of any data message for this service, ais will
 * invoke the register message handler function given below */
static void gms_exec_message_handler (
	void *message,
	unsigned int nodeid);


/* This is the endian conversion function called when a new
 * message is received for this service. */
static void gms_exec_message_endian_convert (void* message);


/* ------------------------------------------------------------------------
 * Openais specific data structure definitions below:
 * These are used for implementing GMS functionality
 *------------------------------------------------------------------------*/

/* Below message types define type of messages "THIS" service
 * would receive and interpret.*/
enum gms_message_req_types
{
    MESSAGE_REQ_EXEC_GMS_NODEJOIN = 0
};


/* Message strucutre which will contain the GMS related values
 * to be passed through totem
 */
struct req_exec_gms_nodejoin {
    mar_req_header_t        header              __attribute__((aligned(8)));
    ClGmsMessageTypeT       gmsMessageType      __attribute__((aligned(8)));
    ClGmsGroupIdT           gmsGroupId          __attribute__((aligned(8)));
    ClGmsClusterMemberT     gmsClusterNode      __attribute__((aligned(8)));
    ClGmsGroupMemberT       gmsGroupNode        __attribute__((aligned(8)));
    ClGmsGroupInfoT         groupData           __attribute__((aligned(8)));
    ClGmsMemberEjectReasonT ejectReason         __attribute__((aligned(8)));
    ClUint64T               contextHandle       __attribute__((aligned(8)));
    ClUint32T               syncNoOfGroups      __attribute__((aligned(8)));
    ClUint32T               syncNoOfMembers     __attribute__((aligned(8)));
};

static int gms_nodejoin_send (void);

void clGmsWrapperUpdateMyIP();
/* ------------------------------------------------------------------------
 * Openais specific data structure definitions below:
 * These are of no interest to us!!
 *------------------------------------------------------------------------*/

#if 0
/* I think we dont need to set any private data. Remove it if the things
 * are working fine by commenting it out. */
/* Private data using to tranmit between the library client and this service */
struct gms_pd 
{
	unsigned char track_flags;
	int tracking_enabled;
	struct list_head list;
	void *conn;
};
#endif

static struct openais_exec_handler gms_exec_service[] = {

	{
		.exec_handler_fn	= gms_exec_message_handler,
		.exec_endian_convert_fn	= gms_exec_message_endian_convert
	}
};

	
/* Openais service handler that will be registered when the service is loaded */
struct openais_service_handler gms_service_handler = {
	.name			    = (unsigned char*)"Openais service wrapper for Clovis Group Membership Service",
	.id			        = GMS_SERVICE,
	.private_data_size	= 0, //TODO: I Think we dont need to set any private data sizeof (struct gms_pd),
	.flow_control		= OPENAIS_FLOW_CONTROL_NOT_REQUIRED, //TODO
	.lib_init_fn		= NULL,
	.lib_exit_fn		= NULL,
	.lib_service		= NULL,
	.lib_service_count	= 0,
	.exec_init_fn		= gms_exec_init_fn,
	.exec_dump_fn		= gms_exec_dump_fn,
	.exec_service		= gms_exec_service,
	.exec_service_count	= sizeof (gms_exec_service) / sizeof (struct openais_exec_handler),
	.confchg_fn		    = gms_confchg_fn,
	.sync_init		    = gms_sync_init,
	.sync_process		= gms_sync_process,
	.sync_activate		= gms_sync_activate,
	.sync_abort		    = gms_sync_abort,
};


/*
 * Below data structures are required for openais to dynamically load this service
 */
static struct openais_service_handler *gms_get_service_handler_ver0 (void);

static struct openais_service_handler_iface_ver0 gms_service_handler_iface = {
	.openais_get_service_handler_ver0	= gms_get_service_handler_ver0
};

static struct lcr_iface openais_gms_ver0[1] = {
	{
		.name			        = "clovis_gms",
		.version		        = 0,
		.versions_replace	    = 0,
		.versions_replace_count = 0,
		.dependencies		    = 0,
		.dependency_count	    = 0,
		.constructor		    = NULL,
		.destructor		        = NULL,
		.interfaces		        = NULL
	}
};

static struct lcr_comp gms_comp_ver0 = {
	.iface_count		= 1,
	.ifaces				= openais_gms_ver0
};

static struct openais_service_handler *gms_get_service_handler_ver0 (void)
{
	return (&gms_service_handler);
}

/* Constructor function which will be called when this service is loaded.
 * This will register the service handlers with openais main executive */
__attribute__ ((constructor)) static void gms_comp_register (void) 
{
	lcr_interfaces_set (&openais_gms_ver0[0], &gms_service_handler_iface);

	lcr_component_register (&gms_comp_ver0);
}



/* ------------------------------------------------------------------------
 * Definitions for functions declared above. But for GMS we dont have to
 * do anything for below functions.
 *------------------------------------------------------------------------*/

static void gms_exec_dump_fn (void) 
{
    /* Finalizing execution: As of now its noop */
    return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_init (void)
{
    /* As of now it is noop */
	return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_activate (void)
{
	return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_abort (void)
{
	return;
}



/* ------------------------------------------------------------------------
 * We hook GMS functionality into below functions.
 *------------------------------------------------------------------------*/
/* exec init function is called as soon as this service object is loaded. */
static int gms_exec_init_fn (struct objdb_iface_ver0 *objdb)
{
    ClRcT rc = CL_OK;
    extern ClRcT
        clCpmComponentRegister(ClCpmHandleT ,const ClNameT *,const ClNameT *);

    //log_init ("GMS");
    rc = clCpmComponentRegister(
            gmsGlobalInfo.cpmHandle,
            &gmsGlobalInfo.gmsComponentName,
            NULL
            );
    if(rc)
    {
        printf ("clCpmComponentRegister failed with rc 0x%x\n",rc);
    }
    gmsGlobalInfo.opState = CL_GMS_STATE_RUNNING;

    /* Initialize my ip address */
    memset(&myAddress, 0, sizeof(ClGmsNodeAddressT));
    clGmsWrapperUpdateMyIP();

    return (0);
}

/*
 * If a processor joined in the configuration change and gms_sync_activate hasn't
 * yet been called, issue a node join to share CLM specific data about the processor
 */
static int gms_sync_process (void)
{
	/*
	 * Send node information to other nodes
	 */
    if (!gmsOpenAisEnable) 
    {
        log_printf(LOG_LEVEL_NOTICE,
                "Nothing doing in sync_process as we are wating for"
               " nodeJoin from CPM\n");
        return 0;
    } 

	return (gms_nodejoin_send ());
}


static int gms_nodejoin_send (void)
{
	struct req_exec_gms_nodejoin req_exec_gms_nodejoin;
	struct iovec                 req_exec_gms_iovec;
    ClGmsClusterMemberT          thisGmsClusterNode;
	int                          result;
	req_exec_gms_nodejoin.header.size = sizeof (struct req_exec_gms_nodejoin);
	req_exec_gms_nodejoin.header.id = 
	             SERVICE_ID_MAKE (GMS_SERVICE, MESSAGE_REQ_EXEC_GMS_NODEJOIN);
    
    _clGmsGetThisNodeInfo(&thisGmsClusterNode);

    memcpy (&req_exec_gms_nodejoin.gmsClusterNode, &thisGmsClusterNode,
                                          sizeof (ClGmsClusterMemberT));
    req_exec_gms_nodejoin.gmsMessageType = CL_GMS_CLUSTER_JOIN_MSG; 

	req_exec_gms_iovec.iov_base = (char *)&req_exec_gms_nodejoin;
	req_exec_gms_iovec.iov_len = sizeof (req_exec_gms_nodejoin);
    result = totempg_groups_mcast_joined (openais_group_handle, &req_exec_gms_iovec, 1, TOTEMPG_AGREED);

	return (result);
}

/* below function is required in config_ch function
 * because the totempg_ifaces_print function returns
 * the IP address corresponding to the in a different format.
 * Hence we have the below function */
char *get_node_ip (unsigned int nodeid)
{
    static char iface_string[256 * INTERFACE_MAX];
    char one_iface[64];
    struct totem_ip_address interfaces[INTERFACE_MAX];
    char **status;
    unsigned int iface_count;
    unsigned int i;
    int res;

    iface_string[0] = '\0';

    res = totempg_ifaces_get (nodeid, interfaces, &status, &iface_count);
    if (res == -1) {
        return ("no interface found for nodeid");
    }
    assert(iface_count <= INTERFACE_MAX);

    /* Please note that in openais.conf file as of now we are specifying
     * ONLY ONE interface. The protocol works for multiple processes as well.
     * But we are limiting it to work on only one interface and the below
     * code breaks only for one interface. */
    for (i = 0; i < iface_count; i++) {
        sprintf (one_iface, "%s",totemip_print (&interfaces[i]));
        strcat (iface_string, one_iface);
        break;
    }
    return (iface_string);
}

static void gms_confchg_fn (
	enum totem_configuration_type configuration_type,
	unsigned int *member_list, int member_list_entries,
	unsigned int *left_list, int left_list_entries,
	unsigned int *joined_list, int joined_list_entries,
	struct memb_ring_id *ring_id)
{
	int     i = 0;
    char    iface_string[256 * INTERFACE_MAX] = "";
    ClRcT   rc = CL_OK;

	log_printf (LOG_LEVEL_NOTICE, "GMS CONFIGURATION CHANGE\n");
	log_printf (LOG_LEVEL_NOTICE, "GMS Configuration:\n");
	for (i = 0; i < member_list_entries; i++) {
		log_printf (LOG_LEVEL_NOTICE, "\t%s\n", totempg_ifaces_print (member_list[i]));
	}
	log_printf (LOG_LEVEL_NOTICE, "Members Left:\n");
	for (i = 0; i < left_list_entries; i++) {
		log_printf (LOG_LEVEL_NOTICE, "\t%s\n", totempg_ifaces_print (left_list[i]));
	}

	log_printf (LOG_LEVEL_NOTICE, "Members Joined:\n");
	for (i = 0; i < joined_list_entries; i++) {
		log_printf (LOG_LEVEL_NOTICE, "\t%s\n", totempg_ifaces_print (joined_list[i]));
	}

    for (i = 0; i < left_list_entries; i++) 
    {
        /* Call Cluster Leave for the nodes which are left.
         * To do that we need to translate IP address to gms nodeID
         * NOTE: Currently we are getting the this left_list[i] to
         * the ip address mapping from the get_interface_ip function.
         * This is not quite reliable and may get into issues when using
         * multiple interfaces...
         */
        strncpy(iface_string, get_node_ip(left_list[i]), (256*INTERFACE_MAX));
        rc = _clGmsEngineClusterLeaveWrapper(CL_GMS_CLUSTER_ID, iface_string);
    }

    return;
}


static void gms_exec_message_endian_convert (void *msg)
{
    struct req_exec_gms_nodejoin *node_join = msg;

    /* Convert the header first */
    swab_mar_req_header_t (&node_join->header);

    /* Covert gmsMessageType */
    node_join->gmsMessageType = swab32(node_join->gmsMessageType);

    /* Covert gmsGroupId */
    node_join->gmsGroupId = swab32(node_join->gmsGroupId);

    /* Covert values inside gmsClusterNode parameter */
    node_join->gmsClusterNode.nodeId = swab32(node_join->gmsClusterNode.nodeId);

    node_join->gmsClusterNode.nodeAddress.iocPhyAddress.portId = 
             swab32(node_join->gmsClusterNode.nodeAddress.iocPhyAddress.portId);

    node_join->gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress = 
        swab32(node_join->gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress);

    node_join->gmsClusterNode.nodeIpAddress.family = 
                         swab32(node_join->gmsClusterNode.nodeIpAddress.family);

    node_join->gmsClusterNode.nodeIpAddress.length = 
                         swab16(node_join->gmsClusterNode.nodeIpAddress.length);

    node_join->gmsClusterNode.nodeName.length = 
                              swab16(node_join->gmsClusterNode.nodeName.length);

    node_join->gmsClusterNode.memberActive = 
                                 swab16(node_join->gmsClusterNode.memberActive);
    
    node_join->gmsClusterNode.bootTimestamp = 
                                swab64(node_join->gmsClusterNode.bootTimestamp);
    
    node_join->gmsClusterNode.initialViewNumber = 
                            swab64(node_join->gmsClusterNode.initialViewNumber);

    node_join->gmsClusterNode.credential = 
                                   swab32(node_join->gmsClusterNode.credential);

    node_join->gmsClusterNode.isCurrentLeader = 
                                   swab16(node_join->gmsClusterNode.isCurrentLeader);

    /* Convert gmsGroupNode parameters */
    node_join->gmsGroupNode.memberId = swab32(node_join->gmsGroupNode.memberId);

    node_join->gmsGroupNode.memberAddress.iocPhyAddress.portId = 
             swab32(node_join->gmsGroupNode.memberAddress.iocPhyAddress.portId);

    node_join->gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress = 
        swab32(node_join->gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress);

    node_join->gmsGroupNode.memberActive = 
                                   swab16(node_join->gmsGroupNode.memberActive);

    node_join->gmsGroupNode.joinTimestamp = 
                                  swab64(node_join->gmsGroupNode.joinTimestamp);

    node_join->gmsGroupNode.initialViewNumber = 
                              swab64(node_join->gmsGroupNode.initialViewNumber);

    node_join->gmsGroupNode.credential = 
                                     swab32(node_join->gmsGroupNode.credential);

    /* Convert groupData parameters */
    node_join->groupData.groupId = swab32(node_join->groupData.groupId);

    node_join->groupData.groupParams.isIocGroup = 
                            swab16(node_join->groupData.groupParams.isIocGroup);

    node_join->groupData.noOfMembers = swab32(node_join->groupData.noOfMembers);

    node_join->groupData.setForDelete = 
                                      swab16(node_join->groupData.setForDelete);

    node_join->groupData.iocMulticastAddr =
                                  swab64(node_join->groupData.iocMulticastAddr);

    node_join->groupData.creationTimestamp =
                                 swab64(node_join->groupData.creationTimestamp);

    node_join->groupData.lastChangeTimestamp =
                               swab64(node_join->groupData.lastChangeTimestamp);

    /* Covert the eject reason */
    node_join->ejectReason = swab32(node_join->ejectReason);

    /* Convert contextHandle */
    node_join->contextHandle = swab64(node_join->contextHandle);

    /* Convert syncNoOfGroups */
    node_join->syncNoOfGroups = swab32(node_join->syncNoOfGroups);

    /* Convert syncNoOfMembers */
    node_join->syncNoOfMembers = swab32(node_join->syncNoOfMembers);
}

static void gms_exec_message_handler (
	void *message,
	unsigned int nodeid)
{
    struct req_exec_gms_nodejoin *req_exec_gms_nodejoin = (struct req_exec_gms_nodejoin *)message;
    ClGmsViewNodeT               *node = NULL;
    ClRcT                         rc = CL_OK;
    ClGmsClusterMemberT           thisGmsClusterNode = {0};
    ClGmsGroupSyncNotificationT   syncNotification = {0};
    char                          nodeIp[256 * INTERFACE_MAX] = "";
    int                           isLocalMsg = 0;

    /* Get the ip address string for the given nodeId */
    strncpy(nodeIp, get_node_ip(nodeid), (256 * INTERFACE_MAX));
    if (strcmp(nodeIp, totemip_print(&my_ip)) == 0)
    {
        isLocalMsg = 1;
    }


    switch (req_exec_gms_nodejoin->gmsMessageType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:

            node = (ClGmsViewNodeT *) clHeapAllocate(sizeof(ClGmsViewNodeT));
            if (node == NULL)
            {
                log_printf (LOG_LEVEL_NOTICE, "clHeapAllocate failed\n");
                return;
            }
            else {
                rc = clVersionVerify(
                        &(gmsGlobalInfo.config.versionsSupported),
                        &(req_exec_gms_nodejoin->gmsClusterNode.gmsVersion)
                        );
                ringVersion.releaseCode =
                    req_exec_gms_nodejoin->gmsClusterNode.gmsVersion.releaseCode;
                ringVersion.majorVersion=
                    req_exec_gms_nodejoin->gmsClusterNode.gmsVersion.majorVersion;
                ringVersion.minorVersion=
                    req_exec_gms_nodejoin->gmsClusterNode.gmsVersion.minorVersion;
                if(rc != CL_OK)
                {
                    ringVersionCheckPassed = CL_FALSE;
                    /* copy the ring version */
                    clGmsCsLeave( &joinCs );
                    log_printf(LOG_LEVEL_NOTICE,
                            "Server Version Mismatch detected ");
                    break;
                }

                _clGmsGetThisNodeInfo(&thisGmsClusterNode);
                if( thisGmsClusterNode.nodeId !=
                        req_exec_gms_nodejoin->gmsClusterNode.nodeId)
                {
                    /* TODO This will never happen... */
                    clGmsCsLeave( &joinCs );
                }

                node->viewMember.clusterMember =
                    req_exec_gms_nodejoin->gmsClusterNode;
                /* If this is local join, then update the IP address */
                if (thisGmsClusterNode.nodeId ==
                        req_exec_gms_nodejoin->gmsClusterNode.nodeId)
                {
                    memcpy(&node->viewMember.clusterMember.nodeIpAddress,
                            &myAddress, sizeof(ClGmsNodeAddressT));
                }

                rc = _clGmsEngineClusterJoin(CL_GMS_CLUSTER_ID,
                        req_exec_gms_nodejoin->gmsClusterNode.nodeId,
                        node);
            }
            break;
        case CL_GMS_CLUSTER_EJECT_MSG:
            /* inform the member about the eject by invoking the ejection
             *  callback registered with the reason UKNOWN */
            /* The below logic is same for the leave as well so we just
             *  fall through the case */
            _clGmsGetThisNodeInfo(&thisGmsClusterNode);
            if( req_exec_gms_nodejoin->gmsClusterNode.nodeId ==
                    thisGmsClusterNode.nodeId)
            {
                rc = _clGmsCallClusterMemberEjectCallBack(
                        req_exec_gms_nodejoin ->ejectReason);
                if( rc != CL_OK )
                {
                    printf("\n _clGmsCallEjectCallBack failed with"
                            "rc:0x%x",rc);
                }
            }
        case CL_GMS_CLUSTER_LEAVE_MSG:
            rc = _clGmsEngineClusterLeave(CL_GMS_CLUSTER_ID,
                    req_exec_gms_nodejoin->gmsClusterNode.nodeId);
            break;
        case CL_GMS_GROUP_CREATE_MSG:
            rc = _clGmsEngineGroupCreate(req_exec_gms_nodejoin->groupData.groupName,
                    req_exec_gms_nodejoin->groupData.groupParams,
                    req_exec_gms_nodejoin->contextHandle, isLocalMsg);
            break;
        case CL_GMS_GROUP_DESTROY_MSG:
            rc = _clGmsEngineGroupDestroy(req_exec_gms_nodejoin->groupData.groupId,
                    req_exec_gms_nodejoin->groupData.groupName,
                    req_exec_gms_nodejoin->contextHandle, isLocalMsg);
            break;
        case CL_GMS_GROUP_JOIN_MSG:

            node = (ClGmsViewNodeT *) clHeapAllocate(sizeof(ClGmsViewNodeT));
            if (!node)
            {
                log_printf (LOG_LEVEL_NOTICE, "clHeapAllocate failed\n");
                return;
            }
            else {
                /* FIXME: Need to verify version */
                memcpy(&node->viewMember.groupMember,&req_exec_gms_nodejoin->gmsGroupNode,
                        sizeof(ClGmsGroupMemberT));
                memcpy(&node->viewMember.groupData, &req_exec_gms_nodejoin->groupData,
                        sizeof(ClGmsGroupInfoT));
                rc = _clGmsEngineGroupJoin(req_exec_gms_nodejoin->groupData.groupId,
                        node, req_exec_gms_nodejoin->contextHandle, isLocalMsg);
            }
            break;
        case CL_GMS_GROUP_LEAVE_MSG:
            rc = _clGmsEngineGroupLeave(req_exec_gms_nodejoin->groupData.groupId,
                    req_exec_gms_nodejoin->gmsGroupNode.memberId,
                    req_exec_gms_nodejoin->contextHandle, isLocalMsg);
            break;
        case CL_GMS_COMP_DEATH:
            rc = _clGmsRemoveMemberOnCompDeath(req_exec_gms_nodejoin->gmsGroupNode.memberId);
            break;
        case CL_GMS_SYNC_MESSAGE:
            /* Need to decipher the other part of the message also */
            syncNotification.noOfGroups = req_exec_gms_nodejoin->syncNoOfGroups;
            syncNotification.noOfMembers = req_exec_gms_nodejoin->syncNoOfMembers;
            if (syncNotification.noOfGroups > 0)
            {
                syncNotification.groupInfoList =
                    message+sizeof(struct req_exec_gms_nodejoin);
                if (syncNotification.noOfMembers > 0)
                {
                    syncNotification.groupMemberList =
                        message+sizeof(struct req_exec_gms_nodejoin) +
                        (sizeof(ClGmsGroupInfoT) * syncNotification.noOfGroups);
                }
            }

            rc = _clGmsEngineGroupInfoSync(&syncNotification);
            break;

        default:
            log_printf (LOG_LEVEL_ERROR, 
                    "Openais GMS wrapper received Message wih invalid [MsgType=%x]. "
                    "This could be because of multicast port clashes.\n",
                    req_exec_gms_nodejoin->gmsMessageType);
            return;
    }
}


/* ------------------------------------------------------------------------
 * Clovis specific wrapper functions.
 *------------------------------------------------------------------------*/
/* Called from Clovis GMS files to send and receive cluster and group messages */

int clGmsSendMsg(ClGmsViewMemberT       *memberNodeInfo,
                 ClGmsGroupIdT           groupId, 
                 ClGmsMessageTypeT       msgType,
                 ClGmsMemberEjectReasonT ejectReason )
{
    struct req_exec_gms_nodejoin req_exec_gms_nodejoin = {{0}};
    struct iovec req_exec_gms_iovec = {0};
    int result = -1;

    req_exec_gms_nodejoin.header.size = sizeof (struct req_exec_gms_nodejoin);
    req_exec_gms_nodejoin.header.id =
                 SERVICE_ID_MAKE (GMS_SERVICE, MESSAGE_REQ_EXEC_GMS_NODEJOIN);

    switch(msgType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:
        case CL_GMS_CLUSTER_LEAVE_MSG:
        case CL_GMS_CLUSTER_EJECT_MSG:
            req_exec_gms_nodejoin.ejectReason = ejectReason;
            memcpy (&req_exec_gms_nodejoin.gmsClusterNode, &memberNodeInfo->clusterMember,
                    sizeof (ClGmsClusterMemberT));
            break;
        case CL_GMS_GROUP_CREATE_MSG:
        case CL_GMS_GROUP_DESTROY_MSG:
        case CL_GMS_GROUP_JOIN_MSG:
        case CL_GMS_GROUP_LEAVE_MSG:
            memcpy (&req_exec_gms_nodejoin.gmsGroupNode, &memberNodeInfo->groupMember,
                    sizeof (ClGmsGroupMemberT));
            memcpy (&req_exec_gms_nodejoin.groupData, &memberNodeInfo->groupData,
                    sizeof(ClGmsGroupInfoT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            break;
        case CL_GMS_COMP_DEATH:
            memcpy (&req_exec_gms_nodejoin.gmsGroupNode, &memberNodeInfo->groupMember,
                    sizeof (ClGmsGroupMemberT));
            break;
        default:
            log_printf (LOG_LEVEL_NOTICE, " clGmsSendMsg: wrong message type:%x\n",
                    msgType);
            return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    req_exec_gms_nodejoin.gmsMessageType = msgType;

    req_exec_gms_iovec.iov_base = (char *)&req_exec_gms_nodejoin;
    req_exec_gms_iovec.iov_len = sizeof (req_exec_gms_nodejoin);

    result = totempg_groups_mcast_joined (openais_group_handle, &req_exec_gms_iovec, 1, TOTEMPG_AGREED);

    return result;
}

void clGmsWrapperUpdateMyIP()
{
    /* This function extracts the latest "this_ip" information
     * about the IP address that this node has bound to
     * and updates a GMS specific local structure.
     * In the config change functions we update my node
     * details with the updated IP in case there was a network
     * up/down and the node has bound to new IP */
    sprintf ((char *)myAddress.value, "%s",totemip_print (&my_ip));
    myAddress.length = strlen ((char *)myAddress.value);
    if (my_ip.family == AF_INET) {
        myAddress.family = CL_GMS_AF_INET;
    } else {
        if (my_ip.family == AF_INET6) {
            myAddress.family = CL_GMS_AF_INET6;
        } else {
            assert (0);
        }
    }
    /* Also update the global gms data of thisnode with new IP */
    gmsGlobalInfo.config.thisNodeInfo.nodeIpAddress = myAddress;
       
    log_printf (LOG_LEVEL_NOTICE, "My Local IP address = %s\n",myAddress.value);
}


int clGmsSendSyncMsg (ClGmsGroupSyncNotificationT *syncNotification)
{
    struct req_exec_gms_nodejoin req_exec_gms_nodejoin = {{0}};
    struct iovec req_exec_gms_iovec = {0};
    void *message = NULL;
    int result = -1;
    int totemMsgLength = 0;
    int groupInfoLength = 0;
    int groupMembersLength = 0;
    int totalLength = 0;

    req_exec_gms_nodejoin.header.size = sizeof (struct req_exec_gms_nodejoin);

    req_exec_gms_nodejoin.header.id = 
        SERVICE_ID_MAKE (GMS_SERVICE, MESSAGE_REQ_EXEC_GMS_NODEJOIN);

    req_exec_gms_nodejoin.syncNoOfGroups = syncNotification->noOfGroups;
    req_exec_gms_nodejoin.syncNoOfMembers = syncNotification->noOfMembers;
    req_exec_gms_nodejoin.gmsMessageType = CL_GMS_SYNC_MESSAGE;

    /* Here we allocate the memory for req_exec_gms_nodejoin structure
     * + groupInfoList + groupMemberList
     */
    totemMsgLength = sizeof(struct req_exec_gms_nodejoin);
    groupInfoLength = sizeof(ClGmsGroupInfoT) * syncNotification->noOfGroups;
    groupMembersLength = sizeof(ClGmsViewNodeT) * syncNotification->noOfMembers;
    totalLength = totemMsgLength + groupInfoLength + groupMembersLength;
    message = clHeapAllocate(totalLength);
    if (message == NULL)
    {
        printf("Couldnt allocate %d bytes of memory \n",totalLength);
        return -1;
    }

    /* Copy totem message */
    memcpy(message, &req_exec_gms_nodejoin,totemMsgLength);

    if (syncNotification->noOfGroups > 0)
    {
        /* Copy groups info list */
        memcpy(message+totemMsgLength,syncNotification->groupInfoList,
                groupInfoLength);
        if (syncNotification->noOfMembers > 0)
        {
            /* Copy group members list */
            memcpy(message+totemMsgLength+groupInfoLength,
                    syncNotification->groupMemberList, groupMembersLength);
        }
    }

    log_printf(LOG_LEVEL_NOTICE, "Sending a sync message with length %d\n",totalLength);


    req_exec_gms_iovec.iov_base = (char *)message;
    req_exec_gms_iovec.iov_len = totalLength;

    result = totempg_groups_mcast_joined (openais_group_handle, &req_exec_gms_iovec, 1, TOTEMPG_AGREED);

    clHeapFree(message);

    return result;
}
