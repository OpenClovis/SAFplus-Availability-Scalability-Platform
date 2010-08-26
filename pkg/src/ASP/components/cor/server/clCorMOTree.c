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
 * File        : clCorMOTree.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This modules implements MO Class tree.
 *****************************************************************************/
/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clBufferApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>

/* Internal Headers */
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorTreeDefs.h"
#include "clCorClient.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorRMDWrap.h"

#include <xdrCorClassDetails_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


extern ClRcT clCorMoClassPathValidate(ClCorMOClassPathPtrT this);
extern ClCorInitStageT  gCorInitStage;
extern ClUint32T gCorSlaveSyncUpDone;


/* GLOBALS */

#ifdef DEBUG
ClRcT corMoTreeDbgInit()
{
    ClRcT ret= CL_OK ;
    ret= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
    }
    return ret;
}
#endif

ClRcT  VDECL(_corMOTreeClassOpRmd) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corClassDetails_t *pClassInfo = NULL;
    CORMOClass_h moClassHandle;
    CORMSOClass_h msoClassHandle;

    CL_FUNC_ENTER();

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("MOT", "EOF", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }
    
    pClassInfo = clHeapAllocate(sizeof(corClassDetails_t));
    if(pClassInfo == NULL) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorClassDetails_t, 4, 0, 0)(inMsgHandle, (void *)pClassInfo)) != CL_OK)
	{
		clHeapFree(pClassInfo);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corClassDetails_t "));
		return rc;
	}
    /* if((rc = clBufferFlatten(inMsgHandle, (ClUint8T **)&pClassInfo))!= CL_OK)
    {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
                CL_FUNC_EXIT();
	        return rc;
    }*/	
    /* if(pClassInfo->version > CL_COR_VERSION_NO)
    {
		clHeapFree(pClassInfo);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Version mismatch"));
        CL_FUNC_EXIT();
        return CL_ERR_VERSION_MISMATCH;
    }*/

	clCorClientToServerVersionValidate(pClassInfo->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pClassInfo);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}
  
    switch(pClassInfo->operation)
    {
        case COR_CLASS_OP_CREATE:
            if(pClassInfo->classType == MO_CLASS)
            {
                rc = _corMOClassCreate(&(pClassInfo->corPath), pClassInfo->maxInstances, &moClassHandle);
                if(rc != CL_OK)
                {
                    clHeapFree(pClassInfo);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
						CL_LOG_MESSAGE_1_MOCLASS_CREATE, rc);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMOClassCreate failure"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            else
            {
                rc = corMOClassHandleGet(&(pClassInfo->corPath), &moClassHandle);
                if(rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOClassHandleGet failure"));
                }
                else
                {
                	rc = _corMSOClassCreate(moClassHandle, pClassInfo->svcId,
                       	                pClassInfo->objClass, &msoClassHandle);
                    if(rc != CL_OK)
               			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMSOClassCreate failure"));
                }

               	if(rc != CL_OK)
               	{
                    clHeapFree(pClassInfo);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_MSOCLASS_CREATE, rc);
               		CL_FUNC_EXIT();
               		return rc;
                }
            }
            break;
        case COR_CLASS_OP_DELETE:
            if(pClassInfo->classType == MO_CLASS)
            {
                rc = _corMOClassDelete(&(pClassInfo->corPath));
                if(rc != CL_OK)
                {
                    clHeapFree(pClassInfo);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_MOCLASS_DELETE, rc);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMOClassDelete failure"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            else
            {
                rc = corMOClassHandleGet(&(pClassInfo->corPath), &moClassHandle);
                if(rc != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOClassHandleGet failure"));
                else
                {
                    rc = _corMSOClassDelete(moClassHandle, pClassInfo->svcId);
                    if(rc != CL_OK)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMSOClassDelete failure"));
                }

                if(rc != CL_OK)
               	{
                    clHeapFree(pClassInfo);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
						CL_LOG_MESSAGE_1_MSOCLASS_DELETE, rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            break;
        case COR_CLASS_OP_EXISTANCE:
            if(pClassInfo->classType == MO_CLASS)
            {
                rc = corMOClassHandleGet(&(pClassInfo->corPath), &moClassHandle);
                if(rc != CL_OK)
                {
					clHeapFree(pClassInfo);
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMOClassHandleGet failure"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            else
            {
                rc = corMOClassHandleGet(&(pClassInfo->corPath), &moClassHandle);
                if(rc != CL_OK)
                {
					clHeapFree(pClassInfo);
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMOClassHandleGet failure"));
                    CL_FUNC_EXIT();
                    return rc;
                }
                rc = corMSOClassHandleGet(moClassHandle, pClassInfo->svcId, &msoClassHandle);
                if(rc != CL_OK)
                {
					clHeapFree(pClassInfo);
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMSOClassHandleGet failure"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            break;
        case COR_CLASS_OP_CLASSID_GET:
            rc = _corMOPathToClassIdGet(&(pClassInfo->corPath), pClassInfo->svcId, &(pClassInfo->objClass));
            if(rc != CL_OK)
            {
				clHeapFree(pClassInfo);
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "corMSOClassHandleGet failure"));
                CL_FUNC_EXIT();
                return rc;
            }
            /*Write to the message.*/
            /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pClassInfo, sizeof(corClassDetails_t)); */
			VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0)(pClassInfo, outMsgHandle, 0);
            break;
        case COR_CLASS_OP_FIRSTCHILD_GET:
            rc = _clCorMoClassPathFirstChildGet(&(pClassInfo->corPath));
            if(rc != CL_OK)
            {
				clHeapFree(pClassInfo);
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "_clCorMoPathFirstChildGet failure"));
                CL_FUNC_EXIT();
                return rc;
            }
            /*Write to the message.*/
            /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pClassInfo, sizeof(corClassDetails_t)); */
			VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0)(pClassInfo, outMsgHandle, 0);
            break;
        case COR_CLASS_OP_NEXTSIBLING_GET:
            rc = _clCorMoClassPathNextSiblingGet(&(pClassInfo->corPath));
            if(rc != CL_OK)
            {
				clHeapFree(pClassInfo);
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "_clCorMoPathNextSiblingGet failure"));
                CL_FUNC_EXIT();
                return rc;
            }
            /*Write to the message.*/
            /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pClassInfo, sizeof(corClassDetails_t)); */
			VDECL_VER(clXdrMarshallcorClassDetails_t, 4, 0, 0)(pClassInfo, outMsgHandle, 0);
            break;
        default:
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("MOClass Request Type %d", pClassInfo->operation));
            break;
    }

    if (pClassInfo->operation == COR_CLASS_OP_CREATE ||
            pClassInfo->operation == COR_CLASS_OP_DELETE)
    {
        if (gCorSlaveSyncUpDone == CL_TRUE)
        {
            ClRcT retCode = CL_OK;

            retCode = clCorSyncDataWithSlave(COR_EO_MOTREE_OP, inMsgHandle);
            if (retCode != CL_OK)
            {
                clLogError("SYNC", "", "Failed to sync data with slave COR. rc [0x%x]", rc);
                /* Ignore the error code. */
            }
        }
    }

    CL_FUNC_EXIT();
	clHeapFree(pClassInfo);
    return rc;
}
    
/**
 *  Create the specified Managed Object type.
 *
 *  API to create a new MO type and add it to the meta structure in
 *  COR. 'class' specified should be already defined.
 * 
 *  @param moPath        		Path of the class in MO Tree. e.g. /chassis/blade
 *  @param maxInstances       	Max imum instances of this MO class type that can be created
 *  @param moClassHandle 	[out] Handle to the class type created
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH) when path specified is invalid
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT) when class definition for the class is not present
 *    CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_TO_ADD_NODE) when addition to MO Tree fails
 *
 */
ClRcT 
_corMOClassCreate(ClCorMOClassPathPtrT moClassPath, 
                 ClInt32T maxInstances, 
                 CORMOClass_h* moClassHandle
                 )
{
    ClCorClassTypeT moClassType;
     CORClass_h corClassHandle;
    ClRcT  rc = CL_OK;

    CL_FUNC_ENTER();
    
    if(!moTree)
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Max instances %d", (ClUint32T)maxInstances));

    if ((moClassPath == NULL) || (moClassHandle == NULL))
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMOClassCreate: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    if ( ( 0 > maxInstances ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMOClassCreate: Negative Max Instance"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }
    
    
    if (clCorMoClassPathValidate(moClassPath) != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to validate the moClassPath argument"));
        return(CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH));
      }
    
    moClassType = clCorMoClassPathToMoClassGet(moClassPath);
    
    /* Validate with data manager, the final class ID */
    if (dmClassIsPresent(moClassType) != CL_TRUE)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid class type specified"));
        return(CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
      }

	/*Get the DM class first*/
	corClassHandle = dmClassGet(moClassType);
	if(NULL != corClassHandle)
		{
		corClassHandle->moClassInstances++;
		}
    
    rc = corMOTreeAdd(moTree, moClassPath, maxInstances);
    if (rc != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Add node by path"));
        return(CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_TO_ADD_NODE));
      }
    
    if (moClassHandle)
      *moClassHandle = corMOTreeFind(moClassPath);
    
    CL_FUNC_ENTER();
    return(CL_OK);
}

/**
 *  Return the MO Class handle associated with the COR Path.
 *
 *  This API returns the MO Class handle associated with the 
 *  COR Path.
 * 
 *  @param moPath         MO path to the MO class
 *  @param moClassHandle   Reference to memory, where moClassHandle is returned
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH)	when the path specified is invalid
 *    CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_USR_BUF)	when got error while obtaining handle of the class in MO tree
 *
 */
ClRcT 
corMOClassHandleGet(ClCorMOClassPathPtrT moClassPath, 
                    CORMOClass_h* moClassHandle)
{
    
    if ((moClassPath == NULL) || (moClassHandle == NULL))
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOClassHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    
    if (clCorMoClassPathValidate(moClassPath) != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to validate the moClassPath argument"));
        return(CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH));
      }
    
    if (moClassHandle)
      {
        *moClassHandle = corMOTreeFind(moClassPath);

        if (!*moClassHandle)
          {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get MO Class handle from tree node"));
            return(CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_USR_BUF));
          } else
            {
              CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nGot MO Class Handle, 0x%08x", 
                                    *((unsigned int *)moClassHandle)));
            }
      }
    
    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Delete the last node in the MO hierarchy.
 *
 *  API to delete the class type specified if there are no instances
 *  in the COR.  NOTE: This API shall delete the class specified in
 *  the ClCorMOClassPath and the subtree of classes, including the service
 *  class ids.
 *                                                                        
 *  @param 	moPath  path to the class to be deleted from MO Tree
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH)	when path specified is not valid
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT)	When class instance is present you can not delete class.
 *    CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_TO_DEL_NODE)	When failed to delete node from MO Tree
 *
 */
ClRcT 
_corMOClassDelete(ClCorMOClassPathPtrT moClassPath)
{
    ClRcT rc;
    ClCorClassTypeT moClassType;
	MOTreeNode_h node;
	ClUint32T numChildNode; 
	CORClass_h corClassHandle;
	

    CL_FUNC_ENTER();
    
    if (moClassPath == NULL)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corMOClassDelete: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    if (clCorMoClassPathValidate(moClassPath) != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to validate the moClassPath argument"));
        return(CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH));
      }

#ifdef DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Delete MOTree object: "));
    clCorMoClassPathShow(moClassPath);
#endif

    moClassType = clCorMoClassPathToMoClassGet(moClassPath);
#if 0
    /* This check is not required. In case if a COR class is used by TWO
  	MOs, We won't be able to delete one MO even if instance of other
  	is present. 

  	Instead we need to look into MO Class Handle to determine how many
  	objects of MO are present*/
  	
    moClassType = clCorMoClassPathToMoClassGet(&nCorPath);
    if ( dmClassInstanceCountGet(moClassType) > 1 )
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Class instance already present or is a base-class"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT));
    }
#endif

   	node = corMOTreeFind(moClassPath);
	if(NULL == node)
	{
       	CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not find node information in the tree."));
       	CL_FUNC_EXIT();
       	return (CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND));
	}
	
	if( ((_CORMOClass_h)node->data)->instances > 0)
		{
		/* Object Instances present */
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Class instance already present"));
		CL_FUNC_EXIT();
		return (CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT));
		}
	
	numChildNode = ( (_CORMOClass_h)(node->data) )->numChildren;
	if(numChildNode > 1)
	{
       	CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Class has children classes. Deletion is not possible."));
       	CL_FUNC_EXIT();
       	return (CL_COR_SET_RC(CL_COR_MO_TREE_ERR_CHILD_CLASS_EXIST));
	}
    rc = corMOTreeDel(moClassPath);
    if ( rc != CL_OK )
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to delete node by path"));
        return rc;
	}
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nDeleted MO Class node from hierarchy"));

		/*Get the DM class first*/
	corClassHandle = dmClassGet(moClassType);
	if(NULL != corClassHandle)
		{
		corClassHandle->moClassInstances--;
                                
		}
    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Create the meta-data for the specified Managed Service Object
 *  class.
 *
 *  API to create a new class type (meta-data) and add it to the meta
 *  structure in COR.  
 * 
 *  @param moClassHandle        Handle of MO class to which this class is attached.
 *  @param svcId        Managed Sevice identifier. Identifies which service does this class cater to. 
 *  @param class         Class Id of the MSO class.
 *  @param msoClassHandle	[out] Class handle of the class created.
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID)	The service Id provided is invalid
 *    CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS)	The unique class Id provided is invalid
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT)	The class is not present
 *
 */
ClRcT 
_corMSOClassCreate(CORMOClass_h   moClassHandle,
                  ClCorServiceIdT    svcId,
                  ClCorClassTypeT class ,
                  CORMSOClass_h* msoClassHandle)
{
    CORClass_h corClassHandle;
	
    CL_FUNC_ENTER();
    
    if( ( NULL == moClassHandle ) || ( NULL == msoClassHandle ) )
      {
          CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "MO Class handle %p, Service Id %d, Class 0x%08x",
                          (void *)moClassHandle, 
                          svcId, 
                          class));
    
    if ( ( CL_COR_INVALID_SRVC_ID >= svcId ) || (svcId > MAX_COR_SERVICES) )
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

    /* First validate the class type */
    if (dmClassIsPresent(class) != CL_TRUE)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid class type specified"));
        return(CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
      }
    corMSOSet(moClassHandle,svcId,class);
	
        *msoClassHandle = corMSOGet(moClassHandle,svcId);
        /* The class is just created. Set the MSO object instances to 0 */
        (*msoClassHandle)->msoInstances = 0;
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Got MSO Class Handle %p",
                              (void *)*msoClassHandle));

		/*Get the DM class first*/
	corClassHandle = dmClassGet(class);
	if(NULL != corClassHandle)
		{
		corClassHandle->moClassInstances++;
		}


    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Return the MSO Class handle that corresponds to the MO COR path
 *  and service ID.
 *
 *  API to return the MSO Class handle that corresponds to the MO COR
 *  path and service ID.
 * 
 *  @param moClassHandle  Class Handle to the MO Class.
 *  @param svcId          Managed Sevice identifier
 *  @param msoClassHandle MSO Class handle that is returned
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID)	the service Id provided is invalid
 *    CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE)	if could not obtain the correct handle
 *
 */
ClRcT 
corMSOClassHandleGet(CORMOClass_h   moClassHandle,
                     ClCorServiceIdT    svcId,
                     CORMSOClass_h* msoClassHandle)
{
    CL_FUNC_ENTER();

    
        if( NULL == moClassHandle )
      {
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "MO Class Handle %p, Service Id %d",
                          (void *)moClassHandle, (ClUint32T)svcId));
    
    if (svcId > MAX_COR_SERVICES)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid service ID specified"));
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
      }
    
    if (msoClassHandle)
      {
        *msoClassHandle = corMSOGet(moClassHandle,svcId);
        if(!*msoClassHandle || !(*msoClassHandle)->classId)
          {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Service Uninitialized"));
            return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
          }
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Got MSO Class Handle %p",
                              (void *)*msoClassHandle));
      }
    else
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL pointer, msoClassHandle specified"));
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE));
      }

    CL_FUNC_EXIT();
    return(CL_OK);
}


/**
 *  Delete the meta-data for the specified Managed Service Object
 *  class from COR.
 *
 *  API to delete the service (and its attributes) from the class type
 *  specified and instances from the COR.
 *                                                                        
 *  @param class      Class Type to delete the meta-data from the COR
 *  @param serviceId  Managed Sevice identifier
 *
 *  @todo   on deletion, all the MSO's for this object type also to be
 *  deleted.
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
_corMSOClassDelete(CORMOClass_h   moClassHandle,
                  ClCorServiceIdT    svcId)
{
    CORMSOClass_h msoClassHandle;
    CORClass_h corClassHandle;
	
    CL_FUNC_ENTER();

    
    if( NULL == moClassHandle )
      {
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "MO Class Handle %p, Service Id %d",
                          (void *)moClassHandle, svcId));
    
    if (svcId > MAX_COR_SERVICES)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid service ID specified"));
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
      }
    
    msoClassHandle = corMSOGet(moClassHandle, svcId);
    if( (NULL == msoClassHandle) || 
        (msoClassHandle->classId == 0) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Service Uninitialized"));
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
    }

    /* Check if object instances are present for this MSO class. If present do not
    allow class deletion */
    if(msoClassHandle->msoInstances > 0)
    	{
    	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "MSO Object present. Can not delete MSO class"));
	return CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT);
    	}

    
    /* Do not delete any class items / structures from the DM.*/
	
	corClassHandle = dmClassGet(msoClassHandle->classId);
	if(NULL != corClassHandle)
		{
		corClassHandle->moClassInstances--;
		}

    corMSOSet(moClassHandle, svcId, 0);
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete MSO Class Handle %p, Service Id %d",
                          (void *)msoClassHandle, (ClUint32T)svcId));

    
    CL_FUNC_EXIT();
    return(CL_OK);
}

#if 0
/**
 *  Show the MSO meta-data contents from the input class handle.
 *
 *  This API displays the meta-data contents from the input class
 *  handle.
 *                                                                        
 *  @param class Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMSOClassShow(CORMSOClass_h msoClassHandle)
{
    CL_FUNC_ENTER();

    
    if( NULL == msoClassHandle )
      {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = %p", msoClassHandle));
    socPrintf("\nNot implemented!!!");

    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Show the MO meta-data contents from the input class handle.
 *
 *  This API displays the meta-data contents from the input class
 *  handle.
 *                                                                        
 *  @param class Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMOClassShow(CORMOClass_h moClassHandle)
{
    CL_FUNC_ENTER();
    

    if( NULL == moClassHandle )
      {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = %p", moClassHandle));
    socPrintf("\nNot implemented!!!");
    
    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Show the meta-data contents from the input class handle.
 *
 *  This API displays the meta-data contents from the input class handle.
 *                                                                        
 *  @param class Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corClassShow(CORClass_h classHandle)
{
    CL_FUNC_ENTER();

    
    if( NULL == classHandle )
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = %p", classHandle));
  
    socPrintf("\nNot implemented!!!");

    CL_FUNC_EXIT();
    return(CL_OK);
}
#endif


/* corMOTreeClassGet can be used intstead. */

#if 0
/**
 *  Get ClCorClassTypeT   from ClCorMOClassPath.
 *
 *  API to get the MO/MSO class type for a given ClCorMOClassPath
 *  @param class     Class Type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT  clCorMoPathToMSOClassTypeGet( ClCorMOClassPathPtrT        pPath, 
                                 ClCorServiceIdT     svc,
                                 ClCorClassTypeT  *pClassType)
{
    ClRcT    rc = CL_OK;

    CL_FUNC_ENTER();
    if ((pPath == NULL) || (pClassType == NULL))
    {
        /* Invalid arguments. */
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* service ID is CL_COR_INVALID_SVC_ID then MO  */
    if (svc == CL_COR_INVALID_SVC_ID)
    {
        *pClassType = pPath->node[pPath->depth-1];
    }
    else
    {
        CORMOClass_h      moClsHdl = NULL;
        CORMSOClass_h     msoClsHdl = NULL;

        /*  Get the MO handle first */
        if (CL_OK != (rc = corMOClassHandleGet(pPath, &moClsHdl)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "MO Class not present"));
            clCorMoClassPathShow(pPath);
            return CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH);
        }

        if ((CL_OK != (rc = corMSOClassHandleGet(moClsHdl, svc,  &msoClsHdl))) ||
            (msoClsHdl == NULL))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "MSO Class not present: Svc [0x %x]", svc));
            clCorMoClassPathShow(pPath);
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT);
        }
        *pClassType = msoClsHdl->classId;
    }
    return rc;
}
#endif

/* ------------------------------ [TB done] ---------------------------- */


#if COR_PROJECT_PHASE2_WORK_ITEMS

/**
 *  Enable persistance on the class added into COR.
 *
 *  API to enables persistance (all class instances will be made
 *  persistent).
 *                                                                        
 *  @param class     Class Type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMOClassPersistEnable(CORMOClass_h moClassHandle)
{
	CL_FUNC_ENTER();
	
	if( NULL == moClassHandle )
	{
		return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
	}
  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", moClassHandle));

	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Disable persistance on the specified class within COR.
 *
 *  API to disables persistance (instance are not made persistent).
 *                                                                        
 *  @param class Class Identifer
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMOClassPersistDisable(CORMOClass_h moClassHandle)
{
	CL_FUNC_ENTER();
	
  
        if( NULL == moClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", moClassHandle));
  
	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Enable forwarding Managed Service requests.
 *
 *  API to enable forwarding Managed Service requests to the
 *  configured remote Managed Object.
 *                                                                        
 *  @param class       Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMOClassForwardingEnable(CORMOClass_h moClassHandle)
{
	CL_FUNC_ENTER();
	
        if( NULL == moClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", moClassHandle));

	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Disable forwarding Managed Service requests.
 *
 *  API to disable forwarding Managed Service requests to the
 *  configured remote Managed Object.
 *                                                                        
 *  @param class Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMOClassForwardingDisable(CORMOClass_h moClassHandle)
{
	CL_FUNC_ENTER();
	

        if( NULL == moClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", moClassHandle));
  
	CL_FUNC_EXIT();
  	return(CL_OK);
}


/**
 *  Enable persistance on the class added into COR.
 *
 *  API to enables persistance (all class instances will be made
 *  persistent).
 *                                                                        
 *  @param class     Class Type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMSOClassPersistEnable(CORMSOClass_h msoClassHandle)
{
	CL_FUNC_ENTER();
	

        if( NULL == msoClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", msoClassHandle));

	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Disable persistance on the specified class within COR.
 *
 *  API to disables persistance (instance are not made persistent).
 *                                                                        
 *  @param class Class Identifer
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMSOClassPersistDisable(CORMSOClass_h msoClassHandle)
{
	CL_FUNC_ENTER();
	

        if( NULL == msoClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", msoClassHandle));
  
	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Enable forwarding Managed Service requests.
 *
 *  API to enable forwarding Managed Service requests to the
 *  configured remote Managed Object.
 *                                                                        
 *  @param class       Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMSOClassForwardingEnable(CORMSOClass_h msoClassHandle)
{
	CL_FUNC_ENTER();
	

        if( NULL == msoClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", msoClassHandle));

	CL_FUNC_EXIT();
  	return(CL_OK);
}

/**
 *  Disable forwarding Managed Service requests.
 *
 *  API to disable forwarding Managed Service requests to the
 *  configured remote Managed Object.
 *                                                                        
 *  @param class Class Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
corMSOClassForwardingDisable(CORMSOClass_h msoClassHandle)
{
	CL_FUNC_ENTER();
	

        if( NULL == msoClassHandle )
        {
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "class = 0x%08x", msoClassHandle));
  
	CL_FUNC_EXIT();
  	return(CL_OK);
}

#endif
