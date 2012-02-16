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
 * File        : clIocApiExt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This is the interface to extended API supported by IOC.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of extended APIs supported by IOC
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */

#ifndef _CL_IOC_ASP_INTERNAL_H_
# define _CL_IOC_ASP_INTERNAL_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <clIocApi.h>
    
    /*
     * Send related Flags 
     */
/** 
 * No session is maintained if the flag is set to 0 (zero). 
 */
# define CL_IOC_NO_SESSION 0

/** 
 * Flag must be set to enable session based send. 
 */
# define CL_IOC_SESSION_BASED (1<<0)



/**
 * The port close notification payload 
 *
 */
    typedef enum ClIocNotificationId
    {
        CL_IOC_NODE_ARRIVAL_NOTIFICATION,
        CL_IOC_NODE_LEAVE_NOTIFICATION,
        CL_IOC_COMP_ARRIVAL_NOTIFICATION,
        CL_IOC_COMP_DEATH_NOTIFICATION,
        CL_IOC_SENDQ_WM_NOTIFICATION,
        CL_IOC_COMM_PORT_WM_NOTIFICATION,
        CL_IOC_LOG_NOTIFICATION,
        CL_IOC_NODE_VERSION_NOTIFICATION,
        CL_IOC_NODE_VERSION_REPLY_NOTIFICATION,
        CL_IOC_NODE_DISCOVER_NOTIFICATION,
    } ClIocNotificationIdT;
    
    typedef struct ClIocQueueNotification
    {
        ClWaterMarkIdT wmID;
        ClWaterMarkT wm;
        ClUint32T queueSize;
        ClUint32T messageLength;
    } ClIocQueueNotificationT;

    typedef struct ClIocNotification
    {
        /*Notification ID*/
        ClIocNotificationIdT id;
        ClUint32T protoVersion;
        ClIocAddressT nodeAddress;
        ClUint32T nodeVersion;
        union  
        {
            /*
             * Node address and comp death would just need
             * node address  
            */
            struct sendqWMNotification
            {
                ClIocQueueNotificationT queueNotification;
            } sendqWMNotification;

            struct commPortWMNotification
            {
                ClIocQueueNotificationT queueNotification;
            } commPortWMNotification;

        } notificationData;

        #define sendqWMNotification    notificationData.sendqWMNotification.queueNotification
        #define commPortWMNotification notificationData.commPortWMNotification.queueNotification

    } ClIocNotificationT;


/** 
 ************************************** 
 *  \brief Returns the maximum payload size. 
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param pSize The maximum supported payload size is returned in this. 
 * 
 *  \retval CL_OK The API is successfully executed. 
 *  \retval  CL_ERR_NOT_INITIALIZED If IOC is not initialized. 
 *  \retval CL_ERR_NULL_POINTER If pSize is NULL. 
 * 
 *  \par Description: 
 *  This API returns the maximum payload size that can be sent over
 *  the IOC. This doesnot include the IOC header size.
 *  
 *  \note
 *  In this release there is no limit over the payload size in 
 *  IOC, so this API may not be very useful. 
 *
 * 
 */
    ClRcT clIocMaxPayloadSizeGet(
    CL_OUT ClUint32T * pSize
    );



/** 
 ************************************** 
 *  \brief Returns the total number of neighbour nodes.  
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param pNumberOfEntries (out) Number of neighbor nodes. 
 * 
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized. 
 *  \retval CL_ERR_NULL_POINTER If \e pNumberOfEntries is NULL.
 * 
 *  \par Description: 
 *  This API returns the total number of neighbor nodes(including
 *  duplicates and local) of the current node. This should be
 *  called before the clIocNeighborListGet() is called.
 *
 *  \sa
 *  clIocNeighborListGet().
 *          
 * 
 */
    ClRcT clIocTotalNeighborEntryGet(
    CL_OUT ClUint32T * pNumberOfEntries
    );



/** 
 ************************************** 
 *  \brief Returns the list of neighbours IOC nodes. 
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \param pNumberOfEntries (in/out) The number of entries the array can
 *  hold is passed by you. IOC will modify this number if it fills less
 *  number of entries in the pAddrList ayrray. 
 * 
 *  \param pAddrList (out) The Array of IOC node address passed by you. The number  
 *  of entries an array can hold is passed in the other parameter \e pNumberOfEntries. 
 * 
 *  \retval CL_OK The API is successfully executed. 
 *  \retval CL_ERR_NOT_INITIALIZED If IOC is not initialized. 
 *  \retval CL_ERR_NULL_POINTER If either \e pNumberOfEntries or \e pAddrList is NULL.          
 *  \retval CL_ERR_NO_MEMORY If the memory allocation or any other resource 
 *  allocation fails. 
 * 
 *  \par Description: 
 *  This API returns the list of neighbor-IOC nodes including the local node.
 *  It takes an Array of ClIocNodeAddressT and the number of entries the
 *  array can hold. IOC will pass the list of neighbors in the array.  
 *  <BR><BR> If the number of entries is less than the passed array size then
 *  pNumberOfEntries is used to inform the exact number of entries.
 *  <BR><BR> You must call the API clIocTotalNeighborEntryGet() to get the total number
 *  of neighbors and accordingly the space to get the addresses can be allocated.
 *
 *  \sa
 *  clIocTotalNeighborEntryGet().
 * 
 * 
 */
    ClRcT clIocNeighborListGet(
    CL_INOUT ClUint32T * pNumberOfEntries,
    CL_OUT ClIocNodeAddressT * pAddrList
    );



/** 
 ************************************** 
 *  \brief Configures and initializes the IOC.
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \par Parameters: 
 *   None 
 * 
 *  \retval CL_OK The API is successfully executed. 
 *  \retval  CL_IOC_ERR_INIT_FAILED If IOC initialization fails.
 * 
 *  \par Description: 
 *  This API is configures and initialize the IOC. This API needs to be
 *  called before any other function of IOC. This function also initiates
 *  the transport configuration and initialization.
 *
 *  \sa
 *  clIocLibFinalize(), clIocLibConfigGet(). 
 * 
 */
    ClRcT clIocLibInitialize(
            ClPtrT pConfig
            );




/** 
 ************************************** 
 *  \brief Cleans up the IOC. 
 * 
 *  \par Header File:
 *  clIocApiExt.h
 *  
 *  \par Library Files:
 *  libClIoc
 * 
 *  \par Parameters: 
 *   None 
 * 
 *  \retval CL_OK The API is successfully executed. 
 * 
 *  \par Description: 
 *  This API is used to perform IOC clean up. This deregists all the
 *  trnasport and cleans up all the data held by it. After this call
 *  no IOC call should be made.
 *
 *  \sa
 *  clIocLibInitialize().
 * 
 * 
 */
    ClRcT clIocLibFinalize(
    void
    );



    ClRcT clIocCommPortDebug(CL_IN ClIocPortT portId, CL_IN ClCharT *pCommand) CL_DEPRECATED;

    ClRcT clIocGeographicalAddressGet(
    CL_IN ClIocNodeAddressT iocNodeAddr,
    CL_OUT ClCharT * pGeoAddr
    ) CL_DEPRECATED;

    ClRcT clIocGeographicalAddressSet(
    CL_IN ClIocNodeAddressT iocNodeAddr,
    CL_IN ClCharT * pGeoAddr
    ) CL_DEPRECATED;

    ClRcT clIocSessionReset(
    CL_IN ClIocCommPortHandleT iocCommPortHdl,
    CL_IN ClIocLogicalAddressT * pIocLogicalAddress
    ) CL_DEPRECATED;

    ClRcT clIocBind(
    CL_IN ClNameT * toName,
    CL_OUT ClIocToBindHandleT * pToHandle
    ) CL_DEPRECATED;


# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_ASP_INTERNAL_H_ */

/**
 *  \}
 */
