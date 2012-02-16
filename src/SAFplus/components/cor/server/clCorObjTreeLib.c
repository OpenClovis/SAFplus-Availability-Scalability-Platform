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
 * File        : clCorObjTreeLib.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements object tree libary.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>
#include <clBufferApi.h>

/*Internal Headers*/
#include "clCorDmProtoType.h"
#include "clCorTreeDefs.h"
#include "clCorRmDefs.h"
#include "clCorPvt.h"
#include "clCorNiIpi.h"
#include "clCorLog.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

extern ClCharT* clCorServiceIdString[];

/* global variables */
ObjTree_h objTree=0;
ClCorMOIdT gUnpackMOId;
ClCorMOIdT gMOIdInPack;

/* local definitions */
static ClRcT _corObjPackFun(ClUint32T idx, void * element, void **buf);
static ClRcT _corObjUnpackFun(ClUint32T idx, void * element, void ** buf);

static ClRcT _corObjShowFun(ClUint32T idx, void * element, void ** buf);
static ClRcT _clCorObjTreeDelete(ClUint32T idx, void * element, void ** cookie);

/**
 *  Intialize the Object Tree library module.
 *
 *  This API intialize the Object Tree library module.
 *                                                                        
 *  @param none
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *
 */
ClRcT
corObjTreeInit()
{
    ClRcT ret=CL_OK;
  
#ifdef DEBUG
    static ClUint8T corObjTreeAddedToDBG = CL_FALSE;
    if (CL_FALSE ==corObjTreeAddedToDBG)
    {
        ret= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
        corObjTreeAddedToDBG = CL_TRUE ;

        if (CL_OK != ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
            CL_FUNC_EXIT();
            return ret;
        }
    }
#endif
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "initialize objTree"));
    if(!objTree)
      {
        ret = corObjTreeCreate(&objTree);
	if(ret != CL_OK)
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_1_OBJECT_TREE_CREATE, ret);
      }

    CL_FUNC_EXIT();
    return (ret);
}

/**
 *  Create new Object tree..
 *
 *  This API creates and intializes the object tree.
 *                                                                        
 *  @param this [out] returns the new allocated tree handle
 *
 *  @returns 
 *    CL_OK  - Success<br>
 */
ClRcT
corObjTreeCreate(ObjTree_h *this)
{
    ClRcT ret=CL_OK;
  
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "create objTree"));

    
    if(this)
      {
        /* create and initialize the obj tree root here */
        ret = mArrayCreate(COR_OBJ_SIZE, this);
        if(ret==CL_OK)
          {
            (*this)->root->id = 1;

            /* set the pack, unpack function pointers */
            mArrayNodePackFPSet((**this), _corObjPackFun);
            mArrayNodeUnpackFPSet((**this), _corObjUnpackFun);

            mArrayDataPackFPSet((**this), _corObjPackFun);
            mArrayDataUnpackFPSet((**this), _corObjUnpackFun);
            
#ifdef DEBUG
            mArrayNameSet((*this), "objTree");
            mArrayNodeNameSet((*this)->root, "/");
#endif
          }
      }

    CL_FUNC_EXIT();
    return(ret);
}

/** 
 * Add the given DM Object handle to the specified MO Id path
 */
ClRcT
corObjTreeAdd(ObjTree_h  this, 
              ClCorMOIdPtrT     moId,
              DMObjHandle_h objHandle)
{
    ClRcT  ret = CL_OK;

    CL_FUNC_ENTER();

    if(this && moId && objHandle)
      {
        ObjTreeNode_h  parent;
        MOTreeNode_h   hMONode   = 0;
        MOTreeNode_h   parentMO  = 0;
        ClCorMOClassPathT tmpCorPath;
        ClCorClassTypeT classId = moId->node[moId->depth-1].type;
        ClCorInstanceIdT instId = clCorMoIdToInstanceGet(moId);
        ObjTreeNode_h  node;
		MArrayVector_h vectorH = NULL;
        
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "inside corObjTreeAdd"));

        clCorMoIdToMoClassPathGet(moId, &tmpCorPath);

        /* get the index list from the moTree and check if the classes
         * are present.
         */
          ret = mArrayId2Idx(moTree, (ClUint32T *) tmpCorPath.node, tmpCorPath.depth);
          if(ret != CL_OK)
          {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
            return ret;
          }
        /* todo: check if max instances are reached. Also there can be
         * instances that are freed in between, how about that?
         */
        parent = corObjTreeFindParent(this, moId);
        if(!parent)
          {
            /* parent unknown */
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_INST_ERR_NODE_NOT_FOUND)));
            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
          }
        /* check if node is there and then add 
         */
        node = mArrayNodeIdIdxNodeFind(parent, classId, instId);
        if(!node)
        {
            int grpId = tmpCorPath.node[tmpCorPath.depth-1];

            ret = mArrayNodeAdd(this, parent, 
                                grpId,
                                instId, /* slot of this class */
                                COR_MO_GRP_SIZE);  /* @todo: to be computed */
        }
        else
        {
            /* Check whether object has been deleted previously */
            CORObject_h tmp = (CORObject_h) node->data;
            if (tmp->dmObjH.classId) 
            {
                CL_DEBUG_PRINT(CL_DEBUG_INFO, (CL_COR_ERR_STR(CL_COR_INST_ERR_MO_ALREADY_PRESENT)));
                ret = CL_COR_SET_RC(CL_COR_INST_ERR_MO_ALREADY_PRESENT);
            }
        }
        if(ret == CL_OK) /* If everything is ok.. then */
          {
            int grpId = tmpCorPath.node[tmpCorPath.depth-1];
            node = mArrayNodeGet(parent, grpId, instId);
            if(node)
              {
                CORObject_h  tmp;
                ret = mArrayDataNodeAdd(node, 
                                        0, /* MSO vector init */
                                        COR_OBJ_SIZE, /* obj Handle size*/
                                        COR_MSO_GRP_SIZE);
                node->id = classId;
                tmp = (CORObject_h) node->data;
                /* tmp->dmObjH = *objHandle;*/
                tmp->dmObjH.classId = objHandle->classId;
                tmp->dmObjH.instanceId = objHandle->instanceId;
                tmp->type   = CL_COR_OBJ_TYPE_MO;
                tmp->flags  = CL_COR_OBJ_FLAGS_DEFAULT;

				vectorH = mArrayGroupGet(parent, grpId);
				if(vectorH != NULL)
					vectorH->numActiveNodes++;
				
                /* Get the MO Class into. Increment the number of instances created */
                clCorMoIdToMoClassPathGet(moId, &tmpCorPath);
                if (NULL != (parentMO = corMOTreeFindParent(moTree, &tmpCorPath)))
                {
                    if (NULL != (hMONode = mArrayNodeIdNodeFind(parentMO, classId)))
                    {
                         _CORMOClass_h   hMOClass = 0;
                         hMOClass = (_CORMOClass_h) hMONode->data;
                         hMOClass->instances++;
                    } /* End of MONode != NULL */
                } /* End of Parent MO != NULL*/
              }
          }
      }
    else
      {
        /* invalid / null pointer */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
      }

    CL_FUNC_EXIT();
    return(ret);
}

/* Delete all the nodes of object tree */
ClRcT corObjectTreeFinalize(void)
{
	MArrayWalk_t tmpWalkDetails = {0};
	MArrayWalk_h walkCookie = &tmpWalkDetails;
	ClCorMOIdT tmpMOId;
	ClCorMOIdPtrT cookie = &tmpMOId;
	ObjTree_t tmpTree = *objTree;
	ClUint32T idx = 0;
	ClRcT rc = CL_OK;

	clCorMoIdInitialize(cookie);
	tmpWalkDetails.fpNode = (MArrayNodeFP) _clCorObjTreeDelete;
	tmpWalkDetails.fpData = (MArrayDataFP) _clCorObjTreeDelete;
	tmpWalkDetails.cookie = (void **)& cookie;
	
	mArrayDelete(idx,
		   &tmpTree,
		   _clCorObjTreeDelete,
		   _clCorObjTreeDelete,
		   (void **)&walkCookie,
		   rc);
	
    clHeapFree(objTree);

	return CL_OK;
}



static
ClRcT
_clCorObjTreeDelete(ClUint32T idx,
               void * element,
               void ** cookie
               )
{
    MArrayWalk_h* walkInfo = (MArrayWalk_h*) cookie;
    ClCorMOIdPtrT* oh = (ClCorMOIdPtrT*) (*walkInfo)->cookie;
    CORObject_h obj = (CORObject_h)element; 

    if(!element)
      {
        if(idx==COR_FIRST_NODE_IDX)
          {
            (*oh)->depth ++;
          } 
        if(idx==COR_LAST_NODE_IDX) 
          {
	   /*free(&obj->dmObjH);*/
           (*oh)->depth --; 
          }
      }
    else
      {
        if(oh && obj->dmObjH.classId)
        {

           if(obj->type == CL_COR_OBJ_TYPE_MO)
              {
		/* Setting the moId for the new value of depth */
                (*oh)->node[(*oh)->depth-1].type = obj->dmObjH.classId;
                (*oh)->node[(*oh)->depth-1].instance = idx;
              }
          }
      }
    return CL_OK;
}

/** 
 *  Delete the MO, given the moId.
 */

ClRcT
corObjTreeNodeDelete(ObjTree_h  this, 
              ClCorMOIdPtrT     moId)
{
  ClRcT  ret = CL_OK;
  CL_FUNC_ENTER(); 
  
   if(this && moId)
   {
       ObjTreeNode_h obj;
       if((obj = corObjTreeNodeFind(this, moId))!=NULL)
       {	
	   /* Make sure that there are no children beneath, including MSOs. */
           ClUint32T i = 0;
           ClInt32T idx = -1;
	  
	  /* Search for MOs */
 	   if(mArrayNodeNextChildGet(obj, &i, &idx)) 
	   {
		   CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
  	              ( "Child MO Instances present, CAN'T delete"));		
       		   CL_FUNC_EXIT();
       		   return(CL_COR_SET_RC(CL_COR_INST_ERR_CHILD_MO_EXIST));
	   }
         /* Search for MSOs */
           if(mArrayDataNodeFirstNodeGet(obj, 0))
            {
    		    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
			      ( "MSO Instances present, CAN'T delete"));
    		    CL_FUNC_EXIT();
    		    return(CL_COR_SET_RC(CL_COR_INST_ERR_MSO_EXIST));
            }
	   else
	   {
     		   /* Delete the MSO vector. */
     		   ret = mArrayDataNodeDelete(obj, 0); /*0 is grp idx for MSO*/ 
	   }
 	   /* Now go ahead and delete the MO node in Instance tree.*/ 
	   /* just memset it to zero's */
	   obj->id = 0;
	   memset(obj->data, 0, sizeof(CORObject_t));
	   if((ret = mArrayNodeDelete(obj) ) != CL_OK)
	   {
   		CL_DEBUG_PRINT(CL_DEBUG_ERROR,
			  ("mArrayNodeDelete failed for MO Node ret = 0x%x\n", ret));
                CL_FUNC_EXIT();
                return ret;
	   }
       }
       else
       {	
	   CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid ClCorMOId. MO instance does not exist."));
           /*clCorMoIdShow(moId);*/
           CL_FUNC_EXIT();
           return(CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND));
       }
       if(ret == CL_OK) /* If everything is ok.. then */
       {
            /* Get the MO Class info. Decrement the number of instances created */
            /* Get the mo class handle */
	       CORMOClass_h  tmpMoClassH;
	       ClCorMOClassPathT      tmpMoPath;
           ObjTreeNode_h  parent;
		   MArrayVector_h vectorH = NULL;
		   ClUint32T grpId = 0;
	       clCorMoIdToMoClassPathGet(moId, &tmpMoPath);
	       if( (ret = corMOClassHandleGet(&tmpMoPath, &tmpMoClassH)) == CL_OK)
	       {
		       ((_CORMOClass_h)tmpMoClassH->data)->instances--;

				/* Have to decrement the count in the vectorNode also */
				/* get the index list from the moTree and check if the classes
		         * are present.
        		 */
		          ret = mArrayId2Idx(moTree, (ClUint32T *) tmpMoPath.node, tmpMoPath.depth);
        		  if(ret != CL_OK)
          			{
			            CL_FUNC_EXIT();
            			CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
			            return ret;
          			}
			        /* todo: check if max instances are reached. Also there can be
         			* instances that are freed in between, how about that?
         			*/
        			parent = corObjTreeFindParent(this, moId);
			        if(!parent)
         			 {
				            /* parent unknown */
				            CL_FUNC_EXIT();
				            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_INST_ERR_NODE_NOT_FOUND)));
				            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
          			  }
            		  grpId = tmpMoPath.node[tmpMoPath.depth-1]; 
					  vectorH = mArrayGroupGet(parent, grpId);
					  if(vectorH != NULL)
						vectorH->numActiveNodes--;
	       }
	       else
	       {
		   CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Could not get mo class handle"));
                   return ret;
	       }
       }
   }
   else
   {
   	   /* invalid / null pointer */
   	   CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
   	   ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
   }
   CL_FUNC_EXIT();
   return(ret);
}

/** 
 * Display the obj tree from the given path
 */
ClRcT 
corObjTreeShowDetails(ClCorMOIdPtrT from, ClBufferHandleT * pMsg)
{
    int idx = 0; 
    ClCorMOIdPtrT cookie = from;
    MArrayWalk_t tmpWalkDetails={0};
    MArrayWalk_h walkCookie = &tmpWalkDetails;
    ObjTree_t tmpTree;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_COR_CLI_STR_LEN];

    /* check if objtree is present */
    if(!objTree)
    {
        clLogError("COR", "DBG", "Object Tree is not initialized.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    tmpTree = *objTree;
    
    corStr[0]='\0';
    sprintf(corStr, "Objects (Instance Tree):");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

    /* moIdh can be NULL, that means its a full tree show */
    if(from)
    {
        tmpTree.root = corObjTreeNodeFind(objTree, from);
    }

    tmpWalkDetails.flags = CL_COR_MO_WALK;
    tmpWalkDetails.cookie = (void **) &cookie;
    tmpWalkDetails.pMsg = pMsg;

	if(cookie && cookie->depth)
		idx = clCorMoIdToInstanceGet(cookie);

    if(tmpTree.root)
    {
        mArrayWalk(idx, &tmpTree, _corObjShowFun, _corObjShowFun, (void**)&walkCookie, rc);
    }

    corStr[0]='\0';
    sprintf(corStr, "\n");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

    return rc;
}

/** 
 * From the object tree, find the node for the corresponding mo id
 */
ObjTreeNode_h
corObjTreeNodeFind(ObjTree_h this,
                   ClCorMOIdPtrT    path
                   )
{
    CL_FUNC_ENTER();

    if(this && path && path->depth>0)
      {
        int i;
        ClUint32T list[CL_COR_HANDLE_MAX_DEPTH];
        ClUint32T idx[CL_COR_HANDLE_MAX_DEPTH];

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "inside corObjTreeNodeFind"));
            
        for(i = 0;i < path->depth && i < CL_COR_HANDLE_MAX_DEPTH; i++) 
          {
            list[i] = path->node[i].type;
            idx[i] = path->node[i].instance;
          }
        return mArrayIdIdxNodeFind(this, list, idx, i);
      }

    CL_FUNC_EXIT();
    return(0);
}


#ifdef GETNEXT

/*
 * Find the next node.
 * If path is empty, then first node is returned.
 */

ObjTreeNode_h
corObjTreeNextNodeFind(ObjTree_h this, ClCorMOIdPtrT path)
{
   ObjTreeNode_h parent;
   ObjTreeNode_h node;
   ObjTreeNode_h tmpNode;


    /* check for empty path. If so, get first node in tree.*/
    if (path->node[0].type == CL_COR_INVALID_MO_ID &&
		    path->node[0].instance == CL_COR_INVALID_MO_INSTANCE)
    {
         ClUint32T grpId = 0;
         ClUint32T idx = -1;
         tmpNode = mArrayNodeNextChildGet(this->root, &grpId, &idx);
	 if(tmpNode)
	 {
	      clCorMoIdAppend(path, tmpNode->id, idx);
 	      return tmpNode;
	 }
	 return 0;
    }
   
    /* Find the actual node.*/ 
     node = corObjTreeNodeFind(this, path);
     if(node)	 
     {
        ClUint32T grpId = 0;
        ClUint32T idx = -1;
   	     
         /*If the node has children, get the first child */
         tmpNode = mArrayNodeNextChildGet(node, &grpId, &idx);
         if(tmpNode)
	 {
              /* update MOID */
	         clCorMoIdAppend(path, tmpNode->id, idx);
                 return tmpNode;
	 }
	 else
	 {
             ClCorMOClassPathT tmpMoPath;

             clCorMoIdToMoClassPathGet(path, &tmpMoPath); 
             mArrayId2Idx(moTree, tmpMoPath.node, tmpMoPath.depth);
            
	     idx = path->node[path->depth-1].instance; /* get the next child */
             grpId = tmpMoPath.node[tmpMoPath.depth-1];
            
	     
	     parent = corObjTreeFindParent(this, path);
	   
	     /* Get the next Sibling */
             tmpNode = mArrayNodeNextChildGet(parent, &grpId, &idx);
	     if(tmpNode)
	     {
	         clCorMoIdSet(path, path->depth, tmpNode->id, idx);
		 return tmpNode;
	     }
	     else
	     {
	       /* Keep on searching the parent's sibling.
		  if not found, go to grandparent and search .... so on */	      
	       do{
	               /* Deprecate last level in moId and moPath */
       		       clCorMoIdTruncate(path, path->depth-1);
       		       clCorMoClassPathTruncate(&tmpMoPath, tmpMoPath.depth-1); 
	       
		       idx = path->node[path->depth-1].instance;
       		       grpId = tmpMoPath.node[tmpMoPath.depth-1];
	      
		       parent = corObjTreeFindParent(this, path);

             	       tmpNode = mArrayNodeNextChildGet(parent, &grpId, &idx);
	    	       if(tmpNode)
	  	       {
	      		       clCorMoIdSet(path, path->depth, tmpNode->id, idx);
	      		       return tmpNode;
       		       } 
	         } while(tmpNode == NULL && (path->depth > 1));
	     }
	 }
      } 
  return 0;
}
#endif



/**
 *  Obj Tree pack routine.
 *
 *  This API packs the Obj Tree.
 *                                                                        
 *  @param cAddr - ClCorMOId path below which to pack the subtree.
 *  @param buf -  buffer to hold the pack
 *  @param size -  [IN/OUT] size of the buffer / size of the packed buffer
 *
 *  @returns 
 *    CL_OK - on Success
 */
ClRcT 
corObjTreePack (ClCorMOIdPtrT  moIdh,
		ClUint16T flags,
                ClBufferHandleT *pBufH
                )
{
    ClRcT ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    ObjTree_t tmpTree;
 
    /* check if objtree is present */
    if(!objTree)
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    tmpTree = *objTree;

    if(pBufH ) 
    {
        if(moIdh)
        {
            gUnpackMOId = *moIdh;
        }
        else
        {
            clCorMoIdInitialize(&gUnpackMOId);
        }

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\n----------BEGIN OBJ TREE PACKET DATA--------------------"));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nMOId: "));
#ifdef DEBUG
     /*   clCorMoIdShow(moIdh);*/
#endif
        if(moIdh && moIdh->depth > 0)
        {
            tmpTree.root = corObjTreeNodeFind(objTree, moIdh);
        }
        if(tmpTree.root) 
        {	
	    /* Packet structure : <sizeParent> <Parent Data> <Actual SubTree>
	     * In case of BD we directly send subTree and don't fill the parent information
             */
            ret = mArrayPack(&tmpTree, flags, OBJTREE, pBufH);
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                      ( "\nResult of the object pack 0x%x\n", ret));
        }
	else
        {
            ret = CL_OK;
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,( "\nNothing to be packed for instance tree.") ); 
	}
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\n----------END OBJ TREE PACKET DATA--------------------")); 
    }
    return(ret);
}

/**
 *  Obj Tree unpack routine.
 *
 *  This API Unpacks the Obj Tree.
 *                                                                        
 *  @param moIdh - ClCorMOId below which to unpack the subtree.
 *  @param buf -  buffer containing packed tree
 *  @param size -  [IN/OUT] size of the buffer / size of the unpacked buffer
 *
 *  @returns 
 *    CL_OK - on Success
 */
ClRcT 
corObjTreeUnpack (ClCorMOIdPtrT moIdh,
                  void *  buf,
                  ClUint32T* size
                  )
{
    ClRcT ret = CL_OK;
    ObjTree_t tmpTree;

   /* check if objtree is present */
    if(!objTree)
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    tmpTree = *objTree;
 

    /* moIdh can be NULL, if so, then its a full unpack at root */

    if(buf && size)
    {
        ClCorMOIdT moId;
        /*int moDepth = 0;
        short sizeParents = 0;
        int i = 0;*/

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ( "\n----------BEGIN OBJ TREE PACKET DATA--------------------"));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nMOId: "));
        dmBinaryShow((Byte_t*)"Received ObjectTree Packet", buf, *size);

        /* packet structure looks like this
         * <sizeofParents> <parent objects> <full subtree>
        */
#if 0
        STREAM_OUT(&sizeParents,buf, sizeof(sizeParents));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\nSize of packed structure %d. Parent size %d\n",
                   *size, sizeParents) );
#endif
        if(moIdh)
        {
            moId = *moIdh;
        }
        else
        {
            clCorMoIdInitialize(&moId);
        }
#ifdef DEBUG
     /*   clCorMoIdShow(&moId);*/
#endif
        
/*	tmpTree.root = corObjTreeNodeFind(objTree, &moId); */
#if 0
        if(moIdh && COR_ISBD && sizeParents > 2) 
        {
            /* now copy the parents (objects) and try to create them
             */
            ObjTreeNode_h node = 0;
            moDepth = moId.depth;
            for(i=0; i< moDepth - 1; i++)
            {
                DMObjHandle_t dummy;
  
                moId.depth = (i+1);

                if( (ret = corObjTreeAdd(objTree, &moId, &dummy) ) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ( "\nCould not add node to tree. Error 0x%x", ret) );
                    return ret;
                }
                node = corObjTreeNodeFind(objTree, &moId);
                if(!node){
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ( "\nCould not find instance tree node." ) );
                    return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
                }
                _corObjUnpackFun(moId.node[i].instance,
                             node->data,
                             &buf);
            }
            moId.depth = moDepth;	/*restore the original depth*/
        }
        /* Now we are done with parent, lets see what is left */
        *size -= sizeParents;
#endif
        if(*size <= 0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nNothing to unpack.") );
            return CL_OK;
        }

        if(moIdh)
        {
            ClInt32T idx = 0;
            ClCorMOIdT tmpMoId  = moId;
            ClCorMOClassPathT tmpPath;

            if(moIdh->depth > 0)
            	tmpTree.root = corObjTreeNodeFind(objTree, moIdh);
            if(tmpTree.root)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                         ( "\nInstance tree node is already present."));
            }
            clCorMoIdToMoClassPathGet(&tmpMoId, &tmpPath);
            if(tmpMoId.depth > 1)
            {
                tmpMoId.depth--;
                if(!corObjTreeNodeFind(objTree, &tmpMoId))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "UNKNOWN Parent(can't merge)."));
                    return CL_ERR_NOT_IMPLEMENTED;
                }
                tmpTree.root = corObjTreeNodeFind(objTree, &tmpMoId);
            	idx=(moId.node[moId.depth-1].instance);
#ifdef DEBUG 
        	    clCorMoClassPathShow(&tmpPath);
#endif
            	if( (ret = mArrayId2Idx(moTree, (ClUint32T *) tmpPath.node, tmpPath.depth) ) == CL_OK)
            	{
                	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nGroup id %d, free slot %d",
                    	      tmpPath.node[tmpPath.depth-1],idx) );
                	ret = mArrayNodeAdd(objTree, tmpTree.root, 
                	                    tmpPath.node[tmpPath.depth-1],
                    	                idx, /* slot of this class */
                        	            COR_MO_GRP_SIZE);  /* @todo: to be computed */
                	tmpTree.root = mArrayNodeGet(tmpTree.root, 
               			                        tmpPath.node[tmpPath.depth-1],
                         	                    idx);
            	}
            	else
            	{
                	CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    	    ( "\nCould not get group index. 0x%x",ret));
                	return ret;
            	}
            }
            else
            {
                tmpTree = *objTree;  /* root is the parent */
            } 
        }
        ret = mArrayUnpack(&tmpTree, OBJTREE, buf, size);
#ifdef DEBUG
        corObjTreeShowDetails(0);
#endif
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ( "\n----------END OBJ TREE PACKET DATA--------------------"));
    }
    else
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    return(ret);
}

/** 
 * Given tree and path, find the parent node.
 */
ObjTreeNode_h
corObjTreeFindParent(ObjTree_h this,
                     ClCorMOIdPtrT    path
                     )
{
    CL_FUNC_ENTER();

    if(this && path && path->depth>0)
      {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "inside corObjTreeFindParent"));
        
        /*
         * a) convert the ClCorMOId to corpath
         * b) query the corPath in moTree and get the indexes
         * c) query the objTree using the indexes to find the object
         */
        if(path->depth>1)
          {
            int i;
            ClUint32T list[CL_COR_HANDLE_MAX_DEPTH];
            ClUint32T idx[CL_COR_HANDLE_MAX_DEPTH];
            
            for(i = 0;i < path->depth && i < CL_COR_HANDLE_MAX_DEPTH; i++) 
              {
                list[i] = path->node[i].type;
                idx[i] = path->node[i].instance;
              }
            return mArrayIdIdxNodeFind(this, list, idx, i-1);
          }
        else
          {
            return this->root;
          }
      }

    CL_FUNC_EXIT();
    return(0);
}
		

/* ----[Pack,Unpack routines]-------------------------------------------- */

static 
ClRcT 
_corObjPackFun(ClUint32T idx,
               void * element, 
               void **dest
               )
{
    CORObject_h obj = (CORObject_h)element;
    ClRcT ret = CL_OK;
    ClUint32T   size = 0;

    ClUint16T flags = (*((CORPackObj_h *)(dest)))->flags;
    ClBufferHandleT *pBufH = (*((CORPackObj_h *)(dest)))->pBufH;

    if( NULL == pBufH )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    if(obj && obj->dmObjH.classId)
    {
        ClUint32T tag = COR_OBJ_TAG;
        ClUint16T tmpFlags;
        ClCorMOIdT modifiedMOId = gUnpackMOId;

        if(modifiedMOId.depth > gMOIdInPack.depth)
            modifiedMOId.depth = gMOIdInPack.depth;

        tmpFlags = rmFlagsGet(&gUnpackMOId);
        /*When sending the information from SD, send it if the flag is global; or if it is local or 
          cache_on_master for the particular blade.*/
        if( (tmpFlags & flags) || ((clCorMoIdCompare(&gMOIdInPack, &modifiedMOId)==0) 
                                    && ( (tmpFlags & CL_COR_OBJ_CACHE_ON_MASTER) || (tmpFlags & CL_COR_OBJ_CACHE_LOCAL) ) ) ){
            /* First pack the tag, then index only */
			ClUint32T tmpTag, tmpIdx;
            STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &tag, tmpTag, sizeof(tag));
            STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &idx, tmpIdx, sizeof(idx));
            /* now stream in Obj data */
			ClUint16T type, flags;
			ClUint32T classId, instanceId; 

            STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &obj->dmObjH.classId, classId ,sizeof(obj->dmObjH.classId));
            STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &obj->dmObjH.instanceId, instanceId ,sizeof(obj->dmObjH.instanceId));
            STREAM_IN_BUFFER_HANDLE_HTONS((*pBufH), &obj->type, type, sizeof(type));
            STREAM_IN_BUFFER_HANDLE_HTONS((*pBufH), &obj->flags, flags, sizeof(flags));

            /* stream the dm object here */
            ret = dmObjHandlePack(pBufH, &obj->dmObjH, &size); 
            if(ret != CL_OK )
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packin the object handle data. rc[0x%x]", ret));
            
        }
        else
            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_TO_PACK);
      }
    return ret;
}

static 
ClRcT 
_corObjUnpackFun(ClUint32T idx,
                 void * element, 
                 void ** dest 
                 )
{
    typedef struct bf {
        ClUint32T tg;
        ClUint32T idx;
    } bf_t;

    void **buf = (*((CORUnpackObj_h *)(dest)))->data;
    ClInt16T instanceId = (*((CORUnpackObj_h *)(dest)))->instanceId;
    CORObject_h obj = (CORObject_h)element;

    if(buf && obj)
      {
        bf_t tmp;
        ClUint32T data;
        ClUint32T tag;

        STREAM_PEEK(&tmp, (*buf), sizeof(bf_t));
		tmp.tg = ntohl(tmp.tg);
		tmp.idx = ntohl(tmp.idx); 
        /* verify both tag and index before picking up */ 
        if(tmp.tg == COR_OBJ_TAG && idx == tmp.idx)
          {
            DMObjHandle_h tmpOH;
            STREAM_OUT_NTOHL(&tag, (*buf), sizeof(tag));
            /* read the index */
            STREAM_OUT(&data, (*buf), sizeof(data));
            /* read the obj data here */
            STREAM_OUT_NTOHL(&obj->dmObjH.classId, (*buf), sizeof(obj->dmObjH.classId));
            STREAM_OUT_NTOHL(&obj->dmObjH.instanceId, (*buf), sizeof(obj->dmObjH.instanceId));

            STREAM_OUT_NTOHS(&obj->type, (*buf), sizeof(obj->type));
            STREAM_OUT_NTOHS(&obj->flags, (*buf), sizeof(obj->flags));
            /* stream out the dm object here */
            /* update the instance id from dm routine here */
            data  = dmObjHandleUnpack(*buf, &tmpOH, instanceId); 
            *buf = (char *)( *buf) + data;
            if(data)
              {
                /* copy the updated dm handle here */
                obj->dmObjH = *tmpOH;
                /* free the tmp object handle here */
                clHeapFree(tmpOH);
              }
            else
              {
                dmBinaryShow((Byte_t*)"Error Packet (dumping 32 bytes before & after)", 
                             (Byte_t *)(*buf)-32, 
                             64);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Unpack of object Failed"));
              }
          }
        else
          {
            /* undo the stream of TAG */
          }
      }
    return CL_OK;
}

/* show routine for the object tree 
 */
static 
ClRcT 
_corObjShowFun(ClUint32T idx,
               void * element, 
               void ** cookie
               )
{
    ClRcT rc = CL_OK;
    CORObject_h obj = (CORObject_h)element;
    ClCharT corStr[CL_COR_CLI_STR_LEN] = {0};
    static ClCharT msoString[CL_COR_CLI_STR_LEN] = {0};
    static ClBoolT msoExists = CL_FALSE;
    static ClCharT buff[CL_COR_CLI_STR_LEN] = {0};

    int** lvls = (int**) cookie;
    ClBufferHandleT * pMsg = (*(MArrayWalk_t**)cookie)->pMsg;

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

            /* Print the MSOs */
            if (msoExists == CL_TRUE)
            {
                /* To remove the trailing ', ' from the string */
                msoString[strlen(msoString)-2] = '\0';

                sprintf(corStr, "%s(%s)", buff, msoString);
                clBufferNBytesWrite(*pMsg, (ClUint8T *) corStr, strlen(corStr));
                memset(msoString, 0, CL_COR_CLI_STR_LEN);
                msoExists = CL_FALSE;
            }
        }
    }
    else
    {
        if(obj->dmObjH.classId) 
        {
            ClCharT className[CL_MAX_NAME_LENGTH] = {0};
            ClBoolT classNameExists = CL_FALSE;

            COR_PRINT_SPACE_IN_BUFFER(**lvls, buff);

            rc = _corNiKeyToNameGet(obj->dmObjH.classId, className);
            classNameExists = (rc == CL_OK ? CL_TRUE : CL_FALSE);

            if (obj->type == CL_COR_OBJ_TYPE_MO)
            {
                if (classNameExists == CL_TRUE)
                    sprintf(corStr, "%s\\%s:%d", buff, className, idx);
                else
                    sprintf(corStr, "%s\\%d:%d", buff, obj->dmObjH.classId, idx);
            
                clBufferNBytesWrite (*pMsg, (ClUint8T *) corStr, strlen(corStr));
            }
            else
            {
                msoExists = CL_TRUE;

                /* Show the MSOs as comma separated strings */
                switch (idx)
                {
                    case CL_COR_SVC_ID_FAULT_MANAGEMENT:
                        strcat(msoString, "fault");
                        break;

                    case CL_COR_SVC_ID_ALARM_MANAGEMENT:
                        strcat(msoString, "alarm");
                        break;

                    case CL_COR_SVC_ID_PROVISIONING_MANAGEMENT:
                        strcat(msoString, "provisioning");
                        break;

                    case CL_COR_SVC_ID_PM_MANAGEMENT:
                        strcat(msoString, "performance monitoring");
                        break;

                    case CL_COR_SVC_ID_AMF_MANAGEMENT:
                        strcat(msoString, "amf");
                        break;

                    case CL_COR_SVC_ID_CHM_MANAGEMENT:
                        strcat(msoString, "chassis");
                        break;

                    default:
                        strcat(msoString, "invalid");
                        break;
                }

                strcat(msoString, ", ");

#if 0
                ClCharT msoString[32] = {0};

                strcpy(msoString, clCorServiceIdString[idx]);
                strcat(msoString, " MSO");

                if (classNameExists == CL_TRUE)
                    sprintf(corStr, "%s*%-12s (Service Id: [%d], Class Id: [0x%08x], Class Name: [%s])", 
                            buff, msoString, idx, obj->dmObjH.classId, className);
                else
                    sprintf(corStr, "%s*%-12s (Service Id: [%d], Class Id: [0x%08x])",
                            buff, msoString, idx, obj->dmObjH.classId);
#endif
            }
                
#if 0
            clBufferNBytesWrite (*pMsg, (ClUint8T *) corStr, strlen(corStr));

            sprintf(corStr, " [DM Inst Id: 0x%08x]", obj->dmObjH.instanceId);
            clBufferNBytesWrite (*pMsg, (ClUint8T *) corStr, strlen(corStr));
#endif
        }
    }

    return CL_OK;
}

/* show routine (show full object) for the object tree 
 */
#ifdef COR_TEST
static 
ClRcT 
_corObjShowDetailsFun(ClUint32T idx,
                      void * element, 
                      void ** cookie
                      )
{
    CORObject_h obj = (CORObject_h)element;
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
        if(obj->dmObjH.classId) 
          {
            char tmp[160];
            if(**lvls < 140)
              {
                memset(tmp,' ', **lvls);
                tmp[**lvls]=0;
                clOsalPrintf("%s", tmp);
              }
            clOsalPrintf("%s", (obj->type==CL_COR_OBJ_TYPE_MSO?"*":""));
            /* GAS: Totally broken: DM_OH_SHOW_DETAILS(&(obj->dmObjH), idx); */
          }
      }
    return CL_OK;
}
#endif

#ifdef DEBUG

/* show routine (show only names) for the object tree 
 */
static 
ClRcT 
_corObjShowNamesFun(ClUint32T idx,
                    void * element, 
                    void ** cookie
                    )
{
    CORObject_h obj = (CORObject_h)element;
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
        if(obj->dmObjH.classId) 
          {
            CORClass_h t;
            char name[CL_COR_MAX_NAME_SZ]= {"\0"};
            COR_PRINT_SPACE(**lvls);            
            DM_OH_GET_TYPE(obj->dmObjH, t);
            _corNiKeyToNameGet(t->classId,name); 
            clOsalPrintf("%s", (obj->type==CL_COR_OBJ_TYPE_MSO?"*":""));
            clOsalPrintf("[%d] %s", idx, t?name:"");
          }
      }
    return CL_OK;
}

#endif


#if REL2
ClRcT moIdGetNextFreeSlot(ClCorMOIdPtrT moId, int *idx)
{
        ClRcT rc = CL_OK;
	ClCorMOIdPtrT moId = (ClCorMOIdPtrT)id;
        ClCorMOIdT tmpMoId = *moId;

        /* check if objtree is present */
        if(!objTree)
            return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

        if( clCorMoIdValidate(moId) != CL_OK)
        {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClCorMOId is invalid.\n"));
	/*	clCorMoIdShow(moId);*/
                return CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH);
        }

        if(!corObjTreeFindParent(objTree, moId) )
        {
            /* parent unknown */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
        }

        tmpMoId.depth = moId->depth - 1;


        if( (rc = clCorMoIdFirstInstanceGet(&tmpMoId) ) == CL_OK)
        {
                /* get the siblings, till there are no more left or there is an error.*/
                while(clCorMoIdNextSiblingGet(&tmpMoId) == CL_OK)
		{
			*idx = tmpMoId.node[tmpMoId.depth - 1].instance;
		}
		(*idx)++;
        }
	else
	{
		 /* it should be the first child */
		*idx = 0; 
		rc= CL_OK;
	}
        return rc;
}
#endif

