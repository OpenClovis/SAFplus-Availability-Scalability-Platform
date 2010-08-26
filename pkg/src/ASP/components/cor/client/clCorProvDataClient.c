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
 * File        : clCorProvDataClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains APIs for storing and restoring the provisioned data.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>

/*Internal headers*/
#include <clCorClient.h>
#include <clCorRMDWrap.h>

#include <xdrCorPersisInfo_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif



/**
 *  Save the provisioned data
 *
 *  API to save the provisioned data to the specified file
 *
 *  @param pFileName  file to write onto
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorDataSave()
{
    ClRcT  rc = CL_OK;
    corPersisInfo_t corData;

	memset(&corData, 0, sizeof(corData));
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorDataSave \n"));

    /* corData.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(corData.version);
    corData.operation = COR_DATA_SAVE;
    
    COR_CALL_RMD(COR_EO_PERSISTENCY_OP,
                 VDECL_VER(clXdrMarshallcorPersisInfo_t, 4, 0, 0),
                 &corData,
                 sizeof(corData), 
                 VDECL_VER(clXdrUnmarshallcorPersisInfo_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataSave returns error,rc=%x",rc));
    }

    
    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Restore provisioned data
 *
 *  API to retrieve the provisioned data from the file
 *
 *  @param pFileName  file to retrieve from
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorDataRestore()
{
    ClRcT  rc = CL_OK;
    corPersisInfo_t corData;

	memset(&corData, 0, sizeof(corData));
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorDataRestore \n"));

    /* corData.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(corData.version);
    corData.operation = COR_DATA_RESTORE;
    
    COR_CALL_RMD(COR_EO_PERSISTENCY_OP,
                 VDECL_VER(clXdrMarshallcorPersisInfo_t, 4, 0, 0),
                 &corData,
                 sizeof(corData), 
                 VDECL_VER(clXdrUnmarshallcorPersisInfo_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    
    if((rc != CL_OK) && (rc != CL_ERR_NOT_EXIST))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataRestore returns error,rc=%x",rc));
    }

    
    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Save the provisioned data at regular interval
 *
 *  API to store the provisioned data at specified intervals
 *
 *  @param pFileName  file to write onto
 *  @param frequency  specified time interval
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorDataFrequentSave(char *pFileName, ClTimerTimeOutT  frequency)
{
    ClRcT  rc = CL_OK;
    corPersisInfo_t corData;

	memset(&corData, 0, sizeof(corData));
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorDataFrequentSave \n"));

    /* Validate input parameters */
    if (pFileName == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataFrequentSave: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    if (strlen(pFileName) >= COR_MAX_FILENAME_LEN)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataFrequentSave: File name is too large"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }


    /*memcpy(corData.fileName, (pFileName), sizeof(corData.fileName));*/
    memcpy(corData.fileName, (pFileName), strlen(pFileName)+1);
    /* corData.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(corData.version);
    corData.operation = COR_DATA_FREQ_SAVE;
    corData.frequency = frequency;
    
    COR_CALL_RMD(COR_EO_PERSISTENCY_OP,
                 VDECL_VER(clXdrMarshallcorPersisInfo_t, 4, 0, 0),
                 &corData,
                 sizeof(corData), 
                 VDECL_VER(clXdrUnmarshallcorPersisInfo_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataFrequentSave returns error,rc=%x",rc));
    }

    
    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Stop the regular saving of provisioned data  
 *
 *  API to stop the regular saving of provisioned data
 *
 *  @param NONE
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorDataFrequentSaveStop()
{
    ClRcT  rc = CL_OK;
    corPersisInfo_t corData;

	memset(&corData, 0, sizeof(corData));
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorDataFrequentSaveStop \n"));

    /* corData.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(corData.version);
    corData.operation = COR_DATA_FREQ_SAVE_STOP;
    
    COR_CALL_RMD(COR_EO_PERSISTENCY_OP,
                 VDECL_VER(clXdrMarshallcorPersisInfo_t, 4, 0, 0),
                 &corData,
                 sizeof(corData), 
                 VDECL_VER(clXdrUnmarshallcorPersisInfo_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorDataFrequentSaveStop returns error,rc=%x",rc));
    }

    
    CL_FUNC_EXIT();
    return rc;
}
