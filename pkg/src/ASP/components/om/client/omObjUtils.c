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
 * File        : omObjUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the methods that are accociated with the base    
 * object defined in the clOmBaseClass.h file.                              
 * FILES_INCLUDED:                                                       
 *                                                                       
 * 1. clovisDefines.h                                                    
 * 2. clOmBaseClass.h                                                   
 *                                                                       
 *************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clOmBaseClass.h>
#include <clCntApi.h>
#include <clOmErrors.h>
#include <clOmApi.h>
#include <clDebugApi.h>

/* Internal Headers*/
#include "clOmDefs.h"
#include "omPrivateDefs.h"

/* GLOBALS */
/* Lookup table (hash table) for OM classes */
extern ClCntHandleT 	ghOmClassHashTbl;

/* FUNCTION_PROTOTYPES */
void displayObjects(ClOmClassControlBlockT *pTab );
/* MACROS */

/* FUNCTION_PROTOTYPES */

/* FUNCTION_DEFINITION */

/**
 *NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *                                                                       
 * Validates the object handle that is provided as an input argument to
 * this routine.
 */
ClRcT
omValidateHandle(ClHandleT handle)
{
	ClOmClassControlBlockT * pTab;
	ClOmClassTypeT   classId = CL_OM_HANDLE_TO_CLASSID(handle);
	ClUint32T instId  = CL_OM_HANDLE_TO_INSTANCEID(handle);

    CL_FUNC_ENTER();

    /* validate the input argument class Id field */
	if (omClassTypeValidate(classId) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid classID field within handle argument"));
	return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
    }
    pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);

    if (instId > pTab->maxObjInst)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid instance ID field within handle argument"));
	return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_INSTANCE));
    }

    if (!mGET_REAL_ADDR(pTab->pInstTab[instId]))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("The Instance Table for this class is not initialized"));
	return (CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_NOINIT));
    }
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 * Get class name
 * 
 * Gives class name given the OM Handle.
 *
 * @param handle - OM Handle
 * @param nameBuf - [OUT]Buffer where class name is copied
 *
 * @returns CL_OK if the handle is valid and class name is obtained
 *					successfully
 *@returns CL_OM_ERR_INVALID_OBJ_HANDLE if the passed object handle 
 *					 is invalid
 */
ClRcT
clOmClassNameGet(ClHandleT handle, char *nameBuf)
{
	ClOmClassControlBlockT * pTab = NULL;

	CL_FUNC_ENTER();
	CL_ASSERT(nameBuf);

	if (omValidateHandle(handle) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input handle argument"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE));
	}

	if (!nameBuf)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL name buffer passed"));
		return (CL_ERR_NULL_POINTER);
	}
    pTab = clOmClassEntryGet(CL_OM_HANDLE_TO_CLASSID(handle));
	CL_ASSERT(pTab);
	strcpy(nameBuf, (const char*) pTab->className);
	CL_FUNC_EXIT();
	return (CL_OK);
}

/**
 * Set class name
 * 
 * Sets the class name in the class control block of the class associated
 * with given OM handle.
 *
 * @param handle - OM Handle
 * @param nameBuf - class name to be set
 *
 * @returns CL_OK if the handle is valid and class name is obtained
 *					successfully
 *@returns CL_OM_ERR_INVALID_OBJ_HANDLE if the passed object handle 
 *					 is invalid
 * @returns CL_OM_ERR_MAX_NAMEBUF_EXCEEDED if the class name is
 *					too big (exceeds 80 characters)
 */
ClRcT
clOmClassNameSet(ClHandleT handle, char *nameBuf)
{
	ClOmClassControlBlockT * pTab = NULL;
	CL_FUNC_ENTER();
	CL_ASSERT(nameBuf);

        /* sanity check for input parameters */
        if(nameBuf == NULL)
        {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL input argument"));
		return (CL_OM_SET_RC(CL_OM_ERR_NULL_PTR));
	}
	if (omValidateHandle(handle) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input handle argument"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE));
	}

	/*
	 * Check the length of the input string, once the CW Object Creator
	 * is modified to understand the new disptach table structure can
	 * take any length. TODO: To change the className[] to char * and
	 * allocate the name buffer dynamically.
	 */
	if (strlen(nameBuf) > 80)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Maximum length exceeded"));
		return (CL_OM_SET_RC(CL_OM_ERR_MAX_NAMEBUF_EXCEEDED));
	}
    pTab = clOmClassEntryGet(CL_OM_HANDLE_TO_CLASSID(handle));
	CL_ASSERT(pTab);
	strcpy(pTab->className, nameBuf);
	CL_FUNC_EXIT();
	return (CL_OK);
}

/**
 * Walk through all the objects of given class
  *
 * This routine walks through all the object instances within the class
 * of objects. The ClOmClassTypeT argument with value zero walks through all
 * the object instances that are within the Object Manager repository. 
 * For every object instance, the routine that is associated with the
 * "fp" argument is called with the object reference as the first
 *
 * @param classId - Class Id of the class for which walk is to be invoked.
 *					0 means walk all the objects in OM.
 * @param fp - Pointer to the function to be invoked for each object.
 *
 * @returns CL_OK if the walk is successful
 * @returns CL_OM_ERR_INVALID_CLASS if passed classId is invalid
 * @returns CL_OM_ERR_INSTANCE_TAB_NOINIT if no instance for the class is present
*/

ClRcT
clOmObjectWalk(ClOmClassTypeT classId, ClRcT (*fp)(void *, void *), void *arg)
{
	int         idx;
	ClOmClassControlBlockT * pTab = NULL;
	ClUint32T i = 0, maxCount = 0;
	ClRcT rc;

	CL_FUNC_ENTER();

	if(fp == NULL)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                				("NULL pointer passed for function pointer"));
		return CL_ERR_NULL_POINTER;
	}

	if(classId)
	{
		if (omClassTypeValidate(classId) != CL_OK)
    	{
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            	    ("Invalid classID"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
    	}
		i = classId;
		maxCount = classId + 1;
	}
	else
	{
		i = 1;
		maxCount = gOmCfgData.maxAppOmClassType;
	} 
	for (;i < maxCount; i++)
	{
		pTab= clOmClassEntryGet(i); 
		if( pTab == NULL ) {
			continue;
		}

		if (!pTab->pInstTab)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,
					("Instance table not initialized"));
			return (CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_NOINIT));
		}

		for (idx =0; idx < pTab->maxObjInst; idx++)
		{
			if (mGET_REAL_ADDR(pTab->pInstTab[idx]))
			{
				if ((rc = fp((void *)mGET_REAL_ADDR(pTab->pInstTab[idx]), arg)) != CL_OK)
				{
					CL_FUNC_EXIT();
					return (rc);
				}
			}
		}
	}
	CL_FUNC_EXIT();
	return (CL_OK);
}


/**
 * NOT IMPLEMENTED
 *
  * This routine translates the global address into a local address.
  *
  */
ClRcT
omGetLocalAddr(ClHandleT globalAddr, ClHandleT *localAddr)
{
	CL_ASSERT(localAddr);
	return (CL_OK);

}


/**
 * NOT IMPLEMENTED
 * This routine translates the local address into a global address.
 */
ClRcT
omGetGlobalAddr(ClHandleT localAddr, ClHandleT *globalAddr)
{
	CL_ASSERT(globalAddr);

	return (CL_OK);
}

/**
 * This routine returns the class of the super class of the parent class
 */
ClRcT
clOmParentClassGet(ClHandleT handle, ClOmClassTypeT *peParentClass)
{
	ClOmClassTypeT classId = CL_OM_HANDLE_TO_CLASSID(handle); 
	ClOmClassControlBlockT * pTab = NULL;

	CL_FUNC_ENTER();

	/* sanity check for input parameters */
	if(NULL == peParentClass)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL input argument"));
		return CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
	}

	if (omClassTypeValidate(classId) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,
					("Invalid classID field within handle argument"));
		return CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS);
	}
	pTab = clOmClassEntryGet(classId);
	if(NULL == pTab)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Class Entry not found for classId %d\n",classId));
		return CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS);
	}
	*peParentClass = pTab->eParentObj;

	CL_FUNC_EXIT();
	return CL_OK;
}


/**
 * Get number of instances for given OM.
 *
 * This routine returns the total number of instances that are managed
 * by Object Manager for given class.
 *
 * @param handle - OM Handle
 * @param numInstances - [OUT]number of instances the class has
 *
 * @returns CL_OK if number of instances could be obtained successfully
 * @returns  CL_OM_ERR_INVALID_CLASS if the class passed by means of handle is
 *								is not valid
 */
ClRcT
clOmNumberOfOmClassInstancesGet(ClHandleT handle, ClUint32T *numInstances)
{
	ClOmClassTypeT classId = CL_OM_HANDLE_TO_CLASSID(handle);
	ClOmClassControlBlockT * pTab = NULL;

	CL_FUNC_ENTER();
	CL_ASSERT(numInstances);

        /* sanity check for input parameters */
        if(numInstances == NULL)
        {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL input argument"));
		return (CL_OM_SET_RC(CL_OM_ERR_NULL_PTR));
	}

	if (omClassTypeValidate(classId) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid classID field within handle argument"));
	return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
    }
    pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);
	*numInstances = pTab->curObjCount;

	CL_FUNC_EXIT();
	return (CL_OK);
}

/**
 * Set class Version
 *
 * This routine sets the current version of class of objects to that
 * provided in the input argument.
 *
 * @param handle - OM handle
 * @param version - Version to be set
 *
 * @returns CL_OK if number of instances could be obtained successfully
 * @returns  CL_OM_ERR_INVALID_CLASS if the class passed by means of handle is
 *								is not valid
*/

ClRcT
clOmClassVersionSet(ClHandleT handle, ClUint32T version)
{
	ClOmClassTypeT classId = CL_OM_HANDLE_TO_CLASSID(handle);
	ClOmClassControlBlockT * pTab = NULL;

	CL_FUNC_ENTER();

	if (omClassTypeValidate(classId) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid classID field within handle argument"));
	return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
    }
	/*
	 * TODO: Check if there is any conversion involved ?
	 *       Also, do we validate is new version if greater that old etc.
	 */
    pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);
	pTab->vVersion = version;

	CL_FUNC_EXIT();
	return (CL_OK);
}

/**
 * Get class version
 *
 * This routine returns the current version that is associated with the
 * class of objects.
 *
 * @param handle - OM handle
 * @param version - [OUT] Version of th class
 *
 * @returns CL_OK if number of instances could be obtained successfully
 * @returns  CL_OM_ERR_INVALID_CLASS if the class passed by means of handle is
 *								is not valid
 */
ClRcT
clOmClassVersionGet(ClHandleT handle, ClUint32T *version)
{
	ClOmClassTypeT classId = CL_OM_HANDLE_TO_CLASSID(handle);
	ClOmClassControlBlockT * pTab = NULL;

	CL_ASSERT(version);
	CL_FUNC_ENTER();

	if (omClassTypeValidate(classId) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid classID field within handle argument"));
	return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
    }

	if (!version)
	{
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL version argument passed"));
  	    return (CL_ERR_INVALID_PARAMETER);
	}

    pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);
	*version = pTab->vVersion;

	CL_FUNC_EXIT();
	return (CL_OK);
}

/**
 * Get OM object reference given handle
 *
 * This routine returns the reference to the OM object, from the input
 * OM object handle
 *
 * @param pThis - OM Object Handle
 * @param objRef - [OUT]Object reference
 *
 * @returns CL_OK if number of instances could be obtained successfully
 * @returns  CL_OM_ERR_INVALID_OBJ_HANDLE if the object handle passed
 *								is not valid
 */
ClRcT 
clOmObjectReferenceByOmHandleGet(ClHandleT pThis, void **objRef)
{
	ClOmClassControlBlockT * 	pTab = NULL;
	ClOmClassTypeT     classId;
	ClUint32T    	instId;

	CL_FUNC_ENTER();

	CL_ASSERT(pThis);
	CL_ASSERT(objRef);

	if (!objRef)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer passed"));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF));
	}

	if (omValidateHandle(pThis) != CL_OK)
	{
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input handle argument"));
	     return (CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE));
	}

	classId = CL_OM_HANDLE_TO_CLASSID(pThis);
	instId  = CL_OM_HANDLE_TO_INSTANCEID(pThis);
        pTab = clOmClassEntryGet(classId);
	CL_ASSERT(pTab);
	*objRef = (char *)mGET_REAL_ADDR(pTab->pInstTab[instId]);
	return (CL_OK);
}

/**
 * Get OM Handle given object reference.
 *
 * This routine returns the OM object handle, given the reference to the
 * om Object.
 *
 * @param pThis - Object reference.
 * @param pHandle - [OUT] Object handle for given object.
 *
 * @returns CL_OK if number of instances could be obtained successfully
 * @returns  CL_OM_ERR_INVALID_OBJ_HANDLE if the object handle passed
 *								is not valid
 */ 
ClRcT 
clOmOmHandleByObjectReferenceGet(void *pThis, ClHandleT *pHandle)
{
	ClRcT		rc = CL_OK;
#ifdef CL_DEBUG
	ClUint8T	aFuncName[] = "clOmOmHandleByObjectReferenceGet()";
#endif

	CL_FUNC_ENTER();

	if (!pThis)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: NULL pointer passed\r\n", aFuncName));
		rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_REF);
		goto ERROR_CASE;
	}

	if (!pHandle)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: NULL pointer passed\r\n", aFuncName));
		rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_OBJ_HANDLE);
		goto ERROR_CASE;
	}

	*pHandle = ((struct CL_OM_BASE_CLASS *)pThis)->__objType;
	return (CL_OK);

	ERROR_CASE: 
		if(pHandle)
			*pHandle = 0;
        return (rc);
}



/**
 *  Validates an OM class type.                          
 *
 *  This API validates the ranges of valid OM class types.
 *
 *  @param classType the class type to validate
 *
 *  @returns
 *    CL_OK - the class type is valid<br>
 *     CL_ERR_INVLD_STATE - wrong initialization state<br>
 *    Runlevel error code - please reference the Runlevel error codes
 *
 */
ClRcT
omClassTypeValidate(ClOmClassTypeT classType)
	{
#ifdef CL_DEBUG
	ClUint8T	aFuncName[] = "omClassTypeValidate()";
#endif

    CL_FUNC_ENTER();

    /* validate the input argument class Id field */
    if (!CL_OM_CMN_CLASS_VALIDATE(classType) &&
		!CL_OM_APP_CLASS_VALIDATE(classType))
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,
			("%s: Invalid class type (0x%x)!!!\r\n", aFuncName, classType));
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
		}
	
    CL_FUNC_EXIT();
    return (CL_OK);
	}


/**
 *  Returns an OM class entry.
 *
 *  This function returns a pointer to an OM class entry given 
 * OM class Id.
 *                                                                        
 *  @param omClassKey - OM class type
 *
 *  @returns 
 *    Pointer to class control block
 */
ClOmClassControlBlockT *
clOmClassEntryGet(ClOmClassTypeT omClassKey)
{
	ClRcT			rc = CL_OK;
#ifdef CL_DEBUG
	ClUint8T		aFuncName[] = "clOmClassEntryGet()";
#endif
	ClCntNodeHandleT tempNodeHandle = 0;
	ClOmClassControlBlockT 		*pClassEntry = 0;

    CL_FUNC_ENTER();

	if ((rc = omClassTypeValidate(omClassKey)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Invalid input OM class (rc = 0x%x)!\r\n", 
			aFuncName, rc));
		CL_ASSERT(NULL);
		CL_FUNC_EXIT();
		return NULL;
	}

	/* find the node for this MO ID */
	if ((rc = clCntNodeFind(ghOmClassHashTbl, 
		(ClCntKeyHandleT)(ClWordT)omClassKey, &tempNodeHandle)) != CL_OK)
		{
		if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
			{
			CL_DEBUG_PRINT(CL_DEBUG_INFO, ("%s: Node does not exist for OM class "
				"(0x%x), with rc 0x%x!\r\n", aFuncName, omClassKey, rc));
			}
		else
			{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to find node for OM class "
				"(0x%x), with rc 0x%x!\r\n", aFuncName, omClassKey, rc));
			}
		return NULL;
		}
	/* now get the OM handle for this MO ID */
	if ((rc = clCntNodeUserDataGet(ghOmClassHashTbl, 
		tempNodeHandle, (ClCntDataHandleT *)&pClassEntry)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to get the OM class entry from the "
			"node for OM class 0x%x, with rc 0x%x!\r\n", aFuncName, 
			omClassKey, rc));
		CL_ASSERT(NULL);
		return  NULL;
		}
 	
   	return pClassEntry;
}


/**
 *  Show OM information
 *
 *  This routine goes through all the classes and display all the objects
 * created for that class.
 *                                                                        
 */
void omShow(char **params ){

	ClOmClassControlBlockT *pTab=NULL;
	char omClassName[100];
	ClUint32T i=0;
	extern ClOmConfigDataT gOmCfgData; 
printf("\n############# maxAppOmClass Type %d ##############",
        gOmCfgData.maxAppOmClassType );

if (isalpha(**params))
 {

	sscanf(*params,"%s",  omClassName); 

       for ( i= 1 ;i < gOmCfgData.maxAppOmClassType ; i++) 
       {

	     pTab= clOmClassEntryGet(i); 
             if( pTab == NULL )
             {
               continue;
             }

             if (strstr(pTab->className , *params)!=NULL )
             {		
 		printf("\nOM class Name         : %s ", pTab->className ); 
		printf("\nno of current objects : %d ", pTab->curObjCount); 

                 /*@todo: Navdeep (has references to cim base class.. which is deleted now.)
                  if(pTab->curObjCount !=0 ) 
                 displayObjects(pTab); */
             }
        }/* end for */
   }

}

/**
 *  Display OM objects
 *
 * Display OM object information.
 *                                                                        
 */
#if 0
void displayObjects(ClOmClassControlBlockT *pTab ){

    int idx =0 ;

/* walk through the instance table and call the display function passing the
 * reference to that object */

	for (idx =0; idx < pTab->curObjCount ; idx++){
        /* call the display method of the object */
       ( (CimBaseDataClass *) mGET_REAL_ADDR( pTab->pInstTab[idx]) ) ->
           fpDisplayObject( (void *)mGET_REAL_ADDR(pTab->pInstTab[idx]) );	

    }
}

#endif
