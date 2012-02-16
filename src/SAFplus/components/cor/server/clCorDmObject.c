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
 * File        : clCorDmObject.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * Handles Data Manager - Instance related routines
 *****************************************************************************/

/* FILES INCLUDED */

#include <stdio.h>
#include <string.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clBitApi.h>
#include <netinet/in.h>
#include <clTxnApi.h>
#include <clCorApi.h>

/*Internal Headers*/
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorLog.h"
#include "clCorTxnClientIpi.h"
//#include "clCorOHLib.h"
#include "clCorRmDefs.h"
#include "clCorTxnInterface.h"

#include <xdrClCorObjectAttrInfoT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALS */
int gObjectCount = 0;
extern void * dmObjectAttrConvert(void *buf, ClInt64T value, CORAttrInfo_t type, ClUint32T size);
extern ClRcT _clCorOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass);

/* STATIC */
static ClCntHandleT attrWalkRTCont = 0;
static ClOsalMutexIdT attrWalkMutex = 0;

static ClRcT _dmObjectInitializedAttrSetCB(CORAttr_h attrH, Byte_h* arg)
{
    ClCorInstanceIdT instId = 0;
    Byte_h* pObjBuf = NULL;
    ClUint32T size = 0;
	ClInt64T attrValue = 0;

    if (! attrH || ! arg)
    {
        clLogError("DM", "INIT", "NULL argument passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pObjBuf = (Byte_h*) arg[0];
    instId = *(ClCorInstanceIdT *) arg[1];

	attrValue = attrH->attrValue.min + instId;

    size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

    if((attrH->userFlags & CL_COR_ATTR_INITIALIZED) && 
            (attrH->attrType.type != CL_COR_ARRAY_ATTR &&
            attrH->attrType.type != CL_COR_CONTAINMENT_ATTR &&
            attrH->attrType.type != CL_COR_ASSOCIATION_ATTR))
    {
        dmObjectAttrConvert(*pObjBuf, attrValue, attrH->attrType, size);
    }

    *pObjBuf += size;

    return CL_OK;
}

ClRcT dmObjectInitializedAttrSet(DMObjHandle_h objHandle, ClCorInstanceIdT instId)
{
    ClRcT rc = CL_OK;
    DMContObjHandle_t objContHandle;
    Byte_h obj = NULL;
    CORClass_h classH = NULL;
    Byte_h arg[] = {
        (Byte_h) &obj,
        (Byte_h) &instId
    };

    objContHandle.dmObjHandle.classId = objHandle->classId;
    objContHandle.dmObjHandle.instanceId = objHandle->instanceId;
    objContHandle.pAttrPath = NULL;

    rc = dmObjectAttrHandleGet(&objContHandle, &classH, (void *) &obj);
    if (rc != CL_OK)
    {
        clLogError("DM", "INIT", "Failed to get DM object handle. rc [0x%x]", rc);
        return rc;
    }

    obj += sizeof(CORInstanceHdr_t);    
    
    rc = dmClassAttrWalk(classH, NULL, _dmObjectInitializedAttrSetCB, (Byte_h *) arg);
    if (rc != CL_OK)
    {
        clLogError("DM", "INIT", "Failed to walk attributes of class [%d]. rc [0x%x]", 
                classH->classId, rc);
        return rc;
    }

    return CL_OK;
}

/** 
 * Object init.
 *
 * API to Given an Object Handle, initializes it as specified in the
 * class type definition.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectInit(DMObjHandle_h objHandle
             )
{
    ClRcT ret = CL_OK;
    Byte_h obj = 0;
    CORInstanceHdr_t hdr = 0;
    CORClass_h cls = 0;
    DMContObjHandle_t objContHandle;
 
  
    CL_FUNC_ENTER();

    if((!dmGlobal) || (!objHandle))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
 
    objContHandle.dmObjHandle.classId = objHandle->classId;
    objContHandle.dmObjHandle.instanceId = objHandle->instanceId;
    objContHandle.pAttrPath  = NULL;

    /*
     *   Lock the object, initialize all the attributes to def value 
     */
    /*DM_OH_GET_VAL(*objHandle, obj); */
    if(CL_OK == (ret = dmObjectAttrHandleGet(&objContHandle, &cls,(void *)&obj)))
    {
        memcpy(&hdr, obj, sizeof(hdr));
 
        COR_INSTANCE_HDR_INIT(hdr);
        /* COR_INSTANCE_LOCK(hdr, dmGlobal->eoId);*/
        COR_INSTANCE_ACTIVE(hdr);
    
        memcpy(obj, &hdr, sizeof(hdr));
        obj += sizeof(CORInstanceHdr_t);
        ret = dmClassAttrWalk(cls,
                          NULL, 
                          dmObjectAttrInit,
                          &obj);
    
        /* unlock the object 
         */
        COR_INSTANCE_UNLOCK(hdr);
    }

    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Create Object Instance.
 *
 *  API to create object instace for the specified class id.  The
 *  newly created object is added to the containment hierarchy as
 *  specified by objHier.
 *
 *  The object consists of
 *   |------------------------|
 *   |     Instance Header    |
 *   |------------------------|
 *   |      Object Buffer     |
 *   | (Base Classes if reqd) |
 *   |   Current Class Attrs  |
 *   |------------------------|
 *
 *  @param this    Data Manager Handle
 *  @param objHier Object Hierarchy Handle
 *  @param cid     Class Identifier
 *
 *  @returns 
 *    CORInstance_h  (non-null) newly created object instance handle </br>
 *      null(0)      on failure.
 * 
 */
ClRcT
dmObjectCreate(ClCorClassTypeT  id,  
               DMObjHandle_h* objHandle
               )
{
    ClRcT     ret = CL_OK;
    CORClass_h tmp = 0;
    ClInt32T sz = 0;
    void * obj = 0;
    DMObjHandle_h  oh = 0;
    ClRcT rc = CL_OK;
    ClUint32T instanceId = 0;

    CL_FUNC_ENTER();

    if((!dmGlobal) || (!objHandle))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create (Class:%04x)", id));
    
    tmp = dmClassGet(id);
    if (tmp)
    {
        if (tmp->objCount == 0)
        {
            /* First instance is created.
             * Compute the class size.
             */
            rc = dmClassSize((CORHashKey_h)(ClWordT) tmp->classId, (CORHashValue_h) tmp, 0, 0);
            if (rc != CL_OK)
            {
                clLogError("DM", "CLS", "Failed to compute size of the class [0x%x]", tmp->classId);
                return rc;
            }

            /* Update the size of the class */
            sz = tmp->size;
        }

        COR_LLIST_GET_FIRST_KEY(tmp->objFreeList, instanceId, ret);
        if (ret == CL_OK)
        {
              COR_LLIST_REMOVE(tmp->objFreeList, (ClWordT)instanceId);
        }
        else
        {
            if(!tmp->objects.head && !tmp->objects.tail)
            {
                corVectorInit(&tmp->objects, 
                          sz+sizeof(CORInstanceHdr_t),
                          tmp->objBlockSz);
            }

            instanceId = tmp->recordId - 1;
        }

        /* By this moment, the objects vector should have been already
         * initialised, so just do a get of the instanceId.
         */
        obj = corVectorExtendedGet(&tmp->objects, instanceId);
        if(obj) 
        {            
            /* create the handle and init 
             */
            *objHandle = (DMObjHandle_h) clHeapAllocate(sizeof(DMObjHandle_t));
            if(*objHandle) 
            {
                oh = (DMObjHandle_h) *objHandle;
                DM_OH_SET(*oh, tmp, instanceId);

                ret = dmObjectInit(oh);
                if(ret==CL_OK)
                {
                    /*
                     * todo: Should take care of reaching
                     * 2^32. Especially in case of objects that are
                     * created and deleted often, this might be
                     * reached soon.
                     */
                    if (instanceId == (tmp->recordId - 1))
                    {
                        tmp->recordId++;
                    }

                    tmp->objCount++;
                    gObjectCount++;
                }
            } 
            else 
            {
		        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR_MEM_ALLOC_FAIL)); 
                ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }      
        } 
        else 
        {
            clLogError("DM", "CLS", "Failed to get the object vector node for class [%d], instance [%u]", 
                    tmp->classId, instanceId);
            ret = CL_COR_SET_RC(CL_COR_ERR_OBJ_NOT_PRESENT);
        } 
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Create (Class:%04x) [Unknown Class]", id));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
    }
    
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Delete Object Instance.
 *
 *  API to delete object instace specified in the objHier (Object
 *  Hierarchy).
 *
 *  @param this    Data Manager Handle
 *  @param objHier Object Hierarchy Handle
 *
 *  @returns 
 *    CORInstance_h  (non-null) deleted object instance handle </br>
 *      null(0)      on failure.
 * 
 */
/* note: assumed that the DMObjHandle_t other itesm are freed
 * before somebody calls out here.
 */
ClRcT
dmObjectDelete(DMObjHandle_h this
               )
{
    ClRcT ret = CL_OK;
    CORInstanceHdr_h hdr = NULL;
    CORInstanceHdr_t tmpHdr = 0;
    DMObjHandle_h objHandle = (DMObjHandle_h) this;
    CORClass_h classH;
    
    CL_FUNC_ENTER();

    if((!dmGlobal) || (!objHandle))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    DM_OH_DBG_VERBOSE(*objHandle, CL_DEBUG_TRACE, "Delete");

    /* check lock and decrement instances before
     * freeing the memory.
     */
    /*DM_OH_GET_VAL(*objHandle, hdr);*/
    DM_OH_GET_VAL(objHandle, hdr);
    
    if(hdr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nFailed to get the Cor Instance Header.\n"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memcpy(&tmpHdr, hdr, sizeof(tmpHdr));

    /* todo: need to check if contained objects have been
     * locked/not
     */
    if(COR_INSTANCE_IS_LOCKED(tmpHdr)
		   /*&& COR_INSTANCE_OWNER(tmpHdr)!=dmGlobal->eoId */) 
      {
        ret = CL_COR_SET_RC(CL_COR_ERR_OBJECT_LOCKED);
      } 
    else if(COR_INSTANCE_IS_CONTAINED(tmpHdr))
      {
        ret = CL_COR_SET_RC(CL_COR_ERR_OBJECT_INVALID);
      }
    else 
      {
        /* todo: need to go thru and pack the vector and also set the
         * recordId appropriately. Action item for later releases.
         */
        ret=dmObjectDisable(this);
        if (ret != CL_OK)
        {
            CL_FUNC_EXIT();
            return ret;
        }
      }

	/*All the operations are successful. Now modify num instances in COR class*/
	classH = dmClassGet(this->classId);
    /* Decrementing the object count as we delete the object. */
	classH->objCount--;
    gObjectCount--;
    
    COR_LLIST_PUT(classH->objFreeList, (ClWordT)this->instanceId, 0);

    /* Freeing the object vector when the object count reaches zero. */
    if(classH->objCount == 0)
    {
        corVectorRemoveAll(&classH->objects);
        classH->recordId = 1;
        COR_LLIST_REMOVE_ALL(classH->objFreeList, ret);
        if (ret != CL_OK)
        {
            clLogError("DMO", "DEL", "Failed to remove obj free list. rc [0x%x]", ret);
        }
    }
  
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Lock object.
 *
 * API to Lock specified object (using handle).
 *
 *  @param this       object handle
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectLock(DMObjHandle_h this
             )
{
    ClRcT ret = CL_OK;
    DMObjHandle_h objHandle = (DMObjHandle_h) this;
  
    CL_FUNC_ENTER();

    if(dmGlobal && objHandle)
      {
        DM_OH_DBG_VERBOSE(*objHandle, CL_DEBUG_TRACE, "Lock");

      }

    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Unlock object.
 *
 * API to UnLock specified object (using handle).
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectUnlock(DMObjHandle_h this
               )
{
    ClRcT ret = CL_OK;
    DMObjHandle_h objHandle = (DMObjHandle_h) this;

    CL_FUNC_ENTER();

    if(dmGlobal && objHandle)
      {
        DM_OH_DBG_VERBOSE(*objHandle, CL_DEBUG_TRACE, "UnLock");

      }

    CL_FUNC_EXIT(); 
    return (ret);
}

/** 
 * Enable object (already created).
 *
 * API to Enable a given object.  The object is already allocated and
 * initialised, but it may be enabled/disabled.  If the object is
 * disabled, then this function would enable it.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectEnable(DMObjHandle_h this
               )
{
    ClRcT ret = CL_OK;
    CORInstanceHdr_h hdr = 0;
    CORInstanceHdr_t tmpHdr = 0;
    DMObjHandle_h objHandle = (DMObjHandle_h) this;
  
    CL_FUNC_ENTER();

    if((!dmGlobal) || (!objHandle))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    DM_OH_DBG_VERBOSE(*objHandle, CL_DEBUG_TRACE, "Enable");
    
    /*DM_OH_GET_VAL(*objHandle, hdr); */
    DM_OH_GET_VAL(objHandle, hdr);
    
    
    /* check if already enabled
     */
    if(hdr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the COR intance header."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memcpy(&tmpHdr, hdr, sizeof(tmpHdr));
    

    if(COR_INSTANCE_IS_ACTIVE(tmpHdr)) 
      {
        DM_OH_DBG_VERBOSE(*objHandle, CL_DEBUG_ERROR, "[Can't enable. Already Active]");
        ret = CL_COR_SET_RC(CL_COR_ERR_OBJECT_ACTIVE);        
      } 
    else 
      {
        COR_INSTANCE_ACTIVE(tmpHdr);
        memcpy(hdr, &tmpHdr, sizeof(tmpHdr));
      }
    
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 * Disable object.
 *
 * API to Disable a given object.  The object should be already
 * present and active to make it disable.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectDisable(DMObjHandle_h this)
{
    ClRcT ret = CL_OK;
    CORInstanceHdr_h hdr = 0;
    CORInstanceHdr_t tmpHdr = 0;
    DMObjHandle_h  objHandle = (DMObjHandle_h) this;

    CL_FUNC_ENTER();

    if((!dmGlobal) || (!objHandle))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Disable {%04x/%x}", 
                          objHandle->classId,
                          objHandle->instanceId));
    
    /*DM_OH_GET_VAL(*objHandle, hdr);*/
    DM_OH_GET_VAL(objHandle, hdr);
    if(hdr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the COR intance header."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memcpy(&tmpHdr, hdr, sizeof(tmpHdr));

    /* check if already enabled
     */
    if(!COR_INSTANCE_IS_ACTIVE(tmpHdr)) 
      {
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Disable (Class:%04x) [Invalid object]", 
                              objHandle->classId));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_OBJECT_INVALID);
        
      } 
    else 
      {
        COR_INSTANCE_UNUSED(tmpHdr);
        memcpy(hdr, &tmpHdr, sizeof(tmpHdr));
      }
    
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * copy object.
 *
 * API to Copy only the instance (not the instance header) into the
 * copy buffer specfied. size is an in/out parameter.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectCopy(DMObjHandle_h this,
             ClBufferHandleT *pBufH,
             ClUint32T*    size
             )
{
    ClRcT ret = CL_OK;
    CORInstanceHdr_h hdr = NULL;
    CORInstanceHdr_t tmpHdr = 0;
    DMObjHandle_h objHandle = (DMObjHandle_h) this;
    
    CL_FUNC_ENTER();

    
    if((!objHandle) || (!size) || (!dmGlobal))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Copy (Class:%04x, Sz:%d)", 
                          objHandle->classId,
                          *size));
    
    /* check lock and then copy only the object
     * without any header info
     */
    /*DM_OH_GET_VAL(*objHandle, hdr);*/
    DM_OH_GET_VAL(objHandle, hdr);
    if(hdr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the COR intance header."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memcpy(&tmpHdr, hdr, sizeof(tmpHdr));
    
    /* check if already locked and who is the owner 
     * for the lock.
     */
    if(COR_INSTANCE_IS_LOCKED(tmpHdr) /* && 
       COR_INSTANCE_OWNER(tmpHdr) != dmGlobal->eoId */) 
      {
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Copy (Class:%04x) [Locked]", 
                              objHandle->classId));
        
        /* locked by some body else 
         */
        ret = CL_COR_SET_RC(CL_COR_ERR_OBJECT_LOCKED);
      } 
    else
    {
        CORClass_h cls;

        DM_OH_GET_TYPE(*objHandle, cls);
        if(cls->size < *size) 
        {
            *size = cls->size;
        }
        if(*size > 0) 
        {
            ret = clBufferNBytesWrite(*pBufH, 
                        (ClUint8T *)((Byte_h)hdr + sizeof(CORInstanceHdr_t)), *size);
            if(CL_OK != ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the objet buffer of"
                    " the class [0x%x] size [%d]. rc[0x%x]", cls->classId, *size, ret));
                return ret;
            }
        }
    }
    
    CL_FUNC_EXIT(); 
    return (ret);
}

/** 
 * Get the bitmap of attribute list.  
 *
 * API to return back bitmap of attributes that are specified in
 * attrList.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmObjectBitmap(DMContObjHandle_h this, 
               ClCorAttrIdT*   attrList,
               ClInt32T      nAttrs,
               ClUint8T*     bits,
               ClUint32T       bitMaskSz
               )
{
    ClRcT ret = CL_OK;
    DMContObjHandle_h objHandle = (DMContObjHandle_h) this;
    CORClass_h corClass;
    void *instance; 


    CL_FUNC_ENTER();

    
    if((!dmGlobal) || (!objHandle) || (!attrList) || (!bits))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    
     ret=dmObjectAttrHandleGet(objHandle, &corClass, &instance); 
    if(ret != CL_OK)
     {
  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get class handle from DM handle. Rc = 0x%x\n", ret));
        return ret;
     }
     CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Bitmap (Class:%04x nAttrs:%d bitmaskSz:%d)", 
                          corClass->classId,
                          nAttrs,
                          bitMaskSz)); 
		
    ret = dmClassBitmap(corClass->classId, attrList, nAttrs, bits, bitMaskSz);

    CL_FUNC_EXIT(); 
    return ret;
}

#if REL2
/** 
 * Get the containment object based on type.
 *
 * API to dmObjectContGet 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */
ClRcT 
dmObjectType2Id(DMObjHandle_h  this, 
                ClCorClassTypeT atype, 
                ClCorAttrIdT*   aid
                )
{
    ClRcT ret;
    ClInt32T* type = aid;
    CORClass_h class;
    void *     instance;

    CL_FUNC_ENTER();
    if ((this == NULL) || (aid == NULL))
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    *aid = atype;
    ret = dmObjectAttrHandleGet(this, &class, &instance);
    if(ret==CL_OK)
      {
        ret = dmClassAttrWalk(class,
                              NULL,
                              dmClassAttrType2Id,
                              (Byte_h*) &type);
        if(ret == CL_COR_SET_RC(CL_COR_ERR_CLASS_PRESENT))
          {
            ret = CL_OK;
          }
        else
          {
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
          }
      }
    
    CL_FUNC_EXIT();  
    return(ret);
}
#endif




#if 0  /* Not required any more */
/** 
 * Get the containment object.
 *
 * API to dmObjectContGet <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT 
dmObjectContGet(DMObjHandle_h this, 
                ClCorAttrIdT    aid, 
                DMObjHandle_h containedObject, 
                ClInt32T      idx
                )
{

    ClRcT ret;
    ClUint32T size = sizeof(DMObjHandle_t);

    CL_FUNC_ENTER();
    
    /* todo: make sure the aid is a container 
     */
	
    ret = dmObjectAttrGet(this, aid, containedObject, &size, idx);
    if(ret == CL_OK)
      {
        /* first copy this handle to value and containedObject and then
         * call AttrGet
         */
        if(this && containedObject)
          {
            *containedObject = *this;
            /* todo: change aid to index later */
            ret = DMohContAdd((DMObjHandle_h)containedObject, aid, idx);
             */
          }
      }
    
    CL_FUNC_EXIT();  
    return(ret);
return 0;
}
#endif


/** 
 * Get the attribute value.
 *
 * API to dmObjectAttrGet <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT dmObjectAttrGet(DMContObjHandle_h this, 
                       ClCorAttrIdT aid,
                       ClInt32T idx,
                       void * value,
                       ClUint32T *size)
{
    Byte_h attrBegin = 0;
    ClRcT ret = CL_OK;
    CORAttr_h attrH = 0;
    CORClass_h class;
    void *     instance;

    CL_FUNC_ENTER();

    if((!this) || (!value) || (!size))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }


    /*DM_OH_GET(*this, class, instance);*/
    ret = dmObjectAttrHandleGet(this, &class, &instance);
    if(ret != CL_OK)
      {
        CL_FUNC_EXIT();
        return(ret);
      }

    attrH = dmClassAttrGet(class, aid);
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
              ( "Get (Class:%04x, Attr:%x, Size:%d, Index: %d)", 
               class->classId,
               aid,
               *size,
               idx));
    
    if(!attrH || attrH->offset < 0) 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                  ( "Get (Class:%04x, Attr:%x) [Attr Unknown]", 
                   class->classId,
                   aid));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
      } 
    else 
      {
        /* first skip the instance header */
        attrBegin = (Byte_h) instance + sizeof(CORInstanceHdr_t);
        attrBegin += attrH->offset; /* move to the attr position */
        
        ret = dmObjectAttrBufCopy(attrH,
                                  attrBegin,
                                  value,
                                  size,
                                  idx < 0 ? 0:idx,
                                  1);     /* copy to value */
      }

    CL_FUNC_EXIT();  
    return ret;
}


/** 
 * Set Attribute Value.
 * 
 * Sets the attribute to the given value.
 *
 *  @param ms  Type definition for the attribute
 *  @param buf Buffer pointing to the attribute value.
 *  @param val Value that needs to be set to the attribute.
 * 
 *  @returns 
 *    ClRcT  CL_OK on success
 *  @todo    lock the class instance first before operation
 */
ClRcT
dmObjectAttrSet(DMContObjHandle_h this,
                ClCorAttrIdT    aid,
                ClInt32T        idx,
                void *         value,
                ClUint32T     size
                )
{
    Byte_h attrBegin;
    ClRcT ret = CL_OK;
    CORAttr_h attrH;
    CORClass_h class;
    void *     instance;
    
    CL_FUNC_ENTER();

    if((!this) || (!value))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    ret = dmObjectAttrHandleGet(this, &class, &instance);
    if(ret != CL_OK)
      {
        CL_FUNC_EXIT();
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unable to get the handle. Rc = 0x%x \n", ret));
        return(ret);
      }
    attrH = dmClassAttrGet(class, aid);
	

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Set (Class:%04x, Attr:%x, Size:%d, Index: %d)", 
                          this->dmObjHandle.classId,
                          aid,
                          size,
                          idx));

	
    if(!attrH || attrH->offset < 0) 
      {
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Set (Class:%04x, Attr:%x) [Attr Unknown] offset [%d]", 
                              this->dmObjHandle.classId,
                              aid, attrH->offset));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
        
      } 
    else 
      {
        /* check if lock present 
         */
        if(dmObjectLock(&(this->dmObjHandle)) == CL_OK) 
          {
            /* first skip the instance header */
            attrBegin = (Byte_h) instance + sizeof(CORInstanceHdr_t);
            attrBegin += attrH->offset; /* move to the attr position */
            
              {
                ret = dmObjectAttrBufCopy(attrH,
                                          attrBegin,
                                          value,
                                          &size,
                                          idx < 0 ? 0:idx,
                                          0); /* copy from value to attrBegin */
              } 
            dmObjectUnlock(&(this->dmObjHandle));
          }
      }

    CL_FUNC_EXIT();  
    return ret;
}

ClRcT dmObjectBufferAttrSet(CORClass_h classH,
                                void* instBuf,
                                ClCorAttrIdT attrId,
                                ClInt32T index,
                                void* value,
                                ClUint32T size)
{
    ClRcT rc = CL_OK;
    Byte_h attrBegin = NULL;
    CORAttr_h attrH = NULL;

    if (!classH || !instBuf)
    {
        clLogError("DM", "ATTRSET", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    clLogTrace("DM", "ATTRSET", "Attr Set : Class : [%d], Attribute : [%d], Size : [%u], Index : [%d]",
            classH->classId, attrId, size, index);

    attrH = dmClassAttrGet(classH, attrId);
    if(!attrH || attrH->offset < 0) 
    {
        clLogError("DM", "ATTRSET", "Unknown attribute for Set : Class : [%d], Attribute : [%d]",
                classH->classId, attrId);
        rc = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
        goto error_exit;
    } 
    else 
    {
        /* first skip the instance header */
        attrBegin = (Byte_h) instBuf + sizeof(CORInstanceHdr_t);
        attrBegin += attrH->offset; /* move to the attr position */

        rc = dmObjectAttrBufCopy(attrH,
                                  attrBegin,
                                  value,
                                  &size,
                                  index < 0 ? 0:index,
                                  0); /* copy from value to attrBegin */
        if (rc != CL_OK)
            goto error_exit;
    }

error_exit:    
    return rc;
}

/** 
 * Get the attribute type
 *
 * API to dmObjectAttrTypeGet.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
CORAttr_h
dmObjectAttrTypeGet(DMContObjHandle_h this,
                    ClCorAttrIdT    aid
                    )
{
    ClRcT ret;
    CORAttr_h attrH = 0;
    CORClass_h class;
    void *     instance;

    CL_FUNC_ENTER();

    if((!this))
    {
        CL_FUNC_EXIT();
        return(0);
    }
    ret = dmObjectAttrHandleGet(this, &class, &instance); 
    if(ret != CL_OK)
    {
        CL_FUNC_EXIT();
        clLogDebug("DM", "ATTRTYPE", "Failed to get DM class and instance.");
        return(0);
    }
    attrH = dmClassAttrGet(class, aid);

    CL_FUNC_EXIT();
    return(attrH);
}


/** 
 * Pack the object
 *
 * API to dmObjectPack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    none
 *
 */
ClRcT
dmObjectPack(CORHashKey_h   key, 
             CORHashValue_h buffer, 
             ClBufferHandleT * pBufH
             )
{
    DMObjHandle_h objHandle = 0;
    CORClass_h class = NULL;
    ClUint32T size = 0;
    int i;
    ClRcT ret = CL_OK;
	ClUint32T tmpPackL;
    
    CL_FUNC_ENTER();

    if(!buffer )
      {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
      }
    /* first set the pack buffer */
    objHandle =  (DMObjHandle_h) buffer;
	
    DM_OH_GET_TYPE(*objHandle, class);
    if (class == 0) 
      {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
      }

    if(NULL == pBufH)
    {
        CL_FUNC_EXIT();
        clLogError("OBP", "DOP", "The pointer passed for the packing buffer is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    /* first stream in class id */
    STREAM_IN_BUFFER_HANDLE_HTONL(*pBufH, &class->classId, tmpPackL, sizeof(class->classId));
    
    /* now stream directly the object
     */
    size = class->size;
    
    if((ret = dmObjectCopy(objHandle, pBufH, &size) ) != CL_OK) 
    {
        clLogError("OBP", "DOP", "Object copy error in packing!");
        return ret;
    } 

    /* loop thru all contained objects and pack all of them
     */
    for(i=0; i < class->nAttrs; i++) 
      {
        CORAttr_h attr = (CORAttr_h) corVectorGet(&class->attrs, i);
        if(attr->attrType.type == CL_COR_CONTAINMENT_ATTR)
          {
            int items = attr->attrValue.max;
            int j;
            Byte_h handle = NULL;

            /* first skip the instance header */
            /*DM_OH_GET_VAL(*objHandle, handle); */
            DM_OH_GET_VAL(objHandle, handle);
            if(handle == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the Object Buffer."));
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }
            handle += sizeof(CORInstanceHdr_t);
            handle += attr->offset; /* move to the attr position */

            /* pack this object for all the instances */
            for(j=0;j<items;j++, handle += sizeof(DMObjHandle_t))
              {
                DMObjHandle_t dmH = {0};
                memcpy(&dmH, handle, sizeof(dmH));

                /* copy to the buffer */
                ret = dmObjectPack((ClPtrT)(ClWordT)j, (CORHashValue_h) &dmH, pBufH);
                if(ret != CL_OK)
                {
                    clLogError("OBP", "DOP", "Packing of the contained object failed. rc[0x%x]", ret);
                    return ret;
                }
              }
          }
      }
    return ret;
    CL_FUNC_EXIT();
}

/** 
 * Unpack the object.
 *
 * API to dmObjectUnpack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClUint32T
dmObjectUnpack(void *  contents, 
               DMObjHandle_h* oh,
               ClInt16T instanceId
               )
{
    Byte_h  buf = contents;
    DMObjHandle_h tmp = 0;
    ClCorClassTypeT id = 0;
    CORClass_h     ch = 0;
    void * obj = 0;
    int i;
    CORInstanceHdr_h hdr = NULL;
    CORInstanceHdr_t tmpHdr = 0;
    
    CL_FUNC_ENTER();

    if((!contents) || (!oh))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    STREAM_PEEK(&id, buf, sizeof(id));
	id = ntohl(id);
    /* look up the class and then fix in the ptr */
    ch = dmClassGet(id);
    if(ch != 0) 
    {
        /* STREAM_OUT(&id, buf, sizeof(id)); */
        STREAM_OUT_NTOHL(&id, buf, sizeof(id));
        *oh = (DMObjHandle_h) clHeapAllocate(sizeof(DMObjHandle_t));
	if( *oh == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}		
        memset( *oh, '\0', sizeof( DMObjHandle_t ) );
        tmp = (DMObjHandle_h)*oh;
        if(instanceId == -1)
        {
            if(!ch->objects.head && !ch->objects.tail)
            {
                corVectorInit(&ch->objects, 
                            ch->size+sizeof(CORInstanceHdr_t),
                            ch->objBlockSz );
            }
    
            DM_OH_SET(*tmp, ch, (ch->recordId-1));
            obj = corVectorExtendedGet(&ch->objects, (ch->recordId-1));
            ch->recordId++;
            ch->objCount++;
            gObjectCount++; 

            /* all the top level objects are available and
             * they are restored as allocated for now
             */
            hdr = (CORInstanceHdr_h)obj;
            memcpy(&tmpHdr, hdr, sizeof(tmpHdr));

            COR_INSTANCE_HDR_INIT(tmpHdr);
            COR_INSTANCE_ACTIVE(tmpHdr);
            memcpy(hdr, &tmpHdr, sizeof(tmpHdr));
        }
        else
        {
            DM_OH_SET(*tmp, ch, instanceId);

            obj = corVectorGet(&ch->objects, instanceId);
            hdr = (CORInstanceHdr_h)obj;
        }
        
        STREAM_OUT(((Byte_h)obj+sizeof(CORInstanceHdr_t)),
                   buf,
                   ch->size);

        /* loop thru all contained objects and unpack all of them
         */
        for(i=0; i < ch->nAttrs; i++) 
          {
            CORAttr_h attr = (CORAttr_h) corVectorGet(&ch->attrs, i);
            if(attr->attrType.type == CL_COR_CONTAINMENT_ATTR)
              {
                int items = attr->attrValue.max;
                int j;
                Byte_h handle = (Byte_h) hdr;
                
                /* first skip the instance header */
                handle += sizeof(CORInstanceHdr_t);
                handle += attr->offset; /* move to the attr position */
                
                /* un-pack this object for all the instances */
                for(j=0;j<items;j++, handle += sizeof(DMObjHandle_t))
                  {
                    DMObjHandle_h tmpHandle=0;
                    int sz;

                    if(instanceId != -1)
                        instanceId = (( (DMObjHandle_h)handle)->instanceId);

                    sz = dmObjectUnpack(buf,&tmpHandle, instanceId);
                    if(tmpHandle)
                      {
        		        /*        *(DMObjHandle_h)handle = *tmpHandle; */
        		        ((DMObjHandle_h)handle)->classId = (tmpHandle->classId); 
        		        ((DMObjHandle_h)handle)->instanceId = (tmpHandle->instanceId);
                        clHeapFree(tmpHandle);
                        buf+=sz;
                      }
                  }
              }
          }
      }
    
    CL_FUNC_EXIT();
    return (buf - (Byte_h)contents);
}


/** 
 * Object handle pack.
 *
 * API to dmObjHandlePack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */
ClRcT
dmObjHandlePack(ClBufferHandleT *pBufH, DMObjHandle_h oh, ClUint32T *size)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if((!pBufH || !oh))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ObjPack(Class:%04x)", oh->classId));
    
    ret = dmObjectPack(0, (CORHashValue_h) oh, pBufH);

    CL_FUNC_EXIT();  
    return ret;
}

/** 
 * Object Handle Unpack.
 *
 * API to dmObjHandleUnpack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */
ClUint32T
dmObjHandleUnpack(void *          buf, 
                  DMObjHandle_h* oh,
                  ClInt16T instanceId
                  )
{
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ObjUnpack "));

    CL_FUNC_EXIT();  
    return dmObjectUnpack(buf, oh, instanceId);
}

/**
 * Object Attribute Value Handle Get function
 *
 * @param dmHandle      DM Object handle
 * @param class         Contained Class handle(OUT)
 * @param instance      Class Instance(OUT)
 * 
 * @return              CL_OK on success
 */
ClRcT
dmObjectAttrHandleGet(DMContObjHandle_h dmContHandle, 
                         CORClass_h *class, 
                         void **instance
                         )
{
    ClRcT   retCode = CL_OK;
    CORClass_h  pClassH = NULL;
    void *      pInstanceH = NULL;
    /* ClUint32T bitIdx = 0;*/

    CL_FUNC_ENTER();

    if(!dmContHandle || !class || !instance)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    DM_OH_GET_TYPE(dmContHandle->dmObjHandle, pClassH);

    if (pClassH)
    {
        pInstanceH = corVectorGet( &(pClassH->objects), dmContHandle->dmObjHandle.instanceId);

        if (pInstanceH && dmContHandle->pAttrPath && dmContHandle->pAttrPath->depth)
        {
            ClInt32T i = 0;
            DMContObjHandle_t tmpOH;
            ClUint32T size = sizeof(DMObjHandle_t);
            tmpOH.pAttrPath = NULL;
            DM_OH_SET(tmpOH.dmObjHandle, pClassH, dmContHandle->dmObjHandle.instanceId);

            for(i=0; i<(dmContHandle->pAttrPath->depth);i++)
            {
            /* get the attribute Id & index for this level */
                    /* ClUint8T attrMask, idxMask;*/
                ClCorAttrIdT attrId;
                /* ClUint32T attrIdx; */
                ClUint32T idx;

                attrId = dmContHandle->pAttrPath->node[i].attrId;
                idx    = dmContHandle->pAttrPath->node[i].index;
                        
                if ( (retCode = dmObjectAttrGet(&tmpOH,	attrId, idx,
                                                   (void *) &tmpOH, &size)) != CL_OK) 
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get object Attribute. Rc = 0x%x\n", retCode));
                    return (retCode);
                }

            }
        
            DM_OH_GET_TYPE(tmpOH.dmObjHandle, pClassH);
            if (pClassH) 
            {
                pInstanceH = corVectorGet(&(pClassH->objects), tmpOH.dmObjHandle.instanceId);
            }
            else
            {
                   retCode = CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
            }
       }

       *instance = pInstanceH;
       *class = pClassH;
    }
    else
    {
        retCode = CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    CL_FUNC_EXIT();
    return (retCode);
}



/** 
 * Object Show.
 *
 * API to Show instance/object information.
 *
 *  @param this     DMContHandle
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */

ClRcT 
dmObjectShow(DMContObjHandle_h dmContObjHdl, ClBufferHandleT msg, ClCorObjectHandleT objHandle)
{
    ClRcT               rc      = CL_OK;
    Byte_h              objBuf  = NULL;
    CORInstanceHdr_h    hdr     = NULL;
    CORClass_h          type    = NULL;
    ClCharT corStr[CL_COR_CLI_STR_LEN]  = {0};
    
    CL_FUNC_ENTER();

    if(!dmGlobal)
      {
        CL_FUNC_EXIT();  
        return CL_OK;
      }
    
    if(CL_OK != (dmObjectAttrHandleGet(dmContObjHdl, &type, (void **)&objBuf)))
      {
        corStr[0]='\0';
        sprintf(corStr, "\n Object Not found !!! \n");
        clBufferNBytesWrite (msg, (ClUint8T*)corStr, strlen(corStr));

        CL_FUNC_EXIT();  
        return CL_OK;
      }
    
    hdr = (CORInstanceHdr_h) objBuf;

    if(COR_INSTANCE_IS_ACTIVE(*hdr)) 
      {
        if(COR_INSTANCE_IS_LOCKED(*hdr)) 
          {
            corStr[0]='\0';
            sprintf(corStr, "\n Locked\n");
            clBufferNBytesWrite (msg, (ClUint8T*)corStr,
                                strlen(corStr));
          } 
        else 
          {
            objBuf+=sizeof(CORInstanceHdr_t);
            classObjBufMsgTrioT tmpTrio = {type, objBuf, objHandle, dmContObjHdl, msg};
            corStr[0]='\0';
            sprintf(corStr, "\n[MSO Class Id : 0x%04x]\n{\n",type->classId);
            clBufferNBytesWrite (msg, (ClUint8T*)corStr, strlen(corStr));

            rc = dmClassAttrWalk(type, NULL, dmObjectAttrShow, (Byte_h *)&tmpTrio);
            if(rc != CL_OK)
            {   
                clBufferClear(msg);
                corStr[0]='\0';
                sprintf(corStr, " Object Show failed. rc[0x%x] \n", rc);
                clBufferNBytesWrite (msg, (ClUint8T*)corStr, strlen(corStr));
                
            }
            else
            {

            corStr[0]='\0';
            sprintf(corStr, "}\n");
            clBufferNBytesWrite (msg, (ClUint8T*)corStr, strlen(corStr));
            }
          }
      }
    
    CL_FUNC_EXIT();  
    return CL_OK;
}

/** 
 * DM Object Attribute Index Get
 * 
 * This function shall give index, given attributeId.
 *  @param objHandle  Object Handle
 *  @param attrId      Attribute Id
 *  @param Idx		[OUT] Index for the attrbute
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *	CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT) - If the 
 *                          attribute is not present
 *
 * 
 */
#if 0
ClRcT
dmObjectAttrIndexGet(DMContObjHandle_h contObjHandle, ClCorAttrIdT attrId, ClUint16T* Idx)
{
	CORAttr_h attrH;
	CORClass_h  pClassH = NULL;
	void * dummyInstance; 

	

	if( (contObjHandle == NULL) || (Idx == NULL) )
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid parameter passed "));
		return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
		}

	        dmObjectAttrHandleGet(contObjHandle, &pClassH, &dummyInstance);


	if(pClassH)
		{
		attrH = dmClassAttrGet(pClassH, attrId);
		}
	else
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get class handle "));
		return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
		}

	if(attrH)
		{
		/* Get the index from the attribute Handle */
		*Idx = attrH->index;
		return CL_OK;
		}
	else
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get class handle "));
		return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
		}
		
}

ClRcT dmObjectAttrIndexToId(DMObjHandle_h objHandle, ClUint16T Idx, ClCorAttrIdT* attrId)
{
	ClRcT ret;
	CORClass_h  pClassH = NULL;
	

	typedef struct IdIdxStruct
		{
		ClCorAttrIdT attId;
		ClUint16T Index;
		}IdIdxStruct_t;

	IdIdxStruct_t param = {*attrId, Idx};
	IdIdxStruct_t* pParam = &param;

	if( (objHandle == NULL) || (attrId == NULL) )
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid Parameter "));
		return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
		}


	DM_OH_GET_TYPE(*objHandle, pClassH);

        	ret = dmClassAttrWalk(pClassH,
                        				 NULL,
				                         dmObjectAttrIdGet,
				                         (Byte_h*)& pParam);

        if(ret == CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_PRESENT))
        	{
        		*attrId = pParam->attId;
		return CL_OK;
        	}	
       else
	   	return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
	
}
#endif

/* 
 ##########################################################################################
    Container to store the information about the transaction started while object walk
    hits any runtime-attribute.
 ##########################################################################################
*/
   
ClInt32T _clCorAttrContCompCB (ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

void _clCorAttrContDelCB(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    _ClCorDmObjectWalkInfoPtrT pWalkInfo = (_ClCorDmObjectWalkInfoPtrT)userData;
    ClUint32T index  = 0;

    clOsalCondDelete(pWalkInfo->condVar);
    clOsalMutexDelete(pWalkInfo->condMutex);
    for (index = 0; index < pWalkInfo->noOfJobs ; index++)
    {
        CL_ASSERT(NULL != pWalkInfo->pAttrBuf);
        CL_ASSERT(NULL != (pWalkInfo->pAttrBuf + index));

        if (NULL != *(pWalkInfo->pAttrBuf + index))
        {
            
            clHeapFree(*(pWalkInfo->pAttrBuf + index));
            *(pWalkInfo->pAttrBuf + index) = NULL;
        }
        
    }

    clHeapFree(pWalkInfo->pAttrBuf);
    pWalkInfo->pAttrBuf = NULL;
    clHeapFree(pWalkInfo->pJobStatus);
    pWalkInfo->pJobStatus = NULL;
    clHeapFree((_ClCorDmObjectWalkInfoPtrT) userData);
}

ClRcT _clCorAttrWalkRTContCreate()
{
    ClRcT       rc = CL_OK;

    rc = clCntThreadSafeLlistCreate (_clCorAttrContCompCB,
                                     _clCorAttrContDelCB,
                                     _clCorAttrContDelCB,
                                     CL_CNT_UNIQUE_KEY,
                                     &attrWalkRTCont );
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while creating the linked list for storing the walk info. rc[0x%x]", rc);
        return rc;
    }

    rc = clOsalMutexCreate(&attrWalkMutex);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF",
                "Failed while creating the mutex. rc[0x%x]", rc);
        clCntDelete(attrWalkRTCont);
        return rc;
    }
     
    return rc;
}


ClRcT _clCorAttrWalkRTContDelete()
{
    ClRcT   rc  = CL_OK;


    rc = clCntDelete(attrWalkRTCont);
    if (CL_OK != rc)
        clLogError("ATW", "RTF", 
                "Failed while deleting the container used for storing the attr walk \
                info. rc[0x%x]", rc);

    rc = clOsalMutexDelete(attrWalkMutex);
    if (CL_OK != rc)
        clLogError("ATW", "RTF", "Failed while deleting the attr walk mutex. rc[0x%x]", rc);

    return rc;
}

/*
 ##########################################################################################
*/
 

ClRcT _clCorAttrCntNodeAdd ( ClTxnTransactionHandleT txnHandle, 
                _ClCorDmObjectWalkInfoPtrT pWalkInfo)
{
    ClRcT                   rc = CL_OK;
    
   clLogTrace("ATW", "RTF",
           "Adding transaction handle [%#llX] in the container. ", 
           txnHandle);

    rc = clCntNodeAdd(attrWalkRTCont, (ClCntKeyHandleT) (ClWordT) txnHandle, pWalkInfo, NULL);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF",
                "Failed while adding the walk info into the container. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}

/**
 * Function to read the attribute handles and put the default attribute values along with the
 * attribute walk informations. This function is called in case of any failure in getting
 * the values of runtime attributes.
 */
ClRcT _clCorRtAttrWalkFillValues (ClBufferHandleT bufMsgHandle, 
                                 _ClCorDmObjectWalkInfoPtrT pWalkInfo, 
                                 CORAttr_h *attrHArr, 
                                 ClUint32T noOfAttrBuf, 
                                 ClBoolT toPutDfltValues)
{
    ClRcT                   rc = CL_OK;
    ClCorObjectAttrInfoT    objAttrInfo = {0};
    ClUint32T               attrSize = 0;
    ClUint32T               index = 0;
    ClCharT                 *attrBuf = NULL;
    CORAttr_h               *attrH = NULL;

    clLogTrace("ATW", "FIL", "Filling the attribute values now. ");

    for (index = 0; index < noOfAttrBuf; index++)
    {
        attrH = (attrHArr + index);
        CL_ASSERT(NULL != (*attrH));
        
        attrSize = dmClassAttrTypeSize((*attrH)->attrType, (*attrH)->attrValue);

        objAttrInfo.attrId   = (*attrH)->attrId;
        objAttrInfo.attrFlag = (*attrH)->userFlags;
        objAttrInfo.attrType = (*attrH)->attrType.type;
        objAttrInfo.lenOrContOp    =  attrSize;

        clLogTrace("ATW", "FIL", "The attribute values filling is: AttrId [0x%x], size [%d]",
                (*attrH)->attrId, attrSize);

        if((*attrH)->attrType.type != CL_COR_ARRAY_ATTR)
        {
            objAttrInfo.attrType =  (*attrH)->attrType.type;
            objAttrInfo.lenOrContOp    =  attrSize;
        }
        else
        {
            /* additional 4 bytes for array/(assoc class) type. */
            objAttrInfo.attrType =  (*attrH)->attrType.type;
            objAttrInfo.lenOrContOp  = attrSize + (sizeof(ClUint32T));
        }

        rc = VDECL_VER(clXdrMarshallClCorObjectAttrInfoT, 4, 0, 0)( (void *)&objAttrInfo,
                bufMsgHandle, 0);
        if(rc != CL_OK)
        {
            clLogError("ATW", "FIL", "Failed while packing the object attribute info. rc[0x%x]", rc);
            CL_FUNC_EXIT();
            return (rc);
        }

        /* Write type and max instance information*/
        if ((*attrH)->attrType.type == CL_COR_ARRAY_ATTR)
        {
            /* array type  */
            ClUint32T tmp = 0;
            tmp = (*attrH)->attrType.u.arrType;
            rc = clXdrMarshallClUint32T( (void *)&tmp, bufMsgHandle, 0);
            if(rc != CL_OK)
            {
                clLogError("ATW", "FIL", "Failed while packing the array type in the buffer. rc[0x%x]", rc);
                return rc;
            }
        }

        if ( (CL_TRUE == toPutDfltValues) || ((NULL != pWalkInfo) && (CL_OK != pWalkInfo->pJobStatus[index])) )
        {
            clLogTrace("ATW", "FIL", "Coz [Flags : %d] or JobStatus [0x%x], putting the default values"
                    , toPutDfltValues, pWalkInfo != NULL ? pWalkInfo->pJobStatus[index] : CL_OK );

            attrBuf = clHeapAllocate(sizeof(ClUint32T) * attrSize);
            if (NULL == attrBuf)
            {
                clLogError("ATW", "FIL", "Failed while allocating the buffer for packing the attribute value.");
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }

            if ((*attrH)->attrType.type == CL_COR_ARRAY_ATTR)
                memset(attrBuf, 0, attrSize);
            else
                dmObjectAttrConvert(attrBuf, (*attrH)->attrValue.init,
                        (*attrH)->attrType, attrSize);

            rc = clBufferNBytesWrite(bufMsgHandle, (ClUint8T *)attrBuf, attrSize);  
            if(CL_OK != rc)
            {
                clHeapFree(attrBuf);
                clLogError("ATW", "FIL", "Failed while marshalling the values of runtime attribute. rc[0x%x]", rc); 
                return rc;
            }
            clHeapFree(attrBuf);
        }
        else
        {

            clLogTrace("ATW", "FIL", "Putting the latest value obtained from the OI.");

            if (!pWalkInfo || !(pWalkInfo->pAttrBuf + index))
            {
                clLogError("ATW", "FIL", "NULL value passed in pWalkInfo or pWalkInfo->pAttrBuf.");
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }

            rc = clBufferNBytesWrite(bufMsgHandle, (ClUint8T *)pWalkInfo->pAttrBuf[index], attrSize);
            if (CL_OK != rc)
            {
                clLogError("ATW", "FIL", "Failed while marshalling the value of runtime attributes. rc[0x%x]", rc);
                return rc;
            }
        }
    }

    return CL_OK;
}

/**
 * The callback function registered for the async transaction. This is used to 
 * get the latest value of the runtime attribute from the primary OI.
 */ 

void _clCorAttrWalkRTCallback (ClTxnTransactionHandleT txnHandle, ClRcT retCode)
{
    ClRcT                       rc = CL_OK;
    ClCntNodeHandleT            nodeH = 0;
    _ClCorDmObjectWalkInfoPtrT  pWalkInfo = NULL;
    ClCntNodeHandleT            prev = 0, next = 0;
    ClTxnAgentRespT             agentResp = {0};
    ClBufferHandleT             msgH = 0;
    ClCorTxnJobHeaderNodeT      *pTxnJobHdrNode = NULL;
    ClCorTxnAttrSetJobNodeT     *pTxnAttrJobNode = NULL;
    ClUint32T                   index = 0;



    clOsalMutexLock(attrWalkMutex);

    rc = clCntNodeFind(attrWalkRTCont, (ClCntKeyHandleT) (ClWordT)txnHandle, &nodeH);
    if ((CL_OK == rc) && (nodeH != 0))
    {
        rc = clCntNodeUserDataGet(attrWalkRTCont, nodeH, (ClCntDataHandleT *)&pWalkInfo);
        if (CL_OK != rc)
        {
            clLogError("ATW", "RTF", 
                    "Failed while getting the user info in the callback for txn-handle [%#llX]. rc[0x%x]", 
                    txnHandle, rc);
            clOsalMutexUnlock(attrWalkMutex);
            return ;
        }
    }
    else
    {
        clLogError("ATW", "RTF", 
                "Failed while getting the node for the txn-handle [%#llX] ", txnHandle);
        clOsalMutexUnlock(attrWalkMutex);
        return ;
    }
    clLogTrace("ATW", "RTF", "Got the response for the job [%#llX] pointer %p", txnHandle, (void*)pWalkInfo);

    if (NULL == pWalkInfo)
    {
        rc = CL_COR_SET_RC(CL_ERR_NULL_POINTER);
        clLogError("ATW", "RTF", 
                "The data obtained from the container for txnHandle [0x%#llX] is NULL. rc[0x%x]", txnHandle, rc);
        clOsalMutexUnlock(attrWalkMutex);
        return;
    }

    if (retCode != CL_OK)
    {
        clLogError("ATW", "RTF", 
                "Got the error from the transaction client [0x%x] for txn-handle [0x%#llX] ", 
                retCode, txnHandle);
        goto fillError; 
    }

    rc = clTxnJobInfoGet(txnHandle, prev,
                        &next, &agentResp);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while getting the response from the OI. rc[0x%x]", rc);
        retCode = rc;
        goto fillError;
    }

    rc = clBufferCreate(&msgH);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while creating the buffer. rc[0x%x]", rc);
        retCode = rc;
        goto fillError;
    }

    rc = clBufferNBytesWrite(msgH, (ClUint8T *)agentResp.agentJobDefn, 
                                agentResp.agentJobDefnSize);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while writing the response from the OI into local buffer. rc[0x%x]", rc);
        retCode = rc;
        clBufferDelete(&msgH);
        goto fillError;
    }

    rc = clCorTxnJobStreamUnpack (msgH, &pTxnJobHdrNode);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while unpacking the response from the OI for txn-handle [0x%#llX]. rc[0x%x]",
                txnHandle, rc);
        clBufferDelete(&msgH);
        retCode = rc;
        goto fillError;
    }

    clBufferDelete(&msgH);

    if ( (pTxnJobHdrNode != NULL) && 
            (pTxnJobHdrNode->head != NULL))
    {
        CL_ASSERT (pTxnJobHdrNode->head->job.numJobs == pWalkInfo->noOfJobs);

        pTxnAttrJobNode  = pTxnJobHdrNode->head->head;
        clLogTrace("ATW", "RTC", "The callback is getting called for [%d] jobs", pWalkInfo->noOfJobs);

        CL_ASSERT(NULL != pWalkInfo->pAttrBuf);

        for (index = 0; index < pWalkInfo->noOfJobs; index++)
        {
            if (pTxnAttrJobNode == NULL)
            {
                clLogError("ATF", "RTF", "The attribute job node is NULL.");
                rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                CL_ASSERT(0);                
                goto fillError;
            }
            
            memcpy(*(pWalkInfo->pAttrBuf + index), pTxnAttrJobNode->job.pValue, pTxnAttrJobNode->job.size) ;
            pWalkInfo->pJobStatus [index] =  pTxnAttrJobNode->job.jobStatus;

            clLogTrace("ATW", "RTC", "Inside the runtime get callback. The "
                    "job status [0x%x] for attrId [0x%x]", 
                    pTxnAttrJobNode->job.jobStatus, 
                    pTxnAttrJobNode->job.attrId);

            pTxnAttrJobNode  = pTxnAttrJobNode->next;
        }
    }
    else
    {
        clLogError("ATW", "RTF", 
                "The NULL pointer obtained after unpacking the txn-job for Txn-handle [%#llX]", txnHandle);
        retCode = CL_COR_SET_RC(CL_ERR_NULL_POINTER);
    }

    clCorTxnJobHeaderNodeDelete (pTxnJobHdrNode);

    clLogTrace("ATW", "RTF", "Done with the unpack and stuff");

fillError:
    pWalkInfo->rc = retCode;
    
    rc = clOsalMutexLock(pWalkInfo->condMutex);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while acquiring the lock in the callback. rc[0x%x]", rc);
        clOsalMutexUnlock(attrWalkMutex);
        return ;
    }

    rc = clOsalCondSignal(pWalkInfo->condVar);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF",
                "Failed while signalling the conditional varibale. rc[0x%x]", rc);
    }

    clOsalMutexUnlock(pWalkInfo->condMutex);

    clOsalMutexUnlock(attrWalkMutex);

    clLogDebug("ATW", "RTF", 
            "Signaled the waiting process for the transaction handle [%#llX] [%p]", 
            txnHandle, (void *)&pWalkInfo->condVar);




    return ;
} 


/**
 *  The function which is doing the jobs of checking if the primary OI exist. If yes, then it is
 *  constructing the jobs and sending it to the primary OI using transaction read functionality.
 *  Using the conditional variable to block the request as the read function is a asynchronous 
 *  function and the walk is done synchronously. It uses a container to store the conditional 
 *  variables and mutex for signalling it from the read completion callback function.
 */ 
ClRcT 
_clCorRuntimeAttrFill (ClCorObjectHandleT objH, ClCorAttrPathPtrT pAttrPath, CORAttr_h *attrHArr, 
            ClUint32T noOfAttrBuf, ClBufferHandleT bufMsgHandle)
{
    ClRcT                       rc = CL_OK, retCode = CL_OK;
    ClTimerTimeOutT             timeVal = {
                                           .tsSec = 0, 
                                           .tsMilliSec = CL_COR_ATTR_WALK_RUNTIME_GET_TIMEOUT
                                          };
    ClCorUserSetInfoT           usrGetData  = {0};
    _ClCorDmObjectWalkInfoPtrT  pWalkInfo   = NULL;
    ClCorAddrT                  compAddr    = {0};
    ClIocAddressT               compAddress = {{0}};
    ClCorTxnJobHeaderNodeT      *pTxnJobHdrNode = NULL;
    ClCorTxnAttrSetJobNodeT     *pTxnAttrJobNode = NULL;
    ClBufferHandleT             msgH        = 0;
    ClPtrT                      pData       = 0;
    ClUint32T                   length      = 0;
    ClTxnTransactionHandleT     txnHandle   = CL_HANDLE_INVALID_VALUE;
    ClTxnJobHandleT             jobHandle   = 0;
    ClCorAttrPathT              attrPath ;
    ClCorMOIdT                  moId;
    CORAttr_h                   *attrH      = NULL;
    ClUint32T                   index       = 0;
    ClInt32T                    attrSize    = 0;
    ClCorClassTypeT             moClassId   = 0;

    clCorMoIdInitialize(&moId);

    clLogDebug("ATW", "RTF", 
            "Inside the function to obtain the latest value of runtime attribute for OI");

    /*First check for the primary OI existence then only go for read from it*/
//    rc = corOH2MOIdGet((COROH_h)objH.tree, &moId);
    rc = clCorObjectHandleToMoIdGet(objH, &moId, NULL);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed to get the MOID from the object handle. rc[0x%x]", rc);
        return rc;
    }
    
    rc = _clCorPrimaryOIGet(&moId, &compAddr);
    if (CL_OK != rc)
    {
        clLogNotice("ATW", "RTF", "There is no primary OI registered, "
                "so putting only default values. rc[0x%x]", rc);
        return rc;
    }

    pWalkInfo = clHeapAllocate(sizeof (_ClCorDmObjectWalkInfoT)); 
    if (NULL == pWalkInfo)
    {
        clLogError("ATW", "RTF", 
                "Failed while allocating the memory for the object walk info");
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }   

    memset(pWalkInfo, 0, sizeof(_ClCorDmObjectWalkInfoT));

    rc = clOsalCondCreate(&pWalkInfo->condVar);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while initializing the conditional variable of the object walk. rc[0x%x]", rc);
        clHeapFree(pWalkInfo);
        return rc;
    }

    rc = clOsalMutexCreate(&pWalkInfo->condMutex);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF",
                "Failed while creating the mutex for object walk info. rc[0x%x]", rc);
        clOsalCondDelete(pWalkInfo->condVar);
        clHeapFree(pWalkInfo);
        return rc;
    }


    pWalkInfo->pAttrBuf = clHeapAllocate(sizeof(ClPtrT) * noOfAttrBuf);
    if (NULL == pWalkInfo->pAttrBuf)
    {
        clOsalCondDelete(pWalkInfo->condVar);
        clOsalMutexDelete(pWalkInfo->condMutex);
        clHeapFree(pWalkInfo);
        clLogError("ATW", "RTF", "Failed while allocating the memory for attribute buffers");
        return CL_COR_SET_RC(CL_ERR_NO_MEMORY);
    }

    memset(pWalkInfo->pAttrBuf, 0, sizeof(ClPtrT) * noOfAttrBuf);

    pWalkInfo->pJobStatus = clHeapAllocate(sizeof(ClPtrT) * noOfAttrBuf);
    if (NULL == pWalkInfo->pJobStatus)
    {
        clOsalCondDelete(pWalkInfo->condVar);
        clOsalMutexDelete(pWalkInfo->condMutex);
        clHeapFree(pWalkInfo->pAttrBuf);
        clHeapFree(pWalkInfo);
        clLogError("ATW", "RTF", "Failed while allocating the memory for attribute buffers");
        return CL_COR_SET_RC(CL_ERR_NO_MEMORY);
    }

    rc = clCorTxnJobHeaderNodeCreate(&pTxnJobHdrNode);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while creating the cor-txn header node. rc[0x%x]", rc);
        goto handleError;
    }

    memcpy(&(pTxnJobHdrNode->jobHdr.moId), &moId, sizeof(ClCorMOIdT));

    pTxnJobHdrNode->jobHdr.srcArch = clBitBlByteEndianGet();


    /* Fill up the OM Class Id. 
     * This is required for prov to create OM object dynamically.
     */
    rc = clCorMoIdToClassGet(&moId, CL_COR_MO_CLASS_GET, &moClassId);
    if (rc != CL_OK)
    {
        clLogError("ATW", "RTF", "Failed to get mo class Id from moId. rc [0x%x]", rc);
        clHeapFree(pTxnJobHdrNode);
        goto handleError;
    }

    rc = _clCorOmClassFromInfoModelGet(moClassId, moId.svcId, &pTxnJobHdrNode->jobHdr.omClassId);
    if (rc != CL_OK)
    {
        clLogError("ATW", "RTF", "Failed to get om class Id for mo class Id [%d] service Id [%d]. rc [0x%x]", 
                moClassId, moId.svcId, rc);
        clHeapFree(pTxnJobHdrNode);
        goto handleError;
    }

    rc = clCorTxnObjJobTblCreate(&pTxnJobHdrNode->objJobTblHdl);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while creating the cor-txn obj job table. rc[0x%x]", rc);
        clHeapFree(pTxnJobHdrNode);
        goto handleError;
    }

    if (pAttrPath != NULL)
    {
        usrGetData.pAttrPath  = pAttrPath;
    }
    else
    {
        clCorAttrPathInitialize (&attrPath);
        usrGetData.pAttrPath = &attrPath;
    }

    clLogInfo("ATW", "RTF", "Going to put all the attribute information for doing the bulk get.");

    for ( attrH = attrHArr, index = 0 ;
            ( index < noOfAttrBuf) ; ++index, attrH = (attrHArr + index) )
    {
        CL_ASSERT(NULL != attrH);
        attrSize = dmClassAttrTypeSize((*attrH)->attrType, (*attrH)->attrValue);
        if (attrSize < 0)
        {
            clLogError("ATW", "RTF", "Invalid size [%d] obtained for attribute [0x%x]",
                    attrSize, (*attrH)->attrId);
            rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
            goto handleError;
        }

        *(pWalkInfo->pAttrBuf + index) = clHeapAllocate (attrSize);
        if (NULL == pWalkInfo->pAttrBuf[index])
        {
            clLogError("ATW", "RTF", "Failed while allocating the memory for attribute buffer.");
            rc = CL_COR_SET_RC(CL_ERR_NO_MEMORY);
            goto handleError;
        }

        usrGetData.attrId     = (*attrH)->attrId;
        if ((*attrH)->attrType.type == CL_COR_ARRAY_ATTR)
            usrGetData.index      = 0;
        else
            usrGetData.index      = CL_COR_INVALID_ATTR_IDX;

        usrGetData.size       = attrSize;
        usrGetData.value      = *(pWalkInfo->pAttrBuf + index);
        usrGetData.pJobStatus = (pWalkInfo->pJobStatus + index);

        clLogInfo("ATW", "RTF", "Inserting the job: AttrId [0x%x], size [%d]", 
                (*attrH)->attrId, attrSize);

        rc = clCorTxnObjJobNodeInsert(pTxnJobHdrNode, CL_COR_OP_GET, &usrGetData);
        if (CL_OK != rc)
        {
            clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
            clLogError("ATW", "RTF",
                    "Failed while adding the jobs to the header node. rc[0x%x]", rc);
            goto handleError;
        }

        /* Setting the type. */
        pTxnAttrJobNode = pTxnJobHdrNode->head->tail;
        pTxnAttrJobNode->job.attrType = (*attrH)->attrType.type;
        pTxnAttrJobNode->job.arrDataType = (*attrH)->attrType.u.arrType;
    }

    pWalkInfo->noOfJobs = index;

    rc = clBufferCreate(&msgH);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", "Failed while creating the buffer. rc[0x%x]", rc);
        clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        goto handleError;
    }

    rc = clCorTxnJobStreamPack(pTxnJobHdrNode, msgH);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while packing the jobs in the buffer. rc[0x%x]", rc);
        clBufferDelete(&msgH);
        clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        goto handleError; 
    }

    /* Resetting the data pointer to avoid its deletion in the header node delete */
    pTxnAttrJobNode = pTxnJobHdrNode->head->head;
    do
    {
        pTxnAttrJobNode->job.pValue = NULL;
        pTxnAttrJobNode = pTxnAttrJobNode->next;
    }while (pTxnAttrJobNode != NULL);

    clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);

    rc = clBufferFlatten(msgH, (ClUint8T **)&pData);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while flattening the buffer. rc[0x%x]", rc);
        clBufferDelete(&msgH);
        goto handleError; 
    }

    rc = clBufferLengthGet(msgH, &length);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while getting the length of the buffer. rc[0x%x]", rc);
        clBufferDelete(&msgH);
        clHeapFree(pData);
        goto handleError; 
    }    

    clBufferDelete(&msgH);

    rc = clCorTxnTxnSessionOpen(&txnHandle);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while opening the session for getting the value from OI. rc[0x%x]", rc);
        clHeapFree(pData);
        goto handleError;
    }
     

    rc = clTxnJobAdd (txnHandle, 
            (ClTxnJobDefnHandleT)pData, length,
            CL_COR_TXN_SERVICE_ID_READ,
            &jobHandle);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while adding the job to the OI job list. rc[0x%x]", rc);
        clHeapFree(pData);
        goto handleError;
    }

    clHeapFree(pData);

    memcpy (&compAddress, &compAddr, sizeof (ClCorAddrT));

    rc = clTxnComponentSet (txnHandle, jobHandle, 
             compAddress, CL_TXN_COMPONENT_CFG_1_PC_CAPABLE);
    if (CL_OK != rc)
    {
         clLogError("ATW", "RTF", 
                 "Failed while adding the component address to the job. rc[0x%x]", rc);
         goto handleError;
    }

    clLogTrace("ATW", "RTF", "Sending request for the job [%#llX] pointer %p", txnHandle, (void*)pWalkInfo);

    /* Adding the container node to access the walk Info in the transaction callback. */
    rc = _clCorAttrCntNodeAdd (txnHandle, pWalkInfo);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                "Failed while adding the node to the attr Walk container. rc[0x%x]", rc);
        goto handleError;
    }

    clOsalMutexLock(pWalkInfo->condMutex);
    
    rc = clTxnReadAgentJobsAsync(txnHandle, _clCorAttrWalkRTCallback);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF",
                 "Failed while starting the read operation on the runtime. rc[0x%x]", rc);
        clOsalMutexUnlock(pWalkInfo->condMutex);
        goto deleteNode;
    }

    rc = clOsalCondWait (pWalkInfo->condVar, pWalkInfo->condMutex, timeVal);
    if (CL_OK != rc)
    {
        clLogError("ATW", "RTF", 
                 "Failed while waiting on the conditional variable [%p]. rc[0x%x]", (void *)&pWalkInfo->condVar, rc);
        clOsalMutexUnlock(pWalkInfo->condMutex);
        goto deleteNode; 
    }
    else
        clLogTrace("ATW", "RTF", "Conditional varibale woken up successfully [%p]", (void *)&pWalkInfo->condVar);

    clOsalMutexUnlock(pWalkInfo->condMutex);

deleteNode:

    retCode = rc;
    /** 
     *  To capture any error happening at the OI or in the transaction. This should be 
     *  done only in case of response coming back within the expected timeout. 
     */
    if (CL_OK != pWalkInfo->rc)
    {
        clLogTrace("ATW", "RTF", "The error returned from OI is [0x%x]", 
                pWalkInfo->rc);
        retCode = pWalkInfo->rc;
    }

    if (CL_OK == retCode)
    {
        clLogTrace("ATW", "RTF", "Successfully got the response. So filling the "
                "latest values for attributes now");
        rc = _clCorRtAttrWalkFillValues(bufMsgHandle, pWalkInfo, attrHArr, noOfAttrBuf, CL_FALSE);
        if (CL_OK != rc)
        {
            clLogError("ATW", "RTF", "Failed while packing the latest values got from the OI. rc[0x%x]", rc);
        }
    }
    else
    {
        clLogTrace("ATW", "RTF", "The response from the txn-client is [0x%x]. "
                "So filling the default values for attributes now", rc);
        rc = _clCorRtAttrWalkFillValues(bufMsgHandle, pWalkInfo, attrHArr, noOfAttrBuf, CL_TRUE);
        if (CL_OK != rc)
        {
            clLogError("ATW", "RTF", "Failed while packing the default values for a runtime attribute list. rc[0x%x]", rc);
        }
        
    }

    /* Protecting the node user data if the txn-callback has acquired the node user data but
       got switched and the conditional timeout has occured. Should not delete the data 
       in this case. 
     */
    clOsalMutexLock(attrWalkMutex);

    clCntAllNodesForKeyDelete(attrWalkRTCont, 
            (ClCntKeyHandleT)(ClWordT) txnHandle);

    clOsalMutexUnlock(attrWalkMutex);

    clLogTrace("ATW", "RTF", 
            "Returning from the runtime packing function with return code [0x%x]", retCode);

    return rc;

handleError:
    clOsalCondDelete(pWalkInfo->condVar);
    clOsalMutexDelete(pWalkInfo->condMutex);
    for (index = 0 ; index < noOfAttrBuf; index++)
    {
        if ((NULL != pWalkInfo->pAttrBuf) && (NULL != (pWalkInfo->pAttrBuf[index])))
        {
            
                clHeapFree(pWalkInfo->pAttrBuf[index]);
                pWalkInfo->pAttrBuf[index] = 0;
        }
        
    }
    clHeapFree(pWalkInfo->pAttrBuf);
    pWalkInfo->pAttrBuf = 0;
    clHeapFree(pWalkInfo->pJobStatus);
    pWalkInfo->pJobStatus = 0;
    clHeapFree(pWalkInfo);

    return rc;
}

/*
 *  This function actually steams-in the attributes to buffer.
 *  The user filter is applied in this function itself. 
 */

/* This function will pack the simple attributes */

ClRcT clCorDmExtObjAttrPackFun(CORAttr_h attrH, Byte_h* cookie)
{
    ClRcT rc = CL_OK; 
    ClCorObjectAttrInfoT   objAttrInfo;
    CORAttr_h              tmpAttrH;
    ClCorDmObjPackAttrWalkerT *pBuf = (ClCorDmObjPackAttrWalkerT *)cookie;
    ClCorObjAttrWalkFilterT      *pFilter = pBuf->pFilter;
    ClCorAttrPathPtrT          pCAttrPath = pBuf->pCurrAttrPath;
    ClCorAttrPathPtrT          pFAttrPath = pFilter ? pFilter->pAttrPath:NULL;
     
    CL_FUNC_ENTER();

    if(attrH->attrType.type != CL_COR_CONTAINMENT_ATTR)
    {
        if(!pFilter ||   /* pack everything. (NO filter) */
            ((pCAttrPath->depth == 0) &&  pFilter->baseAttrWalk == CL_TRUE) || /* pack base object attributes*/
              (pCAttrPath->depth != 0 && 
                ((pFAttrPath == NULL)||  /* pack all contained objects. */
                 (clCorAttrPathCompare(pFAttrPath, pCAttrPath) != -1))))     /* exact or wildcard match. */
        {
            /*@todo: Need to write this  filter neatly. Not everything should come in
                     a if loop (it needs proper comments)
                     Helpless!!! :( could not avoid this if statement. */
            if(!pFilter   ||
                 (pFilter->attrId == CL_COR_INVALID_ATTR_ID) ||    /* no comparison rqrd*/
                    pBuf->attrCmpResult == CL_TRUE ||              /* comparison already PASS */
                      (pBuf->attrCmpResult == -1  &&               /* needs comparison */
                        ((tmpAttrH = dmClassAttrGet(pBuf->clsHdl,   /* check if attrId exists*/
                           pFilter->attrId))? 1: 0) &&
                             ((clCorDmAttrValCompare(tmpAttrH,
                                pBuf,       /* compare the value */
                                 pFilter->attrId, pFilter->index,
                                   pFilter->value, pFilter->size,
                                     pFilter->cmpFlag) == CL_OK) &&
                                       pBuf->attrCmpResult == CL_TRUE)))
                          
            {
                if(!pFilter   ||
                    (pFilter->attrId == CL_COR_INVALID_ATTR_ID) ||     /* no comparison rqrd*/
                       (pBuf->attrCmpResult == CL_TRUE &&              /* comparison already PASS */
                        (pFilter->attrWalkOption != CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR || /*check  Walk Option*/
                                  pFilter->attrId == attrH->attrId)))
                {
                    void *attrBuf = NULL;
                    ClUint8T  *pAttrBegin = NULL;
                    ClUint32T  attrSize = 0;

                    /* first skip the instance header */
                    pAttrBegin  = (ClUint8T *)pBuf->objInstH + sizeof(CORInstanceHdr_t);
                    pAttrBegin += attrH->offset; /* move to the attr position */
                
                    /**
                     * For the cached runtime attributes, packing the attrH in the 
                     * buffer handle. This will be used the hash walk on the 
                     * attribute hash table is done.
                     */
                    if ((attrH->userFlags & CL_COR_ATTR_RUNTIME) &&
                        (! (attrH->userFlags & CL_COR_ATTR_CACHED)))
                    {
                        clLogDebug("ATW", "PAK", "Non-cached runtime attribute [0x%x] found, packing the attribute handle.",
                                attrH->attrId);

                        rc = clBufferNBytesWrite(pBuf->rtAttrBufMsgHdl, (ClUint8T *)&attrH, sizeof(CORAttr_h));
                        if (CL_OK != rc)
                            clLogNotice("ATW", "PAK", "Failed while writing the "
                                    "attribute handle pointer for attribute Id "
                                    "[0x%x] to the buffer handle. rc[0x%x]", 
                                    attrH->attrId, rc);
                       return rc;
                    }

                    attrSize = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

                    /** Stream - in **/
                    objAttrInfo.attrId   = attrH->attrId;
                    objAttrInfo.attrFlag = attrH->userFlags;
                    if(attrH->attrType.type != CL_COR_ARRAY_ATTR &&
                        attrH->attrType.type != CL_COR_ASSOCIATION_ATTR)
                    {
                        objAttrInfo.attrType =  attrH->attrType.type;
                        objAttrInfo.lenOrContOp    =  attrSize;
                    }
                    else
                    {
                        /* additional 4 bytes for array/(assoc class) type.
                        these 4 bytes will be used to store the array type or 
                        the containment class id */
                        objAttrInfo.attrType =  attrH->attrType.type;
                        objAttrInfo.lenOrContOp  = attrSize + (sizeof(ClUint32T));
                    }
         
                    rc = VDECL_VER(clXdrMarshallClCorObjectAttrInfoT, 4, 0, 0)( (void *)&objAttrInfo,
                            pBuf->packBufMsgHandle, 0);
                    if(rc != CL_OK)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                  ( "\n Buffer Message Write Failed. \n"));
                        CL_FUNC_EXIT();
                        return (rc);
                    }
               
                    /* Write type and max instance information*/
                    if (attrH->attrType.type == CL_COR_ARRAY_ATTR ||
                        attrH->attrType.type == CL_COR_ASSOCIATION_ATTR)
                    {
                        /* array type  */
                        ClUint32T tmp = 0;
                        if (attrH->attrType.type == CL_COR_ARRAY_ATTR)
                        {
                            tmp = attrH->attrType.u.arrType;
                        }
                        else
                        {
                            tmp = attrH->attrType.u.remClassId;
                        }
                      
                        rc = clXdrMarshallClUint32T( (void *)&tmp, 
                                            pBuf->packBufMsgHandle, 0);
                        if(rc != CL_OK)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                    ( "\n Buffer Message Write Failed. \n"));
                        }
                    }

                    /* Allocate buffer for attribute */
                    attrBuf = clHeapAllocate(attrSize);
                    if (!attrBuf)
                    {
                        clLogError("OBW", "PAK", "Failed while allocating memory for the attribute buffer.");
                        CL_FUNC_EXIT();
                        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                    }

                    /**
                     * For a runtime and non-cached attribute, looking for 
                     * the primary OI. If the primary OI exist then getting
                     * the latest value. In case of any failure during this 
                     * get operation, putting the default value.
                     */ 
#if 0
                    if ((attrH->userFlags & CL_COR_ATTR_RUNTIME) &&
                        (! (attrH->userFlags & CL_COR_ATTR_CACHED)))
                    {
                        rc = _clCorRuntimeAttrFill(pBuf->objHandle, pCAttrPath, 
                                 attrH, attrBuf, attrSize);
                        if (CL_OK != rc)
                        {
                            clLogNotice("ATW", "PAK", "Failed while getting the latest value \
                                    of runtime attribute [0x%x], packing the default value.", 
                                    attrH->attrId);

                            if (attrH->attrType.type == CL_COR_ARRAY_ATTR ||
                                    attrH->attrType.type == CL_COR_ASSOCIATION_ATTR)
                                memset(attrBuf, 0, attrSize);
                            else
                                dmObjectAttrConvert(attrBuf, attrH->attrValue.init,
                                    attrH->attrType, attrSize);
                        }
                    }
                    else
#endif
                    {
                        /* now stream in the actual value */
                        if(( rc = dmObjectAttrBufCopy(attrH,
                                    (Byte_h)pAttrBegin,
                                     attrBuf,
                                    &attrSize,
                                     0,
                                    1)) != CL_OK)     /* copy to value */
                        {
                            clHeapFree(attrBuf);
                            CL_FUNC_EXIT();
                            return (rc);
                        }
                    }
                    
                    rc = clBufferNBytesWrite(pBuf->packBufMsgHandle,  
                                              (ClUint8T *)attrBuf, attrSize);  
                    if(rc != CL_OK)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                  ( "\n Buffer Message Write Failed. \n"));
                    }
                    clHeapFree(attrBuf);
                }
            }
        }
    }
    else if (!pFilter || pFilter->contAttrWalk == CL_TRUE)
    {
         /* stream-in the pointer of containment attribute */
         /* Check on containment class Id */
         /* These containment attributes will be packed recursively after this function */
         if(!pFilter ||
             ((pFAttrPath == NULL)) ||
               (pFAttrPath->node[pCAttrPath->depth].attrId == CL_COR_ATTR_WILD_CARD) ||
                  (pFAttrPath->node[pCAttrPath->depth].attrId == attrH->attrId))
         {
              rc = clBufferNBytesWrite(pBuf->contAttrBufMsgHandle,
                                       (ClUint8T *)&attrH, sizeof(CORAttr_h));
              if(rc != CL_OK)
              {
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                   ( "\n Buffer Message Write Failed. \n"));
                   CL_FUNC_EXIT();
                   return (rc);
              }
         }
    }

    CL_FUNC_EXIT();
    return (rc);
}

static ClInt32T _corAttrDefCompare(const void* pAttrDef1, const void* pAttrDef2)
{
    return ((*(CORAttr_h *) pAttrDef1)->attrId - (*(CORAttr_h *) pAttrDef2)->attrId);
}

/*
 *  This function packs object (including contained ones) with limited
 *  meta-data information. (like attrType, attrId, value)
 *  The containment information is Seralized in Depth First Fashion.
 *  
 *  NOTE: Simple attributes are packed before we pack attributes of contained object.
 */

ClRcT
clCorDmExtObjPackWithFilter(DMObjHandle_h objHandle, ClUint8T srcArch, 
                       ClCorAttrPathPtrT pStartAttrPath,
                          ClCorObjAttrWalkFilterT *pWalkFilter,
                                ClBufferHandleT bufMsgHandle,
                                ClCorObjectHandleT objHdl)
{
    ClRcT rc = CL_OK;
    ClUint8T  *pInstanceH = NULL;
    CORClass_h class = NULL;
    ClCorObjectAttrInfoT       objAttrInfo;
    ClCorDmObjPackAttrWalkerT  *cookie = NULL;
    ClUint32T msgLen = 0;
    CORAttr_h *attr;
    CORAttr_h *tmpAttr;


    CL_FUNC_ENTER();

    if (!objHandle)
    {
        clLogError("ATW", "PAK", "The DM object handle passed is NULL. ");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* cookie for attribute walk */
    cookie = clHeapAllocate(sizeof(ClCorDmObjPackAttrWalkerT));
    if(!cookie)
    {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    DM_OH_GET_TYPE(*objHandle, class);
    if (class == NULL) 
    {
       CL_FUNC_EXIT();
       clHeapFree(cookie);
       return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
    }
   
    pInstanceH = (ClUint8T *)corVectorGet(&(class->objects), objHandle->instanceId);
    if(pInstanceH == NULL)
    {
       clHeapFree(cookie);
       CL_FUNC_EXIT();
       return CL_COR_SET_RC(CL_COR_ERR_OBJ_NOT_PRESENT);
    }
     
    /** Fill in the cookie **/ 
    cookie->objInstH          =  pInstanceH;
    cookie->packBufMsgHandle  =  bufMsgHandle;
    cookie->pFilter           =  pWalkFilter;
    cookie->pCurrAttrPath     =  pStartAttrPath; /* We are allocating a attrPath and sending it to this function */
    cookie->attrCmpResult     =  -1;
    cookie->clsHdl            =  class;
    cookie->srcArch           =  srcArch; /* sending the source architecture */
    cookie->objHandle         =  objHdl; /* The object handle which will be used to get on runtime attributes. */

    if((rc = clBufferCreate(&cookie->contAttrBufMsgHandle)) != CL_OK)
    {
       clHeapFree(cookie);
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Create Failed. \n"));
       CL_FUNC_EXIT();
       return (rc);
    }

    if((rc = clBufferCreate(&cookie->rtAttrBufMsgHdl)) != CL_OK)
    {
        clBufferDelete(&cookie->contAttrBufMsgHandle);
        clHeapFree(cookie);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Create Failed. \n"));
        CL_FUNC_EXIT();
        return (rc);
    }

    /** First pack the simple attributes (including the inherited ones also) **/
    rc = dmClassAttrWalk(class, NULL, clCorDmExtObjAttrPackFun, (Byte_h*)cookie); 
    if(rc != CL_OK)
    {
        clLogError("OAW", "PAK", "Failed while doing the walk on the object attributes. rc[0x%x]", rc);
        goto handleError;
    }

    rc = clBufferLengthGet(cookie->rtAttrBufMsgHdl, &msgLen);
    if (rc != CL_OK)
    {
        clLogError("OAW", "PAK", "Failed while getting the length of buffer "
                "containing runtime attribute handles. rc[0x%x]", rc );
        goto handleError;
    }

    if (msgLen != 0)
    {
        ClUint32T noOfAttr = 0;

        rc = clBufferFlatten(cookie->rtAttrBufMsgHdl, (ClUint8T **)&attr);
        if (CL_OK != rc)
        {
            clLogError("OAW", "PAK", "Failed while getting the attribute handle "
                    "buffers from the runtime buffer handle. rc[0x%x]", rc);
            goto handleError;
        }

        noOfAttr = msgLen/sizeof(CORAttr_h);

        /* Sort the array of pointers in the ascending of attr Id.
         * Because while adding GET job to ClCorTxnJobHeaderT, the jobs 
         * are sorted/inserted based on the attr Id. So this order should 
         * match while copying back the values retrieved.
         */
        qsort(attr, noOfAttr, sizeof(CORAttr_h), _corAttrDefCompare);

        rc = _clCorRuntimeAttrFill(objHdl, pStartAttrPath, attr, noOfAttr, bufMsgHandle);
        if (CL_OK != rc)
        { 
            clLogInfo("ATW", "RTF", "Failed [0x%x] to pack the latest values for the "
                    "runtime attributes, so putting the default values now", rc);
            rc =_clCorRtAttrWalkFillValues(bufMsgHandle, NULL, attr, msgLen/sizeof(CORAttr_h), CL_TRUE);
            if (CL_OK != rc)
            {
                clLogError("OAW", "PAK", "Failed while packing the default values "
                        "for a runtime attribute list. rc[0x%x]", rc);
                clHeapFree(attr);
                attr = NULL;
                goto handleError;
            }
        }
        
        clHeapFree(attr);
        attr = NULL;
    }


    if (!pWalkFilter || pWalkFilter->contAttrWalk == CL_TRUE)
    {
        /** Pack the contained Attr (if any)**/
        if(((rc = clBufferLengthGet(cookie->contAttrBufMsgHandle,
                                        &msgLen)) == CL_OK) &&  msgLen)
        {
            ClUint32T i = 0;
            ClUint16T tmpDepth = pStartAttrPath->depth;
            /* Flatten the buffer */
            rc = clBufferFlatten(cookie->contAttrBufMsgHandle,
                                                   (ClUint8T **)&attr);
            if(rc != CL_OK)
            {
                /* we can still continue packing other attributes...but not this one.*/
                msgLen = 0;
                clLogError("OAW", "PAK", "Buffer Message Flatten Failed.");
            }

            tmpAttr = attr;
         
            for(i=0; i < msgLen/sizeof(CORAttr_h); i++, attr++) 
            {
                if((*attr)->attrType.type == CL_COR_CONTAINMENT_ATTR)
                {
                   ClUint32T items = (*attr)->attrValue.max;
                   ClUint32T j = 0;
                   ClUint32T temp = 0;
                   ClUint8T *handle = pInstanceH;

                   /* first skip the instance header */
                   handle += sizeof(CORInstanceHdr_t);
                   handle += (*attr)->offset; /* move to the attr position */

                  /* If it is not a wild card then pack only one object. so set the items to j+1 (j to j+1 - one object) */
                  if(pWalkFilter &&
                       (pWalkFilter->pAttrPath != NULL) &&
                         (pWalkFilter->pAttrPath->node[tmpDepth].index
                                                      != CL_COR_INDEX_WILD_CARD))
                   {
                     j = pWalkFilter->pAttrPath->node[tmpDepth].index;
                     temp = j;
                     items = j + 1;
                     handle += (j * sizeof(DMObjHandle_t)); 
                   }
                  /* pack this object for the given instances */
                   for(;j<items;j++, handle += sizeof(DMObjHandle_t))
                   {
                        /** Stream the Attribute **/
                        objAttrInfo.attrId   = (*attr)->attrId;
                        objAttrInfo.attrType = (*attr)->attrType.type;
                        if( i == 0 &&  j == temp)
                         {
                            objAttrInfo.lenOrContOp     = CL_COR_OBJSTREAM_CONT_APPEND; 
                            clCorAttrPathAppend(pStartAttrPath, (*attr)->attrId, j);
                         }
                         else
                         {
			                clCorAttrPathSet(pStartAttrPath, pStartAttrPath->depth, (*attr)->attrId, j);
                            objAttrInfo.lenOrContOp     = CL_COR_OBJSTREAM_CONT_SET;
                         }

                         rc = VDECL_VER(clXdrMarshallClCorObjectAttrInfoT, 4, 0, 0)( (void *)&objAttrInfo,
										bufMsgHandle, 0);
                         if(rc != CL_OK)
                         {
                            clHeapFree(tmpAttr);
                            clLogError("OAW", "PAK", " Failed while packing the object attribute info. rc[0x%x]", rc);
                            CL_FUNC_EXIT();
                            goto handleError;
                         }

                        /* Now stream in the index for the containment attr */
                         rc = clXdrMarshallClUint32T( (ClUint8T *)&j,
										bufMsgHandle, 0);
                         if(rc != CL_OK)
                         {
                            clHeapFree(tmpAttr);
                            clLogError("OAW", "PAK","Failed while packing the containment index. rc[0x%x] ", rc);
                            CL_FUNC_EXIT();
                            goto handleError;
                         }
                       
                        rc = clCorDmExtObjPackWithFilter((DMObjHandle_h)handle,
                                                         srcArch,
                                                           pStartAttrPath, 
                                                             pWalkFilter,
                                                                bufMsgHandle,
                                                                    objHdl);
                        if(rc != CL_OK)
                        {
                            clHeapFree(tmpAttr);
                            clLogError("OAW", "PAK", "Failed while going for the attribute walk for a contained MO. rc[0x%x]", rc);
                            CL_FUNC_EXIT();
                            goto handleError;
                        }
                    }
                } /* if */
            }  /* for */
            /** cd .. directive  **/
            objAttrInfo.attrId   =  0; /*attrId not required, since it is cd .. op*/
            objAttrInfo.attrType = CL_COR_CONTAINMENT_ATTR;
            objAttrInfo.lenOrContOp     = CL_COR_OBJSTREAM_CONT_TRUNCATE; 

            clCorAttrPathTruncate(pStartAttrPath, pStartAttrPath->depth-1);
            /* Stream it in */
            /* if((rc = clBufferNBytesWrite(bufMsgHandle,
                  (ClUint8T *)&objAttrInfo, sizeof(objAttrInfo))) != CL_OK) */
            if((rc = VDECL_VER(clXdrMarshallClCorObjectAttrInfoT, 4, 0, 0)( (void *)&objAttrInfo,
                                                     bufMsgHandle, 0)) != CL_OK)
            {
                clHeapFree(tmpAttr);
                clLogError("OAW", "PAK", "Failed while packing the attribute info with truncate flag. rc[0x%x]", rc);
                CL_FUNC_EXIT();
                goto handleError;
            }
         /* Free the flattened buffer memory */
            clHeapFree(tmpAttr); 
        } 
    } 

handleError:
    /* Delete the buffer */
    clBufferDelete(&cookie->rtAttrBufMsgHdl);
    clBufferDelete(&cookie->contAttrBufMsgHandle);
     
    /* Free cookie */
    clHeapFree(cookie);
    CL_FUNC_EXIT();
    return (rc);


}


/*  
 *  This function  packs the object (including contained)for client.
 *  the packed buffer can be shipped to cor-client library,
 *  and unpacked there.
 *  
 */
ClRcT
clCorDmExtObjPack(DMObjHandle_h objHandle, ClUint8T srcArch,  
                              ClCorObjAttrWalkFilterT *pWalkFilter, 
                                       ClBufferHandleT bufMsgHandle,
                                       ClCorObjectHandleT objHdl)
{
   ClRcT rc = CL_OK;
   ClCorObjectAttrInfoT objAttrInfo = {0, 0, 0}; /* buffer end marker */
   ClCorAttrPathPtrT    pStartAttrPath = NULL;

    CL_FUNC_ENTER();
 
      rc = clCorAttrPathAlloc(&pStartAttrPath);
      if(rc != CL_OK)
      {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Insufficient memory...Failed to allocate \n"));
         CL_FUNC_EXIT();
         return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
      }

    /* Copy only the absolute attribute path (without any wildcards) */
     if((rc =  clCorDmExtObjPackWithFilter(objHandle, srcArch, pStartAttrPath,  pWalkFilter, bufMsgHandle, objHdl))!= CL_OK)
     { 
        clCorAttrPathFree(pStartAttrPath);
        CL_FUNC_EXIT();
        return (rc);
     }
     /** Write the packed object end marker. **/
     if((rc = VDECL_VER(clXdrMarshallClCorObjectAttrInfoT, 4, 0, 0)( (void *)&objAttrInfo, 
								bufMsgHandle, 0)) != CL_OK)
     {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\n Buffer Message Write Failed. \n"));
     }
  
   clCorAttrPathFree(pStartAttrPath);
   CL_FUNC_EXIT();
   return (rc);
}


/**
 * Function to get the value of a runtime attribute from the
 * register OI. If there is no OI, this funciton should fill the
 * default value.
 */

ClRcT
_clCorDmAttrRtAttrGet(ClCorDmObjPackAttrWalkerT *pBuf, CORAttr_h attrH, 
                      ClPtrT *pAttrBuf)
{
    ClRcT   rc = CL_OK;
    ClBufferHandleT bufH = 0;
    ClUint32T type  = 0;
    ClCorObjectAttrInfoT objAttrInfo = {0};
    ClInt32T size = 0, tempSize = 0;

    rc  = clBufferCreate(&bufH);
    if (CL_OK != rc)
    {
        clLogError("OBW", "CMP", 
                "Failed while creating the buffer handle. rc[0x%x]", rc);
        return rc;
    }

    rc = _clCorRuntimeAttrFill(pBuf->objHandle, pBuf->pCurrAttrPath, &attrH, 1, bufH);
    if (CL_OK != rc)
    {
        clLogNotice("OBW", "CMP", 
                "Failed to get the value from the OI, now putting the default value. rc[0x%x]", rc);
        goto handleError;
    }

    rc = VDECL_VER(clXdrUnmarshallClCorObjectAttrInfoT, 4, 0, 0)(bufH, (void *)&objAttrInfo); 
    if (CL_OK != rc)
    {
        clLogError("OBW", "CMP", "Failed to unmarshall the object attribute info. rc[0x%x]", rc);
        goto handleError;
    }

    size = objAttrInfo.lenOrContOp;

    if(objAttrInfo.attrType == CL_COR_ARRAY_ATTR ||
            objAttrInfo.attrType == CL_COR_ASSOCIATION_ATTR)
    {
        rc = clXdrUnmarshallClUint32T(bufH, (void *)&type);
        if (CL_OK != rc)
        {
            clLogError("OAW", "CMP", 
                    "Failed while unmarshalling the array type. rc[0x%x]", rc);
            goto handleError;
        }
        
        if(objAttrInfo.attrType == CL_COR_ASSOCIATION_ATTR)
            type = CL_COR_INVALID_DATA_TYPE;
        
        /* 4-bytes reserved for array type */ 
        size -= sizeof(ClUint32T);

        *pAttrBuf = clHeapAllocate(size);
        if(NULL == *pAttrBuf)
        {
            rc = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            clLogError("OAW", "CMP", 
                    "Failed to allocate the memory for the buffer. rc[0x%x]", rc);
            goto handleError; 
        }

        tempSize = size;
        rc = clBufferNBytesRead(bufH, *pAttrBuf, (ClUint32T *) &size);
        if(CL_OK != rc)
        {
            clLogError("OAW", "CMP", 
                    "Failed to read attribute value from the buffer. rc[0x%x]", rc);
            clHeapFree(*pAttrBuf);
            goto handleError; 
        }
        CL_ASSERT(tempSize == size);
    }
    else
    {
        *pAttrBuf = clHeapAllocate(size);
        if(NULL == *pAttrBuf)
        {
            rc = CL_COR_SET_RC(CL_ERR_NO_MEMORY);
            clLogError("OAW", "CMP", 
                    "Failed to allocate memory for the attribute buffer. rc[0x%x]", rc);
            goto handleError;
        }

        tempSize = size;
        rc = clBufferNBytesRead(bufH, *pAttrBuf, (ClUint32T *) &size);
        if(rc != CL_OK)
        {
            clLogError("OAW", "CMP", 
                    "Failed to read Bytes from the Message. rc[0x%x]", rc);
            clHeapFree(*pAttrBuf);
            goto handleError;
        }

        CL_ASSERT(tempSize == size);
    }

    clBufferDelete(&bufH);
    return rc;

handleError:
    clBufferDelete(&bufH);

    size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
    if (size < 0)
    {
        clLogError("OAW", "CMP", "Failed while getting the size of attribute. rc[0x%x]", rc);
        return rc;
    }

    *pAttrBuf = clHeapAllocate(size);
    if (NULL == *pAttrBuf)
    {
        clLogError("OAW", "CMP", "Failed to allocate the buffer for putting the default value. ");
        return CL_COR_SET_RC(CL_ERR_NO_MEMORY);
    }

    if (attrH->attrType.type == CL_COR_ARRAY_ATTR)
        memset(*pAttrBuf, 0, size);
    else
        dmObjectAttrConvert(*pAttrBuf, attrH->attrValue.init,
                        attrH->attrType, size);
    return CL_OK;
}
