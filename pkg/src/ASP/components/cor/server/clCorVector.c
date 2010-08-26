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
 * File        : clCorVector.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Vector DS API's & Definitions
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>

/* Internal Headers */
#include "clCorVector.h"
#include "clCorDefs.h"
#include "clCorMArray.h"
#include "clCorTreeDefs.h"
#include "clCorPvt.h"
#include "clCorLog.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* [Internal] method prototypes
 */
static ClRcT _corVectorNodeCreate(ClUint32T elementSize,
                                   ClUint16T elements,
                                   CORVectorNode_h* node);
static ClRcT _corVectorNodeDelete(CORVectorNode_h this);
static ClRcT _corVectorNodeWalk(CORVector_h this,
                                 ClRcT (*fp)(CORVectorNode_h node));
static ClInt32T _clCorVectorTreeComp(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2);

FILE *fp = NULL;
/*
extern ObjTreeNode_h
corObjTreeFindParent(ObjTree_h this, ClCorMOIdPtrT    path);
*/
/* GLOBALS */

/*
 *  CORVector contains list of CORVectorNode's. 
 *  
 *    CORVectorNode          CORVectorNode
 *    /-----------\          /-----------\      
 *    |   next ---|--------> |   next ---| ... (to next blocks)
 *    |-----------|          |-----------|
 *    | < Data    |          | < Data    |
 *    |    Buf>   |          |    Buf>   |
 *    |(blockSize |          |(blockSize |
 *    |  elements)|          |  elements)|
 *    \-----------/          \-----------/
 *
 *  CORVectorNode is allocated and maintained within CORVector and not
 *  exposed to user.
 *
 */

#if COR_TEST
/**
 *  Create New Vector.
 *
 *  API to create a Vector. The size of the element and the number of
 *  elements in one block is specified.  The vector can be extended
 *  either by Extend API's or by calling ExtendedGet; on which new
 *  blocks shall be allocated and linked to accomodate the new index.
 *  Index is assumed to be zero based.
 *  
 *  @param elementSize Size of one Element
 *  @param elementsPerBlock   Size of one block (number of elements in a block)
 *  @param instance    [out] returns the newly allocated vector 
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorDelete
 */
ClRcT
corVectorCreate(ClUint32T elementSize,
                ClUint16T elementsPerBlock,
                CORVector_h* instance
                )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();

  if(instance && elementSize>0 && elementsPerBlock>0) 
    {
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create Vector")); 

      (*instance) = (CORVector_h) clHeapAllocate(sizeof(CORVector_t));
	  if(NULL == *instance)
	  {
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
      	CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM))); 
      	return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	  }		

      ret = corVectorInit((*instance), elementSize, elementsPerBlock);
      if(ret != CL_OK)
        {
          /* free the CORVector allocated */
          clHeapFree((*instance));
          *instance = 0;
        } 
      else
        {
          (*instance)->fpPack = 0;
          (*instance)->fpUnpack= 0;
          (*instance)->fpDelete = 0;
        }
          
    } else 
      {
		if(instance){
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INVALID_PARAM))); 
        	ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
		}
		else{
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INVALID_PARAM)));
        	ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
		}
      }
  
  CL_FUNC_EXIT();
  return ret;
}
#endif

/** 
 *  Delete COR Vector Instance.
 *  
 *  API to delete a previously created COR Vector.
 *                                                                        
 *  @param this   CORVector handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorCreate
 *
 */
#if 0
ClRcT 
corVectorDelete(CORVector_h this
                )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete Vector"));
  ret = corVectorRemoveAll(this);  
  if(ret==CL_OK)
    {
      clHeapFree(this);
    }

  CL_FUNC_EXIT();
  return ret;
}
#endif

ClInt32T _clCorVectorTreeComp(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2)
{
    return ((ClWordT) key1 - (ClWordT) key2);
}

/**
 *  Init Vector.
 *
 *  API to initialise Vector data struct that is passed. Thew size of
 *  the element and the number of elements in one block is
 *  specified. 
 *
 *  NOTE: Assumed that the CORVector_h is already allocated and
 *  doesn't have allocated blocks within it (if so, it might lead to
 *  memory leaks).
 *                                                                        
 *  @param this        Already allocated vector handle
 *  @param elementSize Size of one Element
 *  @param elementsPerBlock   Size of one block (number of elements in a block)
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorDelete
 */
ClRcT
corVectorInit(CORVector_h this,
              ClUint32T elementSize,
              ClUint16T elementsPerBlock
              )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(this && elementSize>0 && elementsPerBlock>0) 
    {
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Init Vector")); 

      ret = clCntRbtreeCreate(_clCorVectorTreeComp, NULL, NULL, CL_CNT_UNIQUE_KEY, &this->rbTree);
      if (ret != CL_OK)
      {
          clLogError("VECTOR", "INIT", "Failed to create rbtree for cor vector. rc [0x%x]", ret);
          return ret;
      }

      ret = _corVectorNodeCreate(elementSize, elementsPerBlock, &this->head);
      if(ret == CL_OK)
        {
          (this)->elementSize = (elementSize);
          (this)->elementsPerBlock = (elementsPerBlock);
          (this)->blocks = (1); 
          (this)->tail = (this)->head; 

          /* Add the block id to the rbtree */
          ret = clCntNodeAdd(this->rbTree, (ClCntKeyHandleT) (ClWordT) (this->blocks - 1), (ClCntDataHandleT) this->head, NULL);
          if (ret != CL_OK)
          {
              clLogError("VECTOR", "INTI", "Failed to add data into vector tree. rc [0x%x]", ret);
              return ret;
          }
        }
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }
  
  CL_FUNC_EXIT();
  return ret;
}

/** 
 *  Removes all elements.
 *  
 *  API to delete all nodes within a vector.
 *                                                                        
 *  @param this   CORVector handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorDelete
 *
 */
ClRcT 
corVectorRemoveAll(CORVector_h this
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "RemoveAll nodes in Vector"));

  if(this) 
    {
      /* walk thru all the cor vector elements and then free them.
		 Since the user of corVector can fill any sort of information
		 in that e.g. pointer to some data. It gives an opprtunity to the
		 user to delete those stuff, before the basic cleanup work start
		 in _corVectorNodeDelete which will delete the data in the node
		 and then the node itself. In case of pointers if the user has not
		 deleted it himself, there will be memory leak and dangling pointers.
       */
#if CL_COR_REDUCE_OBJ_MEM
      if(this->fpDelete)
        {
          ret = corVectorWalk(this, this->fpDelete, 0);
        }
#endif
      if(ret==CL_OK)
        {
          /* walk thru everything and delete the nodes */
          ret=_corVectorNodeWalk(this, _corVectorNodeDelete);
        }
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

     if(ret == CL_OK)
    {
     	/* All the nodes deleted successfully. Let's set the head and tail pointers to NULL */
        this->head = this->tail = NULL;
        this->blocks = 0;

        if (this->rbTree)
        {
            ret = clCntDelete(this->rbTree);
            if (ret != CL_OK)
            {
                clLogError("VECTOR", "DELETE", "Failed to delete rbtree container. rc [0x%x]", ret);
            }
            else this->rbTree = 0;
        }
    }
  
  CL_FUNC_EXIT();
  return ret;
}

/** 
 *  Extend the COR Vector.
 *  
 *  API to extend the vector for the given size.
 * 
 *  @param this   CORVector handle
 *  @param index  Extend the vector to accomodate 'index'
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorCreate
 *
 */
ClRcT 
corVectorExtend(CORVector_h this,
                ClUint32T  size
                )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  
  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Extend Vector to size[%d]", size));

  if(this && size>0) 
  {
      int sz = ((this)->elementsPerBlock * (this)->blocks);
      if(size >= sz)
        {
          /* need to add one more vector node and set the pointers
           * appropriately. (this)->blocks is one based, so manipulate
           * as per that.
           */
          int blks = (size/this->elementsPerBlock) + 1;
          /*if(!(size%this->elementsPerBlock)) 
            {
              blks --;
            }*/
          /* now make sure the vector has 'blks' number of blocks */
          while(blks>(this)->blocks && (ret == CL_OK))
            {
              CORVectorNode_h tmp;
              ret = _corVectorNodeCreate(this->elementSize, this->elementsPerBlock, &tmp);
              if(ret == CL_OK)
                {
                  /* add to the list of blocks */
                  if(this->tail)
                  {
                      this->tail->next = tmp;
                  }
                  this->tail = tmp;

                  /* Add the block id and node pointer to the rbtree */
                  ret = clCntNodeAdd(this->rbTree, (ClCntKeyHandleT) (ClWordT) this->blocks, 
                          (ClCntDataHandleT) tmp, NULL);
                  if (ret != CL_OK)
                  {
                      clLogError("VECTOR", "INIT", "Failed to add node reference into tree. rc [0x%x]", ret);
                      return ret;
                  }

                  this->blocks ++;
                }
            }

          CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Extended Vector to Blocks [%d]", 
                               this->blocks));
        } 
    } 
    else 
      {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;  
}

/** 
 *  Extended Get i'th element.
 *  
 *  API to return the i'th element in vector.  
 *
 *  NOTE:If the vector doesn't have i'th element, then the vector is
 *  extended to the size and the address is returned to the caller.
 *  If any validation of size needs to happen, it should be done
 *  calling corVectorSizeGet API, before calling this.
 * 
 *  @param this   CORVector handle
 *  @param idx    Element Index
 *
 *  @returns 
 *    non NULL (address) on CL_OK <br/>
 *    0/NULL  on FAILURE <br/>
 *
 *  @see #corVectorSizeGet
 *  @see #corVectorGet
 *  @see #corVectorExtend
 *
 */
void *
corVectorExtendedGet(CORVector_h this,
                     ClUint32T  idx
                     )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();

  if(this && this->head) 
    {
      if(idx >= (this->elementsPerBlock * this->blocks))
        ret = corVectorExtend(this, idx);
      if(ret==CL_OK)
        {
          CL_FUNC_EXIT();
          return corVectorGet(this, idx);
        }
    }

  CL_FUNC_EXIT();
  return 0;
}

/** 
 *  Get i'th element.
 *  
 *  API to return the i'th element in vector.  If idx is not within
 *  the range, then null pointer is returned back.
 * 
 *  @param this   CORVector handle
 *  @param idx    Element Index
 *
 *  @returns 
 *    non NULL (address) on CL_OK <br/>
 *    0/NULL  on FAILURE <br/>
 *
 *  @see #corVectorSizeGet
 *  @see #corVectorExtend
 *  @see #corVectorFix
 *
 */
void *
corVectorGet(CORVector_h this,
             ClUint32T  idx
             )
{
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();

    if(this && this->head) 
    {
      CORVectorNode_h node = (this)->head;
      int blk = idx/this->elementsPerBlock;
      if(idx >= (this->elementsPerBlock * this->blocks))
        {
          CL_FUNC_EXIT();
          return 0;
        }

      rc = clCntDataForKeyGet(this->rbTree, (ClCntKeyHandleT) (ClWordT) blk, (ClCntDataHandleT *) &node);
      if (rc != CL_OK)
      {
          clLogError("VECTOR", "GET", "Failed to get node reference for key [%d] from tree. rc [0x%x]",
                  blk, rc);
          return NULL;
      }

      idx %= this->elementsPerBlock;

      CL_FUNC_EXIT();
      return node?((char *)(node->data)+(this->elementSize*(idx))):0;
    }

    CL_FUNC_EXIT();
    return 0;
}

/** 
 *  Walk the vector.
 *  
 *  API to Walk the vector elements. NOTE: All the elements are walked
 *  till the maximum index is reached. As there is no means for the
 *  vector API to know if the element is present or not, it simply
 *  loops thru all elements.
 * 
 *  @param this   CORVector handle
 *  @param fp     Function pointer called on each element
 *  @param cookie cookie/context the user wants to pass to the fp
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #corVectorGet
 *
 */
ClRcT 
_corVectorWalk(CORVector_h this,
               CORVectorWalkFP fp,
               void ** cookie,
               CORWalkType_e type
               )

{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();

  if(this && this->head && fp) 
    {
      CORVectorNode_h node = (this)->head;
      void * element = NULL; /* node->data; */
      ClUint32T idx = 0;
      ClUint32T sz = this->elementsPerBlock * this->blocks;

      if (NULL != node)
      {
          element = node->data;
          if (NULL == element)
          {
              clLogError("CVC", "WLK", "The data part of the node is NULL");
              return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
          }
      }

      while(node)
        {
          ret = (*fp)(idx, element, cookie);
          if(ret != CL_OK)
            break;

          /* go to the next element */
          element = (char *)element + this->elementSize;
          idx ++;

	  /*This check is required for equal condition.*/
          if(idx >= sz)
          {
              /*Size is recalculated here because if someone modifies the vector
               *during corVectorWalk (increases vector size), that can be accomodated
               *here.
               */
              sz = this->elementsPerBlock * this->blocks;
              if(idx >= sz)
                  break;
          }

          /* see if we need to navigate to the next block */
          if(!(idx % this->elementsPerBlock))
            {
              node = node->next;
			  if (node != NULL)
              	element = node->data;
            }
        }
      /* check on the walk type and then call once extra
       * with everything null, indicating its end of vector
       */
      if(ret==CL_OK && type == COR_LAST_NODE_INDICATION_WALK)
        {
          ret = (*fp)(COR_LAST_NODE_IDX, 0, cookie);
        }

    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}

/** 
 *  Pack the vector.
 *  
 *  API to pack the vector elements. NOTE: All the elements are packed.
 * 
 *  @param this   CORVector handle
 *  @param dst    buffer where the object have to be packed.
 *  @param size   [in/out] size of the buffer that is passed. Size of the
 *                buffer that is filled
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER) on invalid/null instance handle <br/>
 *
 *  @see #corVectorUnpack
 *
 */
ClRcT 
corVectorPack(CORVector_h this,
              void *  dst
              )
{
  ClRcT ret = CL_OK;
  ClBufferHandleT *pBufH = (((CORPackObj_h)dst)->pBufH) ;

  CL_FUNC_ENTER();
  
  

  if(this && this->head )
    {
      /* NOTE: What about the sizeLeft and the no of elements
       * in the vector ? We can just guess how many can be 
       * by computing the max and see if it can be accomodated.
       * todo: To add check on size
       */
        if(NULL != pBufH)
        { 

#if CL_COR_REDUCE_OBJ_MEM
          /* 
             This callback function is now removed from the CORVector_t structure inorder to 
             reduce the per object memory size.

             fp = this->fpPack : This is now assigned to NULL as the fpPack function is not assigned 
                                 anywhere before calling the vector pack function. 
           */
          CORVectorWalkFP fp = NULL;
		  CORPackObj_t packObject;
		  CORPackObj_h packObj = &packObject;	
#endif

           
		  ClUint16T elementsPerBlock ;
		  ClUint32T blocks ;
		  ClUint32T elementSize ;
          STREAM_IN_BOUNDS_HTONS(pBufH, &this->elementsPerBlock, elementsPerBlock,sizeof(elementsPerBlock));
          STREAM_IN_BOUNDS_HTONL(pBufH, &this->blocks, blocks, sizeof(blocks));
          STREAM_IN_BOUNDS_HTONL(pBufH, &this->elementSize, elementSize, sizeof(elementSize));

#if CL_COR_REDUCE_OBJ_MEM
          /* walk thru all the elements and pack them
           */
          if(fp)
          {
			  packObj->flags = ((CORPackObj_h)dst)->flags;		
              packObj->pBufH = pBufH;
              ret=corVectorWalk(this, fp, (void **) &packObj);
          }
#endif
        } 
        else 
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INSUFFICIENT_BUFFER)));
            ret = CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER);
        }
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_FUNC_EXIT();
    return ret;
}

/** 
 *  UnPack the vector.
 *  
 *  API to unpack the vector elements. 
 * 
 *  @param this   CORVector handle
 *  @param dst    buffer where the object are packed.
 *  @param size   [in/out] size of the buffer that is passed. Size of the
 *                buffer that is read
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_INVALID_BUFFER) on mismatch between instance, stream buffer <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER) on size zero buffer being passed <br/>
 *
 *  @see #corVectorPack
 *
 */
ClRcT 
corVectorUnpack(CORVector_h this,
                void *  src,
                ClUint32T* size
                )
{
  ClRcT ret = CL_OK;
  Byte_h buf = (Byte_h)src;

  CL_FUNC_ENTER();
  
  

  if(this && size)
    {
      if(*size>0)
        {
          CORVector_t tmp;
          
          STREAM_OUT_NTOHS(&tmp.elementsPerBlock,buf, sizeof(this->elementsPerBlock));
          STREAM_OUT_NTOHL(&tmp.blocks, buf, sizeof(this->blocks));
          STREAM_OUT_NTOHL(&tmp.elementSize,buf, sizeof(this->elementSize));


          if(!this->elementsPerBlock && !this->head)
            {
              /* init the vector here */
              ret = corVectorInit(this, tmp.elementSize, tmp.elementsPerBlock);
            }

          /* check if the vectors are same */
          if((tmp.elementsPerBlock != this->elementsPerBlock) ||
             (tmp.elementSize != this->elementSize) ||
             !this->head)
            {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INVALID_BUFFER)));
              ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_BUFFER);
            }
	  	  else 
            {
             /*
                This callback function is now removed from the CORVector_t structure inorder to 
                reduce the per object memory size.

                fp = this->fpUnpack; This is now assigned to NULL, as this callback function is not
                                      assigned anywhere before calling the vector unpack function.
              */  
              CORVectorWalkFP fp = NULL;
              int idxToExtendTo = (tmp.elementsPerBlock*tmp.blocks)-1;

              /* first expand the vector to the given blocks */
              ret = corVectorExtend(this, idxToExtendTo);
              
              /* loop thru the size and then unpack the
               * elements in the vector
               */
              if(ret==CL_OK && fp)
                {
                  ret = corVectorWalk(this, fp, (void **) &buf);
                }
              /* update the size read and return back */
              if(ret == CL_OK)
                {
                  *size = buf - (Byte_h) src;
                }
            }
        } else 
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INSUFFICIENT_BUFFER)));
            ret = CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER);
          }
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}

ClRcT corVectorNodeFind(CORVector_h this, ClUint32T id, ClUint32T index, ClUint32T *idx)
{
    if(this && this->head)
    {
        ClUint32T vecIndex = 0;
        CORVectorNode_h node = (this)->head;
        void * element = NULL ; /* node->data; */
        ClUint32T nodeId =  0 ; /* ((MArrayNode_h)element)->id */
        ClUint32T sz = this->elementsPerBlock * this->blocks;
    
        if (NULL != node)
        {
            element = node->data;
            if (NULL != element)
            {
                nodeId = ((MArrayNode_h)element)->id;
            }
            else
            {
                clLogError("CVC", "NFD", "The data part of the node is NULL");
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }
        }

        while(node)
        {
            nodeId = ((MArrayNode_h)element)->id;
            if(id == nodeId)
            {
                *idx = vecIndex;
                return CL_OK;
            }
            else if(nodeId == 0 && vecIndex == index){
                /*Try to put the information at same index if the information is
                  not locally present and that index is free*/
                *idx = vecIndex;
            }
            else if( (*idx == 0) && (nodeId == 0) && vecIndex){
                /* Mark this node as a free node. If passed node id
                   doesn't match with existing information. This will be used.
                 */
                *idx = vecIndex;
            }
            /* go to the next element */
            element = (char *)element + this->elementSize;
            vecIndex ++;
            
            if(vecIndex >= sz)
                break;
            
            /* see if we need to navigate to the next block */
            if(!(vecIndex % this->elementsPerBlock))
            {
                node = node->next;
				if (node != NULL)
                	element = node->data;
            }
        }
        if(*idx)
            return CL_OK;
        
        sz += this->elementsPerBlock - 1;
        if(corVectorExtend(this, sz) != CL_OK){
            /*printf("Could not extend the vector till %d\n",sz);*/
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        *idx = vecIndex;
        return CL_OK;
    }
    return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
}

#ifdef COR_TEST 
/* Show the vector info
 */
void
corVectorShow(CORVector_h this)
{
  if(this)
    {
      clOsalPrintf("Vector %p TotalSz[%d], Block[n:%d,Sz:%d], ElementSz[%d]\n", 
               this, 
               this->blocks*this->elementsPerBlock,
               this->blocks,
               this->elementsPerBlock,
               this->elementSize);
    }
}
#endif

/* -------------------------------------------------------------------- */


/**
 *  [Internal] Create COR Vector Node.
 *
 *  API to create a COR Vector Node structure. This is not an exposed
 *  data struct and the API should not be called directly from
 *  application.
 *                                                                        
 *  @param elementSize Size of one Element
 *  @param elements    Total elements in one node
 *  @param node        [out] returns the newly allocated vector node
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null node handle <br/>
 *
 *  @see #_corVectorNodeDelete
 */
static ClRcT
_corVectorNodeCreate(ClUint32T elementSize,
                     ClUint16T elements,
                     CORVectorNode_h* node
                     )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(node && elementSize>0 && elements>0) 
    {
      int sz = sizeof(CORVectorNode_t)+((elementSize)*(elements));
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create Vector Node [n:%d, sz:%d]",
                            elements,
                            elementSize));
      
      *node = (CORVectorNode_h) clHeapAllocate(sz);
      if(*node)
        {
          memset((*node), 0, sz);
          (*node)->data = &((*node)->data); 

          /* data points to the elements zone, so that its
           * easy to offset from there
           */
          (*node)->data = (char *)((*node)->data) + sizeof((*node)->data);
        } else 
          {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
            ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
          } 
    } else 
      {
		if(node){
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_INVALID_PARAM)));
        	ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
		}
		else{
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
        	ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
		}
      }

  CL_FUNC_EXIT();
  return ret;
}

/** 
 *  [Internal] Delete COR Vector Node Instance.
 *  
 *  API to delete a previously created COR Vector Node.
 *                                                                        
 *  @param this   CORVector Node handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #_corVectorNodeCreate
 *
 */
static ClRcT 
_corVectorNodeDelete(CORVectorNode_h this
                     )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete Vector Node"));

  if(this) 
    {
      /* free the object */
      clHeapFree(this); 
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}


/** 
 *  [Internal] Walk the COR Vector nodes.
 *  
 *  API to Walk thru all the vector nodes.
 *                                                                        
 *  @param this   CORVector handle
 *  @param fp     Function to be called on each vector node.
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *    return status from the function <br/>
 *
 *  @see #corVectorCreate
 *
 */
static ClRcT
_corVectorNodeWalk(CORVector_h this,
                   ClRcT (*fp)(CORVectorNode_h node)
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Walk Vector Nodes"));

  /* Mahesh & Rajni */
  if (fp == NULL) {
      ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
  }

  if(this) 
    {
      CORVectorNode_h tmp = this->head;
      CORVectorNode_h pNextNode; 

      while(tmp && (ret == CL_OK))
        {
	    /* Mahesh & Rajni
	     * this is needed so that we get a handle on the next node
	     * before we "delete" this node, if fp is the delete function
	     */
	    pNextNode = tmp->next;
	    ret=(*fp)(tmp);
	    tmp = pNextNode;
        }
    }
  else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;  
}

#ifdef UNIT_TEST


/* ----------- These are sample Functions -------------------- */


/** 
 *  [Internal] default pack routine.  
 *
 *  If any special care have to be taken, then
 *  user pack has to be called. It should not come over here.
 */
static ClRcT
_corVectorPackData(ClUint32T idx,
                   void * element,
                   void ** buffer
                   )
{
  if(element)
    {
      int* data = (int*) element;
      if(*data)
        {
          /* worry about packing now */
          Byte_h* buf = (Byte_h*) buffer;
          /* first stream the index, then stream the value */
          STREAM_IN((*buf), &idx, sizeof(idx));
          STREAM_IN((*buf), data, sizeof(*data));
        }
    }

  return (CL_OK);
}


/**
 * [Internal] Default unpack routine.  
 *
 * If any special care have to be taken, then user unpack has to be
 * called. It should not come over here.
 */
static ClRcT
_corVectorUnpackData(ClUint32T idx,
                     void * element,
                     void ** buffer
                     )
{
  if(element && buffer)
    {
      int* data = (int*) element;
      ClUint32T dataIdx;
      Byte_h* buf = (Byte_h*) buffer;

      /* first check if the index is correct, then
       * unpack the stuff
       */
      STREAM_PEEK(&dataIdx, (*buf), sizeof(dataIdx));
      if(idx == dataIdx)
        {
          /* now unpack the data items to the data buffer. First need
             to read the index, then read the buffer
          */
          STREAM_OUT(&dataIdx, (*buf), sizeof(dataIdx));
          STREAM_OUT(data, (*buf), sizeof(*data));
        } 
    }

  return (CL_OK);
}
/**/ 
#endif

#if 0
ClRcT 
_corWalkUp(CORVector_h this,
			   CORWalkUpFP fp,	
               void ** cookie,
               CORWalkType_e type
               )

{
    ClRcT ret = CL_OK;
    ClUint32T depth;
    MArrayWalk_h context = (MArrayWalk_h)cookie;

    CL_FUNC_ENTER();
    
    

    if( (context== NULL) ||((MArrayWalk_h)*((context)->cookie) == NULL)
        || (((ClCorMOIdPtrT)*((MArrayWalk_h)*(context)->cookie)->cookie) == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "MOID Cant be NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    if(this && this->head && fp) 
    {
        CORVectorNode_h node = (this)->head;
        void * element = node->data; 
        ClUint32T idx = 0;
        ClCorMOIdPtrT  pMoId;
        ClUint32T sz = this->elementsPerBlock * this->blocks;
        ClCorObjectHandleT pHandle;  
        ClUint32T bool = 0; 
          
        pMoId = (ClCorMOIdPtrT)*((MArrayWalk_h)*(context)->cookie)->cookie;

        depth = pMoId->depth;

        while(depth>0)
        {

            if((context)->flags == CL_COR_MSO_WALK_UP)
            {
                if(bool == 0)
                    pHandle = *(ClCorObjectHandleT *)element;
                ret = (*fp)(idx, &pHandle, cookie);
                if(ret != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Walk Function on MSO failed \n"));
                    break;
                }
            }
            else if((context)->flags == CL_COR_MO_WALK_UP)
            {
                /* go to the next element */
                if(bool == 0)
                {
                    element = (char *)element + this->elementSize;
                    pHandle = *(ClCorObjectHandleT *)element;
                }
                else
                {
                    element = &pHandle;
                    element = (char *)element + this->elementSize;
                    pHandle = *(ClCorObjectHandleT *)element;

                }
                ret = (*fp)(idx, &pHandle, cookie);
                if(ret != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Walk Function on MO failed \n"));
                    break;
                }
                idx ++;
                if(idx >= sz)
                    break;
            }
            /* go to the parent element */
            clCorMoIdTruncate(pMoId, depth); 

            _clCorObjectHandleGet(pMoId, (ClCorObjectHandleT *) &pHandle); 
            bool = 1;
            depth--;
        }
        /* check on the walk type and then call once extra
         * with everything null, indicating its end of vector
         */
        if(ret==CL_OK && type == COR_LAST_NODE_INDICATION_WALK)
        {
            ret = (*fp)(COR_LAST_NODE_IDX, 0, cookie);
        }

    }
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_FUNC_EXIT();
    return ret;
}
#endif
