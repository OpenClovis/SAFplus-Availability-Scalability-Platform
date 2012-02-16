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
 * ModuleName  : gms
 * File        : clGmsDb.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#include <string.h>
#include <clGmsDb.h>
#include <clCntApi.h>
#include <clGmsErrors.h>
#include <clCntErrors.h>
#include <clGms.h>

/* Track Stuff */

static ClGmsDbTrackParamsT    trackDbParams =  {
        {   /* Track hash table params */
            CL_GMS_MAX_NUM_OF_BUCKETS,
            _clGmsTrackHashCallback,
            _clGmsTrackKeyCompareCallback,
            _clGmsTrackDeleteCallback,
            _clGmsTrackDestroyCallback
        }
};

/* Cluster & group Stuff */

static const ClGmsDbViewParamsT   viewDbParams = {

        {   /* Cluster Hash table params */
            CL_GMS_MAX_NUM_OF_BUCKETS,
            _clGmsViewHashCallback,
            _clGmsViewKeyCompareCallback,
            _clGmsViewDeleteCallback,
            _clGmsViewDestroyCallback
        },

        &trackDbParams
};


/* FIXME:
 */

ClRcT _clGmsDbOpen(
            CL_IN      const    ClUint64T   numOfGroups,
            CL_INOUT   ClGmsDbT** const     gmsDb)
{
    if (gmsDb == NULL) 
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    
    *gmsDb = (ClGmsDbT *) clHeapAllocate(sizeof(ClGmsDbT)
                                        * numOfGroups);

    if (*gmsDb == NULL)
    {
        return CL_GMS_RC(CL_ERR_NO_MEMORY);
    }

    memset(*gmsDb, 0, sizeof(ClGmsDbT)* numOfGroups);

    clLog(DBG,GEN,DB,
            "Created GMS master database successfully");

    return CL_OK;
}


/*  FIXME:
 */
ClRcT   _clGmsDbCreate(
           CL_IN        ClGmsDbT*  const      gmsDb,
           CL_OUT       ClGmsDbT** const      gmsElement)
{
    ClRcT       rc = CL_OK;
    ClUint32T   i = 0;
    ClUint64T   cntIndex = 0;

    if ((gmsDb == NULL) || (gmsElement == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    for(i = 0; i < gmsGlobalInfo.config.noOfGroups; i++)
    {
        if (gmsDb[i].view.isActive == CL_FALSE)
        {
            cntIndex = i;
            break;
        }
    }
    if (i == gmsGlobalInfo.config.noOfGroups)
    {
        return CL_ERR_OUT_OF_RANGE;
    }

    /* Current view database. Holds cluster and groups info */


    rc = clCntHashtblCreate(
                      viewDbParams.htbleParams.gmsNumOfBuckets, 
                      viewDbParams.htbleParams.gmsHashKeyCompareCallback, 
                      viewDbParams.htbleParams.gmsHashCallback,
                      viewDbParams.htbleParams.gmsHashDeleteCallback,
                      viewDbParams.htbleParams.gmsHashDestroyCallback,
                      CL_CNT_UNIQUE_KEY, 
                      &gmsDb[cntIndex].htbl[CL_GMS_CURRENT_VIEW]);

    if (rc != CL_OK)
    {
        return rc;
    }

    /* Cluster joined/left view list. Used for tracking */

    rc = clCntHashtblCreate(
                      viewDbParams.htbleParams.gmsNumOfBuckets, 
                      viewDbParams.htbleParams.gmsHashKeyCompareCallback, 
                      viewDbParams.htbleParams.gmsHashCallback,
                      viewDbParams.htbleParams.gmsHashDeleteCallback,
                      viewDbParams.htbleParams.gmsHashDestroyCallback,
                      CL_CNT_UNIQUE_KEY, 
                      &gmsDb[cntIndex].htbl[CL_GMS_JOIN_LEFT_VIEW]);

    if (rc != CL_OK)
    {
        return rc;
    }

    /* Track hash table create */

    rc = clCntHashtblCreate(
                viewDbParams.trackDbParams->htbleParams.gmsNumOfBuckets, 
                viewDbParams.trackDbParams->htbleParams.
                                                 gmsHashKeyCompareCallback, 
                viewDbParams.trackDbParams->htbleParams.gmsHashCallback,
                viewDbParams.trackDbParams->htbleParams.
                                                     gmsHashDeleteCallback,
                viewDbParams.trackDbParams->htbleParams.
                                                        gmsHashDestroyCallback,
                CL_CNT_UNIQUE_KEY, 
                &gmsDb[cntIndex].htbl[CL_GMS_TRACK]);

    if (rc != CL_OK)
    {
        return rc;
    }

    clGmsMutexCreate(&gmsDb[cntIndex].viewMutex);


    clGmsMutexCreate(&gmsDb[cntIndex].trackMutex);


    gmsDb[cntIndex].view.isActive = CL_TRUE;

    clLog(DBG,GEN,DB,
            "Created View and Track DB for GroupId [%lld]",cntIndex);

    *gmsElement = &gmsDb[cntIndex++];

    return rc;
}


ClRcT   _clGmsDbCreateOnIndex(
           CL_IN        ClUint64T             cntIndex,
           CL_IN        ClGmsDbT*  const      gmsDb,
           CL_OUT       ClGmsDbT** const      gmsElement)
{
    ClRcT       rc = CL_OK;

    if ((gmsDb == NULL) || (gmsElement == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    if (cntIndex >= gmsGlobalInfo.config.noOfGroups)
    {
        return CL_ERR_OUT_OF_RANGE;
    }

    /* Current view database. Holds cluster and groups info */


    rc = clCntHashtblCreate(
                      viewDbParams.htbleParams.gmsNumOfBuckets, 
                      viewDbParams.htbleParams.gmsHashKeyCompareCallback, 
                      viewDbParams.htbleParams.gmsHashCallback,
                      viewDbParams.htbleParams.gmsHashDeleteCallback,
                      viewDbParams.htbleParams.gmsHashDestroyCallback,
                      CL_CNT_UNIQUE_KEY, 
                      &gmsDb[cntIndex].htbl[CL_GMS_CURRENT_VIEW]);

    if (rc != CL_OK)
    {
        return rc;
    }

    /* Cluster joined/left view list. Used for tracking */

    rc = clCntHashtblCreate(
                      viewDbParams.htbleParams.gmsNumOfBuckets, 
                      viewDbParams.htbleParams.gmsHashKeyCompareCallback, 
                      viewDbParams.htbleParams.gmsHashCallback,
                      viewDbParams.htbleParams.gmsHashDeleteCallback,
                      viewDbParams.htbleParams.gmsHashDestroyCallback,
                      CL_CNT_UNIQUE_KEY, 
                      &gmsDb[cntIndex].htbl[CL_GMS_JOIN_LEFT_VIEW]);

    if (rc != CL_OK)
    {
        return rc;
    }

    /* Track hash table create */

    rc = clCntHashtblCreate(
                viewDbParams.trackDbParams->htbleParams.gmsNumOfBuckets, 
                viewDbParams.trackDbParams->htbleParams.
                                                 gmsHashKeyCompareCallback, 
                viewDbParams.trackDbParams->htbleParams.gmsHashCallback,
                viewDbParams.trackDbParams->htbleParams.
                                                     gmsHashDeleteCallback,
                viewDbParams.trackDbParams->htbleParams.
                                                        gmsHashDestroyCallback,
                CL_CNT_UNIQUE_KEY, 
                &gmsDb[cntIndex].htbl[CL_GMS_TRACK]);

    if (rc != CL_OK)
    {
        return rc;
    }

    clGmsMutexCreate(&gmsDb[cntIndex].viewMutex);


    clGmsMutexCreate(&gmsDb[cntIndex].trackMutex);


    gmsDb[cntIndex].view.isActive = CL_TRUE;
    clLog(DBG,GEN,DB,
            "Created View and Track DB for GroupId [%lld]",cntIndex);
    *gmsElement = &gmsDb[cntIndex++];

    return rc;
}
/* FIXME:
 */

ClRcT   _clGmsDbAdd(
            CL_IN  const ClGmsDbT* const  gmsDb,
            CL_IN  const  ClGmsDbTypeT    type,
           /* Suppressing coverity warning for pass by value with below comment */
           // coverity[pass_by_value]
            CL_IN  const  ClGmsDbKeyT     key,
            CL_IN  const  void*    const  data)
{
    ClRcT   rc = CL_OK;
    ClCntKeyHandleT cntKey = NULL;

    if (gmsDb == (const void *)NULL)
        return CL_ERR_NULL_POINTER;

    rc = _clGmsDbGetKey(type, key, &cntKey);
   
    if (rc != CL_OK) return rc;

    rc = clCntNodeAdd(gmsDb->htbl[type], cntKey, (ClCntDataHandleT)data, NULL);

    return rc;
}


/* FIXME:
 */

ClRcT _clGmsDbDelete(
           CL_IN    const ClGmsDbT* const  gmsDb,
           CL_IN    const ClGmsDbTypeT     type,
           /* Suppressing coverity warning for pass by value with below comment */
           // coverity[pass_by_value]
           CL_IN    const ClGmsDbKeyT      key)
{
    ClRcT   rc = CL_OK;
    ClCntKeyHandleT cntKey = NULL;

    if (gmsDb == (const void *)NULL)
        return CL_ERR_NULL_POINTER;

    rc = _clGmsDbGetKey(type, key, &cntKey);

    if (rc != CL_OK) return rc;

    rc = clCntAllNodesForKeyDelete(gmsDb->htbl[type], cntKey);

    return rc;

}

ClRcT  _clGmsDbDeleteAll(
            CL_IN   const ClGmsDbT* const  gmsDb,
            CL_IN   const ClGmsDbTypeT     type)
{
    ClRcT   rc = CL_OK;

    if (gmsDb == (const void *)NULL)
        return CL_ERR_NULL_POINTER;

    rc = clCntAllNodesDelete(gmsDb->htbl[type]);

    return rc;
}

/* FIXME:
 */

ClRcT   _clGmsDbDestroy(
           CL_IN    ClGmsDbT*  const gmsDb)
{
    ClRcT   rc = CL_OK;

    if (gmsDb == (const void*)NULL)
        return CL_ERR_NULL_POINTER;

    rc = clCntAllNodesDelete(gmsDb->htbl[CL_GMS_CURRENT_VIEW]);
    if (rc != CL_OK) 
    {
        return rc; 
    }

    rc = clCntAllNodesDelete(gmsDb->htbl[CL_GMS_JOIN_LEFT_VIEW]);
    if (rc != CL_OK) 
    {
        return rc; 
    }

    rc = clCntAllNodesDelete(gmsDb->htbl[CL_GMS_TRACK]);
    if (rc != CL_OK) 
    {
        return rc; 
    }

    /* Delete the containers */
    rc = clCntDelete((gmsDb->htbl[CL_GMS_CURRENT_VIEW]));
    CL_ASSERT(rc == CL_OK);

    rc = clCntDelete((gmsDb->htbl[CL_GMS_JOIN_LEFT_VIEW]));
    CL_ASSERT(rc == CL_OK);

    rc = clCntDelete((gmsDb->htbl[CL_GMS_TRACK]));
    CL_ASSERT(rc == CL_OK);

    /* Delete the mutex */
    clGmsMutexDelete(gmsDb->viewMutex);

    clGmsMutexDelete(gmsDb->trackMutex);

    memset(gmsDb,0,sizeof(ClGmsDbT));

    return rc;
}


/* FIXME:
 */

ClRcT _clGmsDbClose(
           CL_IN    ClGmsDbT*  const  gmsDb,
           CL_IN    const ClUint64T   numOfGroups)
{
    ClRcT   rc = CL_OK;
    ClInt32T   i = 0x0;

    if (gmsDb == NULL)
        return CL_ERR_NULL_POINTER;

    for (i=0; i < numOfGroups; i++) 
    {
       rc =_clGmsDbDestroy(&gmsDb[i]); 

       if (rc != CL_OK) return rc;

       rc = clOsalMutexDelete(gmsDb[i].viewMutex);
       if (rc != CL_OK) return rc;

       rc = clOsalMutexDelete(gmsDb[i].trackMutex);
       if (rc != CL_OK) return rc;
    }
        
    clHeapFree((void*)gmsDb);

    return rc;
}


/*  FIXME:
 */


ClRcT _clGmsDbFind(
           CL_IN    const ClGmsDbT*  const  gmsDb,
           CL_IN    const ClGmsDbTypeT     type,
           /* Suppressing coverity warning for pass by value with below comment */
           // coverity[pass_by_value]
           CL_IN    const ClGmsDbKeyT      key,
           CL_INOUT void** const           data)
{    
    ClRcT   rc = CL_OK;
    ClCntKeyHandleT cntKey = NULL;
    ClCntNodeHandleT nodeData = NULL;

    if (gmsDb == (const void *)NULL)
        return CL_ERR_NULL_POINTER;
    
    rc = _clGmsDbGetKey(type, key, &cntKey);

    if (rc != CL_OK) return rc;

    rc = clCntNodeFind(gmsDb->htbl[type], cntKey,
                       (ClCntNodeHandleT*) &nodeData);

    if (rc != CL_OK) return rc;

    rc = clCntNodeUserDataGet((ClCntHandleT) gmsDb->htbl[type],
                                    nodeData, (ClCntDataHandleT*)data);
    return rc;
}

/* FIXME:
 */

ClRcT  _clGmsDbGetFirst(
            CL_IN    const ClGmsDbT* const    gmsDb,
            CL_IN    const ClGmsDbTypeT       type,
            CL_OUT   ClCntNodeHandleT** const gmsOpaque,
            CL_INOUT void** const             data)
{
    ClRcT   rc = CL_OK;

    /* Get the first node which is an internal pointer
     * to the container data structure.
     */
    if ((gmsDb == (const void *)NULL) || (data == NULL) || (gmsOpaque == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    rc = clCntFirstNodeGet((ClCntHandleT) gmsDb->htbl[type],
                            (ClCntNodeHandleT*) gmsOpaque);

    if (rc ==  CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        *data = NULL;
        return CL_OK;
    }

    if (rc != CL_OK) return rc;

    /* Get the real data 
     */

    rc = clCntNodeUserDataGet((ClCntHandleT) gmsDb->htbl[type],
                                    (ClCntNodeHandleT)*gmsOpaque,
                                    (ClCntDataHandleT*)(data));

    if (rc ==  CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        *data = NULL;
        return CL_OK;
    }

    return rc;
}

/* FIXME:
 */

ClRcT   _clGmsDbGetNext(
            CL_IN       const ClGmsDbT*    const   gmsDb,
            CL_IN       const ClGmsDbTypeT         type,
            CL_INOUT    ClCntNodeHandleT** const   gmsOpaque,
            CL_OUT      void**             const   data)
{
    ClRcT   rc = CL_OK;
    ClCntNodeHandleT    *gmsOldOpaque = NULL;   

    if ((gmsDb == (const void *)NULL) || (data == NULL) || (gmsOpaque == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    gmsOldOpaque = *gmsOpaque;

    rc = clCntNextNodeGet(gmsDb->htbl[type], (ClCntNodeHandleT) gmsOldOpaque, 
                                                (ClCntNodeHandleT*)gmsOpaque);

    if (rc ==  CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        *data = NULL;
        return CL_OK;
    }

    if (rc != CL_OK) return rc;

    rc = clCntNodeUserDataGet((ClCntHandleT) gmsDb->htbl[type],
                                         (ClCntNodeHandleT)*gmsOpaque,
                                            (ClCntDataHandleT*)(data));

    if (rc ==  CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        *data = NULL;
        return CL_OK;
    }

    return rc;
}

ClRcT   _clGmsDbIndexGet (
        CL_IN        ClGmsDbT*  const      gmsDb,
        CL_IN        ClGmsDbT**  const      gmsElement,
        CL_OUT       ClUint32T*            index)
{
        ClUint32T            i = 0;

        for (i = 0; i < gmsGlobalInfo.config.noOfGroups; i++)
        {
            if ((*gmsElement) == (&gmsDb[i]))
            {
                *index = i;
                return CL_OK;
            }
        }

        return CL_ERR_NOT_EXIST;
}

ClRcT   _clGmsNameIdDbCreate (ClCntHandleT *dbPtr)
{
    ClRcT   rc = CL_OK;

    if (dbPtr == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    /* Create the container */
    rc = clCntHashtblCreate(
            viewDbParams.htbleParams.gmsNumOfBuckets,
            viewDbParams.htbleParams.gmsHashKeyCompareCallback,
            viewDbParams.htbleParams.gmsHashCallback,
            viewDbParams.htbleParams.gmsHashDeleteCallback,
            viewDbParams.htbleParams.gmsHashDestroyCallback,
            CL_CNT_UNIQUE_KEY,
            dbPtr);

    return rc;
}

ClRcT   _clGmsNameIdDbAdd (ClCntHandleT  *dbPtr,
                           ClNameT       *name,
                           ClGmsGroupIdT  id)
{
    ClRcT   rc = CL_OK;
    ClGmsDbKeyT key = {{0}};
    ClGmsNodeIdPairT  *node = NULL;
    ClCntKeyHandleT cntKey = NULL;

    if ((dbPtr == NULL) || (name == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    memcpy(&key.name, name, sizeof(ClNameT));
    rc = _clGmsDbGetKey(CL_GMS_NAME_ID_DB, key, &cntKey);

    if (rc != CL_OK)
    {
        return rc;
    }

    node = (ClGmsNodeIdPairT*)clHeapAllocate(sizeof(ClGmsNodeIdPairT));
    if (node == NULL)
    {
        return CL_ERR_NO_MEMORY;
    }
    memcpy(&node->name, name, sizeof(ClNameT));
    node->groupId = id;

    rc = clCntNodeAdd(*dbPtr, cntKey, (ClCntDataHandleT)node, NULL);
    if (rc != CL_OK)
    {
        clHeapFree(node);
    }
    return rc;
}



ClRcT   _clGmsNameIdDbDelete(ClCntHandleT  *dbPtr,
                             ClNameT       *name)
{
    ClRcT   rc = CL_OK;
    ClGmsDbKeyT key = {{0}};
    ClCntKeyHandleT cntKey = NULL;

    if (dbPtr == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    memcpy(&key.name, name, sizeof(ClNameT));
    rc = _clGmsDbGetKey(CL_GMS_NAME_ID_DB, key, &cntKey);

    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clCntAllNodesForKeyDelete(*dbPtr, cntKey);
    return rc;
}


ClRcT   _clGmsNameIdDbDeleteAll(ClCntHandleT  *dbPtr)
{
    ClRcT   rc = CL_OK;

    if (dbPtr == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    rc = clCntAllNodesDelete(*dbPtr);

    return rc;
}


ClRcT   _clGmsNameIdDbDestroy (ClCntHandleT  *dbPtr)
{
    ClRcT   rc = CL_OK;

    if (dbPtr == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }
    rc = clCntAllNodesDelete(*dbPtr);

    return rc;
}


ClRcT   _clGmsNameIdDbFind(ClCntHandleT  *dbPtr,
                           ClNameT       *name,
                           ClGmsGroupIdT *id)
{
    ClRcT   rc = CL_OK;
    ClGmsDbKeyT key = {{0}};
    ClCntKeyHandleT cntKey = NULL;
    ClCntNodeHandleT nodeData = NULL;
    ClGmsNodeIdPairT*   node = NULL;

    if (dbPtr == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    memcpy(&key.name, name, sizeof(ClNameT));
    rc = _clGmsDbGetKey(CL_GMS_NAME_ID_DB, key, &cntKey);

    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clCntNodeFind(*dbPtr, cntKey, (ClCntNodeHandleT*) &nodeData);

    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clCntNodeUserDataGet(*dbPtr,
            nodeData, (ClCntDataHandleT*)&node);

    if (rc != CL_OK)
    {
        return rc;
    }

    *id =node->groupId;
    return rc;
}
