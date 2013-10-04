#include <clCpmCommon.h>
#include <clDebugApi.h>
#include <clAmsInvocation.h>
#include <clCpmLog.h>
#include "xdrClCpmLocalInfoT.h"

#define CPM_LOG_AREA_DB		"DB"
#define CPM_LOG_CTX_DB_XML	"XML"

ClCpmT *gpClCpm = NULL;

ClRcT cpmInvocationDeleteInvocation(ClInvocationT invocationId)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT cntNodeHandle = 0;
    ClCpmInvocationT *invocationData = NULL;

    clOsalMutexLock(gpClCpm->invocationMutex);

    rc = clCntNodeFind(gpClCpm->invocationTable,
                       (ClCntKeyHandleT)&invocationId, &cntNodeHandle);
    if (rc != CL_OK)
        goto withLock;

    rc = clCntNodeUserDataGet(gpClCpm->invocationTable, cntNodeHandle,
                              (ClCntDataHandleT *) &invocationData);
    if (rc != CL_OK)
        goto withLock;

    rc = clCntNodeDelete(gpClCpm->invocationTable, cntNodeHandle);
    if (rc != CL_OK)
        goto withLock;

    if( (invocationData->flags & CL_CPM_INVOCATION_DATA_COPIED))
    {
        clHeapFree(invocationData->data);
    }

    clHeapFree(invocationData);

    clOsalMutexUnlock(gpClCpm->invocationMutex);

    clLogDebug("INVOCATION", "DEL", "Deleted invocation [%#llx]", invocationId);

    return rc;

  withLock:
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return rc;
}


ClRcT cpmInvocationClearCompInvocation(SaNameT *compName)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle, nextNodeHandle;
    ClCpmInvocationT *invocationData = NULL;
    void *data = NULL;

    /*
     * Check the input parameter 
     */
    if (!compName)
    {
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Invalid parameter passed \n"),
                     CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

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

        if((data = invocationData->data))
        {
            ClUint32T matched = 0;
            if ((invocationData->flags & CL_CPM_INVOCATION_AMS))
            {
                matched = !strncmp((const ClCharT *) (((ClAmsInvocationT*) data)->compName.value), (const ClCharT *) compName->value,
                                ((ClAmsInvocationT*) data)->compName.length);
            }
            else if ((invocationData->flags & CL_CPM_INVOCATION_CPM))
            {
                matched = !strncmp((const ClCharT *) (((ClCpmComponentT*) data)->compConfig->compName), (const ClCharT *) compName->value,
                                compName->length);
            }
            
            if(matched)
            {
                clLogDebug("INVOCATION", "CLEAR", "Clearing invocation for component [%.*s] "
                           "invocation [%#llx]", compName->length, compName->value, invocationData->invocation);
                if (clCntNodeDelete(gpClCpm->invocationTable, nodeHandle) != CL_OK)
                    goto withLock;
                if( (invocationData->flags & CL_CPM_INVOCATION_DATA_COPIED) )
                    clHeapFree(data);
                clHeapFree(invocationData);
            }
        }
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            break;

        if (rc != CL_OK)
            goto withLock;

        nodeHandle = nextNodeHandle;

    }

    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return CL_OK;

    withLock:
    failure:
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return rc;
}


ClRcT cpmInvocationGetWithLock(ClInvocationT invocationId,
                               ClUint32T *cbType,
                               void **data)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT cntNodeHandle = 0;
    ClCpmInvocationT *invocationData = NULL;

    /*
     * Check the input parameter 
     */
    if ((data == NULL) || (cbType == NULL))
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Invalid parameter passed \n"),
                     CL_CPM_RC(CL_ERR_NULL_POINTER));

    rc = clCntNodeFind(gpClCpm->invocationTable,
                       (ClCntKeyHandleT)&invocationId, &cntNodeHandle);
    if (rc != CL_OK)
        goto failure;

    rc = clCntNodeUserDataGet(gpClCpm->invocationTable, cntNodeHandle,
                              (ClCntDataHandleT *) &invocationData);
    if (rc != CL_OK)
        goto failure;

    *data = invocationData->data;
    CL_CPM_INVOCATION_CB_TYPE_GET(invocationId, *cbType);

    return rc;

  failure:
    return rc;
}

ClRcT cpmInvocationGet(ClInvocationT invocationId,
                       ClUint32T *cbType,
                       void **data)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(gpClCpm->invocationMutex);
    rc = cpmInvocationGetWithLock(invocationId, cbType, data);
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return rc;
}

ClRcT cpmInvocationAdd(ClUint32T cbType,
                       void *data,
                       ClInvocationT *invocationId,
                       ClUint32T flags)
{
    ClRcT rc = CL_OK;
    ClCpmInvocationT *temp = NULL;
    ClUint32T invocationKey = 0;

    if (cbType < 0x1LL || cbType >= CL_CPM_MAX_CALLBACK || invocationId == NULL)
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Invalid parameter passed \n"),
                     CL_CPM_RC(CL_ERR_INVALID_PARAMETER));

    temp =
            (ClCpmInvocationT *) clHeapAllocate(sizeof(ClCpmInvocationT));
    if (temp == NULL)
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Unable to allocate memory \n"),
                     CL_CPM_RC(CL_ERR_NO_MEMORY));

    clOsalMutexLock(gpClCpm->invocationMutex);

    invocationKey = gpClCpm->invocationKey++;
    CL_CPM_INVOCATION_ID_GET((ClUint64T) cbType, (ClUint64T) invocationKey,
                             *invocationId);

    temp->invocation = *invocationId;
    temp->data = data;
    temp->flags = flags;

    rc = clCntNodeAdd(gpClCpm->invocationTable, (ClCntKeyHandleT)&temp->invocation,
                      (ClCntDataHandleT) temp, NULL);
    if (rc != CL_OK)
    {
        clLogError("NEW", "INVOCATION", "Invocation add for key [%#llx] returned [%#x]", 
                   temp->invocation, rc);
        goto withLock;
    }

    clLogDebug("NEW", "INVOCATION", "Added entry for invocation [%#llx]", temp->invocation);

    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return rc;

  withLock:
    clOsalMutexUnlock(gpClCpm->invocationMutex);
  failure:
    if (temp != NULL)
        clHeapFree(temp);

    return rc;
}

ClRcT cpmInvocationAddKey(ClUint32T cbType,
                          void *data,
                          ClInvocationT invocationId,
                          ClUint32T flags)
{
    ClRcT rc = CL_OK;
    ClCpmInvocationT *temp = NULL;
    ClUint32T invocationKey = 0;

    if (cbType < 0x1LL || cbType >= CL_CPM_MAX_CALLBACK)
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Invalid parameter passed \n"),
                     CL_CPM_RC(CL_ERR_INVALID_PARAMETER));
    temp =
        (ClCpmInvocationT *) clHeapAllocate(sizeof(ClCpmInvocationT));
    if (temp == NULL)
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Unable to allocate memory \n"),
                     CL_CPM_RC(CL_ERR_NO_MEMORY));

    clOsalMutexLock(gpClCpm->invocationMutex);

    temp->invocation = invocationId;
    temp->data = data;
    temp->flags = flags; 

    CL_CPM_INVOCATION_KEY_GET(invocationId, invocationKey);

    rc = clCntNodeAdd(gpClCpm->invocationTable, (ClCntKeyHandleT)&temp->invocation,
                      (ClCntDataHandleT) temp, NULL);
    if (rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
        {
            clLogWarning("ADD", "INVOCATION", "Invocation entry [%#llx] already exists", invocationId);
            rc = CL_OK;
        }
        else
        {
            clLogError("ADD", "INVOCATION", "Invocation entry [%#llx] returned [%#x]",
                       invocationId, rc);
        }
        goto withLock;
    }

    /*
     * Try preventing reuse of the invocation key for new invocations.
     */
    if(gpClCpm->invocationKey <= invocationKey)
    {
        gpClCpm->invocationKey = invocationKey + 1;
    }

    clLogDebug("ADD", "INVOCATION", "Added entry for invocation [%#llx]", invocationId);
               
    clOsalMutexUnlock(gpClCpm->invocationMutex);
    return rc;

    withLock:
    clOsalMutexUnlock(gpClCpm->invocationMutex);

    failure:
    if (temp != NULL)
        clHeapFree(temp);

    return rc;
}

ClRcT cpmNodeFindLocked(SaUint8T *name, ClCpmLT **cpmL)
{
    ClUint16T nodeKey = 0;
    ClCntNodeHandleT hNode = 0;
    ClCpmLT *tempNode = NULL;
    ClUint32T rc = CL_OK, numNode=0;

    rc = clCksm16bitCompute((ClUint8T *) name, strlen((const ClCharT*)name), &nodeKey);
    CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_CNT_CKSM_ERR, name, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clCntNodeFind(gpClCpm->cpmTable, (ClPtrT)(ClWordT)nodeKey, &hNode);
    CL_CPM_CHECK_3(CL_LOG_SEV_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, "node",
                   name, rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clCntKeySizeGet(gpClCpm->cpmTable, (ClPtrT)(ClWordT)nodeKey,
                         &numNode);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_CNT_KEY_SIZE_GET_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    while (numNode > 0)
    {
        rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                                  (ClCntDataHandleT *) &tempNode);
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR,
                       rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        if (!strcmp(tempNode->nodeName, (const ClCharT*)name))
        {
            *cpmL = tempNode;
            goto done;
        }
        if(--numNode)
        {
            rc = clCntNextNodeGet(gpClCpm->cpmTable,hNode,&hNode);
            if(rc != CL_OK)
            {
                break;
            }
        }
    }

    rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    CL_CPM_CHECK_3(CL_LOG_SEV_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "node", name, rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

done:
    return CL_OK;

failure:
    return rc;
}

/*
 * Called with the cpmTableMutex lock held.
 */
ClRcT cpmNodeFind(SaUint8T *name, ClCpmLT **cpmL)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(name, cpmL);
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    return rc;
}

ClUint32T cpmNodeFindByNodeId(ClUint32T nodeId, ClCpmLT **cpmL)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT cpmNode = 0;
    ClUint32T cpmLCount = 0;
    ClCpmLT *tempCpmL = NULL;
    ClUint32T found = 0;

    cpmLCount = gpClCpm->noOfCpm;
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL && cpmLCount != 0)
    {
        rc = clCntFirstNodeGet(gpClCpm->cpmTable, &cpmNode);
        CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
                       "CPM-L", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        while (cpmLCount)
        {
            rc = clCntNodeUserDataGet(gpClCpm->cpmTable, cpmNode,
                                      (ClCntDataHandleT *) &tempCpmL);
            CL_CPM_CHECK_1(CL_LOG_SEV_ERROR,
                           CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (tempCpmL->pCpmLocalInfo)
            {
                if (tempCpmL->pCpmLocalInfo->nodeId == nodeId)
                {
                    *cpmL = tempCpmL;
                    found = 1;
                    break;
                }
            }
            cpmLCount--;

            if (cpmLCount)
            {
                rc = clCntNextNodeGet(gpClCpm->cpmTable, cpmNode, &cpmNode);
                CL_CPM_CHECK_2(CL_LOG_SEV_ERROR,
                               CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR, "CPM-L", rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }
    if(found == 1)
        return CL_OK;
    else
        return CL_CPM_RC(CL_ERR_DOESNT_EXIST);

  failure:
    *cpmL = NULL;
    return rc;
}

ClRcT cpmPrintDBXML(FILE *fp)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT cpmNode = 0;
    ClUint32T cpmLCount = 0;
    ClCpmLT *cpmL = NULL;

    fprintf(fp,"<cpm>\n");

    /*
     * Walk through the cpm table to find and display SCs 
     */
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    cpmLCount = gpClCpm->noOfCpm;
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL && cpmLCount != 0)
    {
        rc = clCntFirstNodeGet(gpClCpm->cpmTable, &cpmNode);
        if(rc != CL_OK)
        {
            clLogError(CPM_LOG_AREA_DB,CPM_LOG_CTX_DB_XML,CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR, "CPM-L", rc);
            clLogWrite((CL_LOG_HANDLE_APP), (CL_LOG_DEBUG), NULL, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR, ("CPM-L"), (rc));
            goto out_unlock;
        }

        while (cpmLCount)
        {
            rc = clCntNodeUserDataGet(gpClCpm->cpmTable, cpmNode,
                                    (ClCntDataHandleT *) &cpmL);

            if(rc != CL_OK)
            {
                clLogError(CPM_LOG_AREA_DB,CPM_LOG_CTX_DB_XML,CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc);
                clLogWrite((CL_LOG_HANDLE_APP), (CL_LOG_DEBUG), NULL, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc);
                goto out_unlock;
            }

            if (cpmL->pCpmLocalInfo)
            {
                /*
                 * If the node is a SC, display its data 
                 */
                if(!strcmp((const ClCharT *)cpmL->nodeType.value, (const ClCharT *)gpClCpm->pCpmLocalInfo->nodeType.value)
                    || cpmL->pCpmLocalInfo->nodeId == gpClCpm->activeMasterNodeId
                    || cpmL->pCpmLocalInfo->nodeId == gpClCpm->deputyNodeId)
                {
                    fprintf(fp,"<node value=\"%s\">\n",cpmL->pCpmLocalInfo->nodeName);
                    fprintf(fp,"<id value=\"%d\"/>\n",cpmL->pCpmLocalInfo->nodeId);
                    fprintf(fp,"<ha_state value=\"%s\"/>\n",cpmL->pCpmLocalInfo->nodeId == gpClCpm->activeMasterNodeId ? "active" : "standby");
                    fprintf(fp,"</node>\n");
                }
            }

            cpmLCount--;

            if (cpmLCount)
            {
                rc = clCntNextNodeGet(gpClCpm->cpmTable, cpmNode, &cpmNode);
                if(rc != CL_OK)
                {
                    clLogError(CPM_LOG_AREA_DB,CPM_LOG_CTX_DB_XML,CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR, "CPM-L", rc);
                    clLogWrite((CL_LOG_HANDLE_APP), (CL_LOG_DEBUG), NULL, CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR, ("CPM-L"), (rc));
                    goto out_unlock;
                }
            }
        }
    }

    rc = CL_OK;
    out_unlock:
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    fprintf(fp,"</cpm>\n");
    return rc;
}
