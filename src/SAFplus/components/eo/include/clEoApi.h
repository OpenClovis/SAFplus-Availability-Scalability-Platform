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
 * ModuleName  : eo
 * File        : clEoApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This header contains the definitions of various Execution Object (EO)
 *      data types and APIs.
 *
 *
 *****************************************************************************/


/*****************************************************************************/
/******************************** EO APIs ************************************/
/*****************************************************************************/
/*                                                                           */
/* clEoCreate                                                                */
/* clEoWalk                                                                  */
/* clEoServiceValidate                                                       */
/* clEoStateSet                                                              */
/* clEoClientInstall                                                         */
/* clEoClientUninstall                                                       */
/* clEoClientDataSet                                                         */
/* clEoClientDataGet                                                         */
/* clEoServiceInstall                                                        */
/* clEoServiceUninstall                                                      */
/* clEoPrivateDataSet                                                        */
/* clEoPrivateDataGet                                                        */
/* clEoMyEoIocPortSet                                                        */
/* clEoMyEoIocPortGet                                                        */
/* clEoMyEoObjectSet                                                         */
/* clEoMyEoObjectGet                                                         */
/* clEoReceiveStart                                                          */
/*                                                                           */
/*****************************************************************************/

/**
 *  \file
 *  \brief Header file of EO related APIs
 *  \ingroup eo_apis
 */

/**
 *  \addtogroup eo_apis
 *  \{
 */

/*******************************************************************************
  Any execution manager in the form of a process, task or thread (let's call it
  an execution entity) is presumed to contain a set of data and an execution
  context.

  The execution entity running in the system without external communication is
  not very effective in the system, unless it is capable of interacting with
  other execution entities. Since the execution entities are distributed in
  nature, there needs to be a common mechanism that facilitates the
  communication of these execution entities. Basically, the execution entity
  consists of three responsibilities:
  - The attributes and methods required to manage the execution entity.
  - A mechanism to communicate with external execution entities.
  - Implementation specific to the responsibilities of the execution entity.

  Clovis provides the first two functionalities, which are encapsulated within
  the ExecutionObject. The ExecutionObject encapsulates all the common attributes
  and methods that are used to represent the execution entity in the system.
  Also, provided is the mechanism that facilitates the external communication.
  The external communication is provided by creating a background thread that
  receives the messages on behalf of the execution entity from external entities.
  Once the messages are received, decoded and the corresponding method within
  the execution entity is invoked. The user of the ExecutionObject is totally
  unaware of this processing, and can simple assume the implemented methods are
  invoked auto magically.
*******************************************************************************/


#ifndef _CL_EO_API_H_
#define _CL_EO_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/* CL_INCLUDES */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clQueueApi.h>
#include <clIocApi.h>
#include <clIocManagementApi.h>
#include <clRmdApi.h>
#include <clCntApi.h>
#include <clVersion.h>

#include <clEoConfigApi.h>
#include <clEoErrors.h>
#include <clRadixTree.h>

#define CL_MAX_BASIC_LIBS (10)

#define CL_MAX_CLIENT_LIBS (20)

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/** Name of the node.  Loaded from the same-named environment variable.  */
extern ClCharT ASP_NODENAME[CL_MAX_NAME_LENGTH];
/** Name of the component.  Loaded from the same-named environment variable.  */
extern ClCharT ASP_COMPNAME[CL_MAX_NAME_LENGTH];
/** Address of the node.  Loaded from the same-named environment variable.  */
extern ClUint32T ASP_NODEADDR;

/** Working dir where programs are run. Loaded from the same-named environment variable.  */
extern ClCharT ASP_RUNDIR[CL_MAX_NAME_LENGTH];
/** Dir where logs are stored. Loaded from the same-named environment variable.  */
extern ClCharT ASP_LOGDIR[CL_MAX_NAME_LENGTH];
/** Dir where ASP binaries are located. Loaded from the same-named environment variable.  */
extern ClCharT ASP_BINDIR[CL_MAX_NAME_LENGTH];
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].  Deprecated. */
extern ClCharT CL_APP_BINDIR[CL_MAX_NAME_LENGTH];
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0]. */
extern ClCharT ASP_APP_BINDIR[CL_MAX_NAME_LENGTH];

/** Dir where xml config are located. Loaded from the same-named environment variable.  */
extern ClCharT ASP_CONFIG[CL_MAX_NAME_LENGTH];
/** Dir where persistent db files are to be stored. Loaded from the same-named environment variable.  */
extern ClCharT ASP_DBDIR[CL_MAX_NAME_LENGTH];

/** Variable to check if the current node is a system controller node.  Loaded from the same-named environment variable.  */
extern ClBoolT SYSTEM_CONTROLLER; 
/** Variable to check if the current node is a SC capable node.  Loaded from the same-named environment variable.  */
extern ClBoolT ASP_SC_PROMOTE;

/**
 * If you change \c CL_EO_MAX_NO_FUNC, make sure to change
 * \c CL_EO_CLIENT_BIT_SHIFT CL_EO_FN_MASK.
 */
#define CL_EO_MAX_NO_FUNC                64

typedef enum {

/**
 * Adds to the front of the list.
 */
    CL_EO_ADD_TO_FRONT  = 0,

/**
 * Adds to back of the list. It is used with \e clEoServiceValidate.
 */
    CL_EO_ADD_TO_BACK   = 1
}ClEOServiceInstallOrderT;

/**
 * Logs base2 of \c CL_EO_MAX_NO_FUNC.
 */
#define CL_EO_CLIENT_BIT_SHIFT              6

/**
 * Hexadecimal of \c CL_EO_MAX_NO_FUNC -1.
 */
#define CL_EO_FN_MASK                       (0x3F)

#define CL_EO_CLIENT_ID_MASK                (ClUint32T)(~0U >> CL_EO_CLIENT_BIT_SHIFT)

/**
 * Defines a mechanism to generate unique RMD function number for the function.
 */
#define CL_EO_GET_FULL_FN_NUM(cl, fn)       (((cl) << CL_EO_CLIENT_BIT_SHIFT) | \
                                                ((fn) << 0))

/******************************************************************************
 *  Data Types
 *****************************************************************************/

/**
 * The type of the EO data.
 */
typedef ClOsalTaskDataT     ClEoDataT;

/**
 * EO argument type, this argument will always passed.
 */
typedef ClUint32T           ClEoArgT;

typedef enum {

/**
 * This is the service provided by Host EO.
 */
    CL_EO_NATIVE_COMPONENT_TABLE_ID = 0,

/**
 * This value is native EO specific.
 */
    CL_EO_DEFAULT_SERVICE_TABLE_ID,

/**
 * This value is EO Manager specific.
 */
    CL_EO_EO_MGR_CLIENT_TABLE_ID,

/**
 * This value is COR specific.
 */
    CL_EO_COR_CLIENT_TABLE_ID,

/**
 * This value is Event Manager specific.
 */
    CL_EO_EVT_CLIENT_TABLE_ID,

/**
 * This value is CPM specific.
 */
    CL_CPM_MGR_CLIENT_TABLE_ID,

/**
 * This value is Alarm Manager specific.
 */
    CL_ALARM_CLIENT_TABLE_ID,

/*
 * This value is Debug specific.
 */
    CL_DEBUG_CLIENT_TABLE_ID,

/*
 * This value is Transaction Agent-specific.
 */
    CL_TXN_CLIENT_TABLE_ID,

/**
 * This value is GMS specific.
 */
    CL_GMS_CLIENT_TABLE_ID,

/**
 * This value AMS Mgmt Server-specific
 */
    CL_AMS_MGMT_SERVER_TABLE_ID,

/**
 * This value AMS Mgmt Client-specific
 */
    CL_AMS_MGMT_CLIENT_TABLE_ID,

/**
 * This value is specific to Log Client
 */
    CL_LOG_CLIENT_TABLE_ID,
/**
 * This value is specific to Ckpt Client
 */
    CL_EO_CKPT_CLIENT_TABLE_ID,

/**
 * This value is specific to RMD Client
 */
    CL_EO_RMD_CLIENT_TABLE_ID,

/**
 * This value is specific to AMS metric trigger client.
 */ 
    CL_AMS_ENTITY_TRIGGER_TABLE_ID,

/**
 * This value is specific to CPM mgmt. clients.
 */
    CL_CPM_MGMT_CLIENT_TABLE_ID,
/**
 * This value is specific to MSG clients.
 */
    CL_MSG_CLIENT_TABLE_ID,

/**
 * This value is specific to MSG clients & servers.
 */
    CL_MSG_CLIENT_SERVER_TABLE_ID,
/*
 * Extended AMS mgmt server tables.
 */
    CL_AMS_MGMT_SERVER_TABLE2_ID,

/**
 * CLIENT IDs till this value are reserved by Clovis.
 */
    CL_EO_CLOVIS_RESERVED_CLIENTID_END

} ClEoClientIdT;

/**
 * You can specify a new \e clientId greater than \c CL_EO_USER_CLIENT_ID_START.
 */
#define CL_EO_USER_CLIENT_ID_START      CL_EO_CLOVIS_RESERVED_CLIENTID_END

/**
 * EO server Id enum.
 * Server ID is used as a key to save server specific data, that is, server
 * cookie into EO specific data area. Upto 256 cookies can be saved for each
 * server. The cookie key range from \c XXXX_COOKIE_ID to \c XXXX_COOKIE_ID+255.
 */

#define CL_EO_SERVER_COOKIE_BIT_SIZE        8

#define CL_EO_SERVER_COOKIE_BASE(cId)       ((cId) << (CL_EO_SERVER_COOKIE_BIT_SIZE))

/**
 * The Event Channel on which the Water Mark Notification is published.
 * Any interested component must open this global channel with the flags
 * defined here and appropriate filters to receive the events.
 */

#define CL_EO_EVENT_CHANNEL_NAME "CL_EO_EVENT_CHANNEL_NAME"
#define CL_EO_EVENT_SUBSCRIBE_FLAG (CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL)
#define CL_EO_EVENT_DEFAULT_FILTER NULL

/*
 * A typical call to subscribe would be as below:
 *   rc = clEventSubscribe(eoChannelHandle, CL_EO_EVENT_DEFAULT_FILTER, 0, NULL);
 *
 * For a more specific filter during subscribe one can specify the pattern as below:
 *   patterns[0].patternSize = sizeof(CL_EO_NAME);
 *   patterns[0].pPattern = (ClUint8T *)CL_EO_NAME;
 *   
 *   patterns[1].patternSize = sizeof(libraryId);
 *   patterns[1].pPattern = (ClUint8T *)&libraryId;
 *   
 *   patterns[2].patternSize = sizeof(waterMarkId);
 *   patterns[2].pPattern = (ClUint8T *)&waterMarkId;
 *   
 *   patterns[3].patternSize = sizeof(waterMarkType);
 *   patterns[3].pPattern = (ClUint8T *)&waterMarkType;
 *   
 *   patterns[4].patternSize = sizeof(waterMarkValue);
 *   patterns[4].pPattern = (ClUint8T *)(&waterMarkValue);
 */

typedef enum {
    CL_EO_NATIVE_COMPONENT_COOKIE_BASE
                    = CL_EO_SERVER_COOKIE_BASE(CL_EO_NATIVE_COMPONENT_TABLE_ID),
    CL_EO_EO_MGR_SERVER_COOKIE_ID
                    = CL_EO_SERVER_COOKIE_BASE(CL_EO_EO_MGR_CLIENT_TABLE_ID),
    CL_EO_COR_SERVER_COOKIE_ID
                    = CL_EO_SERVER_COOKIE_BASE(CL_EO_COR_CLIENT_TABLE_ID),
    CL_EO_EVT_EVENT_DELIVERY_COOKIE_ID
                    = CL_EO_SERVER_COOKIE_BASE(CL_EO_EVT_CLIENT_TABLE_ID),
    CL_EO_DEBUG_OBJECT_COOKIE_ID
                    = CL_EO_SERVER_COOKIE_BASE(CL_DEBUG_CLIENT_TABLE_ID),
    CL_EO_RMD_CLIENT_COOKIE_ID
                    = CL_EO_SERVER_COOKIE_BASE(CL_EO_RMD_CLIENT_TABLE_ID)
} ClEoServerIdT;


/**
 * \brief RMD with PAYLOAD with REPLY function pointer.
 * 
 *
 * \param data Given while invoking \e clEoClientInstall.
 *
 * \param inMsgHandle Received message over RMD.
 *
 * \param outMsgHandle Reply message if any.
 *
 * \par Description:
 * This is the generic function prototype definition of all the RMD functions
 * which get installed on the EO client object.  In other words, this callback
 * implements the server-side of the RMD call...
 *
 */

typedef ClRcT (* ClEoPayloadWithReplyCallbackT) (
       CL_IN   ClEoDataT data,
       CL_IN   ClBufferHandleT  inMsgHandle,
       CL_OUT  ClBufferHandleT  outMsgHandle);

typedef struct ClEoPayloadWithReplyCallbackServer
{
    ClRcT (*fun)(ClEoDataT, ClBufferHandleT, ClBufferHandleT);
    ClUint32T funId;
    ClUint32T version;
} ClEoPayloadWithReplyCallbackServerT;

typedef struct ClEoPayloadWithReplyCallbackClient
{
    ClUint32T funId;
    ClUint32T version;
} ClEoPayloadWithReplyCallbackClientT;
    
typedef struct ClEoPayloadWithReplyCallbackTableClient
{
    ClUint32T clientID;
    ClEoPayloadWithReplyCallbackClientT *funTable;
    ClUint32T funTableSize;
} ClEoPayloadWithReplyCallbackTableClientT;

typedef struct ClEoPayloadWithReplyCallbackTableServer
{
    ClUint32T clientID;
    ClEoPayloadWithReplyCallbackServerT *funTable;
    ClUint32T funTableSize;
} ClEoPayloadWithReplyCallbackTableServerT;

/**
 *  This structure is EO Service Object.
 */
typedef struct ClEoServiceObj {

/**
 * This is the client service function pointer.
 */
    void                    (*func)();

/**
 * This is the pointer to the next service on the same serviceID.
 */
    struct ClEoServiceObj   *pNextServObj;
}ClEoServiceObjT;

/**
 * This structure contains the pointer to the callback functions to be
 * registered with the EO and the client specific data.
 */

typedef struct {

/**
 * This is the pointer to EO APIs.
 */
    ClEoServiceObjT     funcs[CL_EO_MAX_NO_FUNC];

/**
 * This is the client-specific data.
 */
    ClEoDataT           data;

}ClEoClientObjT;

#define __CLIENT_RADIX_TREE_INDEX(fun_id, version_code) ( (((fun_id & CL_EO_FN_MASK) << CL_VERSION_SHIFT) ) | ((version_code) & CL_VERSION_MASK) )

typedef struct {
    ClUint32T maxClients;
    ClRadixTreeHandleT funTable;
}ClEoClientTableT;

typedef struct {
    ClUint32T maxClients;
    ClRadixTreeHandleT funTable;
    ClEoDataT data;
}ClEoServerTableT;

/**
 * The Execution Object abstracts the properties of a running OS thread or process.
 */

typedef struct ClEoExecutionObj{

/**
 * Execution object name.
 */
    ClCharT                     name[CL_EO_MAX_NAME_LEN];

/**
 * The eoID must be unique on a blade.
 */
    ClEoIdT                     eoID;

/**
 * This indicates the priority of the EO threads where RMD is executed.
 */
    ClOsalThreadPriorityT       pri;

/**
 * This indicates the EO State.
 */
    ClEoStateT                  state;

/**
 * This is the receive loop thread State.
 */
    ClUint32T                   threadRunning;

/**
 * This is the pointer to EO client APIs.
 */
    ClEoClientObjT              *pClient;

/**
 * This is the pointer to EO client tables.
 */
    ClEoClientTableT            *pClientTable;

/**
 * This is the pointer to EO server tables.
 */
    ClEoServerTableT            *pServerTable;
    
/**
 * This indicates the number of RMD threads spawned.
 */
    ClUint32T                   noOfThreads;

/**
 * This is the handle of the container of EO specific data.
 */
    ClCntHandleT                pEOPrivDataHdl;

/**
 * This indicates the EO communication object.
 */
    ClIocCommPortHandleT        commObj;

/**
 * This is the RMD object associated with the EO.
 */
    ClRmdObjHandleT             rmdObj;

/**
 * This indicates whether \e EOInit() has been called or not.
 */
    ClUint32T                   eoInitDone;

/**
 * This is used to set State related flag and counter.
 */
    ClUint32T                   eoSetDoneCnt;

/**
 * This is the TaskID information of receive loop. It is used to delete the EO.
 */
    ClCntHandleT                eoTaskIdInfo;

    ClUint32T                   refCnt;

/**
 * This mutex is used to protect the Execution Object.
 */
    ClOsalMutexT                eoMutex;

    ClOsalCondT                 eoCond;

/**
 * This indicates the requested IOC Communication Port.
 */
    ClIocPortT                  eoPort;

/**
 * This indicates the whether application needs main thread or not.
 */
    ClUint32T                   appType;

/**
 * This is the maximum number of EO clients.
 */
    ClUint32T                   maxNoClients;

/**
 * This application function is called from Main during the initialization process.
 */
    ClEoAppCreateCallbackT      clEoCreateCallout;

/**
 * This application function is called when the EO gets terminated.
 */
    ClEoAppDeleteCallbackT      clEoDeleteCallout;

/**
 * This is application function is called when the EO is moved into suspended state.
 */
    ClEoAppStateChgCallbackT        clEoStateChgCallout;

/**
 * This is the application function that is called when EO healthcheck is
 * performed by CPM
 */
    ClEoAppHealthCheckCallbackT     clEoHealthCheckCallout;

} ClEoExecutionObjT;



/**
 * \brief Function Callback definition for the \e clEoWalk function.
 *
 * \param func Function that implements the RMD.
 *
 * \param eoArg Arguments that need to be passed.
 *
 * \param inMsgHdl Request packet received including the protocol header.
 *
 * \param outMsgHdl Data part of response of a protocol [PDU].
 *
 * \par Description: 
 * As clEoWalk iterates through all RMD functions, it calls
 * this callback for each one.
 */

typedef ClRcT (*ClEoCallFuncCallbackT) (
   CL_IN ClEoPayloadWithReplyCallbackT func,
   CL_IN   ClEoDataT                   eoArg,
   CL_IN   ClBufferHandleT      inMsgHdl,
   CL_OUT  ClBufferHandleT      outMsgHdl);

/**
 * \brief Callback to handle incoming messages marked with a particular protocol
 *
 * \param pThis Handle of the EO object.
 *
 * \param eoRecvMsg Handle of the received message.
 *
 * \param priority  IOC message priority.
 *
 * \param protoType Protocol type.  This is supplied so that 1 implementation can be used
 *  to handle related protocols.
 *
 * \param length Length in bytes of the message (\e eoRecvMsg).
 *
 * \param srcAddr IOC address of the sender.
 *
 * \par Description:
 * The EO contains a "standard" IOC server that listens on a well known port
 * for messages.  The EO demultiplexes these messages by a protocol ID byte 
 * contained in the message and calls the appropriate registered handler for
 * each message.  The handler function definitions must match this prototype.
 *
 * \sa clEoProtoInstall
 *
 */

typedef ClRcT (*ClEoProtoCallbackT) (

    CL_IN ClEoExecutionObjT*    pThis,
    CL_IN ClBufferHandleT       eoRecvMsg,
    CL_IN ClUint8T              priority,
    CL_IN ClUint8T              protoType,
    CL_IN ClUint32T             length,
    CL_IN ClIocPhysicalAddressT srcAddr);


/**
 * This structure contains a list of the protocols registered with the EO.
 */

typedef struct
{

  /**
   * ID of the protocol being registered.
   */
    ClUint8T            protoID;

  /**
   * Name of the protocol being registered.
   */
    ClCharT             name[CL_MAX_NAME_LENGTH];

  /**
   * Blocking receive function of the protocol.
   */
    ClEoProtoCallbackT  func;

  /**
   * Non-blocking receive function of the protocol.
   */
    ClEoProtoCallbackT  nonblockingHandler;

  /** 
   * Flags controlling how this protocol is handled
   */
    ClUint32T           flags;  
 
} ClEoProtoDefT;

#define EO_CLIENT_SYM(sym) sym##Client
#define CL_EO_CLIENT_SYM_MOD(sym, mod) EO_CLIENT_SYM(sym##mod)
#define CL_EO_SERVER_SYM_MOD(sym, mod) sym##mod

#if defined (__SERVER__)

#define CL_EO_CALLBACK_TABLE_DECL(sym)  ClEoPayloadWithReplyCallbackServerT VDECL(sym)
#define CL_EO_CALLBACK_TABLE_VER_DECL(sym, rel, major, minor) ClEoPayloadWithReplyCallbackServerT VDECL_VER(sym, rel, major, minor)
#define CL_EO_CALLBACK_TABLE_LIST_DECL(sym, module) ClEoPayloadWithReplyCallbackTableServerT sym##module
#define CL_EO_CALLBACK_TABLE_LIST_DEF(clnt, sym) { \
        clnt, VDECL(sym), (ClUint32T)sizeof(VDECL(sym))/sizeof(VDECL(sym)[0]) }
#define CL_EO_CALLBACK_TABLE_LIST_VER_DEF(clnt, sym, rel, major, minor) { \
        clnt, VDECL_VER(sym, rel, major, minor), \
            (ClUint32T)sizeof(VDECL_VER(sym, rel, major, minor))/sizeof(VDECL_VER(sym, rel, major, minor)[0]) }

#else

#define CL_EO_CALLBACK_TABLE_DECL(sym)  ClEoPayloadWithReplyCallbackClientT VDECL(EO_CLIENT_SYM(sym))
#define CL_EO_CALLBACK_TABLE_VER_DECL(sym, rel, major, minor) ClEoPayloadWithReplyCallbackClientT VDECL_VER(EO_CLIENT_SYM(sym), rel, major, minor)
#define CL_EO_CALLBACK_TABLE_LIST_DECL(sym, module) ClEoPayloadWithReplyCallbackTableClientT EO_CLIENT_SYM(sym##module)
#define CL_EO_CALLBACK_TABLE_LIST_DEF(clnt, sym) { clnt, VDECL(EO_CLIENT_SYM(sym) ), \
            (ClUint32T)sizeof(VDECL(EO_CLIENT_SYM(sym))) / sizeof(VDECL(EO_CLIENT_SYM(sym))[0]) }
#define CL_EO_CALLBACK_TABLE_LIST_VER_DEF(clnt, sym, rel, major, minor) { \
        clnt, VDECL_VER(EO_CLIENT_SYM(sym), rel, major, minor), \
            (ClUint32T) sizeof(VDECL_VER(EO_CLIENT_SYM(sym), rel, major, minor)) / sizeof(VDECL_VER(EO_CLIENT_SYM(sym), rel, major, minor)[0]) }
#endif

#define CL_EO_CALLBACK_TABLE_LIST_DEF_NULL     { 0, NULL, 0 }

extern ClEoPayloadWithReplyCallbackTableClientT EO_CLIENT_SYM(gAspFuncTable)[];

ClRcT clEoClientInstallTables(ClEoExecutionObjT *pThis,
                              ClEoPayloadWithReplyCallbackTableServerT *table);

ClRcT clEoClientInstallTablesWithCookie(ClEoExecutionObjT *pThis,
                                        ClEoPayloadWithReplyCallbackTableServerT *table,
                                        ClEoDataT data);

ClRcT clEoClientUninstallTables(ClEoExecutionObjT *pThis,
                                ClEoPayloadWithReplyCallbackTableServerT *table);

ClBoolT clEoClientTableFilter(ClIocPortT eoPort, ClUint32T clientID);

ClRcT clEoClientTableRegister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                              ClIocPortT clientPort);

ClRcT clEoClientTableDeregister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                                ClIocPortT clientPort);

/**
 * \brief Install a protocol handler
 * \par Description
 * Installing 2 different handlers on the same protocol will cause a debugging
 * pause when in debug mode, and overwrite the older handler.
 * Passing an invalid protocol id will cause a debugging pause, and then be
 * installed.
 *
 * \param def The protocol definiton -- this structure is copied so you may pass a temporary (stack) variable
 *
 */
  void clEoProtoInstall(ClEoProtoDefT* def);

/**
 * \brief Remove a protocol handler
 * \par Description
 * It is not necessary to remove handlers before quitting so this function is
 * actually unnecessary.
 *
 * \param id The protocol id (the first byte in the message is the protocol identifier).
 */
  void clEoProtoUninstall(ClUint8T id);

/**
 * \brief Switch from one handler to another
 * \par Description
 * This function atomically switches handlers so you can be sure that no 
 * packets are lost.
 *
 * \param def The protocol definiton -- this structure is copied so you may pass a temporary (stack) variable
 */
  void clEoProtoSwitch(ClEoProtoDefT* def);


/************************ EO APIs ***************************/

/**
 ************************************
 *  \brief Performs a walk.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param func Function number to be executed.
 *  \param pFuncCallout Function that will perform the actual execution.
 *  \param inMsgHdl Request message received including protocol header.
 *  \param outMsgHdl (out) Data part of response of a protocol (PDU).
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_ERR_FUNC_NOT_REGISTERED If function is not registered.
 *  \retval CL_EO_ERR_EO_SUSPENDED If EO is in suspended state.
 *
 *  \par Description:
 *  This API is used to perform a walk through the EO for a given RMD function number.
 *  It calls \e rmdInvoke for each of the callback functions registered with an EO for
 *  that RMD function number.
 *
 *  \sa clEoServiceValidate()
 *
 */

extern ClRcT clEoWalk(
        CL_IN   ClEoExecutionObjT           *pThis,
        CL_IN   ClUint32T                   func,
        CL_IN   ClEoCallFuncCallbackT       pFuncCallout,
        CL_IN   ClBufferHandleT      inMsgHdl,
        CL_OUT  ClBufferHandleT      outMsgHdl);

extern ClRcT clEoWalkWithVersion(
        CL_IN   ClEoExecutionObjT           *pThis,
        CL_IN   ClUint32T                   func,
        CL_IN   ClVersionT                  *version,
        CL_IN   ClEoCallFuncCallbackT       pFuncCallout,
        CL_IN   ClBufferHandleT      inMsgHdl,
        CL_OUT  ClBufferHandleT      outMsgHdl);

/**
 ************************************
 *  \brief Validates the function registration.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param func Function to be invoked.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_EO_ERR_FUNC_NOT_REGISTERED If function is not registered.
 *  \retval CL_EO_ERR_EO_SUSPENDED If EO is in a suspended state.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to validate whether the function for which the request is
 *  made is registered or not. This API can be used to check whether the service
 *  provided by a particular EO is available or not before invoking \e clEoWalk().
 *
 *  \sa clEoWalk(), clEoServiceIndexGet()
 *
 */

extern ClRcT clEoServiceValidate(
        CL_IN   ClEoExecutionObjT   *pThis,
        CL_IN   ClUint32T           func);


/**
 ************************************
 *  \brief Installs the function table for a client.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param clientId Id of the Client.
 *  \param pFuncs Pointer to the function table.
 *  \param data Client specific data.
 *  \param nFuncs Number of functions passed that are being installed.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_NO_MEMORY On memory allocation failure.
 *  \retval CL_EO_CL_INVALID_CLIENTID On passing invalid clientId.
 *  \retval CL_EO_CL_INVALID_SERVICEID On passing invalid serviceId.
 *
 *  \par Description:
 *  This API is called by the client to install its function table
 *  with the EO. By calling this API the client is exporting all the
 *  APIs which it provides to the users, which the users can invoke
 *  through RMD calls.
 *
 *  \sa clEoClientUninstall()
 *
 */

extern ClRcT clEoClientInstall(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClUint32T             clientId,
        CL_IN ClEoPayloadWithReplyCallbackT    *pFuncs,
        CL_IN ClEoDataT             data,
        CL_IN ClUint32T             nFuncs);

extern ClRcT clEoClientInstallTable(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClUint32T             clientId,
        CL_IN ClEoDataT             data,
        CL_IN ClEoPayloadWithReplyCallbackServerT    *pFuncs,
        CL_IN ClUint32T             nFuncs);

/**
 ************************************
 *  \brief Uninstalls the function table for client.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param clientId Id of the client.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_ERR_INVALID_CLIENTID On passing invalid clientId.
 *
 *  \par Description:
 *  This API is called by the client to uninstall its function table
 *  with the EO. After calling this API the functions which were previously
 *  exported by this client using \e clEoClientInstall() can no longer be
 *  invoked as the RMD calls.
 *
 *  \sa clEoClientInstall()
 *
 */

extern ClRcT clEoClientUninstall(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClUint32T             clientId);

extern ClRcT clEoClientUninstallTable(ClEoExecutionObjT *pThis,
                                      ClUint32T clientID,
                                      ClEoPayloadWithReplyCallbackServerT *pfunTable,
                                      ClUint32T nentries);

/**
 ************************************
 *  \brief Stores the client specific data.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param clientId Id of the client.
 *  \param data Client specific data.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_ERR_INVALID_CLIENTID On passing invalid clientId.
 *
 *  \par Description:
 *  This API is used to store the client-specific data.
 *
 *  \sa clEoClientDataGet()
 *
 */

extern ClRcT clEoClientDataSet(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClUint32T             clientId,
        CL_IN ClEoDataT             data);


/**
 ************************************
 *  \brief Returns the client specific data.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param clientId Id of the client.
 *  \param pData (out) Client specific data.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_ERR_INVALID_CLIENTID On passing invalid clientId.
 *
 *  \par Description:
 *  This API is used to retrieve the client specific data.
 *
 *  \sa clEoClientDataSet()
 *
 */

extern ClRcT clEoClientDataGet(
        CL_IN   ClEoExecutionObjT   *pThis,
        CL_IN   ClUint32T           clientId,
        CL_OUT  ClEoDataT           *pData);



/**
 ************************************
 *  \brief Installs a particular client function.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param pFunction Function pointer to be installed.
 *  \param iFuncNum Function number.
 *  \param order Order as whether to add to the front or the back of the table.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_EO_CL_INVALID_SERVICEID On passing invalid serviceId.
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid parameters.
 *
 *  \par Description:
 *  This API is used to install a particular client function, identified by
 *  iFuncNum in the EO function table. By calling this API, the application is
 *  registering a service which it wishes to provide to other components.
 *  It can install the new service either to the front or back of the table by
 *  specifying the \e order.
 *
 *  \sa clEoServiceUninstall()
 *
 */


extern ClRcT clEoServiceInstall(
        CL_IN ClEoExecutionObjT             *pThis,
        CL_IN ClEoPayloadWithReplyCallbackT pFunction,
        CL_IN ClUint32T                     iFuncNum,
        CL_IN ClEOServiceInstallOrderT      order);

/**
 ************************************
 *  \brief Uninstalls a particular client function.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param pFunction Function pointer to be uninstalled.
 *  \param iFuncNum Function number.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_EO_FUNC_NOT_REGISTERED On unregistering a function which is not registered.
 *  \retval CL_EO_CL_INVALID_SERVICEID On passing invalid servide ID.
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to uninstall a particular client function from the EO
 *  function table. After calling this API, the service \e pFunction is no longer
 *  available for invoking as an RMD call.
 *
 *  \sa clEoServiceInstall()
 *
 */

extern ClRcT clEoServiceUninstall(
        CL_IN ClEoExecutionObjT                 *pThis,
        CL_IN ClEoPayloadWithReplyCallbackT     pFunction,
        CL_IN ClUint32T                         iFuncNum);


/**
 ************************************
 *  \brief Stores data in EO specific data area.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param type User specified key.
 *  \param pData EO specific data.
 *
 *  \par Return values:
 *  \e CL_ERR_NULL_POINTER: On passing a NULL pointer.\n
 *  Also returns the result of \e clCntNodeAdd.
 *
 *  \par Description:
 *  This API is used to store data in EO specific data area.
 *  For a unique key, there can be only one node.
 *
 *  \sa clEoPrivateDataGet()
 *
 */

extern ClRcT clEoPrivateDataSet(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClUint32T             type,
        CL_IN void                  *pData);



/**
 ************************************
 *  \brief Returns data from EO specific data area.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *  \param type User specified key.
 *  \param data (out) EO specific data.
 *
 *  \par Return values:
 *  \e CL_ERR_NULL_POINTER: On passing a NULL pointer.\n
 *  Also returns the result of \e clCntNodeUserDataGet.
 *
 *  \par Description:
 *  This API is used to retrieve data stored in EO specific data area.
 *
 *  \sa clEoMyEoIocPortSet()
 *
 */


extern ClRcT clEoPrivateDataGet(CL_IN ClEoExecutionObjT* pThis,
        CL_IN   ClUint32T   type,
        CL_OUT  void **data);



/**
 ************************************
 *  \brief Sets the EO thread \e iocPort.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param iocPort Carries the value to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to set the eoId.
 *
 *  \sa clEoMyEoIocPortGet()
 *
 */
 
extern ClRcT clEoMyEoIocPortSet(
        CL_IN ClIocPortT iocPort);



/**
 ************************************
 *  \brief Returns EO IocPort from task specific area.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pIocPort (out) Carries the value to be retrieved.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_STATE If state is invalid.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to retrieve the EO IocPort stored in the task specific area.
 *
 *  \sa clEoMyEoIocPortSet()
 *
 */


extern ClRcT clEoMyEoIocPortGet(
        CL_OUT ClIocPortT *pIocPort);



/**
 ************************************
 *  \brief Stores EO Object in task specific area.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pEoObj Contains \e ClEoExecutionObjT* to be stored.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to store the EO Object in task specific area.
 *
 *  \sa clEoMyEoObjectGet()
 *
 */


extern ClRcT clEoMyEoObjectSet(
        CL_IN ClEoExecutionObjT* eoObj);



/**
 ************************************
 *  \brief Returns EO Object from task specific area.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pEOObj (out) Carries the value to be retrieved.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_STATE If state is invalid.
 *
 *  \par Description:
 *  This API is used to retrieve the EO Object stored in the task specific area.
 *
 *  \sa clEoMyEoObjectSet()
 *
 */


extern ClRcT clEoMyEoObjectGet(
        CL_OUT ClEoExecutionObjT** pEOObj);


/**
 ************************************
 *  \brief Starts receiving messages for a thread.
 *
 *  \par Header File:
 *  clEoApi.h
 *
 *  \par Library Files:
 *  ClEo
 *
 *  \param pThis Handle of the EO.
 *
 *  \par Return values:
 *  None
 *
 *  \par Description:
 *  This API is used to start receiving the message for this thread.
 *  After calling this API the EO starts dequeuing the IOC messages
 *  whenever they are received and handle them to RMD for processing.
 *
 */


extern ClRcT clEoReceiveStart(
        CL_IN ClEoExecutionObjT     *pThis);




/***********************************************************************/
/*                      external symbols we expect here                */
/***********************************************************************/

/*
 * This is the configuration for an EO. Since we can only do one EO
 * per process in RC1, this can be here. But when support for multiple
 * EOs per process shows up, this has to be fixed in some way.
 */

extern ClEoConfigT clEoConfig;

/*
 * The list of libraries to initialize and finalize. The order of elements in
 * the array are as follows:
 *
 * Basic and mandatory libs in clEoBasicLibs[]. These have to be set to
 * CL_TRUE:
 * Osal
 * Timer
 * Buffer
 * Ioc
 * Rmd
 * Eo
 *
 * Basic and optional libs in clEoBasicLibs[]. These can be CL_TRUE or
 * CL_FALSE:
 * Om
 * Hal
 * Dbal
 *
 * These are the really optional client libs in clEoClientLibs[]. These
 * can be either CL_TRUE or CL_FALSE:
 * Cor
 * Cm
 * Name
 * Log
 * Trace
 * Diag
 * Txn
 * Hpi
 * Cli
 * Alarm
 * Gms
 */

extern ClRcT clEoConfigure(ClEoConfigT *eoConfig, 
                           ClUint8T *basicLibs, ClUint32T numBasicLibs,
                           ClUint8T *clientLibs, ClUint32T numClientLibs);

extern ClUint8T clEoBasicLibs[CL_MAX_BASIC_LIBS];

extern ClUint8T clEoClientLibs[CL_MAX_CLIENT_LIBS];

#ifdef __cplusplus
}

#endif


#endif /* _CL_EO_API_H_ */

/** \} */
