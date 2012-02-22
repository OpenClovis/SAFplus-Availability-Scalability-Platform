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
 * File        : clIocIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description : This file contains Clovis IOC data structures and function
 * prototyes, which are used internally only for testing.                                                              
 *******************************************************************************/

#ifndef _CL_IOC_IPI_H_
# define _CL_IOC_IPI_H_

# include <clTimerApi.h>
# include <clIocApi.h>
# include <clIocApiExt.h>

# ifndef __KERNEL__

#  ifdef __cplusplus
extern "C"
{
#  endif

# endif


# define CL_IOC_ADDRESS_TYPE_FROM_NODE_ADDRESS(nodeAddress) (nodeAddress >> 16)

/** 
 * The communication port mode. 
 */
    typedef enum ClIocCommPortMode
    {

    /** 
     * The receive operation is blocked till it receives something or 
     * the timeout happens. 
     */
        CL_IOC_BLOCKING_MODE = 1,

    /** 
     * The receive operation returns back, if there is nothing to receive 
     * on the communication port. 
     */
        CL_IOC_NON_BLOCKING_MODE = 2,

    /** 
     * Invalid Mode. 
     */
        CL_IOC_INVALID_MODE
    } ClIocCommPortModeT;

    typedef struct ClIocDispatchOption
    {
        ClUint32T timeout;
        ClBoolT sync;
    } ClIocDispatchOptionT;

    typedef ClRcT (*ClIocNotificationRegisterCallbackT)(ClIocNotificationT *notification, ClPtrT cookie);
    extern ClBoolT gClIocTrafficShaper;

/*****************communication port  management APIs***********************************/

/** 
 ************************************** 
 *  \page pageIOC108 clIocCommPortModeSet 
 * 
 *  \par Synopsis: 
 *  Sets the communication port in blocking or non-blocking mode. 
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 * 
 *  \par Syntax: 
 *  \code ClRcT clIocCommPortModeSet( 
 *                CL_IN ClIocCommPortHandleT iocCommPort, 
 *                CL_IN ClIocCommPortModeT modeType); 
 *  \endcode 
 * 
 *  \param commPortHandle: Handle of the communication port for which mode is to changed. 
 *  \param modeType: Mode type. It can be blocking or non-blocking.  
 *  
 *  \retval CL_OK: The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED: If the IOC is not initialized. 
 *  \retval CL_IOC_ERR_COMMPORT_INVALID_MODE: If the invalid mode is passed. 
 *  \retval CL_IOC_ERR_COMMPORT_BLOCKED: If the communication port is blocked. 
 *  \retval CL_ERR_INVALID_HANDLE: If the communication port handle is invalid. 
 * 
 *  \par Description: 
 *  This API is used to set the communication port in blocking or non-blocking  mode of operation.  
 *  The mode value can be \e ClIocCommPortModeT enumeartion. This API should be called only when 
 *  no receive or send operation is blocked in the communication port. It is recommended to invoke 
 *  clIocCommPortReceiverUnblock API before this call. 
 *
 *  \par Related APIs:
 *  clIocCommPortCreate(), clIocCommPortDelete(), clIocCommPortModeGet(), 
 *  clIocCommPortReceiverUnblock(), clIocCommPortBlockRecvSet(), clIocCommPortDebug().
 *  
 */
    ClRcT clIocCommPortModeSet(CL_IN ClIocCommPortHandleT iocCommPort,
                               CL_IN ClIocCommPortModeT modeType);



/** 
 ************************************** 
 *  \page pageIOC109 clIocCommPortModeGet 
 * 
 *  \par Synopsis: 
 *  Returns the current mode of communication port. 
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 * 
 *  \par Syntax: 
 *  \code ClRcT clIocCommPortModeGet( 
 *                  CL_IN ClIocCommPortHandleT iocCommPort, 
 *                  CL_OUT ClIocCommPortModeT *pPortMode); 
 *  \endcode 
 * 
 *  \param commPortHandle: Handle of the communication port for which mode is returned. 
 *  \param pPortMode: (out) Current mode of the communication port. 
 * 
 *  \retval CL_OK: The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED: If the IOC is not initialized. 
 *  \retval CL_ERR_INVALID_HANDLE: If the communication port handle is invalid. 
 *  \retval CL_ERR_NULL_POINTER: If the pPortMode is passed as NULL. 
 * 
 *  \par Description: 
 *  This API returns the current mode of the communication port that can be
 *  either blocking or non-blocking. This mode is one of the \e ClIocCommPortModeT
 *  enumeartion values.
 *
 *  \par Related APIs:
 *  clIocCommPortCreate(), clIocCommPortDelete(), clIocCommPortModeSet(), 
 *  clIocCommPortReceiverUnblock(), clIocCommPortBlockRecvSet(), clIocCommPortDebug().
 *  
 * 
 */

    ClRcT clIocCommPortModeGet(CL_IN ClIocCommPortHandleT iocCommPort,
                               CL_OUT ClIocCommPortModeT *pPortMode);




/** 
 ************************************** 
 *  \page pageIOC111 clIocCommPortBlockRecvSet 
 * 
 *  \par Synopsis: 
 *  Starts the receive process again on the communication port. 
 * 
 *  \par Header File:
 *  clIocApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 * 
 *  \par Syntax: 
 *  \code ClRcT clIocCommPortBlockRecvSet( 
 *                  CL_IN ClIocCommPortHandleT commPortHdl); 
 *  \endcode 
 * 
 *  \param commPortHdl: Handle of the communication port on which receive can be restarted. 
 * 
 *  \retval CL_OK: The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized. 
 *  \retval CL_ERR_INVALID_HANDLE: If the communication port handle is invalid. 
 * 
 *  \par Description: 
 *  This API is used to start the receive again on the communication port on which the API  
 *  clIocCommPortReceiverUnblock was called earlier. 
 *
 *  \par Related APIs:
 *  clIocCommPortCreate(), clIocCommPortDelete(), clIocCommPortModeSet(), 
 *  clIocCommPortModeGet(), clIocCommPortReceiverUnblock(), clIocCommPortDebug().
 *  
 * 
 * 
 */

    ClRcT clIocCommPortBlockRecvSet(CL_IN ClIocCommPortHandleT commPortHdl);




/**********************IOC Neighbor's information**********************************/





/********************IOC Addressing Related APIs*********************************/

/**
 **************************************
 *  \page pageIOC216 clIocGeoAddressTablePrint
 *
 *  \par Synopsis:
 *  Prints Geographical addresses.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code     ClRcT clIocGeoAddressTablePrint(void);
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *
 *  \par Description:
 *  This API is used to print Geographical addresses of all the entries in the table.
 *
 *  \par Related APIs:
 *  clIocGeographicalAddressGet(), clIocGeographicalAddressSet().
 *
 */
    ClRcT clIocGeoAddressTablePrint(void);


/*****************************************/

/**
 **************************************
 *  \page pageIOC205 clIocRouteStatusChange
 *
 *  \par Synopsis:
 *  Changes the status of the route.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code     ClRcT clIocRouteStatusChange(
 *                CL_IN ClIocNodeAddressT destAddr,
 *                CL_IN ClUint16T  prefixLen,
 *                CL_IN ClUint8T status);
 *  \endcode
 *
 *  \param destAddr: IOC destination address
 *  \param prefixLen: The length of prefix.
 *  \param status: The new status of route. The status can be \c CL_IOC_ROUTE_UP
 *  or \c CL_IOC_ROUTE_DOWN.
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER: If the prefix length is incorrect,
 *      or destAddr is local or broadcast address.
 *      or the prefix length or the route status is incorrect.
 *  \retval CL_IOC_ERR_ROUTE_NOT_EXIST: If there is no matching route.
 *
 *  \par Description:
 *  This API is used to change the status of a given route.
 *  The blade address and the prefix length are needed to
 *  search the route.
 *  The following are the statuses for route entry.
 *  -# \c CL_IOC_ROUTE_UP
 *  -# \c CL_IOC_ROUTE_DOWN
 *
 *  \par Related APIs:
 *  clIocRouteInsert(), clIocRouteDelete(), clIocRouteTablePrint(),
 *  clIocRoutingTableFlush().
 *
 *
 */
    ClRcT clIocRouteStatusChange(CL_IN ClIocNodeAddressT destAddr,
                                 CL_IN ClUint16T prefixLen,
                                 CL_IN ClUint8T status);


/**
 **************************************
 *  \page pageIOC207 clIocRoutingTableFlush
 *
 *  \par Synopsis:
 *   Deletes the routing entries.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code ClRcT clIocRoutingTableFlush();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *
 *  \par Description:
 *  This API is used to delete all the routing entries from the routing
 *  database. Subsequent route related calls other than \e clIocRouteInsert
 *  will fail.
 *
 *  \par Related APIs:
 *  clIocRouteInsert(), clIocRouteDelete(), clIocRouteStatusChange(),
 *  clIocRouteTablePrint().
 *
 *
 */
    ClRcT clIocRoutingTableFlush(void);


/**
 **************************************
 *  \page pageIOC210 clIocArpEntryStatusChange
 *
 *  \par Synopsis:
 *   Changes the status of an ARP entry.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code     ClRcT clIocArpEntryStatusChange(
 *                CL_IN ClIocNodeAddressT iocAddr,
 *                CL_IN ClUint8T *pXportName,
 *                CL_IN ClUint8T* pLinkName,
 *                CL_IN ClUint8T status);
 *  \endcode
 *
 *  \param iocAddr: The IOC address whose ARP entry status needs to be changed.
 *  \param pXportName: The Transport Name on which the ARP  entry is
 *  learnt or associated.
 *  \param pLinkName: The Link name from which the ARP entry is learnt or added.
 *  \param status: The new status. The status can be \c CL_IOC_ROUTE_UP or
 *  \c CL_IOC_ROUTE_DOWN.
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER: If the \e pXportName or
 *     \e pLinkName size is more than \c CL_IOC_MAX_XPORT_NAME_LENGTH.
 *     or local transport is passed as parameter
 *     or local address or broadcast address is passed as \e iocAddr
 *     or status is not valid.
 *  \retval CL_ERR_NO_MEMORY: If memory allocation fails.
 *  \retval CL_IOC_ERR_XPORT_NOT_REGISTERED: If transport is not registered.
 *  \retval CL_ERR_NOT_EXIST: If ARP entry does not exist.
 *
 *  \par Description:
 *  This API changes the status of an ARP entry. The ARP entry can have the 
 *  following two status:
 *  -# \c CL_IOC_NODE_UP
 *  -# \c CL_IOC_NODE_DOWN
 *
 *  \par Related APIs:
 *  clIocArpInsert(), clIocArpDelete(), clIocArpTablePrint().
 *
 *
 */
    ClRcT clIocArpEntryStatusChange(CL_IN ClIocNodeAddressT iocAddr,
                                    CL_IN ClCharT *pXportName,
                                    CL_IN ClCharT *pLinkName,
                                    CL_IN ClUint8T status);



    ClRcT clIocArpTableGet(ClIocNodeAddressT destAddress,ClCharT *pXportName,
            ClCharT *pLinkName,ClUint8T **ppAddress,ClUint32T *pAddressSize);
    
/***********************IOC Information related  APIs*************************/


/**
 **************************************
 *  \page pageIOC214 clIocStatisticsPrint
 *
 *  \par Synopsis:
 *  Prints the statistics of the IOC.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code     ClRcT clIocStatisticsPrint();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *
 *  \par Description:
 *  This API is used to print the statistics of the IOC. It displays the
 *  number of messages, bytes sent and received, and the number of messages
 *  dropped. It also displays the errors in sending and receiving messages
 *  (CheckSum errors, route and ARP errors, etc).
 *
 *  \par Related APIs:
 *
 *
 */
    ClRcT clIocStatisticsPrint(void);

/***************************TL API*****************************/

/**
 **************************************
 *  \page pageIOC215 clIocTransparencyLayerBindingsListShow
 *
 *  \par Synopsis:
 *  Prints all entries in a given context.
 *
 *  \par Header File:
 *  clIocManagementApi.h
 *
 *  \par Library Files:
 *  libClIoc
 *   
 *
 *
 *  \par Syntax:
 *  \code     ClRcT clIocTransparencyLayerBindingsListShow(
 *                       CL_IN ClUint32T contextId);
 *  \endcode
 *
 *  \param contextId: Id of the context.
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER: If contextId is invalid.
 *
 *  \par Description:
 *  This API is used to print all the entries in a given context.
 *
 *  \par Related APIs:
 *
 *
 */

    ClRcT clIocTransparencyLayerBindingsListShow(CL_IN ClUint32T contextId);




/**
 *******************************************
 *  \page pageIOC305 clIocTransportStatsPrint
 *
 *  \par Synopsis:
 *   Prints the status of transport register.
 * 
 *  \par Header File:
 *  clIocTransportApi.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 * 
 *  \par Syntax:
 *  \code     ClRcT clIocTransportStatsPrint(void);
 *  \endcode
 * 
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API is successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized. 
 *
 *  \par Description:
 *  This API is used to print the statistic of transport. This will print
 *  the data on the console.
 * 
 *  \par Related APIs:
 *  clIocTransportRegister(), clIocLinkRegister(). 
 *  
 *
 */
    ClRcT clIocTransportStatsPrint(void);


/******************************************************************************/
/************************  Multicast Layer APIs  ******************************/
/******************************************************************************/

    typedef enum ClIocUserOpType
    {
        CL_IOC_MCAST_TO_PA_OP,
        CL_IOC_PA_TO_MCAST_OP,
        CL_IOC_LA_TO_PA_OP,
        CL_IOC_PA_TO_LA_OP,
        CL_IOC_NODE_TO_MCAST_OP,
        CL_IOC_NODE_TO_PA_OP,
        CL_IOC_NODE_TO_ARP_OP,
        CL_IOC_NODE_TO_ROUTE_OP,
        CL_IOC_NODE_TO_TL_OP,
        CL_IOC_MAX_OP,
    } ClIocUserOpTypeT;

    typedef struct ClIocMappingArgs
    {
        ClIocAddressT *pAddress;
        void *pAddressList;
        ClUint32T numEntries;
        ClUint32T maxNumEntries;
        ClIocUserOpTypeT mapping;
        ClBoolT copy;
        void *privateData;
    } ClIocMappingArgsT;


/** 
 ************************************** 
 *  \page pageIOC119 clIocGetEntries
 * 
 *  \par Synopsis: 
 *  Display the information based on the user provided 
 *  address mapping.
 *
 *
 *  \par Header File:
 *  clIocIpi.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 * 
 *  \par Syntax: 
 *  \code ClRcT clIocGetEntries(CL_INC ClIocUserOpTypeT op,
 *                        CL_IN ClIocAddressT *pAddress,
 *						  CL_OUT void        **ppAddressList,
 *                        CL_OUT ClUint32T     *pNumEntries)
 *  \endcode 
 * 
 *  \param op: (in) This parameter refers to the type of information being sought.
 *  This parameter can have the following five values:
 *  \arg \c CL_IOC_MCAST_TO_PA_OP: Given a multicast address, return all the physical address on the current node.
 *  The return type would ClIocAddressT.
 *  \arg \c CL_IOC_PA_TO_MCAST_OP: Given a physical address, return all the multicast addresses it is part of.
 *  The physical address must be hosted on current node.The return type would be ClIocAddressT.
 *  \arg \c CL_IOC_PA_TO_LA_OP: Given a physical address, return all the logical addresses associated with it.
 *  The return type would be ClIocTLInfoT.
 *  \arg \c CL_IOC_LA_TO_PA_OP: Given a logical address, return all the physical addresses associated with it.
 *  The physical address must be hosted on current node.The return type would be ClIocAddressT.
 *  \arg \c CL_IOC_NODE_TO_MCAST_OP: For current node, return all the multicast address on that node.
 *  The return type would be ClIocAddressT.
 *  \arg \c CL_IOC_NODE_TO_PA_OP: For current node, return all the active physical addresses on that node.
 *  The return type would be ClIocAddressT.
 *
 * \param pAddress: (in) A pointer to the address which is used to obtain
 *  the physical addresses,multicast addresses or logical addresses 
 *  for a given mapping and address type. For CL_IOC_NODE_TO_MCAST_OP and CL_IOC_NODE_TO_PA_OP, this is ignored.
 *
 * \param ppAddressList: (out) This parameter returns the address of the list
 * for a given mapping. The return type is a generic type which should be typecasted based on the mapping.
 * The return types possible currently are ClIocTLInfoT for CL_IOC_PA_TO_LA_OP and ClIocAddressT for 
 * all the mappings except CL_IOC_PA_TO_LA_OP.
 * The memory is allocated by the IOC and must be freed by the caller.
 *
 * \param pNumEntries: (out) This parameter returns the number of addresses
 * for a given mapping.
 *
 *  \retval CL_OK: The function has successfully executed.
 *  \retval CL_ERR_NOT_INITIALIZED: If IOC is not initialized.
 *  \retval CL_ERR_NULL_POINTER: One of the pointer argument is NULL. 
 *  \retval CL_ERR_INVALID_PARAMETER: Either the parameter op is invalid or the address pointed by pAddress is invalid.
 *  \retval CL_ERR_NOT_EXIST: The address information does not exist
 *  for the mapping that was provided.
 *  \retval CL_ERR_NO_MEMORY: IOC could not get memory to complete the 
 *  requested operation.
 *  \par Description: 
 *  This API is used to display the IOC address information based
 *  on a address mapping. The address information could be a list of
 *  multicast addresses, physical addresses,logical addresses for a given 
 *  node address,physical address or multicast address.
 *
 *  \par Related APIs:
 *  clIocCommPortCreate()
 *  clIocCommPortDelete()
 *  clIocTransparencyRegister()
 *  clIocTransparencyDeregister()
 *  clIocMulticastRegister()
 *  clIocMulticastDeregister()
 *  
 * 
 */

    ClRcT clIocGetEntries(CL_IN ClIocUserOpTypeT op,
                          CL_IN ClIocAddressT *pAddress,
                          CL_OUT void **ppAddressList,
                          CL_OUT ClUint32T *pNumEntries);

    ClRcT clIocDisplayEntries(CL_IN ClIocUserOpTypeT mapping,
                              CL_IN ClIocAddressT *pAddress,
                              CL_IN void *pAddressList,
                              CL_IN ClUint32T numEntries, ClCharT **ppBuf);

    void clIocQueueNotificationUnpack(ClIocQueueNotificationT *pSrcQueue,
                                      ClIocQueueNotificationT *pDestQueue);

/**
 ****************************************
 * \brief Returns the IOC master, with retry logic
 * 
 * \param logicalAddress Your address, for example: CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_LOG_PORT);
 * \param portID         Your port, for example, CL_IOC_LOG_PORT
 * \param pNodeAddress   This function stores the Master's address here
 *
 * \retval CL_OK This API executed successfully. 
 * \retval !CL_OK The IOC layer failed to tell us who the master is.
 * 
 * \par Description
 * This API is used to return the IOC master.  Is contains retry logic so the
 * master will be returned even during failure and transitional periods.
 * 
 * \sa  clTryIocMasterAddressGet()
 */

    ClRcT clIocMasterAddressGet(ClIocLogicalAddressT logicalAddress,
                                ClIocPortT portId,
                                ClIocNodeAddressT *pNodeAddress);

    ClRcT clIocMasterAddressGetExtended(ClIocLogicalAddressT logicalAddress,
                                        ClIocPortT portId,
                                        ClIocNodeAddressT *pNodeAddress,
                                        ClInt32T numRetries,
                                        ClTimerTimeOutT *pDelay);

/**
 ****************************************
 * \brief Gets the IOC master, without retry logic.
 * 
 * \param logicalAddress Your address, for example: CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_LOG_PORT);
 * \param portID         Your port, for example, CL_IOC_LOG_PORT
 * \param pNodeAddress   This function stores the Master's address here
 *
 * \retval CL_OK This API executed successfully. 
 * \retval !CL_OK The IOC layer failed to tell us who the master is.
 * 
 * \par Description
 * This API is used to return the IOC master.  Is contains no retry logic so the
 * master will NOT be returned correctly during failure and transitional periods.
 * 
 * \sa  clIocMasterAddressGet()
 */
    ClRcT clIocTryMasterAddressGet(ClIocLogicalAddressT logicalAddress,
                                ClIocPortT portId,
                                ClIocNodeAddressT *pNodeAddress);
    

ClRcT clIocCompStatusGet(ClIocPhysicalAddressT compAddr, ClUint8T *pStatus);
ClRcT clIocCompStatusEnable(ClIocPhysicalAddressT compAddr);

/** 
 ************************************** 
 *  \brief Returns the status of the node. 
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 *   
 *  \param iocBladeAddr The Blade address of the node whose status is queried. 
 *  \param pStatus A pointer to the node status. The node status is returned in this pointer. 
 *  
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized. 
 *  \retval CL_ERR_NULL_POINTER If \e pStatus is NULL. 
 * 
 *  \par Description: 
 *  This API returns the status of any node in the system. The node status can be 
 *  -# \c CL_IOC_NODE_UP
 *  -# \c CL_IOC_NODE_DOWN
 *  If a node is not present then it is treated as down and accordingly the
 *  status is returned and not the error.  
 *
 *  \sa
 *  
 *         
 * 
 */
    ClRcT clIocRemoteNodeStatusGet(
    ClIocNodeAddressT iocNodeAddr,
    ClUint8T * pStatus
    );

    ClRcT clIocSendWithRelay(ClIocCommPortHandleT commPortHandle,
                             ClBufferHandleT message, ClUint8T protoType,
                             ClIocAddressT *srcAddress, ClIocAddressT *destAddress, 
                             ClIocSendOptionT *pSendOption);

    ClRcT clIocSendWithXport(
                             CL_IN ClIocCommPortHandleT commPortHandle,
                             CL_IN ClBufferHandleT message,
                             CL_IN ClUint8T protoType,
                             CL_IN ClIocAddressT * pDestAddr,
                             CL_IN ClIocSendOptionT * pSendOption,
                             CL_IN ClCharT *xportType,
                             CL_IN ClBoolT proxy
    );

    ClRcT clIocSendWithXportRelay(ClIocCommPortHandleT commPortHandle,
                                  ClBufferHandleT message, ClUint8T protoType,
                                  ClIocAddressT *originAddress, ClIocAddressT *destAddress, 
                                  ClIocSendOptionT *pSendOption,
                                  ClCharT *xportType, ClBoolT proxy);

    ClRcT clIocServerReady(ClIocAddressT *pAddress);

    ClRcT clIocMcastIsRegistered(ClIocMcastUserInfoT *pMcastInfo);

    ClRcT clIocNotificationRegister(ClIocNotificationRegisterCallbackT callback, ClPtrT cookie);

    ClRcT clIocNotificationDeregister(ClIocNotificationRegisterCallbackT callback);

    ClRcT clIocNotificationRegistrants(ClIocNotificationT *notification);

    ClRcT clIocNotificationRegistrantsDelete(void);

    ClRcT clIocNotificationPacketSend(ClIocCommPortHandleT commPort, 
                                      ClIocNotificationT *pNotificationInfo, 
                                      ClIocAddressT *destAddress, 
                                      ClBoolT compat, ClCharT *xportType);

    ClRcT clIocNotificationNodeStatusSend(ClIocCommPortHandleT commPort, ClUint32T status,
                                          ClIocNodeAddressT notificationNodeAddr,
                                          ClIocAddressT *allLocalComps, 
                                          ClIocAddressT *allNodeReps,
                                          ClCharT *xportType);

    ClRcT clIocNotificationCompStatusSend(ClIocCommPortHandleT commPort, ClUint32T status,
                                          ClIocPortT portId,
                                          ClIocAddressT *allLocalComps, 
                                          ClIocAddressT *allNodeReps, ClCharT *xportType);

    ClRcT clIocNotificationPacketRecv(ClIocCommPortHandleT commPort, ClUint8T *recvBuff, ClUint32T recvLen,
                                      ClIocAddressT *allLocalComps, ClIocAddressT *allNodeReps,
                                      void (*syncCallback)(ClIocPhysicalAddressT *srcAddr,
                                                           ClPtrT syncArg), ClPtrT syncArg,
                                      ClCharT *xportType);

    ClRcT clIocNotificationInitialize(void);
    
    ClRcT clIocNotificationFinalize(void);

    void clIocMasterCacheReset(void);

    void clIocMasterCacheSet(ClIocNodeAddressT master);

    ClRcT clIocHighestNodeAddressGet(ClIocNodeAddressT *pNodeAddress);
 
    ClRcT clIocLowestNodeAddressGet(ClIocNodeAddressT *pNodeAddress);
    
    ClBoolT clAspNativeLeaderElection(void);

    ClRcT clIocDispatch(const ClCharT *xportType, 
                        ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption,
                        ClUint8T *buffer, ClUint32T bufSize, ClBufferHandleT message,
                        ClIocRecvParamT *pRecvParam);
    
    ClRcT clIocDispatchAsync(const ClCharT *xportType, ClIocPortT port, 
                             ClUint8T *buffer, ClUint32T bufSize);

# ifndef __KERNEL__

#  ifdef __cplusplus
}
#  endif

# endif

#endif
