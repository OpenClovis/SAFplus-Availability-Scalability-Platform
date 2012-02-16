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
 * This file contains the definitions of the messages used by the CPM
 * while logging.
 */

/* Horrible way of maintainig log messages. Should go away. */

#ifndef _CL_CPM_LOG_H_
#define _CL_CPM_LOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP header files 
 */
#include <clLogApi.h>

#define CL_CPM_CLIENT_LIB     "cpm"

extern ClCharT *clCpmLogMsg[];

/**
 * CPM-Handle/Version related messages
 */
# define CL_CPM_LOG_HANDLE_MSG_START                 0

/**
 * "Passed handle was not for registering the given component [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_HANDLE_INVALID                 clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+1]

/** 
 * "Unable to create handle, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_CREATE_ERR	            clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+2]

/** 
 * "Unable to create handle database, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_DB_CREATE_ERR	        clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+3]

/**
 * "Unable to destroy handle database, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_DB_DESTROY_ERR	        clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+4]

/**
 * "Unable to get handle for component [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_HANDLE_GET_ERR   	            clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+5]

/**
 * "Unable to checkin handle, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_CHECKIN_ERR  	        clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+6]

/**
 * "Unable to checkout handle, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_CHECKOUT_ERR 	        clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+7]

/**
 * "Unable to create component-to-handle mapping, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_COMP_MAP_ERR 	        clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+8]

/**
 * "Unable to delete the component-to-handle-mapping, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_HANDLE_COMP_MAP_DELETE_ERR     clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+9]

/**
 * "Version mismatch, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_VERSION_MISMATCH          	    clCpmLogMsg[CL_CPM_LOG_HANDLE_MSG_START+10]

# define CL_CPM_LOG_HANDLE_MSG_END                   CL_CPM_LOG_HANDLE_MSG_START+10

/** 
 * CPM-AMF related log messages
 */
# define CL_CPM_LOG_AMF_MSG_START                    CL_CPM_LOG_HANDLE_MSG_END

/**
 * "Unable to initialize AMF callbacks, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_INIT_CB_ERR                clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+1]

/**
 *  "Unable to get selection object, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_SEL_OBJ_GET_ERR           clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+2]

/**
 *  "Unable to invoke the pending callbacks, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_DISPATCH_ERR              clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+3]

/**
 *  "Unable to complete activity for CSI, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_CSI_QSCNG_COMPLETE_ERR    clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+4]

/**
 *  "Unable to get HA state of the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_HA_STATE_GET_ERR          clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+5]

/**
 *  "Unable to track changes in the protection group, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_PG_TRACK_ERR              clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+6]

/**
 *  "Unable to stop tracking changes in the protection group, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_PG_TRACK_STOP_ERR         clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+7]

/**
 *  "Unable to report error on the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_COMP_ERR_REPORT_ERR       clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+8]

/**
 *  "Unable to cancel errors reported on the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_COMP_ERR_CLEAR_ERR       clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+9]

/**
 *  "Unable to respond to the AMF, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_AMF_RESPONSE_ERR             clCpmLogMsg[CL_CPM_LOG_AMF_MSG_START+10]

# define CL_CPM_LOG_AMF_MSG_END                      CL_CPM_LOG_AMF_MSG_START+10

/** 
 * Buffer related log messages
 */
# define CL_CPM_LOG_BUF_MSG_START                CL_CPM_LOG_AMF_MSG_END

/**
 * "Unable to create message, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_CREATE_ERR             clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+1]

/**
 * "Unable to delete message, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_DELETE_ERR             clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+2]

/**
 * "Unable to read message, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_READ_ERR               clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+3]

/**
 * "Unable to write message, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_WRITE_ERR              clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+4]

/**
 * "Unable to get message length, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_LENGTH_ERR             clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+5]

/**
 * "Unable to flatten message, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BUF_FLATTEN_ERR            clCpmLogMsg[CL_CPM_LOG_BUF_MSG_START+6]

# define CL_CPM_LOG_BUF_MSG_END                  CL_CPM_LOG_BUF_MSG_START+6

/**
 * CPM-BM related log messages
 */
# define CL_CPM_LOG_BM_MSG_START                     CL_CPM_LOG_BUF_MSG_END

/**
 * "Unable to initialize the boot manager, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_INIT_ERR                    clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+1]

/**
 * "Returning from the boot manager thread"
 */
# define CL_CPM_LOG_0_BM_THREAD_RET_INFO             clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+2]

/**
 * "Unable to reach default boot level [%d], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_BM_START_ERR                   clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+3]

/**
 * "Unable to set required boot level [%d], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_BM_SET_LEVEL_ERR               clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+4]

/**
 * "Setlevel [%d] succeeded"
 */
# define CL_CPM_LOG_1_BM_SET_LEVEL_INFO              clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+5]

/**
 * "Unable to boot all service units, so can't proceed, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_SET_INVALID_STATE           clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+6]

/**
 * "Unable to start service unit [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_BM_SU_INST_ERR                 clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+7]

/**
 * "Unable to terminate service unit [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_BM_SU_TERM_ERR                 clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+8]

/**
 * "Maximum boot level reached, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_MAX_LEVEL_ERR               clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+9]

/**
 * "Minimum boot level reached, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_MIN_LEVEL_ERR               clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+10]

/**
 * "Unable to initialize the boot table, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_INIT_TABLE_ERR              clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+11]

/**
 * "Unable to add component to the boot table, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_COMP_ADD_ERR                clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+12]

/**
 * "Unable to find bootlevel in the boot table, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_BM_LEVEL_FIND_ERR              clCpmLogMsg[CL_CPM_LOG_BM_MSG_START+13]

# define CL_CPM_LOG_BM_MSG_END                       CL_CPM_LOG_BM_MSG_START+13

/**
 * CPM-CKPT related log messages
 */
# define CL_CPM_LOG_CKPT_MSG_START               CL_CPM_LOG_BM_MSG_END

/**
 * "Unable to initialize checkpoint library, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CKPT_INIT_ERR              clCpmLogMsg[CL_CPM_LOG_CKPT_MSG_START+1]

/**
 * "Unable to create checkpoint, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CKPT_CREATE_ERR            clCpmLogMsg[CL_CPM_LOG_CKPT_MSG_START+2]

/**
 * "Unable to create checkpoint dataSet, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CKPT_DATASET_CREATE_ERR    clCpmLogMsg[CL_CPM_LOG_CKPT_MSG_START+3]

/**
 * "Unable to write checkpoint dataSet [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CKPT_DATASET_WRITE_ERR     clCpmLogMsg[CL_CPM_LOG_CKPT_MSG_START+4]

/**
 * "Unable to consume checkpoint, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CKPT_CONSUME_ERR           clCpmLogMsg[CL_CPM_LOG_CKPT_MSG_START+5]

# define CL_CPM_LOG_CKPT_MSG_END                 CL_CPM_LOG_CKPT_MSG_START+5

/**
 * CPM-Event related messages
 */
# define CL_CPM_LOG_EVT_MSG_START                CL_CPM_LOG_CKPT_MSG_END

/**
 * "Unable to perform event operation, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EVT_CPM_OPER_ERR           clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+1]

/**
 * "Unable to open event channel, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EVT_CHANNEL_OPEN_ERR       clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+2]

/**
 * "Unable to allocate event, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EVT_CHANNEL_ALLOC_ERR      clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+3]

/**
 * "Unable to set event attribute, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EVT_ATTR_SET_ERR           clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+4]

/**
 * "Node [%s] arrival"
 */
# define CL_CPM_LOG_1_EVT_PUB_NODE_ARRIVAL_INFO  clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+5]

/**
 * "Node [%s] departure"
 */
# define CL_CPM_LOG_1_EVT_PUB_NODE_DEPART_INFO   clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+6]

/**
 * "Unable to publish event for node [%s] arrival, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_EVT_PUB_NODE_ARRIVAL_ERR   clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+7]

/**
 * "Unable to publish event for node [%s] departure, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_EVT_PUB_NODE_DEPART_ERR    clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+8]

/**
 * "Publish Event for component [%s] eoPort = [0x%x] failure"
 */
# define CL_CPM_LOG_2_EVT_PUB_COMP_FAIURE_INFO   clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+9]

/**
 * "Unable to publish event for component [%s] failure, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_EVT_PUB_COMP_FAIURE_ERR    clCpmLogMsg[CL_CPM_LOG_EVT_MSG_START+10]

# define CL_CPM_LOG_EVT_MSG_END                  CL_CPM_LOG_EVT_MSG_START+10

/**
 * Container/Search related log messages
 */
# define CL_CPM_LOG_CNT_MSG_START                    CL_CPM_LOG_EVT_MSG_END

/**
 * "Unable to create the container, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CNT_CREATE_ERR                 clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+1]

/**
 * "Unable to delete container node [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CNT_DEL_ERR                    clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+2]

/**
 * "Unable to find entity [%s] named [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR          clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+3]

/**
 * "Unable to get first [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR         clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+4]

/**
 * "Unable to get next [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR          clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+5]

/**
 * "Unable to get user data from the container, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR      clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+6]

/**
 * "Unable to add container node [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CNT_ADD_ERR                    clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+7]

/**
 * "Unable to determine the checksum of entity named [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_CNT_CKSM_ERR                   clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+8]

/**
 * "Unable to get eo object, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EO_OBJECT_ERR                  clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+9]

/**
 * "Unable to get the key size, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CNT_KEY_SIZE_GET_ERR           clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+10]

/**
 * "Unable to create the queue, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_QUEUE_CREATE_ERR               clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+11]

/**
 * "Unable to insert element into the queue, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_QUEUE_INSERT_ERR               clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+12]

/**
 * "Unable to delete element from the queue, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_QUEUE_DELETE_ERR               clCpmLogMsg[CL_CPM_LOG_CNT_MSG_START+13]

# define CL_CPM_LOG_CNT_MSG_END                      CL_CPM_LOG_CNT_MSG_START+13

/**
 * COR-CPM interaction related log messages
 */
# define CL_CPM_LOG_COR_MSG_START            CL_CPM_LOG_CNT_MSG_END

/**
 * "Updated COR with the component and SU State"
 */
# define CL_CPM_LOG_0_COR_UPDATE_INFO        clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+1]

/**
 * "Successfully created the COR object for cluster, node, ServiceUnit and component"
 */
# define CL_CPM_LOG_0_COR_CREATE_INFO        clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+2]

/**
 * "Unable to create COR object [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_COR_CREATE_ERR         clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+3]

/**
 * "Unable to set the [%s] attribute in COR for [%s] object, rc=[0x%x]"
 */
# define CL_CPM_LOG_3_COR_UPDATE_ERR         clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+4]

/**
 * "Unable to create MOID, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_COR_MOID_CREATE_ERR    clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+5]

/**
 * "Unable to add entry to the MOID, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_COR_MOID_APPEND_ERR    clCpmLogMsg[CL_CPM_LOG_COR_MSG_START+6]

# define CL_CPM_LOG_COR_MSG_END                  CL_CPM_LOG_COR_MSG_START+6

/** 
 * CPM-LCM related log messages
 */
# define CL_CPM_LOG_LCM_MSG_START                CL_CPM_LOG_COR_MSG_END

/**
 * "Instantiating CompName [%s] imageName [%s]"
 */
# define CL_CPM_LOG_2_LCM_COMP_INST_INFO         clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+1]

/**
 * "Terminating CompName [%s] via eoPort [0x%x]"
 */
# define CL_CPM_LOG_2_LCM_COMP_TERM_INFO         clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+2]

/**
 * "Cleaning up the component [%s]"
 */
# define CL_CPM_LOG_1_LCM_COMP_CLEANUP_INFO      clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+3]

/**
 * "Restarting the component [%s]"
 */
# define CL_CPM_LOG_1_LCM_COMP_RESTART_INFO      clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+4]

/**
 * "%s proxied component [%s] so sending RMD to [0x%x]"
 */
# define CL_CPM_LOG_3_LCM_PROXY_OPER_INFO        clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+5]

/**
 * "Component [%s] did not %s within the specified limit"
 */
# define CL_CPM_LOG_2_LCM_COMP_OPER_ERR          clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+6]

/**
 * "Unable to %s component [%s] rc=[0x%x]"
 */
# define CL_CPM_LOG_3_LCM_COMP_OPER1_ERR         clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+7]

/**
 * "Unable to forward the LCM request to IOC node address [0x%x] port [0x%x] rc = [0x%x]"
 */
# define CL_CPM_LOG_3_LCM_OPER_FWD_ERR           clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+8]

/**
 * "%s Component [%s] via eoPort [0x%x]"
 */
# define CL_CPM_LOG_3_LCM_COMP_REG_INFO          clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+9]

/**
 * "%s Proxied component [%s] via Component: Name = [%s] Port [0x%x]"
 */
# define CL_CPM_LOG_4_LCM_PROXY_COMP_REG_INFO    clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+10]

/**
 * "Already unregistered this component [%s]"
 */
# define CL_CPM_LOG_1_LCM_REG_MULTI_ERR          clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+11]

/**
 * "This component [%s] is proxy but has not unregistered all the proxied component"
 */
# define CL_CPM_LOG_1_LCM_NOT_UNREG_PROXY_ERR    clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+12]

/**
 * "Proxy Unavailable for proxied component [%s]"
 */
# define CL_CPM_LOG_1_LCM_PROXY_UNAVAILABLE_ERR  clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+13]

/**
 * "Node [%s] is down so unable to serve the request, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_LCM_NODE_UNAVAILABLE_ERR   clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+14]

/**
 * "Not enough information is available to reach the node [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_LCM_NODE_NOT_REG_ERR       clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+15]

/**
 * "Unable to get the name of the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR      clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+16]

/**
 * CPM related log messages
 */
# define CL_CPM_LOG_1_LCM_MULT_REG_REQ_ERR       clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+17]

/**
 * "Same registration request is performed multiple times for component [%s]"
 */
# define CL_CPM_LOG_4_LCM_CSI_SET_INFO           clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+18]

/**
 * "CSI set for compName [%s] haState [%d] length [%d] invocation [%d]"
 */
# define CL_CPM_LOG_1_LCM_CSI_DESC_UNPACK_ERR    clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+19]

/**
 * "Unable to unpack CSI descriptor, rc=[0x%x]"
 */
# define CL_CPM_LOG_0_LCM_INVOKE_CALLBACK_INFO   clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+20]

/**
 * "Invoke the callback function"
 */
# define CL_CPM_LOG_3_LCM_RESPONSE_FAILED        clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+21]

/**
 * "Unable to respond to the caller, ioc node Address [0x%x] and port [0x%x] for LCM request, rc=[0x%x]"
 */
# define CL_CPM_LOG_3_LCM_INVALID_PROXY_ERR      clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+22]

/**
 * "[%s] is not the proxy for [%s], so can not unregister, actual proxy is [%s]"
 */
# define CL_CPM_LOG_0_LCM_INST_SCRIPT_ERR        clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+23]

/**
 * "script based instantiation is not supported"
 */
# define CL_CPM_LOG_2_LCM_CSI_RMV_INFO           clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+24]

/**
 * "Unable to perform heartbeat on eoID [0x%x] port [0x%x] at [%s]"
 */
# define CL_CPM_LOG_3_LCM_EO_HB_FAILURE          clCpmLogMsg[CL_CPM_LOG_LCM_MSG_START+25]

# define CL_CPM_LOG_LCM_MSG_END                  CL_CPM_LOG_LCM_MSG_START+25

/**
 * CPM server related log messages
 */
# define CL_CPM_LOG_SERVER_MSG_START                         CL_CPM_LOG_LCM_MSG_END

/**
 * "Initializing the CPM server"
 */
# define CL_CPM_LOG_0_SERVER_COMP_MGR_INIT_INFO              clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+1]

/**
 * "Unable to initialize CPM server, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_COMP_MGR_INIT_ERR               clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+2]

/**
 * "Unable to install signal handler, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_SIG_HANDLER_INSTALL_ERR         clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+3]

/**
 * "Unable to allocate CPM data structure, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_CPM_ALLOCATE_ERR                clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+4]

/**
 * "Unable to get CPM configuration, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_CPM_CONFIG_GET_ERR              clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+5]

/**
 * "Unable to create directory, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_DIR_CREATE_ERR                  clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+6]

/**
 * "Creating the CPM execution object"
 */
# define CL_CPM_LOG_0_SERVER_CPM_EO_CREATE_INFO              clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+7]

/**
 * "Installing function tables of component manager"
 */
# define CL_CPM_LOG_0_SERVER_FUNC_TABLE_INSTALL_INFO         clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+8]

/**
 * "Unable to initialize Event, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_EVENT_INIT_ERR                  clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+9]

/**
 * "Finalizing the CPM server"
 */
# define CL_CPM_LOG_0_SERVER_COMP_MGR_CLEANUP_INFO           clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+10]

/**
 * "Shutting down the node [%s]"
 */
# define CL_CPM_LOG_1_SERVER_COMP_MGR_NODE_SHUTDOWN_INFO     clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+11]

/**
 * "Received node shutdown request"
 */
# define CL_CPM_LOG_1_SERVER_CPM_NODE_SHUTDOWN_INFO          clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+12]

/**
 * "Unable to find EO, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_EO_UNREACHABLE                  clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+13]

/**
 * "Exitting the polling thread"
 */
# define CL_CPM_LOG_0_SERVER_POLL_THREAD_EXIT_INFO           clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+14]

/**
 * "Unable to forward the request to the required node, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_FORWARD_ERR                     clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+15]

/**
 * "Unable to perform the requested operation, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SERVER_ERR_OPERATION_FAILED            clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+16]

/**
 * "Unable to configure [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_2_SERVER_CONFIG_ERR                      clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+17]

/**
 * "Unable to route request for [%s] to [%s], rc=[0x%x]"
 */
# define CL_CPM_LOG_3_SERVER_REQ_ROUTE_ERR                   clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+18]

/**
 * "Unable to execv the child : [%s]"
 */
# define CL_CPM_LOG_1_SERVER_EXECV_ERR                       clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+19]

/**
 * "Shutting down node because of node type mismatch..."
 */
# define CL_CPM_LOG_0_SERVER_NODETYPE_MISMATCH_INFO          clCpmLogMsg[CL_CPM_LOG_SERVER_MSG_START+20]

# define CL_CPM_LOG_SERVER_MSG_END                           CL_CPM_LOG_SERVER_MSG_START+20

/**
 * CPM client related log messages
 */
# define CL_CPM_LOG_CLIENT_MSG_START                         CL_CPM_LOG_SERVER_MSG_END

/**
 * "Unable to initialize the CPM client, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_INIT_ERR                        clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+1]

/**
 * "Successfully initialized the CPM client"
 */
# define CL_CPM_LOG_0_CLIENT_INIT_INFO                       clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+2]

/**
 * "Unable to finalize the CPM client, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_FINALIZE_ERR                    clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+3]

/**
 * "Unable to register the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_REG_ERR                    clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+4]

/**
 * "Unable to unregister the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_UNREG_ERR                  clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+5]

/**
 * "Unable to report the failure of the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_FAILURE_REPORT_ERR         clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+6]

/**
 * "Unable to clear the failure of the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_FAILURE_CLEAR_ERR          clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+7]

/**
 * "Unable to get HA state of the component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_HA_STATE_GET_ERR           clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+8]

/**
 * "Unable to do callback response, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_CB_RESPONSE_ERR            clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+9]

/**
 * "Unable to do quiescing complete, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_QUIESCING_ERR              clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+10]

/**
 * "Unable to do create component handle mapping, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_HANDLE_MAP_CREATE_ERR      clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+11]

/**
 * "Unable to do delete component handle mapping, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_HANDLE_MAP_DELETE_ERR      clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+12]

/**
 * "Unable to find the entry, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_ERR_DOESNT_EXIST                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+13]

/**
 * "Timed out, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_ERR_TIMEOUT                     clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+14]

/**
 * "Unable to initialize the CPM properly, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_ERR_CPM_INIT                    clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+15]

/**
 * "CPM received an invalid request, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_ERR_BAD_OPERATION               clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+16]

/**
 * "Unable to set EO state, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_EO_STATE_SET_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+17]

/**
 * "Unable to register EO, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_EO_REG_ERR                      clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+18]

/**
 * "Unable to function walk EO, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_EO_FUNC_WALK_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+19]

/**
 * "Unable to update EO state , rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_EO_STATE_UPDATE_ERR             clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+20]

/**
 * "Unable to update component logical address , rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_LA_UPDATE_ERR              clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+21]

/**
 * "Unable to get component process ID, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_PID_GET_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+22]

/**
 * "Unable to get component address, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_ADDR_GET_ERR               clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+23]

/**
 * "Unable to get component ID, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_ID_GET_ERR                 clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+24]

/**
 * "Unable to get component status, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_STATUS_GET_ERR             clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+25]

/**
 * "Unable to restart service unit, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_SU_RESTART_ERR                  clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+26]

/**
 * "Unable to register CPM/L, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_CPML_REG_ERR                    clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+27]

/**
 * "Unable to unregister CPM/L, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_CPML_UNREG_ERR                  clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+28]

/**
 * "Unable to get current boot level, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_BM_GET_LEVEL_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+29]

/**
 * "Unable to set current boot level, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_BM_SET_LEVEL_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+30]

/**
 * "Unable to get maximum boot level, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_BM_GET_MAX_LEVEL_ERR            clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+31]

/**
 * "Unable to instantiate component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_INSTANTIATE_ERR            clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+32]

/**
 * "Unable to terminate component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_TERMINATE_ERR              clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+33]

/**
 * "Unable to cleanup component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_CLEANUP_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+34]

/**
 * "Unable to restart component, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_COMP_RESTART_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+35]

/**
 * "Unable to instantiate service unit, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_SU_INSTANTIATE_ERR              clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+36]

/**
 * "Unable to terminate service unit, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_SU_TERMINATE_ERR                clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+37]

/**
 * "Unable to shutdown the node, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLIENT_NODE_SHUTDOWN                   clCpmLogMsg[CL_CPM_LOG_CLIENT_MSG_START+38]

# define CL_CPM_LOG_CLIENT_MSG_END                           CL_CPM_LOG_CLIENT_MSG_START+38

/** 
 * CPM-EO related log messages
 */
# define CL_CPM_LOG_EO_MSG_START             CL_CPM_LOG_CLIENT_MSG_END

/**
 * "Unable to create EO, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EO_CREATE_ERR          clCpmLogMsg[CL_CPM_LOG_EO_MSG_START+1]

/**
 * "Unable to install function table for the client, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EO_CLIENT_INST_ERR     clCpmLogMsg[CL_CPM_LOG_EO_MSG_START+2]

/**
 * "Unable to delete user callout function, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EO_DEL_CALL_OUT_ERR    clCpmLogMsg[CL_CPM_LOG_EO_MSG_START+3]

/**
 * "Unable to set EO object, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_EO_MY_OBJ_SET_ERR      clCpmLogMsg[CL_CPM_LOG_EO_MSG_START+4]

# define CL_CPM_LOG_EO_MSG_END               CL_CPM_LOG_EO_MSG_START+4

/** 
 * CPM parser related log messages
 */
# define CL_CPM_LOG_PARSER_MSG_START                     CL_CPM_LOG_EO_MSG_END

/**
 * "Unable to parse the XML file, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_PARSER_FILE_PARSE_ERR              clCpmLogMsg[CL_CPM_LOG_PARSER_MSG_START+1]

/**
 * "Invalid [%s] value in tag or attribute, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_PARSER_INVALID_VAL_ERR             clCpmLogMsg[CL_CPM_LOG_PARSER_MSG_START+2]

/**
 * "Unable to parse [%s] information, rc=[0x%x]"
 */
# define CL_CPM_LOG_2_PARSER_INFO_PARSE_ERR              clCpmLogMsg[CL_CPM_LOG_PARSER_MSG_START+3]

# define CL_CPM_LOG_PARSER_MSG_END                       CL_CPM_LOG_PARSER_MSG_START+3

/**
 * debug related log messages
 */
# define CL_CPM_LOG_DEBUG_MSG_START                  CL_CPM_LOG_PARSER_MSG_END

/**
 * "Unable to register with debug, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_DEBUG_REG_ERR                  clCpmLogMsg[CL_CPM_LOG_DEBUG_MSG_START+1]

# define CL_CPM_LOG_DEBUG_MSG_END                    CL_CPM_LOG_DEBUG_MSG_START+1

/**
 * IOC related log messages
 */
# define CL_CPM_LOG_IOC_MSG_START                        CL_CPM_LOG_DEBUG_MSG_END

/**
 * "Unable to unblock comm port receiver, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_IOC_COMMPORT_UNBLOCK_ERR           clCpmLogMsg[CL_CPM_LOG_IOC_MSG_START+1]

/**
 * "Unable to get IOC port, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR         clCpmLogMsg[CL_CPM_LOG_IOC_MSG_START+2]

/**
 * "Unable to set IOC port, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_SET_ERR         clCpmLogMsg[CL_CPM_LOG_IOC_MSG_START+3]

# define CL_CPM_LOG_IOC_MSG_END                          CL_CPM_LOG_IOC_MSG_START+3

/**
 * TL related log messages
 */
# define CL_CPM_LOG_TL_MSG_START                        CL_CPM_LOG_IOC_MSG_END

/**
 * "Updating TL succeeded"
 */
# define CL_CPM_LOG_0_TL_UPDATE_INFO                    clCpmLogMsg[CL_CPM_LOG_TL_MSG_START+1]

/**
 * "Unable to update the TL, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_TL_UPDATE_FAILURE                 clCpmLogMsg[CL_CPM_LOG_TL_MSG_START+2]

# define CL_CPM_LOG_TL_MSG_END                          CL_CPM_LOG_TL_MSG_START+2

/**
 * GMS related log messages
 */
# define CL_CPM_LOG_GMS_MSG_START           CL_CPM_LOG_TL_MSG_END

/**
 * "Unable to join the cluster, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_CLUSTER_JOIN_ERR          clCpmLogMsg[CL_CPM_LOG_GMS_MSG_START+1]

/**
 * "Waiting for the track callback..."
 */
# define CL_CPM_LOG_0_TRACK_CB_WAIT_INFO        clCpmLogMsg[CL_CPM_LOG_GMS_MSG_START+2]

/**
 * "Failed to receive the GMS track callback..."
 */
# define CL_CPM_LOG_0_TRACK_CB_RECV_FAILURE     clCpmLogMsg[CL_CPM_LOG_GMS_MSG_START+3]

/**
 * "Unable to initialize GMS, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_GMS_INIT_ERR              clCpmLogMsg[CL_CPM_LOG_GMS_MSG_START+4]

/**
 * "Doing active initiated switch over, calling cluster leave..."
 */
# define CL_CPM_LOG_0_CLUSTER_LEAVE_INFO        clCpmLogMsg[CL_CPM_LOG_GMS_MSG_START+5]

# define CL_CPM_LOG_GMS_MSG_END             CL_CPM_LOG_GMS_MSG_START+5

/**
 * OSAL related log messages
 */
# define CL_CPM_LOG_OSAL_MSG_START           CL_CPM_LOG_GMS_MSG_END

/**
 * "Unable to create mutex, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR  clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+1]

/**
 * "Unable to delete mutex, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_MUTEX_DELETE_ERR  clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+2]

/**
 * "Unable to lock mutex, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR    clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+3]

/**
 * "Unable to unlock mutex, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR  clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+4]

/**
 * "Unable to create condition variable, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_COND_CREATE_ERR   clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+5]

/**
 * "Unable to delete condition variable, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_COND_DELETE_ERR   clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+6]

/**
 * "Unable to signal condition variable, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR   clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+7]

/**
 * "Unable to create task, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_OSAL_TASK_CREATE_ERR   clCpmLogMsg[CL_CPM_LOG_OSAL_MSG_START+8]

# define CL_CPM_LOG_OSAL_MSG_END             CL_CPM_LOG_OSAL_MSG_START+8

/**
 * RMD related log messages
 */
# define CL_CPM_LOG_RMD_MSG_START        CL_CPM_LOG_OSAL_MSG_END

/**
 * "Unable to make RMD call, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_RMD_CALL_ERR       clCpmLogMsg[CL_CPM_LOG_RMD_MSG_START+1]

# define CL_CPM_LOG_RMD_MSG_END          CL_CPM_LOG_RMD_MSG_START+1

/**
 * Timer related log messages
 */
# define CL_CPM_LOG_TIMER_MSG_START      CL_CPM_LOG_RMD_MSG_END

/**
 * "Timer is not initialized, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_TIMER_INIT_ERR     clCpmLogMsg[CL_CPM_LOG_TIMER_MSG_START+1]

/**
 * "Unable to create timer, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_TIMER_CREATE_ERR   clCpmLogMsg[CL_CPM_LOG_TIMER_MSG_START+2]

/**
 * "Unable to start timer, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_TIMER_START_ERR    clCpmLogMsg[CL_CPM_LOG_TIMER_MSG_START+3]

/**
 * "Unable to stop timer, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_TIMER_STOP_ERR     clCpmLogMsg[CL_CPM_LOG_TIMER_MSG_START+4]

# define CL_CPM_LOG_TIMER_MSG_END        CL_CPM_LOG_TIMER_MSG_START+4

/**
 * Miscellaneous log messages
 */
# define CL_CPM_LOG_MISC_MSG_START       CL_CPM_LOG_TIMER_MSG_END

/**
 * "Unable to create SHM area, rc=[0x%x]"
 */
# define CL_CPM_LOG_1_SHM_CREATE_ERR     clCpmLogMsg[CL_CPM_LOG_MISC_MSG_START+1]

/**
 * "ASP_BINDIR path is not set in the environment"
 */
# define CL_CPM_LOG_0_ASP_BINDIR_PATH_ERR  clCpmLogMsg[CL_CPM_LOG_MISC_MSG_START+2]

# define CL_CPM_LOG_MISC_MSG_END         CL_CPM_LOG_MISC_MSG_START+2

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_CPM_LOG_H_ */
