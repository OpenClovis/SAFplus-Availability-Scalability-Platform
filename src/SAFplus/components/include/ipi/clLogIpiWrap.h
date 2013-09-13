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
 * \file 
 * \brief Header file of Log Service related APIs
 * \ingroup log_apis
 */

/**
 * \addtogroup log_apis
 * \{
 */
 
#ifndef CL_LOG_IPI_WRAP_H
#define CL_LOG_IPI_WRAP_H
#include <clIocApi.h>

/**
 * Log severity levels
 */
#define CL_LOG_EMERGENCY      0x1
#define CL_LOG_ALERT          0x2
#define CL_LOG_CRITICAL       0x3
#define CL_LOG_ERROR          0x4
#define CL_LOG_WARNING        0x5
#define CL_LOG_NOTICE         0x6
#define CL_LOG_INFO           0x7
#define CL_LOG_INFORMATIONAL  CL_LOG_INFO 
#define CL_LOG_DEBUG          0x8
#define CL_LOG_DEBUG1         CL_LOG_DEBUG      // DEBUG1 is same as DEBUG
#define CL_LOG_DEBUG2         0x9
#define CL_LOG_DEBUG3         0xa
#define CL_LOG_DEBUG4         0xb
#define CL_LOG_DEBUG5         0xc
#define CL_LOG_TRACE          CL_LOG_DEBUG5
#define CL_LOG_DEBUG6         0xd
#define CL_LOG_DEBUG7         0xe
#define CL_LOG_DEBUG8         0xf
#define CL_LOG_DEBUG9         0x10
#define CL_LOG_MAX            0x15
#define CL_LOG_END            CL_LOG_MAX


/**
 * This contains the list of all common log messages used by ASP
 * components.
 */

extern ClCharT		*clLogCommonMsg[];

/**
 * "[%s] Service Started".
 */
#define	CL_LOG_MESSAGE_1_SERVICE_STARTED 							clLogCommonMsg[0]

/**
 * "[%s] Service Stopped".
 */
#define	CL_LOG_MESSAGE_1_SERVICE_STOPPED  		 	    		clLogCommonMsg[1]

/**
 * "ASP_CONFIG environment variable is not set".
 */
#define	CL_LOG_MESSAGE_0_ENV_NOT_SET							 	clLogCommonMsg[2]

/**
 * "Invalid parameter passed in function [%s]".
 */
#define	CL_LOG_MESSAGE_1_INVALID_PARAMETER  				 		clLogCommonMsg[3]

/**
 * "Index [%d] out of range in [%s]".
 */
#define CL_LOG_MESSAGE_2_OUT_OF_RANGE_INDEX		 				clLogCommonMsg[4]

/**
 * "Invalid handle".
 */
#define CL_LOG_MESSAGE_0_INVALID_HANDLE	 				 		clLogCommonMsg[5]

/**
 * "NULL argument passed".
 */
#define CL_LOG_MESSAGE_0_NULL_ARGUMENT							clLogCommonMsg[6]

/**
 * "Function [%s] is obsolete".
 */
#define CL_LOG_MESSAGE_1_FUNC_OBSOLETE							clLogCommonMsg[7]

/**
 * "XML file [%s] not valid".
 */
#define CL_LOG_MESSAGE_1_XML_ERROR			 		    		clLogCommonMsg[8]

/**
 * "Requested resource does not exist".
 */
#define CL_LOG_MESSAGE_0_RESOURCE_NON_EXISTENT			 		clLogCommonMsg[9]

/**
 * "The buffer passed is invalid".
 */
#define CL_LOG_MESSAGE_0_INVALID_BUFFER    		 				clLogCommonMsg[10]

/**
 * "Duplicate entry".
 */
#define CL_LOG_MESSAGE_0_DUPLICATE_ENTRY   		 				clLogCommonMsg[11]

/**
 * "Paramenter passed is out of range".
 */
#define CL_LOG_MESSAGE_0_PARAM_OUT_OF_RANGE 		 				clLogCommonMsg[12]

/**
 * "No resources available".
 */
#define CL_LOG_MESSAGE_0_RESOURCE_UNAVAILABLE  	 				clLogCommonMsg[13]

/**
 * "Component already initialized".
 */
#define CL_LOG_MESSAGE_0_COMPONENT_INITIALIZED 					clLogCommonMsg[14]

/**
 * "Buffer over run".
 */
#define CL_LOG_MESSAGE_0_BUFFER_OVERRUN     		 				clLogCommonMsg[15]

/**
 * "Component not initialized".
 */
#define CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED	 				clLogCommonMsg[16]

/**
 * "Version mismatch".
 */
#define CL_LOG_MESSAGE_0_VERSION_MISMATCH    		 				clLogCommonMsg[17]

/**
 * "An entry is already existing".
 */
#define CL_LOG_MESSAGE_0_ENTRY_ALREADY_EXISTING 	 				clLogCommonMsg[18]

/**
 * "Invalid State".
 */
#define CL_LOG_MESSAGE_0_INVALID_STATE			 				clLogCommonMsg[19]

/**
 * "Resource is in use".
 */
#define CL_LOG_MESSAGE_0_RESOURCE_BUSY 			 				clLogCommonMsg[20]

/**
 * "Component [%s] is busy, try again".
 */
#define CL_LOG_MESSAGE_1_COMPONENT_BUSY			 				clLogCommonMsg[21]

/**
 * "No callback available for request".
 */
#define CL_LOG_MESSAGE_0_CALLBACK_UNAVAILABLE  	 				clLogCommonMsg[22]

/**
 * "Failed to allocated memory".
 */
#define CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED 				clLogCommonMsg[23]

/**
 * "Failed to communicate with [%s], rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_2_COMMUNICATION_FAILED 	 				clLogCommonMsg[24]

/**
 * "Host [%s] unreachable, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_2_HOST_UNREACHABLE	 		 				clLogCommonMsg[25]

/**
 * "Service [%s] could not be started, rc=[0x%x] ".
 */
#define CL_LOG_MESSAGE_2_SERVICE_START_FAILED		 				clLogCommonMsg[26]

/**
 * "Library [%s] initialization failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED		 				clLogCommonMsg[27]

/**
 * "Reading checkpoint data failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CKPT_READ_FAILED   						clLogCommonMsg[28]

/**
 * "Unable to write Checkpoint dataset, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CKPT_WRITE_FAILED   		 				clLogCommonMsg[29]

/**
 * "Container creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_CREATE_FAILED   						clLogCommonMsg[30]

/**
 * "Container data get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED       				clLogCommonMsg[31]

/**
 * "Container data addition failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED   	 				clLogCommonMsg[32]

/**
 * "Container data delete failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED   				clLogCommonMsg[33]

/**
 * "Unable to get eo object, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED   					clLogCommonMsg[34]

/**
 * "Opening event channel [%s] failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_2_EVT_CHANNEL_OPEN_FAILED   				clLogCommonMsg[35]

/**
 * "Event subscription failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_EVT_SUBSCRIBE_FAILED      				clLogCommonMsg[36]

/**
 * "ASP_BINDIR environment variable not set".
 */
#define CL_LOG_MESSAGE_0_ASP_BINDIR_ENV_NOT_SET		 				clLogCommonMsg[37]

/**
 * "Container node find failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_NODE_FIND_FAILED		 				clLogCommonMsg[38]

/**
 * "Container node get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_NODE_GET_FAILED		 				clLogCommonMsg[39]

/**
 * "Container node add failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_NODE_ADD_FAILED		 				clLogCommonMsg[40]

/**
 * "Container walk failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_WALK_FAILED			 		 		clLogCommonMsg[41]

/**
 * "Container delete failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_DELETE_FAILED				 		clLogCommonMsg[42]

/**
 * "Cor event subscribe failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_EVT_SUBSCRIBE_FAILED	 				clLogCommonMsg[43]

/**
 * "Cor object handle get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_OBJ_HANDLE_GET_FAILED 				clLogCommonMsg[44]

/**
 * "Cor attribute type get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_ATTR_TYPE_GET_FAILED	 				clLogCommonMsg[45]

/**
 * "Cor MOID to class get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_MOID_TO_CLASS_GET_FAILED 			clLogCommonMsg[46]

/**
 * "Cor notify event to moid get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_NOTIFY_EVENT_TO_MOID_GET_FAILED		clLogCommonMsg[47]

/**
 * "Event cookie get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_EVENT_COOKIE_GET_FAILED		 			clLogCommonMsg[48]

/**
 * "Cor event to containment attribute path get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_EVENT_TO_ATTR_PATH_GET_FAILED		clLogCommonMsg[49]

/**
 * "Cor object attribute get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_ATTR_GET_FAILED		 				clLogCommonMsg[50]

/**
 * "Container node delete failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CNT_NODE_DELETE_FAILED 					clLogCommonMsg[51]

/**
 * "Cor object handle to moid get failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_OBJ_HANDLE_TO_MOID_GET_FAILED		clLogCommonMsg[52]

/**
 * "Cor transaction commit failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_COR_TXN_COMMIT_FAILED 					clLogCommonMsg[53]

/**
 * "Function [%s] not implemented".
 */
#define CL_LOG_MESSAGE_1_FUNC_NOT_IMPLEMENTED 					clLogCommonMsg[54]

/**
 * "Timeout".
 */
#define CL_LOG_MESSAGE_0_TIMEOUT				 					clLogCommonMsg[55]

/**
 * "Handle creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED 					clLogCommonMsg[56]

/**
 * "Handle database creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_HANDLE_DB_CREATION_FAILED				clLogCommonMsg[57]

/**
 * "Handle database destroy failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED					clLogCommonMsg[58]

/**
 * "Failed to get handle for component [%s], rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_2_GET_HANDLE_FAILED	 					clLogCommonMsg[59]

/**
 * "Handle checkin failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED 					clLogCommonMsg[60]

/**
 * "Handle checkout failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED 					clLogCommonMsg[61]

/**
 * "Message creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_BUFFER_MSG_CREATION_FAILED				clLogCommonMsg[62]

/**
 * "Message read failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_BUFFER_MSG_READ_FAILED					clLogCommonMsg[63]

/**
 * "Message write failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_BUFFER_MSG_WRITE_FAILED					clLogCommonMsg[64]

/**
 * "Message deletion failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_BUFFER_MSG_DELETE_FAILED 				clLogCommonMsg[65]

/**
 * "Checkpoint creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CKPT_CREATION_FAILED						clLogCommonMsg[66]

/**
 * "Checkpoint dataset creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CKPT_DATASET_CREATION_FAILED				clLogCommonMsg[67]

/**
 * "Mutex creation failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_OSAL_MUTEX_CREATION_FAILED				clLogCommonMsg[68]

/**
 * "Mutex could not be locked, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_OSAL_MUTEX_LOCK_FAILED					clLogCommonMsg[69]

/**
 * "Mutex could not be unlocked, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_OSAL_MUTEX_UNLOCK_FAILED					clLogCommonMsg[70]

/**
 * "RMD call failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_RMD_CALL_FAILED							clLogCommonMsg[71]

/**
 * "Installation of function table for client failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_FUNC_TABLE_INSTALL_FAILED				clLogCommonMsg[72]

/**
 * "User callout function deletion failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_CALLOUT_FUNC_DELETE_FAILED				clLogCommonMsg[73]

/**
 * "Registration with debug failed, rc=[0x%x]".
 */
#define CL_LOG_MESSAGE_1_DBG_REGISTER_FAILED					clLogCommonMsg[74]
/**
 * "Registration with debug failed, rc=[0x%x]".
 */
#define CL_CKPT_LOG_4_CLNT_VERSION_NACK    						clLogCommonMsg[75]

/**
 * The default system stream handle.
 */
extern ClHandleT  CL_LOG_HANDLE_SYS; 

/**
 * The default application stream handle.
 */
extern ClHandleT CL_LOG_HANDLE_APP;
/**
 * The type of the handle for the stream.
 */
typedef  ClHandleT ClLogStreamHdlT;

#define CL_LOG_FILENAME_LENGTH  80   /* The maximum size of file name */
#define CL_LOG_FILEPATH_LENGTH  200  /* The maximum size of file location */
#define CL_LOG_FORMAT_LENGTH    50   /* The maximum format expression length */

/**
 *  Set this bit if the SG name is given.
 */
#define CL_LOG_SG		(1<<0)

/**
 *  Set this bit if the SU name is given.
 */
#define CL_LOG_SU		(1<<1)

/**
 *  Set this bit if the \e compname is given.
 */
#define CL_LOG_COMP		(1<<2)

/**
 * Macros used internally by the Log.
 */
#define CL_LOG_COMPNAME_LENGTH		50

/**
 * Message length.
 */
#define CL_LOG_MSG_LENGTH			178

typedef struct
{

/**
 * Severity of the message.
 */
	ClLogSeverityT   			logSeverity;

/**
 * Handle to the stream.
 */
	ClLogStreamHdlT				streamHdl;

/**
 * Id of the process.
 */
	ClUint32T				pid;

/**
 * Physical address of the component.
 */
	ClIocPhysicalAddressT 	iocAddress;

/**
 * Time when the message was generated.
 */
	ClTimerTimeOutT			time;

/**
 * Component name, this also includes the library name (if any).
 */
	ClCharT 				compName[CL_LOG_COMPNAME_LENGTH];

/**
 * The message.
 */
	ClCharT					msg[CL_LOG_MSG_LENGTH];

}ClLogHeaderT;

typedef struct
{

/**
 *  Maximum log size.
 */
    ClUint32T 	maxRecordSize;

/**
 *  The maximum size of file.
 */
    ClUint64T 	maxFileSize;

/**
 * This stores the original file name.
 */
    ClCharT		fileName[CL_LOG_FILENAME_LENGTH];

/**
 *  The location of file.
 */
    ClCharT 	fileLoc[CL_LOG_FILEPATH_LENGTH];
}ClLogFileConfigT;



/**
 *
 */
typedef struct
{

/**
 *  The format expression.
 */
    ClCharT             format[CL_LOG_FORMAT_LENGTH];

/**
 * The file configuration.
 */
    ClLogFileConfigT    fileConfig;
}ClLogFormatFileConfigT;



/**
 *
 */
typedef struct
{
/**
 *  This bit pattern specifies which fields are applicable.
 */
	ClUint32T	bitMap;

/**
 *  The Service Group name.
 */
	SaNameT	sgName;

/**
 *  The Service Unit name.
 */
	SaNameT	suName;

/**
 *  The Component name.
 */
	SaNameT	compName;
}ClLogFilterPatternT;


typedef ClUint16T ClLogSeverityFlagsT;

extern ClRcT clLogLibInitialize(void);

extern ClRcT clLogLibFinalize(void);


extern ClRcT clLogOpen(CL_OUT  ClLogStreamHdlT         *handle,
                       CL_IN   ClLogFormatFileConfigT  fileConfig);

extern ClRcT clLogClose(CL_IN 	ClLogStreamHdlT handle);

extern ClRcT clLogLevelSet(CL_IN 	ClLogSeverityT	severity);
extern ClRcT clLogLevelGet(CL_OUT 	ClLogSeverityT	*severity);


#endif

/** 
 * \} 
 */
