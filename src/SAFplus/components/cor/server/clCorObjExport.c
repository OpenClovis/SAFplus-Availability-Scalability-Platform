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
 * File        : clCorObjExport.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains object create/delete functions.
 *****************************************************************************/

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clCorMetaData.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorApi.h>

/* Internal Headers*/
#include <clCorTxnApi.h>
#include "clCorTxnInterface.h"
//#include "clCorOHLib.h"
#include "clCorPvt.h"
#include "clCorRt.h"
#include "clCorObj.h"
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorTreeDefs.h"
#include "clCorLog.h"
#include "clCorDeltaSave.h"

#include <xdrCorObjHdlConvert_t.h>
//#include <xdrClCorObjectHandleT.h>
#include <xdrClCorAttrPathT.h>
#include <xdrClCorAttrCmpFlagT.h>
#include <xdrClCorAttrWalkOpT.h>
#include <xdrCorObjectInfo_t.h>
#include <xdrClCorObjectHandleIDLT.h>
#include <clHeapApi.h>


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALS */
extern int gObjectCount;
extern _ClCorServerMutexT	gCorMutexes;
extern ClCorInitStageT    gCorInitStage;


/**
 *  Create an instance of the specified Managed Object within the COR.
 *
 *  This API creates an instance of the specified Managed Object
 *  instance wintin the COR.  If the cAddr has instance id as -1,
 *  then the next available slot is allocated and then returned
 *  back in the cAddr (COR Handle).
 *                                                                        
 *  @param moId   - MOID to the object.
 *  @param handle - Object handle returned
 *
 *  @returns  CL_OK                    - Success<br>
 *            CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID)       - Invalid MOID specofied<br>
 *            CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT)   - Class type not present in COR<br>
 *            CL_COR_SET_RC(CL_COR_ERR_NO_MEM)              - Failed to allocate memory<br>
 *            CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND) - Specified class not present in COR Tree<br>
 */
ClRcT
_corMOObjCreate(ClCorMOIdPtrT moId)
{
    ClRcT rc;
    ClCorClassTypeT  classType;
    DMObjHandle_h   dmh = NULL;
    ClCorMOClassPathT  moClsPath = {{0}};
    ClCorInstanceIdT instId = 0;
    
    CL_FUNC_ENTER();
 

    /* check if objtree is present */
    if(!objTree)
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    
    if(moId)
    {
        /* Make sure all the classes in the MOID are created within COR */
        if (clCorMoIdValidate(moId) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid ClCorMOId specified"));
            return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }

        clCorMoIdToMoClassPathGet(moId, &moClsPath);
        if ((rc = corMOTreeClassGet (&moClsPath, moId->svcId, &classType)) != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not get the classId \
               for the supplied moId/svcId combination\n"));
            return (rc);
        }

        if (dmClassIsPresent(classType) != CL_TRUE)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid class type specified 0x%x", classType));
            return(CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
        }
    
        /* Make sure this Object creation does not exceed the maximum number
           of objects */

        if ((rc = dmObjectCreate(classType, &dmh)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to allocate object within data manager"));
            return (rc);
        }

        /*
         * By default, set INITIALIZED attributes values to instance id, 
         * which can be overriden later by doing attribute set.
         */
        instId = moId->node[moId->depth-1].instance;

        rc = dmObjectInitializedAttrSet(dmh, instId);
        if (rc != CL_OK)
        {
            clLogError("DM", "INIT", "Failed to set values for INITIALIZED attributes. rc [0x%x]", rc);
            return rc;
        }

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Class with type 0x%08x created within data manager",
                              (ClUint32T)classType));
    
        if ((rc=corObjTreeAdd(objTree, moId, dmh)) != CL_OK)
        {
            ClCharT moidName[CL_MAX_NAME_LENGTH];
            /* Freeing the memory allocated by the dmObjectCreate */
               clHeapFree(dmh);
            _clCorMoIdStrGet(moId, moidName);
            if(CL_GET_ERROR_CODE(rc) == CL_COR_INST_ERR_MO_ALREADY_PRESENT)
                CL_DEBUG_PRINT(CL_DEBUG_WARN, ("Object instance [%s] already exists in COR", moidName));
            else
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Object instance [%s] addition returned [%#x]", moidName, rc));
            return(rc);
        }
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Object Instance added to instance tree"));
    
        /* Freeing the memory allocated by the dmObjectCreate */
        clHeapFree(dmh);
    }


    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *  Create an instance of MSO (Managed Service Object).
 *
 *  API to create instance of MSO object at a given MO (Managed
 *  Object).  Service Id identifies the service object that needs to
 *  be created (if not already created) at the Managed object.
 *                                                                        
 *  @param this   	   MOID to the object
 *  @param srvcId      Identfies the Service
 *  @param objHandle   Reference to the Newly created MSO
 *
 *  @returns CL_OK  - Success<br>
 *            CL_COR_SET_RC(CL_COR_ERR_NO_MEM)              - Failed to allocate memory<br>
 *
 *		return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
		return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
		return(CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
		return(CL_COR_SET_RC(CL_COR_INST_ERR_ALREADY_PRESENT));
		return(CL_COR_SET_RC(CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT));
 */
ClRcT
_corMSOObjCreate(ClCorMOIdPtrT         this, 
                ClUint32T      srvcId)
{
    ClRcT rc;
    ObjTreeNode_h   moHandle = 0;
    DMObjHandle_h   dmh;
    ClCorClassTypeT  msoClassId;
    ClCorMOClassPathT moPath;
    CORMOClass_h moClassH;
    CORMSOClass_h msoH;
    ClCorInstanceIdT instId = 0;
    
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Service Id = 0x%08x", (ClUint32T)srvcId));
    
    if(this)
      {
        /* Get MO Object handle from MOID */

        if (!(moHandle = corMOObjGet(this)))
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get MO Object handle from MOID"));
		/*	clCorMoIdShow(this);*/
            return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
          }

        {
          ClCorMOClassPathT tmp;
          CORObject_h mso = corMSOObjGet(moHandle, srvcId);

          if(mso && mso->dmObjH.classId)
            {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "MSO Instance already present"));
              return(CL_COR_SET_RC(CL_COR_INST_ERR_MSO_ALREADY_PRESENT));
            }

          /* Get class ID for MSO from MO tree */
          rc = clCorMoIdToMoClassPathGet(this, &tmp);
          if(rc==CL_OK && 
             corMOTreeClassGet(&tmp, srvcId, &msoClassId)==CL_OK)
          {
              CL_DEBUG_PRINT(CL_DEBUG_TRACE,( "Class type 0x%08x retreived from MO Hierarchy",
                                   (ClUint32T)msoClassId));
              if ((rc = dmObjectCreate(msoClassId,&dmh)) != CL_OK)
              {
                  CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to create object instance in DM"));
                  return(rc); 
              }

              /*
               * By default, set INITIALIZED attributes values to instance id, 
               * which can be overriden later by doing attribute set.
               */
              instId = this->node[this->depth-1].instance;

              rc = dmObjectInitializedAttrSet(dmh, instId);
              if (rc != CL_OK)
              {
                  clLogError("DM", "INIT", "Failed to set values for INITIALIZED attributes. rc [0x%x]", rc);
                  return rc;
              }
          }
          else
          {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get MSO Class handle from MOID"));
              return(CL_COR_SET_RC(CL_COR_MO_TREE_ERR_FAILED_USR_BUF));
          }
          /* set the mso object in instance tree */
          corMSOObjSet(moHandle,srvcId,dmh);
	  /* Freeing the memory allocated by the dmObjcreate */
	  clHeapFree(dmh);
        }
    
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "Added MSO Object instance into COR with handle %p",
                   (void *) (corMSOObjGet(moHandle, srvcId))));
      }

     /* MSO object create successful. Increment the count in MSO class handle for objects
     */
     	if( (rc = clCorMoIdToMoClassPathGet(this, &moPath)) == CL_OK)
		{
		/* Get the mo class handle */
		if( (rc = corMOClassHandleGet(&moPath, &moClassH)) == CL_OK)
			{
			/* Get the MSO class Handle */
			if((rc = corMSOClassHandleGet(moClassH, srvcId, &msoH) ) == CL_OK)
				{
				msoH->msoInstances++;
				}
			else
				{
				CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get mso class handle"));
				return rc;
				}
			}
		else
			{
			CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get mo class handle"));
			return rc;
			}
     		}
	else
		{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorMoIdToMoClassPathGet failed"));
		return rc;
		}
	
    
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *  Delete the instance of the Managed Object within the COR.
 *
 *  This API deletes the instance of the Managed Object that was
 *  previously created with the COR.
 *                                                                        
 *  @param this - Handle to the MO Object instance
 *
 *  @returns CL_OK  - Success<br>
 *           COR_MO_BUSY - The Managed Object is in use or locked state<br>
 */
ClRcT 
_corMOObjDelete(ClCorMOIdPtrT moId)
{
    DMObjHandle_t dmo;
    ClRcT rc;
    CL_FUNC_ENTER();

    rc = moId2DMHandle(moId, &dmo);
    if(rc==CL_OK)
    {
       if((rc = corObjTreeNodeDelete(objTree, moId)) != CL_OK)
       {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Delete MO in instance tree, rc = 0x%x",rc));
	  CL_FUNC_EXIT();
	  return (rc);
       }

		clCorDeltaDbStore(*moId, NULL, CL_COR_DELTA_DELETE);
       /* now go ahead and delete the dm object */
       if((rc =  dmObjectDelete(&dmo) != CL_OK))
       {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Delete  dm Object, rc = 0x%x",rc));
       }
    }
    else
    {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Node not found in the object tree, rc = 0x%x",rc));
	return rc;
    }
    CL_FUNC_EXIT();
    return(rc);
}

/**
 *  Delete the instance of MSO.
 *
 *  API to delete the instance of the Managed Service Object that was
 *  previously created within the MO.
 *                                                                        
 *  @param this  -  Handle to the MSO Object instance.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT 
_corMSOObjDelete(ClCorMOIdPtrT moId)
{
    DMObjHandle_t dmo;
    ClRcT rc;
    ClCorMOClassPathT moPath;
    CORMOClass_h moClassH;
    CORMSOClass_h msoH;


    CL_FUNC_ENTER();

    /* Call into the data manager to free the instance */
    
    /* From the Object resource refered by objHandle */
    rc = moId2DMHandle(moId, &dmo);
    if(rc==CL_OK)
      {
        ObjTreeNode_h obj;
        
        /* delete the DM object */
        rc = dmObjectDelete(&dmo);
        /* now delete the MSO obj in tree */
        if(rc==CL_OK)
          {
            obj = corMOObjGet(moId);
            if(obj)
              {
                /* memset the area with zero, which is equi to delete */
                ClCorMOServiceIdT svcId=moId->svcId;
                CORObject_h dmObj;
                
                dmObj = obj->data;
                if(svcId!=CL_COR_INVALID_SRVC_ID)
                  {
                    dmObj=corMSOObjGet(obj, svcId);
                    if(dmObj)
                      {
                        memset(dmObj, 0, sizeof(CORObject_t));
                        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                                  ( "\n Deleted MSO Object instance \n"));
                      }
                  } 
              }
          }
	  else
	  {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unable to delete Object from DM. rc [0x%x]", rc));
		return rc;
	  }
      }
      else
      {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Node not Found in the Object tree. rc [0x%x]", rc));
	return rc;
      }


     /* MSO object delete successful. Decrement the count in MSO class handle for objects
     */
     	if( (rc = clCorMoIdToMoClassPathGet(moId, &moPath)) == CL_OK)
		{
		/* Get the mo class handle */
		if( (rc = corMOClassHandleGet(&moPath, &moClassH)) == CL_OK)
			{
			/* Get the MSO class Handle */
			if((rc = corMSOClassHandleGet(moClassH, moId->svcId, &msoH) ) == CL_OK)
				{
				msoH->msoInstances--;
				}
			else
				{
				CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get mso class handle"));
				return rc;
				}
			}
		else
			{
			CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get mo class handle"));
			return rc;
			}
     		}
	else
		{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorMoIdToMoClassPathGet failed"));
		return rc;
		}
		if(rc == CL_OK)
			clCorDeltaDbStore(*moId, NULL, CL_COR_DELTA_DELETE);
    
    CL_FUNC_EXIT();
    return(rc);
}


ClRcT 
 _corObjectCountGet(ClUint32T * pCount)
{
    *pCount = gObjectCount;
    return CL_OK;
}


ClRcT
VDECL(_corObjectOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corObjectInfo_t* pData = NULL;

    CL_FUNC_ENTER();
 
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("OBT", "EOF", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pData = clHeapAllocate(sizeof(corObjectInfo_t));
    if(!pData)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                      CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
         return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }

   if((rc = VDECL_VER(clXdrUnmarshallcorObjectInfo_t, 4, 0, 0)(inMsgHandle, pData))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Unmarshal corObjectInfo_t \n"));
         clHeapFree(pData);
         CL_FUNC_EXIT();
         return rc;
    }

	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
        clCorObjectHandleFree(&(pData->pObjHandle));
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

	clOsalMutexLock(gCorMutexes.gCorServerMutex);

    switch(pData->operation)
    {
        case COR_OBJ_OP_FIRSTINST_GET:
            rc = _clCorMoIdFirstInstanceGet(&pData->moId);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "_clCorMoIdFirstInstanceGet failed, rc = %x",rc) );
            }    
            /* Write to the message*/
            VDECL_VER(clXdrMarshallcorObjectInfo_t, 4, 0, 0)(pData, outMsgHandle, 0);
			
        break;

        case COR_OBJ_OP_NEXTSIBLING_GET:
            rc = _clCorMoIdNextSiblingGet(&pData->moId);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "_clCorMoIdNextSiblingGet failed, rc = %x",rc) );
            }    
            /* Write to the message*/
            VDECL_VER(clXdrMarshallcorObjectInfo_t, 4, 0, 0)(pData, outMsgHandle, 0);
        break;
        
        case COR_OBJ_OP_ATTR_TYPE_SIZE_GET:
        {
            CORAttr_h          attrH;
            DMContObjHandle_t  dmContObjHdl = {{0, 0}, NULL};
            ClInt32T           size = 0;

            rc = objHandle2DMHandle(pData->pObjHandle, &(dmContObjHdl.dmObjHandle));
            if (CL_OK != rc)
            {
               clCorObjectHandleFree(&(pData->pObjHandle));
               clHeapFree(pData);
               clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to get DM Object Handle . Return Code [0x%x]\n", rc));
               CL_FUNC_EXIT();
               return (rc);
            }

            /* Set contained AttrPath */
            if(pData->attrPath.depth != 0)
              dmContObjHdl.pAttrPath = &pData->attrPath;

            attrH = dmObjectAttrTypeGet(&dmContObjHdl, pData->attrId);
            if(!attrH)
            {
                clCorObjectHandleFree(&(pData->pObjHandle));
                clHeapFree(pData);
                clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
                rc = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_NOT_PRESENT);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Attribute not found. Return Code [0x%x] \n", rc));
                return (rc);
            }

            size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
            if(size < 0)
            {
                clCorObjectHandleFree(&(pData->pObjHandle));
                clHeapFree(pData);
                clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid size of the attribute."));
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
            }

            /* Marshalling the attribute type */
            rc = clXdrMarshallClInt32T(&attrH->attrType.type, outMsgHandle, 0);
            if(rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the attribute type. rc[0x%x]", rc));
            else
            {
                /* Marshalling the attribute array type */
                rc = clXdrMarshallClInt32T(&attrH->attrType.u.arrType, outMsgHandle, 0);
                if(rc != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the attribute arrayType. rc[0x%x]", rc));
                else
                {
                    /* Marshalling the attribute size */
                    rc = clXdrMarshallClInt32T(&size, outMsgHandle, 0);
                    if(rc != CL_OK)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the attribute size. rc[0x%x]", rc));
                    else
                    {
                        /* Marshalling the user flags */
                        rc = clXdrMarshallClInt32T(&attrH->userFlags, outMsgHandle, 0);
                        if(rc != CL_OK)
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the attribute size. rc[0x%x]", rc));
                    }
                }
            }
        }         
        break;
        default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "INVALID OPERATION, rc = %x", rc) );
                rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        
        break;
    }

    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
    clCorObjectHandleFree(&(pData->pObjHandle));
    clHeapFree(pData);
    return rc;
}

ClRcT
VDECL(_corObjectHandleConvertOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corObjHdlConvert_t* pData = NULL;

    CL_FUNC_ENTER();

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("OBH", "EOF", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pData = clHeapAllocate(sizeof(corObjHdlConvert_t));
    if(pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);   
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorObjHdlConvert_t, 4, 0, 0)(inMsgHandle, (void *)pData))!= CL_OK)
	{
		clHeapFree(pData);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to Unmarshall corObjHdlConvert_t "));
		return rc;
	}
  /* if((rc = clBufferFlatten(inMsgHandle, (ClUint8T **)&pData))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
         CL_FUNC_EXIT();
         return rc;
    }*/
    

    /* if(pData->version > CL_COR_VERSION_NO)
    {
		clHeapFree(pData);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Version mismatch"));
        CL_FUNC_EXIT();
        return CL_ERR_VERSION_MISMATCH;
    } */

	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pData->operation)
    {
        case COR_OBJ_OP_OBJHDL_TO_TYPE_GET:
            rc = _clCorObjectHandleToTypeGet(pData->pObjHandle, &pData->type);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to convt objHdl to type, rc = %x", rc) );
            } 
        /* Write to the message*/
        /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pData, sizeof(corObjHdlConvert_t)); */
		VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0)(pData, outMsgHandle, 0);
        break;

        case COR_OBJ_OP_OBJHDL_TO_MOID_GET:
            rc = clCorObjectHandleToMoIdGet(pData->pObjHandle, &pData->moId, &pData->svcId);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to convt objHdl to moid, rc = %x", rc) );
            }    
        /* Write to the message*/
        /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pData, sizeof(corObjHdlConvert_t)); */
		VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0)(pData, outMsgHandle, 0);
        break;
        case COR_OBJ_OP_OBJHDL_GET:
            rc = _clCorObjectHandleGet(&pData->moId, &pData->pObjHandle);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get objHdl from moid, rc = %x", rc) );
            }    
            else
            {
                /* Update the object handle size. */
                clCorObjectHandleSizeGet(pData->pObjHandle, &pData->ohSize);
            }

        /* Write to the message*/
        /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pData, sizeof(corObjHdlConvert_t)); */
            VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0)(pData, outMsgHandle, 0);
        break;

        default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "INVALID OPERATION, rc = %x", rc) );
                rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        break;
    }
    
	clHeapFree(pData);
    return rc;
}

ClRcT
VDECL(_corObjExtObjectPack) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCorAttrPathT attrPath;
    VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIdl = {0};
    DMObjHandle_t      dmObjHdl;
    ClCorObjAttrWalkFilterT usrFilter;
    ClCorObjAttrWalkFilterT *pUsrFilter = &usrFilter;
    ClUint32T  size = 0;
    ClUint8T *value = NULL;
    ClVersionT 	version;
    ClUint8T srcArch = 0;
    ClUint32T sizeWithoutFilter = 0;
	
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("OBP", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    clOsalMutexLock(gCorMutexes.gCorServerMutex);

    if((rc = clXdrUnmarshallClVersionT(inMsgHandle, &version)) != CL_OK)
    {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
		return rc;
    }

    /* Client To Server Version Check */
    clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
    {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
    }

    memset(&usrFilter, 0, sizeof(ClCorObjAttrWalkFilterT));

    clBufferLengthGet(inMsgHandle, &size);
 
    rc = VDECL_VER(clXdrUnmarshallClCorObjectHandleIDLT, 4, 1, 0)(inMsgHandle, (void *) &objHIdl);
    if (rc != CL_OK)
    {
        clLogError("OAP", "EXP", "Failed to unmarshall ClCorObjectHandleIDLT_4_1_0. rc [0x%x]", rc);
        clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        return rc;
    }

    rc = clXdrUnmarshallClUint8T(inMsgHandle, &srcArch);
    if (CL_OK != rc)
    {
        clLogError("OAP", "EXP",
               "Failed while unmarshalling the source architecture. rc[0x%x]", rc);
        goto handleError;
    }

    /* check if filter is passed as NULL */
	/**
	 * TODO:
	 * kludge:
	 * In the case of NULL filter the size is compared with sizeof ClCorObjectHandleT.
     */

    clBufferReadOffsetGet(inMsgHandle, &sizeWithoutFilter);

    if(size > sizeWithoutFilter)
    {
		rc = clXdrUnmarshallClUint8T(inMsgHandle, &pUsrFilter->baseAttrWalk);
        if (CL_OK != rc)
        {
           clLogError("OAP", "EXP",
                   "Failed while unmarshalling the base attribute walk option. rc[0x%x]", rc);
           goto handleError;
        }

		rc = clXdrUnmarshallClUint8T(inMsgHandle, &pUsrFilter->contAttrWalk);
        if (CL_OK != rc)
        {
            clLogError("OAP", "EXP",
                    "Failed while unmarshalling the containment object walk option. rc[0x%x]", 
                    rc);
            goto handleError;
        }

		rc = VDECL_VER(clXdrUnmarshallClCorAttrPathT, 4, 0, 0)(inMsgHandle, &attrPath);
        if (CL_OK != rc)
        {   
            clLogError("OAP", "EXP",
                    "Failed while unmarshalling the attribute path. rc[0x%x]", rc);
            goto handleError;
        }

		rc = clXdrUnmarshallClInt32T(inMsgHandle, &pUsrFilter->attrId);
        if (CL_OK != rc)
        {
            clLogError("OAP", "EXP", "Failed while unmarshalling the attribute Id. rc[0x%x]", rc);
           goto handleError;
        }

        /* Read the parameters only if attribute id is valid. */
        if(pUsrFilter->attrId != CL_COR_INVALID_ATTR_ID)
        {
		    rc = clXdrUnmarshallClUint32T(inMsgHandle, &pUsrFilter->index);
            if (CL_OK != rc)
            {
                clLogError("OAP", "EXP",
                        "Failed while unmarshalling the index of the attribute. rc[0x%x]", rc);
                goto handleError;
            }

		    rc = VDECL_VER(clXdrUnmarshallClCorAttrCmpFlagT, 4, 0, 0)(inMsgHandle, &pUsrFilter->cmpFlag);
            if (CL_OK != rc)
            {
                clLogError("OAP", "EXP",
                        "Failed while unmarshalling the comparison flags. rc[0x%x]", rc);
                goto handleError;
            }

		    rc = VDECL_VER(clXdrUnmarshallClCorAttrWalkOpT, 4, 0, 0)(inMsgHandle, &pUsrFilter->attrWalkOption);	
            if (CL_OK != rc)
            {
                clLogError("OAP", "EXP",
                        "Failed while unmarshalling the attribute walk option. rc[0x%x]", rc);
                goto handleError;
            }

		    rc = clXdrUnmarshallClInt32T(inMsgHandle, &pUsrFilter->size);
            if (CL_OK != rc)
            {
                clLogError("OAP", "EXP",
                        "Failed while unmarshalling the attribute size. rc[0x%x]", rc);
                goto handleError;
            }

            size = usrFilter.size;
            if(size != 0)
            {
			    rc = clXdrUnmarshallPtrClCharT(inMsgHandle, (void **)&value, size);
                if (CL_OK != rc)
                {
                    clLogError("OAP", "EXP",
                            "Failed while unmarshalling the value. rc[0x%x]", rc);
                   goto handleError;
                }

                if(value == NULL)
	            {
		            clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
                    clCorObjectHandleFree(&(objHIdl.oh));
		            clLogError("OAP", "EXP", "The value obtained after unmarshalling is NULL"); 
		            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	            }
            }
        }
       /** check if the path is empty.**/
        if(attrPath.depth == 0)
            pUsrFilter->pAttrPath = NULL;
        else
            pUsrFilter->pAttrPath = &attrPath;
        /* value for filter */
        pUsrFilter->value = value;

    }
   
    if((rc = objHandle2DMHandle(objHIdl.oh, &dmObjHdl)!= CL_OK))
     {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
		if(value != NULL)
			clHeapFree(value);
        clCorObjectHandleFree(&(objHIdl.oh));
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Failed to get DM Object Handle \n"));
        return (rc);
     }

     rc = clCorDmExtObjPack(&dmObjHdl, srcArch, pUsrFilter, outMsgHandle, objHIdl.oh);
     if (CL_OK != rc)
         clLogError("OAP", "EXP", "Failed while packing the object. rc[0x%x]", rc);

    /* free value */
	if(value != NULL)
      clHeapFree(value);

handleError:

    clCorObjectHandleFree(&objHIdl.oh);
	clOsalMutexUnlock(gCorMutexes.gCorServerMutex);

    return (rc);
}
