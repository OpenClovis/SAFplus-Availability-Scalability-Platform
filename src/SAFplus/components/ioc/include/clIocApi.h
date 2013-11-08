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
 * ModuleName  : ioc
 * File        : clIocApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Ioc data structures and APIs
 *
 *
 *****************************************************************************/

/*****************************************************************************/
/******************************** IOC APIs ***********************************/
/*****************************************************************************/
/*
 */
/*
 * clIocCommPortCreate
 */
/*
 * clIocCommPortDelete
 */
/*
 * clIocSend
 */
/*
 * clIocReceive
 */
/*
 * clIocTransparencyRegister
 */
/*
 * clIocTransparencyDeregister
 */
/*
 * clIocLocalAddressGet
 */
/*
 * clIocBind
 */
/*
 * clIocVersionCheck
 */
/*
 */
/*****************************************************************************/

/**
 *  \file
 *  \brief Header file of Ioc Data Structures and APIs
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */


#ifndef _CL_IOC_API_H_
# define _CL_IOC_API_H_

# ifdef __cplusplus
extern "C"
{
# endif


/*****************************************************************************/
/*********************** User configurable parameters ************************/
/*****************************************************************************/
# include <clCommon.h>
# include <clBufferApi.h>
# include <clIocServices.h>
# include <clIocProtocols.h>
# include <clIocErrors.h>

/*
 * Defines
 */


/**
 * The version of IOC.
 */
# define CL_IOC_HEADER_VERSION 1

# define CL_IOC_NOTIFICATION_VERSION 1


/**
 * Infinite timeout.
 */
# define CL_IOC_TIMEOUT_FOREVER ~0U


/**
 * Communication port creation related flags.
 * If the flag value is 0, it is unreliable messaging.
 */
# define CL_IOC_UNRELIABLE_MESSAGING 0

/**
 * Communication port creation related flags .
 * If the flag value is set(0th bit should be set), it is reliable messaging.
 */
# define CL_IOC_RELIABLE_MESSAGING (1<<0)




    /*
     * The node status and ARP related flags.
     */


/**
 * Node is up.
 */
# define CL_IOC_NODE_UP 1

/**
 * Node is down.
 */
# define CL_IOC_NODE_DOWN 0




/**
 * The link is up.
 */
# define CL_IOC_LINK_UP 2

/**
 * The link is down.
 */
# define CL_IOC_LINK_DOWN 3




/**
 * This address can be used to broadcast a message.
 */
# define CL_IOC_BROADCAST_ADDRESS  0xffffffff

/**
 * This address can be used in place of local IOC address.
 */
# define CL_IOC_RESERVED_ADDRESS  0

/**
 * The maximum size of the geographical address.
 */
# define CL_IOC_GEO_ADDR_MAX_LENGTH 128




/**
 * Physical address type.
 */
# define CL_IOC_PHYSICAL_ADDRESS_TYPE       0

/** 
 * Logical address type.
 */
# define CL_IOC_LOGICAL_ADDRESS_TYPE        1

/**
 * Multicast address type.
 */
# define CL_IOC_MULTICAST_ADDRESS_TYPE      2

/* 
 * Master address type
 */
# define CL_IOC_MASTER_ADDRESS_TYPE         3

/*
 * Local commport address type
 */
# define CL_IOC_INTRANODE_ADDRESS_TYPE      4


# define CL_IOC_USER_ADDRESS_TYPE          10 

/**
 * Broadcast address.
 */
# define CL_IOC_BROADCAST_ADDRESS_TYPE   0xff

    /*
     * Rest of the addresses are not valid
     */


/**
 * This defines all the priorities that can be used with IOC.
 */
typedef enum {
    /**
     * This is the default priority. This tells the IOC to use the lowest priority for sending the packet.
     */
    CL_IOC_DEFAULT_PRIORITY   = 0,

    /**
     * This is the higest priority with which a packet can be sent.
     */
    CL_IOC_HIGH_PRIORITY      = 1,

    /**
     * This is the lowest priority. The packets sent with this priority might get rejected if there is congestion on the transports.
     */
    CL_IOC_LOW_PRIORITY       = 2,

    /*
     * This is the priority for sending ordered proto messages, 
     * delivered in order
     */
    CL_IOC_ORDERED_PRIORITY = 3,

    /* 
     * This is the priority for sending notification protos in order
     */      
    CL_IOC_NOTIFICATION_PRIORITY = 4,

    /**
     * This is reserved by IOC for future use.
     */
    CL_IOC_RESERVED_PRIORITY  = 5,

    CL_IOC_RESERVED_PRIORITY_USER = 6,

    CL_IOC_RESERVED_PRIORITY_USER_END = 16,

    /**
     * This limits the use of the number of priorities that can be used with IOC.
     */
    CL_IOC_MAX_PRIORITIES     = CL_IOC_RESERVED_PRIORITY_USER_END + 1
} ClIocPriorityT;



/**
 * This defines all notification related actions that can be performed on a port.
 */
typedef enum {
    /**
     * Disables component/node arrival/departure notifications.
     */
    CL_IOC_NOTIFICATION_DISABLE = 0,

    /**
     * Enable scomponent/node arrival/departure notifications.
     */
    CL_IOC_NOTIFICATION_ENABLE,
} ClIocNotificationActionT;



#define CL_IOC_ADDRESS_TYPE_BITS (0x8)
#define CL_IOC_ADDRESS_TYPE_MASK ((1<<CL_IOC_ADDRESS_TYPE_BITS)-1)
#define CL_IOC_NODE_MASK (~0U >> CL_IOC_ADDRESS_TYPE_BITS)
#define CL_IOC_ADDRESS_TYPE_SHIFT_WORD (32-CL_IOC_ADDRESS_TYPE_BITS)
#define CL_IOC_ADDRESS_TYPE_SHIFT_DWORD (64-CL_IOC_ADDRESS_TYPE_BITS)
/**
 * Macro to determine Type of address.
 * Pass the address of IOC address as parameter and type is returned.
 */
# define CL_IOC_ADDRESS_TYPE_GET(param)\
    ((ClUint32T)((*((ClUint64T*)(param))) >> CL_IOC_ADDRESS_TYPE_SHIFT_DWORD))

#define CL_IOC_ADDRESS_FORM(addrType,addr,compId)                       \
    (ClUint64T)(                                                        \
    ( (((ClUint64T)(addrType)) & CL_IOC_ADDRESS_TYPE_MASK) << CL_IOC_ADDRESS_TYPE_SHIFT_DWORD ) | \
    ( ( ((ClUint64T)(addr)) & CL_IOC_NODE_MASK) << 32 )    |                      \
    ( ((ClUint64T)(compId)) & 0xffffffff )                              \
    )

#define CL_IOC_TIPC_ADDRESS_TYPE_FORM(t,v) (((t) << CL_IOC_ADDRESS_TYPE_SHIFT_WORD) | ( (v) & CL_IOC_NODE_MASK ) )

#define CL_IOC_TIPC_TYPE_FORM(v)                                \
    CL_IOC_TIPC_ADDRESS_TYPE_FORM(CL_IOC_USER_ADDRESS_TYPE,v)

/**
 * Macro is used to form logical IOC address.
 */
# define CL_IOC_LOGICAL_ADDRESS_FORM(compId)\
     CL_IOC_ADDRESS_FORM(CL_IOC_LOGICAL_ADDRESS_TYPE, 0 , compId)

/** 
 * Macro is used to form multicast IOC address. 
 */
# define CL_IOC_MULTICAST_ADDRESS_FORM(addr, compId)                         \
     CL_IOC_ADDRESS_FORM(CL_IOC_MULTICAST_ADDRESS_TYPE, addr, compId)


/**
 * HA state for Registration with Transparency Layer.
 * State is Active.
 */
# define CL_IOC_TL_ACTIVE 0

/**
 * HA state for Registration with Transparency Layer.
 * State is Standby.
 */
# define CL_IOC_TL_STDBY  1



    /*
     * Type definitions
     */

/**
 * The IOC node address.
 */
    typedef ClUint32T ClIocNodeAddressT;

/**
 * The IOC communication port.
 */
    typedef ClUint32T ClIocPortT;

/**
 * The Communication port handle.
 */
    typedef ClWordT ClIocCommPortHandleT;

/**
 * The Transport handle.
 */
    typedef ClHandleT ClIocToBindHandleT;


/**
 * Port Type. It can be have the following values:
 * \arg \c CL_IOC_UNRELIABLE_MESSAGING: for unreliable messaging
 * \arg \c CL_IOC_RELIABLE_MESSAGING: for reliable messaging
 */
    typedef ClUint32T ClIocCommPortFlagsT;

/**
 * IOC Logical address.
 */
    typedef ClUint64T ClIocLogicalAddressT;

/** 
 * IOC Multicast address. 
 */
    typedef ClUint64T ClIocMulticastAddressT;


/**
 * The IOC Physical address of an application's communication end point.
 */
    typedef struct ClIocPhysicalAddress
    {

/**
 * The IOC Node address.
 */
        ClIocNodeAddressT nodeAddress;

/**
 * The IOC communication end point identification on a node.
 */
        ClIocPortT portId;

    } ClIocPhysicalAddressT;

/**
 * IOC address.
 */
    typedef union ClIocAddress
    {

/**
 * Physical address.
 */
        ClIocPhysicalAddressT iocPhyAddress;

/**
 * Logical address.
 */
        ClIocLogicalAddressT iocLogicalAddress;
        
/** 
 * Multicast address.
 */
        ClIocMulticastAddressT iocMulticastAddress;


    } ClIocAddressT;











/**
 * The send message option.
 */
    typedef enum ClIocMessageOption
    {
/**
 * The message is persistent in nature and is not consumed by IOC
 * on send.
 */
        CL_IOC_PERSISTENT_MSG = 0,

/**
 * The message is non persistent in nature and is consumed by IOC
 * on send. This message cannot be reused.
 */
        CL_IOC_NON_PERSISTENT_MSG
    } ClIocMessageOptionT;





/**
 * Send related options.
 */
    typedef struct ClIocSendOption
    {

/**
 * Message priority.
 */
        ClUint8T priority;

/**
 * The send type, it can be session based or not.
 */
        ClUint8T sendType;
/**
 * The Handle for uniquely identifying Transport and Link.
 */
        ClWordT linkHandle;

/**
 * The message type, it can be persistent or non persistent message.
 */
        ClIocMessageOptionT msgOption;

/**
 * The timeout interval in miliseconds.
 */
        ClUint32T timeout;

    } ClIocSendOptionT;



/**
 * The IOC receive returns this structure along with the message.
 */
    typedef struct ClIocRecvParam
    {
/**
 * Priority of the message received.
 */
        ClUint8T priority;

/**
 * Protocol used.
 */
        ClUint8T protoType;

/**
 * Length of the message received.
 */
        ClUint32T length;

/**
 * Sender address.
 */
        ClIocAddressT srcAddr;

    } ClIocRecvParamT;





/**
 * IOC receive options.
 */

    typedef struct ClIocRecvOption
    {

/**
 * Timeout value for blocking call.
 */
        ClUint32T recvTimeout;
    } ClIocRecvOptionT;

    typedef enum ClIocQueueId
    {
        CL_IOC_SENDQ,
        CL_IOC_RECVQ,
        CL_IOC_QUEUE_MAX,
    } ClIocQueueIdT;

    typedef struct ClIocQueueInfo
    {
        ClUint32T queueSize;
        ClWaterMarkT queueWM;
    } ClIocQueueInfoT;

    typedef struct ClIocQueueStats
    {
        ClIocQueueInfoT queueInfo;
        ClUint32T queueUtilisation;
    } ClIocQueueStatsT;

    typedef struct ClIocLibConfig
    {
/**
 * Version of IOC.
 */
        ClUint8T version;
/**
 * The IOC address of the node.
 */
        ClIocNodeAddressT nodeAddress;
/**
 * The Geographical address.
 */
        ClCharT iocGeoGraphicalAddress[CL_IOC_GEO_ADDR_MAX_LENGTH + 1];
/**
 * Maximum Number of priority.
 */
        ClUint32T iocMaxNumOfPriorities;
/**
 * The reassembly timeout.
 */
        ClUint32T iocReassemblyTimeOut;
/**
 * Maximum number of transport.
 */
        ClUint32T iocMaxNumOfXports;
/**
 * IOC node level heartbeating time interval.
 */
        ClUint32T iocHeartbeatTimeInterval;
/**
 * Maximum number of entries for the Transparency Layer.
 */
        ClUint32T iocTLMaxEntries;

        /*SendQ config*/
        ClIocQueueInfoT iocSendQInfo;

        /*RecvQ config*/
        ClIocQueueInfoT iocRecvQInfo;

        /*The port to which the global sendq events should be sent*/
        ClIocPortT iocNodeRepresentative;

        /*The port to which the global sendq events should be sent*/
        ClBoolT isNodeRepresentative;

    } ClIocLibConfigT;


/*****************************************************************************/
/***************** Transparency Layer related data structures ****************/
/*****************************************************************************/

/**
 * Transparency layer context.
 */
    typedef enum ClIocTLContext
    {

/**
 * The context for Global scope entries.
 */
        CL_IOC_TL_GLOBAL_SCOPE,

/**
 * The context for local scope entries.
 */
        CL_IOC_TL_LOCAL_SCOPE
    } ClIocTLContextT;




/**
 * Data-type for holding the physical address and its state.
 */
    typedef struct
    {
/**
 * State of the component corresponding to the physicalAddr.
 */
        ClUint32T haState;

/**
 * Physical address of the component.
 */
        ClIocPhysicalAddressT physicalAddr;
    } ClIocTLMappingT;


/* This is kept only for backward compatibility */
#define CL_IOC_TL_NO_REPLICATION      0
#define repliSemantics                unused

/**
 * Transparency layer parameters
 */
    typedef struct ClIocTlInfo
    {

/**
 * Logical address of the service.
 */
        ClIocLogicalAddressT logicalAddr;

/**
 * Id of the component providing the service.
 */
        ClUint32T compId;

/**
 * Context, it can be either \c GLOBAL or \c LOCAL.
 */
        ClIocTLContextT contextType;

/**
 * Replication Semantics.
 */
        ClUint32T unused;

/**
 * Active or Standby mode.
 */
        ClUint32T haState;

/**
 * Physical address of the component.
 */
        ClIocPhysicalAddressT physicalAddr;

    } ClIocTLInfoT;


    typedef struct ClIocMcastUserInfo
    {
        ClIocMulticastAddressT mcastAddr;   /* multicast address */
        ClIocPhysicalAddressT physicalAddr; /* IOC physical address */
    } ClIocMcastUserInfoT;


/*****************************************************************************
 *  IOC APIs
 *****************************************************************************/


/**
 **************************************
 *  \brief Creates a communication port.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param portId Id of the communication port to be created. If portId is 0,
 *  then a communication port id is generated by IOC.
 *  \param portType This parameter refers to the type of communication that
 *  can be reliable or unreliable. This parameter can have the following two
 *  values:
 *  \arg \c CL_IOC_UNRELIABLE_MESSAGING, for unreliable messaging.
 *  \arg \c CL_IOC_RELIABLE_MESSAGING, for reliable messaging.
 *  \param pIocCommPortHdl Handle to the communication port used by
 *  applications to send and receive the messages.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized
 *  \retval CL_ERR_NULL_POINTER If the \e pIocCommPortHdl is passed as NULL.
 *  \retval CL_ERR_NOT_IMPLEMENTED If portType is not
 *  \c CL_IOC_UNRELIABLE_MESSAGING.
 *  \retval CL_ERR_INVALID_PARAMETER If the portId is more than
 *  \c CL_IOC_COMMPORT_END.
 *  \retval CL_ERR_NOT_EXIST If 0 is passed as portId and no ephemeral
 *  communication port is free.
 *  \retval CL_IOC_ERR_COMMPORT_REG_FAIL If communication port registration failed.
 *  \retval CL_ERR_NO_MEMORY If the memory allocation or any other resource
 *  allocation fails.
 *  \retval CL_ERR_UNSPECIFIED In case of other failures.
 *
 *  \par Description:
 *  This API is used to create a communication port. A communication port is
 *  compulsary for an ASP application which wants to communicate with another
 *  ASP application. The mode of communication can be reliable or unreliable,
 *  which should be specified through /e portType parameter. The communication
 *  port created will be in the blocking mode.
 *
 *  \sa clIocSend(), clIocReceive(), clIocCommPortDelete().
 *
 *
 */

    ClRcT clIocCommPortCreate(
    CL_IN ClIocPortT portId,
    CL_IN ClIocCommPortFlagsT portType,
    CL_OUT ClIocCommPortHandleT * pIocCommPortHdl
    );




/**
 **************************************
 *  \brief Deletes the communication port.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param iocCommPortHdl Handle to communication port to be deleted.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized
 *  \retval CL_ERR_INVALID_HANDLE If Invalid communication port handle is
 *  passed.
 *  \retval CL_IOC_ERR_COMMPORT_BLOCKED If The communication port is blocked.
 *
 *  \par Description:
 *  This API is used to delete the already existing communication port, which
 *  was created by calling \e clIocCommPortCreate() API. Before deleting the
 *  communication port, it should made sure that no thread is using the
 *  communication port and no one is blocked on receive.
 *
 *  \sa clIocCommPortCreate(), clIocSend(), clIocReceive().
 *
 */
    ClRcT clIocCommPortDelete(
    CL_IN ClIocCommPortHandleT iocCommPortHdl
    );


/**
 **************************************
 *  \brief Gets the socket descriptor used by communication of a CommPort.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param portHandle This is the communication port handle, which was
 *  returned on creating the commport through \e clIocCommPortCreate() API.
 *  \param pSd This is pointer to a variable of type ClInt32T in which the
 *  socket descriptor for the CommPort will be returned.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized
 *  \retval CL_ERR_INVALID_HANDLE If Invalid communication port handle is
 *  passed.
 *  \retval CL_ERR_NULL_POINTER If the address for holding the socket descriptor
 *  is NULL.
 *
 *  \par Description:
 *  This API is used to get the socket descriptor of a communication port, created
 *  by \e clIocCommPortCreate() API. This API is useful when select() or poll() on
 *  few IOC communication ports is to performed. On calling this API with
 *  communication port returns with socket descriptor and then this socket
 *  descriptor can be used in select() or poll(). Now the applications which
 *  are interested in communicating with each other have to use either the
 *  socket descriptor or the communication port handle but the mixed usage,
 *  Mixed usage results into dataloss.
 *
 *  \sa clIocCommPortCreate(), clIocSend(), clIocReceive().
 *
 */
    ClRcT clIocCommPortFdGet(
            CL_IN ClIocCommPortHandleT portHandle, 
            CL_IN const ClCharT *xportType,
            CL_INOUT ClInt32T *pSd
            );

/** 
 ************************************** 
 *  \brief Returns the port Id. 
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param pIocCommPort Handle to the communication port. 
 *  \param pPortId A pointer to the port Id, on successful 
 *  execution. 
 *  
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized 
 *  \retval CL_ERR_NULL_POINTER If the pPortId is passed as NULL. 
 * 
 *  \par Description: 
 *  This API returns the port Id for a given communication port handle.
 *  It needs to be called when the communication port related
 *  parameters are required to be set. This can be called only if the
 *  communication port is created through \e clIocCommPortCreate().
 * 
 *  \sa
 *  clIocCommPortCreate(), clIocSend(), clIocReceive(),
 *  clIocCommPortDelete(), clIocLastErrorGet(). 
 *
 */
    ClRcT clIocCommPortGet(
    CL_IN ClIocCommPortHandleT pIocCommPort,
    CL_OUT ClIocPortT * pPortId
    );


/** 
 ************************************** 
 *  \brief Enables/Disables the notifications form IOC on the port.
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param port Port number for who the notification configuration needs to be modified.
 *  \param action The notification action that needs to performed on the port.
 *  execution. 
 *  
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_DOESNT_EXIST If an invalid port is passed or if the port is not yet created.
 * 
 *  \par Description:
 *  This API enables/disables the port to receive notifications genereted by
 *  IOC. The IOC generated notifications like component/node arrival/departure. And
 *  these notifications are sent to all the components on the node. But the notifications
 *  will reach the applications only if the application enables notification for that port.
 *  And if port disables the notification then no notification received by port will reach
 *  the application. It will be dropped by IOC. By default the ports will be disabled for
 *  receiving notifications.
 * 
 *  \sa
 *  clIocCommPortCreate(), clIocCommPortGet(), clIocCommPortDelete().
 *
 */

    ClRcT clIocPortNotification(
            CL_IN ClIocPortT port,
            CL_IN ClIocNotificationActionT action
            );


/**
 **************************************
 *  \brief Sends message on a communication port.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param commPortHandle Handle to a communication port on which message
 *  is to be sent.
 *
 *  \param message The message to be sent across the communication port.
 *  The message must be created by you and the data to be sent across is
 *  be passed in this message. If the message is persistent, it must be
 *  freed by you.
 *
 *  \param protoType The protocol ID must be specified by you.
 *  \param pDestAddr A pointer to the destination address where message need
 *  to be sent.
 *  \param pSendOption The options available to send a message. If
 *  \e pSendOption is NULL, the default values are used.
 *
 *  \par The structure ClIocSendOptionT has the following fields:
 *
 *  \arg \e priority: The priority to send across a message. If the message
 *  priority is more than the maximum supported value then the message will
 *  be sent with 0 (default value) priority.
 *  \arg \e sendType: This is used in case of sending a message to a logical
 *  address. The values for this parameters are
 *  \c CL_IOC_SESSION_BASED A session will be established on sending a
 *  message with this parameter set to a logical address. Every subsequent
 *  send to the same logical address will end up going to the same physcial
 *  address without doing any lookup for physical address of that logical
 *  address. In case if the destination component changes its physical
 *  location then the send will return with an error and the session will be
 *  lost.
 *  \c CL_IOC_NO_SESSION (default value) Every time the logical address to
 *  physical address conversion is done and sent to the physical address.
 *  \arg \e msgOption: This parameters is used to define the message as
 *  peristent or non-persistent and can have any of the following two values:
 *  -# \c CL_IOC_PERSISTENT_MSG: For persistant message. These buffer messages
 *  will not be deleted on error or in case of successful sending of the
 *  message. here the sening application has to delete the buffer message.
 *  -# \c CL_IOC_NON_PERSISTENT_MSG: Default value, for non-persistant message.
 *  In this case the message will be deleted by IOC.
 *
 *  \arg \e timeout: It is the timeout value  in miliseconds. Default is 0.
 *  If 0 is passed, the first error encountered is returned. If it is
 *  non zero then this API on seeing the Flow control related messages will
 *  try to resend the message until the timeout expires and if the timeout
 *  expires in between it will return the error. If a big message
 *  is sent then fragmentation and reassembly will kickin, if the finite
 *  timeout is specified then it may return the timeout error, if the total
 *  time required to send all the fragments out is more than the 'timeout'
 *  period.
 *
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized.
 *  \retval CL_ERR_INVALID_HANDLE If Invalid communication port handle is passed.
 *  \retval CL_ERR_NULL_POINTER If the pDestAddr is passed as NULL.
 *  \retval CL_ERR_INVALID_BUFFER If the message is invalid.
 *  \retval CL_IOC_ERR_PROTO_IN_USE_WITH_IOC If the protocol id passed is in
 *  use by IOC.
 *  \retval CL_IOC_ERR_INVALID_MSG_OPTION If the message option passed in the
 *  pSendOption is invalid.
 *  \retval CL_ERR_INVALID_PARAMETER If the sendType passed in the pSendOption
 *  is invalid or the destination address is not of supported type, or
 *  the message size is 0.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NOT_EXIST If the logical address is passed and there is no
 *  mapping for it in the Transparency Layer.
 *  \retval CL_IOC_ERR_INVALID_SESSION If session based communication is
 *  requested and the destination is moved to a different location.
 *  \retval CL_IOC_ERR_FLOW_XOFF_STATE If the destination has send an XOFF message.
 *  \retval CL_IOC_ERR_HOST_UNREACHABLE If the host can not be reached.
 *  \retval CL_ERR_BUFFER_OVERRUN If the priority queue has no space left
 *  for this message.
 *  \retval CL_ERR_TIMEOUT If send operation can not be completed within the
 *   specified timeout interval.
 *  \retval CL_ERR_UNSPECIFIED Other errors.
 *
 *  \par Description:
 *  This API is used to send a message to an ASP application. The message
 *  passed can be persistent or non persistent as  specified in the \e
 *  messageType field of the \e sendoption structure. The persistant
 *  messages are required to be deleted by the sending application and
 *  non-persistant messages will be deleted by IOC on completing the send
 *  operation. Since IOC supports fragmentation and reassembly of the messages
 *  any big data can be sent. The thing that needs to be considered at the
 *  time of sending big messages is the timeout value. If the send operation 
 *  cannot completely send the data within that time it will return with
 *  CL_ERR_TIMEOUT error.
 *
 *  \sa
 *  clIocCommPortCreate(), clIocReceive(),
 *  clIocCommPortDelete().
 *
 */
    ClRcT clIocSend(
    CL_IN ClIocCommPortHandleT commPortHandle,
    CL_IN ClBufferHandleT message,
    CL_IN ClUint8T protoType,
    CL_IN ClIocAddressT * pDestAddr,
    CL_IN ClIocSendOptionT * pSendOption
    );



/**
 **************************************
 *  \brief Receives message on communication port.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param commPortHdl Handle of the given communication port.
 *
 *  \param pRecvOption  This structure is used for options like timeout.
 *  If NULL, the structure makes use of all the default values.
 *
 *  \param userMsg (out) Handle to the message. This must be created and freed by you once the call returns.
 *  After the successfully receving the data, the received data is passed to the message.
 *
 *  \param pRecvParam (out) The parameter related to the message priority, origin of the message,
 *  length of the message and protocol is returned here on successful receive. It can not be NULL.
 *  \arg \e priority The priority of the message with which the sender sent the message.
 *  \arg \e protoType The protocol of the message with which sender sent, so
 *  that the receiver can analyse the packet using that prottocol.
 *  \arg \e length Length of the message just received.
 *  \arg \e srcAddr The physical address of the sender of the message.
 *  
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized.
 *  \retval CL_ERR_INVALID_HANDLE If Invalid communication port handle is passed.
 *  \retval CL_ERR_NULL_POINTER If the pRecvParam is passed as NULL.
 *  \retval CL_ERR_INVALID_BUFFER If the message is invalid.
 *  \retval CL_IOC_ERR_TRY_AGAIN On failing to receive any message on non-blocking communication port.
 *  \retval CL_ERR_TIMEOUT If nothing is received within specified timeout interval.
 *  \retval CL_IOC_ERR_RECV_UNBLOCKED If receiver is unblocked.
 *  \retval  CL_ERR_UNSPECIFIED Other errors.
 *
 *  \par Description:
 *  This API is used to receive a message on the communication port. The
 *  messages are received as per the priority, and the same priority
 *  order is maintained through out. The behavior of this call depends
 *  on the current mode of the port, i.e., blocking or non-blocking mode. 
 *  If the mode is set to blocking and there is no data in commport receive queue, the
 *  receiver thread gets blocked. If the mode is non-blocking and there is no data
 *  attached in the commport receive queue then \c CL_IOC_ERR_TRY_AGAIN error code is
 *  returned. To receive data in non-blocking mode, the application has to poll on the communication
 *  port checking for data availability.
 *
 *  \sa
 *  clIocCommPortCreate(), clIocSend(),
 *  clIocCommPortDelete().
 *
 */


    ClRcT clIocReceive(
    CL_IN ClIocCommPortHandleT commPortHdl,
    CL_IN ClIocRecvOptionT * pRecvOption,
    CL_OUT ClBufferHandleT userMsg,
    CL_OUT ClIocRecvParamT * pRecvParam
    );

    ClRcT clIocReceiveAsync(
    CL_IN ClIocCommPortHandleT commPortHdl,
    CL_IN ClIocRecvOptionT * pRecvOption,
    CL_OUT ClBufferHandleT userMsg,
    CL_OUT ClIocRecvParamT * pRecvParam
    );

    ClRcT clIocReceiveWithBuffer(
                                 CL_IN ClIocCommPortHandleT commPortHdl,
                                 CL_IN ClIocRecvOptionT * pRecvOption,
                                 CL_IN ClUint8T *buffer,
                                 CL_IN ClUint32T bufSize,
                                 CL_OUT ClBufferHandleT userMsg,
                                 CL_OUT ClIocRecvParamT * pRecvParam
                                 );

    ClRcT clIocReceiveWithBufferAsync(
                                      CL_IN ClIocCommPortHandleT commPortHdl,
                                      CL_IN ClIocRecvOptionT * pRecvOption,
                                      CL_IN ClUint8T *buffer,
                                      CL_IN ClUint32T bufSize,
                                      CL_OUT ClBufferHandleT userMsg,
                                      CL_OUT ClIocRecvParamT * pRecvParam
                                      );


/** 
 ************************************** 
 *  \brief Unblocks all receive calls.  
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param commPortHdl Handle of the communication port to be unblocked.   
 * 
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If the IOC is not initialized. 
 *  \retval CL_ERR_INVALID_HANDLE If the communication port handle is invalid. 
 * 
 *  \par Description: 
 *  This API is used to unblock the receive call which is blocked inside IOC on  
 *  the given communication port. The blocked receive call is unblocked and returns   
 *  \c CL_IOC_RECV_UNBLOCKED after this call. The receive on this communication port stops after this call. 
 *  To start the receive again on the communication port you must call API clIocCommPortBlockRecvSet. 
 *
 *  \sa
 *  clIocCommPortCreate(), clIocCommPortDelete(), clIocCommPortModeSet(), 
 *  clIocCommPortModeGet(), clIocCommPortBlockRecvSet(), clIocCommPortDebug().
 *  
 * 
 * 
 */

    ClRcT clIocCommPortReceiverUnblock(
    CL_IN ClIocCommPortHandleT commPortHdl
    );




/*********************************************************************************/
/************************  Transparency Layer APIs  ******************************/
/*********************************************************************************/



/**
 **************************************
 *  \brief Registers an application's logical address with Transparency Layer.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \param pTLInfo (in/out) This parameter contains the logical address
 *  registration related information.
 *  \arg \e logicalAddr This is the logical address of the application which is
 *  being registered with the Transparency layer.
 *  \arg \e compId This is component Id of the application, on which the IOC
 *  receive will be blocked for receiving the data.
 *  \arg \e contextType This registers the logical address either in GLOBAL or
 *  in LOCAL context. the GLOBAL context registrations will be updated on all
 *  the nodes immediately.
 *  \arg \e haState The indicates the state of the application, whether it
 *  is active or standby.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized.
 *  \retval CL_ERR_NULL_POINTER If pTLInfo is NULL
 *  \retval CL_IOC_ERR_TL_LIMIT_EXCEEDED If there is no space left in registration database.
 *  \retval CL_ERR_INVALID_PARAMETER If context passed in pTLInfo is not a valid type.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_IOC_ERR_TL_DUPLICATE_ENTRY If the entry already exists.
 *
 *  \par Description:
 *  This API is used to register an application's logical address with the
 *  Transparency Layer. Once the registration is done the application can be
 *  reached on its logical address.
 *
 *  \sa
 *  clIocTransparencyDeregister(),
 *  clIocTransparencyLogicalToPhysicalAddrGet().
 *
 *
 */
    ClRcT clIocTransparencyRegister(
    CL_IN ClIocTLInfoT * pTLInfo
    );



/**
 **************************************
 *  \brief De-registers the application with Transparency Layer.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  ClIoc
 *
 *  \param compId Id of the component which wants to deregister all its
 *  registration with Transparency Layer.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized.
 *
 *  \par Description:
 *  This API is used to deregister the application with Transparency layer.
 *  The application cannot be reached with the logical address after this API
 *  is called for the comp id. But it can be reached through the physical
 *  address if it is known.
 *
 *  \sa clIocTransparencyRegister().
 *
 */
    ClRcT clIocTransparencyDeregister(
    CL_IN ClUint32T compId
    );




/** 
 ************************************** 
 *  \brief Registers an application against a multicast address with IOC Multicast Layer.
 * 
 *  \par Header File:
 *  clIocIpi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 *  \param pMcastInfo This parameter contains the registration related information, which includes the multicast address and IOC physical address.
 * 
 *  \retval CL_OK The function has successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized. 
 *  \retval CL_ERR_NULL_POINTER If pMcastInfo is NULL 
 *  \retval CL_ERR_INVALID_PARAMETER If any field of pMcastInfo is not a valid type. 
 *  \retval CL_ERR_NO_MEMORY IOC could not get enough memory to complete the requested operation.
 *  \retval CL_ERR_ALREADY_EXIST The specified mapping already exist. 
 * 
 *  \par Description: 
 *  This API is used to register an application against a multicast address
 *  with the Multicast Layer. The IOC physical address of an application is
 *  used to register to a multicast group, in which the application is
 *  interested, using the groups multicast address. Any message sent to the
 *  multicast group by any application in the system will reach all the
 *  applications, who have registered for that multicast address.
 *
 *  \sa clIocMulticastDeregister(),
 * 
 * 
 */
    ClRcT clIocMulticastRegister(CL_IN ClIocMcastUserInfoT *pMcastInfo);

/** 
 ************************************** 
 *  \brief De-registers the application against a multicast address
 *  with the IOC Multicast Layer.  
 * 
 *  \par Header File:
 *  clIocIpi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 *  \param pMcastInfo This parameter contains the multicast 
 *  deregistration information which includes physical address to be de-registered and the multicast address
 *  against which the deregistration should happen.
 *  \retval CL_OK The function has successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized.
 *  \retval CL_ERR_NULL_POINTER If pMcastInfo is NULL 
 *  \retval CL_ERR_INVALID_PARAMETER If any of the fields in pMcastInfo is
 *  ascertained to be invalid.
 *  \retval CL_ERR_NOT_EXIST This physical address was not registered 
 *  against this multicast address through a previous invocation of 
 *  clIocMulticastRegister.
 *
 *  \par Description: 
 *  This API is called to deregister the application from the multicast group 
 *  identifed by multicastAddress. The physical address is deregistered from  
 *  the multicast address' list. The multicast address is also
 *  removed from the multicast table if the deregistred address was the
 *  only left physical address with that multicast address group.
 *
 *  \sa clIocMulticastRegister(), clIocMulticastDeregisterAll().
 *  
 *  
 * 
 */
    ClRcT clIocMulticastDeregister(CL_IN ClIocMcastUserInfoT *pMcastInfo);

/** 
 ************************************** 
 *  \brief De-registers the application against a multicast address
 *  with the IOC Multicast Layer.  
 * 
 *  \par Header File:
 *  clIocIpi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 *  \param pMcastAddress This parameter contains the multicast 
 *  deregistration information which includes the multicast address
 *  against which the deregistration should happen.All the physical
 *  addresses for the given multicast address are deregistered along
 *  with the multicast address.
 *  \retval CL_OK The function has successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized.
 *  \retval CL_ERR_NULL_POINTER If pMcastInfo is NULL 
 *  \retval CL_ERR_INVALID_PARAMETER If any of the fields in pMcastInfo is
 *  ascertained to be invalid.
 *  \retval CL_ERR_NOT_EXIST This multicast address was not registered 
 *  through a previous invocation of clIocMulticastRegister.
 *
 *  \par Description: 
 *  This API is called to deregister all the applications from the multicast group 
 *  identifed by multicastAddress. The physical addresses for the 
 *  multicast address are deregistered. The multicast address is also
 *  removed from the multicast table. This should be called only when a node
 *  is shutting down. This is good if called only from ASP AMF component, since
 *  only this component know when the node is going down.
 *
 *  \sa 
 *  clIocMulticastRegister(),
 *  clIocMulticastDeregister().
 *  
 *  
 * 
 */
    ClRcT clIocMulticastDeregisterAll(CL_IN ClIocMulticastAddressT *pMcastAddress);




/**
 **************************************
 *  \brief Returns the local IOC node addrress.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \par Parameters:
 *  None
 *
 *  \par Return values:
 *  On successful execution this API returns the local IOC node
 *  address. In case of error, the API returns 0.
 *
 *  \par Description:
 *  This API returns the IOC node address of the current node.
 *
 */

    ClIocNodeAddressT clIocLocalAddressGet(
    void
    );




/**
 **************************************
 *  \brief Checks for appropriate version of application.
 *
 *  \par Header File:
 *  clIocApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK The API is successfully executed.
 *
 *  \par Description:
 *  This API checks whether the application version matches
 *  with any of the supported versions of the IOC client
 *  library and server module. If it doesnot then returns error.
 *
 *
 */

    ClRcT clIocVersionCheck(
    CL_IN ClVersionT * pVersion
    );


#if 0

    ClRcT clIocTransparencyDeregisterNode(
    CL_IN ClIocNodeAddressT nodeId
    ) CL_DEPRECATED;

#endif

    ClRcT clIocTransparencyLogicalToPhysicalAddrGet(
    CL_IN ClIocLogicalAddressT logicalAddr,
    CL_OUT ClIocTLMappingT ** pPhysicalAddr,
    CL_OUT ClUint32T * pNoEntries
    ) CL_DEPRECATED;


# ifdef __cplusplus
}
# endif
#endif                          /* _CL_IOC_API_H_ */

/**
 *  \}
 */
