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
 * File        : clCorObjHandle.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains MoId to Object Handle conversion routines
 *****************************************************************************/

/* INCLUDES */

#include <string.h>
#include <clCorMetaData.h>
#include <clDebugApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCpmApi.h>
#include <clCorErrors.h>

/* Internal Headers*/
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorObj.h"
#include "clCorPvt.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif
/* GLOBALS */
extern ClUint32T COR_ISBD;
extern ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh, ClNameT *moIdName);

#ifdef DEBUG
ClRcT corObjDbgInit()
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

/**
 *  Return the Object handle to the MSO instance from the MOID and Service ID.
 *
 *  This API returns the object handle corresponding to the MSO Object
 *  from an input MOID and service ID arguments.
 *                                                                        
 *  @param pMOId       Handle to the MO instance within the COR.
 *  @param srvcId     Service ID to which the MSO is associated
 *  @param objHandle  Reference to the COR Object handle that is returned
 *
 *  @returns CL_OK  - Success<br>
 */
#if 0
ClRcT
corMSOObjHandleGet(ClCorMOIdPtrT  pMOId, 
                   ClUint32T      srvcId, 
                   ClCorObjectHandleT* objHandle)
{
    ClInt32T rc = CL_OK;
    ObjTreeNode_h moHandle = 0;

    CL_FUNC_ENTER();

    if ((pMOId == NULL) || (objHandle == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMSOObjHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    clCorMoIdServiceSet(pMOId, srvcId);

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Service ID 0x%08x", (ClUint32T)srvcId));

    /* Get MO Object handle from MOID */
    if ((rc = corMOObjHandleGet(pMOId, objHandle)) != CL_OK)
    {
        ClCharT moIdStr[CL_MAX_NAME_LENGTH];
        moIdStr[0] = 0;

        CL_DEBUG_PRINT(CL_DEBUG_WARN, 
                ( "Failed to get MO Object handle from MOID {%s}", _clCorMoIdStrGet(pMOId, moIdStr)));
        return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
    }

    /* Make sure all the classes in the MOID are created within COR */

    /* make sure that the service object is set
     */
    moHandle = corMOObjGet(pMOId);
    if(moHandle)
      {
        CORObject_h mso = corMSOObjGet(moHandle, srvcId);
        if(mso && mso->dmObjH.classId)
          {
            /* just set the service id bit in the OH (compressed)
             * and return back 
             */
            rc = corOHServiceSet((COROH_h)(*objHandle).tree, srvcId);
            return(rc);
          }
          else {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "MSO object not found"));
            return(CL_COR_SET_RC(CL_COR_INST_ERR_MSO_NOT_PRESENT));
          }
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Failed to get MSO Object handle from MOID for srvcId %d", srvcId));
    return(CL_COR_SET_RC(CL_COR_INST_ERR_MSO_NOT_PRESENT));

}
#endif

/**
 *  Return the handle to the MO object from the MOID.
 *
 *  This API returns the handle to the object, given the MOID
 *                                                                        
 *  @param pMOId       MOID handle
 *  @param objHandle  Handle to the object instance
 *
 *  @returns CL_OK  - Success<br>
 */

#if 0
ClRcT
corMOObjHandleGet(ClCorMOIdPtrT          pMOId, 
                  ClCorObjectHandleT* objHandle)
{
    ClInt32T rc = CL_OK;
    ObjTreeNode_h moHandle = 0;

    CL_FUNC_ENTER();
    
    if ((pMOId == NULL) || (objHandle == NULL))
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOObjHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
	/* Make sure all the classes in the MOID are created within COR */
    if (clCorMoIdValidate(pMOId) != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid ClCorMOId specified"));
        return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
      }

    moHandle = corMOObjGet(pMOId);
    if (moHandle && moHandle->data && 
        ((CORObject_h)moHandle->data)->dmObjH.classId)
      {
        /* copy/set the compressed version of moId */
#if 0          
        rc = corMOId2OHGet (pMOId, (COROH_h)(*objHandle).tree);
        if(rc != CL_OK)
        {
           return (rc);
        }
#endif
        rc = clCorMoIdToObjectHandleGet(pMOId, objHandle);
        if (rc != CL_OK)
        {
            clLogError("OBJ", "HDL", "Failed to get the object handle from MoId. rc [0x%x]", rc);
            return rc;
        }

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nGot MO Object Handle, %p", (void *)moHandle));
        return(CL_OK);
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "MO OBJ: Did not find a valid MO Obj handle"));
    return(CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
}
#endif

ClRcT corMOObjHandleGet(ClCorMOIdPtrT pMOId, ClCorObjectHandleT* objHandle)
{
    ClRcT rc = CL_OK;

    if (!pMOId || !objHandle)
    {
        clLogError("OBJ", "HDL", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = _clCorObjectValidate(pMOId);
    if (rc != CL_OK)
    {
        clLogDebug("OBJ", "GET", "MO Object doesn't exists in COR. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMoIdToObjectHandleGet(pMOId, objHandle);
    if (rc != CL_OK)
    {
        clLogDebug("OBJ", "GET", "Failed to get the Object handle from MoId. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT corMSOObjHandleGet(ClCorMOIdPtrT pMOId,
                ClUint32T srvcId,
                ClCorObjectHandleT* objHandle)
{
    ClRcT rc = CL_OK;

    if (!pMOId || !objHandle)
    {
        clLogError("OBJ", "HDL", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorMoIdServiceSet(pMOId, srvcId);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "HDL", "Failed to set the MoId service. rc [0x%x]", rc);
        return rc;
    }

    rc = corMOObjHandleGet(pMOId, objHandle);
    if (rc != CL_OK)
    {
        clLogDebug("OBJ", "HDL", "Failed to get the MO object handle. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}


ClRcT _clCorObjectValidate(ClCorMOIdPtrT pMoId)
{
    ObjTreeNode_h moHandle = 0;

    if (pMoId == NULL)
    {
        clLogError("OBJ", "VAL", "NULL pointer passed in pMoId.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    moHandle = corMOObjGet(pMoId);

    if (! (moHandle && moHandle->data && 
        ((CORObject_h)moHandle->data)->dmObjH.classId))
    {
        clLogDebug("OBJ", "VAL", "MO object doesn't exist in COR.");
        return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
    }

    if (pMoId->svcId != CL_COR_INVALID_SVC_ID)
    {
        CORObject_h hMso = corMSOObjGet(moHandle, pMoId->svcId);
        if (! (hMso && hMso->dmObjH.classId))
        {
            clLogDebug("OBJ", "VAL", "MSO Object doesn't exist in COR.");
            return CL_COR_SET_RC(CL_COR_INST_ERR_MSO_NOT_PRESENT);
        }
    }

    /* MO and/or Mso object exists in COR. */
    return CL_OK;
}

/**
 *  Return the Object handle from the MOID.
 *
 *  This API returns the object handle corresponding to the user
 *  defined Object within COR, from the MOID.
 *                                                                        
 *  @param pMOId       MOID handle
 *  @param objHandle  Reference to the COR Object handle that is returned
 *
 *  @returns CL_OK  - Success<br>
 */
 ClRcT
 _clCorObjectHandleGet(ClCorMOIdPtrT pMOId, ClCorObjectHandleT *objHandle)
{
    ClRcT rc = CL_OK;
   
    /*
    ClIocNodeAddressT destOM;
    ClIocNodeAddressT localIoc;
    ClIocNodeAddressT masterIoc;
    ClCorObjFlagsT flags;
    ClHandleT eoArg = 0;
    ClEoExecutionObjT* myEOOb;
    ClBufferHandleT  inMsgHdl;
    ClBufferHandleT  outMsgHdl;
    ClUint32T   outHdlSize = sizeof(ClCorObjectHandleT);

    ClUint32T outLen = sizeof(ClCorObjectHandleT);*/
    
    if ((pMOId == NULL) || (objHandle == NULL))
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

#ifdef DISTRIBUTED_COR
     /*Determine from where the object handle has to be fetched */
    rc = _clCorMoIdToLogicalSlotGet(pMOId, &destOM);
    if(rc != CL_OK)
    	{
    	CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
		  ( "corAtMOId2IocAddrGet failed rc => [0x%x]", rc));
	/*Just call the local function if could not get blade address from ClCorMOId*/
           if((rc = clBufferCreate(&inMsgHdl))!= CL_OK)
            {
                  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferCreateAndAllocate - Failed.."));
                 return (rc);
            }
            if((rc = clBufferNBytesWrite (inMsgHdl, (ClUint8T*)pMOId,
		                                    sizeof(ClCorMOIdT))) != CL_OK)
           {
             CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferNBytesWrite  - Failed.."));
	     return (rc);
           }
           if((rc = clBufferCreate(&outMsgHdl))!= CL_OK)
           {
              CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferCreateAndAllocate - Failed.."));
              return (rc);
           }
	   rc = _corObjLocalHandleGet(eoArg, inMsgHdl, outMsgHdl);
           /* Read from the message*/
           clBufferNBytesRead (outMsgHdl, (ClUint8T *)objHandle, &outHdlSize);
        
          /* Delete the in/out messages now.*/
           clBufferDelete(&inMsgHdl);
           clBufferDelete(&outMsgHdl);
           return (rc);
    	}
	
    localIoc = clIocLocalAddressGet();
	
    rc = _clCorObjectFlagsGet(pMOId, &flags);
    if(rc != CL_OK)
    	{
    	CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
		  ( "clCorObjectFlagsGet failed rc => [0x%x]", rc));
	return rc;
    	}
		
    flags &= CL_COR_OBJ_CACHE_MASK;

   if  ((flags == CL_COR_OBJ_CACHE_ONLY_ON_MASTER) && (COR_ISBD))
   	{

	rc = clCpmMasterAddressGet(&masterIoc);
   	
	if(rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get master's IOC Address.  rc => [0x%x]", rc));
		return rc;
		}
	/*TODO: RMD call to be enabled, when distributed COR is enabled. */
	/*
   	rc = !!!OBSOLETErmdCallPayloadReturn!!!(masterIoc, 
   							CL_IOC_COR_PORT, 
   							COR_EO_GET_LOCAL_OBJECT_HANDLE,
   							(char*)pMOId,
   							sizeof(ClCorMOIdT),
   							(char *)objHandle,
   							&outLen,
   							1000,
   							0,
   							0);
							*/
	return rc;
   	}
   else if( (destOM == localIoc) ||
          (flags == CL_COR_OBJ_CACHE_GLOBAL) ||
          ((flags == CL_COR_OBJ_CACHE_ON_MASTER) && (!COR_ISBD)) ||
          ( ((flags == CL_COR_OBJ_CACHE_ONLY_ON_MASTER) && (!COR_ISBD))))
   	{
           if((rc = clBufferCreate(&inMsgHdl))!= CL_OK)
            {
                  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferCreateAndAllocate - Failed.."));
                 return (rc);
            }
            if((rc = clBufferNBytesWrite (inMsgHdl, (ClUint8T*)pMOId,
		                                    sizeof(ClCorMOIdT))) != CL_OK)
            {
              CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferNBytesWrite  - Failed.."));
	      return (rc);
            }
            if((rc = clBufferCreate(&outMsgHdl))!= CL_OK)
            {
               CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clBufferCreate - Failed.."));
               return (rc);
            }
	   rc = _corObjLocalHandleGet(eoArg, inMsgHdl, outMsgHdl);
           /* Write to the message*/
           clBufferNBytesRead(outMsgHdl, (ClUint8T *)objHandle, &outHdlSize);
           /* Delete in/out messages */
	   clBufferDelete(&inMsgHdl);
           clBufferDelete(&outMsgHdl);
   	return( rc );
   	}
   else
   	{
	/*TODO: RMD might be needed when cor runs in distributed mode. */
	/*
   	rc = !!!OBSOLETErmdCallPayloadReturn!!!(destOM, 
   							CL_IOC_COR_PORT, 
   							COR_EO_GET_LOCAL_OBJECT_HANDLE,
   							(char*)pMOId,
   							sizeof(ClCorMOIdT),
   							(char *)objHandle,
   							&outLen,
   							1000,
   							0,
   							0);
        */
	return rc;

   	}
#else
    /* first check the ClCorMOId for the tree portion of the COR.  After it
     * matches the tree, then match the containment that is within the
     * DM
     */
        if (pMOId->depth == 0) /* Additional check */
        {
           return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }

        if(pMOId->svcId >= 0)
        {
            rc = corMSOObjHandleGet(pMOId, pMOId->svcId, objHandle);
        }
        else 
        {
	    rc = corMOObjHandleGet(pMOId, objHandle);
        } 
#endif

  return (rc);
}


/**
 *  Return the Object handle from the MOID.
 *
 *  This API returns the object handle corresponding to the user
 *  defined Object within COR, from the MOID. Will return object
 * handle of the object which is local to the blade.
 *                                                                        
 *  @param this       MOID handle
 *  @param objHandle  Reference to the COR Object handle that is returned
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
_corObjLocalHandleGet(ClUint32T cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
#ifdef DISTRIBUTED_COR
    int depth;
   /* DMObjHandle_t tmpHandle;
    ClCorObjectHandleT tmpHandle1; */
    ClCorMOIdPtrT pMOId = NULL;
    ClCorObjectHandleT  objH;
    ClCorObjectHandleT* objHandle = &objH;
   
    CL_FUNC_ENTER();
   
    if((rc = clBufferFlatten(inMsgHandle, (ClUint8T **)&pMOId))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
         CL_FUNC_EXIT();
         return rc;
    }

    if (pMOId == NULL)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
     }

    /* first check the ClCorMOId for the tree portion of the COR.  After it
     * matches the tree, then match the containment that is within the
     * DM
     */
    depth = pMOId->depth;
    if ( 0 == depth ) /* Additional check */
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
    }

        if(pMOId->svcId >= 0)
        {
            rc = corMSOObjHandleGet(pMOId, pMOId->svcId, objHandle);
        } else {
	    rc = corMOObjHandleGet(pMOId, objHandle);
        } 
    if(rc!=CL_OK)
      {
        CL_FUNC_EXIT();
        return (rc);
      }
 
    if(pMOId->depth==depth)
      {
       /* Write to the message*/
        clBufferNBytesWrite (outMsgHandle, (ClUint8T *)objHandle, sizeof(ClCorObjectHandleT));
        CL_FUNC_EXIT();
        return (rc);
      }
    /* Get the nodes from containment (DM) */
	
      if(rc == CL_OK)
      {
    /* Write to the message*/
       clBufferNBytesWrite (outMsgHandle, (ClUint8T *)objHandle, sizeof(ClCorObjectHandleT));
      }
#endif
    return(rc); 
}


/**
 *  Return the type of object based on the object handle argument.
 *
 *  This API returns the type of the object associated with the object
 *  handle. The possible types supported are MO, MSO and aribitary objects.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *  @param type       Type of COR object associated with the handle
 *
 *  @returns CL_OK  - Success<br>
 */
#if 0
ClRcT
_clCorObjectHandleToTypeGet(ClCorObjectHandleT objHandle, ClCorObjTypesT* type)
{

	ClRcT Rc;
	ClCorMOIdT moId;

        if(NULL == type)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL arguments"));
            return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
        }

	COROH_h corObjHandle = (COROH_h)(&(objHandle.tree));

        CL_FUNC_ENTER();
	if( (Rc = corOH2MOIdGet(corObjHandle, &moId)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid Handle"));
		return Rc;
		}

	if( (Rc = clCorMoIdValidate(&moId)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid Handle"));
		return Rc;
		}

	if(moId.svcId == CL_COR_INVALID_SVC_ID)
		{
		*type = CL_COR_OBJ_TYPE_MO;
		return CL_OK;
		}
	else
		{
		*type = CL_COR_OBJ_TYPE_MSO;
		return CL_OK;
		}
	
}
#endif

ClRcT
_clCorObjectHandleToTypeGet(ClCorObjectHandleT objHandle, ClCorObjTypesT* pObjType)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;

    if (!pObjType)
    {
        clLogError("OBJ", "GET", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorObjectHandleToMoIdGet(objHandle, &moId, NULL);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "GET", "Failed to get the MoId. rc [0x%x]", rc);
        return rc; 
    }

    rc = _clCorObjectValidate(&moId);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "GET", "MO or MSO object doesn't exist. rc [0x%x]", rc);
        return rc;
    }

    if (moId.svcId == CL_COR_INVALID_SVC_ID)
        *pObjType = CL_COR_OBJ_TYPE_MO;
    else
        *pObjType = CL_COR_OBJ_TYPE_MSO;

    return CL_OK;
}

/**
 *  Return the MOID given the objHandle.
 *
 *  This API returns the MOID given the objHandle.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *  @param moId       ClCorMOId
 *
 *  @returns CL_OK  - Success<br>
 */
#if 0
ClRcT
_clCorObjectHandleToMoIdGet(ClCorObjectHandleT this, 
		      ClCorMOIdPtrT pMoId, 
		      ClCorServiceIdT *srvcId)
{
    ClRcT rc;

    CL_FUNC_ENTER();
    
    if ((moId == NULL) || (srvcId == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Object Handle 0x%08x, Service id %d",
			*(ClUint32T*)this.tree, (ClUint32T)*srvcId));

    rc = clCorObjectHandleToMoIdGet(this, pMoId, srvcId
    rc = corOH2MOIdGet ((COROH_h)this.tree, moId);
    if(rc==CL_OK)
      {
        *srvcId = moId->svcId;
      }

    CL_FUNC_EXIT();
    return (rc);
}
#endif

/**
 *  Lock the object instance.
 *
 *  This API Lock the object instance.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
corObjHandleLock(ClCorObjectHandleT objHandle)
{
    ClRcT ret = CL_OK;
    DMObjHandle_t dmo;

    CL_FUNC_ENTER();

    ret = objHandle2DMHandle(objHandle,&dmo);
    if(ret == CL_OK)
      {
        ret = dmObjectLock(&dmo);
      }
		
    CL_FUNC_EXIT();
    return (ret);
}


/**
 *  Unlock the object instance.
 *
 *  This API Unlocks the object instance.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
corObjHandleUnlock(ClCorObjectHandleT objHandle)
{
    ClRcT ret = CL_OK;
    DMObjHandle_t dmo;

    CL_FUNC_ENTER();

    ret = objHandle2DMHandle(objHandle,&dmo);
    if(ret == CL_OK)
      {
        ret = dmObjectUnlock(&dmo);
      }
	
    CL_FUNC_EXIT();
    return (ret);
}

#if 0
/**
 *  Copy out the object attributes.
 *
 *  This API Copies out the object attributes.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *  @param moId       ClCorMOId
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT 
corObjCopyGet (ClCorMOIdPtrT moIdh, 
               ClCorServiceIdT svc,
               void * pBuf,
               ClUint32T *pSize)
{
    ClRcT ret = CL_OK;
    ClCorObjectHandleT objHandle;
    DMObjHandle_t dmo;
  
    CL_FUNC_ENTER();

    
    
    

    if ((moIdh == NULL) || (pBuf == NULL) || (pSize == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL ptr received"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    if (svc == CL_COR_INVALID_SRVC_ID)
        ret = corMOObjHandleGet(moIdh, &objHandle);
    else
        ret = corMSOObjHandleGet(moIdh, svc, &objHandle);
    if(ret == CL_OK) 
      {
        ret = objHandle2DMHandle(objHandle,&dmo);
        if(ret == CL_OK)
          {
            ret = dmObjectCopy(&dmo, pBuf, pSize);
          }
      }

    CL_FUNC_EXIT();
    return (ret);
}
#endif

/**
 *  Return bits that reflect the attributes modified within the object.
 *
 *  This API returns the bits that reflect the attributes modified 
 *  within the object.
 *                                                                        
 *  @param moIdh     ClCorMOId
 *  @param svc     	 Service ID
 *  @param attList   Attribute list
 *  @param pAttrBits Storage where the bits are returned
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
corObjAttrBitsGet (ClCorMOIdT  *pMoId,
                   ClCorAttrPathPtrT  pAttrPath,
                   ClCorAttrListPtrT attList, 
                   ClCorAttrBitsT *pAttrBits)
{
    ClRcT ret = CL_OK;
    DMContObjHandle_t dmContObjHdl;
  
    CL_FUNC_ENTER();


    if ( (attList == NULL) || (pAttrBits == NULL)) 
    {
        CL_DEBUG_PRINT (CL_DEBUG_TRACE, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
        ret = moId2DMHandle(pMoId, &(dmContObjHdl.dmObjHandle));
        if(ret == CL_OK)
          {
           /* Set the attr Path.*/
            dmContObjHdl.pAttrPath = pAttrPath;
            ret = dmObjectBitmap(&dmContObjHdl, 
                                 (ClCorAttrIdT*)&attList->attr,
                                 attList->attrCnt,
                                 pAttrBits->attrBits,
                                 sizeof(ClCorAttrBitsT));
          }

    CL_FUNC_EXIT();
    return (ret);
}
#if REL_2
/**
 *  Return the Object handle from the MOID even if object is not created.
 *
 *  This API returns the object handle which is compressed ClCorMOId, given
 * ClCorMOId. An object handle can also be obtained using _clCorObjectHandleGet
 * API. But _clCorObjectHandleGet will succeed only if the object is already created.
 *                                                                        
 *  @param pMOId       MOID handle
 *  @param objHandle  [OUT] Reference to the COR Object handle that is returned
 *
 *  @returns CL_OK  - Success<br>
 */

ClRcT
clCorMoIdCompress(ClCorMOIdPtrT pMOId, ClCorObjectHandleT * objHandle)
{
    ClRcT rc = CL_OK;
     COROH_t objH;

	 CL_FUNC_ENTER();
	 
    if ((pMOId == NULL) || (objHandle == NULL))
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectHandleGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
     }

	if((rc = corMOId2OHGet(pMOId, &objH)) != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get OH from ClCorMOId"));
		return rc;
		}
	
	/* First make sure that the handle is initialized to NULL*/
	CL_COR_OBJ_HANDLE_INIT(*objHandle);
	memcpy(objHandle, &objH, sizeof(COROH_t));

	return CL_OK;
	
}
#endif 

#ifdef COR_TEST
/**
 *  Display the object contents corresponding to the object handle.
 *
 *  This API display the object contents corresponding to the 
 *  object handle.
 *                                                                        
 *  @param objHandle  Handle to the object instance
 *
 *  @returns CL_OK  - Success<br>
 */
void
clCorObjectHandleShow(ClCorObjectHandleT objHandle)
{
     int i;
    CL_FUNC_ENTER();

    /*TODO: Print the fields with the object handle, and then dump the
     *      object contents.
     */
	
    clOsalPrintf("Tree : ");
    for(i=0;i<8;i++)
    	clOsalPrintf("%4x\t",objHandle.tree[i]);
	
   clOsalPrintf("\n");

    CL_FUNC_EXIT();
}
#endif
