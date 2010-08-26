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
 * File        : clCorObjCont.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Object Tree traversal and data get/set routines.
 *****************************************************************************/
/* FILES INCLUDED */

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clRuleApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clCorApi.h>

/* Internal Headers*/
#include "clCorTreeDefs.h"
#include "clCorClient.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorObj.h"

#include <xdrCorObjFlagNWalkInfoT.h>
#include <xdrClCorObjectHandleIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* local definitions */
static ClRcT _corObjWalkFun(ClUint32T idx, void * element, void ** buf);

char *  pObjHdlList = NULL;
//static int objHdlCount = 0;

extern _ClCorServerMutexT gCorMutexes;

extern ClRcT corMOIdExprCreate(ClCorMOIdPtrT moId, ClRuleExprT* *pRbeExpr);

extern ClCorInitStageT    gCorInitStage;

/**
 *  get the DM Handle from the given object handle
 */
ClRcT
objHandle2DMHandle(ClCorObjectHandleT oh,
                   DMObjHandle_h dmh)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT path; 

    CL_FUNC_ENTER();
    if(!dmh)
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorObjectHandleToMoIdGet(oh, &path, NULL);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "DMH", "Failed to get the moId from object handle. rc [0x%x]", rc);
        return rc;
    }

    rc = moId2DMHandle(&path, dmh);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "DMH", "Failed to get the DM handle from the object handle. rc [0x%x]", rc);
        return rc;
    }

#if 0
    if((ret=corOH2MOIdGet((COROH_h)(oh).tree, &path))==CL_OK)
      { 
         ret=moId2DMHandle(&path, dmh);
      }
#endif

    CL_FUNC_EXIT();
    return rc;
}

/**
 * get DM Handle from given ClCorMOId
 */
ClRcT
moId2DMHandle(ClCorMOIdPtrT path,
              DMObjHandle_h dmh)
{
    ObjTreeNode_h obj;
    CORObject_h dmObj;

    CL_FUNC_ENTER();
    
    if(!path || !dmh)
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(dmh,0,sizeof(DMObjHandle_t));
    obj = corMOObjGet(path);
    if(obj)
    {
        ClCorMOServiceIdT svcId=(path)->svcId;
        
        dmObj = obj->data;
        if(svcId!=CL_COR_INVALID_SRVC_ID)
        { 
            dmObj=corMSOObjGet(obj, svcId);
        } 

        if (dmObj != NULL && dmObj->dmObjH.classId != 0)
        {
            (dmh)->classId = dmObj->dmObjH.classId;
            (dmh)->instanceId = dmObj->dmObjH.instanceId;
        }
        else
        {
            clLogInfo("COR", "OBH", 
                    "The Dm object node doesn't exist. rc[0x%x]", CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
            return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
        }
    } 
    else
    {
	    clLogInfo("COR", "OBJ", 
                "The object node doesn't exist in the object tree. rc [0x%x]", CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
	    return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 * Get the DM Object from the MoId.
 */
                       // coverity[pass_by_value]
Byte_h _corMoIdToDMBufGet(ClCorMOIdT moId)
{
    ClRcT           rc      = CL_OK;
    DMObjHandle_t   dmH     = {0};
    CORClass_h      pClassH = NULL;
    Byte_h          objBuf  = NULL;
    ClCharT         moIdStr[CL_MAX_NAME_LENGTH] = {0};

    CL_FUNC_ENTER();

    _clCorMoIdStrGet(&moId, moIdStr);

    rc = moId2DMHandle(&moId, &dmH);
    if(CL_OK != rc)
    {
        clLogError("TXN", "PRE", "Failed while getting the DM handle for the MO[%s]. rc[0x%x]", moIdStr, rc);
        CL_FUNC_EXIT();
        return NULL;
    } 

    DM_OH_GET_TYPE(dmH, pClassH);
    if(NULL == pClassH)
    {
        clLogError("TXN", "PRE", "Failed while getting the class handle for MO[%s].", moIdStr);
        CL_FUNC_EXIT();
        return NULL;
    }

    objBuf = corVectorGet(&(pClassH->objects), dmH.instanceId);
    if(NULL == objBuf)
    {
        clLogError("TXN", "PRE", "The object buffer is NULL for the MO [%s]", moIdStr);
        CL_FUNC_EXIT();
        return NULL;
    }

    CL_FUNC_EXIT();
    return objBuf;
}


/*
 *  TODO:
 * 1 - walk everything
 * 2 - MSO Walk
 * 3 - top down
 * 4 - bottom up 
 * make the return of the function to ClRcT, so it can be stopped
 */
ClRcT _clCorObjectWalk( ClCorMOIdPtrT cAddr, 
	    ClCorMOIdPtrT           cAddrWC,
        CORObjWalkFP            fp,
        ClCorObjWalkFlagsT      flags,
        ClBufferHandleT         bufferHandle
)
{
    ClCorMOIdT tmpMOId;
    ClCorMOIdPtrT cookie = &tmpMOId;  
    ObjTree_t tmpTree;
    MArrayWalk_t tmpWalkDetails={0};
    MArrayWalk_h walkCookie = &tmpWalkDetails;
    ClRuleExprT* pRbeExpr = NULL;
    ClRcT rc = CL_OK;
    
    CL_FUNC_ENTER();

    /* todo: The cookie can be OH instead of ClCorMOId for optimizations
     */
    if(!objTree)
         return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      tmpTree = *objTree;

    /* do some validations to parameters */
    if(!fp)
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    if(cAddr && cAddr->depth == 0)
        cAddr = NULL;

    if(cAddrWC &&  cAddrWC->depth == 0)
        cAddrWC = NULL;

    if((flags > CL_COR_MSO_WALK_UP) || (flags < CL_COR_MO_WALK))
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    if(cAddr &&
       (((cAddr->svcId != CL_COR_INVALID_SVC_ID) &&
         ((flags == CL_COR_MO_WALK) || (flags == CL_COR_MO_WALK_UP))) ||
        ((cAddr->svcId < CL_COR_INVALID_SVC_ID || cAddr->svcId >= CL_COR_SVC_ID_MAX) &&
         ((flags == CL_COR_MSO_WALK) || (flags == CL_COR_MSO_WALK_UP)))))
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID);
    }

    /* set the root of the walk here */
    if(cAddr)
    {
        if(cAddr->depth>0)
          {
            tmpTree.root = corObjTreeNodeFind(objTree, cAddr);
          }
        *cookie = *cAddr; 
    }
    else
    {
        clCorMoIdInitialize(cookie);
    }
     
    if(tmpTree.root)
    {
		int idx = 0;
        switch(flags)
        {
            case CL_COR_MO_SUBTREE_WALK:
                tmpWalkDetails.flags = CL_COR_MO_SUBTREE_WALK;
                tmpWalkDetails.fpNode = (MArrayNodeFP)fp;
                tmpWalkDetails.fpData = 0;
            break;

            case CL_COR_MO_WALK:
            case CL_COR_MO_WALK_UP:
                tmpWalkDetails.fpNode = (MArrayNodeFP)fp;
                tmpWalkDetails.fpData = 0;
            break;

            case CL_COR_MSO_SUBTREE_WALK:
                tmpWalkDetails.flags = CL_COR_MSO_SUBTREE_WALK;
                tmpWalkDetails.fpNode = 0;
                tmpWalkDetails.fpData = (MArrayNodeFP)fp;
                break;

            case CL_COR_MSO_WALK:
            case CL_COR_MSO_WALK_UP:
                tmpWalkDetails.fpNode = 0;
                tmpWalkDetails.fpData = (MArrayNodeFP)fp;
                break;

            default:
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        } 

        if (bufferHandle)
            tmpWalkDetails.pMsg  =  &bufferHandle;

        tmpWalkDetails.flags =  flags;
        tmpWalkDetails.cookie = (void **) &cookie;

        if(cAddrWC && corMOIdExprCreate(cAddrWC,&pRbeExpr) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("MOId to Expression Creation Failed : [0x%x]",rc));
            return rc;
        }

        tmpWalkDetails.expr = pRbeExpr;

        if(cookie && cookie->depth)
            idx = clCorMoIdToInstanceGet(cookie);

        mArrayWalk(idx,
                   &tmpTree, 
                   _corObjWalkFun, 
                   _corObjWalkFun, 
                   (void **)&walkCookie,
                   rc);
    }

    if(pRbeExpr)
    	clRuleExprDeallocate(pRbeExpr);

    CL_FUNC_EXIT();
    return 0;
}

#ifdef GETNEXT

/**
 *  Function to get the next object based on filter.
 *
 */

ClRcT 
_clCorNextMOGet(ClCorObjectHandleT stOH, 
           ClCorClassTypeT Id,
	   ClCorServiceIdT  svcId,
           ClCorObjectHandleT *nxtOH
           )
{
    ClRcT rc = CL_OK;
    ClCorMOIdPtrT tmpMOId;
    ObjTreeNode_h node;
    CORObject_h   svcObjH;

    ClUint8T clsFound = 0;
    ClUint8T svcFound = 0;

      /* check if objtree is present */
      if(!objTree)
         return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    /*Steps
     * 1. Get the next node from given handle.
     *     if given handle is empty, get first node of tree.
     * 2. Apply the filter on the obtained node's moId.
     * 3. Repeat step 2 unless filtering criterion passes 
     *     OR we reach at last node in tree.
     */
    if(CL_OK != (rc = clCorMoIdAlloc(&tmpMOId)))
       {
       	    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Allocate MoId") );
       	    return (rc);   
       }	       

    if(CL_COR_OBJ_HANDLE_ISNULL(stOH))
    {
      node = corObjTreeNextNodeFind(objTree, tmpMOId);
    }
    else
    {
       if(CL_OK != (rc = corOH2MOIdGet((COROH_h)&stOH.tree, tmpMOId)))
       {
	    clCorMoIdFree(tmpMOId);
       	    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid Object Handle.") );
       	    return (CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE));   
       }	       
       node = corObjTreeNextNodeFind(objTree, tmpMOId);
    }

    /* Apply Filter */
    while(node && !(clsFound && svcFound))
    {
       if(Id == CL_COR_CLASS_WILD_CARD ||
	              tmpMOId->node[tmpMOId->depth-1].type == Id)
	   { 
 	       clsFound = 1;
	       if(svcId == CL_COR_SVC_WILD_CARD ||
    			   (svcObjH = corMSOObjGet(node, svcId))?!COR_OBJECT_ISNULL(*svcObjH):0)
	       {
		 svcFound = 1;
		 break;
	       }
	       /* reset clsFound*/
	       clsFound = 0;
	   }
      /* Filter criterion failed -  Get next object. */	   
      node = corObjTreeNextNodeFind(objTree, tmpMOId);
    }
 
    /* Found the node. */
    if(node)
    { 
	/* @todo: do we need to fill dm part of handle ? */     
        if(CL_OK != (rc = corMOId2OHGet(tmpMOId, (COROH_h)nxtOH->tree)))
	{
	   /* Critical Error.
	    * MoId contents have been corrupted.
	    */
	   clCorMoIdFree(tmpMOId);
           CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,
	  	       ( "MoId contents are corrupted. Error[0x%x]",rc));
       	   return (rc);   
	}
    }else
        {  /* Node is not found
	    Reset handle to NULL
	    */
	  CL_COR_OBJ_HANDLE_INIT(*nxtOH);
	}  
   clCorMoIdFree(tmpMOId);
   return (rc);
}
 
#endif

CORObject_h
corMSOObjSet(ObjTreeNode_h oh,
             ClCorServiceIdT   svcId,
             DMObjHandle_h dmObj
             )
{
    CORObject_h obj = 0;

    CL_FUNC_ENTER();
    
    if(oh)
      {
        obj = mArrayExtendedDataGet(oh, 0, svcId); /* 0 - MSO */
        if(obj)
          {
            /* obj->dmObjH = *dmObj; */
            obj->dmObjH.classId = dmObj->classId;
	    obj->dmObjH.instanceId = dmObj->instanceId;
            obj->type   = CL_COR_OBJ_TYPE_MSO;
            obj->flags  = CL_COR_OBJ_FLAGS_DEFAULT;
          }
      }

    CL_FUNC_EXIT();
    return (obj);
}


/* ----[Internal routines]-------------------------------------------- */

static
ClRcT
_corObjWalkFun(ClUint32T idx,
               void * element,
               void ** cookie
               )
{
    CORObject_h obj = (CORObject_h)element;
    MArrayWalk_h* walkInfo = (MArrayWalk_h*) cookie;
    ClCorMOIdPtrT* oh = (ClCorMOIdPtrT*) (*walkInfo)->cookie;
    ClRuleExprT* pRbeExpr = (ClRuleExprT *) (*walkInfo)->expr;

    if(!element)
    {
        if(idx==COR_FIRST_NODE_IDX)
        {
            (*oh)->depth ++;
        } 
        if(idx==COR_LAST_NODE_IDX) 
        {
           /*  (*oh)->depth --; */
	        clCorMoIdTruncate(*oh, (*oh)->depth-1);
        }
    }
    else
    {
        if(oh && obj->dmObjH.classId)
        {
            if(obj->type == CL_COR_OBJ_TYPE_MO)
            {
                (*oh)->node[(*oh)->depth-1].type = obj->dmObjH.classId;
                (*oh)->node[(*oh)->depth-1].instance = idx;
 	
		        if( pRbeExpr == NULL || 
			        (clRuleExprEvaluate(pRbeExpr, (ClUint32T*) *oh, sizeof(ClCorMOIdT)) == CL_RULE_TRUE) )
		        {
                    if((*walkInfo)->fpNode && 
                            _clCorObjectValidate(*oh) == CL_OK)
//                         corMoId2OHGet((*oh), (COROH_h) objHandle.tree)==CL_OK)
                    {
                        CORObjWalkFP fp = (CORObjWalkFP)(*walkInfo)->fpNode;
                        ClCorObjWalkFlagsT flag = (*walkInfo)->flags;
                        ClCorMOIdT tempMoId = {{{0}}};

                        memcpy(&tempMoId, (*oh), sizeof(ClCorMOIdT));                        
                        clCorMoIdServiceSet(&tempMoId, CL_COR_INVALID_SVC_ID);

                        /* call the mo walk function. 
                         * Don't call this function while traversing bottom-up.
                         */
                        if(flag != CL_COR_MO_SUBTREE_WALK)
                        {
                            if ((*walkInfo)->pMsg && (*(*walkInfo)->pMsg))
                                (*fp)((void *) &tempMoId, *(*walkInfo)->pMsg);
                            else
                                (*fp)((void *) &tempMoId, 0);
                        }
                     }
		        }                
            }
            else 
                if(obj->type == CL_COR_OBJ_TYPE_MSO && 
                    (*walkInfo)->fpData &&
                    ((*oh)->svcId == CL_COR_INVALID_SVC_ID  ||
                    (*oh)->svcId == idx ) )
                {
                    /*
                     * since same function is used for MO/MSO, the depth
                     * will be one level more, so subtract it for now.
                     */
                  
                    (*oh)->depth --; 
	       	        if( pRbeExpr == NULL ||  
  			            (clRuleExprEvaluate(pRbeExpr, (ClUint32T*) *oh, sizeof(ClCorMOIdT)) == CL_RULE_TRUE) )
     		        {
                         if(_clCorObjectValidate(*oh)==CL_OK)
                         {

                             CORObjWalkFP fp = (CORObjWalkFP)(*walkInfo)->fpData;
                             ClCorObjWalkFlagsT flag = (*walkInfo)->flags;
                             ClCorMOIdT tempMoId = {{{0}}};

//                             void * param = (void *) &objHandle;

                             /* call the mso walk function */
//                             corOHServiceSet((COROH_h)objHandle.tree,idx);
                             
                             memcpy(&tempMoId, (*oh), sizeof(ClCorMOIdT));
                             clCorMoIdServiceSet(&tempMoId, idx);

                             if(flag != CL_COR_MSO_SUBTREE_WALK)
                             {

                                 if ((*walkInfo)->pMsg && (*(*walkInfo)->pMsg))
                                    (*fp)((void *) &tempMoId, *(*walkInfo)->pMsg);
                                 else
                                     (*fp)((void *) &tempMoId, 0);
                             }
                         } 
		            }
                       
                    (*oh)->depth ++; 
                }            
        } 
    }

    return CL_OK;
}

void _corObjHdlListGet(void* pMoId, ClBufferHandleT buffer)
{
    ClRcT rc = CL_OK;
    ClCorObjectHandleT objH = NULL;
    VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIDL = {0};

    CL_FUNC_ENTER();

    
    rc = clCorMoIdToObjectHandleGet((ClCorMOIdT *) pMoId, &(objHIDL.oh));
    if (rc != CL_OK)
    {
        clLogError("OBJ", "GET", "Failed to get the object handle. rc [0x%x]", rc);
        return;
    }

    rc = clCorObjectHandleSizeGet(objHIDL.oh, &(objHIDL.ohSize));
    if (rc != CL_OK)
    {
        clLogError("OBJ", "GET", "Failed to get the object handle size. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return;
    }

    rc = VDECL_VER(clXdrMarshallClCorObjectHandleIDLT, 4, 1, 0) ((void *)&objHIDL, buffer, 0);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "GET", "Failed to marshall ClCorObjectHandleIDLT. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return;
    }

    clCorObjectHandleFree(&(objHIDL.oh));

    CL_FUNC_EXIT();
    return;
}

void _corObjSubtreeDelete(void * pMoId, ClBufferHandleT buffer)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;

    CL_FUNC_ENTER();

    moId = *(ClCorMOIdT *) pMoId;

    clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);

    rc = clCorUtilMoAndMSODelete(&moId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Failed to delete Mo and Mso objects. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return;
    }

    CL_FUNC_EXIT();
    return;
}

                         // coverity[pass_by_value]
ClRcT _clCorSubTreeDelete(ClCorMOIdT moId)
{
    ClRcT       rc = CL_OK;

    CL_FUNC_ENTER();

    rc = _clCorObjectWalk(&moId, NULL, _corObjSubtreeDelete, CL_COR_MO_SUBTREE_WALK, 0);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Failed to walk the object tree. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    
    CL_FUNC_EXIT();
    return CL_OK;
}


ClRcT VDECL(_corObjectWalkOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corObjFlagNWalkInfoT* pData = NULL;
    CL_FUNC_ENTER();

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("OBW", "EOF", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pData = clHeapAllocate(sizeof(corObjFlagNWalkInfoT));
    if(!pData)
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
           return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }

    if((rc = VDECL_VER(clXdrUnmarshallcorObjFlagNWalkInfoT, 4, 0, 0)(inMsgHandle, (void *)pData)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to Unmarshall corObjFlagNWalkInfoT"));
            clHeapFree(pData);
        return rc;
    }

	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pData->operation)
    {
        case COR_OBJ_WALK_DATA_GET:
            clOsalMutexLock(gCorMutexes.gCorServerMutex);

#if 0            
            objHdlCount = 0;
            rc = _corObjectCountGet(&iCount);
            pObjHdlList = (char *) clHeapAllocate(iCount*sizeof(ClCorObjectHandleT));
	   	    if(pObjHdlList == NULL)
	        {
       			 clHeapFree(pData);
            	 clOsalMutexUnlock(gCorMutexes.gCorServerMutex); 
			  	 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
 				 CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
				 return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	    	}

            clLogTrace("OBW", "EFN", "Going for the object walk now");
#endif

            rc = _clCorObjectWalk(&pData->moId, &pData->moIdWithWC, _corObjHdlListGet, pData->flags, outMsgHandle);
            if (CL_OK != rc)
            {
                clLogError("OBW", "EFN", 
                        "Failed to do the object walk on server. rc[0x%x]", rc);
            }
#if 0
            else
            {
	            rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *)pObjHdlList,
            	                (ClUint32T)objHdlCount * sizeof(ClCorObjectHandleT));
                if (CL_OK != rc)
                    clLogError("OBW", "EFN", 
                            "Failed to write the object walk information into the out buffer. rc[0x%x]", rc);
            }

            clLogTrace("OBW", "EFN", "Done with the object walk");

            clHeapFree(pObjHdlList);
#endif
            clOsalMutexUnlock(gCorMutexes.gCorServerMutex); 
        break;
        case COR_OBJ_SUBTREE_DELETE:
           clOsalMutexLock(gCorMutexes.gCorServerMutex);
           rc = _clCorSubTreeDelete(pData->moId);
           clOsalMutexUnlock(gCorMutexes.gCorServerMutex); 
        break;
        default:
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "INVALID OPERATION, rc = %x", rc) );
             rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        break;
    }
   
    CL_FUNC_EXIT();
    clHeapFree(pData);
    return rc;
}

#ifdef GETNEXT
ClRcT VDECL(_corObjectGetNextOp) (ClUint32T cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corObjGetNextInfo_t* pData = NULL; 
    CL_FUNC_ENTER();

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("OBN", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pData = clHeapAllocate(sizeof(corObjGetNextInfo_t));
    if(pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);   
    }
    if((rc = clXdrUnmarshallcorObjGetNextInfo_t(inMsgHandle, (void *)pData)) != CL_OK)
	{
		clHeapFree(pData);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corObjGetNextInfo_t"));
		return rc;
	}

  	clCorClientToServerVersionValidate(version, rc);
  	if(rc != CL_OK)
	{
		clHeapFree(pData);
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
	}
	
    switch(pData->operation)
    {
        case COR_OBJ_OP_NEXTMO_GET:
		{
              ClCorObjectHandleT nxtOH;
              rc =  _clCorNextMOGet(pData->objHdl, pData->classId, pData->svcId, &nxtOH);
              /* Write to the message*/
              clBufferNBytesWrite (outMsgHandle, (ClUint8T *)&nxtOH, sizeof(ClCorObjectHandleT));
        } 
        break;
        default:
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "INVALID OPERATION, rc = %x", rc) );
             rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        break;
    }
   
    CL_FUNC_EXIT();
	clHeapFree(pData);
    return rc;
}


#endif
