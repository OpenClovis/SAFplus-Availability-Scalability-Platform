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
 * ModuleName  : ckpt                                                          
 * File        : clCkptHdl.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains functions related to ckpt client side handles.
*
*
*****************************************************************************/
#include  <string.h>
#include  <clCommon.h>
#include  <clIocApi.h>
#include  <clLogApi.h>
#include  <clHandleApi.h>
#include  <clCkptCommon.h>
#include  <clCkptUtils.h>
#include  <clCkptClient.h>
#include  <clCkptErrors.h>
#include  "ckptEockptServerCliServerFuncClient.h"
#include  <clCpmApi.h>

extern ClCkptClntInfoT gClntInfo;


/*
 * Function to create a checkpoint handle. This handle would be returned
 * to the application.
 */
ClRcT ckptLocalHandleCreate(ClCkptHdlT *pCkptHdl)
{
  ClRcT rc = CL_OK;

  rc = clHandleCreate(gClntInfo.ckptDbHdl,sizeof(CkptHdlDbT),(ClHandleT *)pCkptHdl);

  return rc;
}



/*
 * Function to copy and associate the checkpoint related information with
 * the checkpoint handle.
 */
 
ClRcT ckptHandleInfoSet(ClCkptHdlT ckptLocalHdl,CkptHdlDbT *pData)
{
    ClRcT       rc = CL_OK;
    CkptHdlDbT  *pHdlInfo = NULL;

    if(pData == NULL)
        return CL_CKPT_ERR_NULL_POINTER;
    rc = clHandleCheckout(gClntInfo.ckptDbHdl, ckptLocalHdl,
                           (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;

    if(pHdlInfo != NULL)    
    {
        pHdlInfo->hdlType        = pData->hdlType;
        pHdlInfo->activeAddr     = pData->activeAddr;
        pHdlInfo->ckptMasterHdl  = pData->ckptMasterHdl;
        pHdlInfo->ckptActiveHdl  = pData->ckptActiveHdl;
        pHdlInfo->openFlag       = pData->openFlag;
        pHdlInfo->prevMasterAddr = pData->prevMasterAddr;
        pHdlInfo->creationFlag   = pData->creationFlag;
        pHdlInfo->cksum          = pData->cksum;
        pHdlInfo->ckptSvcHdl     = pData->ckptSvcHdl;
        saNameCopy(&pHdlInfo->ckptName, &pData->ckptName);
        if(pHdlInfo->clientList.pClientInfo)
        {
            clHeapFree(pHdlInfo->clientList.pClientInfo);
        }
        memset(&pHdlInfo->clientList, 0, sizeof(pHdlInfo->clientList));
    }
    else
    {
         rc = CL_CKPT_ERR_NULL_POINTER; 
    }
    clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    return rc;
}



/*
 * Function to get the active replica's address, given the checkpoint handle.
 */
 
ClRcT ckptActiveAddressGet( ClCkptHdlT        ckptLocalHdl,
                            ClIocNodeAddressT *pNodeAddr)
{
    ClRcT               rc        = CL_OK;
    CkptHdlDbT          *pHdlInfo = NULL;

    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = ckptHandleCheckout( ckptLocalHdl,
            CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;
#if 0    
    /*
     * In case of SYNC checkpoints, local server is considered as active
     * replica.
     */
    if((pHdlInfo->creationFlag & CL_CKPT_WR_ALL_REPLICAS))
    {
        /*
         * SYNC checkpoint case.
         */
        *pNodeAddr = clIocLocalAddressGet();
    }
    else
    { 
        /* 
         * ASYNC checkpoint case.
         */
        *pNodeAddr = pHdlInfo->activeAddr;
    }
#endif
    *pNodeAddr = pHdlInfo->activeAddr;
    
    if(*pNodeAddr == CL_CKPT_UNINIT_ADDR)
    {
        /* 
         * This case is possible if the checkpoint is collocated checkpoint
         * and there is no active replica associated with it.
         */
        rc = CL_CKPT_ERR_NOT_EXIST;
    }
    
    /*
     * Checkin the information assocaited with the checkpoint handle.
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    return rc;
}



/*
 * Function to get the master handle, given the checkpoint handle.
 */
 
ClRcT ckptMasterHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptHdlT *pCkptMastHdl)
{
    ClRcT       rc = CL_OK;
    CkptHdlDbT  *pHdlInfo = NULL;

    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = ckptHandleCheckout( ckptLocalHdl,
            CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;
        
    *pCkptMastHdl = pHdlInfo->ckptMasterHdl;

    /*
     * Checkin the information assocaited with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    return rc;
}



/*
 * Function to get the active handle, given the checkpoint handle.
 */
 
ClRcT ckptActiveHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptHdlT *pCkptActHdl)
{
    ClRcT       rc = CL_OK;
    CkptHdlDbT  *pHdlInfo = NULL;

    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = ckptHandleCheckout( ckptLocalHdl,
            CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;

    *pCkptActHdl = pHdlInfo->ckptActiveHdl;

    /*
     * Checkin the information assocaited with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptLocalHdl);
    return rc;
}



/*
 * Function to destroy the checkpoint handle. 
 */
 
ClRcT ckptLocalHandleDelete(ClCkptHdlT ckptLocalHdl)
{
  ClRcT rc = CL_OK;

  rc = clHandleDestroy(gClntInfo.ckptDbHdl,ckptLocalHdl);

  return rc;

}



/*
 * Function to get the service handle given the checkpoint handle.
 */
 
ClRcT ckptSvcHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptSvcHdlT *pSvcHdl)
{
    ClRcT       rc = CL_OK;
    CkptHdlDbT  *pHdlInfo = NULL;

    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = ckptHandleCheckout( ckptLocalHdl,
            CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;
        
    *pSvcHdl = pHdlInfo->ckptSvcHdl;
    
    /*
     * Checkin the information assocaited with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    return rc;
}



ClRcT ckptSvcHandleSet(ClCkptHdlT ckptLocalHdl,ClCkptSvcHdlT svcHdl)
{
    ClRcT       rc = CL_OK;
    CkptHdlDbT  *pHdlInfo = NULL;

    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = ckptHandleCheckout( ckptLocalHdl,CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    if(rc != CL_OK)
        return rc;
    pHdlInfo->ckptSvcHdl = svcHdl;
    /*
     * Checkout the information assocaited with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    return rc;
}



/* 
 * Function to check out the specified handle and validates it with 
 * the stored information. 
 */
 
ClRcT ckptHandleCheckout( ClHandleT ckptHdl,
                          ClInt32T  hdlType,
                          void     **ppData) 
{
    ClRcT  rc = CL_OK;
    
    /*
     * Checkout the information associated with the passed handle.
     */
    rc = clHandleCheckout(gClntInfo.ckptDbHdl,ckptHdl,ppData);
    if(rc != CL_OK)
    {
        return CL_CKPT_ERR_INVALID_HANDLE;
    }

    /*
     * Validate the stored information for being NON NULL and match the 
     * passed and stored handle type. Only one handle database is being used 
     * for generating iteration, service and checkpoint related handles.
     * If suppose application tries to delete a checkpoint and specifies 
     * wrong checkpoint handle and assume that handle has been created for
     * iteration related operations. So handle checkout will pass. To avoid 
     * such scenarios handle type check is required.
     */
     if((ppData == NULL)  || 
       ((*(CkptInitInfoT **)ppData)->hdlType != hdlType))
    {
        /*
         * Checkin the information associated with the passed handle and
         * return ERROR.
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        return CL_CKPT_ERR_INVALID_HANDLE;
    }
    return CL_OK;
}                          
