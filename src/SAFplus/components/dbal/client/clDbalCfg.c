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
 * ModuleName  : dbal
 * File        : clDbalCfg.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * DBAL Component configuration module                      
 *************************************************************************/
/** @pkg cl.dbal */

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

#include <string.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clDbalCfg.h>
#include <clOsalApi.h>
#include <clDbalApi.h>
#include <clDbalCfg.h>
#include <clovisDbalInternal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GLOBALs */
static ClDbalFunctionPtrsT dbalFunctionPtrs;
static ClBoolT gClDbalInitialized = CL_FALSE;
ClDbalFunctionPtrsT   *gDbalFunctionPtrs = &dbalFunctionPtrs;
typedef void (*dbalGenericInterfaceT)(ClDbalFunctionPtrsT *ptrs); 
ClRcT (*dbalGenericInterface)(ClDbalFunctionPtrsT *ptrs); 
extern ClRcT dbalGetLibName(ClCharT  *pLibName);
extern ClRcT clDbalInterface(ClDbalFunctionPtrsT *ptrs);
static ClPtrT gDlHandle;
static ClBoolT gDlOpen = CL_FALSE;
/* GLOBALs */

/***************************************************************************************/
/**
* Database Abstraction Layer configuration module entry-point.
*
* This API contains the Database Abstraction Layer configuration module entry-point.
*
* @param none
*
* @returns CL_OK - Success<br>
*/
ClRcT clDbalLibInitialize(void)
{
    ClRcT          rc = CL_OK;

    if(gClDbalInitialized == CL_TRUE)
    {
        return CL_OK;
    }

    memset(gDbalFunctionPtrs, '\0', sizeof(ClDbalFunctionPtrsT));  

#if defined(WITH_PURIFY) || defined(WITH_STATIC)
    rc = clDbalInterface(gDbalFunctionPtrs);  
    if (rc != CL_OK)
    {
        clLogError("DBA", "INI", "Failed to initialize the Dbal library. rc [0x%x]", rc);
        return rc;    
    }
    gDlHandle = NULL;
    gDlOpen = CL_FALSE;
#else
    {
        ClCharT        libName[CL_MAX_NAME_LENGTH];

        /*Read the config XML file for DBAL*/
        memset(libName,'\0',sizeof(libName));
        rc  = dbalGetLibName(libName);   
        if( rc != CL_OK)
        {
            clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Error getting the DBAL plugin filename. rc [0x %x]\n",rc);
        } 
        /*open the dynamic loaded library*/
        rc = CL_DBAL_RC(CL_ERR_UNSPECIFIED);
        gDlHandle = dlopen(libName,RTLD_GLOBAL|RTLD_NOW);
        if(NULL != gDlHandle )
        {
            *(void**)&dbalGenericInterface = dlsym(gDlHandle,"clDbalInterface");
            if (NULL == dbalGenericInterface)
            {
                char* err = dlerror();
                if (!err) err = "unknown";

                clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                               "Error in finding the symbol 'clDbalInterface' in shared library '%s': [%s]", libName, err);

                if(0 != dlclose(gDlHandle))
                {
                    clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, "Error while unloading DBAL shared library!");
                }
                goto out;
            }

            rc = dbalGenericInterface(gDbalFunctionPtrs);
            if (rc != CL_OK)
            {
                clLogError("DBA", "INI", "Failed to initialize the Dbal library. rc [0x%x]", rc);
                dlclose(gDlHandle);
                goto out;
            }
        }
        else
        {
            char* err = dlerror();
            if (!err) err = "unknown";          
            clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, "Error finding opening shared library '%s': [%s]", libName,err);
            goto out;
        }
        rc = CL_OK;
        gDlOpen = CL_TRUE;
    }
    out:
#endif
    if(rc == CL_OK)
    {
        gClDbalInitialized = CL_TRUE;
    }
    return (rc);
}

ClRcT clDbalLibFinalize(void)
{
    if(gClDbalInitialized == CL_FALSE)
    {
        return CL_OK;
    }
    gClDbalInitialized = CL_FALSE;
    gDbalFunctionPtrs->fpCdbFinalize();

#if !defined(WITH_PURIFY) && !defined(WITH_STATIC)
    if(CL_TRUE == gDlOpen)
    {
        if(0 != dlclose(gDlHandle))
        {
            clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                    "Error while unloading DBAL shared library!");
        }
    }
#endif
    return CL_OK;
}

#ifdef __cplusplus
 }
#endif
