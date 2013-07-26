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

/*
 * Standard header files 
 */
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * ASP header files 
 */
#include <clCommonErrors.h>

#include <clCpmApi.h>
#include <clCkptApi.h>
#include <clCkptExtApi.h>
#include <ipi/clCkptIpi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clHandleApi.h>
#include <clList.h>
/*
 * CPM internal header files 
 */
#include <clCpmCkpt.h>
#include <clCpmLog.h>
#include <clCpmMgmt.h>
#include <clClmApi.h>

ClRcT cpmCpmLCheckpointWrite(void);

static ClRcT cpmCkptReplicaChangeCallback(const ClNameT *pCkptName, ClIocNodeAddressT replicaAddr)
{
    ClRcT rc = CL_OK;
    ClCpmLT *pCpmL = NULL;
    ClGmsClusterNotificationBufferT notifyBuff = {0};
    if(!pCkptName || !replicaAddr) return CL_OK;
    if(replicaAddr == clIocLocalAddressGet())
        return CL_OK;
    clOsalMutexLock(&gpClCpm->cpmMutex);
    if(gpClCpm->haState != CL_AMS_HA_STATE_ACTIVE)
    {
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        return CL_OK;
    }
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindByNodeId(replicaAddr, &pCpmL);
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    if (CL_OK != rc)
    {
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        clLogWarning("REP", "CHG", "Replica change event received for ckpt [%.*s], node [%#x] "
                     "thats not present in the cluster.", 
                     pCkptName->length, pCkptName->value, replicaAddr);
        return rc;
    }
    /*
     * Last phase. Check if its present in the GMS view.
     */
    rc = clGmsClusterTrack(gpClCpm->cpmGmsHdl, CL_GMS_TRACK_CURRENT, &notifyBuff);
    if(rc == CL_OK)
    {
        ClUint32T i;
        ClBoolT found = CL_FALSE;
        /*
         * Check for the presence of the replica in the GMS view.
         */
        for(i = 0; i < notifyBuff.numberOfItems; ++i)
        {
            clLogNotice("REP", "CHG", "Processing view member [%#x]", 
                        notifyBuff.notification[i].clusterNode.nodeAddress.iocPhyAddress.nodeAddress);
            if(notifyBuff.notification[i].clusterNode.nodeAddress.iocPhyAddress.nodeAddress == replicaAddr)
            {
                found = CL_TRUE;
                break;
            }
        }
        if(notifyBuff.notification)
            clHeapFree(notifyBuff.notification);
        if(!found)
        {
            clOsalMutexUnlock(&gpClCpm->cpmMutex);
            clLogWarning("REP", "CHG", "Replica change event received for ckpt [%.*s], node [%#x] "
                         "thats not present in the GMS view.", 
                         pCkptName->length, pCkptName->value, replicaAddr);
            return CL_CPM_RC(CL_ERR_NOT_EXIST);
        }
    }
    clOsalMutexUnlock(&gpClCpm->cpmMutex);
    clLogNotice("REP", "CHG", "Allowing replica change for ckpt [%.*s], node [%#x] present in the cluster", 
                pCkptName->length, pCkptName->value, replicaAddr);
    return CL_OK;
}

ClRcT cpmCkptCpmLDatsSet(void)
{
    return cpmCpmLCheckpointWrite();
}

static ClRcT cpmCpmLSerializer(ClInt8T **data, ClUint32T *size)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT message = 0;
    ClUint32T msgLength = 0;
    ClCpmTlvT tlv = { 0 };
    ClCharT *tmpStr = NULL;
    ClUint32T cpmLCount = 0;
    ClCpmLT *cpmL = NULL;
    ClCntNodeHandleT cpmNode = 0;
    ClVersionT version = {.releaseCode = CL_RELEASE_VERSION,
                          .majorVersion = CL_MAJOR_VERSION,
                          .minorVersion = CL_MINOR_VERSION
    };

    rc = clBufferCreate(&message);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    /*
     * Validate the iinput parameters 
     */
    if (data == NULL || size == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    clXdrMarshallClVersionT(&version, message, 0);

    /*Get the CPM\Ls if cpm\G->NOT endian safe but lucky because we dont yet support mixed
     *endian CPMGs
     */
    
    cpmLCount = gpClCpm->noOfCpm;
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL && cpmLCount != 0)
    {
        rc = clCntFirstNodeGet(gpClCpm->cpmTable, &cpmNode);
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
                       "CPM-L", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        while (cpmLCount)
        {
            rc = clCntNodeUserDataGet(gpClCpm->cpmTable, cpmNode,
                                      (ClCntDataHandleT *) &cpmL);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                           CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (cpmL->pCpmLocalInfo)
            {
                tlv.LENGTH = sizeof(ClCpmLocalInfoT);
                tlv.TYPE = CL_CPM_CPML;

                rc = clBufferNBytesWrite(message, (ClUint8T *) &tlv,
                                                sizeof(ClCpmTlvT));
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

                rc = clBufferNBytesWrite(message,
                                                (ClUint8T *) (cpmL->
                                                              pCpmLocalInfo),
                                                sizeof(ClCpmLocalInfoT));
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
            cpmLCount--;
            if (cpmLCount)
            {
                rc = clCntNextNodeGet(gpClCpm->cpmTable, cpmNode, &cpmNode);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                               CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR, "CPM-L", rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }
    rc = clBufferLengthGet(message, &msgLength);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    tmpStr = (ClCharT *) clHeapAllocate(msgLength);
    if (tmpStr == NULL)
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clBufferNBytesRead(message, (ClUint8T *) tmpStr, &msgLength);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    *data = (ClAddrT) tmpStr;
    *size = msgLength;

    clBufferDelete(&message);
    return rc;

  failure:
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
               CL_CPM_LOG_2_CKPT_DATASET_WRITE_ERR, "CPM-L", rc);
    clBufferDelete(&message);
    return rc;
}

static ClRcT cpmCpmLDeSerializerBaseVersion(ClBufferHandleT message, ClUint32T size)
{
    ClRcT rc = CL_OK;
    ClUint32T tempLength = size;
    ClCpmTlvT tlv = { 0 };
    ClUint32T tlvLength = sizeof(ClCpmTlvT);
    ClCpmLT *cpmL = NULL;
    CL_LIST_HEAD_DECLARE(dynamicNodeList);
    typedef struct ClCpmDynamicNode
    {
        ClListHeadT node;
        ClCpmLocalInfoT cpmLocalInfo;
    }ClCpmDynamicNodeT;

    /*
     * Start reading up the data. All the data packed in the TLV format 
     * Type | Length | data, in this case it will have tuple per cpm\L
     * Type should be equal to CL_CPM_CPML
     * Length Should be equal to size of ClCpmLocalInfoT
     */

    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
    {
        rc = clBufferNBytesRead(message, (ClUint8T *) &tlv, &tlvLength);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        tempLength -= sizeof(ClCpmTlvT);

        if (tlv.TYPE != CL_CPM_CPML)
        {
            rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
            goto failure;
        }

        while (tlv.TYPE == CL_CPM_CPML && tempLength != 0)
        {
            ClCpmLocalInfoT cpmLBuffer = {{0}};
            if (tlv.LENGTH != sizeof(ClCpmLocalInfoT))
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
                goto failure;
            }

            rc = clBufferNBytesRead(message, (ClUint8T *) &cpmLBuffer,
                                    &(tlv.LENGTH));
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            tempLength -= tlv.LENGTH;

            clOsalMutexLock(gpClCpm->cpmTableMutex);
            rc = cpmNodeFindLocked(cpmLBuffer.nodeName, &cpmL);
            if(CL_OK != rc)
            {
                clOsalMutexUnlock(gpClCpm->cpmTableMutex);

                if (CL_ERR_DOESNT_EXIST == CL_GET_ERROR_CODE(rc) 
                    ||
                    CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
                {
                    ClCpmDynamicNodeT *dynamicNode = clHeapCalloc(1, sizeof(*dynamicNode));
                    CL_ASSERT(dynamicNode != NULL);
                    rc = CL_OK;
                    memcpy(&dynamicNode->cpmLocalInfo, &cpmLBuffer, sizeof(dynamicNode->cpmLocalInfo));
                    clListAddTail(&dynamicNode->node, &dynamicNodeList);
                    goto next_tlv;
                }
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP,
                           "Unable to find node [%s], error [%#x]",
                           cpmLBuffer.nodeName,
                           rc);
                goto failure;
            }
                
            cpmL->pCpmLocalInfo =
                (ClCpmLocalInfoT *) clHeapAllocate(sizeof(ClCpmLocalInfoT));
            CL_ASSERT(cpmL->pCpmLocalInfo != NULL);

            /*
             * Copy the received checkpoint data 
             */
            memcpy(cpmL->pCpmLocalInfo, &cpmLBuffer, sizeof(ClCpmLocalInfoT));
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            
            next_tlv:

            if (tempLength != 0)
            {
                rc = clBufferNBytesRead(message, (ClUint8T *) &tlv,
                                        &tlvLength);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                tempLength -= sizeof(ClCpmTlvT);
            }
        }
    }

    clBufferDelete(&message);

    /*
     * Now add the dynamic nodes seen to the CPMs view once we have all the slots
     * read as cpmNodeAdd does a slotinfoget which could fail if the master slot wasn't
     * added
     */
    while(!CL_LIST_HEAD_EMPTY(&dynamicNodeList))
    {
        ClCpmDynamicNodeT *node = CL_LIST_ENTRY(dynamicNodeList.pNext, ClCpmDynamicNodeT, node);
        ClCpmLT *cpmL = NULL;
        clListDel(&node->node);
        rc = cpmNodeAdd(node->cpmLocalInfo.nodeName);
        if(rc != CL_OK)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP,
                       "Node [%s] add to CPM returned [%#x]",
                       node->cpmLocalInfo.nodeName, rc);
            clHeapFree(node);
            continue;
        }
        else
        {
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP, "Node [%s] added to CPM", node->cpmLocalInfo.nodeName);
        }
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindLocked(node->cpmLocalInfo.nodeName, &cpmL);
        if(rc == CL_OK)
        {
            cpmL->pCpmLocalInfo = clHeapCalloc(1, sizeof(*cpmL->pCpmLocalInfo));
            CL_ASSERT(cpmL->pCpmLocalInfo != NULL);
            memcpy(cpmL->pCpmLocalInfo, &node->cpmLocalInfo, sizeof(*cpmL->pCpmLocalInfo));
        }
        else
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP,
                       "Node [%s] cpmlocalinfo update skipped",
                       node->cpmLocalInfo.nodeName);
        }
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clHeapFree(node);
    }

    return CL_OK;

    failure:
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
               CL_CPM_LOG_1_CKPT_CONSUME_ERR, rc);
    clBufferDelete(&message);
    return rc;
}

static ClRcT cpmCpmLDeSerializer(ClInt8T *data, ClUint32T size)
{
    ClRcT rc = CL_OK;
    ClUint32T msgLength = 100;
    ClUint32T tempLength = 0;
    ClBufferHandleT message = 0;
    ClVersionT version = {0};

    rc = clBufferCreate(&message);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clBufferNBytesWrite(message, (ClUint8T *) data, size);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clBufferLengthGet(message, &msgLength);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_LENGTH_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if(msgLength <= sizeof(ClVersionT))
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_LENGTH_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    rc = clXdrUnmarshallClVersionT(message, &version);
    if(rc != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP, "Failed unmarshall the version from buffer message. error code [0x%x].", rc);
        goto failure;
    }

    clBufferReadOffsetGet(message, &tempLength);

    msgLength -= tempLength;

    switch(CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion))
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION):
        return cpmCpmLDeSerializerBaseVersion(message, msgLength);

    default: 
        rc = CL_CPM_RC(CL_ERR_VERSION_MISMATCH);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP,
                   "Data deserialize of version [%d.%d.%d] not supported",
                   version.releaseCode, version.majorVersion, version.minorVersion);
        break;
    }

    failure:
    if(message) clBufferDelete(&message);

    return rc;
}

ClVersionT cpmCkptVersion = {'B', 0x01, 0x01};

#define CPM_CKPT_SIZE   1024*1024
#define CPM_CKPT_MAX_SECTION_SIZE   1024*1024
#define CPM_CKPT_MAX_SECTION_ID_SIZE 256
#define CPM_CKPT_RETENTION_DURATION   0
#define CPM_CKPT_MAX_SECTIONS   1

ClRcT cpmCpmLStandbyCheckpointInitialize(void)
{
    ClRcT   rc = CL_OK;
    ClCkptSvcHdlT handle;
    ClCkptCallbacksT   ckptCallbacks;
    ClCkptOpenFlagsT flags = CL_CKPT_CHECKPOINT_CREATE | CL_CKPT_CHECKPOINT_READ;
    ClTimeT time = 0;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};

    ClCkptCheckpointCreationAttributesT ckptAttributes =
    {
        CL_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_ALL_OPEN_ARE_REPLICAS, /*CL_CKPT_WR_ACTIVE_REPLICA,*/
        CPM_CKPT_SIZE,
        CPM_CKPT_RETENTION_DURATION,
        CPM_CKPT_MAX_SECTIONS,
        CPM_CKPT_MAX_SECTION_SIZE,
        CPM_CKPT_MAX_SECTION_ID_SIZE
    };

    if(gpClCpm->ckptOpenHandle != CL_HANDLE_INVALID_VALUE)
    {
        clLogWarning("CKP", "INI", "Standby checkpoint already initialized. Skipping initialization");
        return CL_OK;
    }

    ckptCallbacks.checkpointOpenCallback = NULL;
    ckptCallbacks.checkpointSynchronizeCallback= NULL;

    do
    {
        rc = clCkptInitialize(&handle, &ckptCallbacks, &cpmCkptVersion);
    } while( rc != CL_OK && tries++ < 3 && clOsalTaskDelay(delay) == CL_OK);
    
    if(rc != CL_OK)
    {
        clLogError("CKP", "INI", "CKPT initialize for standby failed with [%#x]", rc);
        goto failure;
    }

    gpClCpm->ckptHandle = handle;

    clCkptReplicaChangeRegister(cpmCkptReplicaChangeCallback);

    if ((rc = clCkptCheckpointOpen(gpClCpm->ckptHandle,
                                   &gpClCpm->ckptCpmLName,
                                   &ckptAttributes,
                                   flags,
                                   time,
                                   &handle)) != CL_OK)
        goto failure;
    gpClCpm->ckptOpenHandle = handle;

    return CL_OK;
failure:
    return rc;
}

ClRcT cpmCpmLActiveCheckpointInitialize(void)
{
    ClRcT   rc = CL_OK;
    ClCkptSvcHdlT handle = 0;
    ClCkptCallbacksT   ckptCallbacks = {0};
    ClCkptOpenFlagsT flags = CL_CKPT_CHECKPOINT_CREATE | CL_CKPT_CHECKPOINT_READ | CL_CKPT_CHECKPOINT_WRITE;
    ClCkptHdlT  ckptHandle = 0;
    ClTimeT time = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T tries = 0;
    ClCkptCheckpointCreationAttributesT ckptAttributes =
    {
        CL_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_ALL_OPEN_ARE_REPLICAS,
        CPM_CKPT_SIZE,
        CPM_CKPT_RETENTION_DURATION,
        CPM_CKPT_MAX_SECTIONS,
        CPM_CKPT_MAX_SECTION_SIZE,
        CPM_CKPT_MAX_SECTION_ID_SIZE
    };

    if(gpClCpm->ckptOpenHandle != CL_HANDLE_INVALID_VALUE)
    {
        clLogWarning("CKP", "INI", "Active checkpoint already initialized. Skipping initialization");
        return CL_OK;
    }

    ckptCallbacks.checkpointOpenCallback = NULL;
    ckptCallbacks.checkpointSynchronizeCallback= NULL;
    
    do
    {
        rc = clCkptInitialize(&handle, &ckptCallbacks, &cpmCkptVersion);
    } while(rc != CL_OK && tries++ < 3 && clOsalTaskDelay(delay) == CL_OK);

    if(rc != CL_OK)
    {
        clLogError("CKP", "INI", "CKPT initialize for active returned [%#x]", rc);
        goto failure;
    }

    gpClCpm->ckptHandle = handle;

    clCkptReplicaChangeRegister(cpmCkptReplicaChangeCallback);

    if ((rc = clCkptCheckpointOpen(handle,
                                   &gpClCpm->ckptCpmLName,
                                   &ckptAttributes,
                                   flags,
                                   time,
                                   &ckptHandle)) != CL_OK)
      goto failure;

    gpClCpm->ckptOpenHandle = ckptHandle;

    rc = clCkptActiveReplicaSet(gpClCpm->ckptOpenHandle);
    if (CL_OK != rc)
    {
        goto failure;
    }

    return rc;

failure:
    return rc;
}

ClRcT cpmCpmLCheckpointRead(void)
{
    ClRcT rc = CL_OK;
    ClCkptIOVectorElementT   *ioVector=NULL;
    ClUint32T erroneousVectorIndex;
    ClCkptSectionIdT sectionId = CL_CKPT_DEFAULT_SECTION_ID;

    if ((rc = clCkptActiveReplicaSetSwitchOver(gpClCpm->ckptOpenHandle)) != CL_OK)
        goto failure;
    
    /*
     * Read the cpm/L DB section
     */
    ioVector = 
        (ClCkptIOVectorElementT *)clHeapCalloc(1,sizeof(ClCkptIOVectorElementT));

    if (ioVector == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        goto failure;
    }
    ioVector->sectionId = sectionId;
    ioVector->dataSize = CPM_CKPT_MAX_SECTION_SIZE;
    ioVector->dataOffset=0;

    if ((rc = clCkptCheckpointRead(gpClCpm->ckptOpenHandle,
                                   ioVector,
                                   1,
                                   &erroneousVectorIndex)) != CL_OK)
        goto failure;
    
    /*
     * Consume the checkpoints
     */
    if ((rc = cpmCpmLDeSerializer(ioVector->dataBuffer, ioVector->dataSize)) != CL_OK)
        goto failure;

    if (ioVector->dataBuffer)
    {
        clHeapFree(ioVector->dataBuffer);
    }
    clHeapFree(ioVector);

    return CL_OK;

failure:
    if (ioVector)
    {
        if (ioVector->dataBuffer)
        {
            clHeapFree(ioVector->dataBuffer);
        }
        clHeapFree(ioVector);
    }
    return rc;
}

ClRcT cpmCpmLCheckpointWrite(void)
{
    ClRcT rc = CL_OK;
    ClInt8T *readData = NULL;
    ClUint32T dataSize = 0;
    ClCkptSectionIdT sectionId = CL_CKPT_DEFAULT_SECTION_ID;

    /* CKPT Server is not yet initialized */
    if (gpClCpm->ckptHandle == CL_HANDLE_INVALID_VALUE)
        return CL_OK;
        
    /* If it is already initialized */
    /*
     * Generate the Checkpointed data
     */
    
    if((rc = cpmCpmLSerializer(&readData, &dataSize)) != CL_OK)
        goto failure;
    
    if ((rc = clCkptSectionOverwrite(gpClCpm->ckptOpenHandle,
                                     &sectionId,
                                     (ClUint8T *)readData,
                                     dataSize)) != CL_OK)
        goto failure;

    clHeapFree(readData);
    
    return CL_OK;
failure:
    if(readData)
        clHeapFree(readData);

    return rc;
}


