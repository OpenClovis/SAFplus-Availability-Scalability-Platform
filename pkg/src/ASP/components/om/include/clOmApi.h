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
 * File        : clOmApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  The file contains OM APIs and related data types.
 *
 *
 *****************************************************************************/
#ifndef _CL_OM_API_H_
#define _CL_OM_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorServiceId.h>
#include "clOmCommonClassTypes.h"

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/* Local object handle related macros. */
#define CL_OM_FORM_HANDLE(X,Y)	((X << 16) | (Y & 0x0000FFFF))
#define CL_OM_HANDLE_TO_CLASSID(X)	(X >> 16)
#define CL_OM_HANDLE_TO_INSTANCEID(X)	(X & 0x0000FFFF)

	

/* Common OM class type value range checking */
#define CL_OM_CMN_CLASS_VALIDATE(omClassType)	\
			((omClassType >= CL_OM_CMN_CLASS_TYPE_START) && \
			 (omClassType < CL_OM_CMN_CLASS_TYPE_END))

#define CL_OM_APP_CLASS_VALIDATE(omClassType)	\
			((omClassType >= gOmCfgData.minAppOmClassType) && \
			 (omClassType < gOmCfgData.maxAppOmClassType))
	
/******************************************************************************
 *  Data Types 
 *****************************************************************************/
/*@todo: Navdeep to remove these typedefs.*/
typedef void    tFunc();
typedef ClRcT (*ifp)(void*, void*, ClUint32T);

			
/* Structure definition of the class control block managed by the dispatch  */
typedef struct ClOmClassControlBlock ClOmClassControlBlockT;
struct ClOmClassControlBlock {

  	char         className[80];    /* Class name for info only (optional) */
	ClUint32T    size;             /* size of the object this CB refers to */
	ClOmClassTypeT    eParentObj;       /* parent object this is derived from */
	ifp          fpInit;           /* object initialization routine */
	ifp          fpDeInit;         /* object de-initialization routine */
	tFunc**        pfpMethodsTab;    /* methods associated with this object */
	ClUint32T    vVersion;         /* Version associated with this class */
        ClUint32T    **pInstTab;         /* Instance table pointer */ 
	ClUint32T    maxObjInst;       /* Maximum number of object instances */
	ClUint32T    curObjCount;      /* count of active number of instances */
	ClUint32T    maxSlots;         /* count of active number of instances */
	ClOmClassTypeT   eMyClassType;     /* My class type */
};
	
typedef struct ClOmConfigData
	{
	ClUint32T	maxOmClassBuckets;	/* Number of lookup buckets in hash table */
	ClInt32T	minAppOmClassType;	/* Lowest value of app OM class type */
	ClInt32T	maxAppOmClassType;	/* Upper value of app OM class type */
	ClUint32T	numOfOmClasses;		/* Number of OM classes registered */
	} ClOmConfigDataT;

typedef struct ClOIConfig
{
    ClBoolT oiDBReload; /* flag to indicate whether OIs db has to be reloaded or not*/
    const ClCharT  *pOIClassPath; /*class path for the OI resources*/
    const ClCharT  *pOIClassFile; /*class file for the OI resources*/
    const ClCharT  *pOIRoutePath; /*route path for the OI resources*/
    const ClCharT  *pOIRouteFile; /*route file for the OI resources*/
    const ClCharT  *pOIPMPath;    /*performance monitoring config file path*/
    const ClCharT  *pOIPMFile;    /*performance monitoring config file*/
}ClOIConfigT;

/* WARNING: clOmInitialize() must be invoked b4 using this structure */
extern ClOIConfigT gClOIConfig;
extern ClOmConfigDataT gOmCfgData;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/* -- OM Initialization function -- */
/**
 *  Initializes the OM class table.                          
 *
 *  This API initializes the OM class lookup table from the pre-configured<br>
 *    tables: common and application.  It also initializes other tables <br>
 *    and prepares the OM library for usage by other components.
 *
 *  @param pOmCfgData Configuration data for OM
 *
 *  @returns
 *    CL_RC_OK - everything is ok<br>
 *    CL_ERR_INVLD_STATE - wrong initialization state<br>
 *    Runlevel error code - please reference the Runlevel error codes
 *
 */
extern ClRcT clOmLibInitialize();
extern ClRcT clOmLibInitializeExtended(void);
/* -- OM ObjectId/ClCorMOId mapping -- */

/**
 *  Add an entry in MOId to OMId mapping table.
 *
 *  This API adds given om handle entry for given MOID into the
 * hash table which maintains MOID to OMID mapping.
 *                                                                        
 *  @param hMoIdKey       - MOID handle for COR object. Serves as a key for 
 *  						accessing the MOId to OMId mapping table
 *  @param hOm      - OM Handle for OM object which is associated with COR object
 *  						represented by the MOId.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmMoIdToOmIdMapInsert(ClCorMOIdPtrT hMoIdKey, ClHandleT hOm);

/**
 *  Remove the entry from moId To Om Hash Tblle.
 *
 *  This API removes an entry corresponding to MOId (key) from the
 *  MOId to OMId mapping table.
 *                                                                        
 *  @param hMoIdKey       - MOId handle for COR object. 
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmMoIdToOmIdMapRemove(ClCorMOIdPtrT hMoIdKey); 

/**
 *  Get the OM Handle given MOId.
 *
 *  This API returns OM Handle given MOId from the MOId to 
 *  OMId mapping table. If the OM Handle can not be found it
 *  returns NULL 
 *                                                                        
 *  @param hMoIdKey       - MOID handle for COR object. Serves as a key for 
 *  						accessing the MOId to OMId mapping table
 *  @param pOmH      - [OUT] OM Handle for OM object which is associated with COR object
 *  						represented by the MOId.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmMoIdToOmHandleGet(ClCorMOIdPtrT hMoIdKey, ClHandleT *pOmH);


/**
 *  Get the MOId given OM Handle
 *
 *  This API returns MOId given OM Handle from the OMId to 
 *  MOId mapping table. If the MOId can not be found it
 *  returns NULL 
 *                                                                        
 *  @param hOmKey       - OM handle for OM object. Serves as a key for 
 *  						accessing the OMId to MOId mapping table
 *  @param pOmH      - [OUT] MOId for COR object which is associated with MO object
 *  						represented by the OM Handle.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmOmHandleToMoIdGet(ClHandleT hOmKey, ClCorMOIdPtrT *phMoId);


/**
 *  Get the MOId given OM Handle
 *
 *  This API returns MOId given OM Handle from the OMId to 
 *  MOId mapping table. If the MOId can not be found it
 *  returns NULL 
 *                                                                        
 *  @param hOmKey       - OM handle for OM object. Serves as a key for 
 *  						accessing the OMId to MOId mapping table
 *  @param pOmH      - [OUT] MOId for COR object which is associated with MO object
 *  						represented by the OM Handle.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmMoIdToOmObjectReferenceGet(ClCorMOIdPtrT hMoIdKey, void **ppOmObj);

/**
 *  Get the MOId given OM object reference.
 *
 *  This API returns MOId of a COR object given object reference
 *  to OM object.
 *                                                                        
 *  @param pOmObjRefKey       - Reference to OM object. 
 *  @param phMoId      - [OUT] Pointer to MOId handle.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmOmObjectReferenceToMoIdGet(void *pOmObjRefKey, ClCorMOIdPtrT *phMoId);

/* -- Class Initialisation/Finalisation -- */


/**
 *                                                                       
 * clOmClassInitialize  - Initializes the OM class.   
 *                                                                       
 * This routine initalizes the Object Manager and configures the Object  
 * Manager repository for the users of the library to create, delete     
 * and manage the objects. The Object Manager database comprises of a    
 * multi-level table, where the first level is the class table and the   
 * second level is the instance table.                                   
 *                                                                       
 * @param pClassTab - Pointer to class control block.
 * @param classId - classId of the class
 * @param maxInstances - Maximum  number of instances the class can have.
 * @param maxSlots - Not used.
 *
 *  @returns CL_RC_OK if the init is successful
 */
extern ClRcT clOmClassInitialize (ClOmClassControlBlockT *pClassTab, ClOmClassTypeT classId, ClUint32T maxInstances, ClUint32T maxSlots);


/**
 * Release resources for allocated class
 *  
 * This routine undoes the operation that was performed in the
 * clOmClassInitialize () Object Manager Library interface. This is called to
 * release the resources that was allocated to facilitate a clean
 * unload of the Object Manager Library.
 *                                                                       
 * @param pClassTab - Class control block.
 * @param classId - Class Id of the class
 * 
 * @returns CL_RC_OK if all the clean up operations are successful
 */
extern ClRcT clOmClassFinalize(ClOmClassControlBlockT *pClassTab, ClOmClassTypeT classId);


/* -- OM Object management API -- */

/**
 * Create OM Object
 *
 * This routine creates an object instance in the calling context, and
 * sets the object type the one specified as the argument. Also, the 
 * object instance is added to the object manager database.
 *
 * @param classId - class Id of class for which the object has to be created.
 * @param numInstances - Number of instances to be created
 * @param handle - [OUT] The handle to first object created 
 * @param pOmObj - [OUT] Pointer to the first OM object created
 * @param pUsrData - User data. Passed to the user defined init function 
 *					in the Class Control Block for the class.
 * @param usrDataLen - User data length
 *
 * @returns return value from the user application constructor.
 */
extern ClRcT clOmObjectCreate(ClOmClassTypeT classId, ClUint32T numInstances, 
	ClHandleT *handle, void** pOmObj, void *pUsrData, ClUint32T usrDataLen);

/**
 *  Create OM & COR objects given OM class Id & MOId
 *
 *  This API creates OM as well as COR objects. For creating
 *  OM object, user need to specify classId and MOId of the
 *  COR Object corresponding to this OM object. In order to
 *  create COR object the user need to specify MOId.
 *                                                                        
 *  @param classId       - Class Id for the OM class
 *  @param omHandle	- [OUT]OM Handle to OM object created
 *  @param objPtr	- 	[OUT] Reference to created OM Object
 *  @param  moId	-	MOId of the COR object to be created.
 *  @param svcId	-	Service Id of service for which the COR and
 *					OM objects are to be created
 *  @param corHandle      - [OUT] Handle to the COR object created.
 *
 *  @returns CL_RC_OK  - Success<br>
 */
extern ClRcT clOmCorAndOmObjectsCreate(ClOmClassTypeT classId, ClHandleT *omHandle, 
	void **objPtr, ClCorMOIdPtrT moId, ClUint32T svcId, ClCorObjectHandleT *corHandle);

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
 * @returns - CL_RC_OK if all the validations and delete operation is successful
 */
extern ClRcT clOmObjectDelete(ClHandleT handle, ClUint32T numInstances, 
	void *pUsrData, ClUint32T usrDataLen);

/**
 * Delete OM object given reference.
 *
 * This routine deletes the object given an input reference to the object
 * 
 * @param pThis - Object reference of the object to be deleted.
 *
 * @returns CL_RC_OK if the delete operation is successful
 *
 */
extern ClRcT clOmObjectByReferenceDelete(void *pThis);

/* -- OM Utilities --- */
/**
 * Get class name
 * 
 * Gives class name given the OM Handle.
 *
 * @param handle - OM Handle
 * @param nameBuf - [OUT]Buffer where class name is copied
 *
 * @returns CL_RC_OK if the handle is valid and class name is obtained
 *					successfully
 *@returns OM_ERR_INVLD_OBJ_HANDLE if the passed object handle 
 *					 is invalid
 */
extern ClRcT clOmClassNameGet(ClHandleT handle, char *nameBuf);

/**
 * Set class name
 * 
 * Sets the class name in the class control block of the class associated
 * with given OM handle.
 *
 * @param handle - OM Handle
 * @param nameBuf - class name to be set
 *
 * @returns CL_RC_OK if the handle is valid and class name is obtained
 *					successfully
 *@returns OM_ERR_INVLD_OBJ_HANDLE if the passed object handle 
 *					 is invalid
 * @returns OM_ERR_MAX_NAMEBUF_EXCEEDED if the class name is
 *					too big (exceeds 80 characters)
 */
extern ClRcT clOmClassNameSet(ClHandleT handle, char *nameBuf);

/**
 * Walk through all the objects of given class
  *
 * This routine walks through all the object instances within the class
 * of objects. The classType_e argument with value zero walks through all
 * the object instances that are within the Object Manager repository. 
 * For every object instance, the routine that is associated with the
 * "fp" argument is called with the object reference as the first
 *
 * @param classId - Class Id of the class for which walk is to be invoked.
 *					0 means walk all the objects in OM.
 * @param fp - Pointer to the function to be invoked for each object.
 *
 * @returns CL_RC_OK if the walk is successful
 * @returns OM_ERR_INVLD_CLASS if passed classId is invalid
 * @returns OM_ERR_INSTANCE_TAB_NOINIT if no instance for the class is present
*/
extern ClRcT clOmObjectWalk(ClOmClassTypeT classId, ClUint32T (*fp)(void *, void *), void *arg);


/**
 * This routine returns the class of the super class of the parent class
 */
extern ClRcT clOmParentClassGet(ClHandleT handle, ClOmClassTypeT *eParentClass);

/**
 * Get number of instances for given OM.
 *
 * This routine returns the total number of instances that are managed
 * by Object Manager for given class.
 *
 * @param handle - OM Handle
 * @param numInstances - [OUT]number of instances the class has
 *
 * @returns CL_RC_OK if number of instances could be obtained successfully
 * @returns  OM_ERR_INVLD_CLASS if the class passed by means of handle is
 *								is not valid
 */
extern ClRcT clOmNumberOfOmClassInstancesGet(ClHandleT handle, ClUint32T *numInstances);

/**
 * Get number of instances for given OM.
 *
 * This routine returns the total number of instances that are managed
 * by Object Manager for given class.
 *
 * @param handle - OM Handle
 * @param numInstances - [OUT]number of instances the class has
 *
 * @returns CL_RC_OK if number of instances could be obtained successfully
 * @returns  OM_ERR_INVLD_CLASS if the class passed by means of handle is
 *								is not valid
 */
extern ClRcT clOmClassVersionSet(ClHandleT handle, ClUint32T version);

/**
 * Get class version
 *
 * This routine returns the current version that is associated with the
 * class of objects.
 *
 * @param handle - OM handle
 * @param version - [OUT] Version of th class
 *
 * @returns CL_RC_OK if number of instances could be obtained successfully
 * @returns  OM_ERR_INVLD_CLASS if the class passed by means of handle is
 *								is not valid
 */
extern ClRcT clOmClassVersionGet(ClHandleT handle, ClUint32T *version);


/**
 * Get OM object reference given handle
 *
 * This routine returns the reference to the OM object, from the input
 * OM object handle
 *
 * @param pThis - OM Object Handle
 * @param objRef - [OUT]Object reference
 *
 * @returns CL_RC_OK if number of instances could be obtained successfully
 * @returns  OM_ERR_INVLD_OBJ_HANDLE if the object handle passed
 *								is not valid
 */
extern ClRcT clOmObjectReferenceByOmHandleGet(ClHandleT pThis, void **objRef);


/**
 * Get OM object reference given handle
 *
 * This routine returns the reference to the OM object, from the input
 * OM object handle
 *
 * @param pThis - OM Object Handle
 * @param objRef - [OUT]Object reference
 *
 * @returns CL_RC_OK if number of instances could be obtained successfully
 * @returns  OM_ERR_INVLD_OBJ_HANDLE if the object handle passed
 *								is not valid
 */
extern ClRcT clOmOmHandleByObjectReferenceGet(void *pThis, ClHandleT *handle);


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
extern ClOmClassControlBlockT * clOmClassEntryGet(ClOmClassTypeT omClassKey);

extern ClRcT clOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass);

/**
 *  Finalize function for OM
 *
 */
extern ClRcT  clOmLibFinalize();




#ifdef __cplusplus
}
#endif

#endif  /* _CL_OM_API_H_ */
