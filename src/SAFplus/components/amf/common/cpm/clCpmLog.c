/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
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

/**
 * This file contains the messages used by the CPM while logging.
 */

/* Horrible way of maintainig log messages. Should go away. */

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCpmLog.h>

ClCharT *clCpmLogMsg[] = {
    "Invalid message",
    /*
     * CL_CPM_LOG_HANDLE_MSG_START: Handle/Version related log messages 
     */
    "Passed handle was not for registering the given component [%s], rc=[0x%x]",    /* ERROR 
                                                                                     */
    "Unable to create handle, rc=[0x%x]",   /* DEBUG */
    "Unable to create handle database, rc=[0x%x]",  /* DEBUG */
    "Unable to destroy handle database, rc=[0x%x]", /* DEBUG */
    "Unable to get handle for component [%s], rc=[0x%x]",   /* DEBUG */
    "Unable to checkin handle, rc=[0x%x]",  /* DEBUG */
    "Unable to checkout handle, rc=[0x%x]", /* DEBUG */
    "Unable to create component-to-handle mapping, rc=[0x%x]",  /* DEBUG */
    "Unable to delete the component-to-handle-mapping, rc=[0x%x]",  /* DEBUG */
    "Version mismatch, rc=[0x%x]",  /* ERROR */

    /*
     * CL_CPM_LOG_AMF_MSG_START: CPM-AMF related messages
     */
    "Unable to initialize AMF callbacks, rc=[0x%x]",    /* DEBUG */
    "Unable to get selection object, rc=[0x%x]",
    "Unable to invoke the pending callbacks, rc=[0x%x]",
    "Unable to complete activity for CSI, rc=[0x%x]",
    "Unable to get HA state of the component, rc=[0x%x]",
    "Unable to track changes in the protection group, rc=[0x%x]",
    "Unable to stop tracking changes in the protection group, rc=[0x%x]",
    "Unable to report error on the component, rc=[0x%x]",
    "Unable to cancel errors reported on the component, rc=[0x%x]",
    "Unable to respond to the AMF, rc=[0x%x]",

    /*
     * CL_CPM_LOG_BUF_MSG_START: Buffer related log messages 
     */
    "Unable to create message, rc=[0x%x]",
    "Unable to delete message, rc=[0x%x]",
    "Unable to read message, rc=[0x%x]",
    "Unable to write message, rc=[0x%x]",
    "Unable to get message length, rc=[0x%x]",
    "Unable to flatten message, rc=[0x%x]",

    /*
     * CL_CPM_LOG_BM_MSG_START: CPM-BM related log messages 
     */
    "Unable to initialize the CPM-BM sub-module, rc=[0x%x]",
    "Returning from the CPM-BM thread",
    "Unable to reach default boot level [%d], rc=[0x%x]",
    "Unable to set required boot level [%d], rc=[0x%x]",
    "Setlevel [%d] succeeded",
    "Unable to boot all service units, so can't proceed, rc=[0x%x]",
    "Unable to start service unit [%s], rc=[0x%x]",
    "Unable to terminate service unit [%s], rc=[0x%x]",
    "Maximum boot level reached, rc=[0x%x]",
    "Minimum boot level reached, rc=[0x%x]",
    "Unable to initialize the boot table, rc=[0x%x]",
    "Unable to add component to the boot table, rc=[0x%x]",
    "Unable to find bootlevel in the boot table, rc=[0x%x]",

    /*
     * Checkpointing related log messages 
     */
    "Unable to initialize checkpoint library, rc=[0x%x]",   /* ERROR */
    "Unable to create checkpoint, rc=[0x%x]",   /* ERROR */
    "Unable to create checkpoint dataSet, rc=[0x%x]",   /* ERROR */
    "Unable to write checkpoint dataSet [%s], rc=[0x%x]",   /* ERROR */
    "Unable to consume checkpoint, rc=[0x%x]",  /* ERROR */

    /*
     * CL_CPM_LOG_EVT_MSG_START: Event related log messages 
     */
    "Unable to perform event operation, rc=[0x%x]",
    "Unable to open event channel, rc=[0x%x]",  /* ERROR */
    "Unable to allocate event, rc=[0x%x]",  /* ERROR */
    "Unable to set event attribute, rc=[0x%x]", /* ERROR */
    "Node [%s] arrival",        /* ALERT */
    "Node [%s] departure",      /* ALERT */
    "Unable to publish event for node [%s] arrival, rc=[0x%x]", /* ERROR */
    "Unable to publish event for node [%s] departure, rc=[0x%x]",   /* ERROR */
    "Publish Event for component [%s] eoPort = [0x%x] failure", /* INFO */
    "Unable to publish event for component [%s] failure, rc=[0x%x]",    /* ERROR 
                                                                         */

    /*
     * CL_CPM_FIND_MSG_START: Container/Search related log messages 
     */
    "Unable to create the container, rc=[0x%x]",    /* DEBUG */
    "Unable to delete container node [%s], rc=[0x%x]",  /* DEBUG */
    "Unable to find entity [%s] named [%s], rc=[0x%x]", /* WARNING */
    "Unable to get first [%s], rc=[0x%x]",  /* ERROR */
    "Unable to get next [%s], rc=[0x%x]",   /* ERROR */
    "Unable to get user data from the container, rc=[0x%x]",    /* ERROR */
    "Unable to add container node [%s], rc=[0x%x]", /* DEBUG */
    "Unable to determine the checksum of entity named [%s], rc=[0x%x]", /* DEBUG 
                                                                         */
    "Unable to get eo object, rc=[0x%x]",   /* DEBUG */
    "Unable to get the key size, rc=[0x%x]",    /* DEBUG */
    "Unable to create the queue, rc=[0x%x]",    /* DEBUG */
    "Unable to insert element into the queue, rc=[0x%x]",   /* DEBUG */
    "Unable to delete element from the queue, rc=[0x%x]",   /* DEBUG */

    /*
     * CL_CPM_COR_MSG_START: COR-CPM interaction related log messages 
     */
    "Updated COR with the component and SU State",  /* INFO */
    "Successfully created the COR object for cluster, node, ServiceUnit and component", /* INFO */
    "Unable to create COR object [%s], rc=[0x%x]",  /* ERROR */
    "Unable to set the [%s] attribute in COR for [%s] object, rc=[0x%x]",   /* ERROR 
                                                                             */
    "Unable to create MOID, rc=[0x%x]", /* ERROR */
    "Unable to add entry to the MOID, rc=[0x%x]",   /* ERROR */

    /*
     * CL_CPM_LOG_LCM_MSG_START: CPM_LCM related log messages 
     */
    "Instantiating CompName [%s] imageName [%s]",   /* INFO */
    "Terminating CompName [%s] via eoPort [0x%x]",  /* INFO */
    "Cleaning up the component [%s]",   /* INFO */
    "Restarting the component [%s]",    /* INFO */
    "%s proxied component [%s] so sending RMD to [0x%x]",   /* Instantiating/Terminating/cleaning 
     *//*
     * INFO
     */
    "Component [%s] did not %s within the specified limit", /* Instantiated/Terminated/cleanedup 
     *//*
     * ERROR 
     */
    "Unable to %s component [%s] rc=[0x%x]",    /* Instantiate/terminate/cleanup/restart 
     *//*
     * ERROR 
     */
    "Unable to forward the LCM request to IOC node address [0x%x] port [0x%x] rc = [0x%x]", /* ERROR 
                                                                                             */
    "%s component [%s] via eoPort [0x%x]",  /* Registration/Unregistration *//* INFO */
    "%s proxied component [%s] via Component: Name = [%s] Port [0x%x]", /* Registration/Unregistration 
     *//*
     * INFO 
     */
    "Already unregistered this component [%s]", /* ERROR */
    "This component [%s] is proxy but has not unregistered all the proxied component",  /* ERROR 
                                                                                         */
    "Proxy unavailable for proxied component [%s]", /* ERROR */
    "Node [%s] is down so unable to serve the request, rc=[0x%x]",  /* ERROR */
    "Not enough information is available to reach the node [%s], rc=[0x%x]",
    "Unable to get the name of the component, rc=[0x%x]",   /* ERROR */
    /*
     * CPM related log messages 
     */
    "Same registration request is performed multiple times for component [%s]", /* ERROR 
                                                                                 */
    "CSI set for compName [%s] haState [%d] length [%d] invocation [%d]",
    "Unable to unpack CSI descriptor, rc=[0x%x]",
    "Invoke the callback function",
    "Unable to respond to the caller, ioc node Address [0x%x] and port [0x%x] for LCM request, rc=[0x%x]",
    "[%s] is not the proxy for [%s], so can not unregister, actual proxy is [%s]",
    "script based instantiation is not supported",
    "CSI remove for compName [%s] csiName [%s]",
    "Unable to perform heartbeat on eoID [0x%x] port [0x%x] at [%s]",

    /*
     * CL_CPM_LOG_SERVER_MSG_START: CPM server related log messages 
     */
    "Initializing the CPM server",
    "Unable to initialize CPM server, rc=[0x%x]",
    "Unable to install signal handler, rc=[0x%x]",
    "Unable to allocate CPM data structure, rc=[0x%x]",
    "Unable to get CPM configuration, rc=[0x%x]",
    "Unable to create directory, rc=[0x%x]",
    "Creating the CPM execution object",
    "Installing function tables of component manager",
    "Unable to initialize Event, rc=[0x%x]",    /* DEBUG */
    "Finalizing the CPM server",    /* DEBUG */
    "Shutting down the node [%s]",  /* ALERT */
    "Received node shutdown request for ioc Address [%x]",   /* ALERT */
    "Unable to find EO, rc=[0x%x]", /* DEBUG */
    "Exitting the polling thread",  /* DEBUG */
    "Unable to forward the request to the required node, rc=[0x%x]",
    "Unable to perform the requested operation, rc=[0x%x]",
    "Unable to configure [%s], rc=[0x%x]",
    "Unable to route request for [%s] to [%s], rc=[0x%x]",
    "Unable to execv the child : [%s]",
    "Shutting down node because of node type mismatch...",

    /*
     * CL_CPM_LOG_CLIENT_MSG_START: CPM client related log messages 
     */
    "Unable to initialize the CPM client, rc=[0x%x]",
    "Successfully initialized the CPM client",
    "Unable to finalize the CPM client, rc=[0x%x]",
    "Unable to register the component, rc=[0x%x]",
    "Unable to unregister the component, rc=[0x%x]",
    "Unable to report the failure of the component, rc=[0x%x]",
    "Unable to clear the failure of the component, rc=[0x%x]",
    "Unable to get HA state of the component, rc=[0x%x]",
    "Unable to do callback response, rc=[0x%x]",
    "Unable to do quiescing complete, rc=[0x%x]",
    "Unable to do create component handle mapping, rc=[0x%x]",
    "Unable to do delete component handle mapping, rc=[0x%x]",
    "Unable to find the entry, rc=[0x%x]",
    "Timed out, rc=[0x%x]",
    "Unable to initialize the CPM properly, rc=[0x%x]",
    "CPM received an invalid request, rc=[0x%x]",
    "Unable to set EO state, rc=[0x%x]",
    "Unable to register EO, rc=[0x%x]",
    "Unable to function walk EO, rc=[0x%x]",
    "Unable to update EO state , rc=[0x%x]",
    "Unable to update component logical address , rc=[0x%x]",
    "Unable to get component process ID, rc=[0x%x]",
    "Unable to get component address, rc=[0x%x]",
    "Unable to get component ID, rc=[0x%x]",
    "Unable to get component status, rc=[0x%x]",
    "Unable to restart service unit, rc=[0x%x]",
    "Unable to register CPM/L, rc=[0x%x]",
    "Unable to unregister CPM/L, rc=[0x%x]",
    "Unable to get current boot level, rc=[0x%x]",
    "Unable to set current boot level, rc=[0x%x]",
    "Unable to get maximum boot level, rc=[0x%x]",
    "Unable to instantiate component, rc=[0x%x]",
    "Unable to terminate component, rc=[0x%x]",
    "Unable to cleanup component, rc=[0x%x]",
    "Unable to restart component, rc=[0x%x]",
    "Unable to instantiate service unit, rc=[0x%x]",
    "Unable to terminate service unit, rc=[0x%x]",
    "Unable to shutdown the node, rc=[0x%x]",

    /*
     * CL_CPM_LOG_EO_MSG_START: CPM-EO related log messages 
     */
    "Unable to create EO, rc=[0x%x]",   /* DEBUG */
    "Unable to install function table for the client, rc=[0x%x]",   /* DEBUG */
    "Unable to delete user callout function, rc=[0x%x]",    /* DEBUG */
    "Unable to set EO object, rc=[0x%x]",   /* DEBUG */

    /*
     * CL_CPM_LOG_PARSER_MSG_START: CPM parser related log messages 
     */
    "Unable to parse the XML file, rc=[0x%x]",
    "Invalid [%s] value in tag or attribute, rc=[0x%x]",
    "Unable to parse [%s] information, rc=[0x%x]",

    /*
     * CL_CPM_LOG_DEBUG_MSG_START: debug related log messages 
     */
    "Unable to register with debug, rc=[0x%x]", /* ERROR */

    /*
     * CL_CPM_LOG_IOC_MSG_START: IOC related log messages 
     */
    "Unable to unblock comm port receiver, rc=[0x%x]",
    "Unable to get IOC port, rc=[0x%x]",
    "Unable to set IOC port, rc=[0x%x]",

    /*
     * CL_CPM_LOG_TL_MSG_START: TL related log messages 
     */
    "Updating TL succeeded",
    "Unable to update the TL, rc=[0x%x]",

    /*
     * CL_CPM_LOG_GMS_MSG_START: GMS related log messages 
     */
    "Unable to join the cluster, rc=[0x%x]",
    "Waiting for the track callback...",
    "Failed to receive the GMS track callback...",
    "Unable to initialize GMS, rc=[0x%x]",
    "Doing active initiated switch over, calling cluster leave...",

    /*
     * CL_CPM_LOG_OSAL_MSG_START: OSAL related log messages 
     */
    "Unable to create mutex, rc=[0x%x]",    /* DEBUG */
    "Unable to delete mutex, rc=[0x%x]",    /* DEBUG */
    "Unable to lock mutex, rc=[0x%x]",  /* DEBUG */
    "Unable to unlock mutex, rc=[0x%x]",    /* DEBUG */
    "Unable to create condition variable, rc=[0x%x]",   /* DEBUG */
    "Unable to delete condition variable, rc=[0x%x]",   /* DEBUG */
    "Unable to signal condition variable, rc=[0x%x]",   /* DEBUG */
    "Unable to create task, rc=[0x%x]", /* DEBUG */

    /*
     * CL_CPM_LOG_RMD_MSG_START: RMD related log messages 
     */
    "Unable to make RMD call, rc=[0x%x]",   /* DEBUG */

    /*
     * CL_CPM_TIMER_MSG_START: Timer related log messages 
     */
    "Timer is not initialized, rc=[0x%x]",  /* DEBUG */
    "Unable to create timer, rc=[0x%x]",    /* DEBUG */
    "Unable to start timer, rc=[0x%x]", /* DEBUG */
    "Unable to stop timer, rc=[0x%x]",  /* DEBUG */

    /*
     * CL_CPM_MISC_MSG_START: Misc log messages 
     */
    "Unable to create SHM area, rc=[0x%x]", /* DEBUG */
    "ASP_BINDIR path is not set in the environment",
};

