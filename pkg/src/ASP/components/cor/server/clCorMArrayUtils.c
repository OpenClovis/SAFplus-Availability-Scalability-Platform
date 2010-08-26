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
 * File        : clCorMArrayUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains mArray Utils API's & Definitions
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCorErrors.h>

/*Internal Headers*/
#include "clCorMArray.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* local definitions */

static ClRcT _mArrayGroupFind(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayNodeFind(ClUint32T idx, void * element, void ** cookie);

static ClRcT _mArrayGroupIdxFind(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayNodeIdxFind(ClUint32T idx, void * element, void ** cookie);
static ClRcT _mArrayGroupIdIdxFind(ClUint32T idx, void * element, void ** cookie);

static ClRcT _mArrayNodeNextFind(ClUint32T idx, void * element, void ** cookie);

/* This file contains routines that are id and name based and are
 * basically utils on top of mArray nodes  
 */

/** 
 * Given an MArray, list of Id's (not indexes), Find the node that
 * matches.
 */
MArrayNode_h
mArrayIdNodeFind(MArray_h this,
                 ClUint32T* list,  /* list of Id's */
                 ClUint32T n       /* no of id's passed */
                 )
{
    int i;

    if(this && list && n>0)
      {
        MArrayNode_h tmp = this->root;
        for(i=0;tmp && i<n;i++)
          {
            tmp = mArrayNodeIdNodeFind(tmp, list[i]);
          }
        return (tmp);
      }
    return 0;
}


/** 
 * Given an MArray, list of Id's (not indexes), list of indexes, Find
 * the node that matches.
 */
MArrayNode_h
mArrayIdIdxNodeFind(MArray_h this,
                    ClUint32T* list,  /* list of Id's */
                    ClUint32T* idx,   /* list of indexes */
                    ClUint32T n       /* no of id's passed */
                    )
{
    int i;

    if(this && list && idx && n>0)
      {
        MArrayNode_h tmp = this->root;
        for(i=0;tmp && i<n;i++)
          {
            tmp = mArrayNodeIdIdxNodeFind(tmp, list[i], idx[i]);
          }
        return (tmp);
      }
    
    return 0;
}

/* convert given id's to corresponding index  */


/**
 *  Return  idx given a node ID.
 *
 *  This routine returns the node idx corresponding to the ID at each
 * level in the passed list.
 *
 *  @param this  - tree handle
 *  @param list  - [IN/OUT] list of class ids/ list of clas indexes (returned)
 *  @param n     - number of id's in the list
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM) on error.
 *           CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY) on unknown key.
 */
ClRcT
mArrayId2Idx(MArray_h this,
             ClUint32T* list,  /* list of Id's */
             ClUint32T n       /* no of id's passed */
             )
{
    ClRcT ret = CL_OK;
    int i;

    if(this && list && n>0)
      {
        MArrayNode_h tmp = this->root;
        for(i=0;tmp && ret==CL_OK && i<n;i++)
          {
            ClUint32T *val = &list[i];
            ClUint32T id = list[i];

            ret = corVectorWalk(&tmp->groups, 
                                _mArrayGroupIdxFind, 
                                (void **)&val);
            if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
              {
                tmp = mArrayNodeIdNodeFind(tmp, id);
                ret = CL_OK;
              }
            else
              {
                ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY);
              }
          }
      } else 
        {
          ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
    
    return ret;
}

/** 
 * Given mArray, groupid and list of indexes, convert the indexes to
 * id.
 */
ClRcT
mArrayIdx2Id(MArray_h this,
             ClUint32T gid,    /* group to look for */
             ClUint32T* list,  /* list of Indexes's */
             ClUint32T n       /* no of indexes passed */
             )
{
    ClRcT ret = CL_OK;
    int i;

    if(this && list && n>0)
      {
        MArrayNode_h tmp = this->root;
        for(i=0;tmp && i<n;i++)
          {
            MArrayVector_h vec = mArrayGroupGet(tmp, gid);
            if(vec && vec->nodes.head && vec->nodes.tail && vec->navigable)
              {
                tmp = (MArrayNode_h) corVectorGet(&(vec)->nodes, list[i]);
                if(tmp)
                  {
                    list[i] = tmp->id;
                  }                
              }
          }
        if(!tmp)
          {
            ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY);
          }
      } else 
        {
          ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
    
    return ret;
}

/** 
 * Given mArrayNode, id and index find the mArrayNode that matches
 * it. Basically the id have to match the group and then index have to
 * be fetched within it.
 */
MArrayNode_h
mArrayNodeIdIdxNodeFind(MArrayNode_h this,
                        ClUint32T   id,
                        ClUint32T   idx
                        )
{
  ClRcT ret;

    if(this && id>0)
      {
        ClUint32T* val[2]={&id,&idx};
        ret = corVectorWalk(&this->groups, _mArrayGroupIdIdxFind, (void **)&val);
        if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
          {
            /* found the node, so return back the address */
            return ((MArrayNode_h)val[0]);
          }
      }
    
    return 0;
}

/** 
 * Given mArrayNode, group id and childId (it can be '-1' to find
 * first child), find the next child in the list.  If childId is -1,
 * then first node is returned (first child).  
 *
 *  @param gid     - [In/OUT]  gid will be filled up with the new gid.
 *  @param childId -  [In/OUT] ChildId will be filled up with 
 *          the new index for the caller.
 */
MArrayNode_h
mArrayNodeNextChildGet(MArrayNode_h this,
                       ClUint32T*  gid,
                       ClInt32T*  childIdx
                       )
{
    ClRcT ret;
    ClUint32T* val[2]={(ClUint32T *)childIdx,0};

    if(this && childIdx)
    {
        /* call only if the vector is inited */
        if(this->groups.head && this->groups.tail)
	{
            /* first find the group, given gid */
            MArrayVector_h grpNode = NULL;
	    /* Search in all the nodes.*/
            while((grpNode = (MArrayVector_h) corVectorGet(&this->groups, *gid)) != NULL)
	    {
            if(grpNode && 
               grpNode->nodes.head && grpNode->nodes.tail &&
               grpNode->navigable)
              {
                ret = corVectorWalk(&grpNode->nodes,
                                    _mArrayNodeNextFind,
                                    (void **)&val);
                if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
                  {
                    /* found the node, so return back the address */
                    return ((MArrayNode_h)val[1]);
                  }
              }
	      /* reset the child Idx to -1 */
	      *childIdx = -1;
	      (*gid)++;
	    }
	}
    }
    return 0;
}

/** 
 * Given mArrayNode, locate the node that matches the id and return it
 * back.
 */
MArrayNode_h
mArrayNodeIdNodeFind(MArrayNode_h this,
                     ClUint32T   id
                     )
{
    ClRcT ret = CL_OK;

    if(this && id>0)
      {
        ClUint32T* val=&id;
        ret = corVectorWalk(&this->groups, _mArrayGroupFind, (void **)&val);
        if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
          {
            /* found the node, so return back the address */
            return ((MArrayNode_h)val);
          }
      }
    
    return 0;
}

/**
 * Given mArrayNode and group id, locate the first non-zero data element in vector.
 */
void *
mArrayDataNodeFirstNodeGet(MArrayNode_h this,
                         ClUint32T   id)
{

  /* call only if the vector is inited */     
    if(this && this->groups.head && this->groups.tail) 
    { 
	    /* first find the group, given gid */
	    MArrayVector_h grpNode = mArrayGroupGet(this, id);

   	    if(grpNode &&
	     		    grpNode->nodes.head && grpNode->nodes.tail &&
			    grpNode->navigable != MARRAY_NAVIGABLE_NODE)
	    {
		    ClUint32T i=0;
		    void * element;
		    ClUint32T elementSize = corVectorElementSizeGet(grpNode->nodes);
        
		    for(i=0;i<corVectorSizeGet(grpNode->nodes);i++)
		    {
			    element = corVectorGet(&grpNode->nodes, i);
			    if(element)
	 		    {
		 		    ClUint8T data[elementSize];
			 	    memset(data, 0, elementSize);
				    /* if it is a non-zero data, return the address. */
				    if(memcmp((const void *)data, (const void *)element, elementSize))
		 			    return element;
			    }
			    else
			    {
				    /* in no case it should come in else..*/
				    return 0;
			    }
	
		   } /* end of for loop*/
           }
    }
    return 0;
}
                                                                                                                            

#if 0
ClRcT
mArrayNodeId2Idx(MArrayNode_h this, ClUint32T* nodeId)
{
	ClRcT ret = CL_OK;

	if(this && nodeId)
	{
		ClUint32T *val = nodeId;
		CORVector_h tmp = &this->groups;

		if(tmp && tmp->head)
		{
		    ret = corVectorWalk(&this->groups,
		                        _mArrayGroupIdxFind,
	                                (void **)&val);
		}
		if(ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
		{
	    	    ret = CL_OK;
		}
		else
		{
                    if(!this->groups.head && !this->groups.tail)
                    {
            	        ret = mArrayNodeInit(this);
                    }
		    ClUint32T size = tmp->elementsPerBlock * tmp->blocks;
		    ClUint32T idxToExtend = size + tmp->elementsPerBlock - 1;
		
		    if(corVectorExtend(tmp, idxToExtend) == CL_OK)
		    {
			*nodeId = size;
			ret = CL_OK;
		    }
		    else
		        ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY);
		}
	}
	else
	{
	    ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
	}
	return ret;
}
#endif

ClRcT mArrayNodeIdGet(ClUint32T idx,
        	       void * element,
                       void ** cookie)
{
    if(element && cookie)
    {
        MArrayNode_h this = (MArrayNode_h) element;
        if(this->groups.head && this->groups.tail){
            **(ClUint32T **)cookie = this->id;
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
        }
    }
    return CL_OK;
}

/* ----- [Internal Functions] -------------------------------- */

/*
 * cookie contains integer (id) that has to be found. Cookie is
 * updated with the index at which the id is found.
 */
static ClRcT
_mArrayNodeIdxFind(ClUint32T idx,
                   void *     element, 
                   void **    cookie
                   )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayNode_h this = (MArrayNode_h) element;
        if(this->id == **((ClUint32T**)cookie))
          {
            /* overload cookie with the new found idx */
            **(ClUint32T**)cookie = idx;
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
          }
      }
    return ret;
}


/*
 * cookie contains integer (id) that has to be found. Cookie is
 * updated with the index at which the id is found.
 */
static ClRcT
_mArrayGroupIdxFind(ClUint32T idx,
                 void *     element, 
                 void **    cookie
                 )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayVector_h this   = (MArrayVector_h)element;
        
        /* call only if the vector is inited */
        if(this->nodes.head && this->nodes.tail && this->navigable)
          {
            ret = corVectorWalk(&this->nodes, _mArrayNodeIdxFind, cookie);
          }
      }
    return ret;
}

/*
 * cookie contains integer (id) that has to be found.  Cookie is
 * returned with node address on match.
 */
static ClRcT
_mArrayNodeFind(ClUint32T idx,
                void *     element, 
                void **    cookie
                )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayNode_h this = (MArrayNode_h) element;
        if(this->id == **((ClUint32T**)cookie))
          {
            /* overload cookie with the new found idx */
            *(MArrayNode_h*)cookie = this;
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
          }
      }
    return ret;
}

/*
 * cookie contains integer (id) that has to be found.  Cookie is
 * returned with node address on match.
 */
static ClRcT
_mArrayGroupFind(ClUint32T idx,
                 void *     element, 
                 void **    cookie
                 )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayVector_h this   = (MArrayVector_h)element;
        
        /* call only if the vector is inited */
        if(this->nodes.head && this->nodes.tail && this->navigable)
          {
            ret = corVectorWalk(&this->nodes, _mArrayNodeFind, cookie);
          }
      }
    return ret;
}

/*
 * Cookie contains {idx, id} and will be updated with {nextitemIdx,
 * nodeaddress}
 */
static ClRcT
_mArrayNodeNextFind(ClUint32T idx,
                    void *     element, 
                    void **    cookie
                    )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayNode_h this = (MArrayNode_h) element;
        ClInt32T idxTmp = *(ClInt32T*)cookie[0];

        if(this->id && (ClInt32T)idx > idxTmp)
          {
            /* overload cookie with the new found idx */
            *(ClUint32T*)cookie[0] = idx;
            cookie[1] = element;
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
          }
      }
    return ret;
}


/*
 * cookie contains {id, index}, returns back {nodeaddress, index}
 */
static ClRcT
_mArrayGroupIdIdxFind(ClUint32T idx,
                      void *     element, 
                      void **    cookie
                      )
{
    ClRcT ret = CL_OK;
  
    if(element && cookie)
      {
        MArrayVector_h this   = (MArrayVector_h)element;
        ClUint32T idTmp = **((ClUint32T**)cookie);
        ClUint32T idxTmp = **((ClUint32T**)cookie+1);
        
        /* call only if the vector is inited */
        if(this->nodes.head && this->nodes.tail && this->navigable)
          {
            MArrayNode_h thisNode = (MArrayNode_h) corVectorGet(&this->nodes,
                                                                idxTmp);
            if(thisNode && thisNode->id == idTmp)
              {
                /* overload cookie with the match */
                *(MArrayNode_h*)cookie = thisNode;
                return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
              }
          }
      }
    return ret;
}
