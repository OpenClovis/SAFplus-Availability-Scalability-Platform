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
 * File        : clCorAttrPath.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Utility module that handles the MOID object.
 *****************************************************************************/

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clBitApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorLog.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


/** 
 *  Constructor for ClCorAttrPath. Initializes the memory and returns back an
 *  empty path. The "index" field in all the entries are set to
 *  -1 as default.
 *
 *  @param this      [out] new clCorAttrPath
 *
 *  @returns 
 *    CL_OK        on success <br/>
 *    CL_COR_ERR_NO_MEM  on out of memory <br/>
 * 
 */

ClRcT
clCorAttrPathAlloc(ClCorAttrPathT **this)
{
    ClCorAttrPathT *tmp = 0;
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    tmp = (ClCorAttrPathT *) clHeapAllocate(sizeof(ClCorAttrPathT));
    *this = tmp;
    if(tmp != NULL) 
    {
    /* init stuff here */
        for(i = 0; i < CL_COR_CONT_ATTR_MAX_DEPTH; i++) 
        {
            tmp->node[i].attrId     = CL_COR_INVALID_ATTR_ID;
            tmp->node[i].index = CL_COR_INVALID_ATTR_IDX;
        }
        tmp->depth = 0; 
    } 
    else 
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
        ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }  
  
    CL_FUNC_EXIT();  
    return (ret);
}

/** 
 *  Initialize ClCorAttrPath.
 *
 *  Resets the path info (if present) and re-inits the 
 *  path to empty.
 *
 *  @param this      Attr Path handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */

ClRcT
clCorAttrPathInitialize(ClCorAttrPathT *this)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  /* init stuff here */
    for( i = 0; i < CL_COR_CONT_ATTR_MAX_DEPTH; i++) 
    {
         this->node[i].attrId     = CL_COR_INVALID_ATTR_ID;
         this->node[i].index = CL_COR_INVALID_ATTR_IDX;
    }
    this->depth = 0; 
	this->tmp = 0;  

    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Does byte conversion on attrPath fields.
 */
ClRcT
clCorAttrPathByteSwap(ClCorAttrPathT *this)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  /* init stuff here */
    for( i = 0; i < CL_COR_CONT_ATTR_MAX_DEPTH; i++) 
    {
         this->node[i].attrId   = CL_BIT_SWAP32(this->node[i].attrId);
         this->node[i].index    = CL_BIT_SWAP32(this->node[i].index);
    }
    this->depth = CL_BIT_SWAP32(this->depth); 
  
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Free/Delete the ClCorAttrPath Handle.
 *
 *  Destructor for ClCorAttrPath.  Removes/Deletes the handle and frees back
 *  any associated memory.
 *
 *  @param this ClCorAttrPath handle
 *
 *  @returns 
 *    ClRcT  CL_OK on successful deletion.
 * 
 */
ClRcT
clCorAttrPathFree(ClCorAttrPathT * this)
{
    CL_FUNC_ENTER();
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    clHeapFree(this); /* no need to check null (clHeapFree will take care) */
    CL_FUNC_EXIT();

    return (CL_OK);
}

/** 
 *  Remove nodes after specified level.
 *
 *  Remove all nodes and reset the MO Id till the level specified.
 *
 *  @param this  ClCorAttrPathT 
 *  @param level level to which the ClCorAttrPathT needs to be truncated
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 * 
 */
ClRcT
clCorAttrPathTruncate(ClCorAttrPathT *this, ClInt16T level)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if(level  == this->depth)
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }

  /* Check for current depth against the level */
    if(level < this->depth && level >= 0) 
    {
    /* init stuff here */
        for(i = level; i < this->depth; i++) 
        {
            this->node[i].attrId     = CL_COR_INVALID_ATTR_ID;
            this->node[i].index = CL_COR_INVALID_ATTR_IDX;
        }
        this->depth = (level);
    
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_INVALID_DEPTH));
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
  
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Set the Attribute Id and index at given node.
 *
 *  @param this      ClCorAttrPathT;
 *  @param level     node level
 *  @param attrId    Attribute Id to be set 
 *  @param index     Index to be set 
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 */

ClRcT
clCorAttrPathSet(ClCorAttrPathT *this, 
        ClUint16T level,
        ClCorAttrIdT attrId, 
        ClUint32T  index)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    } 
    else if ( 0 > index)
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
    else if( (0 > attrId)  && (CL_COR_ATTR_WILD_CARD != attrId))
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }
    else if(level <= this->depth)
    {
        /* Check for current depth against the level */
        this->node[level-1].attrId = attrId;
        this->node[level-1].index = index;
    }
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_INVALID_DEPTH));
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
  
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Add an entry to the Attribute Path.
 * 
 * This API adds an entry to the ClCorAttrPathT. The user explicitly
 * specifies the attrId and the index for the entry.
 *
 *  @param this      ClCorAttrPath
 *  @param attrId    Attribute Id 
 *  @param index     index of the attribute.
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br/>
 *       CL_COR_ERR_MAX_DEPTH if max depth exceeded <br/>
 */

ClRcT 
clCorAttrPathAppend(ClCorAttrPathT *this,
           ClCorClassTypeT attrId,
           ClCorInstanceIdT index)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else if ( ( CL_COR_ATTR_WILD_CARD != attrId ) && ( 0 > attrId ) )
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }
	/* JC - In case of moidByTypeAppend, we may want to fill up invalid MO Instance */
    else if (( CL_COR_INDEX_WILD_CARD != index ) && ( 0 > index ))
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

  /* Check for current depth against the default */
  /* Todo: Need to take care of the error scenario later */
    else if(this->depth < CL_COR_CONT_ATTR_MAX_DEPTH) 
    {
        this->node[this->depth].attrId = attrId;
        this->node[this->depth++].index = index;
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MAX_DEPTH));
        ret = CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH);
    }
  
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Get the AttrPath node depth.
 * 
 * Returns the no of nodes in the hierarchy in the AttrPath.
 *
 *  @param this AttrPath handle
 * 
 *  @returns 
 *    ClInt16T number of elements <br/>
 * 
 */
ClInt16T
clCorAttrPathDepthGet(ClCorAttrPathT *this)
{
    CL_FUNC_ENTER();
    if (this == NULL) return -1;    
    return(this->depth);
}

/** 
 * Display the ClCorAttrPath Handle.
 * 
 * This API displays all the entries within the ClCorAttrPath Handle
 *
 *  @param this ClCorAttrPath handle
 * 
 *  @returns 
 *    none
 * 
 */
void
clCorAttrPathShow(ClCorAttrPathT *this)
{
    ClInt32T i = 0;
    ClInt32T like = 0;
    char tmpBuf[16];

    if(this) 
    {
        clOsalPrintf("AttrPath: ");
        for(i=0,like=0;i<this->depth && this->depth <= CL_COR_CONT_ATTR_MAX_DEPTH;i++) 
        {
            if(i<(this->depth-1) &&
               this->node[i].attrId == this->node[i+1].attrId &&
                this->node[i].index == this->node[i+1].index) 
            {
                like++;
            } 
            else 
            {
                sprintf(tmpBuf,"[%d]",like+1);
                clOsalPrintf("(%04x:%04x)%s", 
                         this->node[i].attrId,
                         this->node[i].index,
                         (like>0?tmpBuf:""));
                clOsalPrintf((i<(this->depth-1))?".":"");
                like = 0;
            }
        }
        clOsalPrintf("\n");
    }
}


/** 
 * Get the Attribute Id.
 * 
 * API returns the class attrId that AttrId refers to (the bottom most
 * Attribute attrId in the hierarchy).
 *
 *  @param this ClCorAttrPath handle
 * 
 *  @returns 
 *   ClCorAttrIdT attrId associated <br/>
 */

ClCorAttrIdT
clCorAttrPathToAttrIdGet(ClCorAttrPathT *this)
{
    return(this->node[this->depth-1].attrId);
}


/** 
 * Get the Index.
 * 
 * API returns the Instance that it referrs to (the bottom most attribute
 * id and its index id in the hierarchy).
 *
 *  @param this ClCorAttrPath handle
 * 
 *  @returns 
 *    ClUint32T Index Id associated <br/>
 * 
 */

ClUint32T
clCorAttrPathIndexGet(ClCorAttrPathT *this)
{
    return(this->node[this->depth-1].index);
}

/** 
 * Set the Index.
 * 
 * API  sets the Index field within the attribute entry
 * to the one specified.
 *
 *  @param this ClCorAttrPath handle
 * 
 *  @returns 
 *    ClUint32T Index Id associated <br/>
 * 
 */
ClRcT
clCorAttrPathIndexSet(ClCorAttrPathT *this, ClUint16T ndepth, 
		    ClUint32T newIndex)
{
    if (ndepth > this->depth)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid depth specified"));
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
    this->node[ndepth-1].index = newIndex;
    return(CL_OK);
}


/**
 *  Compare two attrPath's and check if they are equal.
 *
 *  Comparision API. Compare two attrPath's and see if they are equal. The
 *  comparision goes thru only till the depth. 
 *                                                                        
 *  @param this  ClCorAttrPath handle 1
 *  @param cmp   ClCorAttrPath handle 2
 *
 *  @returns 0 - if both AttrPath's are same (Even if both attrPaths have indexes as wildcards)
 *          -1 - for mis-match
 *           1 - for wild-card match
 */

ClInt32T 
clCorAttrPathCompare(ClCorAttrPathT *this, ClCorAttrPathT *cmp)
{
    ClInt32T i = 0;
    ClInt8T ret = 0;

    CL_FUNC_ENTER();

    if ((this == NULL) || (cmp == NULL))
    {
        CL_FUNC_EXIT();
        return -1;
    }

    if(this->depth != cmp->depth)
    {
        return -1;
    }

  /* Walk thru the values and see if everything matches.
   */
    for(i = 0; i < this->depth; i++) 
    {
        if(cmp->node[i].attrId == this->node[i].attrId)
        {
            if(cmp->node[i].index == CL_COR_INDEX_WILD_CARD &&
                this->node[i].index == CL_COR_INDEX_WILD_CARD)
            {
               /* Both the attrId have indexes as wild cards. So its exact match. But if
                     we had a wildcard match previously, it is wildcard match and not an exact match */
               ret = ret | 0;
            }
            else if (cmp->node[i].index == CL_COR_INDEX_WILD_CARD||
                this->node[i].index == CL_COR_INDEX_WILD_CARD)
            {
                ret = 1;
            }
            else if (cmp->node[i].index != this->node[i].index) 
            {
                return -1; /* otherwise its a zero mis match */
            }
        }
        else if( (cmp->node[i].attrId == CL_COR_ATTR_WILD_CARD) || 
		 (this->node[i].attrId == CL_COR_ATTR_WILD_CARD) )
        {
        /* The AttrId is declared as wildcard. */
        /* Check if the instances are same. If they are same 
               then return wildcard match  */
            if(cmp->node[i].index == this->node[i].index)
          	{
                    ret = 1;
                }
        
        }
        else
        {
            return -1;
        }
    }

    CL_FUNC_EXIT();
    return(ret);
}


/**
 *  Clone a given ClCorAttrPath
 *
 *  Allocates and copies the contents to the new ClCorAttrPath.
 *                                                                        
 *  @param this  ClCorAttrPathT *Handle
 *  @param newH  [out] new clone handle
 *
 *  @returns 
 *    CL_OK        on success <br/>
 *    CL_COR_ERR_NO_MEM  on out of memory <br/>
 */
ClRcT
clCorAttrPathClone(ClCorAttrPathT *this, ClCorAttrPathT **newH)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();

    if ((this == NULL) || (newH == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    ret = clCorAttrPathAlloc(newH);
    if(ret == CL_OK) 
    {
    /* copy the contents here */
        for(i = 0; i < this->depth; i++) 
        {
            (*newH)->node[i].attrId = this->node[i].attrId;
            (*newH)->node[i].index = this->node[i].index;
        }
        (*newH)->depth = this->depth;
    }

    CL_FUNC_EXIT();
    return(ret);
}
