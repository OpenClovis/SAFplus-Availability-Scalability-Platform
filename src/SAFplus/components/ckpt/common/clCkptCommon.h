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
 * ModuleName  : ckpt                                                          
 * File        : clCkptCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains common definitions used by checkpoint server and client
*
*
*****************************************************************************/
#ifndef _CL_CKPT_COMMON_H_
#define _CL_CKPT_COMMON_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clEoApi.h>
#include <clRmdApi.h>
#include <clIdlApi.h>
#include <clCkptApi.h>


typedef ClHandleT CkptMastHdlInfoT;



/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/


#define CKPT_RMD_DFLT_TIMEOUT   CL_RMD_DEFAULT_TIMEOUT       /* RMD Timeout */
#define CKPT_RMD_DFLT_RETRIES   CL_RMD_DEFAULT_RETRIES 
                                           /* RMD retries */

#define CL_CKPT_UNINIT_ADDR     0xFFFFFE   /* Uninitialized address */ 
#define CL_CKPT_UNINIT_VALUE    0xFFFFFE   /* Uninitialized value */

#define CL_CKPT_INVALID_HDL     -1         /* Invalid handle */

#define CL_CKPT_ACTIVE_REP_CHG_EVENT 0      /* Event for active replica 
                                               addr change */
#define CL_CKPT_IMMEDIATE_CONS_EVENT 1      /* Event for immediate 
                                               consumption */

#define CL_CKPT_UPDCLIENT_EVENT_TYPE 0x200  /* Event type of events delivered
                                               to ckpt clients.*/
                                               
#define  CL_CKPT_GEN_SEC_LENGTH    30
/* Channel for publishing the change in active replica address */
#define CL_CKPT_CLNTUPD_EVENT_CHANNEL "CKPT_UPDATE_CLIENT_CHANNEL"


/*
 * Macro for validating ckpt name.
 */
 
#define CL_CKPT_NAME_VALIDATE(pName)\
{\
    do\
    {\
        if(pName == NULL)\
        {\
            CKPT_DEBUG_E(("Null pointer\n"));\
                return CL_CKPT_ERR_NULL_POINTER;\
        }\
    }while(0);\
}



/*
 * Macro for printing error level messages.
 */
 
#define CKPT_DEBUG_E(y)\
do\
{\
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, y);\
}while(0)\



/*
 * Macro for printing trace level messages.
 */
 
#define CKPT_DEBUG_T(y)\
do\
{\
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, y);\
}while(0)\



/*
 * Macro for checking synchronous nature of the checkpoint.
 */
 
#define CL_CKPT_IS_SYNCHRONOUS(creationFlags)                               \
    (CL_CKPT_WR_ALL_REPLICAS == ((creationFlags) & CL_CKPT_WR_ALL_REPLICAS ))



/*
 * Macro for checking collocated nature of the checkpoint.
 */
 
#define CL_CKPT_IS_COLLOCATED(creationFlags)                                \
    ((!CL_CKPT_IS_SYNCHRONOUS(creationFlags)) &&                            \
        (CL_CKPT_CHECKPOINT_COLLOCATED                                      \
            == ((creationFlags) & CL_CKPT_CHECKPOINT_COLLOCATED)))



/*
 * Macros for checkpointing related communication (both client-server
 * and server-server).
 */
 
#define CKPT_SVC_CKPT_OPEN     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)
#define CKPT_SVC_SECN_CREAT    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)
#define CKPT_SVC_SECN_WRITE    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3)
#define CKPT_SVC_CKPT_WRITE    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4)
#define CKPT_SVC_CKPT_READ     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 5)
#define CKPT_SVC_CKPT_CLOSE    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 6)
#define CKPT_SVC_SECN_DELETE   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 7)
#define CKPT_SVC_CKPT_DELETE   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 8)
#define CKPT_SVC_CKPT_STATUS_GET CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 9)    

#define CKPT_PEER_SYNC          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 35)
#define CKPT_SVC_INIT_ITERATION CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 36)
#define CKPT_SVR_ASYNC_OPEN     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 37)
#define CKPT_MAS_DB_PACK        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 38)
#define CKPT_MAS_ACT_ADDR_GET      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 39)
#define CKPT_MAS_INFO_GET       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 40)
#define CKPT_DEPUTY_UPDATE      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 41)
#define CKPT_NACK_SEND          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 42)
#define CKPT_UPDATE_LEADER_ADDR    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 43)
#define CKPT_REM_SEC_ADD        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 44)
#define CKPT_REM_SEC_DEL        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 45)
#define CKPT_REM_SEC_OVERWRITE  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 46)
#define CKPT_GET_CKPT_INFO      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 47)
#define CKPT_REM_CKPT_DEL      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 48)




/**====================================**/
/**     S T R U C T U R E S            **/
/**====================================**/


/*
 * Information passed in the change in active replica address event.
 */

typedef struct {
    ClUint32T         eventType; /* Type of event */
    ClNameT           name;      /* Ckpt Name whose active replica address 
                                    is changing. */
    ClIocNodeAddressT actAddr ;  /* Address of the new active replica */
} ClCkptClientUpdInfoT;



/*
 * Handle related info.
 */
 
typedef struct hdlDb
{
    ClUint8T             hdlType;         /* Type of handle (service
                                             or iteration or checkpoint) */
    ClIocNodeAddressT    activeAddr;      /* Active address of the ckpt */
    ClIocNodeAddressT    prevMasterAddr;  /* Prev master addr - 
                                             for checkpoint handle type */
    ClCkptSvcHdlT        ckptSvcHdl;      /* Service hdl - 
                                             for service handle type */
    ClCkptHdlT           ckptLocalHdl;    /* Local handle -
                                             for checkpoint handle type */
    ClCkptHdlT           ckptMasterHdl;   /* Master handle -
                                             for checkpoint handle type */
    ClCkptHdlT           ckptActiveHdl;   /* Active handle -
                                             for checkpoint handle type */
    ClUint32T            cksum;           /* Index into ckptHdl list */
    ClNameT              ckptName;        /* name of the checkpoint */
    ClUint32T            openFlag;        /* Type of open */
    ClCkptCreationFlagsT creationFlag;    /* Creation flags */
    ClCkptNotificationCallbackT notificationCallback; 
                                          /* Callback for immediate 
                                             consumption */
    ClPtrT                      pCookie; /* Cookie for callback */
}CkptHdlDbT;



/**====================================**/
/**          E X T E R N S             **/
/**====================================**/

/* Routine to pack a vector */
ClRcT ckptIdlHandleUpdate(ClIocNodeAddressT nodeId,ClIdlHandleT handle,ClUint32T numRetries);

/* Routine for getting well known ckpt logical address */
void ckptOwnLogicalAddressGet(ClIocLogicalAddressT * logicalAddress);

/* Routine for getting ckpt master address */	
ClRcT clCkptMasterAddressGet( ClIocNodeAddressT *pIocAddress);

/* Handle delete callback function */
void    ckptHdlDeleteCallback(ClCntKeyHandleT userKey, 
                              ClCntDataHandleT userData);

ClRcT clCkptLeakyBucketInitialize(void);

#ifdef __cplusplus
}
#endif

#endif							/* _CL_CKPT_COMMON_H_ */ 
