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
 * File        : clCorDmAttr.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Handles Data Manager - Attribute related routines
 *****************************************************************************/

/* FILES INCLUDED */
#include <ctype.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorApi.h>
#include <clCorErrors.h>
#include <clBitApi.h>

/* Internal Headers */
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorNiIpi.h"
#include "clCorPvt.h"
#include "clCorDmData.h"


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


#define UNKNOWN_TYPE    0
#define SIGNED_TYPE     1
#define UNSIGNED_TYPE   2

#define CL_COR_CONV_TO_H64(src, val) ((src == clByteEndian) ? val : CL_BIT_SWAP64(val))
#define CL_COR_CONV_TO_H32(src, val) ((src == clByteEndian) ? val : CL_BIT_SWAP32(val))
#define CL_COR_CONV_TO_H16(src, val) ((src == clByteEndian) ? val : CL_BIT_SWAP16(val))
/* Static definitions */
void * dmObjectAttrConvert(void *buf, ClInt64T value, CORAttrInfo_t type, ClUint32T size);

ClRcT clCorRuntimeAttrGet(ClCorObjectHandleT objHandle, ClCorAttrPathPtrT pAttrPath, CORAttr_h attrH,char** data);
ClRcT clCorPrintAttr2Buffer(ClBufferHandleT  bufHdl,CORAttr_h attrH,ClUint8T *data,ClBoolT swapEndian);



ClRcT _dmAttrRangeValidate(CORAttrInfo_t attrType, 
                            ClInt64T newVal, 
                            ClInt64T min, 
                            ClInt64T max);
/** 
 * Data Manager Attribute init routine.
 *
 * API to dmClassAttrInit <Deailed desc>. 
 *
 *  @param this   Attrbute object handle
 *  @param init   Default init value
 *  @param min    min value the attribute can take
 *  @param max    max value the attribute can take
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PARAM)  on null parameter
 *
 *  @todo      
 *
 */
ClRcT
dmClassAttrInit(CORAttr_h this, 
                ClInt64T init,
                ClInt64T min,
                ClInt64T max
                )
{
    ClRcT ret = CL_OK;
    
    CL_FUNC_ENTER();

    if(!this)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Init %d = {%lld} [Min:%lld,Max:%lld]", 
                          this->attrId,
                          init,
                          min,
                          max));
    ret = _dmAttrRangeValidate(this->attrType, init, min, max);
    if ( CL_OK != ret )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrId 0x%x : [Init: %lld, Min:%lld,Max:%lld] [Invalid Value]", 
                              this->attrId,
                              init,
                              min,
                              max));
    }
    else
    {
        /* Set the vales and ranges to the attribute 
         */
        this->attrValue.init = init;
        this->attrValue.min = min;
        this->attrValue.max = max;
    }

    CL_FUNC_EXIT();  
    return (ret);  
}

/** 
 * Returns attribute type size.
 *
 * API to dmClassAttrTypeSize <Deailed desc>. 
 *
 *  @param at    COR attribute type
 *  @param val   Attribute value structure
 * 
 *  @returns 
 *    ClInt32T   size of the attribute type <br>
 *
 *  @todo      
 *
 */
ClInt32T
dmClassAttrTypeSize(CORAttrInfo_t    at, 
                    CORAttrValType_t val
                    )
{
    ClInt32T sz = -1;
    ClInt32T arr = 0;
    ClCorTypeT typ = at.type;

    CL_FUNC_ENTER();

 compute_size:  
    switch((ClInt32T)typ) 
      {
        /* Primitive types */
        case CL_COR_INT8:
        case CL_COR_UINT8:
            sz=1;
            break;
        case CL_COR_INT16:
        case CL_COR_UINT16:
            sz=2;
            break;
        case CL_COR_INT64:
        case CL_COR_UINT64:
            sz=8;
            break;
        case CL_COR_ARRAY_ATTR:
            if(!arr) 
              {
                arr=1;
                typ = at.u.arrType;  
                goto compute_size;
              }
            
            /* note: potential loop problem, to be fixed -- should NOT
             * come here
             */
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ( "***CRITICAL ERROR***  SEND THIS INFO TO SUPPORT!"));
            break;
        case CL_COR_CONTAINMENT_ATTR:
        case CL_COR_ASSOCIATION_ATTR:
          /* Size of the association/containment (ptr) is equivalent
           * to the size of the DM object handle.
           */
          {
            CORClass_h cls = dmClassGet(at.u.remClassId);
            sz = -1;
            /* first validate the assoc class, if not present,
             * then reject it
             */
            if(cls && val.max > 0) 
              {
                sz = sizeof(DMObjHandle_t);
                sz *= val.max;
              }
          }
          break;

        default:
          sz = 4;
          break;    
    }

    if(arr) 
      {
        /* multiply with actual no of instances */
        if(val.min > 0) sz = (val.min*sz);
        if(val.max > 0) sz = (val.max*sz);
      }

    CL_FUNC_EXIT();  
    return (sz);
}


/** 
 * API to Increment pos parameter and return back.
 *
 *
 *  @param this    Attribute handle
 *  @param pos
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)  on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassAttrPos(CORAttr_h this, 
               Byte_h*   pos
               )
{
    CL_FUNC_ENTER();

    if((!this) || (!pos))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    if(this->attrId == 0) /* Entry is deleted */
    {
        CL_FUNC_EXIT();
        return (CL_OK);
    }
    *(*(int**)pos) = *(*(int**)pos) + 1;

    CL_FUNC_EXIT();
    return CL_OK;
}

#if 0
/** 
 * Data Manager Attribute Size.
 *
 * API to dmClassAttrType2Id <Deailed desc>. 
 *
 *  @param this  attribute handle
 *  @param len   len is a cookie param to which the size is accumulated.
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassAttrType2Id(CORAttr_h this, 
                   Byte_h*   len
                   )
{
    ClRcT ret = CL_OK;
  
    CL_FUNC_ENTER();

    if((!this) || (!len))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    if((this->attrType.type==CL_COR_CONTAINMENT_ATTR ||
        this->attrType.type==CL_COR_ASSOCIATION_ATTR) &&
       *(*(int**)len) == this->attrType.u.remClassId)
      {
        *(*(int**)len) = this->attrId;
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "AttrTypeToId Got Id:%d ", 
                              this->attrId));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_PRESENT);  /* not an error just a marker */
      }

    CL_FUNC_EXIT(); 
    return (ret); 
}
#endif

/** 
 * Data Manager Attribute Size.
 *
 * API to dmClassAttrSize <Deailed desc>. 
 *
 *  @param this  attribute handle
 *  @param len   len is a cookie param to which the size is accumulated.
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassAttrSize(CORAttr_h this, 
                Byte_h*   len
                )
{
    ClInt32T sz = 0;
    ClRcT ret = CL_OK;
  
    CL_FUNC_ENTER();

    if((!this) || (!len))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    clLogDebug("DM", "ATR", "Computing attribute size for attrId [%d]", this->attrId);

    sz = dmClassAttrTypeSize(this->attrType, this->attrValue);
    if(sz < 0) 
      { 
        *(*(int**)len) = sz;
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_RELATION);
    
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "AttrSize Id [0x%x]= {%d} [Invalid Size]", 
                              this->attrId,
                              sz));
      } 
    else 
      {
        if(this->userFlags & CL_COR_ATTR_CACHED)
        {
            clLogDebug("DM", "ATR", "Updating attribute id : [%d], offset : [%d]", 
                    this->attrId, *(*(int**)len));

            this->offset =  *(*(int**)len);
            *(*(int**)len) += sz;
        }
      }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "AttrSize Id:%d Size: %d", 
                          this->attrId, 
                          *(*(int**)len)));

    CL_FUNC_EXIT(); 
    return (ret); 
}

/** 
 * Compare attribute definition.
 * 
 * Compares two attribute definitions.  
 *
 *  @param this Attribute handler
 * 
 *  @returns 
 *    ClRcT  CL_OK on success
 * 
 */
ClRcT
dmClassAttrCompare(CORAttr_h this, 
                   CORAttr_h cmp
                   )
{
    CL_FUNC_ENTER();

    /* compare only the type and id, remaining items can be
     * merged, so ignore them for now
     */
    if(!this || 
       !cmp ||
       this->attrId != cmp->attrId ||
       this->attrType.type != cmp->attrType.type) 
      {
        CL_FUNC_EXIT(); 
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_MISMATCH);
      }

    CL_FUNC_EXIT(); 
    return CL_OK;
}

/** 
 * Validate value against meta data.
 *
 * API to Validate attribute value against min & max specified in
 * attribute type.
 * NOTE: This function assumes that 'attrBuf' is in network order. 
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
dmObjectAttrValidate(CORAttr_h this, 
                     Byte_h    attrBuf
                     )
{
    ClRcT ret = CL_OK;
    ClInt64T val = 0;
    ClInt64T min = 0;
    ClInt64T max = 0;
    ClInt8T  dataType = UNKNOWN_TYPE;

    CL_FUNC_ENTER();

    if((!this) || (!attrBuf))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    min = this->attrValue.min;
    max = this->attrValue.max;

    switch((ClInt32T)this->attrType.type) 
    {
        case CL_COR_INT8: 
            { 
                val  = *(ClInt8T *)attrBuf; 
                dataType = SIGNED_TYPE;
            }
            break;
        
        case CL_COR_UINT8: 
            { 
                val = *(ClUint8T *)attrBuf;
                dataType = UNSIGNED_TYPE;
            } 
            break;

        case CL_COR_INT16: 
            { 
                val = (ClInt16T )CL_BIT_N2H16(*(ClInt16T *)attrBuf);
                dataType = SIGNED_TYPE;
            } 
            break;

        case CL_COR_UINT16: 
            { 
                val = (ClUint16T )CL_BIT_N2H16(*(ClUint16T *)attrBuf);
                dataType = UNSIGNED_TYPE;
            } 
            break;

        case CL_COR_INT32:
            {
                val = (ClInt32T ) CL_BIT_N2H32(*(ClInt32T *)attrBuf);
                dataType = SIGNED_TYPE;
            }
            break;

        case CL_COR_UINT32: 
            {
                val = (ClUint32T )CL_BIT_N2H32(*(ClUint32T *)attrBuf);
                dataType = UNSIGNED_TYPE;
            }
            break;

        case CL_COR_INT64:
            {
                val = (ClInt64T )CL_BIT_N2H64(*(ClInt64T *)attrBuf);
                dataType = SIGNED_TYPE;
            }
            break;

        case CL_COR_UINT64: 
            {
                val = (ClUint64T )CL_BIT_N2H64(*(ClUint64T *)attrBuf);
                dataType = UNSIGNED_TYPE;
            }
            break;

        case CL_COR_ARRAY_ATTR:
           
            {
              /* @todo : need to validate for index and value (maybe) */
            }
             

        /* complex data types */
        case CL_COR_CONTAINMENT_ATTR:
        case CL_COR_ASSOCIATION_ATTR:
        default:
          {
    	    CL_FUNC_EXIT(); 
 	    return (ret);
          }
          break;    
    }

    clLogDebug("TXN", "OPE",  "Validate value:%lld [Min:%lld,Max:%lld]", 
                          val, 
                          min, 
                          max);

    if (SIGNED_TYPE == dataType)
    {
         if ((val >= min) &&
                (val <= max))
            {
                ret = CL_OK;
            }else
              {
                ret = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_INVALID_SET);
              }
    }else if(UNSIGNED_TYPE == dataType)
      {
         if (((ClUint64T)val >= (ClUint64T)min) &&
                ((ClUint64T)val <= (ClUint64T)max))
            {
                ret = CL_OK;
            }else
              {
                ret = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_INVALID_SET);
              }
     }else
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);

    CL_FUNC_EXIT(); 
    return (ret);
}


/** 
 *  Data Manager Attribute Init routine.
 *
 *  Initialize the data attribute field.  The default values specified
 *  in the type definition is set to the value.
 *
 *  @param this    Type definition for the attribute
 *  @param buf     Buffer pointing to the attribute value
 *
 *  @returns 
 *    ClRcT  CL_OK on success
 * 
 */

ClRcT
dmObjectAttrInit(CORAttr_h this, 
                 Byte_h*   buf
                 )
{
    ClInt32T sz = 4;
    ClRcT ret   = CL_OK;
    Byte_h src   = 0;
    Byte_h dst   = 0;
    ClInt32T arritems = 0;
    ClInt32T items = 0;
    ClInt32T i = 0;
  
    CL_FUNC_ENTER();

    if((!this) || (!buf))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    dst = *buf;
    sz = dmClassAttrTypeSize(this->attrType, this->attrValue);
    src = (Byte_h)&this->attrValue.init;

    if(sz < 0) 
      {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_RELATION);
      }
    
    /* No need to update the buffer for any non cached attribute */
    if(!(this->userFlags & CL_COR_ATTR_CACHED))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("No need to do anything for Non Cached attribute, attrId[0x%x]", this->attrId));
        return CL_OK;
    }

    /* if its array, then do a memset 
     */
    if(this->attrType.type == CL_COR_ARRAY_ATTR) 
      {
        arritems = this->attrValue.min;
        if(this->attrValue.max > 0) arritems*=this->attrValue.max;
        
        /* fill the init value,as per the data type.
         */
        sz /= arritems;
        while(arritems--) 
         {
            /*
            * this->attrValue.init is of type 64 bit. However it can store 8/16/32 bit type of values
            * However in the object buffer the space is allocated as per the type of the attribute. For
            * example, when the attribute is of type UINT16, then space allocated for this attribute will
            * be exactly 16 bits. Additionally, it is not garunteed that this 16 bit value is aligned to
            * 16 bit boundary in the object. As a result of these factors, when memcpy is performed from
            * attrValue.init to attribute-buffer inside the object, one needs to take care of where the value
            * of attribute is stored inside the attrValue.init buffer. For example, consider an attribute of type
            * UINT16. Its default value will be stored at address &attrValue.init in the BE case, and &attrValue.init
            * + 6 in the LE case. Hence the src destination address to do the copy from, into objectbuffer.attribute would be different.
            
            * To overcome this issue the following is being done.
            * The value in attrValue.init will be in native format. If thsi attribute is of type UINT16, assign this value to
            * a UINT16 variable. Then copy into objectbuffer.attribute. This way the LE/BE issue would be resolved.

            * An another consider is that objectBuffer must be in Network Byte order. This makes the synching with the peer
            * easy. If it were not so, the the peer COR, must traverse the buffer by indexing it via attribute.type. And
            * depending upon this type being 8/16/32/64 bit, it should apply n2hl/s. This is not being currently done. Instead of
            * this, the buffer is always stored in network format. All, the updates occur to in in network byte order format.
            * While reading from this buffer we will be returned  in native format.

             * Hence before copying into objectbuffer.attribute,  the data needs to be convered into native format. The final sequence
             * would be

             * 1. Determine the type of attribute and it size
             * 2. Declare a local variable of that size, and assign into attrValue.init
             * 3. apply h2n appropriately on it.
             * 4. Copy into the objectbuffer.attribute
              */
            dst = *buf;
           *buf = dmObjectAttrConvert(dst, this->attrValue.init, this->attrType, sz);/* memcpy(dst, src, sz);*/
           *buf += sz;
          }
      } 
    else if(this->attrType.type == CL_COR_CONTAINMENT_ATTR) 
      {    
        items = this->attrValue.max;
        sz /= items;
        
        /* need to call init routines of the class (for all instances) 
         */
        for(i=0; ret == CL_OK && i < items; i++) 
          {
              DMObjHandle_h oh;
				
              ret = dmObjectCreate(this->attrType.u.remClassId, &oh);
              if (CL_OK != ret)
              {
                  clLogError("DMO", "COC", 
                          "Failing to create the contained object while initializing the attribute of object. rc[0x%x]", ret);
                  return ret;
              }

              /*  DM_OH_COPY(*(DMObjHandle_h)*buf, *oh);  */
			  /* The containment attr buffer should also be packed in network byte format. */
		 	  STREAM_IN(*buf,(Byte_t*) &(oh->classId), sizeof(oh->classId));
			  STREAM_IN(*buf,(Byte_t*) &(oh->instanceId),  sizeof(oh->instanceId) );
                /* todo: add the contained flag here */
                #if 0
                if(i >= this->attrValue.min) 
                 {
                   dmObjectDisable(oh);
                 }
            *buf += sz;
                #endif

            /* free the allocated handle (copied, so not required) */
            clHeapFree(oh);
          }      
        
      } 
    else if(this->attrType.type == CL_COR_ASSOCIATION_ATTR) 
      { 
        /* just set the memory to zero */
        if(sz > 0) 
          {
            memset(dst, 0, sz);
          }
        *buf += sz;        
      } 
    else 
      {
        *buf = dmObjectAttrConvert(dst, this->attrValue.init, this->attrType, sz);
        *buf += sz;
      }
    
    CL_FUNC_EXIT(); 
    return (CL_OK); 
}

/** 
 * Copy Attribute Buffer.
 * 
 * Copy the attribute buffer from instBuf to attrBuf (if flag is 1,
 * then its a Get and if its 0, then its a Set).
 *
 *  @param this    Type definition for the attribute
 *
 *  @returns 
 *    CORAttrValue_h  Reference to attribute Value on success.
 *    null(0)         on failure
 */
ClRcT
dmObjectAttrBufCopy(CORAttr_h this, 
                    Byte_h    instBuf, 
                    Byte_h    attrBuf,
                    ClUint32T *size,
                    ClInt32T  idx,
                    int       flag     /* 1 is 'from' and 0 is 'to' */
                    )
{
    /* returns the buffer with the value and also
     * returns back the size of the buffer. Size is
     * in and out parameter.
     *
     * todo: need to convert to and fro between
     *   network common byte order!!
     */
    ClInt32T sz = 4;
    ClInt32T items  = 0;
    DMObjHandle_h oh = 0;

    CL_FUNC_ENTER();

    if((!this) || (!instBuf) || (!attrBuf) || (!size))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    sz = dmClassAttrTypeSize(this->attrType, this->attrValue);    
    if(sz < 0) 
      {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_RELATION);
      }

        /* Validate the size for Simple attributes */
     if(this->attrType.type != CL_COR_ARRAY_ATTR &&
            this->attrType.type != CL_COR_ASSOCIATION_ATTR &&
                 this->attrType.type != CL_COR_CONTAINMENT_ATTR)
     {
          if(*size < sz)
          {
              CL_FUNC_EXIT();
              return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE));
          }
 
         #if 0
            else if(clByteEndian == CL_BIT_BIG_ENDIAN)
            { 
              /* move the attrBuf pointer to actual position.*/
                 attrBuf += (*size - sz);
            }
         #endif
     }

    /* if its array, then check the indexes 
     */
    if(this->attrType.type == CL_COR_ARRAY_ATTR)
      {
        items = this->attrValue.min;
        if(this->attrValue.max > 0)  
          {
            items *= this->attrValue.max;
          }
      }
    if(this->attrType.type == CL_COR_CONTAINMENT_ATTR ||
       this->attrType.type == CL_COR_ASSOCIATION_ATTR) 
      {
        items = this->attrValue.max;
      }
    
    if(this->attrType.type == CL_COR_ARRAY_ATTR ||
       this->attrType.type == CL_COR_CONTAINMENT_ATTR ||
       this->attrType.type == CL_COR_ASSOCIATION_ATTR) 
      {
        if(idx < 0 || idx >= items) 
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attribute {%x} Index [%d] [Out Of Range]", 
                                  this->attrId,
                                  idx));

            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX));
          }

        /* array/association items can copy multiple items in one shot
         * so, update remaining size based on index incase of array
         */
        if(this->attrType.type == CL_COR_ARRAY_ATTR  ||
           this->attrType.type == CL_COR_ASSOCIATION_ATTR)
          {
            int elemSize = sz/items;
            sz -= (idx * elemSize);
            instBuf += (idx * elemSize);


             /*can not SET, if user buffer specified is greater than size left*/
             if(*size > sz && flag == 0) 
             {
                CL_FUNC_EXIT();
                return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE));
             }
             else
             { 

               /* if size is lesser than  native type.
                  e.g. 2 bytes for array of type ClUint32T */
               if(*size < elemSize)
               {
                  CL_FUNC_EXIT();
                  return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE));
               }
               /* size has to be multiples of native type. 
                        for e.g. for a Array of data type ClUint32T,
                                 6 as size should not be allowed. 6 is
                                 ok, if data type ClUint8T or ClUint16T */       
               if((*size)%elemSize != 0)
               {
                  CL_FUNC_EXIT();
                  return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE));
               }
               sz = *size;
             }

          } else  /* containment Attribute. Requires exact Size*/
            {
              /* move the buffer to the index item */
              sz /= items;
              instBuf += (idx*sz);
            }

        /* if it is containment, then upgrade or add to the containment
         * handle. Assumption is that the destination handle is already
         * copied and we just need to append the index.
         */
        if(this->attrType.type == CL_COR_CONTAINMENT_ATTR) 
          {
            if(flag == 0) 
              {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cont {%x} [Set not allowed, call Create]", 
                                      this->attrId));

                CL_FUNC_EXIT();
                return CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_INVALID_SET);
              }
            /* assumption is that the buffer contains a DMObjHandle_t
             * and we need to fill up the class_h and object_h
             */
            oh = (DMObjHandle_h) attrBuf;
            DM_OH_COPY(*oh, *(DMObjHandle_h)instBuf);
			STREAM_OUT(&(oh->classId), instBuf, sizeof(oh->classId));
			STREAM_OUT(&(oh->instanceId), instBuf, sizeof(oh->instanceId)); 
            CL_FUNC_EXIT();
            return CL_OK;
          } 
      }
    else if(idx != 0)
      {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Attribute {%x} idx [%d] [idx not allowed here]", 
                              this->attrId,
                              idx));

        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
      }
    
    /* copy the contents as specified 
     * if flag = 0, then its basically a 'Set'
     *    else its basically a 'Get'
     */
    clLog(CL_LOG_SEV_TRACE, "DMO","DBC", "%s(Attr:%x, sz: %d) ", 
                          flag==1?"GET":"SET",
                          this->attrId, 
                          sz);
    
    if(flag == 1)   /* Get Operation */
      {
          memcpy(attrBuf, instBuf, sz); 
      } 
    else           /* Set Operation */
      {
          memcpy(instBuf, attrBuf, sz); 
      }
    *size = sz;
    
    CL_FUNC_EXIT(); 
    return (CL_OK); 
}


/** 
 * Unpack the attribute.
 *
 * API to dmClassAttrUnpack <Deailed desc>. 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClInt32T
 *
 *  @todo      
 *
 */
ClInt32T
dmClassAttrUnpack(CORAttr_h this, 
                  void *    src
                  )
{
    Byte_h buf = src;

    CL_FUNC_ENTER();
    
    if((!this) || (!src))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    /* STREAM_OUT(&this->attrId, buf, sizeof(this->attrId));   */
	STREAM_OUT_NTOHL(&this->attrId, buf, sizeof(this->attrId));

    /* todo: following two needs to be seen, its again structures, so
     * have to be streamed as individual members
     *  --> unions should not be streamed as is!!!
     */
    STREAM_OUT_NTOHL(&this->attrType.type, buf, sizeof(this->attrType.type));
    STREAM_OUT_NTOHL((ClUint32T *)&this->attrType.u, buf, sizeof(this->attrType.u));
    STREAM_OUT(&this->attrValue.init, buf, sizeof(this->attrValue.init));
    STREAM_OUT(&this->attrValue.min, buf, sizeof(this->attrValue.min));
    STREAM_OUT(&this->attrValue.max, buf, sizeof(this->attrValue.max));
	
    this->attrValue.init = CL_BIT_N2H64(this->attrValue.init);
    this->attrValue.max  = CL_BIT_N2H64(this->attrValue.max);
    this->attrValue.min  = CL_BIT_N2H64(this->attrValue.min);
   
    STREAM_OUT_NTOHL(&this->offset, buf, sizeof(this->offset));
    STREAM_OUT_NTOHS(&this->index, buf, sizeof(this->index));
    STREAM_OUT_NTOHL(&this->userFlags, buf, sizeof(this->userFlags));
    
    CL_FUNC_EXIT();
    return (buf-(Byte_h)src);
}


/** 
 * Pack the attribute definition. 
 *
 * API to dmClassAttrPack <Deailed desc>. 
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
/**
 * 
 */
ClRcT
dmClassAttrPack(CORAttr_h this, 
                ClBufferHandleT *    dst
                )
{
    ClRcT           rc = CL_OK;
    ClBufferHandleT *pBufH = dst;
	ClUint32T tmpPackL = 0;
	ClUint16T tmpPackS = 0;
	ClInt64T init, min, max;
  
    if((!this) || (!dst))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

    STREAM_IN_BOUNDS_HTONL(pBufH, &this->attrId, tmpPackL, sizeof(this->attrId)); 

    STREAM_IN_BOUNDS_HTONL(pBufH, &this->attrType.type, tmpPackL, sizeof(this->attrType.type));
    STREAM_IN_BOUNDS_HTONL(pBufH, (ClUint32T *)&this->attrType.u, tmpPackL, sizeof(this->attrType.u));

	/* 
	 * Have to write the 64 bit values. So doing the marshalling saperately and writing
	 * the data in network format.
	 */
	init = this->attrValue.init;
	min = this->attrValue.min; 
	max = this->attrValue.max;
	
    init = CL_BIT_H2N64(init);
    min  = CL_BIT_H2N64(min);
    max  = CL_BIT_H2N64(max);

    STREAM_IN_BUFFER(pBufH, &init, sizeof(this->attrValue.init));
    STREAM_IN_BUFFER(pBufH, &min, sizeof(this->attrValue.min));
    STREAM_IN_BUFFER(pBufH, &max, sizeof(this->attrValue.max));
	
    STREAM_IN_BOUNDS_HTONL(pBufH, &this->offset, tmpPackL, sizeof(this->offset));
	STREAM_IN_BOUNDS_HTONS(pBufH, &this->index, tmpPackS, sizeof(this->index));
    STREAM_IN_BOUNDS_HTONL(pBufH, &this->userFlags, tmpPackL, sizeof(this->userFlags));
 
    CL_FUNC_EXIT();
    return rc ;
}

/** 
 * Show given Attribute type.
 *
 * API to 
 *
 *  @param 
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
dmClassAttrShow(ClUint32T idx, 
                void *     element, 
                void **    pCookie
                )
{
    CORAttr_h tmp = (CORAttr_h) element;
    CORClassShowCookie_t cookie = *(CORClassShowCookie_t*)pCookie;
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    ClBufferHandleT *pMsg= cookie.pMsg;

    CORClass_h    pCls = cookie.classHdl;
    char         attrName[CL_COR_MAX_NAME_SZ] ={'\0'};
    static const char* typeName[]= {
      "void",
      "signedINT8",
      "unsignedINT8",
      "signedINT16",
      "unsignedINT16",
      "signedINT32",
      "unsignedINT32",
      "signedINT64",
      "unsignedINT64",
      "float",
      "double",
      "counter32",
      "counter64",
      "sequence32",
      "invalid",
      "Array",
      "Containment",
      "Association",
      "virtual",
      0
    };

    CL_FUNC_ENTER();
    if(!tmp || 
       (tmp->attrId==0 && 
        tmp->attrType.type==0 && 
        tmp->offset==0 &&
        tmp->attrValue.min==0 &&
        tmp->attrValue.max==0))
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    _corNIAttrNameGet(pCls->classId, tmp->attrId, attrName); 
    corStr[0]='\0';
    sprintf(corStr, "  [%s]",attrName);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

    corStr[0]='\0';
    sprintf(corStr, " 0x%x, %s ",
             tmp->attrId,
             typeName[tmp->attrType.type]
             );
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    
    if(tmp->attrType.type == CL_COR_ARRAY_ATTR) 
      {
        corStr[0]='\0';
        sprintf(corStr, "(%s) [%d elements]",typeName[tmp->attrType.u.arrType],
               (ClUint32T)tmp->attrValue.min);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
      } 
    else if(tmp->attrType.type == CL_COR_CONTAINMENT_ATTR) 
      {
        corStr[0]='\0';
        sprintf(corStr, "(0x%04x) [%d elements]",
                 tmp->attrType.u.remClassId,
                 (ClUint32T)tmp->attrValue.max);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
      } 
    else if(tmp->attrType.type == CL_COR_ASSOCIATION_ATTR) 
      {
        corStr[0]='\0';
        sprintf(corStr, "(0x%04x) [%d references]",
                 tmp->attrType.u.remClassId,
                 (ClUint32T)tmp->attrValue.max);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
      } 
    else 
      {
        corStr[0]='\0';
        sprintf(corStr, " = {%llx} / Range:<%llx,%llx> /",
                 (ClInt64T)tmp->attrValue.init,
                 (ClInt64T)tmp->attrValue.min,
                 (ClInt64T)tmp->attrValue.max);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
      }
    corStr[0]='\0';
    sprintf(corStr, " <%d>  Index:: <%d> \t", tmp->offset,
             tmp->index);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    corStr[0]='\0';
    if(tmp->userFlags & CL_COR_ATTR_CONFIG)
        sprintf(corStr, "   CF");
    else if(tmp->userFlags & CL_COR_ATTR_RUNTIME)
        sprintf(corStr, "   RT");
    else if(tmp->userFlags & CL_COR_ATTR_OPERATIONAL)
        sprintf(corStr, "   OP");
    
    if(tmp->userFlags & CL_COR_ATTR_INITIALIZED)
        strcat(corStr," / INITED");
    else
        strcat(corStr," / N-INITED");

    if(tmp->userFlags & CL_COR_ATTR_WRITABLE)
        strcat(corStr," / WRBLE");
    else    
        strcat(corStr," / N-WRBLE");

    if(tmp->userFlags & CL_COR_ATTR_CACHED)
        strcat(corStr," /   C$");
    else
        strcat(corStr," / N-C$");

    if(tmp->userFlags & CL_COR_ATTR_PERSISTENT)
        strcat(corStr," / PERS");
    else 
        strcat(corStr," / N-PERS");
        
    strcat(corStr,"\n");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    CL_FUNC_EXIT();
    return 0;
}


ClRcT clCorPrintAttr2Buffer(ClBufferHandleT  msgBufHdl,CORAttr_h attrH,ClUint8T *data,ClBoolT swapEndian)
{
    ClCorTypeT              type        = attrH->attrType.type;
    ClCharT                 corStr[CL_COR_CLI_STR_LEN]  = {0};
    ClUint32T               nElements   = 1, sz = 0;
    
    if(type == (ClInt32T)CL_COR_ARRAY_ATTR)
    {
        sprintf(corStr, "[Array nElements:%-3lld] [", attrH->attrValue.min * 
                (attrH->attrValue.max > 0 ? attrH->attrValue.max : 1));
        clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr,
                            strlen(corStr));
    }
    else
    {
        sprintf(corStr, "[");
        clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    }

    if (1)
    {
        if (type==(ClInt32T)CL_COR_ARRAY_ATTR)
        {
            type = attrH->attrType.u.arrType;
            nElements = attrH->attrValue.min * (attrH->attrValue.max > 0 ? attrH->attrValue.max : 1);
        }
   
        if ((nElements>1)&&((type == CL_COR_INT8)||(type==CL_COR_UINT8)))  /* Guess that it might be a string */
        {
            int i;
            int isstring = 1;
            int zeroat = 0;
            for (i=0;i<nElements;i++)
            {
                ClCharT c = ((ClCharT*)data)[i];
            
                if (!((c == 0) || isprint(c)))
                {
                    isstring=0;
                    break;
                }
                if (c==0) { zeroat = i; break; }  /* Find the length for the copy */

            }
            if (isstring)
            {
                clBufferNBytesWrite(msgBufHdl,(ClUint8T *)data,zeroat);
                clBufferNBytesWrite(msgBufHdl,(ClUint8T *)"] [",3);
            }
        
        }
        
        while(nElements) 
        {
            corStr[0]=0; /* Clear corStr just in case no case is taken because at the end we will write it to the buffer */
            switch((ClInt32T)type) 
            {
                /* Primitive types */
                case CL_COR_INT8:
                {
                    sprintf(corStr, "%d",*((ClInt8T *)data));
                    sz = 1;
                    
                } break;
                case CL_COR_UINT8:
                {
                    sprintf(corStr, "%d",*((ClUint8T *)data));
                    sz = 1;
                    break;
                }
                case CL_COR_INT16:
                {
                    ClInt16T var = *((ClInt16T*)data);
                    if (swapEndian) var = CL_BIT_N2H16(var);
                    sprintf(corStr, "%d", var);
                    sz = 2;
                    
                } break;
                case CL_COR_UINT16:
                {
                    ClUint16T var = *((ClUint16T*)data);
                    if (swapEndian) var = CL_BIT_N2H16(var);
                    sprintf(corStr, "%u", var);
                    sz = 2;
                    
                } break;
                case CL_COR_INT32:
                {
                    ClInt32T var = *((ClInt32T*)data);
                    if (swapEndian) var = CL_BIT_N2H32(var);
                    sprintf(corStr, "%d", var);
                    sz = 4;
                    
                } break;
                case CL_COR_UINT32:
                {
                    ClUint32T var = *((ClUint32T*)data);
                    if (swapEndian) var = CL_BIT_N2H32(var);
                    sprintf(corStr, "%u", var);
                    sz = 4;
                } break;
                case CL_COR_INT64:
                {
                    ClInt64T var = *((ClInt64T*)data);
                    if (swapEndian) var = CL_BIT_N2H64(var);
                    sprintf(corStr, "%lld", var);
                    sz = 8;
                    
                } break;
                case CL_COR_UINT64:
                {
                    ClUint64T var = *((ClUint64T*)data);
                    if (swapEndian) var = CL_BIT_N2H64(var);
                    sprintf(corStr, "%llu", var);
                    sz = 8;
                    
                } break;
            /* Complex data types */
            case CL_COR_ASSOCIATION_ATTR:
              {
                DMObjHandle_h   oh_handle                   = NULL;
                ClInt32T        items                       = 0;
                ClInt32T        i                           = 0;
                sprintf(corStr, "  [Assoc Class:0x%04x   ]", attrH->attrType.u.remClassId);
                clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));

                items = attrH->attrValue.max;
                sz = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
                sz /= items;
        
                sprintf(corStr, "  =  [");
                clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));
                for(i = 0; i < items; *data+=sz) 
                  {
                    corStr[0]='\0';
                    oh_handle = *((DMObjHandle_h*)data);

                    sprintf(corStr, "0x%02x",i);
                    clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));
                    i++;
                    if(i < items)
                    {
                        sprintf(corStr,", ");
                        clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));
                    }
                    DM_OH_SHOW_VERBOSE(*oh_handle);
                  }
                sprintf(corStr, "]");
                clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr,
                            strlen(corStr));
                sz=0; /* Already added to data */
              } break;
            case CL_COR_CONTAINMENT_ATTR:
                {
                    sz = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
                
                    corStr[0] = '\0';
                    sprintf(corStr, "  [Contained Class: 0x%04x] ", attrH->attrType.u.remClassId);
                    clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));
                    sprintf(corStr, " [nElements: %3d ", (ClUint32T)attrH->attrValue.max);
                } break;
                
            default:
                {
                    sprintf(corStr,"Invalid attr type ");
                } break;
                
            }

            if (sz) clBufferNBytesWrite (msgBufHdl, (ClUint8T*)corStr, strlen(corStr));

            data += sz;
            nElements--;

            /* Write the separator in */
            if(nElements)
                clBufferNBytesWrite(msgBufHdl, (ClUint8T *)", ", 2);
        }
        clBufferNBytesWrite(msgBufHdl, (ClUint8T *) "]", 1);  
    }
    return CL_OK;
}

/**
 *  Function to get the information of the runtime attribute and display it as part of 
 *  object show CLI output.
 */

ClRcT clCorRuntimeAttrGet(ClCorObjectHandleT objHandle, ClCorAttrPathPtrT pAttrPath, CORAttr_h attrH,char** data)
{   
    ClRcT                           rc              = CL_OK;
    ClRcT                           jobrc              = CL_OK;
    ClCorBundleHandleT              bundleHandle    =  0;
    ClInt32T                        size            = 0;
    ClCorAttrValueDescriptorT       attrDesc        = {0};
    ClCorAttrValueDescriptorListT   attrList        = {0};
    ClCorBundleConfigT              bundleConfig    = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClBoolT                         meFree          = CL_FALSE;
    
    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig); 
    if(CL_OK != rc) 
    {
        return rc;
    }

    size  = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
    if(size < 0)
    {
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
    }

    clDbgCheckNull(data,CL_CID_COR);
    if (*data==NULL)
    {        
      *data = clHeapAllocate(size);
      meFree= CL_TRUE;    
      clDbgCheckNull(*data,CL_CID_COR);
    }
    
    attrDesc.pAttrPath = pAttrPath;
    attrDesc.attrId = attrH->attrId;
    attrDesc.index = CL_COR_INVALID_ATTR_IDX;
    attrDesc.bufferSize = size;
    attrDesc.bufferPtr = *data;
    attrDesc.pJobStatus = &jobrc;
    
    attrList.numOfDescriptor = 1;
    attrList.pAttrDescriptor = &attrDesc;
    
    rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
    
    if (rc == CL_OK) rc = clCorBundleApplySync(bundleHandle);

    if(CL_OK != rc) /* If there's an error delete the buffer */
    {
        if (meFree)
        {
            clHeapFree(*data);
            *data=NULL;
        }
        return rc;
    }
    
    clCorBundleFinalize(bundleHandle);
    /* Return the data buffer.  The user is responsible for deleting it */
    return jobrc;
}

/** 
 * Show given Attribute value.
 * 
 * Show the attribute value.
 *
 *  @param ms      Type definition for the attribute
 *  @param buf     Buffer pointing to the attribute value
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 * 
 */
ClRcT
dmObjectAttrShow(CORAttr_h this, Byte_h*   cookie)
{
    ClCorTypeT      typ                         = 0;
    CORClass_h      clsHdl                      = NULL;
    char            attrName[80]                = {0};
    ClCharT         corStr[CL_COR_CLI_STR_LEN]  = {0};
    ClRcT           rc                          = CL_OK;
    ClBoolT         attrRuntime = CL_FALSE;
    
    if(!this || !cookie)
    {
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    struct classObjBufMsgTrio *tmpTrio = (struct classObjBufMsgTrio *) cookie;

    /* Add the attr's offset to the object buffer. */
    Byte_h buf = (tmpTrio->buf + this->offset);
    ClBufferHandleT msgHdl =  tmpTrio->pMsg;
   
    typ    = this->attrType.type;
    clsHdl = tmpTrio->type;

    clLogDebug("CLI", "ATTRSHOW", "AttrShow: classId [%d], attrId [%d]", 
            clsHdl->classId, this->attrId);

    /* The attribute might be a inherited attribute, so try getting name from parent class */
    while(_corNIAttrNameGet(clsHdl->classId, this->attrId, attrName)!= CL_OK && clsHdl->superClassId != 0)
    {
       clsHdl = dmClassGet(clsHdl->superClassId);       
    }

    sprintf(corStr, "[%-30s] [0x%04x] = ", attrName, this->attrId);
    clBufferNBytesWrite (msgHdl, (ClUint8T*)corStr, strlen(corStr));
    
    /* For runtime attribute or not a read-only runtime attribute we need to go get it */    
    if(((this->userFlags & CL_COR_ATTR_RUNTIME) && (!(this->userFlags & CL_COR_ATTR_CACHED)))
            || ((this->userFlags & CL_COR_ATTR_OPERATIONAL)))
    {
        /* Get a new buffer from RuntimeAttrGet to hold the data */
        buf = NULL;
        attrRuntime = CL_TRUE;        
        rc = clCorRuntimeAttrGet(tmpTrio->objHandle, tmpTrio->pDmContHandle->pAttrPath, this, (char**) &buf);
    }

    if (rc == CL_OK)
    {
        /* No need to swap the buf if it is runtime get. */
        if (attrRuntime)
            clCorPrintAttr2Buffer(msgHdl, this, buf, CL_FALSE);
        else
            clCorPrintAttr2Buffer(msgHdl, this, buf, CL_TRUE);
    }
    else
    {
        ClCharT                 corStr[CL_COR_CLI_STR_LEN]  = {0};
        sprintf(corStr,"Access error: 0x%x", rc);
        clBufferNBytesWrite(msgHdl, (ClUint8T *)corStr, strlen(corStr));

        /* Continue with the next attribute. */
        rc = CL_OK;
    }
    
    /* End this attribute by ending the line */
    clBufferNBytesWrite(msgHdl, (ClUint8T *)"\n", strlen("\n"));
    if (attrRuntime && buf) clHeapFree(buf);

    return (rc);
}

/** 
 * DM Object Attribute Index Set
 * 
 * This function shall be called from attribute walk, to set
 * index for a perticular attribute
 *
 *  @param this      COR attribute Handle
 *  @param buf     user passed buffer (Used to pass indexes)
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 * 
 */
ClRcT
dmObjectAttrIndexSet(CORAttr_h this, 
                 Byte_h*   buf)
{
    CL_FUNC_ENTER();


    if(!this || !buf)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

	ClUint16T* index = (ClUint16T*)(*buf);

	this->index = *index;

	*index = *index + 1;

	CL_FUNC_EXIT();
	return CL_OK;
	
}

/** 
 * DM Object Attribute ID Get
 * 
 * Get the Attribute Id given Attribute Index.
 *
 * This function shall be called from Attribute Walk. This
 * will find out the attribute Id given an index.
 *
 *  @param this      COR attribute Handle
 *  @param buf     user passed buffer (Used to pass indexes)
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 * 
 */
ClRcT
dmObjectAttrIdGet(CORAttr_h this, 
                 Byte_h*   buf)
{
	typedef struct IdIdxStruct
		{
		ClCorAttrIdT attId;
		ClUint16T Index;
		} IdIdxStruct_t;

	IdIdxStruct_t* param;

    	if(!this || !buf)
      		{
        		CL_FUNC_EXIT();
        		return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      		}

	param = (IdIdxStruct_t*)(*buf);

	if(this->index == param->Index)
		{
		/* Found the index. Return Error to stop the walk*/
		param->attId = this->attrId;
		return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_PRESENT);
		}

return CL_OK;
}

ClRcT
_dmAttrRangeValidate(CORAttrInfo_t attrType, 
                    ClInt64T newVal, 
                    ClInt64T min, 
                    ClInt64T max)
{
    ClRcT ret = CL_OK;
    int dataType = UNKNOWN_TYPE;
    ClInt64T  minLimit = 0;
    ClInt64T   maxLimit = 0;

    CL_FUNC_ENTER();


    switch((ClInt32T)attrType.type) 
    {
        /* Primitive types */ 
        case CL_COR_INT8: 
            { 
                dataType = SIGNED_TYPE;
                minLimit = 0xFFFFFFFFFFFFFF80LL; /* 2's complement of -128 */
                maxLimit = 0x0000007F;
            }
            break;
        
        case CL_COR_UINT8: 
            { 
                dataType = UNSIGNED_TYPE;
                minLimit = 0x0;
                maxLimit = 0x000000FF;
            } 
            break;

        case CL_COR_INT16: 
            { 
                dataType = SIGNED_TYPE;
                minLimit = 0xFFFFFFFFFFFF8000LL;
                maxLimit = 0x00007FFF;
            } 
            break;

        case CL_COR_UINT16: 
            { 
                dataType = UNSIGNED_TYPE;
                minLimit = 0x0;
                maxLimit = 0x0000FFFF;
            } 
            break;

        case CL_COR_INT32:
            {
                dataType = SIGNED_TYPE;
                minLimit = 0xFFFFFFFF80000000LL;
                maxLimit = 0x7FFFFFFF;
            }
            break;

        case CL_COR_UINT32: 
            {
                dataType = UNSIGNED_TYPE;
                minLimit = 0x0;
                maxLimit = 0xFFFFFFFF;
				max = (ClUint32T) max;
				min = (ClUint32T) min;
				newVal = (ClUint32T)newVal;
            }
            break;

        case CL_COR_INT64: 
        {
            dataType = SIGNED_TYPE;
            minLimit = 0x8000000000000000LL;
            maxLimit = 0x7FFFFFFFFFFFFFFFLL;
            break;
       	}
        case CL_COR_UINT64 :
        {
            dataType = UNSIGNED_TYPE;
            minLimit = 0x0;
            maxLimit = 0xFFFFFFFFFFFFFFFFLL;
            break;
        }
        case CL_COR_ARRAY_ATTR:
        {
            minLimit = 0x0;
            if ( (min >= minLimit) && ((max == -1) || (max > 0)))
            {
                ClInt64T maxIndexLimit = (max == -1) ? (min) : (min*max);
                if ( (newVal >=0 ) && (newVal < maxIndexLimit) )
                {
                    return (CL_OK);
                }
                return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE));
            }
            return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE));
        }

        /* complex data types */
        case CL_COR_CONTAINMENT_ATTR:
        case CL_COR_ASSOCIATION_ATTR:
        {
                dataType = SIGNED_TYPE;
                minLimit = 0x0;
                maxLimit = 0x7FFFFFFFFFFFFFFFLL;
          /* probably just validate the index ? */
          /* todo: need to see if association handle can be validated */
        }
        break;

        default:
        {
              dataType = SIGNED_TYPE;
        }
        break;    
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Validate value:%lld [Min:%lld,Max:%lld]", 
                          newVal, 
                          min, 
                          max));

    if (SIGNED_TYPE == dataType)
    {
        if( (minLimit    <= min) &&
            (maxLimit    >= max) &&
            (max         >= min) )
        {
            if ( (newVal   >= min) &&
                 (newVal   <= max) ) 
            {
                ret = CL_OK;
            }
            else
            {
                ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE);
            }
        }
        else
        {
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_VAL);
        }
    }
    else if (UNSIGNED_TYPE == dataType)
    {
        if ( ((ClUint64T)minLimit <= (ClUint64T)min) && 
             ((ClUint64T)maxLimit >= (ClUint64T)max) && 
             ((ClUint64T)max >= (ClUint64T)min) )
        {
            if (((ClUint64T)newVal >= (ClUint64T)min) && 
                ((ClUint64T)newVal <= (ClUint64T)max)) 
            { 
                ret = CL_OK;
            }
            else
            {
                ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE);
            }
        }
        else
        {
            ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_VAL);
        }
    }
    else 
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);


    CL_FUNC_EXIT();
    return ret;
}


#if 0
ClRcT
corAttrWalkHdler(CORHashKey_h   key, 
                 CORHashValue_h pData,
                 void *         userData,
                 ClUint32T     dataLength)
{
    CORAttr_h            attrHdl = NULL;
    CORClassWalkInfo_t   *pWalkInfo = NULL;
    ClRcT               rc = CL_OK;

    /* Validations.*/
    if ((pData == 0) || (userData == NULL))
    {
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    attrHdl   = (CORAttr_h) pData;
    pWalkInfo =(CORClassWalkInfo_t *)userData;
    
    /* Call the user callback*/
    rc = pWalkInfo->walkFp(attrHdl->attrId,
                           pWalkInfo->clsHdl,
                           pWalkInfo->cookie);
    return rc;
        
}
/* ------------- Attribute related util APIs ------------------ */
/** 
 *     Walk thru all the attribute of a class
 * 
 *  @param clsHdl    Handle of the class which contains this attrId
 *  @param usrClBck  User callback 
 *  @param cookie    Optional user data
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 * 
 */

ClRcT  clCorClassAttributeWalk(CORClass_h         clsHdl,  
                        ClCorClassAttrWalkFunc  usrClBk, 
                        ClHandleT           cookie)
{
    ClRcT               rc = CL_OK;
    CORClassWalkInfo_t   walkInfo;

    CL_FUNC_ENTER();
    if ((clsHdl == NULL) || (usrClBk == NULL))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    memset(&walkInfo, '\0', sizeof(CORClassWalkInfo_t));
    walkInfo.cookie = cookie;
    walkInfo.walkFp = usrClBk;
    walkInfo.clsHdl = clsHdl;
    HASH_ITR_ARG(clsHdl->attrList, corAttrWalkHdler, (void *) &walkInfo, sizeof(walkInfo.cookie));
    return rc;
}

/** 
 *    API to get the type of an attribute. (ASSOC/ARRAY/SIMPLE....) 
 * 
 *  @param clsHdl    Handle of the class which contains this attrId
 *  @param attrId    attribute ID 
 *  @param pAttrType Pointer to the type of attribute 
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *     CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT) if the attribute is not present
 *
 * 
 */
ClRcT  clCorClassAttributeTypeGet(CORClass_h clsHdl, ClCorAttrIdT attrId,  ClCorAttrTypeT *pAttrType)
{
    ClRcT      rc = CL_OK;
    CORAttr_h   attrHdl = NULL;

    if ((clsHdl == NULL) || (pAttrType == NULL))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    HASH_GET(clsHdl->attrList, attrId, attrHdl);
    if (attrHdl == NULL)
    {
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    }
    *pAttrType = attrHdl->attrType.type;
    return rc;
}
/** 
 *    API to get the minimum, maximum and default values of a given attribute 
 * 
 *  @param clsHdl    Handle of the class which contains this attrId
 *  @param attrId    attribute ID 
 *  @param pInit     Pointer to an integer which will be filled with minimum value.
 *  @param pMin      Pointer to an integer which will be filled with minimum value.
 *  @param pMax      Pointer to an integer which will be filled with minimum value.
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *     CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT) if the attribute is not present
 * 
 */
ClRcT  clCorClassAttributeValuesGet(CORClass_h     clsHdl, 
                             ClCorAttrIdT    attrId,
                             ClInt32T      *pInit,
                             ClInt32T      *pMin,
                             ClInt32T      *pMax)
{
    ClRcT  rc = CL_OK;
    CORAttr_h   attrHdl = NULL;

    if ((clsHdl == NULL) || (pInit == NULL) ||
        (pMin == NULL) || (pMax == NULL))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    HASH_GET(clsHdl->attrList, attrId, attrHdl);
    if (attrHdl == NULL)
    {
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    }
    *pInit = attrHdl->attrValue.init;
    *pMin  = attrHdl->attrValue.min;
    *pMax  = attrHdl->attrValue.max;
    return rc;
}
#endif

ClRcT dmClassAttrCreate(CORClass_h   classHdl, corAttrIntf_t  *pAttr)
{
    ClRcT        rc = CL_OK;
    CORAttrInfo_t attr = {pAttr->attrType, {0}} ; 
    CORAttr_h     attrHdl = NULL;

    /* The following things need to be done for attr create
       If Containment/Assoc
          Check for the related class existence
       If Containment/assoc/arry initialise the attr values
     */
    if (((ClInt32T)CL_COR_CONTAINMENT_ATTR == pAttr->attrType) ||
        ((ClInt32T)CL_COR_ASSOCIATION_ATTR== pAttr->attrType))
    {
        if (NULL == dmClassGet(pAttr->subClassId))
            return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT) ;
        attr.u.remClassId = pAttr->subClassId;
            
    }
    else if ((ClInt32T)CL_COR_ARRAY_ATTR == pAttr->attrType)
        attr.u.arrType = pAttr->subAttrType;

    if (CL_OK != (rc = dmClassAttrAdd(classHdl, pAttr->attrId, attr, &attrHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attr create failed [rc 0x%x]",rc));
        return rc;
    }

    if (((ClInt32T)CL_COR_ASSOCIATION_ATTR== pAttr->attrType)  ||
        ((ClInt32T)CL_COR_ARRAY_ATTR == pAttr->attrType))
    {
        rc = dmClassAttrInit(attrHdl, 0, pAttr->min, pAttr->max);
    }
    else if ((ClInt32T)CL_COR_CONTAINMENT_ATTR == pAttr->attrType)
    {
        rc = dmClassAttrInit(attrHdl, pAttr->min, pAttr->min, pAttr->max);
    }
    return rc;
}



/*
 *  Function to compare the attribute value.
 *  returns CL_OK
 *     result is updated with     CL_TRUE, if comparison passes  
 *                                CL_FALSE, if comparison fails.
 */

ClRcT clCorDmAttrValCompare(CORAttr_h attrH,
                             ClCorDmObjPackAttrWalkerT *pBuf,      /* object reference*/
                              ClCorAttrIdT attrId,  /* attribute to be compared*/
                               ClInt32T index,      /* index for Array/Assoc/containment */
                               void   * value,      /* comparision value */
                               ClUint32T  size,     /* size of value */ 
                               ClCorAttrCmpFlagT cmpFlag)
{
    ClRcT      ret = CL_OK;
    ClRcT      attrType = attrH->attrType.type;
    ClUint8T   dataType;
    ClUint32T  attrSize = 0;
    ClUint8T   *attrBuf = NULL, *tempBuf = NULL;
    ClInt64T   userVal = 0;  
    ClInt64T   corVal  = 0;  
    ClUint8T   *objBuf = pBuf->objInstH;
    ClInt8T    *result = &pBuf->attrCmpResult;
    ClUint8T   srcArch = pBuf->srcArch;

    /* initialize with FALSE*/  
    *result = CL_FALSE;

    clLogDebug("OBW", "CMP", "Inside the function to compare the value ");

    /* This contains the total attribute size (including no. of elements * sizeof one element) */
    attrSize = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

    if(attrType != CL_COR_ARRAY_ATTR &&
            attrType != CL_COR_ASSOCIATION_ATTR &&
            attrType != CL_COR_CONTAINMENT_ATTR)
    {
        /* if it is a simple attribute */
        if(attrSize > size)
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Insufficient buffer specified"));
            return(CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER));
        }
        else if(srcArch == CL_BIT_BIG_ENDIAN)
        { 
            /* move the attrBuf pointer to actual position.*/
            value = (ClUint8T *)value + (size - attrSize);
        }
    }

    /* if invalid index is specified for a array attribute, then set the index to 0 */
    if ((attrType == CL_COR_ARRAY_ATTR) && (index == CL_COR_INVALID_ATTR_IDX))
    {
        index = 0;
    }

    if( ((attrType < CL_COR_SIMPLE_ATTR) &&  (index != CL_COR_INVALID_ATTR_IDX))  /* simple attribute with valid index. */
            || 
            ( ( index == CL_COR_INVALID_ATTR_IDX )       &&        /* containment or association attribute with invalid index.*/ 
              ( attrType == CL_COR_CONTAINMENT_ATTR || attrType == CL_COR_ASSOCIATION_ATTR) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid parameter specified."));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    if ((attrH->userFlags & CL_COR_ATTR_RUNTIME) && 
            (! (attrH->userFlags & CL_COR_ATTR_CACHED)))
    {
        clLogDebug("OBW", "CMP", "The attribute is runtime non-cached type");
        ret = _clCorDmAttrRtAttrGet (pBuf, attrH, (ClPtrT *)&attrBuf);
        if (CL_OK != ret)
        {
            clLogError("OAW", "CMP", "Failed to get the latest value from the OI. rc[0x%x]", ret);
            goto exitError;
        }

        tempBuf = attrBuf;
    }
    else
    {
        attrBuf  = (ClUint8T *)objBuf + sizeof(CORInstanceHdr_t);
        attrBuf  = (ClUint8T *)attrBuf + attrH->offset;
    }

    clLogDebug ("OBW", "CMP", "For attrId [0x%x]: the length [%d] and offset [%d] ",
            attrH->attrId, attrSize, attrH->offset);


    switch (attrType)
    {
        case CL_COR_ARRAY_ATTR:
            {
                ClUint32T sz = 0;
                /* kludge */
                switch ((ClInt32T)attrH->attrType.u.arrType)
                {
                    case CL_COR_INT8:
                    case CL_COR_UINT8:
                        sz = sizeof(ClUint8T);
                        break;

                    case CL_COR_INT16:
                    case CL_COR_UINT16:
                        sz = sizeof(ClUint16T);
                        break;

                    case CL_COR_INT64:
                    case CL_COR_UINT64:
                        sz = sizeof(ClUint64T);
                        break;

                    default:                       
                        sz = sizeof(ClUint32T);
                }

                /** Validate index **/
                if(index >= attrH->attrValue.min || index < 0) 
                {
                    CL_FUNC_EXIT(); 
                    clLogError("OAW", "CMP", "For attrId [0x%x]: Invalid attr index [%d] specified.",
                            attrH->attrId, index);
                    ret = (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX));
                    goto exitError;
                }

                /* Checking whether the size specified by the user is greater than the element size
                   supposed to be taken for the corresponding index */
                if(size > (attrSize - (index * sz)))
                {
                    CL_FUNC_EXIT(); 
                    clLogError("OAW", "CMP", 
                            "For attrId [0x%x]: Buffer overrun. Invalid size [%d] specified wrt index [%d].", 
                            attrH->attrId, size, index);
                    ret = (CL_COR_SET_RC(CL_COR_ERR_BUFFER_OVERRUN));
                    goto exitError;
                }

                /** move the attrBuf to actual index   **/
                attrBuf += (sz*index);
                /* comparison for single element of array. */
                /* Assign the type to attrType so that this same switch statement can be used to compare
                   the single element */
                if(size == sz)
                {
                    attrType = attrH->attrType.u.arrType;                    
                }

                /* Bulk compare */
                else 
                {
                    ClUint8T* userBuf = NULL;
                    ClUint8T* dmBuf = NULL;

                    /* convert the array of values to host order 
                       and convert the values stored in the DM to host order */

                    /* TODO: Since modifying the actual values, have to copy this to another variable */

                    userBuf = (ClUint8T *) clHeapAllocate(size);
                    if (userBuf == NULL)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                        CL_FUNC_EXIT();
                        ret =  (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto exitError;
                    }
                    memset(userBuf, 0, size);

                    dmBuf = (ClUint8T *) clHeapAllocate(size);
                    if (dmBuf == NULL)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                        clHeapFree(userBuf);
                        CL_FUNC_EXIT();
                        ret = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto exitError;
                    }
                    memset(dmBuf, 0, size);

                    memcpy((void *) userBuf, (void *) value, size);
                    memcpy((void *) dmBuf, (void *) attrBuf, size);

                    if (srcArch != clByteEndian)
                    {
                        clLogDebug("OAW", "CMP", "Doing the byte swaping for user buffer. ");
                        ret = clCorObjAttrValSwap(userBuf, size, attrType, attrH->attrType.u.arrType);
                        if (ret != CL_OK)
                        {
                            clLogError("OAW", "CMP", 
                                    "Failed to do byte endian conversion on the "
                                     "user supplied value. rc [0x%x]", ret);
                            clHeapFree(userBuf);
                            clHeapFree(dmBuf);
                            goto exitError;;
                        }
                    }

                    /* if the host is a little endian, then convert the dm values to little endian */ 
                    if (clByteEndian != CL_BIT_BIG_ENDIAN) /* (clByteEndian is CL_BIT_LITTLE_ENDIAN) */
                    {
                        clLogDebug("OAW", "CMP", "Doing the byte swaping for dm buffer. ");
                        ret = clCorObjAttrValSwap(dmBuf, size, attrType, attrH->attrType.u.arrType);
                        if (ret != CL_OK)
                        {
                            clLogError("OAW", "CMP",  
                                    "Failed to do byte endian conversion on the DM buffer. rc [0x%x]", ret);
                            clHeapFree(userBuf);
                            clHeapFree(dmBuf);
                            goto exitError;
                        }
                    }

                    /* Now do memcmp based on the comparison flag specified */
                    switch(cmpFlag)
                    {
                        case  CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO:
                            {
                                if( memcmp(userBuf, dmBuf, size) == 0)
                                    *result = CL_TRUE;
                            }
                            break;

                        case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN:
                            {
                                if( memcmp(userBuf, dmBuf, size) > 0)
                                    *result = CL_TRUE;
                            }
                            break;

                        case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS:
                            {
                                if( memcmp(userBuf, dmBuf, size) >= 0)
                                    *result = CL_TRUE;
                            }
                            break;

                        case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN:
                            {
                                if( memcmp(userBuf, dmBuf, size) < 0)
                                    *result = CL_TRUE;
                            }
                            break;

                        case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS:
                            {
                                if( memcmp(userBuf, dmBuf, size) <= 0)
                                    *result = CL_TRUE;
                            }
                            break;

                        default:
                            /* do nothing, return mismatch (CL_FALSE) */
                            *result = CL_FALSE;
                    }

                    clHeapFree(userBuf);
                    clHeapFree(dmBuf);

                    CL_FUNC_EXIT(); 
                    ret = CL_OK;
                    goto exitError;
                }
            }

            /* Primitive types */ 
        case CL_COR_INT8: 
            /*
             * attrType might have been changed in CL_COR_ARRAY_ATTR case..
             * so this 'if' statement is to accomodate comparison of native data
             * type of array attribute. (for comparison of specific index of array.)
             */
            if(attrType == CL_COR_INT8)
            { 
                userVal = *(ClInt8T *)value;
                corVal  = *(ClInt8T *)attrBuf; 
                dataType = SIGNED_TYPE;
                break;
            }

        case CL_COR_UINT8: 
            if(attrType == CL_COR_UINT8)
            { 
                userVal = *(ClUint8T *)value;
                corVal  = *(ClUint8T *)attrBuf; 
                dataType = UNSIGNED_TYPE;
                break;
            } 

            /* Convert the user specified values to network order */
        case CL_COR_INT16: 
            if(attrType == CL_COR_INT16)
            { 
                userVal = (ClInt16T) CL_COR_CONV_TO_H16(srcArch, *(ClInt16T *)value); /* *(ClInt16T *) value */
                corVal  = (ClInt16T) CL_COR_CONV_TO_H16(CL_BIT_BIG_ENDIAN, *(ClInt16T *)attrBuf); /* CL_BIT_N2H16(*(ClInt16T *)attrBuf); */
                dataType = SIGNED_TYPE;
                break;
            } 

        case CL_COR_UINT16: 
            if(attrType == CL_COR_UINT16)
            { 
                userVal = (ClUint16T) CL_COR_CONV_TO_H16(srcArch, *(ClUint16T *)value);
                corVal  = (ClUint16T) CL_COR_CONV_TO_H16(CL_BIT_BIG_ENDIAN, *(ClUint16T *)attrBuf);
                dataType = UNSIGNED_TYPE;
                break;
            } 

        case CL_COR_INT32:
            if(attrType == CL_COR_INT32)
            {
                userVal = (ClInt32T) CL_COR_CONV_TO_H32(srcArch, *(ClInt32T *)value);
                corVal  = (ClInt32T) CL_COR_CONV_TO_H32(CL_BIT_BIG_ENDIAN, *(ClInt32T *)attrBuf);
                dataType = SIGNED_TYPE;
                break;
            }

        case CL_COR_UINT32: 
            if(attrType == CL_COR_UINT32)
            {
                userVal = (ClUint32T) CL_COR_CONV_TO_H32(srcArch, *(ClUint32T *)value);
                corVal  = (ClUint32T) CL_COR_CONV_TO_H32(CL_BIT_BIG_ENDIAN, *(ClUint32T *)attrBuf);
                dataType = UNSIGNED_TYPE;
                break;
            }

        case CL_COR_INT64:
            if(attrType == CL_COR_INT64)
            {
                userVal = (ClInt64T) CL_COR_CONV_TO_H64(srcArch, *(ClInt64T *)value);
                corVal  = (ClInt64T) CL_COR_CONV_TO_H64(CL_BIT_BIG_ENDIAN, *(ClInt64T *)attrBuf);
                dataType = SIGNED_TYPE;
                break;
            }

        case CL_COR_UINT64:
            if(attrType == CL_COR_UINT64)
            {
                userVal = (ClUint64T) CL_COR_CONV_TO_H64(srcArch, *(ClUint64T *)value);
                corVal  = (ClUint64T) CL_COR_CONV_TO_H64(CL_BIT_BIG_ENDIAN, *(ClUint64T *)attrBuf);
                dataType = UNSIGNED_TYPE;
                break;
            }

            /* complex data types */
        case CL_COR_CONTAINMENT_ATTR:
            {
                /* Comparing the containment attributes in not valid */

                clLogError("OAW", "CMP", 
                        "Invalid Attr ID specified. Cannot compare the contained attributes");
                CL_FUNC_EXIT(); 
                ret = CL_OK;
                goto exitError;
            }
            break;
        case CL_COR_ASSOCIATION_ATTR:
            {
                /* Comparing the association attributes is not valid */

                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Attr ID specified. Cannot compare the Association attributes."));
                CL_FUNC_EXIT(); 
                ret = CL_OK;
                goto exitError;
            }
            break;
        default:
            {
                CL_FUNC_EXIT(); 
                ret = CL_OK;
                goto exitError;
            }
            break;    
    }

    if (SIGNED_TYPE == dataType)
    {
        switch(cmpFlag)
        {
            case  CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO:
                {
                    if(userVal == corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN:
                {
                    if(userVal < corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS:
                {
                    if(userVal <= corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN:
                {
                    if(userVal > corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS:
                {
                    if(userVal >= corVal)
                        *result = CL_TRUE;
                }
                break;
            default:
                {
                    CL_FUNC_EXIT(); 
                    ret = (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
                    goto exitError;
                }
        }
    }
    else if(UNSIGNED_TYPE == dataType)
    {
        switch(cmpFlag)
        {
            case  CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO:
                {
                    if((ClUint64T)userVal == (ClUint64T)corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN:
                {
                    if((ClUint64T)userVal < (ClUint64T)corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS:
                {
                    if((ClUint64T)userVal <= (ClUint64T)corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN:
                {
                    if((ClUint64T)userVal > (ClUint64T)corVal)
                        *result = CL_TRUE;
                }
                break;
            case  CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS:
                {
                    if((ClUint64T)userVal >= (ClUint64T)corVal)
                        *result = CL_TRUE;
                }
                break;
            default:
                {
                    CL_FUNC_EXIT(); 
                    ret = (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
                    goto exitError;
                }
        }
    }
    else
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
        goto exitError;
    }   

exitError:
    clLogDebug ("OBW", "CMP", "For attrId [0x%x] : Cmp flag [%d], Result [%d]", 
            attrH->attrId, cmpFlag, *result);

    if ((attrH->userFlags & CL_COR_ATTR_RUNTIME) && 
            (! (attrH->userFlags & CL_COR_ATTR_CACHED)))
    {
        if (tempBuf != NULL)
            clHeapFree(tempBuf);
    }

    CL_FUNC_EXIT(); 
    return (ret);
}

/* 
 * This function converts a given type into network byte order and 
 * copy the result into the buffer "buf" passed.
*/
void * dmObjectAttrConvert(void *buf, ClInt64T value, CORAttrInfo_t attrType, ClUint32T size)
{
	ClCorTypeT typ = attrType.type;
	ClUint8T arr = 0;
computeType:
	switch((ClInt32T)typ)
	{
		case CL_COR_INT8:
			{
				ClInt8T val = value;
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_UINT8:
			{
				ClUint8T val = value;
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_INT16:
			{
				ClInt16T val = value;
				val = CL_BIT_H2N16(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_UINT16:
			{
				ClUint16T val = value;
				val = CL_BIT_H2N16(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_INT32:
			{
				ClInt32T val = value;
				val = CL_BIT_H2N32(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_UINT32:
			{
				ClUint32T val = value;
				val = CL_BIT_H2N32(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_INT64:
			{
				ClInt64T val = value;
                                val = CL_BIT_H2N64(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_UINT64:
			{
                                ClUint64T val = value;
                                val = CL_BIT_H2N64(val);
				return memcpy(buf, &val, size);
			}
			break;
		case CL_COR_ARRAY_ATTR:
			{
				if(!arr)
				{
					arr = 1;
					typ = attrType.u.arrType;
				}
				goto computeType;
			}
			break;
		default:
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Not a valid attr type"));
	}	
	return NULL;
}

ClRcT dmClassAttrListGet(ClCntKeyHandleT key,
                    ClCntDataHandleT data,
                    void* userArg,
                    ClUint32T userArgSize)
{
    VDECL_VER(ClCorAttrDefIDLT, 4, 1, 0)* pAttrDefIDL = NULL;
    ClCorAttrDefT attrDef = {0};
    CORAttr_h attrH = 0;

    if (!userArg)
    {
        clLogError("DM", "ATTRLIST", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pAttrDefIDL = (VDECL_VER(ClCorAttrDefIDLT, 4, 1, 0) *) userArg;
    attrH = (CORAttr_h) data;

    if (! ((attrH->userFlags & pAttrDefIDL->flags ) == pAttrDefIDL->flags))
    {
        return CL_OK;
    }

    if (! (pAttrDefIDL->numEntries & 3))
    {
        pAttrDefIDL->pAttrDef = clHeapRealloc(pAttrDefIDL->pAttrDef,
                                    (4 + pAttrDefIDL->numEntries) * sizeof(ClCorAttrDefT));
        if (!(pAttrDefIDL->pAttrDef))
        {
            clLogError("DM", "ATTRLIST", "Failed to allocate memory.");
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
    }

    /* Get the info from DM */
    attrDef.attrId = attrH->attrId;
    attrDef.attrType = attrH->attrType.type;

    switch (attrH->attrType.type)
    {
         case CL_COR_CONTAINMENT_ATTR:                         
         case CL_COR_ASSOCIATION_ATTR:
                attrDef.u.attrInfo.arrDataType = CL_COR_INVALID_DATA_TYPE;
                attrDef.u.attrInfo.maxElement = attrH->attrValue.max;
                attrDef.u.attrInfo.classId = attrH->attrType.u.remClassId;
              break;

         case CL_COR_ARRAY_ATTR:
                attrDef.u.attrInfo.arrDataType = attrH->attrType.u.arrType;
                attrDef.u.attrInfo.maxElement = attrH->attrValue.min;
              break;

         default:
                 attrDef.u.attrInfo.arrDataType = attrH->attrType.type;
                 attrDef.attrType = CL_COR_SIMPLE_ATTR;
                 attrDef.u.simpleAttrVals.init = attrH->attrValue.init;
                 attrDef.u.simpleAttrVals.min = attrH->attrValue.min;
                 attrDef.u.simpleAttrVals.max = attrH->attrValue.max;
    }

    memcpy((pAttrDefIDL->pAttrDef + pAttrDefIDL->numEntries), &attrDef, sizeof(ClCorAttrDefT));
    pAttrDefIDL->numEntries++;

    return CL_OK;
}
