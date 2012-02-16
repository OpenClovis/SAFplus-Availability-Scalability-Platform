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
 * ModuleName  : cor
 * File        : clCorDeltaSave.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This modules implements Delta Save.
 *****************************************************************************/

#include <unistd.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorErrors.h>
#include <clDbalApi.h>
#include <netinet/in.h>
#include <clCorApi.h>
#include <clLogApi.h>
#include <clCorUtilityApi.h>
#include <clCpmApi.h>
#include <clCorErrors.h>
#include <sys/time.h>

/* internal */
#include "clCorDeltaSave.h"
#include "clCorVector.h"
#include "clCorDmDefs.h"
#include "clCorTreeDefs.h"
#include "clCorDefs.h"
#include "clCorDmProtoType.h"
#include "clCorObj.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorRt.h"

#include "xdrClCorMOIdT.h"
#include "xdrClCorAttrPathT.h"

#define clCorDeltaDbObjInfoRestore() clCntWalk(gCorDeltaSaveKeyTable, clCorCreateObjFromCont, 0, 0 ) 
#define CL_COR_DELTA_SAVE_ATTR_BUFFER_SIZE 2000

static ClCntHandleT gCorDeltaSaveKeyTable ;
extern ClUint32T gClCorSaveType;
extern CORVector_t  routeVector;
extern ClUint16T   nRouteConfig;
extern _ClCorServerMutexT		gCorMutexes;
extern _ClCorOpDelayT   gCorOpDelay;
extern ClCharT gClCorPersistDBPath[];

ClDBHandleT hDeltaObjDb, hDeltaRouteDb;

/* Version related definitions */
ClVersionT gCorDbVersionT[] = {{CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION}};
ClVersionDatabaseT gCorDbVersionDb = {1, gCorDbVersionT}; 

/* internal api declarations */
ClRcT clCorCreateObjFromCont( ClCntKeyHandleT userKey,  ClCntDataHandleT userData, ClCntArgHandleT userArg, ClUint32T dataLength) ;
ClRcT clCorDeltaDbOperate (CORAttr_h attrH, Byte_h* pBuf) ;

/***************************************************************************************/
/* APIs required by hash table*/
/***************************************************************************************/
static void clCorDeltaSaveTableDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{
    clDbalKeyFree(hDeltaObjDb, (ClDBKeyT)userKey);
	clDbalRecordFree(hDeltaObjDb, (ClDBRecordT) ((ClCorDeltaContDataT *)userData)->data);
	clHeapFree((void *)userData);
}

static void clCorDeltaSaveTableDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
{
    clDbalKeyFree(hDeltaObjDb, (ClDBKeyT)userKey);
	clDbalRecordFree(hDeltaObjDb, (ClDBRecordT) ((ClCorDeltaContDataT *)userData)->data);
	clHeapFree((void *)userData);
}

static ClInt32T clCorDeltaSaveTableKeyCompare(ClCntKeyHandleT key1, 
                                              ClCntKeyHandleT key2)
{
	ClCorDeltaSaveKeyT *entry_x, *entry_y;
	entry_x = (ClCorDeltaSaveKeyT *)key1;
	entry_y = (ClCorDeltaSaveKeyT *)key2;

    if(entry_x->moId.depth != entry_y->moId.depth)
        return entry_x->moId.depth - entry_y->moId.depth;

    if(entry_x->flag != entry_y->flag)
        return entry_x->flag - entry_y->flag;

	if(clCorMoIdCompare(&entry_x->moId, &entry_y->moId) == 0)
	{
		if(clCorAttrPathCompare(&entry_x->attrPath, &entry_y->attrPath) == 0)
			return 0;
		else
			return 1;
	}

    return 1;
}


/***************************************************************************************/
/* APIs required  For Object DB Restoration
***************************************************************************************/


/*
 * This API adds the key and data into the container class. This API is called by  clCorDeltaObjDbCreateCont()
 * Pure MOID "create-operations" are placed as per depth. MSOs create operation are placed
 * at "depth + 1", and MO/MSO set operations are placed at depth+2.
 */

static ClRcT clCorDeltaDbContAdd(ClCorDeltaSaveKeyT *deltaDbKey, ClDBRecordT deltaDbData, ClUint32T dataSize)
{
	ClRcT	rc = CL_OK;
	ClCorDeltaSaveKeyT *tempKey = deltaDbKey;
	ClCorDeltaContDataT *tempCont = clHeapCalloc(1, sizeof(ClCorDeltaContDataT));

    CL_ASSERT(tempCont != NULL);

    tempCont->data = deltaDbData;
    tempCont->dataSize = dataSize;

    if( tempKey == NULL || tempCont->data == NULL)
    {
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    rc = clCntNodeAdd(gCorDeltaSaveKeyTable, (ClCntKeyHandleT) tempKey, (ClCntDataHandleT) tempCont , NULL);
    if(CL_OK != rc)
    {
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
            rc = CL_OK;
        else
            CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Could not add node in the Delta Save Key Table", rc);    

        clHeapFree(tempCont);
    }  
	
	return rc;
}

/* 
 * This API creates the container class and populates it with  data from the obj deltaDb
 * The API iterates over the database, fetching the key and associated data.
 * It  calls clCorDeltaDbContAdd which places MO/MSO create/set operating in the hash list.
 */
ClRcT clCorDeltaDbObjContCreate()
{
	ClRcT rc = CL_OK;
	ClCorDeltaSaveKeyT *deltaDbKey = NULL, *deltaDbKeyNext = NULL;
	ClUint32T keySize = 0, keySizeNext = 0;
	ClDBRecordT deltaDbData, deltaDbDataNext ;
	ClUint32T dataSize = 0;
    ClVersionT dbVersion = {0};

   /* Create the container for retreiving data from the Delta DB */
	if ((rc = clCorDeltaContInit()) != CL_OK)
        return rc ;

	if((rc = clDbalFirstRecordGet( hDeltaObjDb,
								  (ClDBKeyT *)&deltaDbKey, 
								  (ClUint32T *)&keySize, 
								  (ClDBRecordT *)&deltaDbData, 
								  (ClUint32T *)&dataSize )) != CL_OK)
	{
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            clLog(CL_LOG_SEV_NOTICE, "DLS", "OBR", "The object Db doesn't \
                    contain anything. rc[0x%x]", rc);
            return CL_OK;
        }

		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the first Record for the Key. rc [0x%x]",rc));
		return rc;
	}
	
    dbVersion = deltaDbKey->version;

    clLogInfo("DB", "INT", "Obj DB version restored [0x%x.0x%x.0x%x]",
            dbVersion.releaseCode, dbVersion.majorVersion, dbVersion.minorVersion);

	clCorDbVersionValidate(dbVersion, rc);
	if(rc != CL_OK)
    {
        clLogError("DB", "INT", "Cor persistent db version is unsupported. "
                "Version of the db is [0x%x.0x%x.0x%x], but the max version supported is [0x%x.0x%x.0x%x]",
                deltaDbKey->version.releaseCode, deltaDbKey->version.majorVersion, deltaDbKey->version.minorVersion,
                dbVersion.releaseCode, dbVersion.majorVersion, dbVersion.minorVersion);
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
    }

	while(1)
	{
        ClCorClassTypeT classId = 0;
        CORClass_h classH = NULL;
        ClBoolT classExists = CL_FALSE;
        ClCorMOClassPathT moClassPath = {{0}};

        /*
         * Check whether the class information is present in cor before restoring 
         * the object information.
         */
        if (deltaDbKey->moId.svcId == CL_COR_INVALID_SVC_ID)
        {
            classId = deltaDbKey->moId.node[deltaDbKey->moId.depth - 1].type;
            classH = dmClassGet(classId);
            if (classH != NULL)
                classExists = CL_TRUE;
        }
        else 
        {
            clCorMoIdToMoClassPathGet(&deltaDbKey->moId, &moClassPath);
            rc = corMOTreeClassGet(&moClassPath, deltaDbKey->moId.svcId, &classId);
            if (rc == CL_OK)
                classExists = CL_TRUE;
        }

        if (classExists == CL_TRUE)
        {
            /*
             * Update the class id base for dynamic additions later.
             */
            clCorClassConfigCorClassIdUpdate(classId);
            /* keep on storing the keys in the cont only if the class is present */
            if(( rc = clCorDeltaDbContAdd(deltaDbKey, deltaDbData, dataSize)) != CL_OK)
            {	
                clDbalKeyFree(hDeltaObjDb, (ClDBKeyT) deltaDbKey);
                clDbalRecordFree(hDeltaObjDb, (ClDBRecordT)deltaDbData); 
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to add the value in the container from delta Db. rc[0x%x]", rc));
                break;
            }
        }

		if((rc = clDbalNextRecordGet( hDeltaObjDb,
									(ClDBKeyT )deltaDbKey,
 									keySize,
									(ClDBKeyT *) &deltaDbKeyNext,
									(ClUint32T *) &keySizeNext,
									(ClDBRecordT *)&deltaDbDataNext,
									&dataSize)) != CL_OK)
		{
			rc = CL_OK;
			break;	
		}
		deltaDbKey = deltaDbKeyNext;
        deltaDbData = deltaDbDataNext ;
		keySize = keySizeNext;
	}
	return rc;
}

/*
 * This API creates the container class
 */
ClRcT clCorDeltaContInit()
{
	ClRcT rc = CL_OK;

    rc = clCntRbtreeCreate(clCorDeltaSaveTableKeyCompare,
                           clCorDeltaSaveTableDeleteCallBack,
                           clCorDeltaSaveTableDestroyCallBack,
                           CL_CNT_UNIQUE_KEY,
                           &gCorDeltaSaveKeyTable);

    if(CL_OK != rc)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not create Hash Table. rc[0x%x]", rc));    

	return rc;
}



/***************************************************************************************/
/* APIs required  For  Route Db  Restoration
***************************************************************************************/
static ClRcT clCorDeltaDbRouteInfoRestore()
{
    return CL_OK;

#if 0
	ClRcT rc = CL_OK;
	ClCorDeltaRouteSaveKeyT *deltaDbKey = NULL, *deltaDbKeyNext = NULL;
	ClUint32T keySize, keySizeNext;
	ClDBRecordT deltaDbData, deltaDbDataNext ;
	ClUint32T dataSize;
	corRouteApiInfo_t *rmInfo;
    ClVersionT      dbVersion = {0};
	
	if((rc = clDbalFirstRecordGet( hDeltaRouteDb,
								  (ClDBKeyT *)&deltaDbKey, 
								  (ClUint32T *)&keySize, 
								  (ClDBRecordT *)&deltaDbData, 
								  (ClUint32T *)&dataSize )) != CL_OK)
	{
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            clLog(CL_LOG_SEV_NOTICE, "DLS", "RDB", "The route Db doesn't contain anything. rc[0x%x]", rc);
            return CL_OK;
        }

		CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed to get the first Record for the Key. rc [0x%x]",rc));
		return rc;
	}
	
    /* Copy the version information */
    dbVersion = deltaDbKey->version;

    clLogInfo("RES", "ROU", "Route DB version restored [0x%x.0x%x.0x%x]",
            dbVersion.releaseCode, dbVersion.majorVersion, dbVersion.minorVersion);

	clCorDbVersionValidate(deltaDbKey->version, rc);
	if(rc != CL_OK)
    {
        clLogError("DB", "INT", "Cor persistent db version is unsupported. "
                "Version of the db is [0x%x.0x%x.0x%x], but the max version supported is [0x%x.0x%x.0x%x]",
                dbVersion.releaseCode, dbVersion.majorVersion, dbVersion.minorVersion,
                deltaDbKey->version.releaseCode, deltaDbKey->version.majorVersion, deltaDbKey->version.minorVersion);
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
    }

	while(1)
	{
		/* keep on storing the keys in the route Vector  */
		rmInfo = (corRouteApiInfo_t *)deltaDbData;
      	rc = rmRouteAdd(&rmInfo->moId, rmInfo->addr, rmInfo->status);

		if(rc != CL_OK)
		{
			clDbalKeyFree(hDeltaRouteDb, (ClDBKeyT) deltaDbKey);
		    clDbalRecordFree(hDeltaRouteDb, (ClDBRecordT)deltaDbData); 
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to restore RouteInfo: route Addition Failed. rc[0x%x]", rc));
			break;
		}

        if(rmInfo->primaryOI)
        {
            rc = _clCorPrimaryOISet(rmInfo);
        }

		if((rc = clDbalNextRecordGet( hDeltaRouteDb,
									(ClDBKeyT )deltaDbKey,
									keySize,
									(ClDBKeyT *) &deltaDbKeyNext,
									(ClUint32T *) &keySizeNext,
									(ClDBRecordT *)&deltaDbDataNext,
									&dataSize)) != CL_OK)
		{
			clDbalKeyFree(hDeltaRouteDb, (ClDBKeyT) deltaDbKey);
		    clDbalRecordFree(hDeltaRouteDb, (ClDBRecordT)deltaDbData); 
			rc = CL_OK;
			break;	
		}
		clDbalKeyFree(hDeltaRouteDb, (ClDBKeyT) deltaDbKey);
		clDbalRecordFree(hDeltaRouteDb, (ClDBRecordT)deltaDbData); 

		deltaDbKey = deltaDbKeyNext;
		keySize = keySizeNext;
        deltaDbData = deltaDbDataNext ;
	}

	return rc;
#endif

}


/*
 *
 * clCorDeltaDbRestore
 * 
 * This Api restores the db by -
 * 1. walk the db and stores all the keys in the order. A hash is creted from the key, and the key+data are inserted.
 * 2. Read the keys from the container and replay the db. The key is MOID+ATTRPATH+FLAG, the data depends on the key.
 * 3. From route db recreates the route list.
 * 
 */
ClRcT clCorDeltaDbRestore()
{
	ClRcT	rc = CL_OK;

#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif
	
	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		clLogNotice("DLS", "RES", "Delta Save option is not enabled, so skipping the restoration from Db.");
		return CL_OK;
	}

   /*  store the obj delta DB  data in the container class */
	if ((rc = clCorDeltaDbObjContCreate())  != CL_OK)
    {
       return (rc) ;
    }

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[0], NULL);
#endif

    rc = clCorDeltaDbObjInfoRestore() ;
    if(CL_OK != rc)
    {
        clLogError("DLS", "RES", "Failed while restoring the object information. rc[0x%x]", rc);
        return rc;
    }

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[1], NULL);
    gCorOpDelay.objTreeRestoreDb = (delay[1].tv_sec * 1000000 + delay[1].tv_usec)  - 
                                    (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[0], NULL);
#endif

    rc = clCorDeltaDbRouteInfoRestore() ;
    if(CL_OK != rc)
    {
        clLogError("DLS", "RES", "Failed while restoring the route information. rc[0x%x]", rc);
        return rc;
    }

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[1], NULL);
    gCorOpDelay.routeListRestoreDb = (delay[1].tv_sec * 1000000 + delay[1].tv_usec)  - 
                                    (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

	/* Dbal and container read done. Freeing the memory used by the container */

	clCorDeltaSaveTablesCleanUp();

    clLogNotice("DLS", "RES", CL_LOG_MESSAGE_1_DELTA_DB_RESTORE_SUCCESS);
	return rc;
}

/* Cleanup Functions */
void clCorDeltaSaveTablesCleanUp(void)
{
  	ClCntNodeHandleT  nodeH = 0;
	ClCntNodeHandleT  nextNodeH = 0;

   	 clCntFirstNodeGet(gCorDeltaSaveKeyTable, &nodeH);

	while(nodeH)
	{
		clCntNextNodeGet(gCorDeltaSaveKeyTable, nodeH, &nextNodeH);
		clCntNodeDelete(gCorDeltaSaveKeyTable, nodeH); 	
		nodeH = nextNodeH;
	}
	clCntDelete(gCorDeltaSaveKeyTable);
	gCorDeltaSaveKeyTable = 0;
}


/*
 * This API is used to copy the buffer obtained from Db to the DM. While copying,
 * the buffer of the containment attribute remains intact, only the simple and array
 * attributes are memcopied.
 */
ClRcT clCorDbBufToDmBufCopy(CORAttr_h attrH, Byte_h* pBuf)
{
	ClRcT	rc = CL_OK;
	ClInt32T size;

	typedef struct dmDbBuf{
		Byte_h *dmBuf;
		Byte_h *contBuf;	
		ClUint32T *dataSize;
	}dmDbBuf;	
		
	dmDbBuf	*walkInfo = (dmDbBuf *)pBuf;

    if( (pBuf == NULL) || (*pBuf == NULL) || (attrH == NULL) )
    {
        clLogError("DLS", "RES", "The pointer passed is null ");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
	
	size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

	if(size < 0)
	{
        clLogError("DLS", "RES", "The buffer size for attribute [0x%x] is [%d]",
                attrH->attrId, size);
		return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_RELATION);
	}

    if(!(attrH->userFlags & CL_COR_ATTR_CACHED))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Attribute is non cached ... need not do any thing... "));
        return CL_OK;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Flag is [0x%x], attrId [0x%x], size %d ", attrH->userFlags, attrH->attrId, size));

	if(*walkInfo->dataSize <= 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(" got the size ... *walkInfo->dataSize %d", *walkInfo->dataSize));
		return CL_COR_SET_RC(CL_ERR_UNSPECIFIED);
    }
		
    if(attrH->userFlags & CL_COR_ATTR_PERSISTENT)
	{
		memcpy(*walkInfo->dmBuf, *walkInfo->contBuf, size);
	}	 

	*walkInfo->dmBuf += size;
	*walkInfo->contBuf += size;
	*walkInfo->dataSize -=size;

	return rc;
}

/*
 * This API is called while walking the hash table corresponding to the Obj Delta Db.
 * Based on the key, the API performs an MO/MSO create or MO/MSO set. Note that while
 * performing set operating on an MO/MSO, it is assumed that it is created. This is due
 * due to the fact, that the MO/MSO create/set operations are sorted by depth and flag.
 * For setting an MO/MSO, the api obtains DM obj buffer handle corresponding to the attrlist
 * and copies the data into it.
 */
ClRcT clCorCreateObjFromCont (CL_IN ClCntKeyHandleT userKey, CL_IN  ClCntDataHandleT userData,
						CL_INOUT ClCntArgHandleT userArg, CL_IN ClUint32T dataLength)
{
	ClRcT	rc = CL_OK;
	ClCorDeltaSaveKeyT *deltaContKey = (ClCorDeltaSaveKeyT *)userKey;
	ClCorDeltaContDataT *contData = (ClCorDeltaContDataT *) userData;

	/* For create we need not open the dbal and read it as the key in the container contain 
	 * enough info.
     */
	if ( deltaContKey->flag == CL_COR_DELTA_MSO_CREATE )
	{
			_corMSOObjCreate(&deltaContKey->moId, deltaContKey->moId.svcId);
				
	}
	else if (deltaContKey->flag == CL_COR_DELTA_MO_CREATE )
	{
		_corMOObjCreate(&deltaContKey->moId);
	}
	else if(deltaContKey->flag == CL_COR_DELTA_SET)
	{
   		DMContObjHandle_t objContHandle;
		CORClass_h class;
		Byte_h obj, buf;
		DMObjHandle_h dmH = clHeapAllocate(sizeof(DMObjHandle_t));
	
		if(dmH == NULL)
			return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
		
		if((rc = moId2DMHandle(&deltaContKey->moId, dmH)) != CL_OK)
		{
			clHeapFree(dmH);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get dm handle from moId. rc [0x%x]", rc));
			return rc;
		}

   		objContHandle.dmObjHandle.classId = dmH->classId;
   		objContHandle.dmObjHandle.instanceId = dmH->instanceId;

		if(deltaContKey->attrPath.depth != 0)
	   		objContHandle.pAttrPath  = &deltaContKey->attrPath;
		else
	   		objContHandle.pAttrPath  = NULL;
			
    	if(CL_OK == (rc = dmObjectAttrHandleGet(&objContHandle, &class,(void **)&obj)))
   		{
			if(obj && (class->size != 0))
			{
                clLogTrace("DB", "RESTORE", "Restoring an object of classId [0x%x], size : [%d]",
                        class->classId, class->size);

				struct dmDbBuf{
					Byte_h *dmBuf;
					Byte_h *contBuf;	
					ClUint32T *dataSize;
				} walkBuf;	
				ClUint32T size = contData->dataSize;
				Byte_h tempBuf = contData->data;

				buf = (Byte_h) obj + sizeof(CORInstanceHdr_t);
				walkBuf.dmBuf = (Byte_h *)&buf;
				walkBuf.contBuf = (Byte_h *)&tempBuf;
				walkBuf.dataSize = &size;

                if (size != class->size)
                {
                    clLogWarning("DB", "RESTORE", "Class size in the db mismatches with the size in COR.");
                }

				rc = dmClassAttrWalk(class, 0, clCorDbBufToDmBufCopy, (Byte_h *) &walkBuf);
                if(rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while reading from the Db..,\
                        rc[0x%x], classId [0x%x], Currsize[0x%x]", rc, dmH->classId, class->size));
                }
			}
			else
			{
				CL_DEBUG_PRINT(CL_DEBUG_TRACE,("clCorCreateObjFromCont: Object buffer is NULL "));
			}		
    	}
		
		clHeapFree(dmH);
	}
		
	return rc;
}

/*
 * This API opens the objDB and route DB.
*/
ClRcT clCorDeltaDbsOpen(ClDBTypeT deltaDbFlag)
{
	ClRcT rc = CL_OK, ret = CL_OK;
	ClNameT	nodeName = {0};
    ClCharT *dbFileName = NULL; 
    ClUint32T    sizeofFileName = 0;
    FILE* file = NULL;

	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Save Type is not configured as Delta Save"));
		return CL_OK;
	}

   /*
   * Unique Name for file is required for the simulation environment. In the SIM environment
   * both master and slave run on the same Node, hence the DB names will be identical. In order
   * to distinguish this. We need to create fileName as per the nodeName
   */
	rc = clCpmLocalNodeNameGet(&nodeName);
	if(rc != CL_OK)
	{
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to name the Delta Db file: NodeName Get failed 0x%x",rc));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_1_DB_FILENAME_CREATE, rc);
         return(rc) ;
	}

    sizeofFileName = strlen(gClCorPersistDBPath) + sizeof("/") + nodeName.length + sizeof(".corRouteDb") + 1; /* 1 byte for the NULL character */

	dbFileName = clHeapAllocate(sizeofFileName);
    if (dbFileName == NULL)
    {
        clLogError("INI", "DLS", "Failed to allocate memory");
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

	memset(dbFileName , 0 , sizeofFileName);

    strcpy(dbFileName, gClCorPersistDBPath);
    strcat(dbFileName, "/");
    strncat(dbFileName, nodeName.value , nodeName.length);
	strcat(dbFileName, ".corRouteDb");

    clLogInfo("INI", "DLS", "Creating DB File Name : %s, size of file name : %d", dbFileName, sizeofFileName);

    file = fopen(dbFileName,"rb");
	if(!file)
		ret = CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);
    else
        fclose(file);
	
    if (CL_OK !=  (rc  =  clDbalOpen( dbFileName, dbFileName, deltaDbFlag, 
								sizeof(ClCorDeltaRouteSaveKeyT), sizeof(corRouteApiInfo_t), &hDeltaRouteDb)))
    {
        clHeapFree(dbFileName); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Create DB [RC %x]", rc));
        return CL_ERR_NOT_EXIST;
     }

    memset(dbFileName,0,sizeofFileName);

    strcpy(dbFileName, gClCorPersistDBPath);
    strcat(dbFileName, "/");
    strncat(dbFileName, nodeName.value, nodeName.length);
	strcat(dbFileName, ".corObjDb");

    clLogInfo("INI", "DLS", "Creating DB File Name : %s, size of file name : %d", dbFileName, sizeofFileName);

    /*
     * TODO: Walk the MO class tree and determine max size of all the classes. This should come instead of 2000.
     * Ths number is arbitary.
     */
    if (CL_OK !=  (rc  =  clDbalOpen( dbFileName, dbFileName, deltaDbFlag, 
								sizeof(ClCorDeltaSaveKeyT), CL_COR_DELTA_SAVE_ATTR_BUFFER_SIZE, &hDeltaObjDb)))
    {
        clDbalClose(hDeltaRouteDb) ;
        clHeapFree(dbFileName);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Create DB [RC %x]", rc));
        return CL_ERR_NOT_EXIST;
     }

     clHeapFree(dbFileName);
     return ret ;
}

/*
 * This APIs close the obj and route Db
*/
ClRcT clCorDeltaDbsClose()
{
	ClRcT	rc = CL_OK;

	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Save Type is not configured as Delta Save"));
		return CL_OK;
	}


	if (CL_OK != (rc = clDbalClose(hDeltaRouteDb))) 
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Close Route DB"));
     	return rc;
   	}


	if (CL_OK != (rc = clDbalClose(hDeltaObjDb))) 
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Close Obj DB"));
     	return rc;
	}

	return CL_OK ;
}

/***************************************************************************************/
/* APIs required  For Object DB Updating.
***************************************************************************************/


/* Obtain the DM object buffer and its size corresponding MoId+AttrPath  */
                                    // coverity[pass_by_value]
static ClRcT clCorDmClassAttrBufGet( CL_IN ClCorMOIdT moId,
                                     CL_IN ClCorAttrPathPtrT attrPath,
                                     CL_OUT Byte_h *pBuf, 
                                     CL_OUT ClUint32T *size)
{
	ClRcT rc = CL_OK;
    DMContObjHandle_t objContHandle;
	CORClass_h class;
	Byte_h obj, buf;
	DMObjHandle_h dmH = clHeapAllocate(sizeof(DMObjHandle_t));
    ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};

	if(dmH == NULL)
		return CL_ERR_NO_MEMORY;

	if((rc = moId2DMHandle(&moId, dmH)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get dm Handle from moId. rc[0x%x]",rc));
        clHeapFree(dmH);
		return rc;
	}

    objContHandle.dmObjHandle.classId = dmH->classId;
    objContHandle.dmObjHandle.instanceId = dmH->instanceId;
    objContHandle.pAttrPath  = attrPath;
	
    if(CL_OK == (rc = dmObjectAttrHandleGet(&objContHandle, &class,(void **)&obj)))
    {
		buf = (Byte_h) obj + sizeof(CORInstanceHdr_t) ;
        if(class->size == 0)
          *size = CL_COR_DFLT_CLASS_SIZE;
        else
            *size = class->size;

        clLogTrace("DLS", "BGT", "Storing an object of class [0x%x], classSize [%d] into dbs.",
                class->classId, *size);

    	*pBuf = (Byte_h) clHeapAllocate(*size);
        if(NULL == *pBuf)
        {
            clLogError("DLS", "BGT", "Failed while allocating memory for the object buffer of MO[%s]", 
                    _clCorMoIdStrGet(&moId, moIdStr));
            clHeapFree(dmH);
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        if (class->size != 0)
    		memcpy(*pBuf, buf, *size);
    }

    clHeapFree(dmH);
    return (rc);
}

/* 
 * clCorDeltaDbDelete
 *
 * This api deletes the Record corresponnding to MO/MSO/MO+Attr List (The MO set operation)
 * This is called from the Attribute Walk Callback function.
 */

                                // coverity[pass_by_value]
static ClRcT clCorDeltaDbDelete(CL_IN ClCorMOIdT moId , 
                                CL_IN ClCorAttrPathPtrT pAttrPath, 
                                CL_IN ClInt32T flag)
{
	ClRcT rc = CL_OK;
	ClUint32T keySize = sizeof(ClCorDeltaSaveKeyT);
    ClCorDeltaSaveKeyT deltaDbKey;

    memset (&deltaDbKey,0,sizeof(deltaDbKey));
	CL_COR_VERSION_SET(deltaDbKey.version);
	deltaDbKey.moId = moId;
	if(pAttrPath != NULL)
		deltaDbKey.attrPath = *pAttrPath;
	else
		clCorAttrPathInitialize(&deltaDbKey.attrPath);
	deltaDbKey.flag = flag;

    clOsalMutexLock(gCorMutexes.gCorDeltaObjDbMutex);
	if((rc = clDbalRecordDelete( hDeltaObjDb, 
							 (ClDBKeyT) &deltaDbKey, 
							 keySize)) != CL_OK)
	{
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Entry is not present in the Db. rc[0x%x]", rc));
            rc = CL_OK;
        }
        else
		    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the entry from the dbal. rc[0x%x]", rc));
	}

    clOsalMutexUnlock(gCorMutexes.gCorDeltaObjDbMutex);

	return rc;
}

/*
 * clCorDeltaDbalStore
 * 
 * This api takes the delta transaction information and store it in
 * a dbal. The flag informs if the operation is create/set/delete.
 * For MO/MSO create/delete operation, it creates/deletes enteries for 
 * MO+ATTRPATH-SET.
 */

                       // coverity[pass_by_value]
ClRcT clCorDeltaDbStore(CL_IN ClCorMOIdT moId, 
                        CL_IN ClCorAttrPathPtrT pAttrPath, 
                        CL_IN ClCorDeltaSaveTypeT flag)
{
	ClRcT rc = CL_OK;
	ClCorDeltaSaveKeyT deltaDbKey;
	Byte_h data  = NULL;
	ClUint32T dataSize = 0;
 
	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Save Type is not configured as Delta Save"));
		return CL_OK;
	}

    /*
     * Ensure that all the holes and unfilled part of the keys are filled with zero. 
     */
	memset(&deltaDbKey, 0 , sizeof(ClCorDeltaSaveKeyT));

    /* Set the version information */
    deltaDbKey.version.releaseCode = CL_RELEASE_VERSION;
    deltaDbKey.version.majorVersion = CL_MAJOR_VERSION;
    deltaDbKey.version.minorVersion = CL_MINOR_VERSION;

    deltaDbKey.moId = moId;
	if(pAttrPath != NULL)
		deltaDbKey.attrPath = *pAttrPath;
	else
		clCorAttrPathInitialize(&deltaDbKey.attrPath);
	deltaDbKey.flag = flag;

	switch(flag)
	{
		case CL_COR_DELTA_MO_CREATE:
		case CL_COR_DELTA_MSO_CREATE:
		{
			data = (Byte_h)&dataSize;
			dataSize = sizeof(dataSize);

            clOsalMutexLock(gCorMutexes.gCorDeltaObjDbMutex);
			/* Key size is sizeof(ClCorDeltaSaveKeyT) */
            rc = clDbalRecordInsert( hDeltaObjDb,
       	        	                       (ClDBKeyT) &deltaDbKey,
           		                           sizeof(ClCorDeltaSaveKeyT),
               	    	                   (ClDBRecordT) data,
                   	    	               dataSize);
            clOsalMutexUnlock(gCorMutexes.gCorDeltaObjDbMutex);
		   	if (CL_OK != rc)
			{
				if(CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE) 
        		  	 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "DB Record addition failed. rc [0x%x]", rc));
				else
 					rc = CL_OK;
			}
            else
            {
                /* Create the key for all Containment Attribute paths and save/restore them from db */
				clCorDeltaDbAttrPathOp(moId, flag);	
            }
		}
		break;
		case CL_COR_DELTA_SET:
		{
			/*get the data from the dm */
			rc = clCorDmClassAttrBufGet(moId, pAttrPath, &data, &dataSize);
            if(CL_OK != rc)
            {
                clLogError("DLS", "OBJ", 
                        "Failed while getting the object buffer for storing in object DB. rc[0x%x]", rc);
                return rc;
            }

            clOsalMutexLock(gCorMutexes.gCorDeltaObjDbMutex);
			/* Key size is sizeof(ClCorDeltaSaveKeyT) */
		   	if (CL_OK != (rc = clDbalRecordReplace( hDeltaObjDb,
       	        	                       (ClDBKeyT) &deltaDbKey,
           		                           sizeof(ClCorDeltaSaveKeyT),
               	    	                   (ClDBRecordT)data,
                   	    	               dataSize)))
            {
     		  	 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "DB Record addition failed. rc [0x%x]", rc));
            }
            clOsalMutexUnlock(gCorMutexes.gCorDeltaObjDbMutex);

		    clHeapFree(data);	
		}
		break;
		case CL_COR_DELTA_DELETE:
		{
		     if(moId.svcId != CL_COR_INVALID_SVC_ID)
			   rc = clCorDeltaDbDelete(moId, NULL, CL_COR_DELTA_MSO_CREATE);
		     else
		    	rc = clCorDeltaDbDelete(moId, NULL, CL_COR_DELTA_MO_CREATE);

           /* Delete SIMPLE + Containment Set enteries*/

			rc =  clCorDeltaDbAttrPathOp(moId, flag);	
			if(rc != CL_OK)
				CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nFailed to delete entries from the dbal \n"));
		}
			break;
		default:
			rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Invalid value of the flag passed 0x%x \n",rc));
	}

	return rc;
}



/*
 * Depending on "flag" walk the attributes and add/remove moid+attrPath enteries from db
 * Also Add/Delte the simple (i.e. the base class)related enteries  in the db.
 */

ClRcT
                       // coverity[pass_by_value]
clCorDeltaDbAttrPathOp(ClCorMOIdT moId, 
                       ClInt32T flag)
{
	ClRcT	rc = CL_OK;
	CORClass_h class = NULL;
    ClCorMOClassPathT moClsPath;
    ClCorClassTypeT   classId;
	ClCorDeltaSaveKeyT *buf = clHeapAllocate(sizeof(ClCorDeltaSaveKeyT));	
	
    if(!buf)
        return CL_COR_SET_RC(CL_ERR_NO_MEMORY);

	buf->moId = moId;
	clCorAttrPathInitialize(&buf->attrPath);
	buf->flag = flag;

    clCorMoIdToMoClassPathGet(&moId, &moClsPath);
    if ((rc = corMOTreeClassGet (&moClsPath, moId.svcId, &classId)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not get the classId \
                     for the supplied moId/svcId combination\n"));
        return (rc);
	}
    
	class = dmClassGet(classId);
	if(class != NULL)
    {
   	  rc = dmClassAttrWalk(class,
                          NULL, 
                          clCorDeltaDbOperate,
                          (Byte_h *)&buf);
    
    }
	

	/* Adding these statements in order to add the SET entry for the base class */
	if(flag == CL_COR_DELTA_MO_CREATE || flag == CL_COR_DELTA_MSO_CREATE)
		rc = clCorDeltaDbStore(moId, NULL, CL_COR_DELTA_SET);
	else if(flag == CL_COR_DELTA_DELETE )
		rc = clCorDeltaDbDelete(moId, NULL, CL_COR_DELTA_SET);

    clHeapFree(buf);
	return rc;
}

/* 
 * THis callback is called for each attribute of an object. For containmnet attribute this API creates/deletes
*  the MO+ATTRPATH_SET entry in the data  base as specified by the value in pBuf
*/


ClRcT clCorDeltaDbOperate (CORAttr_h attrH, Byte_h* pBuf)
{
	ClRcT rc = CL_OK;
	ClCorDeltaSaveKeyT *buf = *(ClCorDeltaSaveKeyT **)pBuf;
	ClUint32T	size = 0, index;
	/* If it is a containment attribute 
	 * Do:
     * 1. If the attr walk is called during create add all the containment Path entries in db.
     * 2. If this is called while deletion, remove all the entries from the db.
     *
	 */

	if(attrH->attrType.type == CL_COR_CONTAINMENT_ATTR)
	{
		size = attrH->attrValue.max;	
		
		for(index = 0; index < size ; index++)
		{
			CORClass_h cls;
			clCorAttrPathAppend(&buf->attrPath, attrH->attrId, index);	
			if(buf->flag == CL_COR_DELTA_MO_CREATE || buf->flag == CL_COR_DELTA_MSO_CREATE)
			{
				rc = clCorDeltaDbStore(buf->moId, &buf->attrPath, CL_COR_DELTA_SET);
				if(rc != CL_OK)
                {
                    clLogError("DLS", "DEL", "Failed while adding the DB entry. rc[0x%x]", rc);
					return rc;
                }
			}
			else if(buf->flag == CL_COR_DELTA_DELETE )
			{
				rc = clCorDeltaDbDelete(buf->moId, &buf->attrPath, CL_COR_DELTA_SET);
				if(rc != CL_OK)
                {
                    clLogError("DLS" , "DEL", "Failed while deleting the DB entry. rc[0x%x]", rc);
					return rc;
                }
			}

			cls = dmClassGet(attrH->attrType.u.remClassId);
			if(cls != NULL)
				dmClassAttrWalk(cls, 0 , clCorDeltaDbOperate, (Byte_h *)&buf );
			clCorAttrPathTruncate(&buf->attrPath, (buf->attrPath.depth -1 ));
		}
	}
	
	return rc;
}


/***************************************************************************************/
/* APIs required  For  Route Db Updating
***************************************************************************************/
/*
 * This API stores Route Info into Db 
 **/
                                 // coverity[pass_by_value]
ClRcT clCorDeltaDbRouteInfoStore(CL_IN ClCorMOIdT moId, 
                                 // coverity[pass_by_value]
                                 CL_IN corRouteApiInfo_t routeInfo,
                                 CL_IN ClCorDeltaSaveTypeT flag)
{
    return CL_OK;

#if 0
	ClRcT rc = CL_OK;
	ClCorDeltaRouteSaveKeyT deltaDbKey;
	ClUint32T dataSize = 0;

	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Save Type is not configured as Delta Save"));
		return CL_OK;
	}
		
    clOsalMutexLock(gCorMutexes.gCorDeltaRmDbMutex);

	memset(&deltaDbKey, 0 , sizeof(ClCorDeltaRouteSaveKeyT));
    
    deltaDbKey.version.releaseCode = CL_RELEASE_VERSION;
    deltaDbKey.version.majorVersion = CL_MAJOR_VERSION;
    deltaDbKey.version.minorVersion = CL_MINOR_VERSION;

    deltaDbKey.moId = moId;
	deltaDbKey.nodeAddr = routeInfo.addr.nodeAddress;
	deltaDbKey.portId = routeInfo.addr.portId;

	switch(flag)
	{
		case CL_COR_DELTA_ROUTE_CREATE:
		{
			dataSize = sizeof(corRouteApiInfo_t);

			/* Key size is sizeof(ClCorDeltaRouteSaveKeyT) */
		   	if (CL_OK != (rc = clDbalRecordInsert(hDeltaRouteDb,
       	        	                       (ClDBKeyT) &deltaDbKey,
           		                           sizeof(ClCorDeltaRouteSaveKeyT),
               	    	                   (ClDBRecordT) &routeInfo,
                   	    	               dataSize)))
   			{
				if(CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE) 
        		  	 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Route DB Record addition failed. rc [0x%x]", rc));
				else
 					rc = CL_OK;
   			}
		}
		break;
		case CL_COR_DELTA_ROUTE_STATUS_SET:
		{
			/* Key size is sizeof(ClCorDeltaSaveKeyT) */
			dataSize = sizeof(corRouteApiInfo_t);
		   	if (CL_OK != (rc = clDbalRecordReplace( hDeltaRouteDb,
       	        	                       (ClDBKeyT) &deltaDbKey,
           		                           sizeof(ClCorDeltaRouteSaveKeyT),
               	    	                   (ClDBRecordT)&routeInfo,
                   	    	               dataSize)))
   			{
     		  	 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "DB Record addition failed. rc [0x%x]", rc));
   			}
		}
		break;
		case CL_COR_DELTA_ROUTE_DELETE:
		{
			if((rc = clDbalRecordDelete( hDeltaRouteDb, 
										 (ClDBKeyT) &deltaDbKey, 
            	                         sizeof(ClCorDeltaRouteSaveKeyT))) != CL_OK)
			{
				CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the entry from the dbal. rc[0x%x]", rc));
			}
		}
		break;
		default:
			rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Invalid value of the flag passed .rc [0x%x]\n", rc));
	}

    clOsalMutexUnlock(gCorMutexes.gCorDeltaRmDbMutex);
	return rc;
#endif

}







/*****************************************************************************
* APIs used for creatings Dbs in Slave.


*****************************************************************************/
ClRcT 
ClCorDeltaDbSlaveCreate()
{
	ClRcT	rc = CL_OK;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};

    gettimeofday(&delay[0], NULL);
#endif
	rc = _clCorObjectWalk(NULL, NULL, clCorDeltaDbRecreate, CL_COR_MO_WALK, 0);
	if(rc != CL_OK)
	{
		clLogError("DES", "REC", "Storing the MO Objects in the Db failed for Slave. rc[0x%x]", rc);
		return rc;
	}

	rc = _clCorObjectWalk(NULL, NULL, clCorDeltaDbRecreate, CL_COR_MSO_WALK, 0);
	if(rc != CL_OK)
	{
		clLogError("DES", "REC", "Storing the MSO objects in the Db failed for Slave. rc[0x%x]", rc);
		return rc;
	}

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[1], NULL);
    gCorOpDelay.objDbRecreate = (delay[1].tv_sec * 1000000 + delay[1].tv_usec)  - 
                                 (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[0], NULL);
#endif

	rc = clCorDeltaDbRouteRecreate();
	if(rc != CL_OK)
    {
		clLogError("DES", "REC", "Failed to recreate the Route db for Slave. rc[0x%x]", rc);
        return rc;
    }

#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[1], NULL);
    gCorOpDelay.routeDbRecreate =  (delay[1].tv_sec * 1000000 + delay[1].tv_usec)  - 
                                    (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

#ifdef CL_COR_MEASURE_DELAYS
    gCorOpDelay.corDbRecreate = gCorOpDelay.objDbRecreate + gCorOpDelay.routeDbRecreate;
#endif

    clLogNotice("DES", "REC", "Recreated the Object and route Dbs successfully.");
	return rc;
}


void clCorDeltaDbRecreate(void* pMoId, ClBufferHandleT buffer)
{
//	ClRcT	rc = CL_OK;
	ClCorMOIdT moId;

    moId = *(ClCorMOIdT *) pMoId;

	if(moId.svcId != CL_COR_INVALID_SVC_ID)
	{
		clCorDeltaDbStore(moId, NULL,CL_COR_DELTA_MSO_CREATE);
	}
	else
	{
		clCorDeltaDbStore(moId, NULL, CL_COR_DELTA_MO_CREATE);
	}
	return ;
}

ClRcT clCorDeltaDbRouteRecreate()
{
    return CL_OK;

#if 0
	ClRcT rc = CL_OK;
	ClUint32T index = 0, count = 0;
	corRouteApiInfo_t routeInfo;

	for(index = 0; index < nRouteConfig; index++)
	{
		RouteInfo_t *pRt;
        CORStation_t  *pStn = NULL;
		if(NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, index)))
		{
			break;
		}

		for(count = 0; count < pRt->nStations; count++)
		{
			pStn = corVectorGet(&pRt->stnVector, count);
			routeInfo.moId = *pRt->moH;
			routeInfo.addr.nodeAddress =  pStn->nodeAddress;
			routeInfo.addr.portId =  pStn->portId;
            routeInfo.status = pStn->status;		
            routeInfo.primaryOI = pStn->isPrimaryOI;

			rc = clCorDeltaDbRouteInfoStore(*pRt->moH, routeInfo, CL_COR_DELTA_ROUTE_CREATE);
			if(rc != CL_OK)
			{
				CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to recreated Route Db for Slave"));
				return rc;
			}
		}
			
	}	
	
	return rc;
#endif

}
