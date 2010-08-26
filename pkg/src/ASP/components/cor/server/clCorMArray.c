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
 * File        : clCorMArray.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains MArray DataStructure functions.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>
#include <clBufferApi.h>

/* Internal Headers*/
#include "clCorMArray.h"
#include "clCorDefs.h"
#include "clCorTreeDefs.h"
#include "clCorPvt.h"
#include "clCorLog.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* Globals */

/*
 *  MArray
 *      --> MArrayNode    (marray node )
 *      --> MArrayData    (data)
 *
 *  
 *          MArrayNode  
 *    /-----------------------------\  
 *    |   (   groups vector   )     |  
 *    |    |           |            |  
 *    |    |           |            |  
 *    |    v           v            |  
 *    | [MArrayVec1] [MArrayVec2].. |  
 *    |                             |  
 *    |-----------------------------|  
 *    | <node data (user defined)>  |  
 *    \-----------------------------/  
 *
 *  MArrayNode - contains node data (whose size and value is
 *  maintained by user).  
 * 
 *  MArrayVec - can be either a MArrayNode / Vector of data nodes. if
 *  its a data node, then the nodes in that vector is maintained by
 *  user.
 *
 * todo:
 * 1) returns codes incase of fp not present, should be differenet
 * 2) pack / unpack of vector misses a scenario - how about
 *    if the user passes an empty vector and exepects us to create it
 *
 */

static ClRcT _mArrayNodeShow(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayDataShow(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayNodePack(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayNodeUnpack(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayDataPack(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayDataUnpack(ClUint32T idx, void * element, void ** cookie);
#ifdef DISTRIBUTED_COR
static ClRcT _mArrayNodeRemap(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayVectorRemap(ClUint32T idx, void * element, void ** cookie);

static ClRcT _mArrayNodeUpdate(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayVectorUpdate(ClUint32T idx, void * element, void ** cookie);
#endif
ClInt32T _modifyPath(CORVector_h vec, ClUint32T id, ClUint32T index, ClUint32T type,
                        ClUint32T *nodeExists, ClUint32T *instanceId);
ClRcT _corObjRemap(CORVector_h vec, ClCorMOClassPathPtrT moPath);
ClRcT swap(CORVector_h vec, ClUint32T slot, ClUint32T idx);
ClRcT _corMONodeUpdateFun(ClCorMOClassPathT moPath, ClUint32T *totalChild);
ClRcT _updateNodeInstance(ClUint32T id, ClUint32T index, ClUint32T type, ClUint32T init);

static int nCount = 0;
static int lvls = 0;
extern ClCorMOIdT gUnpackMOId;
static ClRcT mArrayGroupVectorFree(CORVector_h this);


#define APPEND_IN_MOID  1
#define REDUCE_MOID     0

#define MSO -1

/**
 *  Create New MArray instance.
 *
 *  API to create an MArray instance.
 *                                                                        
 *  @param nodeSize    Size of one node
 *  @param instance    [out] handle to newly created mArray instance
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayDelete
 */
ClRcT
mArrayCreate(ClUint32T nodeSize,
             MArray_h* instance
             )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(instance && nodeSize>0) 
    {
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create MArray")); 

      (*instance) = (MArray_h) clHeapAllocate(sizeof(MArray_t));
      if(*instance)
        {
          /* create the group vector here */
          ret = mArrayNodeCreate(nodeSize, 
                                &((*instance)->root));
          if(ret == CL_OK)
            {
              (*instance)->nodeSize = nodeSize;
              (*instance)->fpPackData = 0;
              (*instance)->fpPackNode = 0;
              (*instance)->fpUnpackData = 0;
              (*instance)->fpUnpackNode = 0;
#ifdef CL_COR_REDUCE_OBJ_MEM
              (*instance)->fpDeleteData = 0;
              (*instance)->fpDeleteNode = 0;
#endif 

#ifdef DEBUG
              (*instance)->name[0] = 0;
#endif 
            }
          else 
            {
              clHeapFree(*instance);
            }

        } else
          {
	   
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
            ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
          } 
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}

#if 0
/** 
 *  Delete mArray Instance.
 *  
 *  API to delete a previously created mArray.
 *                                                                        
 *  @param this   mArray instance handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayCreate
 *
 */
ClRcT 
mArrayDelete(MArray_h this
             )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete MArray"));

  if(this) 
    {
      /* todo: walk thru everything and delete all the nodes on root */
      if(ret == CL_OK)
        {
          /* free the object */
          clHeapFree(this);
        }      
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}

#endif
/**
 *  Init MArray Node.
 *
 *  API to init already allocated MArray node.
 *                                                                        
 *  @param this     handle to  mArray node
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayCreate
 */
ClRcT
mArrayNodeInit(MArrayNode_h this)
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(this) 
    {
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Init MArrayNode")); 

      /* init the group vector here */
      ret = corVectorInit(&(this->groups),
                          MARRAY_GROUP_NODE_SZ, 
                          MARRAY_GROUP_BLK_SZ
                          );
      if(ret == CL_OK)
        {
          (this)->id = 0;
          (this)->data = &((this)->data);
          (this)->data = (char *)((this)->data) + sizeof((this)->data);
#ifdef DEBUG
          (this)->name[0] = 0;
#endif 
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
 *  Create New MArray Node.
 *
 *  API to create an MArray node.
 *                                                                        
 *  @param nodeSize    Size of one node
 *  @param instance    [out] handle to newly created mArray node
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayDelete
 */
ClRcT
mArrayNodeCreate(ClUint32T nodeSize,
                 MArrayNode_h* instance
                 )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(instance && nodeSize>0) 
    {
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create MArrayNode")); 

      (*instance) = (MArrayNode_h) clHeapAllocate(sizeof(MArrayNode_t) + nodeSize);
      if(*instance)
        {
          ret = mArrayNodeInit(*instance);
          if(ret != CL_OK)
            {
              clHeapFree(*instance);
            }
        } else
          {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
            ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
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
 *  Delete COR Vector Instance.
 *  
 *  API to delete a previously created COR Vector.
 *                                                                        
 *  @param this   MArray handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayNodeCreate
 *
 */
ClRcT 
mArrayNodeDelete(MArrayNode_h this)
{
  ClRcT ret = CL_OK;
  MArrayVector_h vectorH = NULL;
  ClUint32T vectorSz = 0, index = 0;
  CORVectorNode_h node1 = NULL, node2 = NULL;
  CL_FUNC_ENTER();
  
  if (NULL == this)
  {
      clLogError("MAV", "MND", 
              "The pointer to the MArray Node obtained is NULL");
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
  }

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete MArrayNode"));

  /* Code for removing all the child nodes of a particular Node type. 
     Index is starting from '1' as we are already removing the MSO nodes
     which are at index '0'. This code do a walk on all the group vector 
     nodes and see are there any mArrayNodes for this child type. If there
     are any, we free those nodes.
  */
  vectorSz = this->groups.blocks * this->groups.elementsPerBlock;
  for(index = 1 ; index < vectorSz ; index++)
  {
        vectorH = corVectorGet(&this->groups, index);
        node1 = vectorH->nodes.head;
        while(node1 != NULL)
        {
            node2 = node1->next;
            clHeapFree(node1);
            node1 = node2;
        }
  }

  if(this) 
    {
      /* todo: walk thru everything and delete the nodes underneath it */
      ret = corVectorRemoveAll(&this->groups);
      if(ret == CL_OK)
        {
		 memset(&this->groups, '\0', sizeof(CORVector_t) ); 
          /* free the object */
          /*clHeapFree(this);*/
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
 *  Create MArrayVector Node.
 *
 *  API to create a MArrayVector structure. 
 *                                                                        
 *  @param elementSize Size of one Element
 *  @param elementsPerBlock   Total elements in one vector node (Block)
 *  @param navigable   1 - yes, this vector will hold MArrayNode's
 *  @param node        [out] returns the newly allocated MArray vector node
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null node handle <br/>
 *
 *  @see #mArrayNodeDelete
 */
#if 0
ClRcT
mArrayVectorCreate(ClUint32T elementSize,
                   ClUint16T elementsPerBlock,
                   ClUint16T navigable,
                   MArrayVector_h* node
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(node && elementSize>0 && elementsPerBlock>0) 
    {
      int sz = sizeof(MArrayVector_t);
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create MArrayVector Node [n:%d, sz:%d] Navigation:%d",
                            elementsPerBlock,
                            elementSize,
                            navigable));
      
      *node = (MArrayVector_h) clHeapAllocate(sz);
      if(*node)
        {
          ret = corVectorInit(&((*node)->nodes), elementSize, elementsPerBlock);
          if(ret == CL_OK)
            {
              (*node)->navigable = navigable;
            }
          else 
            {
              clHeapFree(*node);
            }
        } else 
          {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
            ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
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
 *  Delete MArray Vector Node Instance.
 *  
 *  API to delete a previously created MArray Vector Node.
 *                                                                        
 *  @param this   MArray Vector handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 *  @see #mArrayVectorCreate
 *
 */
ClRcT 
mArrayVectorDelete(MArrayVector_h this
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete MArrayVector Node"));

  if(this) 
    {
      /* delete the vector first */
      ret = corVectorRemoveAll(&this->nodes);
      if(ret == CL_OK)
        {
          /* free the object */
          clHeapFree(this);
        }
    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}
#endif

/**
 *  Add mArray Node at the given index.
 *
 *  API to create and add an MArray Node
 *                                                                        
 *  @param group    group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 */
ClRcT
mArrayNodeAdd(MArray_h   tree,
              MArrayNode_h this,
              ClUint16T groupId,
              ClUint32T idx,
              ClUint32T elementsPerBlock
              )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(this) 
    {
      MArrayVector_h vectorH;
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Add mArrayNode at [%d,%d]",
                            groupId,
                            idx));
      /* first locate the group node */
      vectorH = (MArrayVector_h) corVectorExtendedGet(&this->groups, groupId);
      if(vectorH) 
        {
          /* if the vector is not inited, then go ahead and init */
          if(!vectorH->nodes.head && !vectorH->nodes.tail)
            {
              vectorH->navigable = MARRAY_NAVIGABLE_NODE;
              ret = corVectorInit(&vectorH->nodes, 
                                  sizeof(MArrayNode_t)+ tree->nodeSize,
                                  elementsPerBlock);
             }
          if((vectorH)->navigable == MARRAY_NAVIGABLE_NODE)
            {
              MArrayNode_h node;
              node = (MArrayNode_h) corVectorExtendedGet(&(vectorH)->nodes, idx);
              /* if no node present, then create a new node */
              if(node && !node->groups.head && !node->groups.tail)
                {
                  ret = mArrayNodeInit(node);
                }
              else 
                {
                  /* set the return type to already present */
                  ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY);
                }
              
            } else 
              {
                /* set it as non navigable */
                ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_NODE_REF);
              }
        }
      else
        {
          /* todo: need to put an error code */
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
          ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
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
 *  Add mArray Node at the given index.
 *
 *  API to create and add an MArray Node
 *                                                                        
 *  @param group    group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 */
ClRcT
mArrayDataNodeAdd(MArrayNode_h this,
                  ClUint16T group,
                  ClUint32T elemSize,
                  ClUint32T elementsPerBlock
                  )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  

  if(this) 
    {
      MArrayVector_h vectorH;
      
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Add mArrayDataNode at [%d]",
                            group));

      /* first locate the group node */
      vectorH = (MArrayVector_h) corVectorExtendedGet(&this->groups, group);
      if(vectorH) 
        {
          /* if the vector is not inited, then go ahead and init */
          if(!vectorH->nodes.head && !vectorH->nodes.tail)
            {
              vectorH->navigable = !MARRAY_NAVIGABLE_NODE;
              ret = corVectorInit(&vectorH->nodes, 
                                  elemSize,
                                  elementsPerBlock);
            } 
        }
      else
        {
          /* todo: need to put an error code */
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
          ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
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
 *  Delete the DataNode in NodeVector.
 *  @param group    group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *
 */

ClRcT
mArrayDataNodeDelete(MArrayNode_h this,
                  ClUint16T group)
{
      	ClRcT ret = CL_OK;
      	CL_FUNC_ENTER();
      	if(this) 
    	{
	      	MArrayVector_h vectorH;
	  	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Delete mArrayDataNode at [%d]",
		      			group));
	  	/* first locate the group node */
	  	vectorH = (MArrayVector_h) corVectorGet(&this->groups, group);
	  	if(vectorH) 
		{
	      		/* check, if vector is inited .. if so delete */
	      		if(vectorH->nodes.head && vectorH->nodes.tail)
	    		{
		  		ret = corVectorRemoveAll(&vectorH->nodes);
			} 
		}
	       	else
		{
		      	/* todo: need to put an error code */
	      		CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
	      		ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
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
 *  Returns the node (mArray Node) reference.
 *
 *  API to return the mArray Node.
 *                                                                        
 *  @param groupId  group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM) on memory allocation FAILURE <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on invalid/null instance handle <br/>
 *
 */
MArrayNode_h
mArrayNodeGet(MArrayNode_h this,
              ClUint16T groupId,
              ClUint32T idx
              )
{
  CL_FUNC_ENTER();

  if(this) 
    {
      MArrayVector_h vectorH;

      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Get mArrayNode at [%d,%d]",
                            groupId,
                            idx));
      vectorH = mArrayGroupGet(this, groupId);
      if(vectorH)
        {
          MArrayNode_h node = 0;
          if(vectorH->navigable == MARRAY_NAVIGABLE_NODE)
            {
              node = (MArrayNode_h) corVectorGet(&vectorH->nodes, idx);
              return node;
            } 
        }
    } 

  CL_FUNC_EXIT();
  return 0;
}

/**
 *  Returns the Data Nodereference.
 *
 *  API to return the mArray Data Node.
 *                                                                        
 *  @param groupId  group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    0  - Non-Success<br>
 *
 */
void *
mArrayDataGet(MArrayNode_h this,
              ClUint16T groupId,
              ClUint32T idx
              )
{
  CL_FUNC_ENTER();

  if(this) 
    {
      MArrayVector_h vectorH;

      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Get mArray at [%d,%d]",
                            groupId,
                            idx));
      vectorH = mArrayGroupGet(this, groupId);
      if(vectorH)
        {
          void * node = 0;
          if(vectorH->navigable != MARRAY_NAVIGABLE_NODE)
            {
              node = corVectorGet(&vectorH->nodes, idx);
            } 
          return (node);
        }
    } 

  CL_FUNC_EXIT();
  return 0;
}


/**
 *  Returns the Data Nodereference. 
 *
 *  API to return the mArray Data Node. If the idx is not present, it
 *  extends the data vector and makes sure its available.
 *                                                                        
 *  @param groupId  group index
 *  @param idx      element index within group
 *
 *  @returns 
 *    0  - Non-Success<br>
 *
 */
void *
mArrayExtendedDataGet(MArrayNode_h this,
                      ClUint16T groupId,
                      ClUint32T idx
                      )
{
  CL_FUNC_ENTER();
  

  if(this) 
    {
      MArrayVector_h vectorH;

      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Get mArray at [%d,%d]",
                            groupId,
                            idx));
      vectorH = mArrayGroupGet(this, groupId);
      if(vectorH)
        {
          void * node = 0;
          if(vectorH->navigable != MARRAY_NAVIGABLE_NODE)
            {
              node = corVectorExtendedGet(&vectorH->nodes, idx);
            } 
          return (node);
        }
    } 

  CL_FUNC_EXIT();
  return 0;
}

/** 
 * Display the marray tree.
 *
 * API to display the marray tree.  The tree's nodes and data elements
 * are displayed in tree form (please note that the data is just a
 * peek (first 4 bytes)).
 *
 *  @param this    mArray object handle
 * 
 *  @returns 
 *    none
 */

void
mArrayShow(MArray_h this, ClBufferHandleT *pMsgHdl)
{
    ClCharT corStr[CL_COR_CLI_STR_LEN];
                                                                                                                             

  if(this)
    {
      corStr[0]='\0';
      sprintf(corStr, "Details {nodeSz:%d}",this->nodeSize);
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
#ifdef DEBUG
      corStr[0]='\0';
      sprintf(corStr, "[%s] :", this->name);
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
#endif 
      lvls = 0;
      _mArrayWalk(this, _mArrayNodeShow, _mArrayDataShow, (void**)&pMsgHdl);
      lvls = 0;
      corStr[0]='\0';
      sprintf(corStr, "\n\n");
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    }
}
/** 
 * pack the marray tree.
 *
 * API to pack the marray contents.
 *
 *  @param this       object handle
 *  @param dst        destination buffer
 *  @param size       [in/out] size of the destination buffer
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo size to be computed upahead to check for the mem corruption.
 *
 */
ClRcT
mArrayPack(MArray_h this,
	   ClUint16T flags,
	   ClUint16T type,
       ClBufferHandleT *pBufH
           )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  
  
  

  /* @todo: need to take care of names and send it across 
   */

  if(this && this->root && pBufH)
    {
      MArrayStream_t packInfo;

      CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Pack mArray "));

      packInfo.pBufferHandle = pBufH ;
      packInfo.pThis = this;
      packInfo.flags = flags;
      packInfo.type = type;

	  ClUint32T nodeSize ;
      STREAM_IN_BOUNDS_HTONL(packInfo.pBufferHandle,
                       &this->nodeSize, 
					   nodeSize,
                            sizeof(nodeSize));
	
      /* call the root pack to pack into packInfo.buf and update
       * packInfo structure.
       */
      ret = _mArrayNodePack(0, this->root, (void **)&packInfo);

    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}               


/** 
 * Unpack the marray tree.
 *
 * API to Unpack the marray contents.
 *
 *  @param this       object handle
 *  @param dst        buffer containing packed data
 *  @param size       size of the buffer
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */
ClRcT
mArrayUnpack(MArray_h this,
             ClUint32T type,
             void *  src,
             ClUint32T* size
             )
{
  ClRcT ret = CL_OK;
  Byte_h buf = (Byte_h)src;

  CL_FUNC_ENTER();
  
  

  if(this && buf && size)
    {
      MArrayStream_t unpackInfo;

      CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "UnPack mArray [size passed:%d]",
                           *size));
      /* first allocate the root node */
      unpackInfo.buf   = buf;
      unpackInfo.size  = *size;
      unpackInfo.pThis = this;
      unpackInfo.type = type;

      STREAM_OUT_NTOHL(&this->nodeSize, unpackInfo.buf, sizeof(this->nodeSize));
      unpackInfo.size -= sizeof(this->nodeSize);
      
      if(!this->root)
        {
          MArrayNode_h tmp;
          
          tmp = clHeapAllocate(sizeof(MArrayNode_t));
	  if(tmp == NULL)
	  {
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	  }
          memset(tmp,0,sizeof(MArrayNode_t));
          this->root = tmp;
        }
      /* call unpack on the first node
       */
      ret = _mArrayNodeUnpack(0, this->root, (void **)&unpackInfo);
      nCount = 0;
      /* compute the size and return back */
      if(ret==CL_OK)
        {
          *size -= (unpackInfo.size);
        }

    } else 
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

  CL_FUNC_EXIT();
  return ret;
}               

#ifdef DEBUG

/** 
 * Set name of the mArray.
 *
 * API to set the name of mArray.
 *
 *  @param this   mArray object handle
 *  @param name   name to be set.
 * 
 */
void
mArrayNameSet(MArray_h this, char* name)
{
  if(this && name)
    {
      strncpy(this->name, name, DEBUG_NAME_SZ);
    }  
}

/** 
 * Set name of the mArray Node.
 *
 * API to set the name of mArray Node.
 *
 *  @param this   mArray node object handle
 *  @param name   name to be set.
 * 
 */
void
mArrayNodeNameSet(MArrayNode_h this, char*name)
{
  if(this && name)
    {
      strncpy(this->name, name, DEBUG_NAME_SZ);
    }
}


#endif

#ifdef DISTRIBUTED_COR

ClRcT mArrayRemap(void * element){
	return _mArrayNodeRemap(0, element, 0);
}

ClRcT mArrayUpdate(ClUint32T idx,
                    void * element,
                    void ** cookie){
	return _mArrayNodeUpdate(idx, element, cookie);
}
#endif

/* -------------------------------------------------------------------- */


/**
 * [Internal] MArray Vector walk routine.
 *
 * MArray Vector walk routine.  If its navigable, then call MArray
 * Node Walk else call the data walk function.
 *
 */
static ClRcT
_mArrayVectorWalk(ClUint32T idx,
                  void *     element, 
                  void **    cookie
                  )
{
  ClRcT ret = CL_OK;
  
  if(element && cookie)
    {
      MArrayWalk_h context = (MArrayWalk_h)cookie;
      MArrayVector_h this   = (MArrayVector_h)element;

      /* call only if the vector is inited */
      if(this->nodes.head && this->nodes.tail)
        {
          if(this->navigable)
            {
              ret = corVectorWalk(&this->nodes, _mArrayNodeWalk, cookie);
            }
          else
            {
              if(context->fpData)
                {
                  ret = corVectorWalk(&(this)->nodes, 
                                      context->fpData, 
                                      context->cookie);
                }
            }
        }
    }
      
  return ret;
}

/**
 * [Internal] MArray Node walk routine.
 *
 * MArray walk routine.  Right now its a depth first search routine
 * (walk). When it hits a node, the fpNode is called and when the data
 * vector is hit, a corVectorWalk is called on the datas, that inturn
 * invokes the fpData routine.
 *
 */
ClRcT
_mArrayNodeWalk(ClUint32T nodeId,
                void *     element, 
                void **    cookie
                )
{
  ClRcT ret = CL_OK;
  MArrayNode_h this = (MArrayNode_h) element;
  MArrayWalk_h context = (MArrayWalk_h)cookie;

  CL_FUNC_ENTER();

  if(this && cookie)
  {
      /* if group is inited then invoke a vector walk */
      if(this->groups.head && this->groups.tail)
      {
          if(context->fpNode)
          {
              ret = (*context->fpNode)(nodeId,
                                        context->internal?this:this->data, 
                                        context->cookie);

              /* send the __group begin__ marker call */
              ret = (*context->fpNode)(COR_FIRST_NODE_IDX, 0, context->cookie);
          }
          if((context->flags == CL_COR_MO_WALK_UP) || (context->flags == CL_COR_MSO_WALK_UP)) 
          {
			#if 0
              ret = _corWalkUp(&this->groups, _mArrayVectorWalk, cookie, COR_NORMAL_WALK);
			#endif
          }
          else
          {
              ret = corVectorWalk(&this->groups, _mArrayVectorWalk, cookie);
          }

          /* send the __group end__ marker call */
          if(context->fpNode)
          {
              ret = (*context->fpNode)(COR_LAST_NODE_IDX, 0, context->cookie);
              /*When we want to traverse the tree bottom up, we need to call function fpNode here*/
              if( (context->flags == CL_COR_MO_SUBTREE_WALK) || (context->flags == CL_COR_MSO_SUBTREE_WALK) )
              {
                  /*When traversing bottom up we take care that function inside context cookie
                   * is not called when we call it before vector walk, however, it is called after vector walk. 
                   */
                  (*(MArrayWalk_h *)(context->cookie))->flags = CL_COR_MO_WALK;
                  ret = (*context->fpNode)(nodeId,
                                       context->internal ? this:this->data, 
                                       context->cookie);
                  if(context->flags == CL_COR_MO_SUBTREE_WALK)
                      (*(MArrayWalk_h *)(context->cookie))->flags = CL_COR_MO_SUBTREE_WALK;
                  if(context->flags == CL_COR_MSO_SUBTREE_WALK)
                      (*(MArrayWalk_h *)(context->cookie))->flags = CL_COR_MSO_SUBTREE_WALK;
              }
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

static ClRcT
_mArrayVectorDelete(ClUint32T idx,
                  void *     element, 
                  void **    cookie
                  )
{
  ClRcT ret = CL_OK;
  
  if(element && cookie)
    {
      MArrayWalk_h context = (MArrayWalk_h)cookie;
      MArrayVector_h this   = (MArrayVector_h)element;

      /* call only if the vector is inited */
      if(this->nodes.head && this->nodes.tail)
        {
          if(this->navigable)
            {
              ret = corVectorWalk(&this->nodes, _mArrayNodeDelete, cookie);
            }
          else
            {
              if(context->fpData)
                {
                    corVectorRemoveAll(&(this->nodes)); 
                }
            }
        }
    }
      
  return ret;
}

ClRcT
_mArrayNodeDelete(ClUint32T nodeId,
                void *     element, 
                void **    cookie
                )
{
  ClRcT ret = CL_OK;
  MArrayNode_h this = (MArrayNode_h) element;
  MArrayWalk_h context = (MArrayWalk_h)cookie;
  /*  CORVector_t grp = this->groups; */

  CL_FUNC_ENTER();

  if(this && cookie)
  {
      /* if group is inited then invoke a vector walk */
      if(this->groups.head && this->groups.tail)
      {
          if(context->fpNode)
          {
              ret = (*context->fpNode)(nodeId,
                                        context->internal?this:this->data, 
                                        context->cookie);

              /* send the __group begin__ marker call */
              ret = (*context->fpNode)(COR_FIRST_NODE_IDX, 0, context->cookie);
          }

          ret = corVectorWalk(&this->groups, _mArrayVectorDelete, cookie);

          /* send the __group end__ marker call */
          if(context->fpNode)
          {
	      mArrayGroupVectorFree(&this->groups);
              ret = (*context->fpNode)(COR_LAST_NODE_IDX, 0, context->cookie);  
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

ClRcT _mArrayFreeNodeGet(ClUint32T idx,
                      void *     element, 
                      void **    cookie
                      )
{
    MArrayNode_h this = NULL;
    ClCorInstanceIdT* inst = NULL;

    CL_FUNC_ENTER();
    
    this = (MArrayNode_h) element;
    inst = (ClCorInstanceIdT*) cookie;

    *inst = idx;
    
    if ((this != NULL) && (this->id == 0))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_DUPLICATE));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

static ClRcT
mArrayGroupVectorFree(CORVector_h this)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
  

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Walk Vector Nodes"));
    if(this) 
    {
        CORVectorNode_h tmp = this->head;
        void *element = NULL ; /* tmp->data; */
        void* Node = NULL; 
        MArrayVector_h delNode = NULL ; /* (MArrayVector_h) element; */
        CORVectorNode_h VectNode = NULL ; /* delNode->nodes.head; */
        CORVectorNode_h nextVectNode = NULL;
        CORVectorNode_h pNextNode = NULL ; /* tmp->next; */
        ClUint32T idx = 0;
        ClUint32T sz = this->elementsPerBlock * this->blocks;

        if (tmp != NULL)
        {
        	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("the Size of the Block is %d , tmp %p", sz, (ClUint8T*) tmp));

            element = tmp->data;
            delNode = (MArrayVector_h) element;
            if (delNode != NULL)
                VectNode = delNode->nodes.head; 
            else
            {
                clLogError("MRY", "GVF", "The data part of the node is NULL");
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }

            pNextNode = tmp->next;
        }
        else
        {
            clLogError("MAV", "FNG", "The pointer to the head node is NULL");
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }


        while(tmp && (ret == CL_OK))
        {
            Node = (char *) element + this->elementSize; 
            if(delNode->nodes.head && delNode->nodes.tail)
            { 
                /* Walk each node in a vector and free the memory */
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" head %p", (ClUint8T*)delNode->nodes.head));
                VectNode = delNode->nodes.head->next;
                if(delNode->nodes.head)
                    clHeapFree(delNode->nodes.head);
                while(VectNode)
                {
                    nextVectNode = VectNode->next;
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" head %p", (ClUint8T*)VectNode));
                    clHeapFree(VectNode);
                    VectNode = nextVectNode;
                }
            }
            element = Node ;
            delNode = (MArrayVector_h)element;
              
            idx++;

            if(idx >= sz)
            {
                sz = this->elementsPerBlock * this->blocks;
                if(idx >=sz)
                {
                    if(tmp)
                        clHeapFree(tmp);
                    break;
                }
            }
        
            if(!(idx % this->elementsPerBlock))
            {
                pNextNode = tmp->next;
                if(tmp)
                    clHeapFree(tmp);
                tmp = pNextNode;
                if (tmp != NULL)
                    element = tmp->data;
                delNode = (MArrayVector_h)element;
	        }
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
 * [Internal] mArrayVector Pack.
 *
 * API to pack mArrayVector. API to pack the mArray Vector and its
 * information.
 */
static ClRcT
_mArrayVectorPack(ClUint32T idx,
                  void *     element, 
                  void **    cookie
                  )
{
  ClRcT ret = CL_OK; 
  MArrayStream_h context = (MArrayStream_h)cookie;
  CORPackObj_t packObject;
  CORPackObj_h packObj = &packObject;

  if (!element && cookie)
  {
      if (idx == COR_FIRST_GROUP_IDX)
      {
          ClUint32T tag = COR_FIRST_GROUP_TAG;
		  ClUint32T tmpTag ;
          STREAM_IN_BOUNDS_HTONL(context->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));

      }
      if (idx == COR_LAST_GROUP_IDX)
      {
          ClUint32T tag = COR_LAST_GROUP_TAG;
		  ClUint32T tmpTag;
          STREAM_IN_BOUNDS_HTONL(context->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));
      }
  }

  if(element && cookie)
    {
      MArrayVector_h this   = (MArrayVector_h)element;

      /* pack only if the vector is inited */
      if(this->nodes.head && this->nodes.tail)
        {
          /* first stream in the index, then the marray vector information */
		  ClUint32T tmpIdx ;
          ClUint32T activeNodes = 0; 
          ClUint16T navigable = 0;
            STREAM_IN_BUFFER_HANDLE_HTONL(*(context->pBufferHandle),&idx, tmpIdx, sizeof(tmpIdx));
            STREAM_IN_BUFFER_HANDLE_HTONS(*(context->pBufferHandle), &this->navigable, navigable, sizeof(this->navigable));
            STREAM_IN_BUFFER_HANDLE_HTONL(*(context->pBufferHandle), &this->numActiveNodes, activeNodes, sizeof(this->numActiveNodes));

          if(this->navigable)
	    {
		packObj->flags = context->flags;
                packObj->pBufH = context->pBufferHandle;
            	/* stream in the vector data */
#ifdef CL_COR_REDUCE_OBJ_MEM
            	corVectorPackFPSet(this->nodes, 0);
#endif
            	ret=corVectorPack(&this->nodes, (void *)packObj);
            	if(ret==CL_OK)
                {
                    ret = _mArrayNodePack(COR_FIRST_NODE_IDX, 0, cookie);
                    ret = corVectorWalk(&this->nodes, _mArrayNodePack, cookie);
                    if(ret != CL_OK && ret != CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK))
                        return ret;
                    ret = _mArrayNodePack(COR_LAST_NODE_IDX, 0, cookie);
                }
                else
                {
                    if(ret != CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK))
                        return ret;
                }
            }
          else
            {
              if(context->pThis && context->pThis->fpPackData)
                {
#ifdef CL_COR_REDUCE_OBJ_MEM
                  corVectorPackFPSet(this->nodes, 0);
#endif 
				  packObj->flags = context->flags;
                  packObj->pBufH = context->pBufferHandle;
                  ret=corVectorPack(&this->nodes, (void *)packObj);
                  if(ret==CL_OK)
                    {
                      ret = _mArrayDataPack(COR_FIRST_DATA_IDX, 0, cookie);
                      ret = corVectorWalk(&this->nodes, _mArrayDataPack, cookie);
                      if(ret != CL_OK && ret != CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK))
                          return ret;
                      ret = _mArrayDataPack(COR_LAST_DATA_IDX, 0, cookie);
                    }
                }
            }
        }
    }
  return ret;
}

/** 
 * [Internal] Node Pack.
 *
 * API to pack mArrayNode.  cookie points to a valid (MArrayStream)
 * object which contains the MArray_h handle, which inturn points to
 * the data and node pack functions provided by the user.
 *
 */
static ClRcT
_mArrayNodePack(ClUint32T idx,
                void * element, 
                void ** cookie
                )
{
  ClRcT ret = CL_OK;
  MArrayStream_h stream = (MArrayStream_h)cookie;
  void *tmpPtr = NULL;

  if(!element && cookie)
    {
	  /*** HTONL ****/
      if(idx==COR_FIRST_NODE_IDX)
        {
          ClUint32T tag = COR_FIRST_NODE_TAG;
		  ClUint32T tmpTag ;
          /* put tag for first node */
          STREAM_IN_BOUNDS_HTONL(stream->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));
        } 
      if(idx==COR_LAST_NODE_IDX) 
        {
          ClUint32T tag = COR_LAST_NODE_TAG;
		  ClUint32T tmpTag;
          /* put tag for last node */
          STREAM_IN_BOUNDS_HTONL(stream->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));
        }
    }
  if(element && cookie)
    {
      MArrayNode_h this = (MArrayNode_h) element;

      if(this->groups.head && this->groups.tail)
        {
          CORPackObj_t packObject;
          CORPackObj_h packObj = &packObject;

          /*Store these values temporarily. So that this can be restored
            back if the node is not packed, due to flags or something.*/
	  	  /*** HTONL ****/
		  ClUint32T tmpIdx ;	   
		  ClUint32T id ;
          STREAM_IN_BOUNDS_HTONL(stream->pBufferHandle, &idx, tmpIdx, sizeof(tmpIdx));
          STREAM_IN_BOUNDS_HTONL(stream->pBufferHandle, &this->id, id, sizeof(id));
	  if( (stream->type == OBJTREE) &&  this->id != 1){
	    clCorMoIdAppend(&gUnpackMOId, this->id, idx);
	  }
          
#ifdef DEBUG
          STREAM_IN_STR(stream->pBufferHandle, this->name);
#else
          STREAM_IN_STR(stream->pBufferHandle, tmpPtr);  /* @Gagan -- No need to check for endianness for strings*/
#endif
          /* first stream in the node data */
          /* @todo, need to mention if streamed or not */
          if(stream->pThis && stream->pThis->fpPackNode)
            {
                packObj->pBufH = stream->pBufferHandle;
				packObj->flags = stream->flags;
                ret=(*stream->pThis->fpPackNode)(idx,
                                               this->data, 
                                               (void * *)&packObj);
            }
          if(ret==CL_OK/* || ret == CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK)*/)
            {
		ClRcT rc =CL_OK;
#ifdef CL_COR_REDUCE_OBJ_MEM
              	/* stream in the vector data */
            	corVectorPackFPSet(this->groups, 0);
#endif
            	rc=corVectorPack(&this->groups, (void *)packObj);
              	if(rc==CL_OK)
                {
                    /* now do a walk and process nodes */
		    if(ret == CL_OK){
                        ret = _mArrayVectorPack(COR_FIRST_GROUP_IDX, 0, cookie);
                	ret = corVectorWalk(&(this)->groups, _mArrayVectorPack, cookie);
                        if(ret != CL_OK && ret != CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK))
                            return ret;
                  	ret = _mArrayVectorPack(COR_LAST_GROUP_IDX, 0, cookie);
			gUnpackMOId.depth--;
		    }
                }
            }
	    else
	    {
		if(ret == CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK))
		{
	    	    ret = CL_OK;
                    gUnpackMOId.depth--;
		}
	    }
	}
    }
  return ret;
}

static ClRcT
_mArrayDataPack(ClUint32T idx, void * element, void ** cookie)
{
    ClRcT ret = CL_OK;
    MArrayStream_h stream = (MArrayStream_h) cookie;
    CL_FUNC_ENTER();
    if (!element && cookie)
    {
        if (idx == COR_FIRST_DATA_IDX)
        {
            ClUint32T tag = COR_FIRST_DATA_TAG;
			ClUint32T tmpTag ;
            STREAM_IN_BOUNDS_HTONL( stream->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));
        }
        if (idx == COR_LAST_DATA_IDX)
        {
            ClUint32T tag = COR_LAST_DATA_TAG;
			ClUint32T tmpTag ;
            STREAM_IN_BOUNDS_HTONL( stream->pBufferHandle, &tag, tmpTag, sizeof(tmpTag));
        }
    }

    if (element && cookie)
    {
	CORPackObj_t packObject;
	CORPackObj_h packObj = &packObject;

	packObj->flags = stream->flags;
        packObj->pBufH = stream->pBufferHandle;

        ret = (stream->pThis->fpPackData)(idx, element, (void * *)&packObj);
    }
    CL_FUNC_EXIT();
    return ret;
}


/** 
 * [Internal] mArrayVector Unpack.
 *
 * API to unpack mArrayVector. API to unpack the mArray Vector and its
 * information.
 */
static ClRcT
_mArrayVectorUnpack(ClUint32T idx,
                    void *     element, 
                    void **    cookie
                    )
{
  ClRcT ret = CL_OK; 
  MArrayStream_h context = (MArrayStream_h)cookie;

  if (!element && cookie)
  {
      ClUint32T tag;
      ClUint32T expect = (idx == COR_FIRST_GROUP_IDX) ? COR_FIRST_GROUP_TAG:
          (idx == COR_LAST_GROUP_IDX) ? COR_LAST_GROUP_TAG:0;
      STREAM_PEEK(&tag, (context->buf), sizeof(tag));
	  tag = ntohl(tag);
      if (tag != expect)
      {
          ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_TAG);
	  CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Expected Tag 0x%x, received 0x%x\n",expect, tag));
      }
      else
      {
          STREAM_OUT_BOUNDS_NTOHL(&tag, context->buf, sizeof(tag), context->size);
      }
  }
  
  if(element && cookie)
    {
      ClUint32T tmp =0 ;
      MArrayVector_h this   = (MArrayVector_h)element;

      if(context->size > 0) 
        {
          ClUint32T tag = COR_VECTOR_TAG;
          /* First verify the tag, index, before picking up the data
           */
          STREAM_PEEK(&tmp, (context->buf), sizeof(tmp));
		  tmp = ntohl(tmp);
#if 1
          if(tag == COR_VECTOR_TAG && idx == tmp ) 
#else
          if (1)
#endif
            {
              CORVector_h tvec;
              /* read the index */
              STREAM_OUT_BOUNDS_NTOHL(&tmp,(context->buf),sizeof(tmp),(context->size));
              /* read the navigable / not field */
              STREAM_OUT_BOUNDS_NTOHS(&this->navigable,(context->buf), 
                                sizeof(this->navigable),
                                (context->size));
              STREAM_OUT_BOUNDS_NTOHL(&this->numActiveNodes,(context->buf), 
                                sizeof(this->numActiveNodes),
                                (context->size));
              if(this->navigable)
                {
                  ClUint32T size = context->size;
                  
#ifdef CL_COR_REDUCE_OBJ_MEM
                  /* stream out the vector data */
                  corVectorUnpackFPSet(this->nodes, 0);
#endif
                  ret=corVectorUnpack(&this->nodes, context->buf, &size);
                  if(ret==CL_OK)
                    {
                      context->buf += size;
                      context->size -= size;
                      tvec = ( (MArrayStream_h)cookie)->vec;
                      ( (MArrayStream_h)cookie)->vec = &this->nodes;
                      ret = _mArrayNodeUnpack(COR_FIRST_NODE_IDX, 0, cookie);
                      ret = corVectorWalk(&this->nodes, _mArrayNodeUnpack, cookie);
                      ret = _mArrayNodeUnpack(COR_LAST_NODE_IDX, 0, cookie);
                      ( (MArrayStream_h)cookie)->vec = tvec;
                    }
                }
              else
                {
                  if(context->pThis && context->pThis->fpUnpackData)
                    {
                      ClUint32T size = context->size;
                      
#ifdef CL_COR_REDUCE_OBJ_MEM
                      corVectorUnpackFPSet(this->nodes, 0);
#endif
                      ret=corVectorUnpack(&this->nodes, context->buf, &size);
                      if(ret==CL_OK)
                        {
                          context->buf += size;
                          context->size -= size;
                          tvec = ( (MArrayStream_h)cookie)->vec;
                          ( (MArrayStream_h)cookie)->vec = &this->nodes;
                          ret = _mArrayDataUnpack(COR_FIRST_DATA_IDX, 0, cookie);
                          ret = corVectorWalk(&this->nodes, _mArrayDataUnpack, cookie);
                          ret = _mArrayDataUnpack(COR_LAST_DATA_IDX, 0, cookie);
                          ( (MArrayStream_h)cookie)->vec = tvec;
                        }
                    }
                }
            }
          else
            {
            }
        }
    }
      
  return ret;
}


/** 
 * [Internal] Node Pack.
 *
 * API to pack mArrayNode.  cookie points to a valid (MArrayStream)
 * object which contains the MArray_h handle, which inturn points to
 * the data and node pack functions provided by the user.
 *
 */
static ClRcT
_mArrayNodeUnpack(ClUint32T idx,
                  void * element, 
                  void ** cookie
                  )
{
  ClRcT ret = CL_OK;
  MArrayStream_h stream = (MArrayStream_h)cookie;

  if(!element && cookie)
    {
      ClUint32T tag;
      ClUint32T expect = (idx==COR_FIRST_NODE_IDX)?COR_FIRST_NODE_TAG:
        (idx==COR_LAST_NODE_IDX)?COR_LAST_NODE_TAG:0;

      /* first check for its presence */
      STREAM_PEEK(&tag, (stream->buf), sizeof(tag));
	  tag = ntohl(tag);
      if(tag!=expect)
        {
          ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_TAG); 
        }
      else
        {
          STREAM_OUT_BOUNDS_NTOHL(&tag, stream->buf, sizeof(tag), stream->size);
        }
    }
  
	if(cookie && element && (stream->size > 0))
	{
	    ClRcT rv  = CL_OK;	
	    static ClUint32T   tmpId, tmpIdx, found, nodeExists, instanceId;
	    MArrayNode_h this = (MArrayNode_h) element;
	    Byte_h buf;
		    
	    /* First verify the index, before picking up the data
	    */
	    if(!found)
	    {
		STREAM_PEEK(&tmpIdx, (stream->buf), sizeof(idx));
		tmpIdx = ntohl(tmpIdx);
   		STREAM_PEEK(&tmpId, (stream->buf + sizeof(idx)), sizeof(idx));
		tmpId = ntohl(tmpId);
		if(tmpIdx == COR_LAST_GROUP_TAG || tmpIdx == COR_LAST_NODE_TAG)
		{
		    return CL_COR_SET_RC(CL_COR_UTILS_ERR_FOUND_END_TAG);
	  	}	
	 	CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Node id is 0x%x, index passed is %d, received is 0x%x\n",tmpId, idx, tmpIdx));
	  	if(nCount){
	  	    rv = _modifyPath(stream->vec, tmpId, tmpIdx, stream->type, &nodeExists, &instanceId);
		    found = 1;
		    if(rv != -1){
		        tmpIdx = rv;
		    }
		}
		else
		{
		    nCount = 1;
		}
	    }
            /*tmpIdx tells the location where the information was at the sending side.
              We modify it to get the location at the receiver where to unpack.
              idx is the current location in the corVector while unpacking.
              If we have already crossed idx, we need to return to proper location*/
	    if(idx >= tmpIdx && idx != COR_LAST_NODE_TAG)
            {
                if(idx > tmpIdx){
                    this = (MArrayNode_h)corVectorGet(stream->vec, tmpIdx);
                }
	        found = 0;	
                buf = stream->buf;
                /* read the index */
                STREAM_OUT_NTOHL(&tmpIdx, (stream->buf), sizeof(idx));
                STREAM_OUT_NTOHL(&this->id,stream->buf,sizeof(this->id));
		if(!nodeExists)
		    _updateNodeInstance(this->id, tmpIdx, stream->type, APPEND_IN_MOID);
                /* read the name */
#ifdef DEBUG
                STREAM_OUT_STR(this->name, stream->buf, sizeof(this->name));
#else
                {
                    char* tmp = 0;
                    STREAM_OUT_STR(tmp, stream->buf, 0);
                }
#endif
                /*
                * note, since init may not be called, we need to make
                * sure that data points to the appropriate location
                * (buffer)
                */
                (this)->data = &((this)->data);
                (this)->data = (char *)((this)->data) + sizeof((this)->data);
                /* first stream out the node data */
                /* @todo, check if streamed or not - to be done at pack */
                if(stream->pThis && stream->pThis->fpUnpackNode)
                {
                    CORUnpackObj_t unpackObject;
                    CORUnpackObj_h pUnpackObj = &unpackObject;
		    /* This function will only be called if there is new node or we want to 
		    override data of existing node. If we want the data of existing node to
		    remain as it is, this function will not be called.*/
                    pUnpackObj->data = &stream->buf;
                    pUnpackObj->instanceId= instanceId;
                    ret=(*stream->pThis->fpUnpackNode)(tmpIdx, 
                                                        this->data, 
                                                        (void **)&pUnpackObj);
                }
                if(ret==CL_OK)
                {
                    CORVector_h tvec;
                    ClUint32T size;                  
                    stream->size -= (stream->buf - buf);
                    size = stream->size;
              
#ifdef CL_COR_REDUCE_OBJ_MEM
                    /* stream out the vector data */
                    corVectorUnpackFPSet(this->groups, 0);
#endif
                    ret=corVectorUnpack(&this->groups, stream->buf, &size);
                    if(ret==CL_OK)
                    {
                        stream->buf += size;
                        stream->size -= size;
                        /* now do a walk and unpack nodes */
                        tvec = ( (MArrayStream_h)cookie)->vec;
                        ( (MArrayStream_h)cookie)->vec = &this->groups;
                        ret = _mArrayVectorUnpack(COR_FIRST_GROUP_IDX, 0, cookie);
                        ret = corVectorWalk(&(this)->groups,_mArrayVectorUnpack, cookie);
                        ret = _mArrayVectorUnpack(COR_LAST_GROUP_IDX, 0, cookie);
                        ( (MArrayStream_h)cookie)->vec = tvec;
		        if(!nodeExists)
		  	    _updateNodeInstance(this->id, tmpIdx, stream->type, REDUCE_MOID);		
                    }
                }
            }
            else if(idx==COR_LAST_NODE_TAG)
            {
                /* inform that the list have been navigated */
                ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_FOUND_END_TAG);
            }
        }
  
  return ret;
}

static ClRcT
_mArrayDataUnpack(ClUint32T idx, void * element, void ** cookie)
{
    ClRcT ret = CL_OK;
    MArrayStream_h stream = (MArrayStream_h) cookie;

    if (!element && cookie)
    {
        ClUint32T tag;
        ClUint32T expect = (idx == COR_FIRST_DATA_IDX) ? COR_FIRST_DATA_TAG:
            (idx == COR_LAST_DATA_IDX) ? COR_LAST_DATA_TAG:0;

        STREAM_PEEK(&tag, (stream->buf), sizeof(tag));
		tag = ntohl(tag);
        if (tag != expect)
        {
            ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_TAG);
        }
        else
        {
            STREAM_OUT_BOUNDS_NTOHL(&tag, stream->buf, sizeof(tag), stream->size);
        }
    }

    if (element && cookie && (stream->size > 0))
    {
        ClUint32T nodeExists = 0, instanceId = MSO; 
        Byte_h buf = stream->buf; 
        CORUnpackObj_t unpackObject;
        CORUnpackObj_h pUnpackObj = &unpackObject;
        pUnpackObj->data = &stream->buf;
        
        _modifyPath(stream->vec, 0, idx, stream->type, &nodeExists, &instanceId);

        pUnpackObj->instanceId = instanceId;

        (stream->pThis->fpUnpackData)(idx, element, (void * *)(&pUnpackObj)); 
        stream->size -= (stream->buf - buf);
    }
    return ret;
}



/** 
 * [Internal] Node Show.
 *
 * API to display mArrayNode.
 *
 */
#if COR_TEST
static ClRcT
_mArrayNodeShow(ClUint32T idx,
                void * element, 
                void ** cookie
                )
{
  int** lvls = (int**) cookie;
  /* receive the marker calls, an idx -1 means end and 0 means beginning
   */
  if(!element)
    {
      if(idx==COR_FIRST_NODE_IDX)
        {
          (**lvls)+=4;
        } 
      if(idx==COR_LAST_NODE_IDX) 
        {
          (**lvls)-=4;
        }
    }
  else
    {
      MArrayNode_h this = (MArrayNode_h) element;

      if(idx==0) clOsalPrintf("\n");
      COR_PRINT_SPACE(**lvls);
      clOsalPrintf("[%d/0x%x] ", idx,(this->id));
#ifdef DEBUG
      clOsalPrintf("%s ",(this->name));
#endif
      clOsalPrintf("(grps:%d 0x%x)", 
               corVectorSizeGet(this->groups),
               (this)->data?*((int*)(this)->data):0);
    }
  
  return CL_OK;
}

#endif

static ClRcT
_mArrayNodeShow(ClUint32T idx,
                void * element, 
                void ** cookie
                )
{
  ClCharT corStr[CL_COR_CLI_STR_LEN];
  ClBufferHandleT *pMsgHdl = *(ClBufferHandleT **)cookie;
  /* receive the marker calls, an idx -1 means end and 0 means beginning
   */
  if(!element)
    {
      if(idx==COR_FIRST_NODE_IDX)
        {
          (lvls)+=4;
        } 
      if(idx==COR_LAST_NODE_IDX) 
        {
          (lvls)-=4;
        }
    }
  else
    {
      MArrayNode_h this = (MArrayNode_h) element;

      if(idx==0)
      {
          corStr[0]='\0';
          sprintf(corStr, "\n");
          clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
      }
      corStr[0]='\0';
      COR_PRINT_SPACE_IN_BUFFER(lvls, corStr);
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));

      corStr[0]='\0';
      sprintf(corStr, "[%d/0x%x] ", idx,(this->id));
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
#ifdef DEBUG
      corStr[0]='\0';
      sprintf(corStr, "%s ",(this->name));
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
#endif
      corStr[0]='\0';
      sprintf(corStr, "(Max-Inst:[%d])",
               (this)->data?((_CORMOClass_h)(this)->data)->maxInstances:0);
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    }
  
  return CL_OK;
}

/** 
 * [Internal] Data Show.
 *
 * API to display data node (just a peek into the first 4 bytes).
 *
 */
#ifdef COR_TEST
static ClRcT
_mArrayDataShow(ClUint32T idx,
                void * element,
                void ** cookie
                )
{
  int** lvls = (int**) cookie;

  if(element)
    {
      int* data = (int*) element;
      if(idx%16==0) 
        {
          COR_PRINT_SPACE(**lvls);
        }
      clOsalPrintf("0x%x ",(*data));
    }
    
  return CL_OK;
}
#endif
static ClRcT
_mArrayDataShow(ClUint32T idx,
                void * element,
                void ** cookie
                )
{
  ClCharT corStr[CL_COR_CLI_STR_LEN];
  ClBufferHandleT *pMsgHdl = *(ClBufferHandleT **)cookie;

  if(element)
    {
      int* data = (int*) element;
      if(idx%16==0) 
      {
         corStr[0]='\0';
         COR_PRINT_SPACE_IN_BUFFER(lvls, corStr);
         clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));

      }
      corStr[0]='\0';
      sprintf(corStr, "0x%x ",(*data));
      clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));
    }
    
  return CL_OK;
}


/* Finds the slot where the next information needs to be placed during unpack.
    vec : Vector start position in the node whose cor vector has node with given "class id"
    id : class id
    index : index at which it was during pack function
    type : moTree or ObjTree
    nodeExists[out] : whether the node already exists or not
    instanceId[in/out] : (IN )MSO or not
                         (OUT) In dmObject what is the instance id for this object

returns:
    index where new node needs to be unpacked.
    -1 in case of error.
    
   */

ClInt32T _modifyPath(CORVector_h vec, ClUint32T id, ClUint32T index, ClUint32T type, ClUint32T *nodeExists, ClUint32T *instanceId)
{
    ClUint32T idx = 0;

    if(type == 1 && *instanceId == MSO)
        return -1;
    
    if(vec && vec->head ) 
    {
        CORVectorNode_h node = vec->head;
        MArrayNode_h element = node->data;
        ClUint32T nodeId = element->id;

        *nodeExists = 0;

        if(type  == OBJTREE)
        {
            if( (element = corVectorExtendedGet(vec, index) ) == NULL)
            {
                return -1;
            }
            else
            {
                CORObject_h obj = NULL;
                DMObjHandle_t dmObj;
                if(*instanceId != MSO)
                {
                    nodeId = element->id;
                    if(nodeId){
                        *nodeExists = 1;
                        obj = element->data;

                        if(obj){
                            dmObj = obj->dmObjH;
                            *instanceId = dmObj.instanceId;
                        }
                    }
                    else{
                        *instanceId = -1;
                    }
                }
                else
                {
                    obj = (CORObject_h)element;
                    dmObj = obj->dmObjH;
                    if(dmObj.classId)
                    {
                        *nodeExists = 1;
                        *instanceId = dmObj.instanceId;
                    }
                    else{
                        *instanceId = -1;
                    }
                }
            }
            return index;
        }
        if(index == 0 && nodeId == 0)
        {
            return 0;
        }

        /*for MOTree search if the node is there or not*/
        if(corVectorNodeFind(vec, id, index, &idx) == CL_OK)
            return idx;
        else
            return -1;
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        return 0;
    }
}

#ifdef DISTRIBUTED_COR
ClRcT _mArrayNodeRemap(ClUint32T idx,
                        void * element,
                        void ** cookie)
{
    ClRcT ret = CL_OK;
    static ClCorMOClassPathT moPath;

    MArrayNode_h this = (MArrayNode_h) element;
    if(this && this->groups.head && this->groups.tail)
    {
	CORVector_h vector;
	if(this->id != 1)/*Leave the root*/
	   clCorMoClassPathAppend(&moPath, this->id);
	vector = &(this)->groups;
        ret = corVectorWalk(&(this)->groups, _mArrayVectorRemap, cookie);
#ifdef DEBUG
	clCorMoClassPathShow(&moPath);
#endif
        _corObjRemap(vector, &moPath);
	if(this->id != 1)
	    moPath.depth--;
    }
    return ret;
}


ClRcT _mArrayVectorRemap(ClUint32T idx,
                        void * element,
                        void ** cookie)
{
    ClRcT ret = CL_OK;
    if(element)
    {
        MArrayVector_h this   = (MArrayVector_h)element;
        if(this->nodes.head && this->nodes.tail)
        {
            if(this->navigable)
            {
                ret = corVectorWalk(&this->nodes, _mArrayNodeRemap, cookie);
            }
        }
    }
    return ret;
}

ClRcT _mArrayNodeUpdate(ClUint32T idx,
                        void * element,
                        void ** cookie)
{
    ClRcT ret = CL_OK;
	static ClCorMOClassPathT moPath;

    MArrayNode_h this = (MArrayNode_h) element;
    if(this && this->groups.head && this->groups.tail)
    {
		if(this->id != 1)/*Leave the root*/
			clCorMoClassPathAppend(&moPath, this->id);
		_CORMOClass_h mo = (_CORMOClass_h)this->data;
		_corMONodeUpdateFun(moPath, &mo->numChildren);
        ret = corVectorWalk(&(this)->groups, _mArrayVectorUpdate, cookie);
		if(this->id != 1)
			moPath.depth--;
	}
	return ret;
}

ClRcT _mArrayVectorUpdate(ClUint32T idx,
                           void * element,
                           void ** cookie)
{
    ClRcT ret = CL_OK;
    if(element)
    {
        MArrayVector_h this   = (MArrayVector_h)element;
        if(this->nodes.head && this->nodes.tail)
        {
            if(this->navigable)
            {
                ret = corVectorWalk(&this->nodes, _mArrayNodeUpdate, cookie);
            }
        }
    }
    return ret;
}

ClRcT _corObjRemap(CORVector_h vec, ClCorMOClassPathPtrT moPath)
{
    ClCorMOClassPathT tmpPath = *moPath;
	ClRcT ret = CL_OK;
    if(moPath)
    {
        clCorMoClassPathAppend(&tmpPath, 1);
#ifdef DEBUG
	clCorMoClassPathShow(&tmpPath);
#endif
        if(vec && vec->head )
        {
            ClUint32T idx = 0, id = 0;
            ClUint32T *nodeId = &id;
            CORVectorNode_h node = vec->head;
            MArrayVector_h this = (MArrayVector_h)(node->data);
            ClUint32T sz = vec->elementsPerBlock * vec->blocks;
            ClUint8T *marked = (ClUint8T *)clHeapCalloc(1,sz);
	    if(NULL == marked)
	    {
		return CL_ERR_NO_MEMORY;
	    }

            while(node){
        	if(this->nodes.head && this->nodes.tail)
		{
               	    if( this->navigable && !marked[idx] )
                    {
		        ret = corVectorWalk(&this->nodes, mArrayNodeIdGet, (void * *)&nodeId);
			if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE)){
			    tmpPath.node[tmpPath.depth-1] = *nodeId;
			    if( (ret = mArrayId2Idx(moTree, (ClUint32T *)tmpPath.node, tmpPath.depth) ) == CL_OK)
			    {
			        ClUint32T slot = tmpPath.node[tmpPath.depth-1];
				if(slot != idx)
				{
				    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Swapping taking place.\n") );
				    swap(vec, slot, idx);
				    marked[slot] = 1;
				    continue;
				}
			    }
			    tmpPath = *moPath;
        		    clCorMoClassPathAppend(&tmpPath, 1);
			}
                    }
		}
		{
		    void * vp = this;
                    vp = (char *)vp + vec->elementSize;
		    this = vp;
		}
                idx ++;
                if(idx >= sz)
                    break;
                if(!(idx % vec->elementsPerBlock))
                {
                    node = node->next;
                    this = node->data;
                }
            }
            clHeapFree(marked);
        }
    }
    return ret;
}

ClRcT swap(CORVector_h vec, ClUint32T slot, ClUint32T idx)
{
    ClUint32T mark1 = 0, mark2 = 0;
    CORVectorNode_h node1,  node2;
	MArrayVector_h mVec1, mVec2;
    ClUint32T sz = vec->elementsPerBlock * vec->blocks;

	node1 = node2 = (vec)->head;
	mVec1 = mVec2 = (MArrayVector_h)(node1->data);

    if(slot >= sz || idx >= sz)
        return 1;/*Sachin : put proper error value*/
    for(mark1 = 0; mark1 < slot; mark1++)
    {
        mVec1 += vec->elementSize;
        mark1++;
        if(!(mark1 % vec->elementsPerBlock))
        {
            node1 = node1->next;
            mVec1 = node1->data;
        }
    }
    for(mark2 = 0; mark2 < slot; mark1++)
    {
        mVec2 += vec->elementSize;
        mark1++;
        if(!(mark1 % vec->elementsPerBlock))
        {
            node2 = node2->next;
            mVec2 = node2->data;
        }
    }
    MArrayVector_h mVec;
    mVec = mVec1;
    mVec1 = mVec2;
    mVec2 = mVec;
    return CL_OK;
}

ClRcT _corMONodeUpdateFun(ClCorMOClassPathT moPath, ClUint32T *totalChildren)
{
	ClUint32T numChild = 0;
	ClRcT rc = CL_OK;

    if( (rc = _clCorMoClassPathFirstChildGet(&moPath) ) == CL_OK)
    {
		numChild++;
        /* get the siblings, till there are no more left.*/
        while(_clCorMoClassPathNextSiblingGet(&moPath) == CL_OK){
        	numChild++;
		}
    }
	if(moPath.depth != 1)
		*totalChildren = numChild + 1;
	else
		*totalChildren = numChild;
	return rc;
}

#endif

ClRcT _updateNodeInstance(ClUint32T id, ClUint32T index, ClUint32T type, ClUint32T init)
{
    ClRcT ret = CL_OK;
    static ClCorMOIdT moId;
    ClCorMOClassPathT moPath;
    MArrayNode_h node = 0;

    if(type  == OBJTREE)
    {
        if(init)
        {
            if(id != 1)
                clCorMoIdAppend(&moId, id, index);
            ret = clCorMoIdToMoClassPathGet(&moId, &moPath);
            if(ret != CL_OK)
                    return ret;
            node = corMOTreeNodeFind(moTree, &moPath);
            if(NULL == node)
                return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
            else
            {
                _CORMOClass_h mo = node->data;
                mo->instances++;
            }
        }
        else if(id != 1){
            moId.depth--;
        }
    }
    /*clCorMoIdShow(&moId);*/
    return ret;
}
