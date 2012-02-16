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
 * File        : clCorMOTreeByPath.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This modules contains the routines to get indexed form of MOId.
 *****************************************************************************/

#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>

/* Internal Header */
#include "clCorTreeDefs.h"
#include "clCorUtilsProtoType.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


/**
 *  Convert class IDs into class idx within a ClCorMOId.
 *
 *  This routine walks through all the valid entries in a ClCorMOId and
 * converts class ids into corresponding class index. Class
 * index represents the index of the class in ClCorMOId tree node array.
 * Note that this conversion is done "in-place", i.e., input ClCorMOId is
 * changed to have idx instead of class ID.
 *
 *  @param moId  - ClCorMOId to convert.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) or CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS) on error.
 */
ClRcT 
corMOTreeClass2IdxConvert(ClCorMOIdPtrT moId)
{
    int i;
    ClRcT rc;
    ClCorMOClassPathT path;

    CL_FUNC_ENTER();
    
    if(!moTree)
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    
    if (moId == NULL)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    /* convert moId to corPath */
    rc = clCorMoIdToMoClassPathGet(moId, &path);
    if(rc == CL_OK)
      {
        if((rc = mArrayId2Idx(moTree, (ClUint32T *)path.node, path.depth))!=CL_OK)
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOTreeClassIdxGet failed rc => [0x%x]", rc));
          }
        else
          {
            /* copy the corPath indexes to ClCorMOId */
            for(i=0;(i<moId->depth) && (i < CL_COR_HANDLE_MAX_DEPTH);i++)
              {
                moId->node[i].type=path.node[i];
              }
          }
      }

    CL_FUNC_EXIT();
    return(rc);
}


/**
 *  Convert class idx's into class IDs within a ClCorMOId.
 *
 *  This routine walks through all the valid entries in a moID and
 * converts class ids into corresponding class index. Class
 * index represents the index of the class in ClCorMOId tree node array.
 * Note that this conversion is done "in-place", i.e., input ClCorMOId is
 * changed to have class ID in place of class Idx.
 *
 *  @param moIdx  - ClCorMOId to convert.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) or CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS) on error.
 */
ClRcT 
corMOTreeIdx2ClassConvert(ClCorMOIdPtrT moIdx)
{
    int i;
    ClRcT rc;
    ClCorMOClassPathT path;

    CL_FUNC_ENTER();

    
    if (moIdx == NULL)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    
    /* convert moId to corPath */
    rc = clCorMoIdToMoClassPathGet(moIdx, &path);
    if(rc == CL_OK)
      {
        if((rc = mArrayIdx2Id(moTree, 1, (ClUint32T *)path.node, path.depth))!=CL_OK)
          {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOTreeClassIdxGet failed rc => [0x%x]", rc));
          }
        else
          {
            /* copy the class Id's from corPath to ClCorMOId */
            for(i=0;(i<moIdx->depth) && (i < CL_COR_HANDLE_MAX_DEPTH);i++)
              {
                moIdx->node[i].type=path.node[i];
              }
          }
      }

    CL_FUNC_EXIT();
    return(rc);
}
