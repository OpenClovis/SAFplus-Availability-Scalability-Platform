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

/**
 *  \file
 *  \brief Header file for APIs and data types exposed by CPM for
 *         providing extended functionality
 *  \ingroup cpm_apis
 */

/**
 *  \addtogroup cpm_apis
 *  \{
 */

#ifndef _CL_CPM_EXT_API_H_
#define _CL_CPM_EXT_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCpmApi.h>

#include <clEventApi.h>
#include <clCorMetaData.h>

/*
 * The static target slot/target info read from targetconf.xml
 */

typedef struct ClTargetSlotInfo
{
    ClCharT name[80];
    ClInt32T addr;
    ClCharT linkname[40];
    ClCharT arch[80];
} ClTargetSlotInfoT;

typedef struct ClTargetInfo
{
    ClCharT version[80];
    ClCharT trapIp[40];
    ClBoolT installPrerequisites;
    ClBoolT instantiateImages;
    ClBoolT createTarballs;
    ClInt32T tipcNetid;
    ClInt32T gmsMcastPort;
    ClUint32T numSlots;
} ClTargetInfoT;

/**
 * The enum which indicates which field of ClCpmSlotInfoT is set and
 * all other information related to it will be filled. It is used by
 * the API clCpmSlotInfoGet().
 */
typedef enum
{
    /**
     * Flag indicating that slot ID of the node
     * is being passed.
     */
    CL_CPM_SLOT_ID,
    /**
     * Flag indicating that IOC address of the node
     * is being passed.
     */
    CL_CPM_IOC_ADDRESS,
    /**
     * Flag indicating that MO ID of the node is
\
     * being passed.
     */
    CL_CPM_NODE_MOID,
    /**
     * Flag indicating that name of the node is
     * being passed.
     */
    CL_CPM_NODENAME,

} ClCpmSlotInfoFieldIdT;

/**
 * The structure filled by the clCpmSlotInfoGet() API.
 */
typedef struct
{
    /**
     * Slot ID of the node.
     */
    ClUint32T           slotId;
    /**
     * IOC address of the node.
     */
    ClIocNodeAddressT   nodeIocAddress;
    /**
     * MOID of the node
     */
    ClCorMOIdT          nodeMoId;
    /**
     * Name of the node.
     */
    ClNameT             nodeName;
}ClCpmSlotInfoT;

/*
 * CPM node config set.
 */
typedef struct ClCpmNodeConfig
{
    ClCharT nodeName[CL_MAX_NAME_LENGTH];
    ClNameT nodeType;
    ClNameT nodeIdentifier;
    ClNameT nodeMoIdStr;
    ClCharT cpmType[CL_MAX_NAME_LENGTH];
}ClCpmNodeConfigT;

/**
 * Types of the events published by the Component Manager.
 */
typedef enum
{
    /**
     * Component death event.
     */
    CL_CPM_COMP_EVENT,
    /**
     * Node arrival/departure event.
     */
    CL_CPM_NODE_EVENT
} ClCpmEventTypeT;
    
/**
 ************************************
 *  \brief Extracts the event payload data.
 * 
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param eventHandle Event handle.
 *  \param eventDataSize Size of the event data.
 *  \param cpmEventType Type of the event published.
 *  \param payLoad (out) The actual payload.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 * 
 *  \par Description:
 *  This API extracts the payload data from the flat buffer returned by
 *  the event delivery callback for the events published by the CPM. The
 *  \e eventHandle and \e eventDataSize are the parameters obtained from 
 *  event delivery callback.
 *  This API must be called only in the event subscriber's context of event 
 *  delivery callback, which is invoked when the event is published by the 
 *  CPM.
 * 
 *  \par Library Files:
 *  ClAmfClient 
 *
 */
extern ClRcT clCpmEventPayLoadExtract(CL_IN ClEventHandleT eventHandle, 
                                      CL_IN ClSizeT eventDataSize,
                                      CL_IN ClCpmEventTypeT cpmEventType,
                                      CL_OUT void *payLoad);

/**
 ************************************
 *  \brief Returns the process ID of the component.
 * 
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param compName Name of the component.
 *  \param[out] pid Process ID of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_EXIST On passing invalid component name.
 * 
 *  \par Description:
 *  This API returns the process id \e pid of the component \e compName.
 * 
 *  \par Library Files:
 *  ClAmfClient 
 *
 */
extern ClRcT clCpmComponentPIDGet(CL_IN ClNameT *compName,
                                  CL_OUT ClUint32T *pid);

/**
 ************************************
 *  \brief  Returns the Slot related information
 *          [node moID, nodeName and IocAddress], provides the mapping
 *          between slot ID, IOC address, mo ID and name of the given node
 *          depending on the flag that is passed to it.
 * 
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param flag The flag which indicates that which field of the structure
 *  \e slotInfo is filled in by the user.
 *  \param slotInfo The structure which will get filled by this API and will
 *  contain other information about node corresponding to the field which was 
 *  filled by the user.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_DOESNT_EXIST On passing the invalid flag or passing 
 *  invalid value in any of the fields.
 * 
 *  \par Description:
 *  This API provides mapping between the attributes which are unique to node
 *  namely, slot ID, IOC address, MOID and node name. Depending on the flag
 *  which is given by the user, this API assumes that the corresponding field
 *  has been given a proper value by the user and fills the remaining fields 
 *  of the structure corresponding to the information in this field. For example
 *  suppose the user wants to know the MOID given the slot ID, then he should
 *  pass the flag CL_CPM_SLOT_ID (which are exposed in clCpmApi.h) and set the
 *  \e slotId field of \e slotInfo structure to some valid slot number.
 * 
 *  \par Library Files:
 *  libClAmfClient 
 * 
 */

/*
 * CM requirement.
 * Single function which returns the consolidated information
 * about the slot.
 */
extern ClRcT clCpmSlotInfoGet(CL_IN ClCpmSlotInfoFieldIdT flag,
                              CL_OUT ClCpmSlotInfoT *slotInfo);


/**
 ************************************
 *  \brief  Returns the Slot related information
 *          [ nodeName and IocAddress], provides the mapping
 *          between slot ID, IOC address,and name of the given node
 *          depending on the flag that is passed to it.
 * 
 *  \par Header File:
 *  clCpmExtApi.h
 *
 *  \param flag The flag which indicates that which field of the structure
 *  \e slotInfo is filled in by the user.
 *  \param slotInfo The structure which will get filled by this API and will
 *  contain other information about node corresponding to the field which was 
 *  filled by the user.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER On passing the invalid flag or passing 
 *  invalid value in any of the fields.
 * 
 *  \par Description:
 *  This API provides mapping between the attributes which are unique to node
 *  namely, slot ID, IOC address and node name. Depending on the flag
 *  which is given by the user, this API assumes that the corresponding field
 *  has been given a proper value by the user and fills the remaining fields 
 *  of the structure corresponding to the information in this field. For example
 *  suppose the user wants to know the nodename given the slot ID, then he should
 *  pass the flag CL_CPM_SLOT_NODENAME (which are exposed in clCpmApi.h) and set the
 *  \e slotId field of \e slotInfo structure to some valid slot number.
 * 
 *  \par Library Files:
 *  libClAmfClient 
 * 
 */

extern ClRcT clCpmSlotGet(CL_IN ClCpmSlotInfoFieldIdT flag,
                          CL_OUT ClCpmSlotInfoT *slotInfo);


/**
 ************************************
 *  \brief Returns the IOC address for the given node.
 * 
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param nodeName The name of the node whose IocAddress is 
 *  required 
 *  \param pIocAddress IOC address of the given node \e nodeName.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_DOESNT_EXIST CPM library is not able to retrieve the 
 *  nodeName.
 * 
 *  \par Description:
 *  This API returns the physical IOC address of the node which is passed
 *  as an argument.
 * 
 *  \par Library Files:
 *  libClAmfClient 
 * 
 */
extern ClRcT clCpmIocAddressForNodeGet(CL_IN ClNameT nodeName, 
                                       CL_OUT ClIocAddressT *pIocAddress);

/**
 ************************************
 *  \brief Checks if the given component has been restarted.
 * 
 *  \par Header File:
 *  clCpmExtApi.h
 *
 *  \param compName The name of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_DOESNT_EXIST CPM library is not able to retrieve the 
 *  component name.
 * 
 *  \par Description:
 *  This API returns true if the component has been restarted before.
 * 
 *  \par Library Files:
 *  libClAmfClient 
 * 
 */
extern ClBoolT clCpmIsCompRestarted(CL_IN ClNameT compName);

extern ClRcT clCpmNodeConfigSet(ClCpmNodeConfigT *nodeConfig);

extern ClRcT clCpmNodeConfigGet(const ClCharT *nodeName, ClCpmNodeConfigT *nodeConfig);

extern ClRcT
clCpmComponentFailureReportWithCookie(CL_IN ClCpmHandleT cpmHandle,
                                      CL_IN const ClNameT *pCompName,
                                      CL_IN ClUint64T instantiateCookie,
                                      CL_IN ClTimeT errorDetectionTime,
                                      CL_IN ClAmsLocalRecoveryT recommendedRecovery,
                                      CL_IN ClUint32T alarmHandle);

extern ClRcT 
clCpmTargetSlotInfoGet(const ClCharT *name, ClIocNodeAddressT addr, ClTargetSlotInfoT *slotInfo);

extern ClRcT 
clCpmTargetInfoGet(ClTargetInfoT *targetInfo);

extern ClRcT
clCpmTargetSlotListGet(ClTargetSlotInfoT *slotInfo, ClUint32T *numSlots);

extern ClRcT
clCpmTargetVersionGet(ClCharT *aspVersion, ClUint32T maxBytes);

extern ClBoolT 
clCpmIsSCCapable(void);

#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CPM_EXT_API_H_*/


/** \} */
