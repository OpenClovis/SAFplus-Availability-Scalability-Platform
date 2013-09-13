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
 * ModuleName  : amf
 * File        : clAmsEventServerApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS event API.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_EVENT_SERVER_API_H_
#define _CL_AMS_EVENT_SERVER_API_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <string.h>

#include <clCommon.h>
#include <clAms.h>
//#include <clAmsEventClientApi.h>
#include <clEoApi.h>
#include <clFaultDefinitions.h>

/******************************************************************************
 * Data types
 *****************************************************************************/

// XXX: This is a temporary place holder and likely to change

typedef struct
{
    ClUint8T                    category;
    ClUint8T                    subCategory;
    ClUint8T                    severity;
    ClUint16T                   cause;
    ClAmsLocalRecoveryT         recovery;
    ClUint32T                   escalation;
} ClAmsFaultDescriptorT;

/******************************************************************************
 * Event API functions
 *****************************************************************************/

/*
 * clAmsEventEntityPublishFault
 * ----------------------------
 * This is the basic function provided by AMS to get event input regarding
 * a fault in the system. 
 *
 * It is unlikely that this function will be called by other ASP entities 
 * directly. Wrapper functions will be written to translate arguments 
 * passed by other ASP entities to FM.
 *
 * @param
 *   entity                     - The AMS entity against which a fault is
 *                                being published. Typically, the entity
 *                                will be a node, su, component.
 *   faultDescriptor            - The details of the fault, recovery action
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEventPublishFault(
        CL_IN       const ClAmsEntityT                    *entity,
        CL_IN       const ClAmsFaultDescriptorT           *faultDescriptor);

/*
 * TODO
 *
 * Write a wrapper function that has the same prototype as the function
 * that alarm uses to inform fault manager of a fault. Then this function
 * can translate the arguments into something understood by AMS and 
 * internally call clAmsEventPublishFault.
 *
 * This function will take as input:
 *   source component
 *   fault type, severity
 *   recommended recovery
 *
 * This function provide as output:
 *   recommended recovery
 *   escalation
 */
 
/*
 * clAmsEventEntityPresence
 * ------------------------
 * Inform AMS of entity presence. 
 *
 * @param
 *   entity                     - The AMS entity against which a presence
 *                                or absence information is being reported.
 *                                Typically, the entity will be a node.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEventNodePresence(
        CL_IN       ClAmsEntityT            *entity,
        CL_IN       ClBoolT                 presence);

/******************************************************************************
 * Internal Event functions to enable/disable event handling by AMS
 *****************************************************************************/

extern ClRcT clAmsEventApiInstantiate(
        CL_IN ClAmsT *ams);


extern ClRcT clAmsEventApiTerminate(
        CL_IN ClAmsT *ams);

/******************************************************************************
 * Functions for handling client side requests
 *****************************************************************************/

extern ClRcT
_clAmsEventInitialize( 
        ClBufferHandleT in_buffer,
        ClBufferHandleT out_buffer);

extern ClRcT
_clAmsEventFinalize( 
        ClBufferHandleT in_buffer,
        ClBufferHandleT out_buffer);

extern ClRcT
_clAmsEventEntityFault(
        ClBufferHandleT in_buffer,
        ClBufferHandleT out_buffer);

extern ClRcT
_clAmsEventEntityJoin(
        ClBufferHandleT in_buffer,
        ClBufferHandleT out_buffer);

extern ClRcT
_clAmsEventEntityLeave(
        ClBufferHandleT in_buffer,
        ClBufferHandleT out_buffer);

/******************************************************************************
 * Server side fns that do actual work
 *****************************************************************************/

extern ClRcT clAmsEventEntityJoin2(
        CL_IN   ClAmsT *ams,
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type);

extern ClRcT clAmsEventEntityHasLeft2(
        CL_IN   ClAmsT *ams,
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type);

extern ClRcT clAmsEventEntityIsLeaving2(
        CL_IN   ClAmsT *ams,
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type);

extern ClRcT clAmsEventEntityFault2(
        CL_IN   ClAmsT *ams,
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type,
        CL_IN   ClAmsFaultDescriptorT *fault);

extern ClRcT
unmarshalFaultReport (
        CL_IN       ClBufferHandleT  in,
        CL_IN       ClFaultEventT           **faultEvent );

extern ClRcT
clAmsGetFaultReport(
        const SaNameT     *compName,
        ClAmsLocalRecoveryT recommendedRecovery,
        ClUint64T instantiateCookie);

/******************************************************************************
 * Event API functions
 *****************************************************************************/

/*
 * clAmsEventEntityJoin
 * --------------------
 * Inform AMS of entity presence. 
 */

extern ClRcT clAmsEventEntityJoin(
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type);

/*
 * clAmsEventEntityLeave
 * ---------------------
 * Inform AMS of entity presence. 
 */

extern ClRcT clAmsEventEntityLeave(
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type);

/*
 * clAmsEventEntityFault
 * ---------------------
 * This is the basic function provided by AMS to get event input regarding
 * a fault in the system. 
 *
 * It is unlikely that this function will be called by other ASP entities 
 * directly. Wrapper functions will be written to translate arguments 
 * passed by other ASP entities to FM.
 */

extern ClRcT clAmsEventEntityFault(
        CL_IN   SaNameT *name,
        CL_IN   ClAmsEntityTypeT type,
        CL_IN   ClAmsFaultDescriptorT *fault);

/*
 * clAmsEventEntityFaultReport
 * ---------------------------
 * Alarm Manager uses the prototype below to report a fault to the fault
 * service, receiver can be FM or AMS.
 *
 * This fn translates the MOID on which the fault is reported to component
 * name and calls clAmsEventEntityFault.
 */


/*
extern ClRcT clAmsEventEntityFaultReport2(
        CL_IN   ClCorMOIdPtrT   hMoId,
        CL_IN   ClUint8T        category, 
        CL_IN   ClUint8T        subCategory, 
        CL_IN   ClUint8T        severity, 
        CL_IN   ClUint16T       cause, 
        CL_IN   void            *pData, 
        CL_IN   ClUint32T       len);
*/

# ifdef __cplusplus
}
# endif

#endif  /* _CL_AMS_EVENT_SERVER_API_H_ */
