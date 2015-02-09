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
 * File        : clCorDmClass.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Handles Data Manager - Class related routines
 *****************************************************************************/

/* FILES INCLUDED */

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>

/*Internal Headers*/
#include "clCorDmDefs.h" 
#include "clCorDmProtoType.h" 
#include "clCorNiIpi.h"
#include "clCorLog.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALS */

/** 
 * Compute size for the given class.
 *
 * API to compute size for all the class definitions. This function is
 * used by hashtable
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      need to fix the order in which its initialized.
 *
 */
ClRcT
dmClassSize(CORHashKey_h   key, 
            CORHashValue_h classBuf, 
            void *         userArg,
            ClUint32T dataLength
            )
{
    CORClass_h tmp = 0;
    ClInt32T sz = 0;
    ClInt32T* val = 0;
    ClUint16T Id = 0;
    ClUint16T* attrIdx = &Id;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if(!classBuf)
      {
        CL_FUNC_EXIT();  
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

    tmp = (CORClass_h) classBuf; 
    if(tmp->size < 0) 
      {
        /* compute the full size */
        clLogDebug("DM", "ATR", "Computing class size for class id : [0x%x]", tmp->classId);

        val = &sz;
        rc = dmClassAttrWalk(tmp, NULL, dmClassAttrSize, (Byte_h*) &val);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Class Attr Walk failed. Failed to compute the size of the attributes. rc [0x%x]", rc));
            CL_FUNC_EXIT();
            return rc;
        }
		
        sz = (sz + sizeof(ClWordT)-1) & ~(sizeof(ClWordT)-1);

   /* Assign indexes to attributes. To be used in compressed 
         * DM Object Handle */
        rc = dmClassAttrWalk(tmp,
   				NULL,
   				dmObjectAttrIndexSet,
   				(Byte_h*)&attrIdx);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Class Attr Walk failed. Failed to find the indexes of the attributes. rc [0x%x]", rc));
            CL_FUNC_EXIT();
            return rc;
        }

        tmp->size = sz;
        clLogDebug("DM", "ATR", "Size of the class [0x%x] is : [%d]", tmp->classId, tmp->size);

        /* initialize the objects vector */
        
        /* todo: check the return value
         */
      }

    CL_FUNC_EXIT();
    return CL_OK;
}

/** 
 *  Class Type create API.
 *
 *  Creates a new class type with the given information like class id,
 *  number of attributes, max and minimum instances for the class.
 *  NOTE: What happens in case of different versions?? need to figure
 *  out.
 *
 *  @param id            class id
 *  @param nAttrs        Number of attributes in class.
 *
 *  @returns 
 *    CORClass_h   (non-null) valid class handle
 *      null(0)    on failure.
 * 
 */
ClRcT
dmClassCreate(ClCorClassTypeT id,
              ClCorClassTypeT inhId)
{
    CORClass_h tmp = 0;
    CORClass_h tmp1 = 0;
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if(!dmGlobal)
    {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassCreate (Class:%04x, Inh:%04x)", id, inhId));
    
    /* check if id & inhId is already present 
     */
    HASH_GET(dmGlobal->classTable, id, tmp);
    if(inhId > 0) 
    {
        HASH_GET(dmGlobal->classTable, inhId, tmp1);
    }
    
    if(tmp) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassCreate (Class:%04x, Inh:%04x) [Class present]", id, inhId));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_PRESENT);
    } 
    else if(tmp1 == 0 && inhId > 0) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassCreate (Class:%04x, Inh:%04x) [Superclass unknown]", id, inhId));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
    } 
    else 
    {
        /* Create the new class
         */
        tmp = (CORClass_h) clHeapAllocate(sizeof(CORClass_t));
        if(tmp != 0) 
        {
            /* init stuff here */
            tmp->classId = id;
            tmp->superClassId = inhId; 
            tmp->size = -1;
            tmp->version.releaseCode = CL_RELEASE_VERSION;
            tmp->version.majorVersion = CL_MAJOR_VERSION;
            tmp->version.minorVersion = CL_MINOR_VERSION;
            tmp->flags = 0;
            tmp->moClassInstances = 0;

            if (CL_OK != (ret = HASH_CREATE(&tmp->attrList)))
            { 
                  clHeapFree(tmp);
                  CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to create hash table for Attributes [rc 0x%x]", ret));
                  CL_FUNC_EXIT();  
                  return (ret);
            }
        
            if (CL_OK != (ret = COR_LLIST_CREATE(&tmp->objFreeList)))
            {
                clHeapFree(tmp);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to create hash table for free objects [rc 0x%x]", ret));
                CL_FUNC_EXIT();
                return (ret);
            }

            tmp->nAttrs = 0;
            tmp->noOfChildren = 0;
            tmp->recordId = 1;
            tmp->objCount = 0;
            tmp->objBlockSz = COR_OBJ_DEF_BLOCK_SIZE;

            /* init the vector, check the return value
             * and remove class/hashtable
             */
            if (CL_OK != (ret = corVectorInit(&tmp->attrs, 
                          sizeof(CORAttr_t),
                          COR_CLASS_DEF_BLOCK_SIZE)))
            {
                HASH_FREE(tmp->attrList);
                COR_LLIST_FREE(tmp->objFreeList);
                clHeapFree(tmp);
    	        CL_FUNC_EXIT();  
     	        return (ret);
	        }

            /* add the newly created class to the class table */
            if (CL_OK != (ret = HASH_PUT(dmGlobal->classTable, id, tmp)))
	        {
              corVectorRemoveAll(&tmp->attrs);
              HASH_FREE(tmp->attrList);
              COR_LLIST_FREE(tmp->objFreeList);
              clHeapFree(tmp);
       	      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to add class in Data Manager [rc 0x%x]", ret));
    	      CL_FUNC_EXIT();  
     	      return (ret);
	        }
            /* declare the inh class as base class */
            if(inhId > 0) 
            {
                tmp1->noOfChildren++;
                COR_CLASS_SETAS_BASE(*tmp1);
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

    CL_FUNC_EXIT();  
    return (ret);
}

/** 
 * Delete a class from class handle.
 *
 * API to dmClassByHandleDelete <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassByHandleDelete(CORClass_h classHandle
                      )
{
    ClRcT ret = CL_OK;
    CORClass_h tmp = 0;

    CL_FUNC_ENTER();

    if((!classHandle) || (!dmGlobal))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassDelete (Class:%04x)", 
                          classHandle->classId));


    /* check if instances are there and also base classes
     * by default cannot be deleted, they may be in other
     * inherited classes.
     */
    if (COR_CLASS_IS_BASE(*classHandle))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassDelete (Class:%04x) [Class is base]", classHandle->classId));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_IS_BASE);
    }
    else if(classHandle->objCount == 0 && 
           (classHandle->moClassInstances == 0))
      {
        
        /* get the handle to the parent class */
        HASH_GET(dmGlobal->classTable,classHandle->superClassId, tmp);
        if(tmp)
        {
            tmp->noOfChildren--;
            if(tmp->noOfChildren == 0)
                COR_CLASS_RESETAS_BASE(*tmp);
        }
        /* now we can remove from hash table & free it 
         */
        HASH_REMOVE(dmGlobal->classTable, classHandle->classId);
        /* free the attribute list hashtable and the elements in
         * the vector 
         */
        HASH_FREE(classHandle->attrList);
        COR_LLIST_FREE(classHandle->objFreeList);

        corVectorRemoveAll(&classHandle->attrs);
        /* nothing to remove in the objects vector */
        clHeapFree(classHandle);
      } 
    else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassDelete (Class:%04x) [Instances present]", 
                              classHandle->classId));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT);
      }
    
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 *  Class Type delete API.
 *
 *  Deletes an existing class Type. 
 *  NOTE: Need to figure out what happens to the instances ???
 *
 *  @param this   Data Manager Handle
 *  @param id     Class Identifier
 *
 *  @returns 
 *    CORClass_h   (non-null) deleted class handle
 *      null(0)    on failure.
 * 
 */
/*Currently we are having another API clCorClassDelete for same purpose. However,
  it takes class handle as input. If we plan to remove it. This will be useful.*/
ClRcT
dmClassDelete(ClCorClassTypeT id)
{
    ClRcT ret = CL_OK;
    CORClass_h tmp = 0;
    
    CL_FUNC_ENTER();

    if((!dmGlobal))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassDelete (Class:%04x)", id));
    
    /* check if class already present 
     */
    HASH_GET(dmGlobal->classTable, id, tmp);
    if(tmp) 
      {
        ret = dmClassByHandleDelete(tmp);
      } 
    else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassDelete (Class:%04x) [Unknown class]", 
                              id));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
      }
    
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 *  Get Class Type.
 *
 *  Returns Class Type information for the specified class id.
 *
 *  @param this   Data Manager Handle
 *  @param id     Class Identifier
 *
 *  @returns 
 *    CORClass_h   (non-null) class type handle
 *      null(0)    on failure.
 * 
 */
CORClass_h 
dmClassGet(ClCorClassTypeT id
           )
{
    CORClass_h tmp = 0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassGet (Class:%04x)", id));

    if(dmGlobal)
      {
        HASH_GET(dmGlobal->classTable, id, tmp);
      }

    CL_FUNC_EXIT();
    return (tmp);
}


/** 
 * Add Attr details for Class Type.
 * 
 * Gets attribute details for a given attribute id.
 * NOTE: if get/set is there, do we need to have add?? and if 
 *   instances are there, then disallowed.  All the following API's listed
 *   here can be taken into account only if there are no instances.
 *
 *  @param this    COR Class Type handle
 *  @param attr    Attr details (can be null)
 * 
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) if null arguments are passed <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT) if instances of this class are present <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_IS_BASE)  if this class is already inherited by another class <br/>
 *
 *  @todo right now allows to add a attribute id even if the same id
 *  is present in base classes. Need to decide on it and fix it.
 * 
 */
ClRcT 
dmClassAttrAdd(CORClass_h    this,
               ClCorAttrIdT   id,
               CORAttrInfo_t typ,
               CORAttr_h*    attr
               )
{
    CORAttr_h tmp = 0;
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if((!this))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "AttrCreate (Class:%04x, Attr:%x, Type: %x)", 
                          this->classId,
                          id, 
                          typ.type));
    
    /* - check if the attribute id is already present / not 
     * - check if its a base class, that indicates
     *    the class is closed for further additions of attributes,
     * - check if it doesn't have any instances, then its closed
     *    for attribute additions
     */
    tmp = dmClassAttrGet(this, id);
    if(tmp) 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrCreate (Class:%04x, Attr:%x, Type: %x) [Already defined]", 
                              this->classId,
                              id, 
                              typ.type));
        
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_PRESENT);
      } 
    else if(this->objCount>0)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrCreate (Class:%04x, Attr:%x, Type: %x) [Class have instances]", 
                              this->classId,
                              id, 
                              typ.type));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT);
      } 
    else if (COR_CLASS_IS_BASE(*this))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrCreate (Class:%04x, Attr:%x, Type: %x) [Class is a base-class]", 
                              this->classId,
                              id, 
                              typ.type));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_IS_BASE);
    }
    else 
      {
        /*TODO: cast it. */
        tmp = corVectorExtendedGet(&this->attrs,this->nAttrs);
        if(attr)
          {
            *attr = tmp;
          }        
        if(tmp != 0) 
          {
            /* init stuff here */
            tmp->attrId = id;
            tmp->attrType = typ;
            tmp->attrValue.init = 0;
            tmp->attrValue.min = -1;
            tmp->attrValue.max = -1;
            tmp->offset = -1;
            if(CL_COR_CONTAINMENT_ATTR == tmp->attrType.type)
                tmp->userFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED;
            else
                tmp->userFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_WRITABLE | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT;

            /* add to attr list */
            HASH_PUT(this->attrList, id, tmp);
            this->nAttrs ++;
          } 
        else 
          {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR_MEM_ALLOC_FAIL)); 
            ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
          }  
      }
    
    CL_FUNC_EXIT();  
    return (ret);
}


/** 
 * Attribute Walk function.
 *
 * API to Walk thru the list of attributes (in the order they appear
 * in memory (from inherited to current object) and run the function
 * specified in argument against it.  Buf is passed around these
 * functions, so in case any cumulative work to be done, they
 * co-ordinate and work on buf.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassAttrWalk(CORClass_h this, 
                CORAttr_h  till,
                ClRcT   (*attrfp)(CORAttr_h, Byte_h*),
                Byte_h*    buf
                )
{
    ClInt32T i = 0;
    ClRcT ret = CL_OK;
    CORClass_h parent = 0; 
  
    CL_FUNC_ENTER();

    if((!this) || (!attrfp))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    /* check if this is inherited, if so, we need to walk thru the
     * parent attributes first
     */
    if(this->superClassId > 0) 
      {
        parent = dmClassGet(this->superClassId);
        if(parent) 
          {
            ret = dmClassAttrWalk(parent, till, attrfp, buf);
          } 
        else 
          {
            /* there is some issue with the parent class, report it */
          }  
      }  
    
    for(; i < this->nAttrs; i++) 
      {
        CORAttr_h attr = (CORAttr_h) corVectorGet(&this->attrs, i);
        if ( (attr == NULL) || (attr->attrId == 0))
        {
            clLogTrace("DMC", "AWK", 
                    "Either attrH is NULL or attrId is zero, so continuing ...");
            continue;
        }
        
        if((till != 0 && 
            till->attrId == attr->attrId) ||
           ret == CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED)) 
          {
            /* stop the walk here */
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED);
            break;
          }
        
        ret = (*attrfp)(attr, buf);
        if(ret != CL_OK) break;
      }

    CL_FUNC_EXIT();  
    return ret;
}


/** 
 * Get Attr details for Class Type.
 * 
 * Gets attribute details for a given attribute id.
 *
 *  @param this    COR Class Type handle
 *  @param aid     Attr id (index) 
 * 
 *  @returns 
 *    CORAttr_h    non-null, successful and holds the attribute info
 *     null(0)     attribute not present
 * 
 */
CORAttr_h 
dmClassAttrGet(CORClass_h  this,
               ClCorAttrIdT aid
               )
{
    CORAttr_h tmp = 0;
    CORClass_h parent = 0; 

    CL_FUNC_ENTER();

    if(this)
      {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "AttrGet (Class:%04x, Attr:%x)", 
                              this->classId,
                              aid));
        
        /* - check if the attribute id is already present / not using HT
         * - if not present, then check in the base class for its
         *   presence.
         */
        HASH_GET(this->attrList, aid, tmp);
        if(!tmp) 
          {
            if(this->superClassId > 0) 
              {
                parent = dmClassGet(this->superClassId);
                if(parent) 
                  {
                      clLogDebug("DM", "GET", "For class [%d], dm is present", this->superClassId);
                    tmp = dmClassAttrGet(parent, aid);
                  } 
              }
          }
      }
    
    CL_FUNC_EXIT(); 
    return tmp;
}

/** 
 * Compare class definition.
 * 
 * Compares two class definitions.  
 *
 *  @param this Class handler
 *  @param cmp  Compare class handler
 * 
 *  @returns 
 *    ClRcT  CL_OK on success
 * 
 */
ClRcT
dmClassCompare(CORClass_h this, 
               CORClass_h cmp
               )
{
    ClInt32T i = 0;
    CORAttr_h tmp = 0;
    
    CL_FUNC_ENTER();

    if((!this) || (!cmp))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    if(this->classId != cmp->classId ||
       this->superClassId != cmp->superClassId ||
       (this->version.releaseCode != cmp->version.releaseCode ||
        this->version.majorVersion != this->version.majorVersion ||
        this->version.minorVersion != this->version.minorVersion) ||
       this->nAttrs != cmp->nAttrs) 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassCompare (Classes: %04x,%04x)", 
                              this->classId,
                              cmp->classId));

        CL_FUNC_EXIT();  
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_MISMATCH);
      }

    /* run thru all the attributes and verify 
     * if they compare!.  
     */
    for(i = 0; i < this->nAttrs; i++) 
      {
        CORAttr_h attr = (CORAttr_h) corVectorGet(&this->attrs, i);
        tmp = dmClassAttrGet(cmp, attr->attrId);
        
        if(!tmp || 
           dmClassAttrCompare(attr, tmp) != CL_OK) 
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassCompare (Attr: %x) mismatch", 
                                  attr->attrId));

            CL_FUNC_EXIT();  
            return CL_COR_SET_RC(CL_COR_ERR_CLASS_MISMATCH);
          }
      }
    
    CL_FUNC_EXIT(); 
    return CL_OK;
}



/** 
 * Get the bitmap of attribute list.  
 *
 * API to return back bitmap of attributes that are specified in
 * attrList.
 *
 *  @param this       class handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT
dmClassBitmap(ClCorClassTypeT classId, 
              ClCorAttrIdT*   attrList,
              ClInt32T      nAttrs,
              ClUint8T*     bits,
              ClUint32T       bitMaskSz
              )
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;
    CORAttr_h ah = 0; 
    ClInt32T pos = 0;
    ClInt32T* val = &pos;
    CORClass_h class;

    CL_FUNC_ENTER();

    if((!dmGlobal) || (classId <= 0) || (!attrList) || (!bits))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Class Bitmap (Class:%04x nAttrs:%d bitmaskSz:%d)", 
                          classId,
                          nAttrs,
                          bitMaskSz));
    
    class = dmClassGet(classId);
    if(class)
      {
        /* We do not zero the input bits, effectively OR'ing with it.  
         */
        for(i = 0; i < nAttrs; i++) 
          {
            /* Reset the attribute position */
            pos = 0;
            ah = dmClassAttrGet(class, attrList[i]);
            if(ah) 
              {
                /* find the position of the attribute
                 */
                dmClassAttrWalk(class,ah,dmClassAttrPos, (Byte_h*) &val);
                /* add pos to the bitmap 
                 */
                clLogTrace("DM", "BIT", "Bitmap found : Attribute [%d], Position : [%d]",
                        attrList[i], pos);

                SET_BIT(bits, pos);
              } 
            else 
              {                
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                          ( "Bitmap (Class:%04x nAttrs:%d) [Unknown attr %x]",
                           classId,
                           nAttrs,
                           attrList[i]));
                
                ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
                break;
              } 
          }
      }
    else
      {
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
      }
    
    CL_FUNC_EXIT(); 
    return (ret);
}


/** 
 * Pack class information to the given buffer.
 *
 * API to dmClassPack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassPack(CORHashKey_h   key, 
            CORHashValue_h classBuf, 
            void *         userArg,
            ClUint32T     dataLength
            )
{
    ClRcT       rc = CL_OK;
    CORClass_h tmp = 0;
    ClInt32T i = 0;
    ClPtrT * dummy = (ClPtrT *) userArg;
    ClBufferHandleT *pBufH = NULL;

    
    CL_FUNC_ENTER();

    if((!classBuf) || (!dummy))
      {
        CL_FUNC_EXIT();  
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

    /* first set the pack buffer from userArg */
    pBufH = *dummy;
    tmp = (CORClass_h) classBuf;
	ClUint32T tmpPackL ;
	ClUint16T tmpPackS ;

    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->classId, tmpPackL, sizeof(tmp->classId));
    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->superClassId, tmpPackL, sizeof(tmp->superClassId));
    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->size, tmpPackL, sizeof(tmp->size));

	/* The version and flag are 8 bit value so no need to convert */
    STREAM_IN_BUFFER(pBufH, &tmp->version.releaseCode, sizeof(tmp->version.releaseCode));
    STREAM_IN_BUFFER(pBufH, &tmp->version.majorVersion, sizeof(tmp->version.majorVersion));
    STREAM_IN_BUFFER(pBufH, &tmp->version.minorVersion, sizeof(tmp->version.minorVersion));

    STREAM_IN_BUFFER(pBufH, &tmp->flags, sizeof(tmp->flags));
    STREAM_IN_BOUNDS_HTONS(pBufH, &tmp->nAttrs, tmpPackS, sizeof(tmp->nAttrs));
    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->recordId, tmpPackL, sizeof(tmp->recordId));
    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->objCount, tmpPackL, sizeof(tmp->objCount));
    STREAM_IN_BOUNDS_HTONL(pBufH, &tmp->objBlockSz, tmpPackL, sizeof(tmp->objBlockSz));

    /* pack the attrs here */
    for(; i < tmp->nAttrs; i++) 
      {
        CORAttr_h attr = (CORAttr_h) corVectorGet(&tmp->attrs, i);
        rc  = dmClassAttrPack(attr, pBufH);
        if(CL_OK != rc)
        {
            clLog(CL_LOG_SEV_ERROR,"DCP","", "Failed while packing the attribute \
                        information. rc[0x%x]", rc);
            return rc;
        }
      }
    
    CL_FUNC_EXIT(); 
    return CL_OK;
}

/** 
 * Returns number of instances of the given class.
 *
 * API to dmClassInstanceCountGet <Deailed desc>. 
 *
 *  @param classId      COR Class Type
 *  @param 
 * 
 *  @returns 
 *      ClUint32T  number of instances
 *
 *  @todo      
 *
 */
#if 0
ClUint32T
dmClassInstanceCountGet(ClCorClassTypeT classId)
{
    CORClass_h  tmp = 0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassInstanceCountGet (Class:%04x)", classId));
    tmp = dmClassGet(classId);
    if (tmp)
    {
        CL_FUNC_EXIT();
        return tmp->recordId;
    }
    return 0;
}
#endif

/** 
 * Unpack Class buffer.
 *
 * API to UnPack class information from the given buffer. Alloc's a
 * new class and returns back.  note:Need to handle the merge
 * scenario, when there is the class already defined, need to verify
 * all the stuff before concluding on error.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo its going to fail, because the replay is not exactly the
 * same order in which classes were created.  so, need to make sure in
 * packing that the same order is used to pack!!!
 *
 */
ClUint32T
dmClassUnpack(void *      contents, 
              CORClass_h* ch
              )
{
    ClRcT       rc = CL_OK;
    Byte_h buf = (Byte_h)contents;
    Byte_h tmpBuf = buf;
    ClCorClassTypeT id = 0;
    ClCorClassTypeT inhId = 0;
    CORClass_h tmp = 0;
    ClInt32T i = 0;
    ClInt32T sz = 0;
    
    CL_FUNC_ENTER();

    if((!ch) || (!contents))
      {
        CL_FUNC_EXIT();  
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    /*  STREAM_OUT(&id, buf, sizeof(id));
    STREAM_OUT(&inhId, buf, sizeof(inhId));
	*/
    STREAM_OUT_NTOHL(&id, buf, sizeof(id));
    STREAM_OUT_NTOHL(&inhId, buf, sizeof(inhId));
    
    /* should not call this */
    /* dmClassCreate(id, inhId, ch); */
    *ch = (CORClass_h) clHeapAllocate(sizeof(CORClass_t));
    if(*ch == NULL)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
			CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
	return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    tmp = *ch;
    tmp->classId = id;
    tmp->superClassId = inhId;
    /* STREAM_OUT(&tmp->size, buf, sizeof(tmp->size));
    STREAM_OUT(&tmp->version, buf, sizeof(tmp->version));
    STREAM_OUT(&tmp->flags, buf, sizeof(tmp->flags));
    STREAM_OUT(&tmp->nAttrs, buf, sizeof(tmp->nAttrs));
    
    STREAM_OUT(&tmp->recordId, buf, sizeof(tmp->recordId)); 
	*/
	STREAM_OUT_NTOHL(&tmp->size, buf, sizeof(tmp->size));
    STREAM_OUT(&tmp->version.releaseCode, buf, sizeof(tmp->version.releaseCode));
    STREAM_OUT(&tmp->version.majorVersion, buf, sizeof(tmp->version.majorVersion));
    STREAM_OUT(&tmp->version.minorVersion, buf, sizeof(tmp->version.minorVersion));

    STREAM_OUT(&tmp->flags, buf, sizeof(tmp->flags));
    STREAM_OUT_NTOHS(&tmp->nAttrs, buf, sizeof(tmp->nAttrs));
    
    STREAM_OUT_NTOHL(&tmp->recordId, buf, sizeof(tmp->recordId)); 
    STREAM_OUT_NTOHL(&tmp->objCount, buf, sizeof(tmp->objCount)); 
	
    /* record id to be inited to one, ignore the streamed one
     */
    tmp->recordId = 1;

    /** objCount inited to 0, ignore the streamed one */
    tmp->objCount = 0;

    /* STREAM_OUT(&tmp->objBlockSz, buf, sizeof(tmp->objBlockSz));  */
    STREAM_OUT_NTOHL(&tmp->objBlockSz, buf, sizeof(tmp->objBlockSz));
    
    /* create & initialize the hash table here @todo: check for the
     * return value/ error condition in all these cases.
     */
    HASH_CREATE(&tmp->attrList);

    /* Create the Hash list for containing ptrs to deleted objects */
    rc = COR_LLIST_CREATE(&tmp->objFreeList);
    if (CL_OK != rc)
    { 
        clLogError("DMC", "UPK", 
                "Failed to create the container for storing the deleted job");
        CL_ASSERT(0);
    }

    corVectorInit(&tmp->attrs, 
                  sizeof(CORAttr_t),
                  COR_CLASS_DEF_BLOCK_SIZE);

    for(; i < tmp->nAttrs; i++) 
      {
        CORAttr_h attr = (CORAttr_h) corVectorExtendedGet(&tmp->attrs, i);
        sz = dmClassAttrUnpack(attr, buf);
        if (attr->attrId != 0)
            HASH_PUT(tmp->attrList, attr->attrId, attr);
        buf += sz;
      }

    CL_FUNC_EXIT(); 
    return (buf - tmpBuf);
}

/** 
 * Show class information. 
 *
 * API used by hash table function to print the class
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassShow(CORHashKey_h   key, 
            CORHashValue_h classBuf, 
            void *         userArg,
            ClUint32T dataLength
            )
{
    CORClass_h tmp = 0;
    CORClassShowCookie_t cookie;
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    char     className[CL_COR_MAX_NAME_SZ] = {"\0"};
    ClBufferHandleT *pMsg= (ClBufferHandleT *)userArg;
    CL_FUNC_ENTER();

    if((!classBuf))
      {
        CL_FUNC_EXIT();  
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

    tmp = (CORClass_h) classBuf;  

    corStr[0]='\0';
    sprintf(corStr, "Class %04x", tmp->classId);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    _corNiKeyToNameGet(tmp->classId, className);
    corStr[0]='\0';
    sprintf(corStr, " [%s] ",className);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    corStr[0]='\0';
    sprintf(corStr, " <parent:%04x,ver:[0x%x.0x%x.0x%x],flag:0x%x,seq:%x,objcnt:%x,sz:%d,blksz:%d> {\n",
             tmp->superClassId,
             tmp->version.releaseCode,
             tmp->version.majorVersion,
             tmp->version.minorVersion,
             tmp->flags,
             tmp->recordId,
             tmp->objCount,
             tmp->size,
             tmp->objBlockSz);

    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

    cookie.classHdl = tmp;
    cookie.pMsg     = pMsg; 
    corVectorWalk(&tmp->attrs, dmClassAttrShow, (void * *)&cookie);
    corStr[0]='\0';
    sprintf(corStr, " }\n");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    
    corStr[0]='\0';
    sprintf(corStr, "\n");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    
    CL_FUNC_EXIT(); 
    return CL_OK;
}



/** 
 * Show class information [verbose]. 
 *
 * API used by hash table function to print the class information
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassVerboseShow(CORHashKey_h   key, 
                   CORHashValue_h classBuf, 
                   void *         userArg,
                   ClUint32T dataLength
                   )
{
    CORClass_h tmp = 0;
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    char       className[CL_COR_MAX_NAME_SZ] = {"\0"};
    ClBufferHandleT* pMsgHdl = (ClBufferHandleT* )userArg;
  
    CL_FUNC_ENTER();

    if((!classBuf))
      {
        CL_FUNC_EXIT();  
        return CL_OK;
      }

    tmp = (CORClass_h) classBuf;  
    
    corStr[0]='\0';
    sprintf(corStr, "Class 0x%04x", tmp->classId);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));

    _corNiKeyToNameGet(tmp->classId, className);
    corStr[0]='\0';
    sprintf(corStr, " [%s] ",className);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    
    corStr[0]='\0';
    sprintf(corStr, " parent:0x%04x attrs:%2d flag:0x%02x ver:[0x%x.0x%x.0x%x] nxtrec:%d objcount:%d sz:%d",
             tmp->superClassId,
             tmp->nAttrs,
             tmp->flags,
             tmp->version.releaseCode,
             tmp->version.majorVersion,
             tmp->version.minorVersion,
             tmp->recordId,
             tmp->objCount,
             tmp->size);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    /* show the objects for this class */
    corStr[0]='\0';
    sprintf(corStr, "[alloc:%d/%d] \n",
             (tmp->size>-1)?corVectorSizeGet(tmp->objects):0,
             tmp->objBlockSz);
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));

    CL_FUNC_EXIT(); 
    return CL_OK;
}

/** 
 * Delete Attr from Class Type.
 * 
 * Removes the attribute from the class type.  The operation is
 * enabled only if there are no instances created already.
 * NOTE: Do we have to give this API at all ??? 
 *
 *  @param this    COR Class Type handle
 *  @param aid     Attr id (index) 
 * 
 *  @returns 
 *      CL_OK   on success</br>
 * 
 */
ClRcT
dmClassAttrDel(CORClass_h this, 
               ClCorAttrIdT attrId)
{
    ClRcT ret = CL_OK;
    CORAttr_h tmp = NULL;
    CL_FUNC_ENTER();

    if ( NULL == this )
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "AttrDelete (Class:%04x, Attr:%x)", 
                          this->classId, attrId));

    HASH_GET(this->attrList, attrId, tmp);
    if (tmp)
    {
        if (this->objCount>0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrDelete (Class:%04x, Attr:%x) [Class have instances]",
                                this->classId, attrId));
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_INSTANCES_PRESENT);
        }
        else if (COR_CLASS_IS_BASE(*this))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrDelete (Class:%04x, Attr:%x) [Class is a base-class]",
                                this->classId, attrId));
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_IS_BASE);
        }
        else
        {
            HASH_REMOVE(this->attrList, attrId);
            memset(tmp, 0, sizeof(CORAttr_t));
        }
    }
    else
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    }

    CL_FUNC_EXIT();
    return ret;
}


/*
*   Function to get the class id of leaf node in Attribute path.
*/

ClRcT dmAttrPathToClassIdGet(ClCorClassTypeT containerClsId,
                                    ClCorAttrPathPtrT pAttrPath, ClCorClassTypeT *containedClsId)
{
                                                                                                                            
    CORClass_h pClass = NULL; /* class to be reached from AttrPath*/
    CORAttr_h  pContAttrHdl = NULL;  /* attribute definition for contained attribute*/
    ClUint32T  i = 0;
 
    pClass = dmClassGet(containerClsId);
    if(pClass)
    {  
       while(i < pAttrPath->depth)
        {
           pContAttrHdl = dmClassAttrGet(pClass, pAttrPath->node[i].attrId);
           if(pContAttrHdl)
           {
             /* Check if it is not a Containment Attribute.*/
               if(pContAttrHdl->attrType.type != CL_COR_CONTAINMENT_ATTR)
                 {
                  return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE));
                 }
                /* get the class handle for contained attribute. */
                pClass = dmClassGet(pContAttrHdl->attrType.u.remClassId);
                /* no need to check for pClass  as it is taken care
                   while creating containment attribute */
           }
             else
             {
               return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT));
             }
         /* increment the index */
         i++;
        }
      /* we got the final class id.*/
       *containedClsId = pClass->classId;
    }
    else
    {
       return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
    }
   return CL_OK;
}
