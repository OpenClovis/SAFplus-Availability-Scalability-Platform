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
 * File        : clCorMoClassPath.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * client library for moClassPath utilities
 *****************************************************************************/

#include <clCommon.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clLogApi.h>

/*Internal Headers*/

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif
#include <clCorLog.h>

/** 
 *  Create a MO Path.
 *
 *  Constructor for COR Handle.	 Initializes the memory and 
 *  returns back an empty COR Path. The "instance" field in
 *  all the entries are set to -1 as default. By default the
 *  number of entries is set to 20, and is grown dynamically
 *  when a new entry is added.
 *
 *  @param this      [out] new COR Path handle
 *
 *  @returns 
 *    CL_OK        on success <br/>
 *    CL_COR_ERR_NO_MEM  on out of memory <br/>
 * 
 */
ClRcT
clCorMoClassPathAlloc(ClCorMOClassPathPtrT *this)
{
  ClCorMOClassPathPtrT tmp;
  ClRcT ret = CL_OK;
  int i;
  
  CL_FUNC_ENTER();
  
  if (NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }
  
  tmp = (ClCorMOClassPathPtrT) clHeapAllocate(sizeof(ClCorMOClassPathT));
  *this = tmp;
  if(tmp!=0) {
    /* init stuff here */
    for(i=0;i<CL_COR_HANDLE_MAX_DEPTH;i++) {
      tmp->node[i] = CL_COR_INVALID_MO_ID;
    }
    tmp->depth = 0; 
    tmp->qualifier = CL_COR_MO_PATH_ABSOLUTE; 
  } else {
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
    ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
  }  
  
  CL_FUNC_EXIT();  
  return (ret);
}

/** 
 *  Initialize moPath.
 *
 *  Resets the path info (if present) and re-inits the 
 *  path to empty.
 *
 *  @param this      COR Path handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */
ClRcT
clCorMoClassPathInitialize(ClCorMOClassPathPtrT this)
{
  ClRcT ret = CL_OK;
  int i;

  CL_FUNC_ENTER();
  
  if (NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }
  /* init stuff here */
  for(i=0;i<CL_COR_HANDLE_MAX_DEPTH;i++) {
    this->node[i] = CL_COR_INVALID_MO_ID;
  }
  this->depth = 0; 
  this->qualifier = CL_COR_MO_PATH_ABSOLUTE; 
  
  CL_FUNC_EXIT();
  return (ret);
}

/** 
 *  Free/Delete the COR Path Handle.
 *
 *  Destructor for COR Handle.  Removes/Deletes the handle and frees
 *  back any associated memory. 
 *
 *  @param this COR handle
 *
 *  @returns 
 *    ClRcT  CL_OK on successful deletion.
 * 
 */
ClRcT 
clCorMoClassPathFree(ClCorMOClassPathPtrT this)
{
  CL_FUNC_ENTER();
  if (this == NULL)
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); 
  clHeapFree(this);
  CL_FUNC_EXIT();

  return (CL_OK);
}


/** 
 *  Remove nodes after specified level.
 *
 *  Remove all nodes and reset the cor path till the level specified.
 *
 *  @param this  COR Path
 *  @param level level to which the COR Path needs to be truncated
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 * 
 */
ClRcT
clCorMoClassPathTruncate(ClCorMOClassPathPtrT this, ClInt16T level)
{
  ClRcT ret = CL_OK;
  int i=0;

  CL_FUNC_ENTER();
  
  if (NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }

  if(level  == this->depth)
  {
      CL_FUNC_EXIT();
      return CL_OK;
  }

  /* Check for current depth against the level */
  if(level < this->depth && level >= 0) {
    /* init stuff here */
    for(i=level;i<this->depth;i++) 
      this->node[i]  = CL_COR_INVALID_MO_ID;
    this->depth = (level);
  } else {
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_INVALID_DEPTH));
    ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
  }
  
  CL_FUNC_EXIT();
  return (ret);
}

/** 
 *  Set the Class Type at given node.
 *
 *  Set class type at the given node (level).
 *
 *  @param this  COR Path
 *  @param level node level
 *  @param type  Class Type to be set 
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 * 
 */
ClRcT
clCorMoClassPathSet(ClCorMOClassPathPtrT this, 
           ClUint16T level,
           ClCorClassTypeT type)
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  
  if ( NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }

  if( (CL_COR_CLASS_WILD_CARD != type) && (type < 0) )
  	return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);

  /* Check for current depth against the level */
  if(level <= this->depth) {
    this->node[level-1] = type;
  } else {
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_INVALID_DEPTH));
    ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
  }
  
  CL_FUNC_EXIT();
  return (ret);
}

/** 
 * Add an entry to the COR Path.
 * 
 * This API adds an entry to the COR Path. 
 *
 *  @param this      COR Path
 *  @param type      Node type 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br/>
 *       CL_COR_ERR_MAX_DEPTH if max depth exceeded <br/>
 * 
 */
ClRcT 
clCorMoClassPathAppend(ClCorMOClassPathPtrT  this, 
              ClCorClassTypeT  type)
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  
  if (NULL == this)
  {
      ret = (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  } 
  else if( 0 >= type ) 
  { 
      ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS); 
  }
  else if(this->depth < CL_COR_HANDLE_MAX_DEPTH) {
  /* Check for current depth against the default */
    this->node[this->depth++] = type;
  } else {
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MAX_DEPTH));
    ret = CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH);
  }
  
  CL_FUNC_EXIT();
  return (ret);
}

/** 
 * Get the COR path node depth.
 * 
 * Returns the no of nodes in the hierarchy in the COR Path.
 *
 *  @param this COR handle
 * 
 *  @returns 
 *    ClInt16T number of elements <br/>
 * 
 */
ClInt32T
clCorMoClassPathDepthGet(ClCorMOClassPathPtrT this)
{
  CL_FUNC_ENTER();
  

  if (NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }
  return(this->depth);
}

/** 
 * Display the COR Path.
 * 
 * This API displays all the entries within the COR Path.
 *
 *  @param this COR handle
 * 
 *  @returns 
 *    none
 * 
 */
void
clCorMoClassPathShow(ClCorMOClassPathPtrT this)
{
#ifdef DEBUG
  ClUint32T i;

  if(this)
  {
    clOsalPrintf("\nMOPath:");
    clOsalPrintf("%s", clCorMoClassQualifierStringGet(this->qualifier));
    for(i=0;(i<this->depth) && (i < CL_COR_HANDLE_MAX_DEPTH);i++) {
      clOsalPrintf("%4x", this->node[i]);
      clOsalPrintf((i<(this->depth-1))?".":"");
    }
    clOsalPrintf("\n");
  }
#endif
}

/** 
 * Validate MO Path.
 * 
 * This API validates the MO Path (node types).
 *
 *  @param this COR handle
 * 
 *  @returns 
 *    CL_OK if validation successful <br/>
 * 
 */
ClRcT
clCorMoClassPathValidate(ClCorMOClassPathPtrT this)
{
    ClRcT ret = CL_OK; 
    CL_FUNC_ENTER(); 
    
    if (NULL == this) 
    {
        ret = (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)); 
    }
    else if ( this->depth > CL_COR_HANDLE_MAX_DEPTH )
    {
        ret= CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
    else if ( (this->qualifier < CL_COR_MO_PATH_ABSOLUTE) ||
              (this->qualifier > CL_COR_MO_PATH_QUALIFIER_MAX))
    {
        ret= CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH);
    }

#if EYAL
    else if ( (this->depth > 0) && 
              (this->qualifier >= CL_COR_MO_PATH_ABSOLUTE) && 
              (this->qualifier <= CL_COR_MO_PATH_QUALIFIER_MAX) ) 
    { 
        ClCorMOClassPathPtrT  pCORPath;
        CORMOClass_h moClassHandle;
        int i;
        clCorMoClassPathAlloc(&pCORPath);
        clCorMoClassPathInitialize(pCORPath);
        pCORPath->qualifier = this->qualifier;
        for (i = 0; i < this->depth; i++ )
        {
            clCorMoClassPathAppend(pCORPath, this->node[i]);
            if ( (i < this->depth - 1) && 
                 (ret = corUsrBufPtrGet(pCORPath, (void**) &moClassHandle)) != CL_OK) 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid MO-Class"));
                break;
            }
        }
        clCorMoClassPathFree(pCORPath);
    }
    else 
    { 
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH); 
    }
#endif
    CL_FUNC_EXIT(); 
    return ret;
}


/** 
 * Get the Class Type.
 * 
 * API returns the class type that it referrs to (the bottom most
 * class type in the hierarchy).
 *
 *  @param this COR handle
 * 
 *  @returns 
 *    ClCorClassTypeT  class type associated <br/>
 * 
 */
ClCorClassTypeT
clCorMoClassPathToMoClassGet(ClCorMOClassPathPtrT this)
{
  return(this->node[this->depth-1]);
}



/** 
 * Derive the cor path from given a moId.
 * 
 * API updates the moPath based on the moId. Both
 * moId and moPath are allocated by the caller.
 *
 *  @param moIdh: handle to moId 
 *         corIdh: handle to moPath which is updated. 
 * 
 *  @returns CL_OK on successs.
 */
ClRcT
clCorMoIdToMoClassPathGet(ClCorMOIdPtrT moIdh, ClCorMOClassPathPtrT corIdh)
{
  int i;

  CL_FUNC_ENTER();
  
  
  if ( (NULL == moIdh) || (NULL == corIdh) )
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }

  for(i=0;i<CL_COR_HANDLE_MAX_DEPTH;i++) {
    corIdh->node[i] = moIdh->node[i].type;
  }
  corIdh->depth = moIdh->depth; 
  corIdh->qualifier = moIdh->qualifier; 

  CL_FUNC_EXIT();
  return(CL_OK);
}

#if 0
/**
 *  Return the COR Path associated with this COR.
 *
 *  This API returns the COR Path associated with this COR.
 *                                                                        
 *  @param moId - Reference to MOID object.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
corMyCORPathGet (ClCorMOClassPathPtrT cpath)
{
	ClRcT    rc = CL_OK;
	ClCorMOIdT myMoId;

	rc = clCorMyMOIdGet(&myMoId);

	if(rc != CL_OK)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get My ClCorMOId"));
		return rc;
		}

	if ((rc = clCorMoIdToMoClassPathGet(&myMoId, cpath)) != CL_OK)
		{ 
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to translate MOID to COR Path"));
		}
	return rc;
	
}


/** 
 *  Normalize corPath.
 *
 *  This routine normalizes the corPath. The changes includes:
 *    1. Converting the corPath into absolute path.
 *    2. Filling the invalid part of the path with INVALID values. 
 *
 *  @param this      Cor path handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */
ClRcT
clCorMoClassPathNormalize(ClCorMOClassPathPtrT this)
{
  ClRcT rc = CL_OK;
  int i=0;

  CL_FUNC_ENTER();
  

  if (NULL == this)
  {
      CL_FUNC_EXIT();
      return (CL_COR_ERR_NULL_PTR);
  }

  switch(this->qualifier)
  {
      case CL_COR_MO_PATH_RELATIVE:
	  {
	      ClCorMOIdPtrT pwd;
	      ClCorMOClassPathT corPwd;
		  ClEoExecutionObjT* eoObj = NULL;

              rc = clEoMyEoObjectGet (&eoObj);
              if(rc != CL_OK)
              {
                  CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clEoMyEoObjectGet failed, rc = %x \n", rc));
                  return rc;
              }


		  /* get the current path from the EO specific area */
		  if ((rc = clEoPrivateDataGet(eoObj,
                          CL_EO_COR_SERVER_COOKIE_ID,
						  (void **)&pwd)) != CL_OK) 
		  {
     		 CL_FUNC_EXIT();
	 		 return (rc);
		  }


          clCorMoIdToMoClassPathGet(pwd, &corPwd);

		  /* append current path to the 'this' */
          if ((rc = clCorMoClassPathConcatenate(&corPwd, this, 1)) != CL_OK)
		  {
     		 CL_FUNC_EXIT();
	 		 return (rc);
		  }

          /* 
		   * pwd is either CL_COR_MO_PATH_ABSOLUTE or 
		   * CL_COR_MO_PATH_RELATIVE_TO_BASE. It can never be
		   * relative!!!
		   */
          CL_ASSERT (this->qualifier != CL_COR_MO_PATH_RELATIVE);
		  
          if (this->qualifier == CL_COR_MO_PATH_ABSOLUTE)
	          break;
		  /* else falls through to handle RELATIVE_TO_BASE */

	  }
      case CL_COR_MO_PATH_RELATIVE_TO_BASE:
	  {
            #if 0
	      ClCorMOClassPathT base;
		  /* get the base path */
          if ((rc = corMyCORPathGet(&base)) != CL_OK)
		  {
     		 CL_FUNC_EXIT();
	 		 return (rc);
		  }
		  /* append the base path to the 'this' */
          if ((rc = clCorMoClassPathConcatenate(&base, this, 1)) != CL_OK)
          {
             CL_FUNC_EXIT();
             return (rc);
          }
            #endif
	      break;
	  }
      case CL_COR_MO_PATH_ABSOLUTE:
	      break;
      default:
          CL_DEBUG_PRINT(CL_DEBUG_ERROR,
		     ( "clCorMoClassPathNormalize: Invalid moId qualifier => [0x%x]",
			 this->qualifier));
	      break;
  }

  /* init stuff here */
  for(i=this->depth;i<CL_COR_HANDLE_MAX_DEPTH;i++) {
    this->node[i] = CL_COR_INVALID_MO_ID;
  }
  CL_FUNC_EXIT();
  return (rc);
}

#endif

/**
 *  Check if the corPath contains any wild cards.
 *
 *  API to check if ClCorMOId contains any wild cards.
 *
 *  @param moId      MO Id handle
 *
 *  @returns CL_TRUE if moId contains wild card, CL_FALSE otherwise.
 */
int
clCorDoesPathContainWildCard (ClCorMOClassPathPtrT corPath)
{
    int i;
                                                                                      
    CL_FUNC_ENTER();
    if (corPath == NULL) return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                                                                                      
    for (i = 0; i < corPath->depth; i++)
    {
        if (corPath->node[i] == CL_COR_CLASS_WILD_CARD)
        {
            CL_FUNC_EXIT();
            return (CL_TRUE);
        }
    }
                                                                                      
    CL_FUNC_EXIT();
    return (CL_FALSE);
}


/** INTERNAL ROUTINE
 *  Check if the AttrPath contains any wild cards.
 *
 *  @returns CL_TRUE if moId contains wild card, CL_FALSE otherwise.
 */

int
clCorDoesAttrPathContainWildCard (ClCorAttrPathPtrT pAttrPath)
{
    int i;
    CL_FUNC_ENTER();

    for (i = 0; i < pAttrPath->depth; i++)
    {
        if (pAttrPath->node[i].attrId == CL_COR_ATTR_WILD_CARD)
        {
            CL_FUNC_EXIT();
            return (CL_TRUE);
        }
    }
                                                                                      
    CL_FUNC_EXIT();
    return (CL_FALSE);
}


/** 
 * Concatenate an ClCorMOId to another.
 * 
 * This API concatenates two moIds.
 *
 *  @param part1 : upper part of the moId. 
 *  @param part2 : lower part of the moId
 *  @param copyWhere : Indicates where concatenated moID
 *   be copied. If 0, part1 is modified by suffixing part2
 *   to it. If 1, part2 is modified by pre-pendeding part1
 *   to it.
 *  @returns 
 *    ClRcT  CL_OK on success <br/>
 *       CL_COR_ERR_MAX_DEPTH if max depth exceeded <br/>
 * 
 */
ClRcT 
clCorMoIdConcatenate(ClCorMOIdPtrT part1, ClCorMOIdPtrT part2, ClInt32T copyWhere)
{
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    
    

    if ((part1 == NULL) || (part2 == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  /* Check for overflow */
    if((part1->depth + part2->depth) >= CL_COR_HANDLE_MAX_DEPTH) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MAX_DEPTH));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH));
    }

    if (copyWhere == 0)
    {
        for(i = 0; i < part2->depth; i++) 
        {
            part1->node[part1->depth + i].type = part2->node[i].type;
            part1->node[part1->depth + i].instance = part2->node[i].instance;
        }
        part1->depth += part2->depth;
    } 
    else 
    {
        for(i = part2->depth-1; i >= 0; i--) 
        {
	  /* move the part2 by part1->depth */
            part2->node[i+part1->depth].type = part2->node[i].type;
            part2->node[i+part1->depth].instance = part2->node[i].instance;
        }
        for(i = 0; i < part1->depth; i++) 
        {
	  /* prepend part1 */
            part2->node[i].type = part1->node[i].type;
            part2->node[i].instance = part1->node[i].instance;
        }
        part2->depth += part1->depth;
        part2->qualifier = part1->qualifier;
    }
  
    CL_FUNC_EXIT();
    return (CL_OK);
}


/** 
 * Concatenate an ClCorMOClassPath to another.
 * 
 * This API concatenates two CORPaths.
 *
 *  @param part1 : upper part of the corPath. 
 *  @param part2 : lower part of the corPath
 *  @param copyWhere : Indicates where concatenated corPath
 *   be copied. If 0, part1 is modified by suffixing part2
 *   to it. If 1, part2 is modified by pre-pendeding part1
 *   to it.
 *  @returns 
 *    ClRcT  CL_OK on success <br/>
 *       CL_COR_ERR_MAX_DEPTH if max depth exceeded <br/>
 * 
 */
ClRcT 
clCorMoClassPathConcatenate(ClCorMOClassPathPtrT part1, ClCorMOClassPathPtrT part2, int copyWhere)
{
  int i;

  CL_FUNC_ENTER();
  
  
  if ( (NULL == part1) || (NULL == part2) )
  {
      CL_FUNC_EXIT();
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
  }


  /* Check for overflow */
  if((part1->depth + part2->depth) >= CL_COR_HANDLE_MAX_DEPTH) {
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MAX_DEPTH));
    CL_FUNC_EXIT();
    return (CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH));
  }

  if (copyWhere == 0)
  {
    for(i=0; i<part2->depth; i++) {
      part1->node[part1->depth + i] = part2->node[i];
    }
    part1->depth += part2->depth;
  } else {
    for(i=0; i<part1->depth; i++) {
	  /* move the part2 by part1->depth */
      part2->node[part1->depth + i] = part2->node[i];
	  /* prepend part1 */
      part2->node[i] = part1->node[i];
    }
    part2->depth += part1->depth;
    part2->qualifier = part1->qualifier;
  }
  
  CL_FUNC_EXIT();
  return (CL_OK);
}

/**
 *  Compare two moClassPath's and check if they are equal.
 *
 *  Comparision API. Compare two ClCorMOClassPath's and see if they are equal. The
 *  comparision goes thru only till the depth. 
 *                                                                        
 *  @param this  ClCorMOClassPath handle 1
 *  @param cmp   ClCorMOClassPath Handle 2
 *
 *  @returns CL_OK - if both ClCorMOClassPath are same
 */

ClRcT clCorMoClassPathCompare(ClCorMOClassPathPtrT hMoPath1, ClCorMOClassPathPtrT hMoPath2)
{
	ClInt32T i = 0;
	
	if (hMoPath1->depth != hMoPath2->depth)
	{
	     return CL_COR_SET_RC(CL_COR_UTILS_ERR_MOCLASSPATH_MISMATCH);
	}
	for (i = 0; i < hMoPath1->depth; i++)
	{
		if (hMoPath1->node[i] != hMoPath2->node[i])
		{
			return CL_COR_SET_RC(CL_COR_UTILS_ERR_MOCLASSPATH_MISMATCH);
		}
	}
	return CL_OK;
}

#if 0
/** 
 * Get the MO path qualifier.
 * 
 * Returns the MO path qualifier which indicates
 * whether the path is absolute, relative or relative
 * to the location of the blade.
 *
 *  @param this COR handle
 *         pQualifier: [OUT] qualifier value is returned here.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoClassPathQualifierGet(ClCorMOClassPathPtrT this, ClCorMoPathQualifierT *pQualifier)
{
  CL_FUNC_ENTER();
  
  

  if ( (NULL == this) || (NULL == pQualifier) )
  {
      CL_FUNC_EXIT();
      return (CL_COR_ERR_NULL_PTR);
  }

  *pQualifier = this->qualifier; 

  CL_FUNC_EXIT();
  return(CL_OK);
}

/** 
 * Set the COR path qualifier.
 * 
 * Sets the COR path qualifier which indicates
 * whether the path is absolute, relative or relative
 * to the location of the blade.
 *
 *  @param this COR handle
 *         qualifier: qualifier value to set.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoClassPathQualifierSet(ClCorMOClassPathPtrT this, ClCorMoPathQualifierT qualifier)
{
  CL_FUNC_ENTER();
  

  if ( NULL == this )
  {
      CL_FUNC_EXIT();
      return (CL_COR_ERR_NULL_PTR);
  }

  if (qualifier > CL_COR_MO_PATH_QUALIFIER_MAX)
  {
      CL_FUNC_EXIT();
      return (CL_COR_ERR_INVALID_PARAM);
  }

  this->qualifier = qualifier;

  CL_FUNC_EXIT();
  return(CL_OK);
}
#endif
