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
 * This file implements CPM-COR interaction.
 */

/*
 * Standard header files 
 */
#include <string.h>

/*
 * ASP header files 
 */
# include <clDebugApi.h>
# include <clLogApi.h>
# include <clCorApi.h>
# include <clCorMetaData.h>
# include <clCorUtilityApi.h>
# include <clCorErrors.h>

/*
 * CPM internal header files 
 */
# include <clCpmCor.h>
# include <clCpmLog.h>
# include <clCpmInternal.h>


/*
 * XDR header files 
 */
#include <clXdrApi.h>
#include "xdrClCpmSlotInfoRecvT.h"
#include "xdrClCpmSlotInfoFieldIdT.h"

ClRcT cpmCorMoIdToMoIdNameGet(ClCorMOIdT *moId, SaNameT *moIdName)
{
#ifdef USE_COR
    
    ClRcT rc = CL_OK;

    moIdName->length = 0; /* Clear the MOID name */
    moIdName->value[0] = 0;
    
    if(cpmIsAspSULoaded("corSU"))
    {
        rc = clCorMoIdToMoIdNameGet(moId, moIdName);
        /* 
         * Double check - cor might not be enabled
         */
        if(CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE)
            rc = CL_OK; 
    }
    return rc;
#else
    return CL_ERR_NOT_SUPPORTED;    
#endif    
}

                            // coverity[pass_by_value]
ClRcT cpmCorNodeObjectCreate(SaNameT nodeMoIdName)
{
#ifdef USE_COR
    
    ClRcT rc = CL_OK;
    ClCorMOIdT nodeMoId = {{{0}}};
    ClCorObjectHandleT objH;
    ClInt16T depth;
    /*
     * COR not loaded.
     */
    if(!cpmIsAspSULoaded("corSU"))  return CL_OK;

    rc = clCorMoIdNameToMoIdGet(&nodeMoIdName, &nodeMoId);
    CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to get MoId from MoId name, rc=[0x%x]\n", rc), rc);

    depth = clCorMoIdDepthGet(&nodeMoId);
    CL_CPM_CHECK(CL_DEBUG_ERROR,
                     ("Unable to get MoId depth, rc=[0x%x]\n", rc), rc);

    for(nodeMoId.depth = 1; nodeMoId.depth <= depth; nodeMoId.depth++)
    {
        clCorMoIdServiceSet(&nodeMoId, CL_COR_INVALID_SRVC_ID);
        rc = clCorUtilMoAndMSOCreate(&nodeMoId, &objH);
        if(rc != CL_OK)
        {
            /* Check if the object is already present. If so just continue */
            if( (CL_GET_ERROR_CODE(rc) == CL_COR_INST_ERR_MSO_ALREADY_PRESENT) ||
                    (CL_GET_ERROR_CODE(rc) == CL_COR_INST_ERR_MO_ALREADY_PRESENT) )
            {
                /* The MO or MSO is already present. Just continue */
                rc = CL_OK;
                continue;
            }
            else
            {
                CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("Unable to create objects, rc=[0x%x]\n", rc), rc);
            }
        }
    }

failure:
    return rc;
#else
    return CL_ERR_NOT_SUPPORTED;    
#endif       
}

/* 
 * Given slot Id return the nodeName, MoId and IOC address of the node.
 */

ClRcT VDECL(cpmSlotInfoGet)(ClEoDataT data,
                            ClBufferHandleT inMsgHandle,
                            ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT hNode = 0;
    ClCpmLT *cpmL = NULL;
    ClUint32T cpmLCount = 0;
    ClCpmSlotInfoRecvT  slotInfoRecv;
    ClUint32T found = 0;

    rc = VDECL_VER(clXdrUnmarshallClCpmSlotInfoRecvT, 4, 0, 0)(inMsgHandle, (void *)&slotInfoRecv);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clCntFirstNodeGet(gpClCpm->cpmTable, &hNode);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get first cpmTable Node %x\n", rc),
                 rc);
    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                              (ClCntDataHandleT *) &cpmL);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to get container Node data %x\n", rc),
                 rc);

    cpmLCount = gpClCpm->noOfCpm;

    switch(slotInfoRecv.flag)
    {
        case CL_CPM_SLOT_ID:
        {
            while (cpmLCount)
            {
                if(cpmL->pCpmLocalInfo != NULL && 
                        cpmL->pCpmLocalInfo->status == CL_CPM_EO_ALIVE &&
                        slotInfoRecv.slotId == cpmL->pCpmLocalInfo->slotNumber)
                {
                    slotInfoRecv.nodeIocAddress = cpmL->pCpmLocalInfo->cpmAddress.nodeAddress;

                    memcpy(&slotInfoRecv.nodeMoIdStr, &cpmL->pCpmLocalInfo->nodeMoIdStr, sizeof(SaNameT));

                    strcpy((ClCharT *)slotInfoRecv.nodeName.value, cpmL->pCpmLocalInfo->nodeName);
                    slotInfoRecv.nodeName.length = strlen((const ClCharT *)slotInfoRecv.nodeName.value);

                    rc = VDECL_VER(clXdrMarshallClCpmSlotInfoRecvT, 4, 0, 0)((void *)&slotInfoRecv, outMsgHandle, 0);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

                    found = 1;
                    break;

                }
                cpmLCount--;
                if (cpmLCount)
                {
                    rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                            rc);
                    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                            (ClCntDataHandleT *) &cpmL);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                            ("Unable to get container Node data %d\n", rc), rc);
                }
            }
            break;
        }
        case CL_CPM_IOC_ADDRESS:
        {
            while (cpmLCount)
            {
                if(cpmL->pCpmLocalInfo != NULL && 
                        cpmL->pCpmLocalInfo->status == CL_CPM_EO_ALIVE &&
                        slotInfoRecv.nodeIocAddress == cpmL->pCpmLocalInfo->cpmAddress.nodeAddress)
                {
                    slotInfoRecv.slotId = cpmL->pCpmLocalInfo->slotNumber;

                    memcpy(&slotInfoRecv.nodeMoIdStr, &cpmL->pCpmLocalInfo->nodeMoIdStr, sizeof(SaNameT));

                    strcpy((ClCharT *)slotInfoRecv.nodeName.value, cpmL->pCpmLocalInfo->nodeName);
                    slotInfoRecv.nodeName.length = strlen((const ClCharT *)slotInfoRecv.nodeName.value);

                    rc = VDECL_VER(clXdrMarshallClCpmSlotInfoRecvT, 4, 0, 0)((void *)&slotInfoRecv, outMsgHandle, 0);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

                    found = 1;
                    break;
                }
                cpmLCount--;
                if (cpmLCount)
                {
                    rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                            rc);
                    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                            (ClCntDataHandleT *) &cpmL);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                            ("Unable to get container Node data %d\n", rc), rc);
                }
            }
            break;
        }
        case CL_CPM_NODE_MOID:
        {
            while (cpmLCount)
            {
                    if (cpmL->pCpmLocalInfo != NULL && cpmL->pCpmLocalInfo->status == CL_CPM_EO_ALIVE
                                    && (strcmp((const ClCharT *) slotInfoRecv.nodeMoIdStr.value,
                                                    (const ClCharT *) (cpmL->pCpmLocalInfo->nodeMoIdStr.value)) == 0))
                    {
                    slotInfoRecv.slotId = cpmL->pCpmLocalInfo->slotNumber;

                    slotInfoRecv.nodeIocAddress = cpmL->pCpmLocalInfo->cpmAddress.nodeAddress;

                    strcpy((ClCharT *)slotInfoRecv.nodeName.value, cpmL->pCpmLocalInfo->nodeName);
                    slotInfoRecv.nodeName.length = strlen((const ClCharT *)slotInfoRecv.nodeName.value);

                    rc = VDECL_VER(clXdrMarshallClCpmSlotInfoRecvT, 4, 0, 0)((void *)&slotInfoRecv, outMsgHandle, 0);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

                    found = 1;
                    break;
                }
                cpmLCount--;
                if (cpmLCount)
                {
                    rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                            rc);
                    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                            (ClCntDataHandleT *) &cpmL);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                            ("Unable to get container Node data %d\n", rc), rc);
                }
            }
            break;
        }
        case CL_CPM_NODENAME:
        {
            while (cpmLCount)
            {
                    if (cpmL->pCpmLocalInfo != NULL && cpmL->pCpmLocalInfo->status == CL_CPM_EO_ALIVE
                                    && (strcmp((const ClCharT *) slotInfoRecv.nodeName.value, cpmL->pCpmLocalInfo->nodeName) == 0))
                {
                    slotInfoRecv.slotId = cpmL->pCpmLocalInfo->slotNumber;

                    slotInfoRecv.nodeIocAddress = cpmL->pCpmLocalInfo->cpmAddress.nodeAddress;

                    memcpy(&slotInfoRecv.nodeMoIdStr, &cpmL->pCpmLocalInfo->nodeMoIdStr, sizeof(SaNameT));

                    rc = VDECL_VER(clXdrMarshallClCpmSlotInfoRecvT, 4, 0, 0)((void *)&slotInfoRecv, outMsgHandle, 0);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to write message \n"), rc);

                    found = 1;
                    break;
                }
                cpmLCount--;
                if (cpmLCount)
                {
                    rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("\n Unable to Get Node  Data \n"),
                            rc);
                    rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                            (ClCntDataHandleT *) &cpmL);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                            ("Unable to get container Node data %d\n", rc), rc);
                }
            }
            break;
        }

        default:
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Invalid flag passed.\n"));
            break;
        }
    }

    if (!found)
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                     ("Unable to find information about given entity, rc=[0x%x]\n", rc));
        goto failure;
    }

    return CL_OK;

failure:
    return rc;

}
