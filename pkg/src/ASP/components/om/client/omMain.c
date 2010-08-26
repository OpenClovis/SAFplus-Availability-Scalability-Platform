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
 * File        : omMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the routines that create/delete and manage the   
 * objects in the system. This module is part of the Object Manager      
 * component.                                                            
 *************************************************************************/
#include <string.h>
#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clOmErrors.h>
#include <clOsalApi.h>
#include <clCksmApi.h>
#include <clCorUtilityApi.h>
/* Internal Headers */
#include "clOmDefs.h"
#include "omPrivateDefs.h"

/* Configuration related parameters  */
#define OM_CLASS_TABLE_MAX_BUCKETS 50

/*@todo : Jayesh : Move this to configuration */

#define OM_APP_MIN_CLASS_TYPE (CL_OM_CMN_CLASS_TYPE_END + 1)
#define OM_APP_MAX_CLASS_TYPE (CL_OM_CMN_CLASS_TYPE_END + 1024)


/* The following should be defined in the configuration
      and linked along with Om library*/
extern ClOmClassControlBlockT pAppOmClassTbl[];
extern ClUint32T appOmClassCnt;

/* Following definitions are not configurable. */
extern ClOmClassControlBlockT gBaseClassTable; 
/**
 * 
 *  Indicates if om class init is done or not
 *
 * DO NOT DOCUMENT. INTERNAL DATA STRUCTURE.
 */
ClUint32T omClassInitState = CL_FALSE;

/* GLOBALS */
/**
 * 
 *  Lookup table (hash table) for OM classes 
 *
 * DO NOT DOCUMENT. INTERNAL DATA STRUCTURE.
 */
ClCntHandleT 	ghOmClassHashTbl = 0;
ClCntHandleT    ghOmClassNameHashTbl = 0;

ClOIConfigT gClOIConfig;

/**
 * 
 *  Configuration data for OM. Following is the list of configurable parameters
 * for ASP
 * 
 *	ClUint32T	maxOmClassBuckets;	// Number of lookup buckets in hash table <br>
 *	ClOmClassControlBlockT 	*pAppOmClassTbl;	// Application OM class table <br>
 *	ClUint16T	appOmClassCnt;		// Number of classes <br>
 *	ClInt32T	minAppOmClassType;	// Lowest value of app OM class type <br>
 *	ClInt32T	maxAppOmClassType;	// Upper value of app OM class type <br>
 *	ClUint32T	numOfOmClasses;		// Number of OM classes registered <br>
 *
 */
ClOmConfigDataT				gOmCfgData;
/**
 *  Class Table for ASP classes.
 *
 * This table consists of definition of all OM classes present in ASP. Such a table also
 * need to be created for all the applications. This table contains following information
 * for each class
 *
 *  	ClUint8T     className[80];    // Class name for info only (optional) <br>
 *	ClUint32T    size;             // size of the object this CB refers to <br>
 *	ClOmClassTypeT    eParentObj;       // parent object this is derived from <br>
 *	ifp            fpInit;           // object initialization routine <br>
 *	ifp            fpDeInit;         // object de-initialization routine <br>
 *	tFunc**        pfpMethodsTab;    // methods associated with this object <br>
 *	ClUint32T    vVersion;         // Version associated with this class <br>
 *        !!!OBSOLETEclUintPtr_t!!!*  pInstTab;         // Instance table pointer <br>
 *	ClUint32T    maxObjInst;       // Maximum number of object instances <br>
 *	ClUint32T    curObjCount;      // count of active number of instances <br>
 *	ClUint32T    maxSlots;         // count of active number of instances <br>
 *	ClOmClassTypeT   eMyClassType;     // My class type <br>
 */


/* FUNCTION_PROTOTYPES */
ClRcT omCORTableInit(void);
static ClInt32T omClassKeyCompare(ClCntKeyHandleT key1, 
					ClCntKeyHandleT key2);
static ClInt32T omClassNameKeyCompare(ClCntKeyHandleT key1, 
					ClCntKeyHandleT key2);

static ClUint32T omClassHashFn(ClCntKeyHandleT key);
static ClUint32T omClassNameHashFn(ClCntKeyHandleT key);

static void omClassEntryDelCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);
static void omClassEntryDestroyCallBack(ClCntKeyHandleT userKey, 
				ClCntDataHandleT userData);

static ClRcT 
clOmClassNameInitialize (ClOmClassControlBlockT *pClassTab, char *pClassName, ClUint32T maxInstances, 
                         ClUint32T maxSlots);


ClRcT clOmLibInitializeExtended(void)
{
    ClRcT		rc = CL_OK;
    ClUint32T	classIdx = 0;
    ClUint32T	omCfgDataSz = sizeof(ClOmConfigDataT);
    ClOmClassControlBlockT	*pOmClassEntry = NULL;
    ClCharT oiClassFile[CL_MAX_NAME_LENGTH] = {0};
    ClCharT oiRouteFile[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pOIClassFile = oiClassFile;
    ClCharT *pOIRouteFile = oiRouteFile;
    extern ClRcT showMgrModAdd(char *, char *, void (*fp)(char**));
    extern void omShow (char ** );


    CL_FUNC_ENTER();

    /*
     *First load the OI class configuration. into COR
     */
    if(!gClOIConfig.pOIClassFile)
    {
        clLogError("OI", "CONFIG", "No OI class file is specified to configure the OI classes");
        return CL_OM_SET_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(!gClOIConfig.pOIRouteFile)
    {
        clLogError("OI", "CONFIG", "No OI route file is specified to configure the OI objects");
        return CL_OM_SET_RC(CL_ERR_INVALID_PARAMETER);
    }

    if(gClOIConfig.pOIClassPath)
        snprintf(oiClassFile, sizeof(oiClassFile), "%s/%s", gClOIConfig.pOIClassPath, 
                 gClOIConfig.pOIClassFile);
    else
        pOIClassFile = (ClCharT*)gClOIConfig.pOIClassFile;

    if(gClOIConfig.pOIRoutePath)
        snprintf(oiRouteFile, sizeof(oiRouteFile), "%s/%s", gClOIConfig.pOIRoutePath,
                 gClOIConfig.pOIRouteFile);
    else
        pOIRouteFile = (ClCharT*)gClOIConfig.pOIRouteFile;

    rc = clCorConfigLoad(pOIClassFile, pOIRouteFile);
    if(rc != CL_OK)
    {
        clLogError("OI", "CONFIG", "Loading OI class configuration [%s] to COR returned with error [%#x]",
                   pOIClassFile, rc);
        return rc;
    }

    /* 
       Initialize config data struct and make a 
       copy of the configuration data. 
    */
    memset(&gOmCfgData, 0, omCfgDataSz);
    /* Fill up the gOmCfgData */
    gOmCfgData.maxOmClassBuckets = OM_CLASS_TABLE_MAX_BUCKETS;
    gOmCfgData.minAppOmClassType = OM_APP_MIN_CLASS_TYPE;
    gOmCfgData.maxAppOmClassType = OM_APP_MAX_CLASS_TYPE;
    gOmCfgData.numOfOmClasses = 0;

    /* Create a hash table for storing OM classes */
    rc = clCntHashtblCreate(gOmCfgData.maxOmClassBuckets, omClassKeyCompare, 
                            omClassHashFn, omClassEntryDelCallBack, 
                            omClassEntryDestroyCallBack, CL_CNT_NON_UNIQUE_KEY, &ghOmClassHashTbl);

    rc |= clCntHashtblCreate(gOmCfgData.maxOmClassBuckets, omClassNameKeyCompare, 
                            omClassNameHashFn, omClassEntryDelCallBack, 
                            omClassEntryDestroyCallBack, CL_CNT_NON_UNIQUE_KEY, &ghOmClassNameHashTbl);

    if ((rc != CL_OK) || (ghOmClassHashTbl == 0) 
        || (ghOmClassNameHashTbl == 0))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("%s: Failed to create a hash table "
                                       "for OM classes (rc = 0x%x)!!!\n\r", __FUNCTION__, rc));
        return (rc);
    }


    /* Consolidate OM class tables into a hash table */
    /* Configure the common OM class table */

    /* Initialize the base class here */


    if ((rc = clOmClassInitialize (&gBaseClassTable, 
                                   gBaseClassTable.eMyClassType, 
                                   gBaseClassTable.maxObjInst,
                                   gBaseClassTable.maxSlots)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to initialize OM with rc = "
                                        "0x%x!\r\n", __FUNCTION__, rc));
        return (rc);
    }


    /* Accumulate the number of classes registered w/ OM */
    /*omClassCnt = classIdx;*/
    pOmClassEntry = NULL;

    /* Configure the application OM class table */
    classIdx = 0;

    if (appOmClassCnt > 0)
    {
        for (; classIdx < appOmClassCnt; classIdx++)
        {
            pOmClassEntry = &(pAppOmClassTbl[classIdx]);
            if (pOmClassEntry->eMyClassType == CL_OM_INVALID_CLASS)
            {
                /* skip invalid class */
                /* 
                   This way invalid classes and missing, i.e. a table
                   size if bigger than the number of classes
                   are allowed in the table.
                */
                /*skippedClassCnt++;*/
                continue;
            }

            if ((rc = clOmClassNameInitialize (pOmClassEntry, 
                                               pOmClassEntry->className,
                                               pOmClassEntry->maxObjInst,
                                               pOmClassEntry->maxSlots)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to initialize OM with rc = "
                                                "0x%x!\r\n", __FUNCTION__, rc));
                return (rc);
            }
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("%s: Not initializing OM Application class "
                                        "table because it does not exist...ignoring.\r\n",
                                        __FUNCTION__));
    }

    /* Get the total number of classes registered w/ OM */
    /*gOmCfgData.numOfOmClasses = omClassCnt + classIdx - skippedClassCnt;*/

    /* Initialize the COR/OM handle translation table */
    if (rc == CL_OK)
    {
        if((rc = omCORTableInit()) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to setup the COR translation "
                                            "table with rc = 0x%x\r\n", rc));
        }
    }

#ifdef DEBUG
    /* Add OM into the debug agent */
    if (!omClassInitState)
        dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
#endif

    omClassInitState = CL_TRUE;
    /* add the om to show manager */ 
    /*@todo: Navdeep (show manager dependency) 
      showMgrModAdd(COMP_PREFIX, COMP_NAME, omShow); 
    */
    CL_FUNC_EXIT();
    return (rc);
}

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
 *    CL_OK - everything is ok<br>
 *    ==>FIXME:Define(==>FIXME:Define(CL_ERR_INVLD_STATE)<==)<== - wrong initialization state<br>
 *    Runlevel error code - please reference the Runlevel error codes
 *
 */
ClRcT clOmLibInitialize()
{
    ClRcT		rc = CL_OK;
    ClUint32T	classIdx = 0;
    ClUint32T	omCfgDataSz = sizeof(ClOmConfigDataT);
    ClOmClassControlBlockT	*pOmClassEntry = NULL;

    /*
      ClUint32T      cmnOmClassCnt = 0; 
    */
    /*ClUint32T	omClassCnt = 0;
      ClUint32T	skippedClassCnt = 0;*/
    extern ClRcT showMgrModAdd(char *, char *, void (*fp)(char**));
    extern void omShow (char ** );


    CL_FUNC_ENTER();

    /*
     * Check if the OI has its own DB and reload its db
     */
    if(gClOIConfig.oiDBReload)
        return CL_OK;

    /* Get a count of the common classes */


    /* 
       Initialize config data struct and make a 
       copy of the configuration data. 
    */
    memset(&gOmCfgData, 0, omCfgDataSz);
    /* Fill up the gOmCfgData */
    gOmCfgData.maxOmClassBuckets = OM_CLASS_TABLE_MAX_BUCKETS;
    gOmCfgData.minAppOmClassType = OM_APP_MIN_CLASS_TYPE;
    gOmCfgData.maxAppOmClassType = OM_APP_MAX_CLASS_TYPE;
    gOmCfgData.numOfOmClasses = 0;

    /* Create a hash table for storing OM classes */
    rc = clCntHashtblCreate(gOmCfgData.maxOmClassBuckets, omClassKeyCompare, 
                            omClassHashFn, omClassEntryDelCallBack, 
                            omClassEntryDestroyCallBack, CL_CNT_NON_UNIQUE_KEY, &ghOmClassHashTbl);

    if ((rc != CL_OK) || (ghOmClassHashTbl == 0))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("%s: Failed to create a hash table "
                                       "for OM classes (rc = 0x%x)!!!\n\r", 
                                       __FUNCTION__, rc));
        return (rc);
    }


    /* Consolidate OM class tables into a hash table */
    /* Configure the common OM class table */

    /* Initialize the base class here */


    if ((rc = clOmClassInitialize (&gBaseClassTable, 
                                   gBaseClassTable.eMyClassType, 
                                   gBaseClassTable.maxObjInst,
                                   gBaseClassTable.maxSlots)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to initialize OM with rc = "
                                        "0x%x!\r\n", __FUNCTION__, rc));
        return (rc);
    }




    /* Accumulate the number of classes registered w/ OM */
    /*omClassCnt = classIdx;*/
    pOmClassEntry = NULL;

    /* Configure the application OM class table */
    classIdx = 0;

    if (appOmClassCnt > 0)
    {
        for (; classIdx < appOmClassCnt; classIdx++)
        {
            pOmClassEntry = &(pAppOmClassTbl[classIdx]);
            if (pOmClassEntry->eMyClassType == CL_OM_INVALID_CLASS)
            {
                /* skip invalid class */
                /* 
                   This way invalid classes and missing, i.e. a table
                   size if bigger than the number of classes
                   are allowed in the table.
                */
                /*skippedClassCnt++;*/
                continue;
            }

            if ((rc = clOmClassInitialize (pOmClassEntry, 
                                           pOmClassEntry->eMyClassType, 
                                           pOmClassEntry->maxObjInst,
                                           pOmClassEntry->maxSlots)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to initialize OM with rc = "
                                                "0x%x!\r\n", __FUNCTION__, rc));
                return (rc);
            }
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("%s: Not initializing OM Application class "
                                        "table because it does not exist...ignoring.\r\n",
                                        __FUNCTION__));
    }

    /* Get the total number of classes registered w/ OM */
    /*gOmCfgData.numOfOmClasses = omClassCnt + classIdx - skippedClassCnt;*/

    /* Initialize the COR/OM handle translation table */
    if (rc == CL_OK)
    {
        if((rc = omCORTableInit()) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to setup the COR translation "
                                            "table with rc = 0x%x\r\n", rc));
        }
    }

#ifdef DEBUG
    /* Add OM into the debug agent */
    if (!omClassInitState)
        dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
#endif

    omClassInitState = CL_TRUE;
    /* add the om to show manager */ 
    /*@todo: Navdeep (show manager dependency) 
      showMgrModAdd(COMP_PREFIX, COMP_NAME, omShow); 
    */
    CL_FUNC_EXIT();
    return (rc);
}


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
 *  @returns CL_OK if the init is successful
 */
 /* @todo : Jayesh : Remove the last three parameters of the function*/
ClRcT 
clOmClassInitialize (ClOmClassControlBlockT *pClassTab, ClOmClassTypeT classId, ClUint32T maxInstances, 
       ClUint32T maxSlots)
{
	ClUint32T **tmp_ptr;
	ClUint32T  tmp_size;
	ClRcT		rc = CL_OK;

	CL_ASSERT(pClassTab);
    CL_FUNC_ENTER();

        /* Sanity check for pClassTab */
        if(pClassTab == NULL)
        {
            rc = CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL PTR PASSED"));
            CL_FUNC_EXIT();
            return rc;
        }
	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("pClassTab = %p, eClass = %d, maxInstances = %d, "
		"maxSlots = %d", (void *)pClassTab, (ClUint32T)classId, 
		(ClUint32T)maxInstances, maxSlots));

	/*  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("eClass = %d",(ClUint32T)classId)); */

	/*
	 * @todo: To validate the maxSlots field, it is not used 
     * in this release.
	 */

	/* Validate the input arguments */
	if (!maxInstances)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_MAX_INSTANCE);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid (zero) max instances argument "
			"(rc = 0x%x)!\r\n", rc)); 
		return (rc);
	}

	if (omClassTypeValidate(classId) != CL_OK)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid class type argument "
			"(rc = 0x%x). classId %d!\r\n", rc, classId)); 
		return (rc);
	}

	/*
	 * Check if the instance table for the specified class is already
	 * allocated. If allocated take either of two actions.
	 * 1. Return an error ot the user and indicate already allocated
	 * 2. Run thru' all the instances and free up the objects that are
 	 *    allocated in the OM context. and release the instance table
	 *    to make room for the new one.
	 */
	if (pClassTab->pInstTab)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_EXISTS);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
			("The Instance Table for this class already exists "
			 "(rc = 0x%x)!\r\n", rc));
		return (rc);
	}

	/* Allocate memory for the instance table for the specified class. */
	tmp_size = (maxInstances * sizeof(ClUint32T*));
	if ((tmp_ptr = (ClUint32T **)clHeapAllocate(tmp_size)) == NULL)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_NO_MEMORY);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Not enough memory available in the "
			"system (rc = 0x%x)\r\n", rc));
		return (rc);
	}

	/* Now setup the Peer class table */
	pClassTab->pInstTab = tmp_ptr;

	/* Add this class entry into the OM class lookup table */
	rc = clCntNodeAdd(ghOmClassHashTbl, 
			(ClCntKeyHandleT)(ClWordT)classId, (ClCntDataHandleT)pClassTab, NULL);
	if (rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to add OM class to hash table, "
			"w/ rc 0x%x!\r\n", __FUNCTION__, rc));
		return (rc);
		}
	gOmCfgData.numOfOmClasses++;
   	CL_FUNC_EXIT();
	return (rc);
}

static ClRcT 
clOmClassNameInitialize (ClOmClassControlBlockT *pClassTab, char *pClassName, ClUint32T maxInstances, 
                         ClUint32T maxSlots)
{
	ClUint32T **tmp_ptr;
	ClUint32T  tmp_size;
	ClRcT		rc = CL_OK;

	CL_ASSERT(pClassTab);
    CL_FUNC_ENTER();

    /* Sanity check for pClassTab */
    if(pClassTab == NULL || !pClassName)
    {
        rc = CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL PTR PASSED"));
        CL_FUNC_EXIT();
        return rc;
    }
	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("pClassTab = %p, Class = %s, maxInstances = %d, "
                                    "maxSlots = %d", (void *)pClassTab, pClassName, 
                                    (ClUint32T)maxInstances, maxSlots));

	/*  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("eClass = %d",(ClUint32T)classId)); */

	/*
	 * @todo: To validate the maxSlots field, it is not used 
     * in this release.
	 */

	/* Validate the input arguments */
	if (!maxInstances)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_INVALID_MAX_INSTANCE);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid (zero) max instances argument "
                                        "(rc = 0x%x)!\r\n", rc)); 
		return (rc);
	}

	/*
	 * Check if the instance table for the specified class is already
	 * allocated. If allocated take either of two actions.
	 * 1. Return an error ot the user and indicate already allocated
	 * 2. Run thru' all the instances and free up the objects that are
 	 *    allocated in the OM context. and release the instance table
	 *    to make room for the new one.
	 */
	if (pClassTab->pInstTab)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_EXISTS);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("The Instance Table for this class already exists "
                        "(rc = 0x%x)!\r\n", rc));
		return (rc);
	}

	/* Allocate memory for the instance table for the specified class. */
	tmp_size = (maxInstances * sizeof(ClUint32T*));
	if ((tmp_ptr = (ClUint32T **)clHeapAllocate(tmp_size)) == NULL)
	{
		rc = CL_OM_SET_RC(CL_OM_ERR_NO_MEMORY);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Not enough memory available in the "
                                        "system (rc = 0x%x)\r\n", rc));
		return (rc);
	}

	/* Now setup the Peer class table */
	pClassTab->pInstTab = tmp_ptr;

	/* Add this class entry into the OM class lookup table */
	rc = clCntNodeAdd(ghOmClassNameHashTbl, 
                      (ClCntKeyHandleT)pClassName, (ClCntDataHandleT)pClassTab, NULL);
	if (rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s: Failed to add OM class to hash table, "
                                        "w/ rc 0x%x!\r\n", __FUNCTION__, rc));
		return (rc);
    }
	gOmCfgData.numOfOmClasses++;
   	CL_FUNC_EXIT();
	return (rc);
}

/*
 * Insert the class tab entry into the class id table.
 */
static ClRcT 
omClassEntryReload (ClCharT *pClassName, ClOmClassTypeT classId)
{
	ClRcT		rc = CL_OK;
    ClOmClassControlBlockT *pClassTab = NULL;
    ClCntNodeHandleT nodeHandle = 0;

    CL_FUNC_ENTER();

    if(!pClassName)
    {
        rc = CL_OM_SET_RC(CL_OM_ERR_NULL_PTR);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL PTR PASSED"));
        CL_FUNC_EXIT();
        return rc;
    }

	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("OM entry reload for class = %s, class type %d", pClassName, classId));

    /*
     * First check if the class id entry is already present in the om class id table.
     */
    if(clCntNodeFind(ghOmClassHashTbl, (ClCntKeyHandleT)(ClWordT)classId, &nodeHandle) == CL_OK)
        return CL_OK;

	/* Add this class entry into the OM class lookup table */
	rc = clCntNodeFind(ghOmClassNameHashTbl, (ClCntKeyHandleT)pClassName, &nodeHandle);
	if (rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to find OM Class name [%s] entry in om class table",
                                        pClassName));
		return (rc);
    }
    
    rc = clCntNodeUserDataGet(ghOmClassNameHashTbl, nodeHandle, (ClCntDataHandleT*)&pClassTab);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to get data for OM Class [%s] from om class name table",
                                        pClassName));
        return rc;
    }

    /*
     * Insert this entry into the class id table.
     */
    pClassTab->eMyClassType = classId;

    rc = clCntNodeAdd(ghOmClassHashTbl, (ClCntKeyHandleT)(ClWordT)pClassTab->eMyClassType,
                      (ClCntDataHandleT)pClassTab, NULL);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to add entry [%#x] for class [%s] to OM class id table",
                                        classId, pClassName));
        return rc;
    }

   	CL_FUNC_EXIT();
	return (rc);
}

ClRcT clOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass)
{
    if(!pOmClass)
        return CL_OM_SET_RC(CL_ERR_INVALID_PARAMETER);

    if(gClOIConfig.oiDBReload)
    {
        ClCharT omClassName[CL_MAX_NAME_LENGTH] = {0};
        ClRcT rc = CL_OK;
        rc = clCorOmClassNameFromInfoModelGet(moClass, svcId, pOmClass, omClassName,
                                              (ClUint32T)sizeof(omClassName)-1);
        if(rc != CL_OK)
        {
            clLogError("OM", "INFO", "OM class name get for mo class [%d], svc [%d] returned with error [%#x]",
                       moClass, svcId, rc);
            return rc;
        }
        return omClassEntryReload(omClassName, *pOmClass);
    }
    return clCorOmClassFromInfoModelGet(moClass, svcId, pOmClass);
}

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
 * @returns CL_OK if all the clean up operations are successful
 */
ClRcT 
clOmClassFinalize(ClOmClassControlBlockT *pClassTab, ClOmClassTypeT classId)
{
	ClUint32T   maxEntries;
	ClUint32T   instCount;
	ClUint32T **tmpInstance;

	CL_ASSERT(pClassTab);
    CL_FUNC_ENTER();

	/* TODO: Delete this module from the debug agents module list */

	/* Validate the input arguments */
	if (omClassTypeValidate(classId) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid input arguments")); 
		return (CL_OM_SET_RC(CL_OM_ERR_INVALID_CLASS));
	}

	/*
	 * OPEN_ISSUE: Check if the instance table for the specified 
	 * class is already released. If released take either of two 
	 * actions.
	 * 1. Silently discard the request and return.
	 * 2. Return an ERROR indicating a duplicate free.
	 */
	if (!pClassTab->pInstTab)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                   ("Duplicate free of Instance Table for this class"));
		return (CL_OM_SET_RC(CL_OM_ERR_INSTANCE_TAB_DUPFREE));
	}

	/*
	 * Now, run through the instance table and free all the memory 
	 * allocated in OM context. Use the reserved bits encoded within
	 * the address to determine this.
	 */
        maxEntries = pClassTab->maxObjInst;	
	tmpInstance = pClassTab->pInstTab;	
	for (instCount = 0; instCount < maxEntries; instCount++)
	{
		if (tmpInstance[instCount] && mALLOC_BY_OM(tmpInstance[instCount]))
			clHeapFree((ClPtrT)mGET_REAL_ADDR(tmpInstance[instCount]));
	}

	/* Now, free the Instance table	for this class */
	clHeapFree((void *)tmpInstance);

	/* Now setup the Peer class table */
	pClassTab->pInstTab    = (ClUint32T **)NULL;
	pClassTab->curObjCount = 0;


   	CL_FUNC_EXIT();
	return (CL_OK);
}



/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This is hash table function to compare keys for the class lookup table.
 */
static ClInt32T omClassKeyCompare(ClCntKeyHandleT key1, 
                                  ClCntKeyHandleT key2)
{
	if (!key1 || !key2)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omClassKeyCompare(): bad key(s), key1 %p & "
                                       "key2 %p!!!\r\n", (void *)key1, (void *)key2));
		return CL_OM_SET_RC(CL_OM_ERR_INTERNAL);
    }
    return ((ClWordT)key1 - (ClWordT)key2);
}

static ClInt32T omClassNameKeyCompare(ClCntKeyHandleT key1, 
                                      ClCntKeyHandleT key2)
{
	if (!key1 || !key2)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omClassNameKeyCompare(): bad key(s), key1 %p & "
                                       "key2 %p!!!\r\n", (void *)key1, (void *)key2));
		return CL_OM_SET_RC(CL_OM_ERR_INTERNAL);
    }
    return strcmp((const char*)key1, (const char *)key2);
}


/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This is hash  function  for the class lookup table.
 */
static ClUint32T omClassHashFn(ClCntKeyHandleT key)
{
	if (key == 0)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omClassHashFn(): bad key!!!\r\n"));
		return CL_OM_SET_RC(CL_OM_ERR_INTERNAL);
    }
	return ((ClWordT)key % gOmCfgData.maxOmClassBuckets);
}

static ClUint32T omClassNameHashFn(ClCntKeyHandleT key)
{
    char *pClassName = (char*)key;
    ClUint32T cksum = 0;
    if(!pClassName)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omClassNameHashFn(): bad key!!!\r\n"));
		return CL_OM_SET_RC(CL_OM_ERR_INTERNAL);
    }
    clCksm32bitCompute((ClUint8T*)pClassName, (ClUint32T)strlen(pClassName), &cksum);
	return (cksum % gOmCfgData.maxOmClassBuckets);
}

/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This is delete callback  function  for the class lookup table.
 */
static void omClassEntryDelCallBack(ClCntKeyHandleT userKey, 
                                    ClCntDataHandleT userData)
{
    /* do nothing here */
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omClassEntryDelCallBack Function Do nothing!!\n\r"));
}

/**
 * NOT AN EXTERNAL API. PLEASE DO NOT DOCUMENT.
 *  
 * This is destroy callback  function  for the class lookup table.
 */
static void omClassEntryDestroyCallBack(ClCntKeyHandleT userKey, 
                                        ClCntDataHandleT userData)
{
    /* do nothing here */
	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("omClassEntryDestroyCallBack Function Do "
                                   "nothing!!\n\r"));
}



ClRcT clOmClassFiniWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                                ClCntArgHandleT userArg, ClUint32T dataLength)
{
   ClRcT rc = CL_OK;
   ClOmClassControlBlockT *pClassTab;

   if(!userData)
      return (CL_OM_SET_RC(CL_OM_ERR_NULL_PTR));
   pClassTab = (ClOmClassControlBlockT *)userData;
 
   rc = clOmClassFinalize(pClassTab, pClassTab->eMyClassType);
   if(rc != CL_OK)
   {
     CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
        ("\n clOmClassFinalize Failed for class %d.  rc [0x%x] \n", pClassTab->eMyClassType, rc));
   }
   return (CL_OK);
}


/* Finalize for Object Manger.  */
ClRcT  clOmLibFinalize()
{
   ClRcT rc = CL_OK;
   /* Finalize all the OM Classes.*/
   rc = clCntWalk(ghOmClassHashTbl, clOmClassFiniWalk, NULL, 0);
   return (CL_OK);
}


