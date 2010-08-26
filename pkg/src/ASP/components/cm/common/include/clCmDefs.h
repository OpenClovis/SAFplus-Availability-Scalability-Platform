/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : cm
 * File        : clCmDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This private header contains the enumerations and definitions internal 
 * to the chassis manager module                                           
 **************************************************************************/


#ifndef _CM_DEFS_H 
#define _CM_DEFS_H 

#ifdef __cplusplus 
extern "C" { 
#endif 

#include <stdio.h>

#include <SaHpi.h>    
//#include <oh_utils.h>

#include <clCommon.h>
#include <clEoApi.h>
#include <clOsalApi.h>
#include <clCmApi.h>
#include <clEventApi.h>
#include <clCpmApi.h>
#include <ipi/clCmIpi.h>

#include <clVersionApi.h>

/**
 * Return code generator macro for CM related errors
 */
#define CL_CM_ERROR(error)              (CL_RC(CL_CID_CM, error))


#define CL_CM_INVALID_PHYSICAL_SLOT 0xff 

/* Default values for auto-extract and aut-insert timeouts */
#define CL_CM_MIN_AUTOINSERT_TIMEOUT   (5*1000000000LL) /* 5 seconds */
#define CL_CM_MIN_AUTOEXTRACT_TIMEOUT  (0*1000000000LL) /* 0 seconds */

/* Name of the component registered with Debug Server */
#define CLOVIS_CHASSIS_MANAGER  "ClChassisManager"
#define INVALID_ALARM_ID 0x0 
#define CL_CHASSIS_MANAGER_RMDCALL_TIMEOUT CL_RMD_DEFAULT_TIMEOUT    
#define CL_CHASSIS_MANAGER_RMDCALL_RETRIES CL_RMD_DEFAULT_RETRIES    


/* EO Client side function enumerations */ 
#define  CL_CHASSIS_MANAGER_FRU_STATE_GET CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x01)
#define  CL_CHASSIS_MANAGER_OPERATION_REQUEST CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x02)
#define  CL_CHASSIS_MANAGER_CPM_RESPONSE_HANDLE CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x03)


/* Chassis manager operation request structures */ 
typedef struct
ClCmOpReq
{
    ClVersionT      version;
    ClUint32T       chassisId;
    ClUint32T       physicalSlot;
    ClCmFruOperationT   request;

}ClCmOpReqT;    

typedef struct ClCmFruAddress
{
    ClVersionT      version;
    ClUint32T       chassisId;
    ClUint32T       physicalSlot;
}ClCmFruAddressT;    


/* Response structure from the CPMG */
typedef struct cpmResponse
{

    ClUint32T  seqNum;/* cookie to identify for which resource the query was made */
    ClUint8T  goAhead;/* Boolean response from CPMG whether to goahead with extraction or
                         insertion of the Card */
}cpmResponseT;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Event interface Information                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef struct ClCmEvtIface
{

    ClUint8T              eventIfaceInitDone;    /* CL_TRUE or CL_FALSE */ 
    ClEventHandleT        EvtHandle;        /* Handle to the Event */
    ClEventChannelHandleT EventChannelHandle;/* Handle to the CM Event Channel
                                                */
    ClEventInitHandleT    EventLibHandle;    /*Handle to the Event Library */

}ClCmEvtIfaceInfoT;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Platform related information                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define CL_CM_MAX_HPI_RESOURCES_PER_DOMAIN 64 
typedef struct ClCmPlatformInfo 
{

    ClUint32T no_of_sensors;
    ClUint32T no_of_inventories;
    ClUint32T no_of_frus;
    SaHpiSessionIdT hpiSessionId; /* session id of the first session */
    ClUint8T        stopEventProcessing; /* switch for turning HPI Event
                                            processing CLTRUE or CL_FALSE */
    ClUint8T         discoveryInProgress; /*CL_TRUE if discovery in progress */
    SaHpiResourceIdT fruResourceIds[CL_CM_MAX_HPI_RESOURCES_PER_DOMAIN];
}ClCmPlatformInfoT;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Chassis manager Context Information                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

typedef struct ClCmContext
{
    ClEoExecutionObjT* executionObjHandle; /* Handle to the ChassisManager 
                                              execution Object */
    ClCpmHandleT       cmCompHandle; /* Handle to the Chassis manager component */
    ClUint8T            cmInitDone;  /* CL_TRUE or CL_FALSE */     
    ClCmPlatformInfoT  platformInfo; /* platform related information */
    ClCmEvtIfaceInfoT  eventInfo;    /* event interface information  */
    ClOsalTaskIdT   eventListenerThread ;
}ClCmContextT;    


ClRcT VDECL(clCmServerApiFruStateGet)( ClEoDataT ,
                                       ClBufferHandleT,
                                       ClBufferHandleT);

ClRcT VDECL(clCmServerOperationRequest)( ClEoDataT ,
                                         ClBufferHandleT, 
                                         ClBufferHandleT);

ClRcT VDECL(clCmServerCpmResponseHandle)(ClEoDataT,
                                         ClBufferHandleT,
                                         ClBufferHandleT);


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*    information of waiting FRUs for CPMG's response                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/




ClRcT cmDebugRegister( ClEoExecutionObjT *peoObj );

ClRcT cmDebugDeregister( ClEoExecutionObjT *peoObj );
/*------------------- CM HPI Interface funcs -----------------*/

extern ClRcT clCmHpiInterfaceInitialize(void );

extern ClRcT clCmHpiInterfaceFinalize(void );









ClRcT clCmFullHotSwapHandler(SaHpiSessionIdT sessionid,
                             SaHpiEventT * pEvent,
                             SaHpiRptEntryT  * pRptEntry);

ClRcT clCmSimpleHotSwapHandler ( SaHpiSessionIdT sessionid , 
                                 SaHpiEventT * pEvent ,
                                 SaHpiRptEntryT  * pRptEntry);

ClRcT clCmHotSwapCpmUpdate( SaHpiRptEntryT * pRptEntry,
                            ClCmCpmMsgTypeT cm2CpmMsgType);
                            
void clCmHPICpmResponseHandle(ClCpmCmMsgT * pCpm2CmMsg);



ClRcT WalkThroughDomain  ( SaHpiDrtEntryT   *);

ClRcT WalkThroughResource (SaHpiRptEntryT* ,
                           SaHpiSessionIdT );



/************Events Published by CM *************/
typedef enum 
{
    CM_FRU_STATE_TRANSITION_EVENT =0,
    CM_ALARM_EVENT 
}CmEventIdT;



extern ClRcT clCmPublishEvent(CmEventIdT cmEventId ,ClUint32T payLoadLen,void * pEventData );


/*-------------------------Utility Stuff ------------------------*/


extern ClUint32T clCmPhysicalSlotFromEntityPath( SaHpiEntityPathT * pEpath );

extern void cmPrint( char * , ... );


extern ClRcT clCmEventInterfaceInitialize(void );

extern ClRcT clCmEventInterfaceFinalize( void );



extern ClRcT clCmHandleFruHotSwapEvent( SaHpiHsStateT prevState , 
                                        SaHpiHsStateT presState , 
                                        SaHpiRptEntryT * pRptEntry );

extern ClRcT clCmSimpleHotSwapHandler ( SaHpiSessionIdT sessionid , 
                                 SaHpiEventT * pEvent ,
                                 SaHpiRptEntryT  * pRptEntry);

extern SaHpiResourceIdT clCmResIdFromEpath(SaHpiEntityPathT * pEpath);

extern  ClRcT clCmGetResourceIdFromSlotId(ClUint32T chassisId,
                                         ClUint32T physicalSlot,
                                         SaHpiResourceIdT *pResourceId);


/*------------------------- Version Stuff ------------------------*/

/* CM Fru State operations structure with version field */ 
extern ClVersionDatabaseT gCmClientToServerVersionDb;

#define clCmClientToServerVersionValidate(version) \
		do\
		{ \
			if(clVersionVerify(&gCmClientToServerVersionDb, (&version) ) != CL_OK) \
			{ \
			    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to validate the client version .")); \
				return CL_ERR_VERSION_MISMATCH;  \
			} \
		}while(0) 


#define CL_CM_VERSION_SET(version)     \
		do { 							\
			version.releaseCode = (ClUint8T)'B' ; \
			version.majorVersion = 0x1; \
			version.minorVersion = 0x1; \
		}while(0)

/************************Debugging Macros ************************/
#ifdef CL_CM_CLIENT_DEBUG     
    #define CL_CM_CLIENT_DEBUG_PRINT(Z) \
    do{\
        printf("\n File=%s,Function=%s,Line=%d ",__FILE__,__FUNCTION__,__LINE__);\
        printf Z;\
        printf("\n");\
      }while(0)
#else
    #define CL_CM_CLIENT_DEBUG_PRINT(Z) 
#endif
    
#if 0
#ifdef CL_CM_SERVER_DEBUG     
    #define CL_CM_SERVER_DEBUG_PRINT(Z) \
    do{\
        printf("\n File=%s,Function=%s,Line=%d ",__FILE__,__FUNCTION__,__LINE__);\
        printf Z;\
        printf("\n");\
      }while(0)
#else
    #define CL_CM_SERVER_DEBUG_PRINT(Z) 
#endif
   
#ifdef CL_CM_HS_DEBUG     
    #define CL_CM_HS_DEBUG_PRINT(Z) \
    do{\
        printf("\n File=%s,Function=%s,Line=%d ",__FILE__,__FUNCTION__,__LINE__);\
        printf Z;\
        printf("\n");\
      }while(0)
#else
    #define CL_CM_HS_DEBUG_PRINT(Z) 
#endif
#endif


#ifdef __cplusplus 
}
#endif 
#endif  /* end #ifndef _CM_DEFS_H */ 
