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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorMoIdToNodeNameTable.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * MOId to node name  live table.
 *****************************************************************************/

#include <string.h>
#include <clLogApi.h>
#include <clCorMoIdToNodeNameTable.h>
#include <clCorErrors.h>
#include <clCorClient.h>
#include <clCorUtilityApi.h>
#include <clCorPvt.h>
#include <clCorLog.h>
#include <clCpmApi.h>
#include <crc.h>

/* internal */
#include <xdrCorClientMoIdToNodeNameT.h>

ClCntHandleT nodeNameToMoIdTableHandle;
ClCntHandleT moIdToNodeNameTableHandle;

#define NODE_NAME_TO_MOID_NUM_BUCKETS 20
#define MOID_TO_NODE_NAME_NUM_BUCKETS 20

extern ClCorInitStageT    gCorInitStage;
/**
 *  Get Node Name given MoId.
 *
 *  API to get Logical NodeName given MOId. 
 *
 *
 *  @param pMoId      MOId of the Blade.
 *  @param nodeName   Node name corresponding to blade MOId
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST) If an entry is not present for given MOId<br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) if moId is NULL <br/>
 */
ClRcT _clCorMoIdToNodeNameGet(ClCorMOIdPtrT pMoId, ClNameT** nodeName)
{
    ClRcT rc;
    ClUint16T OriginalDepth;
    ClCorMOServiceIdT  svcId;
    ClCorMOClassPathT moclassPath;
    CORMOClass_h moClassH;

	if(pMoId == NULL)
		return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    /* First check if the MOId passed by the user is valid or not*/

    /* The minimum condition that must be satisfied is */
    /* taht the MO class should exist*/
    rc = clCorMoIdToMoClassPathGet(pMoId, &moclassPath);

    if(CL_OK != rc)
    	CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nCould not get MO class path from MOID", rc);

    /* Check if the MO class exists or not */
    rc = corMOClassHandleGet(&moclassPath, &moClassH);
    
    if (rc != CL_OK)
    	{
    	/* The mo class does not exist. Return*/
    	CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nIncorrect path specified. MO class does not exist\n", rc);
    	}

    /* The user has passed MOId, Since we are going to work on MOId */
    /* Passed by the user, let's store the original depth and svc Id */

    OriginalDepth = pMoId->depth;
    svcId = pMoId->svcId;

    pMoId->svcId = CL_COR_INVALID_SVC_ID;

    for(; pMoId->depth>=1; pMoId->depth--)
    	{
    	/* Find if the MOId is present in the Hash Table */
    	rc = clCntDataForKeyGet(moIdToNodeNameTableHandle, (ClCntKeyHandleT) pMoId, (ClCntDataHandleT *) nodeName);
    	if(CL_OK == rc && *nodeName !=NULL )
    		{
    		/* Found the node name for given MOId. Return.*/
    		/* Before returning restore svc Id and depth */
    		pMoId->depth = OriginalDepth;
    		pMoId->svcId = svcId;
    		return CL_OK;
    		}
    	}

    /* If we are here then the node Name could not be found*/
    /* Restore original depth and svcId and return */

    pMoId->depth = OriginalDepth;
    pMoId->svcId = svcId;

    CL_COR_RETURN_ERROR(CL_DEBUG_TRACE, "\n Node Name not found for given MOId.\n", CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST));

}

/**
 *  Get MOId given Logical NodeName.
 *
 *  API to get MOId from node name . The MOId reflects the current state of card. If a card is plugged
 * in then exact MOId is returned. If card is not plugged in MOId with wildcard cardtype is returned.
 *
 *  @param logicalNodeName [IN] the node name corresponding to blade MOId
 *  @param pMoId      [OUT]MOId of the card.
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST) If an entry is not present for given node name<br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) if node name is NULL <br/>
 */


ClRcT _clCorNodeNameToMoIdGet(ClNameT *nodeName, ClCorNodeDataPtrT *dataNode)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;
    ClNameT *tempName = NULL;

	if(nodeName == NULL)
		return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    /* Get the data from the hash table */

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" Node find for node Name  %s", nodeName->value));

    /* Do the node find and get the node handle for a particular node name. Now walk all the 
     * node for that particular node handle type.
     */
    rc = clCntNodeFind(nodeNameToMoIdTableHandle, (ClCntKeyHandleT)nodeName, &nodeHandle);   
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed to get the node Handle for the Node Name %s. rc [0x%x]", nodeName->value, rc));
        return rc;
    }

    while(nodeHandle != 0)
    {
        rc = clCntNodeUserKeyGet(nodeNameToMoIdTableHandle, nodeHandle, (ClCntKeyHandleT *) &tempName);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the user key from the node handle. rc[0x%x]", rc));
            break;
        }

        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Got the Nodename %s", tempName->value));
        rc = clCntNodeUserDataGet(nodeNameToMoIdTableHandle, nodeHandle, (ClCntDataHandleT *)dataNode);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the user data from the node handle. rc [0x%x]", rc));
            break;
        }

        if((strcmp (nodeName->value, (*dataNode)->nodeName.value)) == 0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Got the MoId for a Node Name %s", (*dataNode)->nodeName.value));
            return CL_OK;
        } 
        
        rc = clCntNextNodeGet(nodeNameToMoIdTableHandle, nodeHandle, &nodeHandle);  
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed to get the next node handle. rc[0x%x]", rc));
            break;
        }
    }

    if(CL_OK != rc)
        CL_COR_RETURN_ERROR(CL_DEBUG_TRACE, "Could not get MOId from the node name", rc);    
    return CL_OK;
}


/***************************************************************************************/
/* APIs required by hash table*/
/***************************************************************************************/
static void clCorNodeNameTableNodeNameDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{
	/* The data is moid and key is node name. Freeing it. */
    clHeapFree(((ClCorNodeDataPtrT)userData)->pMoId);
	clHeapFree((void *) userData);
	clHeapFree((void *)userKey);

}

static void clCorNodeNameTableNodeNameDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{
	/* The data is moid and key is node Name. freeing it. */
    clHeapFree(((ClCorNodeDataPtrT)userData)->pMoId);
    clHeapFree((void *)userKey);
	clHeapFree((void *) userData);
}

static ClUint32T clCorNodeNameKeyGen(ClNameT *name)
{
	ClUint32T hashVal = 0, keyLen = 0;

    if(crc((unsigned char *)name->value, name->length, &hashVal, &keyLen) != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the key using crc algorithm . "));
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Generating Key %u keyLen %u for the node Name %s", hashVal, keyLen, name->value));
    return (hashVal);
}

static ClUint32T clCorNodeNameTableNodeNameHashFn(ClCntKeyHandleT key)
{
    /* This is for node name  to MoId mapping. The key is node name */
    ClNameT *name = (ClNameT*)key;

    ClUint32T hashVal = clCorNodeNameKeyGen(name);
    return (hashVal % NODE_NAME_TO_MOID_NUM_BUCKETS);
}

static ClInt32T clCorNodeNameTableNodeNameKeyCompare(ClCntKeyHandleT key1, 
			ClCntKeyHandleT key2)
{
	ClNameT *name_x, *name_y;
	name_x = (ClNameT*)key1;
	name_y = (ClNameT*)key2;

	ClUint32T count_x = 0 , count_y = 0;

	count_x = clCorNodeNameKeyGen(name_x);
	count_y = clCorNodeNameKeyGen(name_y);
    /*The key is node name in this hash table */
    if(count_x > count_y)
        return 1;
    else if(count_x < count_y)
        return -1;
    else
        return 0;
}



static void clCorNodeNameTableMoIdDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{

	/* The key is MOId and data is node name. Just deallocate it. */
	clHeapFree((void *) userKey);
	clHeapFree((void *) userData);
}

static void clCorNodeNameTableMoIdDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{
	/* The key is MOId and data is node name. Just deallocate it. */
	clHeapFree((void *) userKey);
	clHeapFree((void *) userData);
}

static ClUint32T clCorNodeNameTableMoIdHashFn(ClCntKeyHandleT key)
{
    /* The key is MOId here */
    ClUint32T hashVal;
    ClCorMOIdPtrT pMoId= (ClCorMOIdPtrT)key;
    hashVal = pMoId->node[pMoId->depth - 1].instance;
    hashVal %= MOID_TO_NODE_NAME_NUM_BUCKETS;
    
    return hashVal;
}

static ClInt32T clCorNodeNameTableMoIdKeyCompare(ClCntKeyHandleT key1, 
			ClCntKeyHandleT key2)
{
    ClUint32T result;
    ClCorMOIdPtrT moId1 = (ClCorMOIdPtrT)key1;
    ClCorMOIdPtrT moId2 = (ClCorMOIdPtrT)key2;
	
    result = clCorMoIdCompare(moId1, moId2);
    if((result == 0) || (result == 1))
        {
		/* In case of exact match and wild card match .. return 0 */
        return 0;
        }
    else if(moId1->node[moId1->depth -1].instance > moId2->node[moId2->depth -1].instance)
        return 1;
    else
        return -1;
        
}

/***************************************************************************************/
/* APIs which interact with Hash table */
/***************************************************************************************/

/* API to create the tables that map MOId to NodeName */
/* This creates the hash tables */

ClRcT clCorMoIdNodeNameMapCreate(void)
{

    ClRcT rc;

    /* Create NODE NAME - MOID hash table */
    rc = clCntHashtblCreate(NODE_NAME_TO_MOID_NUM_BUCKETS,
                                                        clCorNodeNameTableNodeNameKeyCompare, 
                                                        clCorNodeNameTableNodeNameHashFn, 
                                                        clCorNodeNameTableNodeNameDeleteCallBack, 
                                                        clCorNodeNameTableNodeNameDestroyCallBack, 
                                                        CL_CNT_NON_UNIQUE_KEY,
                                                        &nodeNameToMoIdTableHandle);

    if(CL_OK != rc)
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Could not create Hash Table", rc);    

    clCntHashtblCreate(MOID_TO_NODE_NAME_NUM_BUCKETS,
                                                        clCorNodeNameTableMoIdKeyCompare, 
                                                        clCorNodeNameTableMoIdHashFn, 
                                                        clCorNodeNameTableMoIdDeleteCallBack, 
                                                        clCorNodeNameTableMoIdDestroyCallBack, 
                                                        CL_CNT_UNIQUE_KEY,
                                                        &moIdToNodeNameTableHandle);

    if(CL_OK != rc)
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Could not create Hash Table", rc);    

    return CL_OK;
    
}


/* This API adds an entry into the hash table for given MOId <-> NodeName map */
/* This API shall be called when a new entry is to be added in the table. This typically */
/* happens only once during the booting of the system */
ClRcT clCorMOIdNodeNameMapAdd(ClCorMOIdPtrT pMoId, ClNameT *nodeName)
{
    ClRcT rc;
    /* Add the mapping in both the hash tables */
    /* In this table key is node Name and value is MoId */
    /* allocate MoId first */
    ClCorMOIdPtrT keyMoId = clHeapAllocate(sizeof(ClCorMOIdT));
	ClNameT		  *dataNodeName  = clHeapAllocate (sizeof(ClNameT));
	ClNameT		  *keyNodeName  = clHeapAllocate (sizeof(ClNameT));
    ClCorNodeDataPtrT dataNode = clHeapAllocate(sizeof(ClCorNodeDataT));
    if(dataNode == NULL)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to allocate Node Data")); 
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM) ;
    }
    memset(&dataNode->nodeName, 0, sizeof(ClNameT));
    dataNode->pMoId = clHeapAllocate(sizeof(ClCorMOIdT));

    if( keyMoId == NULL || dataNodeName == NULL || dataNode->pMoId == NULL || keyNodeName == NULL)
    {
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    memcpy(keyMoId, pMoId, sizeof(ClCorMOIdT));
    memcpy(dataNodeName, nodeName, sizeof(ClNameT));
    memcpy(dataNode->pMoId, pMoId, sizeof(ClCorMOIdT));
    memcpy(&dataNode->nodeName, nodeName, sizeof(ClNameT));
    memcpy(keyNodeName, nodeName, sizeof(ClNameT));

    rc = clCntNodeAdd(nodeNameToMoIdTableHandle, (ClCntKeyHandleT) dataNodeName, (ClCntDataHandleT )dataNode , NULL);
    
    if(CL_OK != rc)
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Could not add node in nodeNameToMoIdHandle", rc);    
    
    /* MOID to Node Name map - In this table key is MOId and value is node name   */
    rc = clCntNodeAdd(moIdToNodeNameTableHandle, (ClCntKeyHandleT) keyMoId, (ClCntDataHandleT)keyNodeName, NULL);

    if(CL_OK != rc)
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Could not add node in moIdToNodeNameHandle", rc);    
    
    return CL_OK;
}

/* The RMD function on the server side which takes care of MOId to NodeName */
ClRcT VDECL(_corMoIdToNodeNameTableOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
	ClRcT rc;
	corClientMoIdToNodeNameT tab = {{0}};
	ClNameT		 *nodeName = NULL;
    ClCorNodeDataPtrT  dataNode = NULL;
 
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("MNT", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

	VDECL_VER(clXdrUnmarshallcorClientMoIdToNodeNameT, 4, 0, 0)(inMsgHandle, (ClUint8T*)&tab);
	
	clCorClientToServerVersionValidate(tab.version, rc);
    if(rc != CL_OK)
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);

	switch (tab.op)
		{
		case COR_MOID_TO_NODE_NAME_GET :
			rc = _clCorMoIdToNodeNameGet(&tab.moId, &nodeName);
			if(rc == CL_OK)
				tab.nodeName = *nodeName;
			else
			{
				rc = CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);
			}
			break;
		case COR_NODE_NAME_TO_MOID_GET :
			rc = _clCorNodeNameToMoIdGet(&tab.nodeName, &dataNode);
			if(CL_OK == rc)
				{
				/* Copy the MOId */
				tab.moId = *dataNode->pMoId;
				}
			else
            {
				rc = CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("For the Nodename %s no MoId found. rc[0x%x]", tab.nodeName.value, rc));
            }
			break;
		default :
			CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nInvalid operation passed\n", CL_ERR_DOESNT_EXIST);
			break;
		}

	if(CL_OK == rc)
		{
		/* Operation successfull. Copy message buffer */
		rc = VDECL_VER(clXdrMarshallcorClientMoIdToNodeNameT, 4, 0, 0)(&tab, outMsgHandle, 0);
		}

	return rc;
	
}

/* Cleanup Functions */
void clCorMoIdToNodeNameTablesCleanUp(void)
{
    	ClCntNodeHandleT  nodeH = 0;
	ClCntNodeHandleT  nextNodeH = 0;

   	 clCntFirstNodeGet(nodeNameToMoIdTableHandle, &nodeH);

	while(nodeH)
		{
		clCntNextNodeGet(nodeNameToMoIdTableHandle, nodeH, &nextNodeH);
		clCntNodeDelete(nodeNameToMoIdTableHandle, nodeH); 	
		nodeH = nextNodeH;
		}

	nodeH = 0;
   	 clCntFirstNodeGet(moIdToNodeNameTableHandle, &nodeH);

	while(nodeH)
		{
		clCntNextNodeGet(moIdToNodeNameTableHandle, nodeH, &nextNodeH);
		clCntNodeDelete(moIdToNodeNameTableHandle, nodeH); 	
		nodeH = nextNodeH;
		}
}

void clCorMoIdToNodeNameTableFinalize(void)
{
	clCorMoIdToNodeNameTablesCleanUp();
	clCntDelete(nodeNameToMoIdTableHandle);
    nodeNameToMoIdTableHandle=0;
	clCntDelete(moIdToNodeNameTableHandle);
    moIdToNodeNameTableHandle = 0;
}
