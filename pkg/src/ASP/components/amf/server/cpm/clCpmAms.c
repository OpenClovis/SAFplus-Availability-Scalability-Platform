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
 * This file implements CPM-AMS interaction.
 */

/*
 * Standard header files 
 */
#include <string.h>

/*
 * ASP header files 
 */
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clCmIpi.h>
#include <clCmApi.h>
#include <clIocErrors.h>
#include <clIocIpi.h>
/*
 * CPM internal header files 
 */
#include <clCpmCommon.h>
#include <clCpmLog.h>
#include <clCpmInternal.h>
#include <clCpmAms.h>
#include <clCpmClient.h>
#include <clAmsInvocation.h>
#include <clCpmMgmt.h>

/*
 * XDR header files 
 */
#include "xdrClCpmCompCSISetT.h"
#include "xdrClCpmCompCSIRmvT.h"
#include "xdrClCpmResponseT.h"
#include "xdrClCpmPGTrackT.h"
#include "xdrClCpmPGResponseT.h"
#include "xdrClCpmPGTrackStopT.h"
#include "xdrClCpmHAStateGetSendT.h"
#include "xdrClCpmHAStateGetRecvT.h"
#include "xdrClCpmQuiescingCompleteT.h"
#include "xdrClAmsCSIStateDescriptorT.h"
#include "xdrClCpmCSIDescriptorNameValueT.h"
#include "xdrClAmsPGNotificationT.h"

ClRcT VDECL(cpmComponentHAStateGet)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmHAStateGetSendT requestBuff;
    ClCpmHAStateGetRecvT replyBuff;

    rc = VDECL_VER(clXdrUnmarshallClCpmHAStateGetSendT, 4, 0, 0)(inMsgHandle,
                                             (void *) &requestBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"), rc);

    if (CL_CPM_IS_ACTIVE())
    {
        if (gpClCpm->cpmToAmsCallback != NULL &&
            gpClCpm->cpmToAmsCallback->compHAStateGet != NULL)
        {
            gpClCpm->cpmToAmsCallback->compHAStateGet(&requestBuff.compName,
                                                      &requestBuff.csiName,
                                                      &replyBuff.haState);
            rc = VDECL_VER(clXdrMarshallClCpmHAStateGetRecvT, 4, 0, 0)((void *) &replyBuff,
                                                   outMsgHandle, 0);
        }
        else
            rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);

failure:
    return rc;
}

                                   // coverity[pass_by_value]
static ClRcT _cpmCsiDescriptorPack(ClAmsCSIDescriptorT csiDescriptor,
                                   ClAmsHAStateT haState,
                                   ClUint8T **buffer,
                                   ClUint32T *bufferLength)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT message;
    ClUint32T msgLength = 0;
    ClUint32T numAttr = 0;
    ClCpmCSIDescriptorNameValueT attrType = 0;
    ClAmsCSIAttributeListT *attrList = NULL;
    ClUint8T *attributeName = NULL;
    ClUint8T *attributeValue = NULL;
    ClUint8T *tmpStr = NULL;
    ClUint32T attributeNameLength = 0;
    ClUint32T attributeValueLength = 0;

    rc = clBufferCreate(&message);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to create message \n"), rc);

    rc = clXdrMarshallClUint32T((void *) &(csiDescriptor.csiFlags), message, 0);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

    if (CL_AMS_CSI_FLAG_TARGET_ALL != csiDescriptor.csiFlags)
    {
        rc = clXdrMarshallClNameT((void *) &(csiDescriptor.csiName), message, 0);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
    }

    if (CL_AMS_HA_STATE_ACTIVE == haState)
    {
        rc = VDECL_VER(clXdrMarshallClAmsCSIActiveDescriptorT, 4, 0, 0)((void *)
                                                    &(csiDescriptor.
                                                      csiStateDescriptor.
                                                      activeDescriptor),
                                                    message,
                                                    0);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
    }
    else if (CL_AMS_HA_STATE_STANDBY == haState)
    {
        rc = VDECL_VER(clXdrMarshallClAmsCSIStandbyDescriptorT, 4, 0, 0)((void *)
                                                     &(csiDescriptor.
                                                       csiStateDescriptor.
                                                       standbyDescriptor),
                                                     message,
                                                     0);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
    }
    
    if (CL_AMS_CSI_FLAG_ADD_ONE == csiDescriptor.csiFlags)
    {
        attrList = &(csiDescriptor.csiAttributeList);

        /*
         * Here, we need to pack the attribute List 
         */
        rc = clXdrMarshallClUint32T((void *) &(attrList->numAttributes), message,
                                    0);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

        while (numAttr != csiDescriptor.csiAttributeList.numAttributes)
        {
            attributeName = (attrList->attribute[numAttr]).attributeName;
            attributeNameLength = strlen((ClCharT *) attributeName) + 1;

            attributeValue = (attrList->attribute[numAttr]).attributeValue;
            attributeValueLength = strlen((ClCharT *) attributeValue) + 1;

            /*
             * Atribute Name Packing: Type | Length | Value 
             */
            attrType = CL_CPM_CSI_ATTR_NAME;
            rc = VDECL_VER(clXdrMarshallClCpmCSIDescriptorNameValueT, 4, 0, 0)((void *) &(attrType),
                                                           message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

            rc = clXdrMarshallClUint32T((void *) &attributeNameLength, message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

            rc = clXdrMarshallArrayClCharT((void *) attributeName,
                                           attributeNameLength, message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

            /*
             * Atribute value Packing: Type | Length | Value 
             */
            attrType = CL_CPM_CSI_ATTR_VALUE;
            rc = VDECL_VER(clXdrMarshallClCpmCSIDescriptorNameValueT, 4, 0, 0)((void *) &(attrType),
                                                           message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

            rc = clXdrMarshallClUint32T((void *) &attributeValueLength, message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

            rc = clXdrMarshallArrayClCharT((void *) attributeValue,
                                           attributeValueLength, message, 0);
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);
        
            numAttr++;
        }
    }

    rc = clBufferLengthGet(message, &msgLength);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get length of the message \n"),
                 rc);

    rc = clBufferFlatten(message, &tmpStr);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to flatten the message \n"), rc);

    *buffer = tmpStr;
    *bufferLength = msgLength;

  failure:
    clBufferDelete(&message);
    return rc;
}

/*
 * This function will be called by AMS, which is in the local process context 
 * of CPM as a direct function call. Dependening on the location of the 
 * component CPM needs to direct the appropriate the CPM\L. If the nodeName 
 * is same as CPM\G local node, it will directly talk to the component by 
 * invoking appropriate call in to the component
 *
 */
ClRcT _cpmComponentCSISet(ClCharT *targetComponentName,
                          ClCharT *targetProxyComponentName,
                          ClCharT *targetNodeName,
                          ClInvocationT invocation,
                          ClAmsHAStateT haState,
                          // coverity[pass_by_value]
                          ClAmsCSIDescriptorT csiDescriptor)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSISetT *sendBuff = NULL;
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;
    ClCpmLT *cpmL = NULL;
    ClNameT compName={0};
    ClNameT proxyCompName={0};
    ClNameT nodeName={0};
    ClUint8T *buffer = NULL;
    ClUint32T bufferLength = 0;

    /*
     * Input param check 
     */
    if (targetComponentName == NULL || targetProxyComponentName == NULL || targetNodeName == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Null pointer passed"),
                     CL_CPM_RC(CL_ERR_NULL_POINTER));

    strcpy(compName.value, targetComponentName);
    compName.length = strlen(targetComponentName);
    strcpy(proxyCompName.value, targetProxyComponentName);
    proxyCompName.length = strlen(targetProxyComponentName);
    strcpy(nodeName.value, targetNodeName);
    nodeName.length = strlen(targetNodeName);

    /*
     * pack the ClAmsCSIDescriptorT 
     */
    rc = _cpmCsiDescriptorPack(csiDescriptor, haState, &buffer, &bufferLength);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("unable to pack the csiDescriptor %x\n", rc),
                 rc);

    /*
     * Allocate the memory for the send buffer 
     */
    sendBuff = (ClCpmCompCSISetT *) clHeapCalloc(1, sizeof(*sendBuff));
    if (sendBuff == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                     CL_CPM_RC(CL_ERR_NO_MEMORY));

    /*
     * Populate the send buffer appropriatly 
     */
    memcpy(&(sendBuff->compName), &(compName), sizeof(ClNameT));
    memcpy(&(sendBuff->proxyCompName), &(proxyCompName), sizeof(ClNameT));
    memcpy(&(sendBuff->nodeName), &(nodeName), sizeof(ClNameT));
    sendBuff->invocation = invocation;
    sendBuff->haState = haState;
    sendBuff->bufferLength = bufferLength;
    sendBuff->buffer = buffer;
    
    /*
     * If the local Node name and the passed node name matches 
     */
    if (!strcmp(gpClCpm->pCpmConfig->nodeName, nodeName.value))
    {
        /*
         * Find the component Name from the component Hash Table 
         */
        rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);

        /*
         * Could be a proxied on a different node. 
         */
        if(rc != CL_OK 
           && 
           cpmComponentAddDynamic(compName.value) == CL_OK)
        {
            rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
        }

        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find component %s \n", compName.value), rc);

        /* PROXIED: Added Switch-Case
         * Check for component property. For proxied components CSI set request
         * is sent to the managing proxy component
         */
        switch(comp->compConfig->compProperty)
        {
            case CL_AMS_COMP_PROPERTY_SA_AWARE:
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(), comp->eoPort,
                        CPM_CSI_SET_FN_ID, (ClUint8T *) sendBuff,
                        (ClUint32T)sizeof(*sendBuff), NULL, NULL,
                        CL_RMD_CALL_ATMOST_ONCE, 0, 0, 0, NULL,
                        NULL, MARSHALL_FN(ClCpmCompCSISetT, 4, 0, 0)
                        );
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
            case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
            {
                rc = cpmCompFind(proxyCompName.value, gpClCpm->compTable, &proxyComp);
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                        ("Unable to find component %s \n", proxyCompName.value), rc);

                /* Make an RMD to the proxyComp. */
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                               proxyComp->eoPort,
                                               CPM_CSI_SET_FN_ID,
                                               (ClUint8T *) sendBuff,
                                               (ClUint32T)sizeof(*sendBuff),
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0,
                                               0,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmCompCSISetT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);

                /*  FIXME: added for timebeing to plugin in existing code.
                 *  clean it up- states for App comps not be maintained by CPM */   
                if (comp->compConfig->compProperty == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
                {
                    comp->compPresenceState = CL_AMS_PRESENCE_STATE_INSTANTIATED;
                }
                break;
            }
            case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
            default:
            {
                /* PROXIED: */
                rc = CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);
                break;
            }
        }
    }
    /*
     * Else forward it to the right cpm\L 
     */
    else if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
    {
        rc = cpmNodeFind(nodeName.value, &cpmL);
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find corresponding Node %s \n",
                      nodeName.value), rc);
        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            rc = CL_CPM_CALL_RMD_ASYNC_NEW(cpmL->pCpmLocalInfo->cpmAddress.
                                           nodeAddress,
                                           cpmL->pCpmLocalInfo->cpmAddress.
                                           portId,
                                           CPM_COMPONENT_CSI_ASSIGN,
                                           (ClUint8T *) sendBuff,
                                           (ClUint32T)sizeof(*sendBuff),
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           0,
                                           0,
                                           NULL,
                                           NULL,
                                           MARSHALL_FN(ClCpmCompCSISetT, 4, 0, 0));
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to set the CSI for component %s on node %s rc=%x\n ",
                          compName.value, nodeName.value, rc), rc);
        }
        else
        {
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Node is down so unable to serve the request %s \n",
                          nodeName.value),
                         CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED));
        }
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);

  failure:
    if (sendBuff != NULL)
    {
        clHeapFree(sendBuff);
        sendBuff = NULL;
    }
    if (buffer != NULL)
    {
        clHeapFree(buffer);
        buffer = NULL;
    }

    return rc;
}

ClRcT VDECL(cpmComponentCSISet)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSISetT *recvBuff = NULL;
    ClUint32T msgLength = 0;
    ClCpmComponentT *comp = NULL;
    /* PROXIED: Added proxyComp */
    ClCpmComponentT *proxyComp = NULL;

    rc = clBufferLengthGet(inMsgHandle, &msgLength);

    if (msgLength >= sizeof(ClCpmCompCSISetT))
    {
        recvBuff = (ClCpmCompCSISetT *) clHeapCalloc(1, sizeof(*recvBuff));
        if (recvBuff == NULL)
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
        rc = VDECL_VER(clXdrUnmarshallClCpmCompCSISetT, 4, 0, 0)(inMsgHandle, (void *)recvBuff);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
    }
    else
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                     CL_CPM_RC(CL_ERR_INVALID_BUFFER));

    /*
     *  When we are in this function, then it is for the component of the local 
     *  node, make RMD to the component which will in turn invoke the callback. 
     */

    /* PROXIED: Added Switch-Case
     * Check for component property. For proxied components CSI set request
     * is sent to the managing proxy component.
     */
    if (!strcmp(gpClCpm->pCpmConfig->nodeName, recvBuff->nodeName.value))
    {
        /*
         * Find the component Name from the component Hash Table 
         */
        rc = cpmCompFind(recvBuff->compName.value, gpClCpm->compTable, &comp);
        if(rc != CL_OK 
           &&
           cpmComponentAddDynamic(recvBuff->compName.value) == CL_OK)
        {
            rc = cpmCompFind(recvBuff->compName.value, gpClCpm->compTable, &comp);
        }
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find component %s \n",
                      recvBuff->compName.value), rc);
        switch(comp->compConfig->compProperty)
        {
            case CL_AMS_COMP_PROPERTY_SA_AWARE:
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                               comp->eoPort,
                                               CPM_CSI_SET_FN_ID,
                                               (ClUint8T *) recvBuff,
                                               (ClUint32T)sizeof(*recvBuff),
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0,
                                               0,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmCompCSISetT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
            case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
            {
                rc = cpmCompFind(recvBuff->proxyCompName.value, gpClCpm->compTable, &proxyComp);
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                        ("Unable to find component %s \n", recvBuff->proxyCompName.value), rc);

                /* Make an RMD to the proxyComp. */
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                               proxyComp->eoPort,
                                               CPM_CSI_SET_FN_ID,
                                               (ClUint8T *) recvBuff,
                                               (ClUint32T)sizeof(*recvBuff),
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0,
                                               0,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmCompCSISetT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
            default:
            {
                /* PROXIED: */
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
                break;
            }
        }
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);

  failure:
    if (recvBuff != NULL)
    {
        if(recvBuff->buffer)
        {
            clHeapFree(recvBuff->buffer);
        }
        clHeapFree(recvBuff);
        recvBuff = NULL;
    }
    return rc;
}

ClRcT _cpmComponentCSIRmv(ClCharT *targetComponentName,
                          ClCharT *targetProxyComponentName,
                          ClCharT *targetNodeName,
                          ClInvocationT invocation,
                          ClNameT *csiName,
                          ClAmsCSIFlagsT csiFlags)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSIRmvT sendBuff={0};
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;
    ClCpmLT *cpmL = NULL;
    ClNameT compName={0};
    ClNameT proxyCompName={0};
    ClNameT nodeName={0};

    /*
     * Check/validate the input param check 
     */
    if ((targetComponentName == NULL) ||
        (targetProxyComponentName == NULL) ||
        (targetNodeName == NULL))
    {
        rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                   "NULL pointer passed");
        goto failure;
    }

    if ((csiName == NULL) && (csiFlags != CL_AMS_CSI_FLAG_TARGET_ALL))
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                   "CSI name is NULL and csi flag is not target all");
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    strcpy(compName.value, targetComponentName);
    compName.length = strlen(targetComponentName);
    strcpy(proxyCompName.value, targetProxyComponentName);
    proxyCompName.length = strlen(targetProxyComponentName);
    strcpy(nodeName.value, targetNodeName);
    nodeName.length = strlen(targetNodeName);

    /*
     * Populate the send buffer appropriatly 
     */
    memcpy(&(sendBuff.compName), &(compName), sizeof(ClNameT));
    memcpy(&(sendBuff.proxyCompName), &(proxyCompName), sizeof(ClNameT));
    memcpy(&(sendBuff.nodeName), &(nodeName), sizeof(ClNameT));
    if (csiName != NULL)
    {
        memcpy(&(sendBuff.csiName), csiName, sizeof(ClNameT));
    }

    sendBuff.invocation = invocation;
    sendBuff.csiFlags = csiFlags;

    /*
     * If the local Node name and the passed node name matches 
     */
    if (!strcmp(gpClCpm->pCpmConfig->nodeName, nodeName.value))
    {
        /*
         * Find the component Name from the component Hash Table 
         */
        rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find component %s \n", compName.value), rc);

        switch(comp->compConfig->compProperty)
        {
            case CL_AMS_COMP_PROPERTY_SA_AWARE:
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(), 
                                               comp->eoPort,
                                               CPM_CSI_RMV_FN_ID, 
                                               (ClUint8T *) &sendBuff, 
                                               sizeof(ClCpmCompCSIRmvT), 
                                               NULL,
                                               NULL, 
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0, 
                                               0,
                                               NULL,
                                               NULL, 
                                               MARSHALL_FN(ClCpmCompCSIRmvT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
            case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
            {
                rc = cpmCompFind(proxyCompName.value, gpClCpm->compTable, &proxyComp);
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                        ("Unable to find proxy component %s \n", proxyCompName.value), rc);

                /* Make an RMD to the proxyComp. */
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(), 
                                               proxyComp->eoPort,
                                               CPM_CSI_RMV_FN_ID, 
                                               (ClUint8T *) &sendBuff, 
                                               sizeof(ClCpmCompCSIRmvT), 
                                               NULL,
                                               NULL, 
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0, 
                                               0,
                                               NULL,
                                               NULL, 
                                               MARSHALL_FN(ClCpmCompCSIRmvT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                /*  FIXME: added for timebeing to plugin in existing code.
                 *  clean it up- states for App comps not be maintained by CPM */   
                if (comp->compConfig->compProperty == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
                {
                    comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
                }
                break;
            }
            case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
            default:
            {
                /* PROXIED: Not allowed for npnp */
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
                break;
            }
        }
    }
    /*
     * Else forward it to the right cpm\L 
     */
    else if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
    {
        rc = cpmNodeFind(nodeName.value, &cpmL);
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find corresponding Node %s \n",
                      nodeName.value), rc);
        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            rc = CL_CPM_CALL_RMD_ASYNC_NEW(cpmL->pCpmLocalInfo->cpmAddress.
                                           nodeAddress,
                                           cpmL->pCpmLocalInfo->cpmAddress.
                                           portId,
                                           CPM_COMPONENT_CSI_RMV,
                                           (ClUint8T *) &sendBuff,
                                           sizeof(ClCpmCompCSIRmvT),
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           0,
                                           0,
                                           NULL,
                                           NULL,
                                           MARSHALL_FN(ClCpmCompCSIRmvT, 4, 0, 0));
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to Rmv the CSI for component %s on node %s rc=%x\n ",
                          compName.value, nodeName.value, rc), rc);
        }
        else
        {
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Node is down so unable to serve the request %s\n",
                          nodeName.value),
                         CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED));
        }
    }

    return rc;

  failure:
    return rc;
}

ClRcT _cpmComponentPGTrack(CL_IN ClIocAddressT iocAddress,
                           CL_IN ClCpmHandleT cpmHandle,
                           // coverity[pass_by_value]
                           CL_IN ClNameT     csiName,
                           CL_IN ClAmsPGNotificationBufferT *notificationBuffer,
                           CL_IN ClUint32T   numberOfMembers,
                           CL_IN ClUint32T   error
)
{
    ClRcT   rc = CL_OK;
    ClCpmPGResponseT *sendBuff = NULL;
    ClUint32T   msgLength = 0;
    ClBufferHandleT message = 0;
    ClUint8T *tmpStr = NULL;
    ClUint32T i = 0;
    ClUint32T totalLength = 0;
    

    rc = clBufferCreate(&message);
    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to create message \n"), rc);

    rc = clXdrMarshallClUint32T((void *) &(notificationBuffer->numItems), 
            message, 0);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

    
    for (i = 0; i < notificationBuffer->numItems; ++i) 
    {
        rc = VDECL_VER(clXdrMarshallClAmsPGNotificationT, 4, 0, 0)(
                (void *)&(notificationBuffer->notification[i]), message, 0);
        CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);

    }

    rc = clBufferLengthGet(message, &msgLength);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get length of the message \n"),
                 rc);

    totalLength = sizeof(ClCpmPGResponseT) + msgLength;
    sendBuff = (ClCpmPGResponseT *) clHeapAllocate(totalLength);
    if (sendBuff == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                     CL_CPM_RC(CL_ERR_NO_MEMORY));
    
    
    sendBuff->cpmHandle = cpmHandle;
    memcpy(&(sendBuff->csiName), &csiName, sizeof(ClNameT));
    sendBuff->numberOfMembers = numberOfMembers;
    sendBuff->error = error;

    rc = clBufferFlatten(message, &tmpStr);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to flatten the message \n"), rc);

    sendBuff->buffer = tmpStr;
    sendBuff->bufferLength = msgLength;

    rc = CL_CPM_CALL_RMD_ASYNC_NEW(iocAddress.iocPhyAddress.nodeAddress,
                                   iocAddress.iocPhyAddress.portId,
                                   CPM_PROTECTION_GROUP_TRACK_FN_ID,
                                   (ClUint8T *) sendBuff,
                                   msgLength,
                                   NULL,
                                   NULL,
                                   0,
                                   0,
                                   0,
                                   0,
                                   NULL,
                                   NULL,
                                   MARSHALL_FN(ClCpmPGResponseT, 4, 0, 0));

failure:
    if(sendBuff != NULL)
    {
        if(sendBuff->buffer != NULL)
            clHeapFree(sendBuff->buffer);
        clHeapFree(sendBuff);
    }
    clBufferDelete(&message);
    return rc;
}

ClRcT _cpmNodeDepartureAllowed(ClNameT *nodeName,
                               ClCpmNodeLeaveT nodeLeave
                               )
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL = NULL;
    ClCmCpmMsgT cmRequest;
    ClCpmCmMsgT cpmResponse;

    memset(&cmRequest, 0, sizeof(ClCmCpmMsgT));
    memset(&cpmResponse, 0, sizeof(ClCpmCmMsgT));

    rc = cpmDequeueCmRequest(nodeName, &cmRequest);
    if (rc == CL_OK)
    {
        clLogMultiline(CL_LOG_NOTICE, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                       "After dequeueing the message from CM queue:\n"
                       "Message type : [%s]\n"
                       "Physical slot : [%d]\n"
                       "Sub slot : [%d]\n"
                       "Resource Id : [%d]\n",
                       (cmRequest.cmCpmMsgType == CL_CM_BLADE_SURPRISE_EXTRACTION) ?
                       "surprise extraction" :
                       (cmRequest.cmCpmMsgType == CL_CM_BLADE_REQ_INSERTION) ?
                       "blade insertion" :
                       (cmRequest.cmCpmMsgType == CL_CM_BLADE_REQ_EXTRACTION) ?
                       "blade extraction" :
                       (cmRequest.cmCpmMsgType == CL_CM_BLADE_NODE_ERROR_REPORT) ?
                       "node error report":
                       (cmRequest.cmCpmMsgType == CL_CM_BLADE_NODE_ERROR_CLEAR) ?
                       "node error clear" : "invalid message",
                       cmRequest.physicalSlot,
                       cmRequest.subSlot,
                       cmRequest.resourceId);

        memcpy(&(cpmResponse.cmCpmMsg), &cmRequest, sizeof(ClCmCpmMsgT));
        cpmResponse.cpmCmMsgType = CL_CPM_ALLOW_USER_ACTION;
        
        if ((cmRequest.cmCpmMsgType != CL_CM_BLADE_NODE_ERROR_REPORT) 
            &&
            (cmRequest.cmCpmMsgType != CL_CM_BLADE_NODE_ERROR_CLEAR))
        {
            rc = clCmCpmResponseHandle(&cpmResponse);
            if (CL_OK != rc)
            {
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                           "Responding to CM failed, error [%#x]",
                           rc);
            }
        }
    }
    else
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                     "Dequeueing CM message failed, error [%#x]",
                     rc);
    }
    
    if (!strcmp(nodeName->value, gpClCpm->pCpmLocalInfo->nodeName) && 
        CL_CPM_IS_ACTIVE())
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                   "CPM/G active got termination request for itself...");
        
        if (cmRequest.cmCpmMsgType != CL_CM_BLADE_NODE_ERROR_REPORT)
        {
            cpmActiveInitiatedSwitchOver();
            cpmSelfShutDown();
        }
    }
    else
    {
        rc = cpmNodeFind(nodeName->value, &cpmL);
        if (rc == CL_OK)
        {
            if (cmRequest.cmCpmMsgType != CL_CM_BLADE_NODE_ERROR_REPORT)
            {
                if (cpmL->pCpmLocalInfo != NULL 
                    && 
                    nodeLeave == CL_CPM_NODE_LEAVING)
                {
                    clCpmClientRMDAsyncNew(cpmL->pCpmLocalInfo->
                                           cpmAddress.nodeAddress,
                                           CPM_NODE_SHUTDOWN, 
                                           (ClUint8T *)&(cpmL->pCpmLocalInfo->
                                                         cpmAddress.nodeAddress),
                                           1,
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           0,
                                           CL_IOC_HIGH_PRIORITY,
                                           clXdrMarshallClUint32T);
                }
                else
                {
                    if(cpmL->pCpmLocalInfo != NULL)
                    {
                        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                     "Node shutdown called for already left "
                                     "node [%s], ignoring the request and "
                                     "continuing...",
                                 nodeName->value);
                    }
                    else
                    {
                        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                     "Node shutdown called for unregistered "
                                     "node [%s], ignoring the request and "
                                     "continuing...",
                                     nodeName->value);
                        
                    }
                }
            }
        }
    }

    return CL_OK;
}

static __inline__ void cpmRebootNode(void)
{
    const ClCharT *pPersonality = "controller";
    if(CL_CPM_IS_WB()) pPersonality = "payload";
    cpmReset(NULL, pPersonality);
    /*unreached*/
}

void cpmResetNodeElseCommitSuicide(ClUint32T restartFlag)
{
    ClBoolT watchdogRestart = CL_TRUE;
    /*
     * Treat restart ASP opcode as a restart NODE as its a switchover
     * with autorepair which should reboot the node - SAF
     */
    if(restartFlag == CL_CPM_RESTART_ASP)
        restartFlag = CL_CPM_RESTART_NODE;

    if(restartFlag == CL_CPM_RESTART_NODE
       && clParseEnvBoolean("ASP_NODE_REBOOT_DISABLE") == CL_TRUE)
    {
        /*
         * We also disable watchdog restarts here
         */
        watchdogRestart = CL_FALSE;
        restartFlag = CL_CPM_RESTART_NONE;
    }

    /*
     * ASP node restart is defined, than only restart ASP.  
     */
    if(clParseEnvBoolean("ASP_NODE_RESTART") == CL_TRUE)
        restartFlag = CL_CPM_RESTART_ASP;

    /*
     * Overrides everything else above.
     */
    if(restartFlag == CL_CPM_HALT_ASP)
    {
        watchdogRestart = CL_FALSE;
    }

    /*
     * To avoid processing previous suicide request sent for this
     * node, when this node is registering.
     */

    if (gpClCpm->bmTable->currentBootLevel ==
        gpClCpm->pCpmLocalInfo->defaultBootLevel)
    {

        switch(restartFlag)
        {
        case CL_CPM_RESTART_ASP:
            {
                ClTimerTimeOutT delay = {.tsSec = 5, .tsMilliSec = 0 };
                const ClCharT *personality = "controller";
                if(CL_CPM_IS_WB())
                    personality = "payload";
                clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                              "This node would be restarted in [%d] secs",
                              delay.tsSec);
                cpmRestart(&delay, personality);
            }
            break;
        case CL_CPM_RESTART_NODE:
            {
                cpmRebootNode();
            }
            break;
        case CL_CPM_RESTART_NONE:
        case CL_CPM_HALT_ASP:
            if(!watchdogRestart)
            {
                /*
                 * Tell the watchdog to disable ASP restarts.
                 */
                FILE *fptr = fopen(CL_CPM_RESTART_DISABLE_FILE, "w");
                if(fptr) fclose(fptr);
            }
            /*
             * fall through
             */
        default:
            {
                cpmCommitSuicide();
            }
            break;
        }
    }
}

ClRcT VDECL(cpmResetNodeElseCommitSuicideRmd)(ClEoDataT data,
                                              ClBufferHandleT inMsgHandle,
                                              ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T restartFlag = 0;

    rc = clXdrUnmarshallClUint32T(inMsgHandle, &restartFlag);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                   "Unable to unmarshall data, error [%#x]", rc);
        goto failure;
    }

    cpmResetNodeElseCommitSuicide(restartFlag);
    
    return CL_OK;
    
failure:
    return rc;
}

/*  FAILFAST
 *  Does the failfast of the given node. ASP components running on the
 *  node are not terminated and graceful shut down does not happen.
 *  AMS is already aware of Node's departure when this function is
 *  called. The node is also restarted if its configured to be restartable
 */

static ClRcT _cpmNodeFailFastRestart(ClNameT *nodeName, ClUint32T restartFlag)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL = NULL;
        
    if (CL_CPM_IS_ACTIVE())
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                      "Node failfast/failover called for node [%*s]...",
                      nodeName->length, nodeName->value);

        if (!strcmp(nodeName->value,
                    gpClCpm->pCpmLocalInfo->nodeName))
        {
            cpmResetNodeElseCommitSuicide(restartFlag);
        }
        else
        {
            rc = cpmNodeFind(nodeName->value, &cpmL);
            if (CL_OK != rc)
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                           "Unable to find node [%s], error [%#x]",
                           nodeName->value,
                           rc);
                goto failure;
            }
            
            if (cpmL->pCpmLocalInfo)
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(cpmL->pCpmLocalInfo->
                                               cpmAddress.nodeAddress,
                                               cpmL->pCpmLocalInfo->
                                               cpmAddress.portId,
                                               CPM_RESET_ELSE_COMMIT_SUICIDE,
                                               (ClUint8T *) &restartFlag,
                                               sizeof(restartFlag),
                                               NULL,
                                               NULL,
                                               0,
                                               0,
                                               0,
                                               0,
                                               NULL,
                                               NULL,
                                               clXdrMarshallClUint32T);
                if (CL_OK != rc)
                {
                    if ((CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) ||
                        (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE))
                    {
                        clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                    "Node [%s] is not reachable, probably "
                                    "it has been failed over already.",
                                    cpmL->pCpmLocalInfo->nodeName);
                    }
                }
            }
            else
            {
                clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                            "Node [%s] is not registered, probably "
                            "it has been failed over already.",
                            cpmL->nodeName);
            }
        }
    }
    else
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                      "Node failfast called on [%s] node [%*s] !!! "
                      "This indicates that the cluster has become "
                      "unstable. Doing self shutdown...",
                      CL_CPM_IS_STANDBY() ? "standby":
                      CL_CPM_IS_WB()      ? "worker node" :
                      "<invalid-node-type>",
                      nodeName->length,
                      nodeName->value);
        cpmSelfShutDown();
    }

    return CL_OK;

failure:
    return rc;
}

static ClRcT cpmResetLegacyNode(ClNameT *nodeName)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL = NULL;
    /*
     * Get the chassis id, slot ID configured for NON-asp aware nodes
     * to trigger a reset.
     */
    rc = cpmNodeFind(nodeName->value, &cpmL);
    if(rc == CL_OK)
    {
        if(cpmL->slotID)
        {
            clLogCritical("LEGACY-NODE", "RESET", "CPM resetting legacy node [%s] "
                          "with chassis id [%d], slotID [%d]", 
                          nodeName->value, cpmL->chassisID, cpmL->slotID);
            rc = clCmBladeOperationRequest(cpmL->chassisID, cpmL->slotID, CL_CM_RESET_REQUEST);
        }
        else
        {
            clLogCritical("LEGACY-NODE", "RESET", "CPM legacy node [%s] not configured for reset "
                          " [%d:%d]", nodeName->value, cpmL->chassisID, cpmL->slotID);
        }
    }
    else
    {
        clLogCritical("LEGACY-NODE", "RESET", "CPM legacy node [%s] not found",
                      nodeName->value);
    }
    return rc;
}

ClRcT _cpmNodeFailFast(ClNameT *nodeName, ClBoolT isASPAware)
{
    if(isASPAware)
        return _cpmNodeFailFastRestart(nodeName, CL_CPM_RESTART_NODE);

    return cpmResetLegacyNode(nodeName);
}

/*  FAILOVER
 *  FIXME: Currently this is same as the _cpmNodeFailFast() function,
 *  but it needs to be changed to handle failover scenario. 
 *  Need to define the proper behavior
 *  May need to cleanup the components and close external resources 
 *  before bringing down the node.
 *  For current solution just power off the failed over node. But
 *  as CM does not support power off, doing reset as of now
 *  Does the failover of the given node.
 */
ClRcT _cpmNodeFailOver(ClNameT *nodeName, ClBoolT isASPAware)
{
    if(isASPAware)
        return _cpmNodeFailFastRestart(nodeName, CL_CPM_RESTART_NONE);

    return cpmResetLegacyNode(nodeName);
}

ClRcT _cpmNodeFailOverRestart(ClNameT *nodeName, ClBoolT isASPAware)
{
    if(isASPAware)
        return _cpmNodeFailFastRestart(nodeName, CL_CPM_RESTART_ASP);

    return cpmResetLegacyNode(nodeName);
}

ClRcT _cpmNodeHalt(ClNameT *nodeName, ClBoolT aspAware)
{
    if(aspAware)
        return _cpmNodeFailFastRestart(nodeName, CL_CPM_HALT_ASP);

    return cpmResetLegacyNode(nodeName);
}

ClRcT _cpmClusterReset(void)
{
    ClRcT rc = CL_OK;
    ClUint32T numNodes = 0;
    ClIocNodeAddressT *pNeighbors = NULL;
    ClUint32T i;
    /*
     * First get the neighbors running in the cluster.
     */
    clIocTotalNeighborEntryGet(&numNodes);
    if(!numNodes)
        return CL_CPM_RC(CL_ERR_NOT_EXIST);
    pNeighbors = clHeapCalloc(numNodes, sizeof(*pNeighbors));
    CL_ASSERT(pNeighbors != NULL);
    clIocNeighborListGet(&numNodes, pNeighbors);
    if(!numNodes)
    {
        clHeapFree(pNeighbors);
        return CL_CPM_RC(CL_ERR_NOT_EXIST);
    }

    for(i = 0; i < numNodes; ++i)
    {
        ClUint8T status = 0;
        ClUint32T restartFlag = CL_CPM_RESTART_NODE;
        if(pNeighbors[i] == clIocLocalAddressGet())
            continue;
        clIocRemoteNodeStatusGet(pNeighbors[i], &status);
        if(status)
        {
            rc = CL_CPM_CALL_RMD_ASYNC_NEW(pNeighbors[i],
                                           CL_IOC_CPM_PORT,
                                           CPM_RESET_ELSE_COMMIT_SUICIDE,
                                           (ClUint8T *) &restartFlag,
                                           sizeof(restartFlag),
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           0,
                                           0,
                                           NULL,
                                           NULL,
                                           clXdrMarshallClUint32T);
            if (CL_OK != rc)
            {
                if ((CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) ||
                    (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE))
                {
                    clLogNotice("CLUSTER", "RESET",
                                "Node [%#x] is not reachable, probably "
                                "it has been failed over already.",
                                pNeighbors[i]);
                }
            }
        }
        else
        {
            clLogNotice("CLUSTER", "RESET",
                        "Node [%#x] is not registered, probably "
                        "it has been failed over already.",
                        pNeighbors[i]);
        }
            
    }
    clHeapFree(pNeighbors);
    if(numNodes > 1)
    {
        /*
         * Delay before killing ourselves.
         */
        sleep(2);
    }
    cpmResetNodeElseCommitSuicide(CL_CPM_RESTART_NODE);
    return CL_OK;
}

ClRcT VDECL(cpmComponentCSIRmv)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompCSIRmvT recvBuff;
    ClUint32T msgLength = 1;
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;

    rc = VDECL_VER(clXdrUnmarshallClCpmCompCSIRmvT, 4, 0, 0)(inMsgHandle, (void *) &recvBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"), rc);

    /*
     *  when we are in this function, then it is for the component of the local 
     *  node, make RMD to the component which will in turn invoke the callback 
     */

    if (!strcmp(gpClCpm->pCpmConfig->nodeName, recvBuff.nodeName.value))
    {
        /*
         * Find the component Name from the component Hash Table 
         */
        rc = cpmCompFind(recvBuff.compName.value, gpClCpm->compTable, &comp);
        CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to find component %s \n",
                      recvBuff.compName.value), rc);

        switch(comp->compConfig->compProperty)
        {
            case CL_AMS_COMP_PROPERTY_SA_AWARE:
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                               comp->eoPort,
                                               CPM_CSI_RMV_FN_ID,
                                               (ClUint8T *) &recvBuff,
                                               msgLength,
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0, 0, 0,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmCompCSIRmvT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
            case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
            {
                rc = cpmCompFind(recvBuff.proxyCompName.value, gpClCpm->compTable, &proxyComp);
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                        ("Unable to find component %s \n", recvBuff.proxyCompName.value), rc);

                /* Make an RMD to the proxyComp. */
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                               proxyComp->eoPort,
                                               CPM_CSI_RMV_FN_ID,
                                               (ClUint8T *) &recvBuff,
                                               msgLength,
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0,
                                               0,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmCompCSIRmvT, 4, 0, 0));
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                break;
            }
            case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
            default:
            {
                /* PROXIED: */
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
                break;
            }
        }
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);

  failure:
    return rc;
}

ClRcT VDECL(cpmCBResponse)(ClEoDataT data,
                           ClBufferHandleT inMsgHandle,
                           ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmResponseT responseBuff;
    ClUint32T cbType = 0;
    ClIocNodeAddressT masterIocAddress;
    ClInvocationT invocation;
    ClCpmComponentT *comp = NULL;
    ClUint32T cpmShutDown = 0;

    clOsalMutexLock(&gpClCpm->cpmShutdownMutex);

    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    cpmShutDown = gpClCpm->cpmShutDown;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);

    if(cpmShutDown == 2)
    {
        clLogDebug("CPM", "RESPONSE", "Skipping response as cpm is shutdown");
        goto failure;
    }

    rc = VDECL_VER(clXdrUnmarshallClCpmResponseT, 4, 0, 0)(inMsgHandle, (void *) &responseBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                 rc);

    invocation = responseBuff.invocation;
    CL_CPM_INVOCATION_CB_TYPE_GET(invocation, cbType);

    clLogDebug("CPM", "RESPONSE", "Got response for invocation [%#llx]", invocation);

    switch (cbType)
    {
    case CL_CPM_CSI_SET_CALLBACK:
    case CL_CPM_CSI_RMV_CALLBACK:
    case CL_CPM_CSI_QUIESCING_CALLBACK:
        {
            if (CL_CPM_IS_ACTIVE())
            {
                if (gpClCpm->cpmToAmsCallback != NULL &&
                    gpClCpm->cpmToAmsCallback->csiOperationComplete != NULL)
                {
                    gpClCpm->cpmToAmsCallback->
                        csiOperationComplete(responseBuff.invocation,
                                             responseBuff.retCode);
                }
                else
                    rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
            }
            else            /* Forward it to Active master */
            {
                ClInt32T tries = 0;
                ClTimerTimeOutT delay = {.tsMilliSec = 100, .tsSec = 0 };

                /*
                 * This is the critical path to forward responses to AMS master.
                 * We try hard to ensure that the response is forwarded to
                 * the master since there could be a window where the master
                 * is transitioning and any CSI replay responses for the components 
                 * in this node would be lost thereby leading to fault escalations.
                 */

                do
                {
                    rc = clCpmMasterAddressGet(&masterIocAddress);
                    if(rc == CL_OK)
                    {
                        rc = CL_CPM_CALL_RMD_ASYNC_NEW(masterIocAddress,
                                                       CL_IOC_CPM_PORT,
                                                       CPM_CB_RESPONSE,
                                                       (ClUint8T *) &responseBuff,
                                                       sizeof(ClCpmResponseT),
                                                       NULL,
                                                       NULL,
                                                       0,
                                                       0,
                                                       0,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       MARSHALL_FN(ClCpmResponseT, 4, 0, 0));
                    }
                } while(rc != CL_OK && 
                        ++tries < 5 && 
                        clOsalTaskDelay(delay) == CL_OK);
                CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed with [%#x]\n", rc), rc);
            }
            break;
        }
    case CL_CPM_TERMINATE_CALLBACK:
        {
            clOsalMutexLock(&gpClCpm->compTerminateMutex);
            rc = cpmInvocationGet(invocation, &cbType, (void **) &comp);
            if (rc == CL_OK && comp != NULL && comp->compConfig != NULL)
            {
                cpmResponse(responseBuff.retCode, comp, CL_CPM_TERMINATE);
                /*
                 * Delete the invocation
                 */
                cpmInvocationDeleteInvocation(invocation);
            }
            else
            {
                clLogInfo("CPM", "RESPONSE", "Skipping terminate processing for invocation [%#llx]", invocation);
            }
            clOsalMutexUnlock(&gpClCpm->compTerminateMutex);
            break;
        }
    case CL_CPM_PROXIED_INSTANTIATE_CALLBACK:
        {
            rc = cpmInvocationGet(invocation, &cbType, (void **) &comp);
            if (rc == CL_OK && comp != NULL)
            {
                cpmResponse(responseBuff.retCode, comp, CL_CPM_PROXIED_INSTANTIATE);
                /*
                 * Delete invocation.
                 */
                cpmInvocationDeleteInvocation(invocation);
            }
            else
            {
                clLogInfo("CPM", "RESPONSE", "Skipping proxied instantiate response processing for "
                          "invocation [%#llx]", invocation);
            }
            break;
        }
    case CL_CPM_PROXIED_CLEANUP_CALLBACK:
        {
            rc = cpmInvocationGet(invocation, &cbType, (void **) &comp);
            if (rc == CL_OK && comp != NULL)
            {
                cpmResponse(responseBuff.retCode, comp, CL_CPM_PROXIED_CLEANUP);
                /*
                 * Delete the invocation entry.
                 */
                cpmInvocationDeleteInvocation(invocation);
            }
            else
            {
                clLogInfo("CPM", "RESPONSE", "Skipping proxied cleanup response processing for "
                          "invocation [%#llx]", invocation);
            }
            break;
        }
    case CL_CPM_HB_CALLBACK:
        {
            rc = cpmInvocationGet(invocation, &cbType, (void **) &comp);
            if (rc == CL_OK && comp != NULL)
            {
                cpmResponse(responseBuff.retCode, comp, CL_CPM_HEALTHCHECK);
                /*
                 * Delete invocation entry.
                 */
                cpmInvocationDeleteInvocation(invocation);
            }
            else
            {
                clLogInfo("CPM", "RESPONSE", "Skipping heartbeat response processing for invocation [%#llx]", invocation);
            }
            break;
        }
    default:
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    }

    failure:
    clOsalMutexUnlock(&gpClCpm->cpmShutdownMutex);
    return rc;
}

ClRcT VDECL(cpmQuiescingComplete)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmQuiescingCompleteT responseBuff;
    ClUint32T cbType = 0;
    ClIocNodeAddressT masterIocAddress;

    rc = VDECL_VER(clXdrUnmarshallClCpmQuiescingCompleteT, 4, 0, 0)(inMsgHandle,
                                                (void *) &responseBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
            rc);

    CL_CPM_INVOCATION_CB_TYPE_GET(responseBuff.invocation, cbType);

    if (CL_CPM_IS_ACTIVE())
    {
        /*
         * Call _AMS_ Function 
         */
        if (gpClCpm->cpmToAmsCallback != NULL &&
            gpClCpm->cpmToAmsCallback->csiQuiescingComplete != NULL)
        {
            gpClCpm->cpmToAmsCallback->csiQuiescingComplete(responseBuff.
                                                            invocation,
                                                            responseBuff.
                                                            retCode);
        }
        else
            rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
    }
    else                        /* Forward it to Active master */
    {
        rc = clCpmMasterAddressGet(&masterIocAddress);
        if(rc != CL_OK)
        {
            CL_CPM_CHECK(CL_DEBUG_ERROR, ("Failed to get CPM master address. error code [0x%x].\n", rc), rc);
            return rc;
        }

        rc = CL_CPM_CALL_RMD_ASYNC_NEW(masterIocAddress,
                                       CL_IOC_CPM_PORT,
                                       CPM_COMPONENT_QUIESCING_COMPLETE,
                                       (ClUint8T *) &responseBuff,
                                       sizeof(ClCpmQuiescingCompleteT),
                                       NULL,
                                       NULL,
                                       0,
                                       0,
                                       0,
                                       0,
                                       NULL,
                                       NULL,
                                       MARSHALL_FN(ClCpmQuiescingCompleteT, 4, 0, 0));
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
    }


  failure:
    return rc;
}

ClRcT VDECL(cpmPGTrack)(ClEoDataT data,
                        ClBufferHandleT inMsgHandle,
                        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmPGTrackT recvBuff;
    ClIocAddressT iocAddress;
    ClAmsPGNotificationBufferT *notificationBuffer = NULL;

    if (!CL_CPM_IS_ACTIVE())
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
        goto failure;
    }
    
    rc = VDECL_VER(clXdrUnmarshallClCpmPGTrackT, 4, 0, 0)(inMsgHandle, (void *) &recvBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                 rc);

    if (gpClCpm->cpmToAmsCallback != NULL &&
        gpClCpm->cpmToAmsCallback->pgTrack != NULL)
    {
        if (recvBuff.responseNeeded == CL_TRUE)
        {
            memcpy(&iocAddress, &recvBuff.iocAddress.clIocAddressIDLT,
                   sizeof(iocAddress));
            /****************/
            notificationBuffer = clHeapAllocate(sizeof(ClAmsPGNotificationBufferT));
            if(notificationBuffer == NULL)
            {
                rc = CL_ERR_NO_MEMORY;
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, 
                        ("Failed to allocate [%zd] bytes of memory, rc=[0x%x]\n", sizeof(ClAmsPGNotificationBufferT),rc), 
                        rc);
            }

            /*
             * Bug 4457:
             * Catch the return value of AMS callback pgTrack and goto 
             * failure if return code is not CL_OK.
             */
            rc = gpClCpm->cpmToAmsCallback->pgTrack(iocAddress, 
                                               recvBuff.cpmHandle,
                                               &(recvBuff.csiName),
                                               recvBuff.trackFlags,
                                               notificationBuffer);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, 
                        ("PG Track invocation returned error, rc=[0x%x]\n", rc), 
                        rc);

            rc = clXdrMarshallClUint32T(
                        (void *)&notificationBuffer->numItems, outMsgHandle, 0);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, 
                        ("Unable to write the message \n"), rc);

            if (notificationBuffer->notification == NULL)
            {
                /* FIXME: handle this case */
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_CPM_CLIENT_LIB,
                        CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CPM_CLIENT_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory"),
                        CL_CPM_RC(CL_ERR_NO_MEMORY));
            }
            else
            {
                ClUint32T i = 0;
                
                for (i = 0; i < notificationBuffer->numItems; ++i) 
                {
                    rc = VDECL_VER(clXdrMarshallClAmsPGNotificationT, 4, 0, 0)(
                            (void *)&(notificationBuffer->notification[i]), 
                            outMsgHandle, 0); 
                    CPM_CLIENT_CHECK(CL_DEBUG_ERROR, 
                            ("Unable to write the message \n"), rc);
                }
            }
        }
        else
        {
            memcpy(&iocAddress, &recvBuff.iocAddress.clIocAddressIDLT,
                   sizeof(iocAddress));
            /*
             * Bug 4457:
             * Catch the return value of AMS callback pgTrack and goto 
             * failure if return code is not CL_OK.
             */
            rc = gpClCpm->cpmToAmsCallback->pgTrack(iocAddress, 
                                               recvBuff.cpmHandle,
                                               &(recvBuff.csiName),
                                               recvBuff.trackFlags, NULL);
            CPM_CLIENT_CHECK(CL_DEBUG_ERROR, 
                        ("PG Track invocation returned error, rc=[0x%x]\n", rc), 
                        rc);
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
        goto failure;
    }

  failure:
    
    if(notificationBuffer != NULL)
    {
        if(notificationBuffer->notification)
            clHeapFree(notificationBuffer->notification);
        clHeapFree(notificationBuffer);
    }
    return rc;
}

ClRcT VDECL(cpmPGTrackStop)(ClEoDataT data,
                            ClBufferHandleT inMsgHandle,
                            ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmPGTrackStopT recvBuff;
    ClIocAddressT iocAddress;
    
    if(! CL_CPM_IS_ACTIVE())
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
        goto failure;
    }
    
    rc = VDECL_VER(clXdrUnmarshallClCpmPGTrackStopT, 4, 0, 0)(inMsgHandle, (void *) &recvBuff);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                 rc);

    if (gpClCpm->cpmToAmsCallback != NULL &&
        gpClCpm->cpmToAmsCallback->pgTrackStop != NULL)
    {
        memcpy(&iocAddress, &recvBuff.iocAddress.clIocAddressIDLT,
               sizeof(iocAddress));
        gpClCpm->cpmToAmsCallback->pgTrackStop(iocAddress,
                                               recvBuff.cpmHandle,
                                               &(recvBuff.csiName));
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_BAD_OPERATION);

  failure:
    return rc;
}


/*
 * This IPI deallocates the AMS to CPM call structure.
 */
void clCpmAmsToCpmFree(CL_IN ClCpmAmsToCpmCallT *callback)
{
    if (callback != NULL)
    {
        clHeapFree(callback);
        callback = NULL;
    }
}

/*
 * This IPI allocates and initialize the AMS to CPM call structure.
 */
ClRcT clCpmAmsToCpmInitialize(CL_IN ClCpmAmsToCpmCallT **callback)
{
    ClRcT rc = CL_OK;
    ClCpmAmsToCpmCallT *tmp = NULL;

    /*
     * Check the input parameter 
     */
    if (callback == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid parameter passed \n"),
                     CL_CPM_RC(CL_ERR_NULL_POINTER));

    tmp = (ClCpmAmsToCpmCallT *) clHeapAllocate(sizeof(ClCpmAmsToCpmCallT));
    if (tmp == NULL)
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to allocate memory \n"),
                     CL_CPM_RC(CL_ERR_NULL_POINTER));

    /*
     * Initialize the callback pointers 
     */
    tmp->compInstantiate = _cpmComponentInstantiate;
    tmp->compTerminate = _cpmComponentTerminate;
    tmp->compCleanup = _cpmComponentCleanup;
    tmp->compRestart = _cpmComponentRestart;
    tmp->compCSISet = _cpmComponentCSISet;
    tmp->compCSIRmv = _cpmComponentCSIRmv;
    tmp->compPGTrack = _cpmComponentPGTrack;
    tmp->nodeDepartureAllowed = _cpmNodeDepartureAllowed;
    tmp->cpmIocAddressForNodeGet = _cpmIocAddressForNodeGet;
    tmp->cpmNodeNameForNodeAddressGet = _cpmNodeNameForNodeAddressGet;
    tmp->cpmNodeFailFast = _cpmNodeFailFast;
    tmp->cpmNodeFailOver = _cpmNodeFailOver;
    tmp->cpmNodeFailOverRestart = _cpmNodeFailOverRestart;
    tmp->cpmEntityAdd = cpmEntityAdd;
    tmp->cpmEntityRmv = cpmEntityRmv;
    tmp->cpmEntitySetConfig = cpmEntitySetConfig;
    tmp->cpmNodeHalt = _cpmNodeHalt;
    tmp->cpmClusterReset = _cpmClusterReset;
    *callback = tmp;

    return CL_OK;

  failure:
    return rc;
}


ClRcT cpmReplayInvocationAdd(ClUint32T cbType, const ClCharT *pCompName, 
                             const ClCharT *pNode, ClBoolT *pResponsePending)
{
    ClCpmComponentT *comp = NULL;
    ClInvocationT invocation = 0;
    ClNameT nodeName = {0};
    ClIocAddressT nodeAddress = {{0}};
    ClAmsInvocationT *pInvocation = NULL;
    ClBoolT addInvocation = CL_TRUE;
    ClRcT rc = CL_OK;

    if(!pCompName || !pNode) 
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(!gpClCpm->polling) 
        return CL_CPM_RC(CL_ERR_INVALID_STATE);

    clNameSet(&nodeName, pNode);

    rc = _cpmIocAddressForNodeGet(&nodeName, &nodeAddress);

    if(rc != CL_OK)
    {
        clLogError("INVOCATION", "REPLAY", "Node address for node [%s] returned [%#x]", pNode, rc);
        return rc;
    }

    clOsalMutexLock(gpClCpm->compTableMutex);

    if(nodeAddress.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
    {
        rc = cpmCompFindWithLock((ClCharT*)pCompName, gpClCpm->compTable, &comp);
        if(rc != CL_OK)
        {
            clLogError("INVOCATION", "ADD", "Component [%s] not found in the comp table",
                       pCompName);
            goto out_unlock;
        }
    }
    /*
     * Remote node instantiate pending. So mark it and do it after failover prologue is through.
     */
    pInvocation = clHeapCalloc(1, sizeof(*pInvocation));
    CL_ASSERT(pInvocation != NULL);
    pInvocation->cmd = cbType;
    clNameSet(&pInvocation->compName, pCompName);
    clNameSet(&pInvocation->csiName,  pNode);

    switch(cbType)
    {
    case CL_AMS_INSTANTIATE_REPLAY_CALLBACK:
        {
            if(comp)
            {
                if(comp->eoPort)
                {
                    if(pResponsePending)
                        *pResponsePending = CL_FALSE;
                }
                else
                    addInvocation = CL_FALSE;
            }
            
        }
        break;

    case CL_AMS_TERMINATE_REPLAY_CALLBACK:
        {
            if(comp)
            {
                if(!comp->eoPort)                
                {
                    if(pResponsePending)
                        *pResponsePending = CL_FALSE;
                }
                else
                    addInvocation = CL_FALSE;
            }
        }
        break;

    default: 
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        addInvocation = CL_FALSE;
        goto out_unlock;
    }

    if(addInvocation)
    {
        pInvocation->csiTargetOne = comp ? CL_TRUE : CL_FALSE;
        rc = cpmInvocationAdd(cbType, pInvocation, &invocation, 
                              CL_CPM_INVOCATION_AMS | CL_CPM_INVOCATION_DATA_COPIED);
    }

    out_unlock:
    clOsalMutexUnlock(gpClCpm->compTableMutex);

    if(!addInvocation)
        clHeapFree(pInvocation);

    return rc;
}

ClRcT cpmReplayInvocationsGet(ClAmsInvocationT ***ppInvocations, 
                              ClUint32T *pNumInvocations, ClBoolT canDelete)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0, nextNodeHandle = 0;
    ClCpmInvocationT *invocationData = NULL;
    ClAmsInvocationT **pInvocations = NULL;
    ClUint32T numInvocations = 0;
    void *data = NULL;
    ClUint32T cbType = 0;

    if(!ppInvocations || !pNumInvocations)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(gpClCpm->invocationMutex);

    rc = clCntFirstNodeGet(gpClCpm->invocationTable, &nodeHandle);
    if (rc != CL_OK)
        goto withLock;


    while (nodeHandle)
    {
        rc = clCntNodeUserDataGet(gpClCpm->invocationTable, nodeHandle,
                                  (ClCntDataHandleT *) &invocationData);

        if (rc != CL_OK)
            goto withLock;

        rc = clCntNextNodeGet(gpClCpm->invocationTable, nodeHandle,
                              &nextNodeHandle);

        CL_CPM_INVOCATION_CB_TYPE_GET(invocationData->invocation, cbType);
        
        if(cbType != CL_AMS_INSTANTIATE_REPLAY_CALLBACK 
           &&
           cbType != CL_AMS_TERMINATE_REPLAY_CALLBACK
           &&
           cbType != CL_AMS_RECOVERY_REPLAY_CALLBACK)
        {
            nodeHandle = nextNodeHandle;
            continue;
        }

        if((data = invocationData->data))
        {
            ClInvocationT invocation = invocationData->invocation;
            ClAmsInvocationT *pInvocation = (ClAmsInvocationT*)data;
            if(cbType == CL_AMS_RECOVERY_REPLAY_CALLBACK)
            {
                invocation = pInvocation->invocation; /*as its instantiate cookie*/
            }
            pInvocations = clHeapRealloc(pInvocations, sizeof(*pInvocations)*(numInvocations+1));
            CL_ASSERT(pInvocations != NULL);
            pInvocation->invocation = invocation;
            pInvocations[numInvocations++] = pInvocation;
        }
        if(canDelete)
        {
            if (clCntNodeDelete(gpClCpm->invocationTable, nodeHandle) != CL_OK)
                goto withLock;
            clHeapFree(invocationData);
        }

        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            break;

        if (rc != CL_OK)
            goto withLock;

        nodeHandle = nextNodeHandle;

    }
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    *ppInvocations = pInvocations;
    *pNumInvocations = numInvocations;
    return CL_OK;

    withLock:
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    if(pInvocations)
        clHeapFree(pInvocations);
    return rc;
}

ClRcT cpmReplayInvocations(ClBoolT canDelete)
{
    ClAmsInvocationT **pAmsInvocations = NULL;
    ClUint32T numInvocations = 0;
    ClUint32T i;
    ClRcT rc = CL_OK;
    
    if(!gpClCpm->polling) 
        return CL_CPM_RC(CL_ERR_INVALID_STATE);

    rc = cpmReplayInvocationsGet(&pAmsInvocations, &numInvocations, canDelete);
    if(rc != CL_OK || !numInvocations)
    {
        return rc;
    }
    for(i = 0; i < numInvocations; ++i)
    {
        ClCpmCompRequestTypeT type = CL_CPM_REQUEST_NONE;
        if(pAmsInvocations[i]->cmd == CL_AMS_INSTANTIATE_REPLAY_CALLBACK)
            type = CL_CPM_INSTANTIATE;
        else if(pAmsInvocations[i]->cmd == CL_AMS_TERMINATE_REPLAY_CALLBACK)
            type = CL_CPM_TERMINATE;
        else if(pAmsInvocations[i]->cmd == CL_AMS_RECOVERY_REPLAY_CALLBACK)
            type = CL_CPM_CLEANUP;

        switch(type)
        {
        case CL_CPM_INSTANTIATE:
        case CL_CPM_TERMINATE:
            {
                /*
                 * Check for remote nodes here.
                 */
                if(!pAmsInvocations[i]->csiTargetOne)
                {
                    ClIocAddressT nodeAddress = {{0}};
                    ClIocAddressT compAddress = {{0}};
                    rc = _cpmIocAddressForNodeGet(&pAmsInvocations[i]->csiName, &nodeAddress);
                    if(rc != CL_OK)
                    {
                        clLogWarning("REPLAY", "INVOCATION", "Ignoring [%s] replay for component [%s] "
                                     "as the node [%s] to which it belongs is not present",
                                     type == CL_CPM_INSTANTIATE ? "instantiate" : "terminate",
                                     pAmsInvocations[i]->compName.value, pAmsInvocations[i]->csiName.value);
                        goto next;
                    }
                    clLogDebug("REPLAY", "INVOCATION", "Getting component address for [%s]",
                               pAmsInvocations[i]->compName.value);
                    rc = clCpmComponentAddressGet(nodeAddress.iocPhyAddress.nodeAddress,
                                                  &pAmsInvocations[i]->compName, &compAddress);
                    if(rc != CL_OK)
                    {
                        clLogWarning("REPLAY", "INVOCATION", "Could not get component [%s] address "
                                     "because of error [%#x]", pAmsInvocations[i]->compName.value, rc);
                        goto next;
                    }
                    if((type == CL_CPM_INSTANTIATE && !compAddress.iocPhyAddress.portId)
                       ||
                       (type == CL_CPM_TERMINATE && compAddress.iocPhyAddress.portId))
                    {
                        clLogWarning("REPLAY", "INVOCATION", "Skipping [%s] replay for component [%s] "
                                     "which is pending response", 
                                     type == CL_CPM_INSTANTIATE ? "instantiate" : "terminate",
                                     pAmsInvocations[i]->compName.value);
                        goto next;
                    }
                }
                if(gpClCpm->cpmToAmsCallback &&
                   gpClCpm->cpmToAmsCallback->compOperationComplete)
                {
                    clLogNotice("REPLAY", "INVOCATION", "Replaying [%s] invocation for component [%s]",
                                type == CL_CPM_INSTANTIATE ? "instantiate" : "terminate",
                                pAmsInvocations[i]->compName.value);
                    gpClCpm->cpmToAmsCallback->compOperationComplete(pAmsInvocations[i]->compName,
                                                                     type,
                                                                     CL_OK);
                }
            }
            break;

        case CL_CPM_CLEANUP:
            {
                clLogNotice("REPLAY", "INVOCATION", "Replaying recovery invocation for component [%s]",
                            pAmsInvocations[i]->compName.value);
                clCpmComponentFailureReportWithCookie(0, &pAmsInvocations[i]->compName, 
                                                      pAmsInvocations[i]->invocation,
                                                      0, CL_AMS_RECOVERY_NO_RECOMMENDATION, 0); 
            }
            break;

        default: break;
        }

        next:
        clHeapFree(pAmsInvocations[i]);
    }
    clHeapFree(pAmsInvocations);
    return rc;
}
