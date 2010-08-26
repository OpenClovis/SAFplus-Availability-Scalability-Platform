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
 * File        : clCorMOTreeApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains exported APIs for MOClass Tree manipulation.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorClient.h>
#include <clCorRMDWrap.h>

#include <xdrCorClassDetails_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif



/**
 *  Create MO class.
 *
 *  API to create a MO class.
 *
 *  @param corPath           MO Path
 *  @param maxInstances      max no. of instances
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *    CL_COR_ERR_INVALID_PARAM   Invalid parameter
 *
 */

ClRcT 
clCorMOClassCreate(ClCorMOClassPathPtrT corPath, 
                 ClInt32T maxInstances)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n COMP_MGR: Inside clCorMOClassCreate\n"));

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMOClassCreate: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    if ( ( 0 > maxInstances ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMOClassCreate: Negative Max Instance"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MO_CLASS;
    classDetails.maxInstances = maxInstances;
    classDetails.operation    = COR_CLASS_OP_CREATE;

    COR_CALL_RMD(COR_EO_MOTREE_OP,
                 VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                 &classDetails,
                 sizeof(corClassDetails_t), 
                 VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
   
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMOClassCreate returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Delete MO class.
 *
 *  API to delete a MO class.
 *
 *  @param corPath           MO Path
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorMOClassDelete(ClCorMOClassPathPtrT corPath)
{
    ClRcT   rc = CL_OK;
    corClassDetails_t classDetails;

    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMOClassDelete: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MO_CLASS;
    classDetails.operation    = COR_CLASS_OP_DELETE;

    COR_CALL_RMD(COR_EO_MOTREE_OP,
                 VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                 &classDetails,
                 sizeof(corClassDetails_t), 
                 VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
   
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMOClassDelete returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Create MSO class.
 *
 *  API to create a MO class.
 *
 *  @param corPath           MO Path
 *  @param svcId             Service ID 
 *  @param class             MO class type
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *    CL_COR_SVC_ERR_INVALID_ID  Invalid service ID
 *    CL_COR_ERR_INVALID_CLASS   Invalid class  
 *
 */

ClRcT 
clCorMSOClassCreate(ClCorMOClassPathPtrT     corPath,
                  ClCorServiceIdT    svcId,
                  ClCorClassTypeT class)
{
    ClRcT   rc = CL_OK;
    corClassDetails_t classDetails;
	
    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMSOClassCreate: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    if ( ( CL_COR_INVALID_SRVC_ID >= svcId ) || (svcId > CL_COR_SVC_ID_MAX) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid service ID specified"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
    }

    if ( 0 >= class )
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MSO_CLASS;
    classDetails.svcId        = svcId;
    classDetails.objClass      = class;
    classDetails.operation    = COR_CLASS_OP_CREATE;

    COR_CALL_RMD(COR_EO_MOTREE_OP,
                 VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                 &classDetails,
                 sizeof(corClassDetails_t), 
                 VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
   
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMSOClassCreate returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}
    


/**
 *  Delete MSO class.
 *
 *  API to delete a MSO class.
 *
 *  @param corPath           MO Path
 *  @param svcId             Service ID
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorMSOClassDelete(ClCorMOClassPathPtrT       corPath,
                  ClCorServiceIdT    svcId)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;
	
    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMSOClassDelete: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MSO_CLASS;
    classDetails.svcId        = svcId;
    classDetails.operation    = COR_CLASS_OP_DELETE;

    COR_CALL_RMD(COR_EO_MOTREE_OP,
                 VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                 &classDetails,
                 sizeof(corClassDetails_t), 
                 VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMSOClassDelete returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  MO class existance.
 *
 *  API to check whether MO class exists or not.
 *
 *  @param corPath           MO Path
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT
clCorMOClassExist(ClCorMOClassPathPtrT corPath)
{
    ClRcT    rc = CL_OK;
    corClassDetails_t classDetails;
	
    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOClassExist: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MO_CLASS;
    classDetails.operation    = COR_CLASS_OP_EXISTANCE;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_MOTREE_OP,
                                     VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails,
                                     sizeof(corClassDetails_t), 
                                     VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                                     NULL,
                                     NULL,
                                     rc);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMOClassExist returns error,rc=%x",rc));
    }

    CL_FUNC_EXIT();
    return rc;
}



/**
 *  MSO class existance.
 *
 *  API to check whether MSO class exists or not.
 *
 *  @param corPath           MO Path
 *  @param svcId             Service ID
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT
clCorMSOClassExist(ClCorMOClassPathPtrT    corPath,
                  ClCorServiceIdT  svcId)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;
	
    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMSOClassExist: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.classType    = MSO_CLASS;
    classDetails.svcId        = svcId;
    classDetails.operation    = COR_CLASS_OP_EXISTANCE;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_MOTREE_OP,
                                     VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails,
                                     sizeof(corClassDetails_t), 
                                     VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                                     NULL,
                                     NULL,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMSOClassExist returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Get class ID.
 *
 *  API to getting the class ID from ClCorMOClassPath
 *
 *  @param corPath           MO Path
 *  @param svcId             service Id
 *  @param pClassId          class ID
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT
clCorMOPathToClassIdGet(ClCorMOClassPathPtrT  corPath, ClCorServiceIdT svcId, ClCorClassTypeT* pClassId)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;
    ClUint32T size = sizeof(corClassDetails_t);

    CL_FUNC_ENTER();
 
    /* Validate input parameters */
    if ((corPath == NULL) || (pClassId == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCorMOPathToClassIdGet failed: NULL input parameter \n"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.operation    = COR_CLASS_OP_CLASSID_GET;
    classDetails.svcId        = svcId;
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_MOTREE_OP,
                                     VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails, 
                                     sizeof(corClassDetails_t),
                                     VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorMOPathToClassIdGet returns error,rc=%x",rc));
    }
    else
    {
        *pClassId = classDetails.objClass;
    }
    
    CL_FUNC_EXIT();
    return rc;
}
    


/**
 *  Get first child.
 *
 *  API to getting the first child in the moTree. updates corPath on success
 *
 *  @param corPath           MO Path
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT
clCorMoClassPathFirstChildGet(ClCorMOClassPathPtrT  corPath)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;
    ClUint32T size = sizeof(corClassDetails_t);

    CL_FUNC_ENTER();
 
    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCorMoClassPathFirstChildGet failed: NULL input parameter \n"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.operation    = COR_CLASS_OP_FIRSTCHILD_GET;
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_MOTREE_OP,
                                     VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails, 
                                     sizeof(corClassDetails_t),
                                     VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorMoClassPathFirstChildGet returns error,rc=%x",rc));
    }
    else
    {
        *corPath = classDetails.corPath; 
    }

    CL_FUNC_EXIT();
    return rc;
}
    

/**
 *  Get the next sibling.
 *
 *  API to getting the next sibling in the moTree. updates corPath on succes
 *
 *  @param corPath           MO Path
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT
clCorMoClassPathNextSiblingGet(ClCorMOClassPathPtrT  corPath)
{
    ClRcT  rc = CL_OK;
    corClassDetails_t classDetails;
    ClUint32T size = sizeof(corClassDetails_t);

    CL_FUNC_ENTER();
 
    /* Validate input parameters */
    if (corPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCorMoClassPathNextSiblingGet failed: NULL input parameter \n"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&classDetails, 0, sizeof(corClassDetails_t));
    /* classDetails.version      = CL_COR_VERSION_NO;*/
	CL_COR_VERSION_SET(classDetails.version);
    classDetails.corPath      = *(corPath);
    classDetails.operation    = COR_CLASS_OP_NEXTSIBLING_GET;
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_MOTREE_OP,
                                     VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails, 
                                     sizeof(corClassDetails_t),
                                     VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0),
                                     &classDetails,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorMoClassPathNextSiblingGet returns error,rc=%x",rc));
    }
    else
    {
        *corPath = classDetails.corPath; 
    }

    CL_FUNC_EXIT();
    return rc;
}
    
