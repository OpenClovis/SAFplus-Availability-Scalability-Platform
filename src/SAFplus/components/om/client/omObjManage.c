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
 * ModuleName  : om
 * File        : omObjManage.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the routines that create,delete and manage the   
 * objects in the system. This module is part of the Object Manager      
 * component.                                                            
 *************************************************************************/
#include <string.h>
#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clDebugApi.h>
#include <clOmBaseClass.h>
#include <clOmApi.h>
#include <clOmErrors.h>
#include <clLogApi.h>
#include "clOmDefs.h"
#include "omCompId.h"
#include "omPrivateDefs.h"

/* GLOBALS */

/* FORWARD DECLARATION */
static ClRcT omInitObjHierarchy(ClOmClassControlBlockT * pTab, ClOmClassTypeT classId, 
	void *objPtr, void *pUsrData, ClUint32T usrDataLen);
static ClRcT omFiniObjHierarchy(ClOmClassTypeT classId, void *objPtr, 
	void *pUsrData, ClUint32T usrDataLen);
static ClRcT omGetFreeInstSlot(ClUint32T **pInst, ClUint32T max, ClUint32T* pInstIdx);
static ClRcT omCommonDeleteObj(ClHandleT handle, ClUint32T numInstances,
	int flag, void *pUsrData, ClUint32T usrDataLen);
void *omCommonCreateObj(ClOmClassTypeT classId, ClUint32T numInstances, 
	ClHandleT *handle, void *pExtObj, int flag, ClRcT *rc, void *pUsrData,
	ClUint32T usrDataLen);

/*
 * Create OM Object
 *
 * This routine creates an object instance in the calling context, and
 * sets the object type the one specified as the argument. Also, the 
 * object instance is added to the object manager database.
 *
 * @param classId - class Id of class for which the object has to be created.
 * @param numInstances - Number of instances to be created
 * @param handle - [OUT] The handle to first object created 
 * @param pOmObj - [OUT] Pointer to first object created
 * @param pUsrData - User data. Passed to the user defined init function 
 *					in the Class Control Block for the class.
 * @param usrDataLen - User data length
 *
 * @returns rc - return value from the constructor
 */
ClRcT 
clOmObjectCreate(ClOmClassTypeT classId, ClUint32T numInstances, 
	ClHandleT *handle, void** pOmObj, void *pUsrData, ClUint32T usrDataLen)
{
	void *ptr;
	ClRcT rc;

	ptr = omCommonCreateObj(classId, numInstances, handle, 
				0, CL_OM_CREATE_FLAG, &rc, pUsrData, usrDataLen);

    /* Store the OM object pointer */
    *pOmObj = ptr;

    return rc;
}

/**
 * Add object to object manager repository.
 *
 * This routine adds the object that was allocated externally into the 
 * Object Manager repository.    
 *
 * @param classId - Class Id of the class 
 * @param numInstances - Number of instances to be created.
 * @param handle - [OUT] Object handle of created object.
 * @param pObjPtr - Object pointer 
 */                                                                       
ClRcT 
omAddObj(ClOmClassTypeT classId, ClUint32T numInstances, ClHandleT *handle, 
		 void *pObjPtr)
{

	ClRcT rc = CL_OK;

        /* Sanity checks for input parameters */
        if((handle == NULL) || (pObjPtr == NULL))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL input parameter"));
		return CL_OM_SET_RC(CL_OM_ERR_NULL_PTR); 
	}
   
	omCommonCreateObj(classId, numInstances, handle, pObjPtr, 
		CL_OM_ADD_FLAG, &rc, NULL, 0);
	return rc;
}




/**
 * Delete OM Object
 *
 * This routine validates the handle argument, and frees the memory held 
 * by the object. It does not free the object if it was allocated by an  
 * external memory pool.
 *
 * @param handle - Handle to the OM Object to be deleted.
 * @param numInstances - Number os instances to be deleted.
 * @param pUsrData - User data
 * @param usrDataLen - User data length
 *
 * @returns - CL_OK if all the validations and delete operation is successful
 */
ClRcT 
clOmObjectDelete(ClHandleT handle, ClUint32T numInstances, void *pUsrData,
	ClUint32T usrDataLen)
{
	int rc;

	if ((rc = omCommonDeleteObj(handle, numInstances, 
					CL_OM_DELETE_FLAG, pUsrData, usrDataLen)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("omDelete object failed"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}

	return (CL_OK);
}

/**
 * Delete OM object given reference.
 *
 * This routine deletes the object given an input reference to the object
 * 
 * @param pThis - Object reference of the object to be deleted.
 *
 * @returns CL_OK if the delete operation is successful
 *
 */
ClRcT 
clOmObjectByReferenceDelete(void *pThis)
{
	ClHandleT handle;
	int rc;

	CL_FUNC_ENTER();

	CL_ASSERT(pThis);

	if (!pThis)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer passed"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}
	handle = ((struct CL_OM_BASE_CLASS *)pThis)->__objType;
	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("The Handle is %#llX, Obj reference = %p", 
				handle, pThis));

	if ((rc = omCommonDeleteObj(handle, 1, CL_OM_DELETE_FLAG, NULL, 0)) != 
		CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete OM object"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}

	return (CL_OK);
}

/**
 * Remove OM Object
 *
 * This routine validates the handle argument, and frees the memory held 
 * by the object. It only frees the object if it was allocated by an     
 * external memory pool, It will NOT free object(s) created by OM.   
 *                                                                       
 * @param handle - Handle to the object to be removed
 * @param numInstances - Number of instances to be removed
 *
 * @returns CL_OK if remove operation is successful
 */
ClRcT 
omRemoveObj(ClHandleT handle, ClUint32T numInstances)
{
	ClRcT rc = CL_OK;

	if ((rc = omCommonDeleteObj(handle, numInstances, 
					            CL_OM_REMOVE_FLAG, NULL, 0)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("omDelete object failed"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}

	return (CL_OK);
}


/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This function returns the next available free instance slot number
 * for object.
 */
static ClRcT
omGetFreeInstSlot(ClUint32T **pInst, ClUint32T max, ClUint32T* pInstIdx)
{
	ClUint32T  idx;

	for (idx = 0; idx < max; idx++)
	{
		if (!mGET_REAL_ADDR(pInst[idx]))
        {
            *pInstIdx = idx;
            return CL_OK;
        }
	}

    /* Slot not found */
	return CL_OM_SET_RC(CL_ERR_NOT_EXIST);
}

/*
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This function initializes the object hierarchy. This function is recursively
 * called for all the parents of given object and for each object Init function
 * is called.
 * 
 * @param pTab - Class Control Block.
 * @param classId - Class Id of the class
 * @param objPtr - Object pointer
 * @param pUsrData - User data
 * @param usrDataLen - User data length
 *
 * @return return the 'rc' value got from the user app constructor callback.
 */
static ClRcT
omInitObjHierarchy(ClOmClassControlBlockT * pTab, ClOmClassTypeT classId, void *objPtr,
	void *pUsrData, ClUint32T usrDataLen)
{
    ClRcT rc = CL_OK;

	if (pTab->eParentObj != CL_OM_INVALID_CLASS)
    {
		rc = omInitObjHierarchy(clOmClassEntryGet(pTab->eParentObj), 
				pTab->eParentObj, (void *)objPtr, pUsrData, usrDataLen); 
        if (rc != CL_OK)
        {
            clLogError("OMC", "INI", "Failed to initialize the parent OM object. rc [0x%x]", rc);
            return rc;
        }
	}

	rc = pTab->fpInit((void *)objPtr, pUsrData, usrDataLen);
    if (rc != CL_OK)
    {
        clLogError("OMC", "INI", "Failed to initialize the OM object. rc [0x%x]", rc);
        return rc;
    }

    return rc;
}

/*
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *
 * De-Init the object hierarchy
 *
 * This function removes the initialization done for given object and its parents
 *
 */
static ClRcT
omFiniObjHierarchy(ClOmClassTypeT classId, void *objPtr, void *pUsrData,
	ClUint32T usrDataLen)
{
	ClOmClassTypeT tmpClass = classId;
	ClOmClassControlBlockT	*pTab = NULL;
	ClRcT		rc = CL_OK;

	while ((tmpClass != CL_OM_INVALID_CLASS) && (rc == CL_OK))
	{
		pTab = clOmClassEntryGet(tmpClass);
		CL_ASSERT(pTab);
		rc = pTab->fpDeInit((void *)objPtr, pUsrData, usrDataLen);
		tmpClass = pTab->eParentObj;
	}
	return rc;
}

/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *
 * This routine creates an object instance in the calling context, and
 * sets the object type the one specified as the argument. Also, the
 * object instance is added to the object manager database.
 */
void *
omCommonCreateObj(ClOmClassTypeT classId, ClUint32T numInstances, 
				ClHandleT *handle, void *pExtObj, int flag, ClRcT *rc,
				void *pUsrData, ClUint32T usrDataLen)
{
	int 		  idx;
	char         *pObjPtr = NULL;
	ClUint32T **pInst;
	ClUint32T   instIdx = 0;
	ClOmClassControlBlockT *      pTab;
	char         *tmpPtr;
	ClUint32T	instBlkLen = 0;

    CL_FUNC_ENTER();

    if (NULL == rc)
    {
        clLogError("OMG", "OMC", "Null value passed for return code");
        return (NULL);
    }

	if(NULL == handle || ( (flag == CL_OM_ADD_FLAG) && (NULL == pExtObj) ) )
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL handle is passed."));
        *rc = CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
		return (NULL);
	}

	*rc = 0;
	/* validate the input arguments */
	if (omClassTypeValidate(classId) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input classId arguments"));
        *rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS);
		return (NULL);
    }
	pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);

	if (!numInstances)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input numInstances arguments"));
        *rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_INSTANCE);
		return (NULL);
    }

	/* Get obj memory block length */
	instBlkLen = pTab->size * numInstances;

Reallocate:
	/*
	 * Check if the class control structure is initalized with the instance
	 * table. This is done during the initialization of the class table.
	 */
    if (!(pInst = pTab->pInstTab))
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Instance table for the class does not exist"));
        *rc = CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_NOT_EXISTS);
		return (NULL);
	}

	/* Find the first empty slot in the instance table */
    *rc = omGetFreeInstSlot(pInst, pTab->maxObjInst, &instIdx);
	if (CL_GET_ERROR_CODE(*rc) == CL_ERR_NOT_EXIST)
	{
        ClUint32T **tmp_ptr = NULL;
        ClUint32T  tmp_size = 0;

        clLogDebug("OMC", "OBC", "No free slot found in the OM class [0x%x] buffer for this object. "
                "Reallocating the class buffer size.", classId);

        /* No free slot found. Need to allocate maInstances number of slots more */
        pTab->maxObjInst = pTab->maxObjInst * 2 ; /* Double the size of max instances */
        tmp_size = (pTab->maxObjInst * sizeof(ClUint32T *));
        
        tmp_ptr = pTab->pInstTab;

        tmp_ptr = (ClUint32T **) clHeapRealloc(tmp_ptr, tmp_size);
        
        if (NULL == tmp_ptr)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for Instance Table"));
            *rc = CL_OM_SET_RC(CL_OM_ERR_NO_MEMORY);
            return (NULL);
        }

        pTab->pInstTab = tmp_ptr;

        goto Reallocate;
	}

    clLogTrace("OMC", "OBC", "Allocating the index [%u] in the OM class [0x%x] buffer.", 
            instIdx, classId);

	/* Check if we have room for the contiguous instance available to 
	 * allocate the object instances requested by the user.
	 * NOTE: We could enhance this later to allow dis-contiguous slots
	 */
	for (idx = instIdx; idx < (instIdx + numInstances); idx++)
	{
		if (mGET_REAL_ADDR(pInst[idx]))
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to fit requested num instances"));
			*rc = CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_NOSLOTS);
			return (NULL);
		}
	}

	/* Allocate the memory for the object instances */
	if (flag == CL_OM_CREATE_FLAG)
	{
		pObjPtr = (char*)clHeapAllocate(instBlkLen);
		
		if(NULL == pObjPtr)
		{
			/* 
			 * TODO: To check if we have to go a free the instances
			 *       that were allocated when (numInstances > 1) req.
			 */
#if (CW_PROFILER == YES)
			/* TODO: lockId = osLock(); */
			/* TODO: pOM->perfStat.memoryAllocFail++; */
			/* TODO: osUnLock(lockId); */
#endif
			CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("unable to allocate memory from heap!!"));
			CL_FUNC_EXIT();
			*rc = CL_ERR_NO_MEMORY;
			return (NULL);
		}
		/* Reset object block contents to 0 */
		memset(pObjPtr, 0, instBlkLen);
		
		tmpPtr = pObjPtr;
	}
	else if (flag == CL_OM_ADD_FLAG)
	{
		tmpPtr = pExtObj;
	}
	else
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Unknown flag argument passed"));
		*rc = CL_ERR_INVALID_PARAMETER;
		return (NULL);
	}

    /* Now, add it to the instance table */
	for (idx = instIdx; idx < (instIdx + numInstances); idx++)
	{
		/*
		 * Cautionary check, if the address is *NOT* aligned on a four 
		 * byte boundry
		 */
		if ((ClWordT)tmpPtr & INST_BITS_MASK)
		{
			CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Allocated buffer not on word aligned boundry"));
			*rc = CL_OM_SET_RC(CL_OM_ERR_ALOC_BUF_NOT_ALIGNED);
			return (NULL);
		}

		/* Start adding the object to the instance table */
    	((struct CL_OM_BASE_CLASS *)tmpPtr)->__objType = CL_OM_FORM_HANDLE(classId, instIdx);

		/* TODO: lockId = osLock(); */
		if (flag == CL_OM_CREATE_FLAG)
			pInst[idx] = (ClUint32T *)mSET_ALLOC_BY_OM(tmpPtr);
		else
			pInst[idx] = (ClUint32T *)tmpPtr;
    	pTab->curObjCount++;
	
#if (CW_PROFILER == YES)
		/* pOM->perfStat.objectCreated++; */
#endif
		/* TODO: osUnLock(lockId); */

		/* Now, start calling the initializer method for the class hierarchy */
		*rc = omInitObjHierarchy(pTab, classId, (void *)tmpPtr, pUsrData, usrDataLen);
		tmpPtr += pTab->size;
	}

	/* return the handle argument */
	*handle = CL_OM_FORM_HANDLE(classId, instIdx);
    CL_FUNC_EXIT();

	if (flag == CL_OM_CREATE_FLAG)
		return((void *)pObjPtr);
	else
		return(NULL);
}


/**
 *NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *
 * This routine validates the handle argument, and frees the memory held 
 * by the object. It does not free the object if it was allocated by an  
 * external memory pool, but removes it from the repository.             
 */
static ClRcT
omCommonDeleteObj(ClHandleT handle, ClUint32T numInstances, int flag,
	void *pUsrData, ClUint32T usrDataLen)
{
	ClOmClassControlBlockT 	*pTab = NULL;
	ClOmClassTypeT	classId = CL_OM_INVALID_CLASS;
	ClUint32T	instId = 0;
	ClUint32T	idx = 0;
	ClUint8T*	tmpPtr = NULL;
#ifdef CL_DEBUG
	ClInt8T	aFuncName[] = "omCommonDeleteObj()";
#endif
	ClRcT		rc = CL_OK;

    CL_FUNC_ENTER();

	if (omValidateHandle(handle) != CL_OK)
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input handle argument"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE));
	}

	classId = CL_OM_HANDLE_TO_CLASSID(handle);
	instId  = CL_OM_HANDLE_TO_INSTANCEID(handle);
	pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);

	/* Validate the numInstances agrument */
	if (!numInstances || (numInstances > pTab->curObjCount))
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid numInstances specified"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_INSTANCE));
	}

	/* Do checks to ensure the call combination is allowed */
	if ((flag == CL_OM_DELETE_FLAG) && 
		(!mALLOC_BY_OM(pTab->pInstTab[instId])))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
				("Deleting an object not allocated from OM memory pool"));
		return (CL_OM_SET_RC(CL_OM_ERR_OBJ_NOT_ALLOCATED_BY_OM));
	} 
	else if ((flag == CL_OM_REMOVE_FLAG) && 
			(mALLOC_BY_OM(pTab->pInstTab[instId])))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
				("Removing an object allocated in OM pool"));
		return (CL_OM_SET_RC(CL_OM_ERR_OBJ_ALLOCATED_BY_OM));
	}

	/* In a loop, delete an object instance at a time, and cleanup */
	tmpPtr = (ClUint8T *)mGET_REAL_ADDR(pTab->pInstTab[instId]);
	for (idx = instId; idx < (numInstances + instId); idx++)
	{
		/* Now call the finilize method for all the object in the hierarchy */
		rc = omFiniObjHierarchy(classId, 
				(void *)mGET_REAL_ADDR(pTab->pInstTab[idx]), pUsrData,
				usrDataLen);
		if (rc != CL_OK)
			{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Call to OM object's destructor "
				"failed, rc 0x%x!!!\r\n", aFuncName, rc));
    		CL_FUNC_EXIT();
			return rc;
			}

		/* TODO: lockId = osLock(); */
		pTab->pInstTab[idx] = (ClUint32T *)NULL;
    	pTab->curObjCount--;

#if (CW_PROFILER == YES)
		/* TODO: pOM->perfStat.objectDeleted++; */
#endif
		/* TODO: osUnLock(lockId); */
	}
	
	/* Free the allocated buffer */
	if (tmpPtr)
	   clHeapFree(tmpPtr);

    CL_FUNC_EXIT();
	return rc;
}
