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
 *      |  +-------+ +-------+ +-------+        +=======+  |  +=======+
 *      |  |  AMF  | |  EVS  | |  CLM  |       ||  GMS  || | ||  GMS  ||
 *      |  |Service| |Service| |Service| * * * ||Wrapper|=====| Server||
 *      |  +-------+ +-------+ +-------+       ||Service|| | ||       ||
 *      |                                       +=======+  |  +=======+
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

#include "clovis_gms_wrapper.h"


/* ------------------------------------------------------------------------
 * Macros
 *------------------------------------------------------------------------*/
#define SERVICE_ID_EXTRACT(a) ((a) & 0x0000FFFF)


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


/* This is a modified service to send synch before leader 
 * election takes place; so that all nodes are synched. */
static void synch_init(void);


/* ------------------------------------------------------------------------
 * Openais specific data structure definitions below:
 * These are used for implementing GMS functionality
 *------------------------------------------------------------------------*/
ClVersionT          ringVersion;
ClGmsNodeAddressT   myAddress;

/* Below message types define type of messages "THIS" service
 * would receive and interpret.*/
enum gms_message_req_types
{
    MESSAGE_REQ_EXEC_GMS_NODEJOIN = 0
};


/* Message strucutre which will contain the GMS related values
 * to be passed through totem
 */
ClVersionT  curVer = {CL_RELEASE_VERSION, 1, CL_MINOR_VERSION};

/* In this release support for previous version is just a place holder which
 * can be replaced when the code changes for current verion */

static int gms_nodejoin_send ();

void clGmsWrapperUpdateMyIP();

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
    clLog(DBG,OPN,AIS, "Registered Clovis GMS service with openais");
}



/* ------------------------------------------------------------------------
 * Definitions for functions declared above. But for GMS we dont have to
 * do anything for below functions.
 *------------------------------------------------------------------------*/

/*
 * This is a noop for this service
 */
static void gms_exec_dump_fn (void) 
{
    /* Finalizing execution: As of now its noop */
    clLog(TRACE,OPN,AIS, "exec_dump function is called for ClovisGMS service");
    return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_init (void)
{
    /* As of now it is noop */
    clLog(TRACE,OPN,AIS, "sync_init function is called for ClovisGMS service");
	return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_activate (void)
{
    clLog(TRACE,OPN,AIS, "sync_activate function is called for ClovisGMS service");

    // modifed to make call for synch
    clLog(TRACE,OPN,AIS, "synch_init function is called!");
    synch_init();

	return;
}


/*
 * This is a noop for this service
 */
static void gms_sync_abort (void)
{
    clLog(TRACE,OPN,AIS, "sync_abort function is called for ClovisGMS service");
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

    log_init ("GMS");
    clLog (DBG,OPN,AIS,"Initializing clovis GMS service");
    rc = clCpmComponentRegister(
            gmsGlobalInfo.cpmHandle,
            &gmsGlobalInfo.gmsComponentName,
            NULL
            );
    if(rc)
    {
        clLog(ERROR,OPN,AIS,
                "clCpmComponent register failed with rc 0x%x",rc);
    }
    clLogMultiline(DBG,OPN,AIS,
            "clCpmComponentRegister successful. Updating GMS state as RUNNING");
    gmsGlobalInfo.opState = CL_GMS_STATE_RUNNING;

    /* Initialize my ip address */
    memset(&myAddress, 0, sizeof(ClGmsNodeAddressT));
    clGmsWrapperUpdateMyIP();

    return (0);
}


/* This is a modified service to send synch before leader 
 * election takes place; so that all nodes are synched. */
static void synch_init(void)
{

    ClRcT              		rc 		 = CL_OK;
    ClGmsGroupSyncNotificationT syncNotification = {0};
    ClUint32T          		noOfItems 	 = 0;
    void*              		buf 		 = NULL;
    ClGmsDbT*          		thisViewDb 	 = NULL;
    ClUint32T          		i 		 = 0;
    ClInt32T           		result 		 = 0;

    // Only leader should be sending out, synch Info (group => default cluster)
    if (gmsGlobalInfo.config.thisNodeInfo.isCurrentLeader)
    {
        clLog(INFO,GROUPS,NA, "I am leader of the cluster. So sending Groups Sync message for the new node.");

        /* Send SYNC message with entire group database */
        syncNotification.noOfGroups = 0;
        syncNotification.noOfMembers = 0;

        clGmsMutexLock(gmsGlobalInfo.dbMutex);
        clLog(TRACE,CLM,NA, "Acquired mutex. Now gathering groups info");

        // Sending updates only for other groups; default cluster group nodes update each other
        for (i=1; i < gmsGlobalInfo.config.noOfGroups; i++)
        {
            if ((gmsGlobalInfo.db[i].view.isActive == CL_TRUE) &&
                (gmsGlobalInfo.db[i].viewType == CL_GMS_GROUP || (gmsGlobalInfo.db[i].viewType == CL_GMS_CLUSTER)) )
            {
                ClInt32T j = 0;
                /* These 2 conditions should indicate that the group exists and is active.
                */
                thisViewDb = &gmsGlobalInfo.db[i];

                /* Get the group Info for thisViewDb */
                syncNotification.groupInfoList = (ClGmsGroupInfoT*)realloc(syncNotification.groupInfoList,
                                                  sizeof(ClGmsGroupInfoT)*(syncNotification.noOfGroups+1));
                if (syncNotification.groupInfoList == NULL)
                {
                    clLog(ERROR,CLM,NA, "Could not allocate memory while gathering group information, synch failed!");
                    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
                    rc = CL_ERR_NO_MEMORY;
                    return;
                }

                memcpy(&syncNotification.groupInfoList[syncNotification.noOfGroups], &(thisViewDb->groupInfo),sizeof(ClGmsGroupInfoT));
                syncNotification.noOfGroups++;

                /* Get the list of members of this group */
                rc = _clGmsViewGetCurrentViewNotification(thisViewDb, &buf, &noOfItems);
                if (rc != CL_OK)
                {
                   clLog(ERROR,CLM,NA, "_clGmsViewGetCurrentViewNotification failed while sending SYNC message. rc = 0x%x\n",rc);
                   clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
                    return;
                }

                clLog(TRACE,CLM,NA, "group info for group [%d]; with members [%d]", thisViewDb->groupInfo.groupId, noOfItems);

               if ((noOfItems == 0) || (buf == NULL))
               {
                   buf = NULL;
                   noOfItems = 0;
                   continue;
               }

               syncNotification.groupMemberList = (ClGmsViewNodeT*)realloc(syncNotification.groupMemberList,
                                                    sizeof(ClGmsViewNodeT)*(noOfItems+syncNotification.noOfMembers));
               if (syncNotification.groupMemberList == NULL)
               {
                    clLog(ERROR,CLM,NA, "Could not allocate memory while gathering group information");
                    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
                    rc = CL_ERR_NO_MEMORY;
                    return;
               }

                memset(&syncNotification.groupMemberList[syncNotification.noOfMembers], 0, sizeof(ClGmsViewNodeT)*noOfItems);
                for (j = 0; j < noOfItems; j++)
                {
                    memcpy(&syncNotification.groupMemberList[syncNotification.noOfMembers].viewMember.groupMember,
                           &( ( (ClGmsGroupNotificationT*)buf )[j].groupMember ),
                            sizeof(ClGmsGroupMemberT) );
                    syncNotification.groupMemberList[syncNotification.noOfMembers].viewMember.groupData.groupId =
                        thisViewDb->groupInfo.groupId;
                    clLog(DBG,GEN,NA, "Sync group Id [%d]", thisViewDb->groupInfo.groupId);
                    syncNotification.noOfMembers++;
                }

                clHeapFree(buf);
                buf = NULL;
                noOfItems = 0;
            }
        }

        clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
        clLog(TRACE,CLM,NA, "Gathered group information for [%d] groups with [%d] members. Now sending it over multicast",
                        syncNotification.noOfGroups, syncNotification.noOfMembers);

        if (syncNotification.noOfGroups > 0)
        {
            /* Send the multicast message */
            result = clGmsSendMsg(NULL, 0x0, CL_GMS_SYNC_MESSAGE, 0, 0, (void *)&syncNotification);
            if (result < 0)
            {
                clLog(ERROR,GROUPS,NA, "Openais Sync Message Send failed");
            }

            clLog(TRACE,CLM,NA, "Group information is sent over multicast");
        }

        /* Free the pointer used to gather the group member info.
         * since the memory is allocated using realloc, free it 
         * using normal free
         */
        if (syncNotification.noOfGroups > 0)
        {
            free(syncNotification.groupInfoList);
            if (syncNotification.noOfMembers > 0)
            {
                free(syncNotification.groupMemberList);
            }
        }
    }
    else
    {
        clLog(CRITICAL,CLM,NA, "Node is not a leader; so expecting the Synch message from Leader!");
    }

    return;
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

    /* In this sync message always send the latest version */
	return (gms_nodejoin_send ());
}


static int gms_nodejoin_send (void)
{
    /* For now this function sends only latest version. It needs to be 
     * modified in future when version changes */
    /* Send the join message with given version */
    mar_req_header_t                    header = {0};
	struct VDECL(req_exec_gms_nodejoin) req_exec_gms_nodejoin;
	struct iovec                 req_exec_gms_iovec;
    ClGmsClusterMemberT          thisGmsClusterNode;
	int                          result;
    ClRcT                        rc = CL_OK;
    ClUint32T                    clusterVersion;
    ClBufferHandleT bufferHandle = 0;
    ClUint8T        *message = NULL;
    ClPtrT          temp = NULL;
    ClUint32T       length = 0;
    

    rc = clNodeCacheMinVersionGet(NULL, &clusterVersion);
    if (rc != CL_OK)
    {
        clLog(ERROR,OPN,AIS,
                "Error while getting version from the version cache. rc 0x%x",rc);

        curVer.releaseCode = CL_RELEASE_VERSION;
        curVer.majorVersion = CL_MAJOR_VERSION;
        curVer.minorVersion = CL_MINOR_VERSION;
    } else {
        curVer.releaseCode = CL_VERSION_RELEASE(clusterVersion);
        curVer.majorVersion = CL_VERSION_MAJOR(clusterVersion);
        curVer.minorVersion = CL_VERSION_MINOR(clusterVersion);
    }

    clLog(DBG,OPN,AIS,
            "This node is sending join message for version %d, %d, %d",
            curVer.releaseCode, curVer.majorVersion, curVer.minorVersion);
    /* Get the version and send it */
    req_exec_gms_nodejoin.version.releaseCode = curVer.releaseCode;
    req_exec_gms_nodejoin.version.majorVersion = curVer.majorVersion;
    req_exec_gms_nodejoin.version.minorVersion = curVer.minorVersion;

    _clGmsGetThisNodeInfo(&thisGmsClusterNode);

    memcpy (&req_exec_gms_nodejoin.specificMessage.gmsClusterNode, &thisGmsClusterNode,
                                          sizeof (ClGmsClusterMemberT));

    // node join is send for default cluster group - 0
    req_exec_gms_nodejoin.gmsGroupId     = 0;
    memcpy (&req_exec_gms_nodejoin.specificMessage.gmsClusterNode, &thisGmsClusterNode, sizeof (ClGmsClusterMemberT));
    req_exec_gms_nodejoin.gmsMessageType = CL_GMS_CLUSTER_JOIN_MSG; 

    /* Create a buffer handle and marshall the eliments */
    rc = clBufferCreate(&bufferHandle);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to create buffer while sending message on totem. rc 0x%x",rc);
        return rc;
    }

    rc = marshallReqExecGmsNodeJoin(&req_exec_gms_nodejoin,bufferHandle);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to marshall the data while sending message on totem. rc 0x%x",rc);
        goto buffer_delete_return;
    }

    rc = clBufferLengthGet(bufferHandle, &length);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to get buffer length. rc 0x%x",rc);
        goto buffer_delete_return;
    }

    rc = clBufferFlatten(bufferHandle, &message);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "clBufferFlatten failed with rc 0x%x",rc);
        goto buffer_delete_return;
    }

    /* We need to prepend the total message length in the beginning of the
     * message so that we can find the length while unmarshalling */
    temp = clHeapAllocate(length + sizeof(mar_req_header_t));
    if (temp == NULL)
    {
        clLogError(OPN,AIS, 
                "Failed to allocate memory while sending the message");
        goto buffer_delete_return;
    }

	header.id = SERVICE_ID_MAKE (GMS_SERVICE, MESSAGE_REQ_EXEC_GMS_NODEJOIN);
    header.size = length + sizeof(mar_req_header_t);

    memcpy(temp,&header,sizeof(mar_req_header_t));
    memcpy(temp+sizeof(mar_req_header_t), message, length);

    req_exec_gms_iovec.iov_base = temp;
    req_exec_gms_iovec.iov_len = length + sizeof(mar_req_header_t);

    clLog(DBG,OPN,AIS,
            "Sending node join from this node in sync_process");
    result = totempg_groups_mcast_joined (openais_group_handle, &req_exec_gms_iovec, 1, TOTEMPG_AGREED);

buffer_delete_return:
    if (message != NULL)
        clHeapFree(message);
    if (temp != NULL)
        clHeapFree(temp);
    clBufferDelete(&bufferHandle);
    return result;
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
    int res;

    iface_string[0] = '\0';

    res = totempg_ifaces_get (nodeid, interfaces, &status, &iface_count);
    if (res == -1) {
        clLog(ERROR,OPN,AIS,
                "totempg_ifaces_get failed");
        return ("no interface found for nodeid");
    }
    assert(iface_count <= INTERFACE_MAX);

    /* Please note that in openais.conf file as of now we are specifying
     * ONLY ONE interface. The protocol works for multiple processes as well.
     * But we are limiting it to work on only one interface
     */
    sprintf (one_iface, "%s",totemip_print (&interfaces[0]));
    strncat (iface_string, one_iface, sizeof(iface_string)-1);

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

	clLog (NOTICE,OPN,AIS, "GMS CONFIGURATION CHANGE");
	clLog (NOTICE,OPN,AIS, "GMS Configuration:");
	for (i = 0; i < member_list_entries; i++) {
		clLog (NOTICE,OPN,AIS, "\t%s", totempg_ifaces_print (member_list[i]));
	}
	clLog(NOTICE,OPN,AIS, "Members Left:");
	for (i = 0; i < left_list_entries; i++) {
		clLog (NOTICE,OPN,AIS, "\t%s", totempg_ifaces_print (left_list[i]));
	}

	clLog(NOTICE,OPN,AIS, "Members Joined:");
	for (i = 0; i < joined_list_entries; i++) {
		clLog (NOTICE,OPN,AIS, "\t%s", totempg_ifaces_print (joined_list[i]));
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
        strncpy(iface_string, get_node_ip(left_list[i]), (256*INTERFACE_MAX)-1);
        clLog(DBG,OPN,AIS, "Invoking cluster leave for node with IP %s",iface_string);
        rc = _clGmsEngineClusterLeaveWrapper(CL_GMS_CLUSTER_ID, iface_string);
    }

    return;
}


static void gms_exec_message_endian_convert (void *msg)
{
    /* We only need to convert the header as all the other
     * elements are being marshalled/unmarshalled using xdr
     */
    mar_req_header_t              *header = msg;

    clLog(DBG,OPN,AIS, "Converting endianness for this message");

    swab_mar_req_header_t (header);
}

static void gms_exec_message_handler (
	void *message,
	unsigned int nodeid)
{
    mar_req_header_t              header = {0};
    struct VDECL(req_exec_gms_nodejoin) req_exec_gms_nodejoin = {{0}};
    ClGmsViewNodeT               *node = NULL;
    ClRcT                         rc = CL_OK;
    ClGmsClusterMemberT           thisGmsClusterNode = {0};
    char                          nodeIp[256 * INTERFACE_MAX] = "";
    int                           isLocalMsg = 0;
    int                           verCode = 0;
    ClBufferHandleT               bufferHandle = NULL;

    /* Get the ip address string for the given nodeId */
    strncpy(nodeIp, get_node_ip(nodeid), (256 * INTERFACE_MAX)-1);
    if (strcmp(nodeIp, totemip_print(this_ip)) == 0)
    {
        isLocalMsg = 1;
    }

    /* Unmarshall the incoming message */
    rc = clBufferCreate(&bufferHandle);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to create buffer while unmarshalling the received message. rc 0x%x",rc);
        return;
    }

    memcpy(&header, message, sizeof(mar_req_header_t));

    rc = clBufferNBytesWrite(bufferHandle, (ClUint8T *)message+sizeof(mar_req_header_t), header.size-sizeof(mar_req_header_t));
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to retrieve data from buffer. rc 0x%x",rc);
        goto out_delete;
    }

    rc = unmarshallReqExecGmsNodeJoin(bufferHandle, &req_exec_gms_nodejoin);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,"Failed to unmarshall the data. rc 0x%x",rc);
        goto out_delete;
    }

    verCode = CL_VERSION_CODE(req_exec_gms_nodejoin.version.releaseCode, 
            req_exec_gms_nodejoin.version.majorVersion,
            req_exec_gms_nodejoin.version.minorVersion);
    clLog(DBG,OPN,AIS,
            "Received a %d message from version [%d.%d.%d].",req_exec_gms_nodejoin.gmsMessageType,
            req_exec_gms_nodejoin.version.releaseCode, req_exec_gms_nodejoin.version.majorVersion, 
            req_exec_gms_nodejoin.version.minorVersion);
    /* Verify version */
    if (verCode > CL_VERSION_CODE(curVer.releaseCode, curVer.majorVersion, curVer.minorVersion)) {
        /* I received a message from higher version and it dont know
         * how to decode it. So it discarding it. */
        clLog(NOTICE,OPN,AIS,
                "Version mismatch detected. Discarding the message ");
        goto out_delete;
    }

    // message type & message data
    clLog(DBG,OPN,AIS,"message type %d from groupId %d!\n", req_exec_gms_nodejoin.gmsMessageType, req_exec_gms_nodejoin.gmsGroupId);

    /* This message is from same version. So processing it */
    switch (req_exec_gms_nodejoin.gmsMessageType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:

            clLog(DBG,OPN,AIS,
                  "Received multicast message for cluster join from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.portId);

            node = (ClGmsViewNodeT *) clHeapAllocate(sizeof(ClGmsViewNodeT));
            if (node == NULL)
            {
                clLog (ERROR,OPN,AIS, "clHeapAllocate failed");
                goto out_delete;
            }
            else {
                rc = clVersionVerify(
                        &(gmsGlobalInfo.config.versionsSupported),
                        &(req_exec_gms_nodejoin.specificMessage.gmsClusterNode.gmsVersion)
                        );
                ringVersion.releaseCode =
                    req_exec_gms_nodejoin.specificMessage.gmsClusterNode.gmsVersion.releaseCode;
                ringVersion.majorVersion=
                    req_exec_gms_nodejoin.specificMessage.gmsClusterNode.gmsVersion.majorVersion;
                ringVersion.minorVersion=
                    req_exec_gms_nodejoin.specificMessage.gmsClusterNode.gmsVersion.minorVersion;
                if(rc != CL_OK)
                {
                    ringVersionCheckPassed = CL_FALSE;
                    /* copy the ring version */
                    clGmsCsLeave( &joinCs );
                    clLog (ERROR,OPN,AIS,
                            "Server Version Mismatch detected for this join message");
                    break;
                }

                _clGmsGetThisNodeInfo(&thisGmsClusterNode);
                if( thisGmsClusterNode.nodeId !=
                        req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeId)
                {
                    /* TODO This will never happen... */
                    clGmsCsLeave( &joinCs );
                }

                node->viewMember.clusterMember =
                    req_exec_gms_nodejoin.specificMessage.gmsClusterNode;
                /* If this is local join, then update the IP address */
                if (thisGmsClusterNode.nodeId ==
                        req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeId)
                {
                    memcpy(&node->viewMember.clusterMember.nodeIpAddress,
                            &myAddress, sizeof(ClGmsNodeAddressT));
                }

                rc = _clGmsEngineClusterJoin(req_exec_gms_nodejoin.gmsGroupId,
                        req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeId,
                        node);
            }
            break;
        case CL_GMS_CLUSTER_EJECT_MSG:
            clLog (DBG,OPN,AIS,
                   "Received cluster eject multicast message from ioc node [%#x:%#x]",
                   req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress,
                   req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.portId);
            /* inform the member about the eject by invoking the ejection
             *  callback registered with the reason UKNOWN */
            /* The below logic is same for the leave as well so we just
             *  fall through the case */
            _clGmsGetThisNodeInfo(&thisGmsClusterNode);
            if( req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeId ==
                    thisGmsClusterNode.nodeId)
            {
                rc = _clGmsCallClusterMemberEjectCallBack(
                        req_exec_gms_nodejoin.ejectReason);
                if( rc != CL_OK )
                {
                    clLog(ERROR,OPN,AIS,"_clGmsCallEjectCallBack failed with"
                            "rc:0x%x",rc);
                }
            }
        case CL_GMS_CLUSTER_LEAVE_MSG:
            clLog(DBG,OPN,AIS,
                  "Received cluster leave multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.portId);
            rc = _clGmsEngineClusterLeave(CL_GMS_CLUSTER_ID,
                    req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeId);
            break;
        case CL_GMS_GROUP_CREATE_MSG:
            clLog(DBG,OPN,AIS,
                  "Received group create multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.portId);

            rc = _clGmsEngineGroupCreate(req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupName,
                    req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupParams,
                    req_exec_gms_nodejoin.contextHandle, isLocalMsg);
            break;
        case CL_GMS_GROUP_DESTROY_MSG:
            clLog(DBG,OPN,AIS,
                  "Received group destroy multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.portId);

            rc = _clGmsEngineGroupDestroy(req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupId,
                    req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupName,
                    req_exec_gms_nodejoin.contextHandle, isLocalMsg);
            break;
        case CL_GMS_GROUP_JOIN_MSG:
            clLog(DBG,OPN,AIS,
                  "Received group join multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.portId);

            node = (ClGmsViewNodeT *) clHeapAllocate(sizeof(ClGmsViewNodeT));
            if (!node)
            {
                log_printf (LOG_LEVEL_NOTICE, "clHeapAllocate failed");
                goto out_delete;
            }
            else {
                /* FIXME: Need to verify version */
                memcpy(&node->viewMember.groupMember,&req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode,
                        sizeof(ClGmsGroupMemberT));
                memcpy(&node->viewMember.groupData, &req_exec_gms_nodejoin.specificMessage.groupMessage.groupData,
                        sizeof(ClGmsGroupInfoT));
                rc = _clGmsEngineGroupJoin(req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupId,
                        node, req_exec_gms_nodejoin.contextHandle, isLocalMsg);
            }
            break;
        case CL_GMS_GROUP_LEAVE_MSG:
            clLog(DBG,OPN,AIS,
                  "Received group leave multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberAddress.iocPhyAddress.portId);

            rc = _clGmsEngineGroupLeave(req_exec_gms_nodejoin.specificMessage.groupMessage.groupData.groupId,
                    req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberId,
                    req_exec_gms_nodejoin.contextHandle, isLocalMsg);
            break;
        case CL_GMS_COMP_DEATH:
            clLog(DBG,OPN,AIS,
                  "Received comp death multicast message");
            rc = _clGmsRemoveMemberOnCompDeath(req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode.memberId);
            break;
        case CL_GMS_LEADER_ELECT_MSG:
            clLog(DBG,OPN,AIS,
                  "Received leader elect multicast message from ioc node [%#x:%#x]",
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.nodeAddress,
                  req_exec_gms_nodejoin.specificMessage.gmsClusterNode.nodeAddress.iocPhyAddress.portId);
            rc = _clGmsEnginePreferredLeaderElect(req_exec_gms_nodejoin.specificMessage.gmsClusterNode, 
                    req_exec_gms_nodejoin.contextHandle,
                    isLocalMsg);
            break;
        case CL_GMS_SYNC_MESSAGE:
            clLog(DBG,OPN,AIS,
                    "Received gms synch multicast message");
            rc = _clGmsEngineGroupInfoSync((ClGmsGroupSyncNotificationT *)(req_exec_gms_nodejoin.dataPtr));
            clHeapFree(((ClGmsGroupSyncNotificationT *)req_exec_gms_nodejoin.dataPtr)->groupInfoList);
            clHeapFree(((ClGmsGroupSyncNotificationT *)req_exec_gms_nodejoin.dataPtr)->groupMemberList);
            clHeapFree(req_exec_gms_nodejoin.dataPtr);
            break;

        case CL_GMS_GROUP_MCAST_MSG:
            _clGmsEngineMcastMessageHandler(
                    &(req_exec_gms_nodejoin.specificMessage.mcastMessage.groupInfo.gmsGroupNode),
                    &(req_exec_gms_nodejoin.specificMessage.mcastMessage.groupInfo.groupData),
                    req_exec_gms_nodejoin.specificMessage.mcastMessage.userDataSize,
                    req_exec_gms_nodejoin.dataPtr);
            break;
        default:
            clLogMultiline(ERROR,OPN,AIS,
                    "Openais GMS wrapper received Message wih invalid [MsgType=%x]. \n"
                    "This could be because of multicast port clashes.",
                    req_exec_gms_nodejoin.gmsMessageType);
            goto out_delete;
    }
    clLog(TRACE,OPN,AIS,
            "Processed the received message. Returning");
    out_delete:
    clBufferDelete(&bufferHandle);
}


/* ------------------------------------------------------------------------
 * Clovis specific wrapper functions.
 *------------------------------------------------------------------------*/
/* Called from Clovis GMS files to send and receive cluster and group messages */

int clGmsSendMsg(ClGmsViewMemberT       *memberNodeInfo,
                 ClGmsGroupIdT           groupId, 
                 ClGmsMessageTypeT       msgType,
                 ClGmsMemberEjectReasonT ejectReason,
                 ClUint32T               dataSize,
                 ClPtrT                  dataPtr)
{
    mar_req_header_t        header = {0};
    struct VDECL(req_exec_gms_nodejoin) req_exec_gms_nodejoin = {{0}};
    struct iovec    req_exec_gms_iovec = {0};
    int             result = -1;
    ClRcT           rc = CL_OK;
    ClUint32T       clusterVersion = 0;
    ClBufferHandleT bufferHandle = 0;
    ClUint8T        *message = NULL;
    ClUint32T       length = 0;
    ClPtrT          temp = NULL;

    rc = clNodeCacheMinVersionGet(NULL, &clusterVersion);
    if (rc != CL_OK)
    {
        clLog(ERROR,OPN,AIS,
                "Error while getting version from the version cache. rc 0x%x",rc);

        curVer.releaseCode = CL_RELEASE_VERSION;
        curVer.majorVersion = CL_MAJOR_VERSION;
        curVer.minorVersion = CL_MINOR_VERSION;
    } else {

        curVer.releaseCode = CL_VERSION_RELEASE(clusterVersion);
        curVer.majorVersion = CL_VERSION_MAJOR(clusterVersion);
        curVer.minorVersion = CL_VERSION_MINOR(clusterVersion);
    }

    /* Get the version and send it */
    req_exec_gms_nodejoin.version.releaseCode = curVer.releaseCode;
    req_exec_gms_nodejoin.version.majorVersion = curVer.majorVersion;
    req_exec_gms_nodejoin.version.minorVersion = curVer.minorVersion;

    /* For now we send message without caring about version. Later on
     * we need to change it accordingly */
    
    switch(msgType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:
        case CL_GMS_CLUSTER_LEAVE_MSG:
        case CL_GMS_CLUSTER_EJECT_MSG:
            clLog(DBG,OPN,AIS,
                    "Sending cluster %s multicast message",
                    msgType == CL_GMS_CLUSTER_JOIN_MSG ? "join":
                    msgType == CL_GMS_CLUSTER_LEAVE_MSG ? "leave" : "eject");
            req_exec_gms_nodejoin.ejectReason = ejectReason;
            memcpy (&req_exec_gms_nodejoin.specificMessage.gmsClusterNode, &memberNodeInfo->clusterMember,
                    sizeof (ClGmsClusterMemberT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            break;
        case CL_GMS_GROUP_CREATE_MSG:
        case CL_GMS_GROUP_DESTROY_MSG:
        case CL_GMS_GROUP_JOIN_MSG:
        case CL_GMS_GROUP_LEAVE_MSG:
            clLog(DBG,OPN,AIS,
                    "Sending group %s multicast message",
                    msgType == CL_GMS_GROUP_CREATE_MSG ? "create" : 
                    msgType == CL_GMS_GROUP_DESTROY_MSG ? "destroy" :
                    msgType == CL_GMS_GROUP_JOIN_MSG ? "join" : "leave");
            memcpy (&req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode, 
                    &memberNodeInfo->groupMember,
                    sizeof (ClGmsGroupMemberT));
            memcpy (&req_exec_gms_nodejoin.specificMessage.groupMessage.groupData, 
                    &memberNodeInfo->groupData,
                    sizeof(ClGmsGroupInfoT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            break;
        case CL_GMS_COMP_DEATH:
            clLog(DBG,OPN,AIS,
                    "Sending comp death multicast message");
            memcpy (&req_exec_gms_nodejoin.specificMessage.groupMessage.gmsGroupNode, 
                    &memberNodeInfo->groupMember,
                    sizeof (ClGmsGroupMemberT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            break;
        case CL_GMS_LEADER_ELECT_MSG:
            clLog(DBG,OPN,AIS,
                    "Sending leader elect multicast message");
            memcpy (&req_exec_gms_nodejoin.specificMessage.gmsClusterNode, &memberNodeInfo->clusterMember,
                    sizeof (ClGmsClusterMemberT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            break;
        case CL_GMS_SYNC_MESSAGE:
            clLog(DBG,OPN,AIS,
                    "Sending gms synch multicast message");
            req_exec_gms_nodejoin.dataPtr = dataPtr;
            break;
        case CL_GMS_GROUP_MCAST_MSG:
            memcpy (&req_exec_gms_nodejoin.specificMessage.mcastMessage.groupInfo.gmsGroupNode, 
                    &memberNodeInfo->groupMember,
                    sizeof (ClGmsGroupMemberT));
            memcpy (&req_exec_gms_nodejoin.specificMessage.mcastMessage.groupInfo.groupData,
                    &memberNodeInfo->groupData,
                    sizeof(ClGmsGroupInfoT));
            req_exec_gms_nodejoin.contextHandle = memberNodeInfo->contextHandle;
            req_exec_gms_nodejoin.specificMessage.mcastMessage.userDataSize = dataSize;
            req_exec_gms_nodejoin.dataPtr = dataPtr;
            break;
        default:
            clLog(DBG,OPN,AIS,
                    "Requested wrong message to be multicasted. Message type %d",
                    msgType);
            return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    req_exec_gms_nodejoin.gmsMessageType = msgType;
    req_exec_gms_nodejoin.gmsGroupId = groupId;

    /* Create a buffer handle and marshall the eliments */
    rc = clBufferCreate(&bufferHandle);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to create buffer while sending message on totem. rc 0x%x",rc);
        return rc;
    }

    rc = marshallReqExecGmsNodeJoin(&req_exec_gms_nodejoin,bufferHandle);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to marshall the data while sending message on totem. rc 0x%x",rc);
        goto buffer_delete_return;
    }

    rc = clBufferLengthGet(bufferHandle, &length);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "Failed to get buffer length. rc 0x%x",rc);
        goto buffer_delete_return;
    }

    rc = clBufferFlatten(bufferHandle, &message);
    if (rc != CL_OK)
    {
        clLogError(OPN,AIS,
                "clBufferFlatten failed with rc 0x%x",rc);
        goto buffer_delete_return;
    }

    header.id = SERVICE_ID_MAKE (GMS_SERVICE, MESSAGE_REQ_EXEC_GMS_NODEJOIN);
    header.size = length + sizeof(mar_req_header_t);

    /* We need to prepend the total message length in the beginning of the
     * message so that we can find the length while unmarshalling */
    temp = clHeapAllocate(header.size);
    if (temp == NULL)
    {
        clLogError(OPN,AIS, 
                "Failed to allocate memory while sending the message");
        goto buffer_delete_return;
    }

    memcpy(temp,&header, sizeof(mar_req_header_t));
    memcpy(temp+sizeof(mar_req_header_t), message, length);

    req_exec_gms_iovec.iov_base = temp;
    req_exec_gms_iovec.iov_len = length + sizeof(mar_req_header_t);

    result = totempg_groups_mcast_joined (openais_group_handle, &req_exec_gms_iovec, 1, TOTEMPG_AGREED);

    clLog(DBG,OPN,AIS,
            "Done with sending multicast message of type %d",msgType);

buffer_delete_return:
    if (message != NULL)
        clHeapFree(message);

    if (temp != NULL)
        clHeapFree(temp);

    clBufferDelete(&bufferHandle);
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
    sprintf ((char *)myAddress.value, "%s",totemip_print (this_ip));
    myAddress.length = strlen ((char *)myAddress.value);
#ifndef OPENAIS_TIPC
    if (this_ip->family == AF_INET) {
        myAddress.family = CL_GMS_AF_INET;
    } else {
        if (this_ip->family == AF_INET6) {
            myAddress.family = CL_GMS_AF_INET6;
        } else {
            assert (0);
        }
    }
#else
    myAddress.family = AF_TIPC;
#endif

    /* Also update the global gms data of thisnode with new IP */
    gmsGlobalInfo.config.thisNodeInfo.nodeIpAddress = myAddress;
       
    clLog(DBG,OPN,AIS, "My Local IP address = %s",myAddress.value);
}

