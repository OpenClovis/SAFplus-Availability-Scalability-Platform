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
 * File        : clCorMOTreeManage.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This module contains MOTree Utils API's & Definition
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>

/* Internal Headers*/
#include "clCorTreeDefs.h"
#include "clCorDmProtoType.h"
#include "clCorPvt.h"


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/**
 *  MOTree walk function pointer typedef.
 */
/* TODO: This needs to be moved, when MO Tree walk is exposed. */
typedef ClRcT (*ClCorMOTreeWalkFuncT)(void * data);


/* local definitions */
extern ClRcT corNiKeyToNameGet(ClCorClassTypeT key, char *name );

static ClRcT _corMOPackFun(ClUint32T idx, void * element, void ** buf);
static ClRcT _corMOUnpackFun(ClUint32T idx, void * element, void ** buf);
static ClRcT _corMSOPackFun(ClUint32T idx, void * element, void ** buf);
static ClRcT _corMSOUnpackFun(ClUint32T idx, void * element, void ** buf);
#ifdef COR_TEST
static ClRcT _corMOShowFun(ClUint32T idx, void * element, void ** buf);
static ClRcT _corMSOShowFun(ClUint32T idx, void * element, void ** buf);
#endif

extern ClRcT corMOTreeClass2IdxConvert(ClCorMOIdPtrT moId);
extern ClRcT  clCorMoClassPathValidate(ClCorMOClassPathPtrT this);

/* global variables */
MOTree_h moTree=0;

/**
 *  Intialize the Managed Object Class Tree library module.
 *
 *  This API intialize the Managed Object Class Tree library module.
 *                                                                        
 *  @param none
 *
 *  @returns 
 *    CL_OK  - Success<br>
 *
 */
ClRcT
corMOTreeCreate(MOTree_h *this)
{
    ClRcT ret=CL_OK;
  
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "initialize moTree"));
    if(this)
      {
        /* create and initialize the mo tree root here */
        ret = mArrayCreate(COR_MO_SIZE, this);
        if(ret==CL_OK)
        {
            _CORMOClass_h  tmp = (_CORMOClass_h)((*this)->root->data);
            tmp->numChildren = 1;

            (*this)->root->id = 1;

            /* set the pack, unpack function pointers */
            mArrayNodePackFPSet((**this), _corMOPackFun);
            mArrayNodeUnpackFPSet((**this), _corMOUnpackFun);

            mArrayDataPackFPSet((**this), _corMSOPackFun);
            mArrayDataUnpackFPSet((**this), _corMSOUnpackFun);
            
#ifdef DEBUG
            mArrayNameSet((*this), "motree");
            mArrayNodeNameSet((*this)->root, "/");
#endif
          }
      }

    CL_FUNC_EXIT();
    return(ret);
}

ClRcT 
_moTreeNodeDelete(ClUint32T Idx,
		  void* element,
		  void** cookie)
{
	ClRcT rc = CL_OK;
	MArrayNode_h moH = (MArrayNode_h) element;
	
	if(moH)
		CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Class Id is %d ", moH->id));	
	return rc;
}

ClRcT corMoTreeFinalize(void)
{
	ClCorMOClassPathT MoTreePath;
        ClCorMOClassPathPtrT cookie = &MoTreePath;
	MArrayWalk_t tmpWalkDetails = {0};
	MArrayWalk_h walkCookie = &tmpWalkDetails;
	MOTree_t tmpTree = *moTree;
	ClUint32T idx = 0;
	ClRcT rc = CL_OK;

        clCorMoClassPathInitialize(cookie);
        tmpWalkDetails.flags = CL_COR_MOTREE_WALK;
	tmpWalkDetails.fpNode = (MArrayNodeFP) _moTreeNodeDelete;
	tmpWalkDetails.fpData = (MArrayNodeFP) _moTreeNodeDelete;
	tmpWalkDetails.cookie = (void **) &cookie;
	
        mArrayDelete(idx, &tmpTree, _moTreeNodeDelete, _moTreeNodeDelete, (void **)&walkCookie, rc);	
    clHeapFree(moTree);
	return CL_OK;
}


/* @todo: add headers and comments to the below functions */
/* @todo: write code to validate all the nodes in the tree (useful after unpack) */
/* @todo: right now, the full tree is packed and unpacked the other
   side, this has to be fixed */

ClRcT
corMOTreeAdd(MOTree_h   this, 
             ClCorMOClassPathPtrT  path,
             ClUint32T maxInst)
{
    ClRcT  ret = CL_OK;

    CL_FUNC_ENTER();

    if(this && path)
      {
        MOTreeNode_h parent;
		MArrayVector_h vectorH = NULL;
        
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "inside corMOTreeAdd"));

        parent = corMOTreeFindParent(this, path);
        if(parent)
          {
            ClCorClassTypeT classId = clCorMoClassPathToMoClassGet(path);
            _CORMOClass_h parentMO = (_CORMOClass_h) parent->data;
            int slot = parentMO->numChildren;
            MArrayNode_h  node;

            /* check if node is there and then add 
             */
            node = mArrayNodeIdNodeFind(parent, classId);
            if(!node)
            {
                /*Root is handled differently where first child is at slot 0.*/
                if(slot != 0)
                    moPathGetNextFreeSlot(path, &slot);

                ret = mArrayNodeAdd(this,
                                    parent,
                                    1, /* MO so add it as 1 */
                                    slot, /* slot of this class */
                                    COR_MO_GRP_SIZE);
                if(ret == CL_OK)
                  {
                    node = mArrayNodeGet(parent, 1, slot);
                    if(node)
                      {
                        _CORMOClass_h  tmp;
                        ret = mArrayDataNodeAdd(node, 
                                                0, /* MSO vector init */
                                                COR_MSO_SIZE,
                                                COR_MSO_GRP_SIZE);
                        /* todo: check for ret value */
                        tmp = (_CORMOClass_h) node->data;
                        node->id      = classId;
                        tmp->numChildren = 1;  /* todo: replace to maxIndex */
                        tmp->flags    = 0;
                        tmp->maxInstances = maxInst;
                        tmp->instances = 0;
                        /* also update the parent with indexes */
                        parentMO->numChildren++;
						vectorH = mArrayGroupGet(parent, 1);
						if(vectorH != NULL)
							vectorH->numActiveNodes++;
                      }
                  }
              }
            else
              {
                /* node already found */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_FOUND)));
                ret = CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_FOUND);
              }
          }
        else
          {
            /* parent unknown */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
            ret = CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
      }
    else
      {
        /* invalid / null pointer */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_UTILS_ERR_INVALID_NODE_REF)));
        ret = CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_NODE_REF);
      }

    CL_FUNC_EXIT();
    return(ret);
}

ClRcT _clCorMoClassPathValidate(ClCorMOClassPathT* moPath, ClCorServiceIdT svcId)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classId = -1;

    rc = corMOTreeClassGet(moPath, svcId, &classId);
    if (rc != CL_OK)
    {
        clLogError("CLS", "VAL", "Failed to get the class info from the class path. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

/** 
 *
 * API to corMOTreeClassGet <Deailed desc>. 
 *
 *  @param path    corPath handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *
 */
ClRcT 
corMOTreeClassGet(ClCorMOClassPathPtrT path, 
                  ClCorMOServiceIdT srvcId,
                  ClCorClassTypeT* classId
                  )
{
    ClRcT  ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    CORMSOClass_h msoClass;
    CORMOClass_h  moClassHandle;

    CL_FUNC_ENTER();

    if(path && classId)
      {
        moClassHandle = corMOTreeFind(path);
        if (!moClassHandle)
          {
            CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to get MO Class info from MO Tree"));
            CL_FUNC_EXIT();
            return(CL_COR_SET_RC(CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT));
          }
        if(srvcId == CL_COR_INVALID_SVC_ID)
          {
            *classId = moClassHandle->id;
            CL_FUNC_EXIT();
            return (CL_OK);
          }
        else
          {
            msoClass = corMSOGet(moClassHandle,srvcId);
            if(msoClass && msoClass->classId)
              {
                *classId = msoClass->classId;
                CL_FUNC_EXIT();
                return (CL_OK);
              }
            else
              {
                ret = CL_COR_SET_RC(CL_COR_MO_TREE_ERR_CLASS_NO_PRESENT);
              }
          }
      }

    CL_FUNC_EXIT();
    return (ret);
}
                                            
/* 
 * API to verify the max Instance check  
 *
 *  @param path    corPath handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 */
ClRcT corMOInstaceValidate(ClCorMOIdPtrT   hMoId)
{
    _CORMOClass_h    hMoClass= 0;
    MOTreeNode_h     hNode   = 0;
    ClCorMOClassPathT        moPath;
    MOTreeNode_h     parent=0;
    ClCorClassTypeT   classId = 0;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH];

    if(NULL ==  hMoId)
        return CL_FALSE;
     
    if(!moTree)
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

    memset(&moPath, '\0', sizeof (ClCorMOClassPathT));

    /* Convert the MO Id to mo Path */
    if ( CL_OK != clCorMoIdToMoClassPathGet(hMoId, &moPath))
    {
        return CL_FALSE;
    }

    /* Get the MO Class hanlde for the given class*/
    if (NULL == (parent = corMOTreeFindParent(moTree, &moPath)))
    {
        return CL_FALSE;
    }
    
    classId = clCorMoClassPathToMoClassGet(&moPath);

    if (NULL == (hNode = mArrayNodeIdNodeFind(parent, classId)))
    {
        return CL_FALSE;
    }

    if (NULL == (hMoClass = (_CORMOClass_h) hNode->data))
    {
        return CL_FALSE;
    }

    /**  
     * The max instance should not be less than equal to zero.
     */ 
    if (hMoClass->maxInstances >= 0)
    {
        return  CL_TRUE;
    }
    else
    {
        clLogInfo("TXN", "PRE", "Max Instances [%d] configured is invalid for the MO [%s]", 
                hMoClass->maxInstances, _clCorMoIdStrGet(hMoId, moIdStr));
        return CL_FALSE;
    }
    return CL_TRUE; 
}

/**
 *  MO Tree pack routine.
 *
 *  This API packs the MO Tree.
 *                                                                        
 *  @param cAddr - COR path below which to pack the subtree.
 *  @param buf -  buffer to hold the pack
 *  @param size -  [IN/OUT] size of the buffer / size of the packed buffer
 *
 *  @returns 
 *    CL_OK - on Success
 */
ClRcT 
corMOTreePack (ClCorMOClassPathPtrT cAddr,
               ClBufferHandleT *pBuf
               )
{
    ClRcT ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    MOTree_t tmpTree;

   if(!moTree)
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   tmpTree = *moTree;

    if(pBuf)
      {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\n----------BEGIN MO TREE PACKET DATA--------------------"));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nCOR PATH: "));

		/* cAddr can be NULL, that means its a full pack from root */
		if(cAddr && cAddr->depth > 0)
	    	    tmpTree.root = corMOTreeNodeFind(moTree, cAddr);

		if(tmpTree.root){
	    	    ret = mArrayPack(&tmpTree, CL_COR_OBJ_FLAGS_ALL, MOTREE,  pBuf);
		}
		else{
	    	    ret = CL_OK;	
		}

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\n----------END MO TREE PACKET DATA--------------------"));
      }
    return(ret);
}

/**
 *  MO Tree unpack routine.
 *
 *  This API Unpacks the MO Tree.
 *                                                                        
 *  @param cAddr - COR path below which to unpack the subtree.
 *  @param buf -  buffer containing packed tree
 *  @param size -  [IN/OUT] size of the buffer / size of the unpacked buffer
 *
 *  @returns 
 *    CL_OK - on Success
 */
ClRcT 
corMOTreeUnpack (ClCorMOClassPathPtrT cAddr,
                 void *  buf,
                 ClUint32T* size
                 )
{
    ClRcT ret = CL_OK;
    MOTree_t tmpTree = {0};
    ClCorMOClassPathT tmpPath = {{0}};
    ClInt32T idx = 0;
 
    if(!moTree)
      return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    tmpTree = *moTree;

    /* cAddr can be NULL, if so, then its a full unpack at root */
    clCorMoClassPathInitialize(&tmpPath);
    if(buf && size)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                  ( "\n----------BEGIN MO TREE PACKET DATA--------------------"));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nCOR PATH: "));
	
        if(cAddr)
        {
            /*  1. If node is already present, the scenario is not supported as of now.
            *  2. Otherwise goto the parent. That must be available otherwise its an error.
            *	3. Find a free slot in the child node of the parent and unpack the info there.
            *	3 is required because the logical address we get from BD is 0 so we need to take 
            *	care to unpack it at an empty slot
            */
            tmpTree.root = corMOTreeFind(cAddr);

            if(!tmpTree.root)
            {
                tmpPath = *cAddr;
                if(tmpPath.depth > 1)
                {
                    tmpPath.depth--;
#ifdef DEBUG 
                    clCorMoClassPathShow(&tmpPath);
#endif
                    tmpTree.root = corMOTreeFind(&tmpPath);
                    if(!tmpTree.root)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "UNKNOWN Parent(can't merge)."));
                        return CL_ERR_NOT_IMPLEMENTED;
                    }
                }
                else
                    tmpTree.root = moTree->root;

                /*tmpTree.root = corMOTreeFind(&tmpPath);*/
            
                ret = moPathGetNextFreeSlot(cAddr, &idx);
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nNext free slot is %d",idx));
                if(ret == CL_OK)
                {
                    tmpTree.root = mArrayNodeGet(tmpTree.root, 
                                                    1,/* 1 is the node group vector */
                                                    idx);
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\nCould not get free slot to unpack the node.\n") );
                    return ret; 	
                }
            }	
        }
        dmBinaryShow((Byte_h) "Unpacked MOTree Packet", buf, *size);

        ret = mArrayUnpack(&tmpTree, MOTREE, buf, size);

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ( "\n----------END MO TREE PACKET DATA--------------------"));
    }
    else
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    return(ret);
}

/** prints the motree info */
#ifdef COR_TEST
void 
corMOTreeShowDetails()
{
    int lvl = 0;
    int* cookie = &lvl;
    ClRcT rc;
 
    if(!moTree)
      return;

    clOsalPrintf("MO Tree:");
    mArrayWalk(0, moTree, _corMOShowFun, _corMSOShowFun, (void**)&cookie, rc);
}
#endif

/** Given MOTree, path; finds the parent node
 */
MOTreeNode_h
corMOTreeFindParent(MOTree_h this,
                    ClCorMOClassPathPtrT path
                    )
{
    CL_FUNC_ENTER();

    if(this && path && path->depth>0)
      {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "inside corMOTreeFindParent"));
        if(path->depth>1)
          {
            return mArrayIdNodeFind(this, (ClUint32T *)path->node, path->depth-1);
          }
        else
          {
            return this->root;
          }
      }

    CL_FUNC_EXIT();
    return(0);
}

ClRcT 
corMOTreeNodeDel(ClCorMOClassPathPtrT path)
{
	ClRcT rc = CL_OK;
   	MArrayNode_h node = NULL;

        if(!moTree)
           return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

   	node = corMOTreeFind(path);
   	if(NULL == node)
   	{
       	CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not find node information in the tree."));
       	CL_FUNC_EXIT();
       	return (CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND));
   	}
    rc = mArrayDataNodeDelete(node, 0);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the MSO . rc[0x%x]", rc));
        return rc;
    }

	rc = mArrayNodeDelete(node);
	if(rc == CL_OK){

    	MOTreeNode_h parent = corMOTreeFindParent(moTree, path);
		MArrayVector_h vectorH = mArrayGroupGet(parent, 1);
        _CORMOClass_h parentMO = (_CORMOClass_h) parent->data;
        parentMO->numChildren--;

		node->id = 0;	
		memset(node->data, 0, sizeof(_CORMOClass_h) );
		if(vectorH != NULL)
			vectorH->numActiveNodes--;
		#if 0
		/* Not needed. We have seperate API to delete COR class*/
		moClassType = clCorMoClassPathToMoClassGet(path);
	
		rc = dmClassDelete(moClassType);
		if(rc != CL_OK){
       		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not delete dmClass."));
       		CL_FUNC_EXIT();
       		return rc;
   		}
		#endif
	}
	else{
      	CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not delete node from mArray"));
    	CL_FUNC_EXIT();
      	return rc;
	}
	return rc;
}

#ifdef DISTRIBUTED_COR
ClRcT corMOTreeUpdate()
{
	ClRcT ret = CL_OK;
    
        if(!moTree)
           return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

	ret = mArrayUpdate(0, moTree->root, 0);
	return ret;
}
#endif

/* ----[Pack,Unpack routines]-------------------------------------------- */

static 
ClRcT 
_corMOPackFun(ClUint32T idx,
              void * element, 
              void ** dest
              )
{
    _CORMOClass_h mo = (_CORMOClass_h) element;
    ClBufferHandleT *pBufH = (*((CORPackObj_h *)(dest)))->pBufH;

    if(NULL == pBufH )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(pBufH && mo && mo->numChildren)  
      {
        ClUint32T tag =COR_MO_TAG, tmpTag;
		ClUint32T tmpIdx ;
        /* First pack the tag, then index only */
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &tag, tmpTag, sizeof(tag));
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &idx, tmpIdx, sizeof(tmpIdx));
        /* now stream in MO data */

	/*Traversr MOTree to compute it*/
		/**** HTONL ****/
		ClUint32T numChildren, maxInstances, instances;
		ClUint16T  flags;
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &mo->numChildren, numChildren, sizeof(mo->numChildren));
        STREAM_IN_BUFFER_HANDLE_HTONS((*pBufH), &mo->flags, flags, sizeof(mo->flags));
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &mo->maxInstances, maxInstances, sizeof(mo->maxInstances));
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &mo->instances, instances,  sizeof(mo->instances));
      }
    return CL_OK;
}

static 
ClRcT 
_corMOUnpackFun(ClUint32T idx,
                void * element, 
                void ** dest
                )
{
    _CORMOClass_h mo = (_CORMOClass_h)element;
    ClUint32T maxInstances = 0, instances = 0;
    
    void * *buf = (*((CORUnpackObj_h *)(dest)))->data;    

    if(buf && mo)
      {
        ClUint32T data;
        ClUint32T tag;

        STREAM_OUT_NTOHL(&tag, (*buf), sizeof(tag));
        STREAM_PEEK_NTOHL(&data, (*buf), sizeof(data));
	/*Data match should not be done as it can be at a different location*/
        /* verify tag*/
        if(tag == COR_MO_TAG)
          {
            /* read the index */
            STREAM_OUT_NTOHL(&data, (*buf), sizeof(data));
            /* read the MO data here */
            STREAM_OUT_NTOHL(&mo->numChildren, (*buf), sizeof(mo->numChildren));
            STREAM_OUT_NTOHS(&mo->flags, (*buf), sizeof(mo->flags));
            STREAM_OUT_NTOHL(&maxInstances, (*buf), sizeof(mo->maxInstances));
            if(maxInstances > mo->maxInstances)
                mo->maxInstances = maxInstances; 
            STREAM_OUT_NTOHL(&instances, (*buf), sizeof(mo->instances)); 
          }
        else
          {
            /* undo the stream of MO TAG */
            (*buf) = (char *)(*buf) - sizeof(tag);
          }
      }
    return CL_OK;
}

static 
ClRcT 
_corMSOPackFun(ClUint32T idx,
               void * element, 
               void ** dest
               )
{
    CORMSOClass_h mso = (CORMSOClass_h) element;
    ClBufferHandleT *pBufH = (*((CORPackObj_h *)(dest)))->pBufH;

    if(NULL == pBufH )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR)));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    if(pBufH && mso && mso->classId)  
      {
        ClUint32T tag = COR_MSO_TAG;
        /* First pack the tag, then index only */
		ClUint32T tmpTag ;
		ClUint32T tmpIdx ;
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &tag, tmpTag, sizeof(tag));
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &idx, tmpIdx, sizeof(tmpIdx));
        /* now stream in MSO data */
		ClUint32T classId ;
		ClUint16T flags ;
        STREAM_IN_BUFFER_HANDLE_HTONL((*pBufH), &mso->classId, classId, sizeof(classId));
        STREAM_IN_BUFFER_HANDLE_HTONS((*pBufH), &mso->flags, flags, sizeof(flags));
      }
    return CL_OK;
}

static 
ClRcT 
_corMSOUnpackFun(ClUint32T idx,
                 void * element, 
                 void ** dest
                 )
{
    typedef struct bf {
        ClUint32T tg;
        ClUint32T idx;
    } bf_t;
    CORMSOClass_h mso = (CORMSOClass_h)element;
    void * *buf = (*((CORUnpackObj_h *)(dest)))->data;

    if(buf && mso)
      {
          bf_t  tmp;
        ClUint32T data;
        ClUint32T tag;

        STREAM_PEEK(&tmp, (*buf), sizeof(bf_t));
		tmp.tg = ntohl(tmp.tg);
		tmp.idx = ntohl(tmp.idx);
        /* verify both tag and index before picking up */
        if(tmp.tg == COR_MSO_TAG && idx == tmp.idx )
          {
			/***** NTOHL *****/
            STREAM_OUT_NTOHL(&tag, (*buf), sizeof(tag));
            /* read the index */
            STREAM_OUT_NTOHL(&data, (*buf), sizeof(data));
            /* read the MO data here */
            STREAM_OUT_NTOHL(&mso->classId, (*buf), sizeof(mso->classId));
            STREAM_OUT_NTOHS(&mso->flags, (*buf), sizeof(mso->flags));
          }
        else
          {
            /* undo the stream of MO TAG */
          }
      }
    return CL_OK;
}

/* ----[Show routines]-------------------------------------------- */
#ifdef COR_TEST
static 
ClRcT 
_corMOShowFun(ClUint32T idx,
              void * element, 
              void ** cookie
              )
{
    _CORMOClass_h mo = (_CORMOClass_h) element;

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
        if(mo->numChildren)  
          {
            COR_PRINT_SPACE(**lvls);
            clOsalPrintf("[Cls=0x%x Instances:%d Max:%d flgs=0x%x ]",
                     mo->numChildren,
                     mo->instances,
                     mo->maxInstances,
                     mo->flags);
          }
      }
    return CL_OK;
}

static 
ClRcT 
_corMSOShowFun(ClUint32T idx,
               void * element, 
               void ** cookie
               )
{
    CORMSOClass_h mso = (CORMSOClass_h) element;
    int** lvls = (int**) cookie;

    if(mso && mso->classId)  
      {
        COR_PRINT_SPACE(**lvls);
        clOsalPrintf("[Svc=%d Cls=0x%x flgs=0x%x] ",
                 idx,
                 mso->classId,
                 mso->flags);
      }
    return CL_OK;
}
#endif

/* This function gives the next free slot available in a node
 * @param : path (IN) : path of the node
 * @param : idx (OUT) : free slot number
 */ 

ClRcT moPathGetNextFreeSlot(void *corPath, int *idx)
{
	ClRcT rc = CL_OK;
	ClCorMOClassPathT tmpPath;
	ClCorMOClassPathPtrT path = (ClCorMOClassPathPtrT)corPath;
        MOTreeNode_h parent;

        if(!moTree)
           return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

	if( (rc = clCorMoClassPathValidate(path) ) != CL_OK)
	{
       	    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClCorMOClassPath is invalid. 0x%x\n",rc));
	    return CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH);
	}
	tmpPath = *path;

	if(path->depth)
            tmpPath.depth = path->depth - 1;

        parent = corMOTreeFindParent(moTree, path);
	if(!parent)
        {
            /* parent unknown */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
        }

	if( (rc = _clCorMoClassPathFirstChildGet(&tmpPath) ) == CL_OK)
	{
            /* get the siblings, till there are no more left.*/
            while(_clCorMoClassPathNextSiblingGet(&tmpPath) == CL_OK)
                    ;

            /* The class id's to be converted to appropriate index
             * so that we can find the next free index 
             */
            rc=mArrayId2Idx(moTree, (ClUint32T *)tmpPath.node, tmpPath.depth);
            if(rc==CL_OK)
            {
                *idx = tmpPath.node[tmpPath.depth - 1];
            }
            (*idx)++;
	} 
	else
	{
            _CORMOClass_h parentMO = (_CORMOClass_h) parent->data;
	    *idx = parentMO->numChildren;  /* it should be the first child */
	    rc= CL_OK;
	}
	return rc;
}

#if 0

ClRcT _moTreeWalk(ClUint32T idx, void * element, void ** cookie)
{
    _CORMOClass_h obj      = (_CORMOClass_h)element;
    MArrayWalk_h* walkInfo = (MArrayWalk_h*) cookie;
    ClCorMOTreeWalkFuncT  fp;
    ClCorMOClassTreeWalkInfoT classData;
    if (obj)
    {
        if(walkInfo)
        {
            memcpy(&classData, obj, sizeof(ClCorMOClassTreeWalkInfoT));
             
            fp  = (ClCorMOTreeWalkFuncT)(*walkInfo)->fpNode;
            if(fp)
            {
                    (fp)((void *) &classData);
            }
        }
    }
    return CL_OK;
}

	
ClRcT _msoTreeWalk(ClUint32T idx, void * element, void ** cookie)
{
    CORMSOClass_h obj      = (CORMSOClass_h)element;
    MArrayWalk_h* walkInfo = (MArrayWalk_h*) cookie;
    ClCorMOTreeWalkFuncT  fp;
    ClCorMOClassTreeWalkInfoT classData;
    memset(&classData,0, sizeof(ClCorMOClassTreeWalkInfoT));

    if (obj)
    {
        if(walkInfo)
        {
            memcpy(&classData, obj, sizeof(CORMSOClass_h));
             
            fp  = (ClCorMOTreeWalkFuncT)(*walkInfo)->fpNode;
            if(fp)
            {
                    (fp)((void *) &classData);
            }
        }
    }
    return CL_OK;
}


ClRcT
clCorMOTreeWalk(ClCorMOClassPathPtrT path, ClCorMOTreeWalkFuncT fptr)
{
    int idx = 0;
    ClCorMOClassPathPtrT cookie = path;
    MArrayWalk_t tmpWalkDetails={0};
    MArrayWalk_h walkCookie = &tmpWalkDetails;
    MOTree_t  tmpTree;
    ClRcT rc = CL_OK;

    if(!moTree)
         return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    tmpTree = *moTree;

    /* path can be NULL, that means its a full tree walk */
    if(path)
    {
        tmpTree.root = corMOTreeNodeFind(moTree, path);
    }
    tmpWalkDetails.flags = CL_COR_MOTREE_WALK;
    tmpWalkDetails.fpNode = (MArrayNodeFP) fptr;
    tmpWalkDetails.fpData = (MArrayNodeFP) fptr;

    tmpWalkDetails.cookie = (void **) &cookie;
                                                                                                                             
    if(path  && path->depth)
    {
        if((rc = mArrayId2Idx(moTree, (ClUint32T *)path->node, path->depth))!=CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOTreeClassIdxGet failed rc => \
[0x%x]", rc));
	return rc;
        }
        idx =  path->node[0];
    }
                                                                                                                             
    if(tmpTree.root)
    {
        mArrayWalk(idx, &tmpTree, _moTreeWalk, _msoTreeWalk, (void**)&walkCookie, rc);
    }
    clOsalPrintf("\n");
    return rc;
}

#endif
