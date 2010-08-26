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
 * ModuleName  : om
 * File        : omCORTab.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clOmErrors.h>
#include <clOmApi.h>
#include <clLogApi.h>
#include "clOmDefs.h"
#include "omCORTab.h"
#include "omCompId.h"

/* Globals */
/**
 * 
 *  Lookup table (hash table)  - moIdToOmHHashTbl
 *
 * DO NOT DOCUMENT. INTERNAL DATA STRUCTURE.
 */
static omCorLkupTbl_t	gOmCorLkupTbl;


/* Locals */
static void omCorDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);
static void omCorDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);
static void omCorMoIdDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);
static void omCorMoIdDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);
static ClUint32T omCorMoIdHashFn(ClCntKeyHandleT key);
static ClUint32T omCorOmHHashFn(ClCntKeyHandleT key);
static ClInt32T moIdKeyCompare(ClCntKeyHandleT key1, 
			ClCntKeyHandleT key2);
static ClInt32T omHKeyCompare(ClCntKeyHandleT key1, 
			ClCntKeyHandleT key2);
static ClRcT omCORTruncateMoId(ClCorMOIdPtrT hMoId, ClCorMOIdPtrT *pMoIdOut);



/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  This API creates the hash tables used for MOID to OMID
 *  conversion and vice versa.
 */

ClRcT
omCORTableInit(void)
{
	ClRcT	rc = CL_OK;


	/* Create a hash table for MOID to OM handle conversion */
    rc = clCntHashtblCreate(MOID_TO_OMH_MAX_BUCKETS, moIdKeyCompare, 
			omCorMoIdHashFn, omCorDeleteCallBack, omCorDestroyCallBack, 
	        CL_CNT_NON_UNIQUE_KEY, &gOmCorLkupTbl.moIdToOmHHashTbl);
    if( rc != CL_OK )
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("OM: clOmInitialize error MOID-to-OMH Hash table "
			"create FAILED(rc = 0x%x)!!!\n\r", rc));
		return rc;
		}

	/* Create a hash table for OM handle to MOID conversion */
    rc = clCntHashtblCreate(OMH_TO_MOID_MAX_BUCKETS, omHKeyCompare, 
			omCorOmHHashFn, omCorMoIdDeleteCallBack, omCorMoIdDestroyCallBack, 
			CL_CNT_NON_UNIQUE_KEY, &gOmCorLkupTbl.omHToMoIdHashTbl);
    if( rc != CL_OK )
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("OM: clOmInitialize error MOID-to-OMH Hash table "
			"create FAILED(rc = 0x%x)!!!\n\r", rc));
		return rc;
		}

	return rc;
}



/**
 *  Add an entry in ClCorMOId to OMId mapping table.
 *
 *  This API adds given om handle entry for given MOID into the
 * hash table which maintains MOID to OMID mapping.
 *                                                                        
 *  @param hMoIdKey       - MOID handle for COR object. Serves as a key for 
 *  						accessing the ClCorMOId to OMId mapping table
 *  @param hOm      - OM Handle for OM object which is associated with COR object
 *  						represented by the ClCorMOId.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmMoIdToOmIdMapInsert(ClCorMOIdPtrT hMoIdKey, ClHandleT hOm)
	{
	ClRcT		rc = CL_OK;
	ClCorMOIdPtrT		hTmpMoId = NULL;
#ifdef CL_DEBUG
	ClUint8T	aFuncName[] = "omCORMoIdAdd()";
#endif

        /* sanity check for input parameters */
        rc = clCorMoIdValidate(hMoIdKey);
	if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "Invalid ClCorMOId specified"));
            return rc;
        }

	/* Truncate MO ID */
	rc = omCORTruncateMoId(hMoIdKey, &hTmpMoId);
	if (rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to truncate MO ID w/ rc 0x%x!\r\n",
			aFuncName, rc));
		return(rc);
		}

	/* Store OM handle in hash table */
	rc = clCntNodeAdd(gOmCorLkupTbl.moIdToOmHHashTbl, 
			(ClPtrT)(ClWordT)hTmpMoId, (ClPtrT)(ClWordT)hOm, NULL);
	if (rc != CL_OK)
		{
		clHeapFree((void *)hTmpMoId);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to add OM handle to hash table, "
			"w/ rc 0x%x!\r\n", aFuncName, rc));
		return (rc);
		}

	/* Store MO ID in hash table */
	rc = clCntNodeAdd(gOmCorLkupTbl.omHToMoIdHashTbl, 
			(ClPtrT)(ClWordT)hOm, (ClCntDataHandleT)hTmpMoId, NULL);
	if (rc != CL_OK)
		{
		clHeapFree((void *)hTmpMoId);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to add to MO ID to hash table, "
			"w/ rc 0x%x!\r\n", aFuncName, rc));
		return (rc);
		}

	return rc;
	}

/**
 *  Remove the entry from moId To Om Hash Tblle.
 *
 *  This API removes an entry corresponding to ClCorMOId (key) from the
 *  ClCorMOId to OMId mapping table.
 *                                                                        
 *  @param hMoIdKey       - ClCorMOId handle for COR object. 
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmMoIdToOmIdMapRemove(ClCorMOIdPtrT hMoIdKey)
	{
	ClRcT				rc = CL_OK;
	ClCorMOIdPtrT				hTmpMoId = NULL;
	ClCntKeyHandleT	hTmpOmKey = 0;
#ifdef CL_DEBUG
	ClUint8T			aFuncName[] = "omCORMoIdAdd()";
#endif
	ClCntNodeHandleT 	nodeHandle = 0;

	CL_ASSERT(hMoIdKey);

        /* sanity check for input parameters */
        rc = clCorMoIdValidate(hMoIdKey);
	if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "Invalid ClCorMOId specified"));
            return rc;
        }

	/* Truncate MO ID */
	rc = omCORTruncateMoId(hMoIdKey, &hTmpMoId);
	if (rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to truncate MO ID w/ rc 0x%x!\r\n",
			aFuncName, rc));
		return (rc);
		}

	/*** Delete node from the MO ID hash table ***/
	/* Look up the node for MO ID in hash table */
	if ((rc = clCntNodeFind(gOmCorLkupTbl.moIdToOmHHashTbl, 
		(ClCntKeyHandleT)hTmpMoId, &nodeHandle)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to find node for MO ID, with rc "
			"0x%x\r\n", aFuncName, rc));
		/*clCorMoIdShow(hTmpMoId);*/
		clHeapFree((void *)hTmpMoId);
		return rc;
		}
	/* Get the OM handle for this MO ID for use as key later */
	if ((rc = clCntNodeUserDataGet(gOmCorLkupTbl.moIdToOmHHashTbl, 
		nodeHandle, (ClCntDataHandleT *)&hTmpOmKey)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to get the OM handle from the "
			"node for MO ID, with rc 0x%x\r\n", aFuncName, rc));
	/*	clCorMoIdShow(hTmpMoId);*/
		return rc;
		}
	rc = clCntNodeDelete(gOmCorLkupTbl.moIdToOmHHashTbl, nodeHandle);
	if (rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to remove MO ID from hash table, "
			"w/ rc 0x%x!\r\n", aFuncName, rc));
	/*	clCorMoIdShow(hTmpMoId);*/
		clHeapFree((void *)hTmpMoId);
		return (rc);
		}

	/*** Delete node from the OM handle hash table ***/
	/* Look up the node for OM Handle in hash table */
	if ((rc = clCntNodeFind(gOmCorLkupTbl.omHToMoIdHashTbl, 
		hTmpOmKey, &nodeHandle)) != CL_OK)
		{
		clHeapFree((void *)hTmpMoId);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to find node for OM Handle %p, "
			"with rc 0x%x\r\n", aFuncName, hTmpOmKey, rc));
		return rc;
		}
	rc = clCntNodeDelete(gOmCorLkupTbl.omHToMoIdHashTbl, nodeHandle);
	if (rc != CL_OK)
		{
		clHeapFree((void *)hTmpMoId);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to remove OM handle %p "
			"from hash table, w/ rc 0x%x!\r\n", aFuncName, (void *)hTmpOmKey, rc));
		return (rc);
		}
	clHeapFree((void *)hTmpMoId);
	return rc;
	}


/**
 *  Get the OM Handle given ClCorMOId.
 *
 *  This API returns OM Handle given ClCorMOId from the ClCorMOId to 
 *  OMId mapping table. If the OM Handle can not be found it
 *  returns NULL 
 *                                                                        
 *  @param hMoIdKey       - MOID handle for COR object. Serves as a key for 
 *  						accessing the ClCorMOId to OMId mapping table
 *  @param pOmH      - [OUT] OM Handle for OM object which is associated with COR object
 *  						represented by the ClCorMOId.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmMoIdToOmHandleGet(ClCorMOIdPtrT hMoIdKey, ClHandleT *pOmH)
	{
	ClRcT				rc = CL_OK;
#ifdef CL_DEBUG
	ClUint8T			aFuncName[] = "clOmMoIdToOmHandleGet()";
#endif
	ClCntNodeHandleT 	nodeHandle = 0;
  	ClCntDataHandleT hTmpData = 0;
	ClCorMOIdPtrT				hTmpMoId = NULL;
	
	CL_ASSERT(pOmH && hMoIdKey);

        /* sanity check for input parameters */
        rc = clCorMoIdValidate(hMoIdKey);
	if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "Invalid ClCorMOId specified"));
            return rc;
        }
         
        if(pOmH == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "NULL argument specified"));
            return CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
        }
  
	/* Truncate MO ID */
	rc = omCORTruncateMoId(hMoIdKey, &hTmpMoId);
	if (rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to truncate MO ID w/ rc 0x%x!\r\n",
			aFuncName, rc));
		return (rc);
		}

	/* find the node for this MO ID */
	if ((rc = clCntNodeFind(gOmCorLkupTbl.moIdToOmHHashTbl, 
		(ClCntKeyHandleT)hTmpMoId, &nodeHandle)) != CL_OK)
		{
		/* Use 'trace' because some invocations are just queries */
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("%s: Failed to find node for MO ID, with rc "
			"0x%x\r\n", aFuncName, rc));
        clHeapFree((void *)hTmpMoId);
		return rc;
		}
	/* now get the OM handle for this MO ID */
	if ((rc = clCntNodeUserDataGet(gOmCorLkupTbl.moIdToOmHHashTbl, 
		nodeHandle, (ClCntDataHandleT *)&hTmpData)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to get the OM handle from the "
			"node for MO ID, with rc 0x%x\r\n", aFuncName, rc));
/*		clCorMoIdShow(hTmpMoId);*/
        clHeapFree((void *)hTmpMoId);
		return rc;
		}
	/* return OM handle */
	*pOmH = (ClWordT)hTmpData;
	clHeapFree((void *)hTmpMoId);

	return rc;
	}

/**
 *  Get the ClCorMOId given OM Handle
 *
 *  This API returns ClCorMOId given OM Handle from the OMId to 
 *  ClCorMOId mapping table. If the ClCorMOId can not be found it
 *  returns NULL 
 *                                                                        
 *  @param hOmKey       - OM handle for OM object. Serves as a key for 
 *  						accessing the OMId to ClCorMOId mapping table
 *  @param pOmH      - [OUT] ClCorMOId for COR object which is associated with MO object
 *  						represented by the OM Handle.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmOmHandleToMoIdGet(ClHandleT hOmKey, ClCorMOIdPtrT *phMoId)
	{
	ClRcT				rc = CL_OK;
#ifdef CL_DEBUG
	ClUint8T			aFuncName[] = "clOmMoIdToOmHandleGet()";
#endif
	ClCntNodeHandleT 	nodeHandle = 0;
  	ClCntDataHandleT hTmpData = 0;
	
	if(phMoId == NULL)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer passed for the ClCorMOId Handle"));
		return CL_ERR_NULL_POINTER;
		}
	if(hOmKey == 0)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Key !!  Key can not be Zero"));
		return CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE);
		}
	*phMoId = NULL;
	/* find the node for this OM Handle */
	if ((rc = clCntNodeFind(gOmCorLkupTbl.omHToMoIdHashTbl, 
		(ClPtrT)(ClWordT)hOmKey, &nodeHandle)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to find node for MO ID, with rc "
			"0x%x\r\n", aFuncName, rc));
		return rc;
		}
	/* now get the MO ID for this OM handle */
	if ((rc = clCntNodeUserDataGet(gOmCorLkupTbl.omHToMoIdHashTbl, 
		nodeHandle, &hTmpData)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to get the MO ID from the "
			"node for OM Handle 0x%x, with rc 0x%x\r\n", aFuncName, 
			(ClUint32T)hOmKey, rc));
		return rc;
		}
	*phMoId = (ClCorMOIdPtrT)hTmpData;
	return rc;
	}


/**
 *  Get the OM object reference given ClCorMOId.
 *
 *  This API returns reference (pointer to OM object) corresponding to
 *  given COR object represented by ClCorMOId.
 *                                                                        
 *  @param hMoIdKey       - MOID handle for COR object. 
 *  @param ppOmObj      - [OUT] Pointer to OM Object.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmMoIdToOmObjectReferenceGet(ClCorMOIdPtrT hMoIdKey, void **ppOmObj)
	{
	ClRcT		rc = CL_OK;
  	void		*pTmpObj = NULL;
	ClHandleT 	hOm = 0;
	
	CL_ASSERT(hMoIdKey && ppOmObj);

        /* sanity check for input parameters */
        rc = clCorMoIdValidate(hMoIdKey);
	if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "Invalid ClCorMOId specified"));
            return rc;
        }

        if(ppOmObj == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "NULL argument specified"));
            return CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
        }

	*ppOmObj = NULL;
	/* Look up OM handle */
	rc = clOmMoIdToOmHandleGet(hMoIdKey, &hOm);
	if (rc != CL_OK)
		{
		return rc;
		}
	/* proceed if we have the OM handle */
	if (rc == CL_OK)
		{
		rc = clOmObjectReferenceByOmHandleGet(hOm, &pTmpObj);
		}
	*ppOmObj = pTmpObj;
	return rc;
	}

/**
 *  Get the ClCorMOId given OM object reference.
 *
 *  This API returns ClCorMOId of a COR object given object reference
 *  to OM object.
 *                                                                        
 *  @param pOmObjRefKey       - Reference to OM object. 
 *  @param phMoId      - [OUT] Pointer to ClCorMOId handle.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT clOmOmObjectReferenceToMoIdGet(void *pOmObjRefKey, ClCorMOIdPtrT *phMoId)
	{
	ClRcT		rc = CL_OK;
	ClHandleT 	hOm = 0;
	
	CL_ASSERT(pOmObjRefKey && phMoId);
        /* sanity check for input parameters */
        if((pOmObjRefKey == NULL) || (phMoId == NULL))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "NULL argument specified"));
            return CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
        }

	*phMoId = NULL;
	/* Look up OM handle by OM obj reference */
	rc = clOmOmHandleByObjectReferenceGet(pOmObjRefKey, &hOm);
	if (rc != CL_OK)
		{
		return rc;
		}
	/* Look up MO ID by OM handle */
	return (clOmOmHandleToMoIdGet(hOm, phMoId));
	}


/**
 *  Create OM & COR objects given OM class Id & ClCorMOId
 *
 *  This API creates OM as well as COR objects. For creating
 *  OM object, user need to specify classId and ClCorMOId of the
 *  COR Object corresponding to this OM object. In order to
 *  create COR object the user need to specify ClCorMOId.
 *                                                                        
 *  @param classId       - Class Id for the OM class
 *  @param omHandle	- [OUT]OM Handle to OM object created
 *  @param objPtr	- 	[OUT] Reference to created OM Object
 *  @param  moId	-	ClCorMOId of the COR object to be created.
 *  @param svcId	-	Service Id of service for which the COR and
 *					OM objects are to be created
 *  @param corHandle      - [OUT] Handle to the COR object created.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clOmCorAndOmObjectsCreate(ClOmClassTypeT     classId, 
			   ClHandleT     *omHandle,
			   void          **objPtr,
			   ClCorMOIdPtrT          moId,
			   ClUint32T      svcId,
			   ClCorObjectHandleT *corHandle)
{
	ClRcT rc = CL_OK;
	void *ptr = NULL;

	CL_ASSERT(objPtr && omHandle && corHandle);
        /* sanity check for input parameters */
        if((objPtr == NULL) || (omHandle == NULL) || (corHandle == NULL))
        {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL input argument"));
		return (rc);
	}
        rc = clCorMoIdServiceSet(moId, svcId);
        if(rc != CL_OK)
        {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to Set the service ID"));
		return(rc);
	}
	if ((rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, moId,
					         (ClCorObjectHandleT *)corHandle)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create MSO object"));
		return(rc);
	}

	rc = clOmObjectCreate(classId, 1 /*numInstances*/, omHandle,
            &ptr, (void *)moId, sizeof(ClCorMOIdT));
	if(ptr == NULL)
	{
		if ((rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, *corHandle)) != CL_OK)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete MSO object, as a result of OM object creation"));
			return(rc);
		}

		return(CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}

	*objPtr = ptr;

	/* Add MO ID & OM Handle to table */
	if (clOmMoIdToOmIdMapInsert(moId, *omHandle) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to install OM/COR table entry"));
		/* delete COR obj */
		if ((rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, *corHandle)) != CL_OK)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete MSO object!\r\n"));
			return(rc);
		}

		/* delete OM obj */
		if ((rc = clOmObjectByReferenceDelete(ptr)) != CL_OK) 
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete OM object!\r\n"));
			return(rc);
		}

		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}
	return(CL_OK);
}



/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  Hash key compare function for moId to OMId mapping table.
 *
 *  This function does byte by byte comparison of ClCorMOId passed.
 *  
 *  @param key1	- 	First ClCorMOId handle to be compared
 *  @param key2	-	Second ClCorMOId handle
 */

static ClInt32T moIdKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
	{
	ClUint16T	moIdLen = 0;
/*	ClCorMOIdPtrT		hMoId = (ClCorMOIdPtrT)key1;  */
	ClInt32T	cmpResult = 0;

	if (!key1 || !key2)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("moIdKeyCompare(): bad key(s), key1 %p & "
			"key2 %p!!!\r\n", (void *)key1, (void *)key2));
		return(CL_OM_SET_RC(CL_OM_ERR_INTERNAL));
		}
/*	moIdLen = COR_CURR_MOH_SIZE(hMoId) + COR_MOID_HDR_SIZE; */
        moIdLen = sizeof(ClCorMOIdT);
	cmpResult = memcmp((void *)key1, (void *)key2, moIdLen);
	return cmpResult;
	}

/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 * 
 *   Hash key compare function for OMId to moId mapping table.
 *
 *  This API compares OM handles. Returns 0 if both the keys are same. Else returns
 *  non 0 value.
 *
 *  @param key1	- 	FirstOM handle to be compared
 *  @param key2	-	Second OM handle

 */                                                                                
static ClInt32T omHKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
	{
	if (!key1 || !key2)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("moIdKeyCompare(): bad key(s), key1 %p & "
			"key2 %p!!!\r\n", (void *)key1, (void *)key2));
		return (CL_OM_SET_RC(CL_OM_ERR_INTERNAL));
		}
    return ((ClWordT)key1 - (ClWordT)key2);
	}


/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  Hash function for ClCorMOId to OMId mapping table.
 *
 *  This function calculates the hash value given ClCorMOId which is the
 *  key for ClCorMOId to OMId mapping table.
 *
 *  @param key      - 	Key for Hash Table (ClCorMOId in this case).
 *
 *  @returns 	-	Hash Value for the given Key
 */                                                                                                                                                                
static ClUint32T omCorMoIdHashFn(ClCntKeyHandleT key)
	{
	ClCorMOIdPtrT		hMoId = (ClCorMOIdPtrT)key;
	ClUint8T	*pTmp = NULL;
	ClUint32T	hashVal = 0;
	ClUint16T	moIdLen = 0;

	if (hMoId == NULL)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omCorMoIdHashFn(): bad key!!!\r\n"));
		return (CL_OM_SET_RC(CL_OM_ERR_INTERNAL));
		}
	pTmp = (ClUint8T *)hMoId;
	moIdLen = COR_CURR_MOH_SIZE(hMoId) + COR_MOID_HDR_SIZE;
	
	while(moIdLen--)
		{
		hashVal = (ClUint32T)(*pTmp++ ^ (hashVal << HASH_SHIFT));
		}
	return (ClUint32T)(hashVal % MOID_TO_OMH_MAX_BUCKETS);
	}


/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  Hash function for OMId to ClCorMOId mapping table.
 *
 *  This function calculates the hash value given OMId which is the
 *  key for OMId to ClCorMOId mapping table.
 *
 *  @param key      - 	Key for Hash Table (ClCorMOId in this case).
 *
 *  @returns 	-	Hash Value for the given Key (OM Handle)
 */                                                                                                                                                                
static ClUint32T omCorOmHHashFn(ClCntKeyHandleT key)
	{
	if (key == 0)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omCorOmHHashFn(): bad key!!!\r\n"));
		return (CL_OM_SET_RC(CL_OM_ERR_INTERNAL));
		}
	return ((ClWordT)key % OMH_TO_MOID_MAX_BUCKETS);
	}

/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  dummy callback function for delete.
 *
 */                                                                                                                                                                
static void omCorDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
	{
   /* do nothing here */
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omCorDeleteCallBack Function Do nothing!!\n\r"));
	}

/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  dummy callback function for destroy.
 *
 */                                                                                                                                                                
static void omCorDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
	{
   /* do nothing here */
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omCorDestroyCallBack Function Do nothing!!\n\r"));
	}

/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  callback function for delete.
 *
 */                                                                                                                                                                
static void omCorMoIdDeleteCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
	{
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omCorMoIdDeleteCallBack called!\n\r"));

	/* Delete the stored MO ID */
	if ((void *)userData != NULL)
		{
		clHeapFree((void *)userData);
		}
	}

/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  callback function for destroy.
 *
 */                                                                                                                                                                
static void omCorMoIdDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData)
	{
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omCorMoIdDestroyCallBack called!\n\r"));

	/* Delete the stored MO ID */
	if ((void *)userData != NULL)
		{
		clHeapFree((void *)userData);
		}
	}


/**
 *  NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 *  Truncate the moId.
 *
 *  This API is used to truncate the moId for the purpose of using it
 *  as a key to moId to OMId mapping table.
 *
 *  @param hMoId      - 	ClCorMOId handle (non-truncated).
 *  @param pMoIdOut 	- 	[OUT] Truncated ClCorMOId
 *
 *  @returns 	-	CL_OK if the ClCorMOId can be truncated successfully
 */                                                                                                                                                                
static ClRcT omCORTruncateMoId(ClCorMOIdPtrT hMoId, ClCorMOIdPtrT *pMoIdOut)
	{
	ClUint16T	moIdLen = 0;
	ClCorMOIdPtrT		hTmpMoId = NULL;
#ifdef CL_DEBUG
	ClUint8T	aFuncName[] = "omCORTruncateMoId()";
#endif
	CL_ASSERT(hMoId && pMoIdOut);
	*pMoIdOut = NULL;

	/* Truncate MO ID to save space */
	moIdLen = COR_MOID_HDR_SIZE + COR_CURR_MOH_SIZE(hMoId);
	/* @todo: this is not efficient at all to store all empty MO handles */
	/* size should be moIdLen, but not possible at this time */
	if ((hTmpMoId = clHeapAllocate(sizeof(ClCorMOIdT))) == NULL)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to allocate memory for MO ID!"
			"\r\n", aFuncName));
		return (CL_OM_SET_RC(CL_OM_ERR_NO_MEMORY));
		}
	/* copy the MO Handles at different depths */
	memset(hTmpMoId, 0, sizeof(ClCorMOIdT));
	memcpy(&(hTmpMoId->node[0]), &(hMoId->node[0]), COR_CURR_MOH_SIZE(hMoId));
	/* copy the MO ID header info */
	hTmpMoId->svcId = hMoId->svcId;
	hTmpMoId->depth = hMoId->depth;
	hTmpMoId->qualifier = hMoId->qualifier;

	/* return the truncated MO ID */
	*pMoIdOut = hTmpMoId;

	return CL_OK;
	}
