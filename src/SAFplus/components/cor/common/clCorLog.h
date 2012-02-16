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
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains LOG related definitions of COR.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_LOG_H_
#define _CL_COR_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>                                                                               

/* GLOBAL DEFINITIONS */
extern ClCharT *gCorClientLibName;

extern ClCharT *clCorLogMsg[];

/***   Macros corresponding to the log messages *****/
/**
 * " Failed to Initialize %s. rc [0x%x]"  
 */
#define CL_LOG_MESSAGE_2_INIT					                    clCorLogMsg[0] 

/**
 * " Failed to register COR-Service with transaction-agent. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_REGISTER_WITH_TRANSACTION_AGENT	        clCorLogMsg[1]

/**
 * " Failed to process all the transaction-jobs. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_PROCESS_TRANSACTION_JOBS		            clCorLogMsg[2]

/**
 *  " Failed to get component-list from route-manager or components interested. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_GET_COMP_LIST				                clCorLogMsg[3]

/**
 * " Failed to set component-addresss for this job. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_SET_COMP_ADDRESS			                clCorLogMsg[4]

/**
 *  " Failed to create COR Class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CLASS_CREATE				                clCorLogMsg[5]

/**
 * " Failed to delete COR Class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CLASS_DELETE				                clCorLogMsg[6]

/**
 *  " Failed to create COR Class attribute. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_ATTR_CREATE				                clCorLogMsg[7]

/**
 * " Failed to delete COR Class attribute. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_ATTR_DELETE				                clCorLogMsg[8]

/**
 *  " Failed to set COR Class attribute value. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_ATTR_VALUE_SET				                clCorLogMsg[9]

/**
 *  " Failed to set COR Class attribute flag. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_ATTR_FLAG_SET				                clCorLogMsg[10]

/**
 * " Failed to get COR Class attribute. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_ATTR_GET 				                    clCorLogMsg[11]

/**
 * " Data synchronization with master COR Failed.   rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_DATA_SYNC_MASTER_COR			            clCorLogMsg[12]

/**
 *  " Data synchronization with master COR completed succesfully." 
 */
#define CL_LOG_MESSAGE_0_SYNC_COMPLETE 				                clCorLogMsg[13]

/**
 *  " Data restoration from persistent database Failed.  rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_DATA_RESTORE				                clCorLogMsg[14]

/**
 *  " Loaded the default information model to COR" 
 */
#define CL_LOG_MESSAGE_0_INFORMATION_MODEL 			                clCorLogMsg[15]

/**
 *  " Default information model could not be loaded to COR. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_INFORMATION_MODEL			                clCorLogMsg[16]

/**
 * " Data restoration from persistent database done successfully. " 
 */
#define CL_LOG_MESSAGE_0_DATA_RESTORE 				                clCorLogMsg[17]

/**
 * " %s lib initialization Failed.  rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_2_LIB_INIT 				                    clCorLogMsg[18]

/**
 *  " COR could not get data from any source. No Information model present. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_INFORMATION_MODEL_ABSENT		            clCorLogMsg[19]

/**
 *  " Failed to get EO Object.  rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_EO_OBJECT_GET				                clCorLogMsg[20]

/**
 * " COR failed to get mapping information from XML.  rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_MAPPING_INFO_FROM_XML_GET		            clCorLogMsg[21]

/**
 *  "COR server fully up" 
 */ 
#define CL_LOG_MESSAGE_0_COR_COMPONENT_INIT 			            clCorLogMsg[22]

/**
 *  " COR EO interface is already initialized." 
 */
#define CL_LOG_MESSAGE_0_EO_ALREADY_INIT 			                clCorLogMsg[23]

/**
 *  " Native function table installation Failed. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_NATIVE_FUNCTION_TABLE_INIT 		        clCorLogMsg[24]

/**
 * " Failed to create MO class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_MOCLASS_CREATE				                clCorLogMsg[25]

/**
 *  " Failed to create MSO Class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_MSOCLASS_CREATE 			                clCorLogMsg[26]

/**
 * " Failed to delete MO class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_MOCLASS_DELETE 			                clCorLogMsg[27]

/**
 * " Failed to delete MSO Class. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_MSOCLASS_DELETE 			                clCorLogMsg[28]

/**
 * " Event Publish Channel Open Failed for COR. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_EVENT_PUBLISH_CHANNEL_OPEN		            clCorLogMsg[29]

/** 
 *" Event Message Allocation Failed for COR. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_EVENT_MESSAGE_ALLOCATION		            clCorLogMsg[30]

/**
 *  " Event Subscribe Channel Open Failed for CPM (component termination). rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CHANNEL_OPEN_COMP_TERMINATION		        clCorLogMsg[31]

/**
 *  " CPM event (component termination) subscription Failed. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CPM_EVENT_SUBSCRIBE_COMP_TERMINATION	    clCorLogMsg[32]

/**
 * " Event Subscribe Channel Open Failed for CPM (node arrival). rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CHANNEL_OPEN_NODE_ARRIVAL 		            clCorLogMsg[33]

/**
 *  " CPM event (node arrival) subscription Failed. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_CPM_EVENT_SUBSCRIBE_NODE_ARRIVAL	        clCorLogMsg[34]

/**
 *  " Event Publish Failed for COR. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_EVENT_PUBLISH 				                clCorLogMsg[35]

/**
 *  " Failed to get COR Object Attribute. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_OBJECT_ATTR_GET 			                clCorLogMsg[36]

/**
 *  " Failed to set COR Object Attribute. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_OBJECT_ATTR_SET 			                clCorLogMsg[37]

/**
 *  " Failed to create Object Tree. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_OBJECT_TREE_CREATE			                clCorLogMsg[38]

/**
 *  " Hello request to COR Master returns error. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_HELLO_REQUEST				                clCorLogMsg[39]

/**
 * " %sUnpack - Failed to unpack.  rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_2_UNPACK 				                    clCorLogMsg[40]

/**
 * " %s - Sync request failed. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_2_SYNC 					                    clCorLogMsg[41]

/**
 *  " Failed to add COR instance to corList. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_1_COR_LIST_ADD 				                clCorLogMsg[42]

/**
 * " Failed to pack the %s Tree. rc [0x%x]" 
 */
#define CL_LOG_MESSAGE_2_TREE_PACK 				                    clCorLogMsg[43]

/**
 * " Failed to create COR Object. rc [0x%x]. %s" 
 */
#define CL_LOG_MESSAGE_2_OBJECT_CREATE				                clCorLogMsg[44]

/**
 *  " Failed to delete COR Object. rc [0x%x]. %s" 
 */
#define CL_LOG_MESSAGE_2_OBJECT_DELETE				                clCorLogMsg[45]

/**
 *    " Failed to create Delta Db File Name. rc [0x%x] " 
 */
#define CL_LOG_MESSAGE_1_DB_FILENAME_CREATE			                clCorLogMsg[46]

/**
 *    " Successfully restored the object information from Db. " 
 */
#define CL_LOG_MESSAGE_1_DELTA_DB_RESTORE_SUCCESS	                clCorLogMsg[47]

/**
 *    " COR Initialization Succesfully Completed "     
 */
#define CL_LOG_MESSAGE_0_COR_INITIALIZATION_COMPLETED	            clCorLogMsg[48]

/**
 *     " COR Finalization  Succesfully Completed "  
 */
#define CL_LOG_MESSAGE_0_COR_FINALIZATION_COMPLETED	                clCorLogMsg[49]


/**
 * "IM creation failure. Configuration attribute [0x%x:0x%x] has incorrect qualifiers[%s]."
 */
#define CL_LOG_MESSAGE_3_CONFIG_ATTR_FLAG_INVALID                   clCorLogMsg[50] 

/**
 * "IM creation failure. Runtime attribute [0x%x:0x%x] has incorrect qualifiers [%s]."
 */
#define CL_LOG_MESSAGE_3_RUNTIME_ATTR_FLAG_INVALID                  clCorLogMsg[51] 

/**
 * "IM creation failure. Operational attribute [0x%x:0x%x] has incorrect qualifiers [%s]."
 */
#define CL_LOG_MESSAGE_3_OPERATIONAL_ATTR_FLAG_INVALID              clCorLogMsg[52] 

/**
 * " IM creation failure. Attribute [0x%x:0x%x] flag is invalid [%s]."
 */
#define CL_LOG_MESSAGE_3_ATTR_FLAG_INVALID                          clCorLogMsg[53] 

/**
 * "Failed while adding the OI [0x%x:0x%x] in the OI-List"
 */
#define CL_LOG_MESSAGE_2_OI_REGISTRATION_FAILED                      clCorLogMsg[54] 

/**
 * " MO[%s] donot have any OI configured."
 */
#define CL_LOG_MESSAGE_1_OI_NOT_REGISTERED                          clCorLogMsg[55] 

/**
 * " Preprocessing failed for the get operation for the MO [%s]. "
 */
#define CL_LOG_MESSAGE_1_GET_PREPROCESSING_FAILED                   clCorLogMsg[56] 

/**
 *  "Attribute Id -[0x%x] do not exit for classId [0x%x] "
 */
#define CL_LOG_MESSAGE_2_ATTR_DO_NOT_EXIST                          clCorLogMsg[57]


#ifdef __cplusplus
}
#endif
#endif  /*  _CL_COR_LOG_H_ */
