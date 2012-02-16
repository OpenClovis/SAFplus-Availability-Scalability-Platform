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
 * File        : clCorErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  This file contains all the external error Ids for COR.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of all External Error Ids for COR.
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */

#ifndef _CL_COR_ERRORS_H_
#define _CL_COR_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCommonErrors.h>

/* DEFINES */
#define CL_COR_SET_RC(ERR_ID)                   (CL_RC(CL_CID_COR, ERR_ID))

/**
 * There is no memory in the system.
 */ 
#define	CL_COR_ERR_NO_MEM		                CL_ERR_NO_MEMORY

/** 
 * The invalid parameter supplied to the function.
 */ 
#define	CL_COR_ERR_INVALID_PARAM	            CL_ERR_INVALID_PARAMETER

/**
 * The null pointer has been encountered while processing.
 */ 
#define	CL_COR_ERR_NULL_PTR	 	                CL_ERR_NULL_POINTER

/**
 * The entry being searched does not exist in COR.
 */ 
#define	CL_COR_ERR_NOT_EXIST	 	            CL_ERR_NOT_EXIST

/**
 * An invalid handle is supplied to the function.
 */ 
#define	CL_COR_ERR_INVALID_HANDLE	            CL_ERR_INVALID_HANDLE

/**
 * The user buffer supplied to the functoin is invalid.
 */ 
#define	CL_COR_ERR_INVALID_BUFFER	            CL_ERR_INVALID_BUFFER

/**
 * The buffer overrun has occured while processing the user buffer.
 */ 
#define	CL_COR_ERR_BUFFER_OVERRUN 	            CL_ERR_BUFFER_OVERRUN
   
/**
 * The entry being added is already existing in the list.
 */ 
#define	CL_COR_ERR_DUPLICATE		            CL_ERR_DUPLICATE

/**
 * There is no resource remaining in the system.
 */ 
#define	CL_COR_ERR_NO_RESOURCE		            CL_ERR_NO_RESOURCE

/**
 * The COR server has not responded within the specified timeout.
 */ 
#define	CL_COR_ERR_TIMEOUT		                CL_ERR_TIMEOUT

/**
 * The resource is unavilable for time being so COR server has send a try 
 * again in the response. 
 */ 
#define CL_COR_ERR_TRY_AGAIN                    CL_ERR_TRY_AGAIN

/**
 * The class Identifier passed is invalid.
 */
#define CL_COR_ERR_INVALID_CLASS		        0x100	

/**
 * The processing has reached an invalid state.
 */
#define CL_COR_ERR_INVALID_STATE		        0x101	

/**
 * The invalid MSP ID is supplied.
 */
#define CL_COR_ERR_INVALID_MSP_ID		        0x102	

/**
 * The respective append functions cannot add any more nodes to the MOID 
 * or MO class path. 
 */
#define CL_COR_ERR_MAX_DEPTH		            0x103		

/**
 * The depth specified in the MOID or Object Handle is invalid.
 */
#define CL_COR_ERR_INVALID_DEPTH		        0x104	

/**
 * The class being created is already present in the COR.
 */
#define CL_COR_ERR_CLASS_PRESENT		        0x105	

/**
 * The class being searched is not present in the COR.
 */
#define CL_COR_ERR_CLASS_NOT_PRESENT	        0x106	

/**
 * The class information cannot be modified as it has instances present
 * the object tree.
 */
#define CL_COR_ERR_CLASS_INSTANCES_PRESENT	    0x107	

/**
 * The error shows that the value supplied to the attribute is invalid. 
 */
#define CL_COR_ERR_CLASS_ATTR_INVALID_VAL	    0x108	

/**
 * The class already has an attribute with the attribute Identifier being added.
 */
#define CL_COR_ERR_CLASS_ATTR_PRESENT	        0x109		

/**
 * The object is locked so no more operation can be done till it is unlocked.
 */
#define CL_COR_ERR_OBJECT_LOCKED	   	        0x10a			

/**
 * The object on which operation is requested is invalid.
 */
#define CL_COR_ERR_OBJECT_INVALID	   	        0x10b			

/**
 * The attribute being searched is not present in the class.
 */
#define CL_COR_ERR_CLASS_ATTR_NOT_PRESENT	    0x10c	

/**
 * The end marker for the class being searched has encountered.
 */
#define CL_COR_ERR_CLASS_ATTR_TILL_REACHED	    0x10d

/**
 * The value of attribute is more than the range of value of its data type.
 */
#define CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE	    0x10e	

/**
 * The index specified for the attribute is invalid. This can be an index 
 * for an array attribute which is out of range of the array length or
 * for a simple attribute it is not specified as CL_COR_INVALID_ATTR_IDX.
 */
#define CL_COR_ERR_CLASS_ATTR_INVALID_INDEX	    0x10f		

/**
 * The size of the attribute is not matching its data type.
 */
#define CL_COR_ERR_CLASS_ATTR_INVALID_RELATION	0x110	

/**
 * The set operation is done on a containment attribute.
 */
#define CL_COR_ERR_OBJ_ATTR_INVALID_SET         0x111		

/**
 * The MO class path specified is invalid.
 */
#define CL_COR_ERR_CLASS_INVALID_PATH           0x112		

/**
 * The object being enabled is already active.
 */
#define CL_COR_ERR_OBJECT_ACTIVE                0x113			

/**
 * The error shows that OI is already present in the route list.
 */
#define CL_COR_ERR_ROUTE_PRESENT                0x114			

/**
 * The disable request for an invalid COR instance has arrived.
 */
#define CL_COR_ERR_UNKNOWN_COR_INSTANCE         0x115	

/**
 * The buffer supplied is not enough for the processing.
 */
#define CL_COR_ERR_INSUFFICIENT_BUFFER          0x116		

/**
 * The classes does not match either due to class identifier or 
 * attribute identifier present in the classes.
 */
#define CL_COR_ERR_CLASS_MISMATCH               0x117			

/**
 * The class attribute data type specified is invalid.
 */
#define CL_COR_ERR_CLASS_ATTR_INVALID_TYPE      0x118	

/**
 * The specified OI does not exist in the route list.
 */
#define CL_COR_ERR_ROUTE_NOT_PRESENT            0x119		

/**
 * An unsupported state has reached while processing the request.
 */
#define CL_COR_ERR_NOT_SUPPORTED		        0x11a		

/**
 * The Object Handle (OH) mask provided is invalid.
 */
#define CL_COR_ERR_INVALID_OH_MASK		        0x11b		

/**
 * The COR EO is already been initialized.
 */
#define CL_COR_ERR_ALREADY_INIT  	            0x11c			

/**
 * The object is created for a class containing initialized attributes, 
 * without specifying any value for them.
 */
#define CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED   0x11d			

/**
 * The attribute being set does not exist in the object.
 */
#define CL_COR_ERR_OBJ_ATTR_NOT_PRESENT         0x11e		

/**
 * This error will occur when base class is deleted. 
 */
#define CL_COR_ERR_CLASS_IS_BASE                0x11f				

/**
 * The managed object does not exist in the COR.
 */
#define CL_COR_ERR_OBJ_NOT_PRESENT              0x120			

/**
 * The size specified for the attribute is invalid. This can happen in the
 * the case of set or get operation on the Managed object attribute.
 */
#define CL_COR_ERR_INVALID_SIZE                 0x121			

/**
 * The version specified by the client is not supported at server.
 */
#define CL_COR_ERR_VERSION_UNSUPPORTED          0x122			



/* MO_TREE-specific error IDs, follows base COR base error IDs and starts at 0x130 */

/**
 * The error occured while adding the node to the MO class tree.
 */
#define CL_COR_MO_TREE_ERR_FAILED_TO_ADD_NODE       0x130	

/**
 * The failure occured while deleting the MO-class tree node.
 */
#define CL_COR_MO_TREE_ERR_FAILED_TO_DEL_NODE       0x131	

/**
 * The error specifies that a failure occured while getting the user buffer
 * from the MO class tree.
 */
#define CL_COR_MO_TREE_ERR_FAILED_USR_BUF           0x132		

/**
 * The MO class tree do not have a node corresponding 
 * to the class identifier specified.
 */
#define CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT         0x133		

/**
 * The MO class tree node not found for a given MO class path.
 */
#define CL_COR_MO_TREE_ERR_NODE_NOT_FOUND           0x134		

/**
 * The class node already exist in the MO class tree.
 */
#define CL_COR_MO_TREE_ERR_NODE_FOUND               0x135			

/**
 * The maximum instance for the MO class tree has reached.
 */ 
#define CL_COR_MO_TREE_ERR_MAX_INST                 0x136   

/**
 * The MO tree node cannot be deleted as the child node(s) exists.
 */
#define CL_COR_MO_TREE_ERR_CHILD_CLASS_EXIST        0x137		


/* INSTANCE-specific error IDs, follows MO_TREE error IDs ans start at 0x140 */

/**
 * The Mananged object node does not exist in the object tree.
 */
#define CL_COR_INST_ERR_NODE_NOT_FOUND              0x140		

/**
 * The Managed service object for a Managed object is already present in 
 * the object tree.
 */
#define CL_COR_INST_ERR_MSO_ALREADY_PRESENT         0x141	

/**
 * The Managed object is already present in the object tree.
 */
#define CL_COR_INST_ERR_MO_ALREADY_PRESENT          0x142	

/**
 * The Managed service object for a MO doesn't exist in the object tree.
 */
#define CL_COR_INST_ERR_MSO_NOT_PRESENT             0x143		

/**
 * The invalid MOID is supplied to the function. This can be either due to an 
 * invalid class identifier or invalid instance identifier for a class.
 */
#define CL_COR_INST_ERR_INVALID_MOID                0x144		

/**
 * An error occured while upacking the object tree.
 */
#define CL_COR_INST_ERR_UNPACK_FAILED               0x145		

/**
 * The object instance node is already present in the object tree.
 */
#define CL_COR_INST_ERR_NODE_ALREADY_PRESENT        0x146	

/**
 * A failure has occured while packing the instance tree.
 */
#define CL_COR_INST_ERR_NODE_NOT_TO_PACK	 	    0x147		

/**
 * The object node cannot be chopped as there is a child node(s) existing.
 */
#define CL_COR_INST_ERR_CHILD_MO_EXIST		        0x148			

/**
 * The object node cannot be delete as there are MSO node(s) existing.
 */
#define CL_COR_INST_ERR_MSO_EXIST			        0x149			

/**
 * The request for the object instance creation has exceeded the maximum 
 * instance limit specified while modeling. 
 */
#define CL_COR_INST_ERR_MAX_INSTANCE		        0x14a			

/**
 * An error occurred while looking for the parent in the object tree.
 */
#define CL_COR_INST_ERR_PARENT_MO_NOT_EXIST		    0x14b



/* TXN-specific error IDs, follows INST error IDs and starts at 0x150 */

/**
 * The transaction is completed for this trasaction id.
 */
#define CL_COR_TXN_ERR_OUT_OF_TXN                   0x150

/**
 * The transaction id is invalid.
 */
#define CL_COR_TXN_ERR_INVALID_ID                   0x151

/**
 * This error will occur when both set and delete operation are done on
 * same Managed Object in one transaction.
 */ 
#define CL_COR_TXN_ERR_INVALID_STATE                0x152

/**
 * The transaction spans mutlitple object. This is an obselete error code.
 */
#define CL_COR_TXN_ERR_SPANS_MULTI_OBJ              0x153

/**
 * The Invalid operation type has been supplied. 
 * The operation type should be one of value present in the enum ClCorOpsT.
 */
#define CL_COR_TXN_ERR_INVALID_OP                   0x154		

/**
 * The failure occured to get the previous job when there is only one 
 * job in the transacation.
 */
#define CL_COR_TXN_ERR_FIRST_JOB                    0x155		

/**
 * The get-next is done after reaching the last job in the job-list.
 */
#define CL_COR_TXN_ERR_LAST_JOB                     0x156		

/**
 * There are no jobs in the transaction being started.
 */
#define CL_COR_TXN_ERR_ZERO_JOBS                    0x157		

/**
 * The job-Identifier specified is invalid.
 */
#define CL_COR_TXN_ERR_INVALID_JOB_ID               0x158		

/**
 * An error has occured while getting the transaction failed-jobs for a 
 * given cor-transaction session identifier.
 */
#define CL_COR_TXN_ERR_FAILED_JOB_GET               0x159		

/**
 * There are no transaction failed-jobs existing for the given 
 * cor-transaction session identifier. 
 */
#define CL_COR_TXN_ERR_FAILED_JOB_NOT_EXIST         0x15a		

/**
 * A set operation is performed on a runtime cached attribute by a non-primary OI.
 */
#define CL_COR_ERR_RUNTIME_CACHED_SET               0x15b

/**
 * A set operation is performed on a non-writable attribute.
 */
#define CL_COR_ERR_ATTR_NON_WRITABLE_SET            0x15c

/**
 * The transaction job walk is terminated. This can be used to break the transaction 
 * job walk at any desired point.
 */
#define CL_COR_TXN_ERR_JOB_WALK_TERMINATE           0x15d


/* Notify-specific error IDs, follows TXN error IDs and starts at 0x160 */

/**
 * The invalid operation type is specified. The operation type should be one 
 * of values specified in ClCorOpsT.
 */
#define CL_COR_NOTIFY_ERR_INVALID_OP                0x160			

/**
 * The service Id of the MOID is specified as wild card while doing event subscription.
 */
#define CL_COR_NOTIFY_ERR_CANNOT_RESOLVE_CLASS      0x161	


/* Managed Services error IDs, follows Notify error IDs and starts at 0x170 */

/**
 * The Object flag specified is invalid. 
 */
#define CL_COR_SVC_ERR_INVALID_FLAGS                0x170	

/**
 * The service Id for the MSO specified in the MOID is invalid. This should 
 * be one of the value defined in ClCorServiceIdT.
 */
#define CL_COR_SVC_ERR_INVALID_ID                   0x171		

/**
 * The COR-List is not initialized.  
 */
#define CL_COR_INTERNAL_ERR_INVALID_COR_LIST        0x172		

/**
 * The object flag type supplied is invalid.
 */
#define CL_COR_INTERNAL_ERR_INVALID_RM_FLAGS        0x173		


/* COR Utils error IDs, follows SVC error IDs and starts at 0x180 */

/**
 * The COR server could not find a specified entry  in the object tree 
 * or the class tree.
 */
#define CL_COR_UTILS_ERR_INVALID_KEY                0x180			

/**
 * The member does not exist in the list.
 */
#define CL_COR_UTILS_ERR_MEMBER_NOT_FOUND           0x181	

/**
 * The invalid node is referred by giving a MO-class  path.
 */
#define CL_COR_UTILS_ERR_INVALID_NODE_REF           0x182		

/**
 * The tags supplied while packing MO-class tree or object tree at the 
 * active COR are not found proper after unpacking at the standby COR.
 */
#define CL_COR_UTILS_ERR_INVALID_TAG                0x183			

/**
 * The end tag is encountered while upacking the object or MO-class tree.
 */
#define CL_COR_UTILS_ERR_FOUND_END_TAG              0x184		

/**
 * The failure occured in comparing two MO-Class paths.
 */
#define CL_COR_UTILS_ERR_MOCLASSPATH_MISMATCH       0x185	



/* COR Initialization related error IDs. It starts from 0x190 onwards*/

/** 
 * The config attribute is either marked as non-cached or non-persistent. 
 */
#define CL_COR_ERR_CONFIG_ATTR_FLAG                 0x190

/** 
 * The attribute flag of a runtime attribute is marked as writable. 
 */
#define CL_COR_ERR_RUNTIME_ATTR_WRITE               0x191

/** 
 * A runtime attribute is marked as persistent without being marked as cached.
 */
#define CL_COR_ERR_ATTR_PERS_WITHOUT_CACHE          0x192

/** 
 *  The attribute user flag passed is invalid.
 */
#define CL_COR_ERR_ATTR_FLAGS_INVALID               0x193

/** 
 * The attribute data type is invalid.
 */
#define CL_COR_ERR_OP_ATTR_TYPE_INVALID             0x194


/* COR Session related error IDs, start from 0x1a0 onwards*/

/** 
 * The attribute data not found at the OI or at the COR.
 */
#define CL_COR_ERR_GET_DATA_NOT_FOUND               0x1a0

/** 
 * A failure occured in the bundle initialization.
 */
#define CL_COR_ERR_BUNDLE_INIT_FAILURE              0x1a1

/** 
 * Failure while doing a bundle apply.
 */
#define CL_COR_ERR_BUNDLE_APPLY_FAILURE             0x1a2

/** 
 * The bundle is applied without adding any jobs to it.
 */
#define CL_COR_ERR_ZERO_JOBS_BUNDLE                 0x1a3    

/** 
 *  A read-only attribute is being set.
 */
#define CL_COR_ERR_ATTR_READ_ONLY                   0x1a4

/** 
 * The failure occured finalizing the bundle.
 */
#define CL_COR_ERR_BUNDLE_FINALIZE                  0x1a5

/** 
 *  The bundle timeout has occured in the case of a synchronous bundle.
 */
#define CL_COR_ERR_BUNDLE_TIMED_OUT                 0x1a6

/**
 * The bundle is applied or finalized to a bundle for which the response 
 * has not yet reached after applying. 
 */
#define CL_COR_ERR_BUNDLE_IN_EXECUTION              0x1a7

/**
 * The bundle type (ClCorBundleConfigT) is supplied wrongly to the bundle 
 * initialize. For this release it should be only CL_COR_BUNDLE_NON_TRANSACTIONAL.
 */
#define CL_COR_ERR_BUNDLE_INVALID_TYPE              0x1a8



/* COR OI registration related error IDS. It starts from 0x1c0 onwards. */

/* OI list related errors :
1. OI is not registered. 
2. A read OI is already registered for this MO. 
*/

/** 
 * The OI is not registered when doing get of the primary OI. It also 
 * indicate that a primary OI flag is being set for OI without adding 
 * it to the route list. 
 */
#define CL_COR_ERR_OI_NOT_REGISTERED                0x1c0

/** 
 * An OI is already registered as a primary OI for the Managed object.
 */
#define CL_COR_ERR_OI_ALREADY_REGISTERED            0x1c1

/* COR COMM error IDs, follows Session error IDs and starts at 0x1d0 */


/**
 * The invalid Operation type supplied to the comm.
 */
#define CL_COR_COMM_ERR_INVALID_OP                  0x1d0      



/* COR CLI error IDs, follows comm error Ids and starts from 0x1e0 */

/**
 * The invalid usage of the CLI command has occured.
 */
#define CL_COR_CLI_ERR_INVALID_USAGE                0x1e0



/* Error string declarations */

/**
 * The string indicating that memory allocation has failed.
 */ 
#define CL_COR_ERR_STR_MEM_ALLOC_FAIL           "Memory allocation failed!"

/**
 * The string indicating that invalid depth has been specified.
 */ 
#define CL_COR_ERR_STR_MAX_DEPTH                "Max depth already used!"

/**
 * The string indicating that invalid depth has been provided.
 */ 

#define CL_COR_ERR_STR_INVALID_DEPTH            "Invalid depth"
/**
 * The string indicating that invalid parameter has been provided.
 */ 

#define CL_COR_ERR_INVALID_PARAM_STR            "Invalid parameter passed"
/**
 * The string indicating that pointer passed is NULL.
 */ 

#define CL_COR_ERR_NULL_PTR_STR                 "NULL value passed"
/**
 * The string indicating that memory has been exhausted.
 */ 

#define CL_COR_ERR_NO_MEM_STR                   "Out of memory!"
/**
 * The string indicating that insufficient buffer has been supplied.
 */ 

#define CL_COR_ERR_INSUFFICIENT_BUFFER_STR      "Insufficient buffer passed!"

/**
 * The string indicating that buffer passed is invalid.
 */ 
#define	CL_COR_ERR_INVALID_BUFFER_STR           "Invalid buffer"

/**
 * The string indicating that node referred using the MO class path is invalid.
 */  
#define	CL_COR_UTILS_ERR_INVALID_NODE_REF_STR  "Invalid Node"

/**
 * The string to indicate that node is already present in the object or MO class tree.
 */ 
#define	CL_COR_MO_TREE_ERR_NODE_FOUND_STR       "Node already found"
 
/**
 * The string indicating that node is not present in the object or the MO class tree.
 */ 
#define CL_COR_MO_TREE_ERR_NODE_NOT_FOUND_STR   "Unknown node referenced"

/**
 * The string indicating that the node referenced is not present. 
 */ 
#define CL_COR_INST_ERR_NODE_NOT_FOUND_STR      "Unknown Instance node referred"

/**
 * The string indicating that the MO is already present in the object tree.
 */ 
#define CL_COR_INST_ERR_MO_ALREADY_PRESENT_STR  "MO Object already present"

/**
 * Macro to contruct the error string macros.
 */ 
#define CL_COR_ERR_STR(ERR)               ERR##_STR


#ifdef __cplusplus
}

#endif

#endif	/*	 _CL_COR_ERRORS_H_ */


/** \} */
