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
 * ModuleName  : hal
 * File        : clHalObject.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains definitions for HAL.                             
 *************************************************************************/

/** @pkg cl.hal */

/* INCLUDES */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clHalApi.h>
#include <clHalInternal.h>
#include <stdlib.h>
#include <string.h>
#include <clOsalApi.h>
#include <clCorUtilityApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/**@#-*/
extern ClHalConfT halConfig ;
/* 
Function for key comaprision , uses deviceAccess Priority as the key 
*/
static int halKeyCompare (ClCntKeyHandleT key1 , ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

static void halObjContainerDestroy(ClCntKeyHandleT userKey, 
                                  ClCntDataHandleT userData);

ClRcT halObjCreate(ClUint32T omId,
                    ClCorMOIdPtrT moId,
                    ClHalObjectHandleT *const phalObjHandle)
{
    HalObjectT * phalObject = NULL;
    ClRcT ret=CL_OK;
#ifdef CL_DEBUG
    char aFunction[]="halObjCreate";
#endif
    
    CL_FUNC_ENTER();

    if (NULL == phalObjHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error NULL Pointer  \n ",aFunction));
        CL_FUNC_EXIT();
        return (CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    phalObject =(HalObjectT*) clHeapAllocate((ClUint32T)sizeof(HalObjectT));
        
    if (NULL == phalObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error Malloc Failed\n ",aFunction));
        CL_FUNC_EXIT();
        return (CL_HAL_SET_RC( CL_ERR_NO_MEMORY)) ;
    }

    memset(phalObject,0,sizeof(HalObjectT));
    
    phalObject->omId=omId;

    if(NULL != moId)
    {
        phalObject->moId = (ClCorMOIdPtrT) clHeapAllocate ((ClUint32T)sizeof(ClCorMOIdT)); 

        if (NULL == phalObject->moId)
        {
            clHeapFree (phalObject);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n %s Error Malloc Failed\n ",aFunction));
            CL_FUNC_EXIT();
            return (CL_HAL_SET_RC( CL_ERR_NO_MEMORY)) ;
        }

        memset (phalObject->moId,0,sizeof(ClCorMOIdT));
        memcpy (phalObject->moId, moId, sizeof(ClCorMOIdT));
    }
    else 
    {
        /* Error Condition */
        /* This needs to be uncommented after fixing the call to clOmObjectCreate &
        clOmCorAndOmObjectsCreate by passing hMOId ,sizeof(ClCorMOIdT) as the last 2parameters.
        As of now  NULL is being passed */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n %s Error Invalid Parameter \n ",aFunction));
        CL_FUNC_EXIT();
        return (CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }

    ret = clCntLlistCreate(halKeyCompare,NULL,halObjContainerDestroy,CL_CNT_UNIQUE_KEY,
    &(phalObject->hTableRefDevObject));
    if (CL_OK !=ret)
    {
        clHeapFree (phalObject);
        phalObject =NULL;
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error returned by \
        clCntLlistCreate \n ", aFunction));
        CL_FUNC_EXIT(); 
        return ret;
    }

    (*phalObjHandle) =  (ClHalObjectHandleT)phalObject;

    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK  \n ",aFunction));
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT halObjDevObjectInstall(ClHalObjectHandleT hHalObjHandle,
                              ClUint32T deviceId,
                              ClUint32T deviceAccessPriority)

{
    HalObjectT * phalObject = NULL;
    ClRcT ret =CL_OK;
    ClHalDeviceObjectH *phDevObjectHandle =NULL;
    ClUint32T maxRspTime =0;
#ifdef CL_DEBUG
    char aFunction[]="halObjDevObjectInstall";
#endif

    CL_FUNC_ENTER();

    phalObject = (HalObjectT *) hHalObjHandle;
    
    if (NULL ==phalObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Invalid Hal Obj Handle ",aFunction));
        CL_FUNC_EXIT();
        return CL_HAL_SET_RC(CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s phalObject->omId=%d,deviceId=%d,  \
    deviceAccessPriority=%d", aFunction,phalObject->omId, deviceId,
    deviceAccessPriority));

    /* To Do  Print ClCorMOId phalObject->moId
    */

    #ifdef DEBUG 
    clCorMoIdShow(phalObject->moId);
    #endif


    phDevObjectHandle =(ClHalDeviceObjectH*)clHeapAllocate((ClUint32T)sizeof(ClHalDeviceObjectH));

    if (NULL == phDevObjectHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s No Memory ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NO_MEMORY));
    }

    ret = halDevObjHandleGet( deviceId, phDevObjectHandle);
    if (CL_OK != ret)
    {
        clHeapFree (phDevObjectHandle);
        phDevObjectHandle =NULL;
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error returned by halDevObjHandleGet",
        aFunction));
        CL_FUNC_EXIT();
        return ret ; 
    }

   ret = clCntNodeAdd(phalObject->hTableRefDevObject,
        (ClCntKeyHandleT)(ClWordT) deviceAccessPriority,
        (ClCntDataHandleT) phDevObjectHandle, NULL);

    if(CL_OK != ret)
    {
        clHeapFree(phDevObjectHandle);
        phDevObjectHandle =NULL;
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error returned by clCntNodeAdd",aFunction));
        CL_FUNC_EXIT(); 
        return ret ;
    }

    ret = halDevObjMaxRspTimeGet(* phDevObjectHandle,  &maxRspTime);

    if (CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error returned by halDevObjMaxRspTimeGet",
        aFunction));
        clHeapFree (phDevObjectHandle);
        phDevObjectHandle =NULL;
        CL_FUNC_EXIT(); 
        return ret ;
    }

    phalObject->maxRspTime += maxRspTime;
    phalObject->numDevObj++;
    
    ret =halDevObjRefCountIncr( *phDevObjectHandle);
    if (CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error returned by halDevObjRefCountIncr",
        aFunction));
        clHeapFree (phDevObjectHandle);
        phDevObjectHandle =NULL;
        CL_FUNC_EXIT(); 
        return ret ;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n %s CL_OK ", aFunction));
    CL_FUNC_EXIT(); 
    return CL_OK;
}

ClRcT halObjCreateDevObjectInstall(ClUint32T omId,
                                    ClCorMOIdPtrT moId,
                                    ClHalDevObjectInfoT * const pDevObjInfo, 
                                    ClUint32T nDevObj, 
                                    ClHalObjectHandleT * phalObjHandle)
{
    ClRcT ret = CL_OK;
    ClUint32T index =0;
    ClHalObjectHandleT hTempHandle = 0;
#ifdef CL_DEBUG
    char aFunction[]="halObjCreateDevObjectInstal";
#endif

    CL_FUNC_ENTER();
    
    if ( NULL ==pDevObjInfo ||NULL==phalObjHandle )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    
    if (nDevObj <= 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error Invalid Parameter ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }

    ret =halObjCreate (omId, moId, phalObjHandle);

    if ( CL_OK != CL_GET_ERROR_CODE(ret))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error returned by halObjCreate",
        aFunction));
        CL_FUNC_EXIT();
        return ret;
    }

    hTempHandle = (*phalObjHandle);

    for(index =0 ; index <nDevObj ; index ++)
    {
        ret=halObjDevObjectInstall(hTempHandle, pDevObjInfo[index].deviceId,
        pDevObjInfo[index].deviceAccessPriority);

        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error returned by \
            halObjDevObjectInstall ", aFunction));
            CL_FUNC_EXIT(); 
            return ret;
        }
    }

    CL_FUNC_EXIT(); 
    return CL_OK;
}

ClRcT clHalObjectOperate(ClHalObjectHandleT halObjHandle,
                       ClUint32T operation,
                       ClUint32T suboperation,
                       ClHalAccessOrderT accessOrder,
                       void * const pUserData,
                       ClUint32T usrdataLen,
                       ClUint32T flag, 
                       ClUint32T userflag2)
{
    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =0;
    ClCntDataHandleT userDataHandle=0;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret =CL_OK;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectOperate";
#endif

    CL_FUNC_ENTER();

    #if 0
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s,halObjHandle=%L,operation=%u,suboperation=%d,\
    accessOrder=%d, pUserData=%x,usrdataLen=%u,flag=%u,userflag2=%u \n", 
    aFunction,halObjHandle,operation,suboperation,accessOrder,
    pUserData,usrdataLen,flag, userflag2));
    #endif
    
    pHalObject = (HalObjectT *)halObjHandle;

    if (NULL ==pHalObject )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(operation >= halConfig.halDevNumOperations)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
        
    if (CL_HAL_ACCESS_ORDER_INCR==accessOrder)
    {
        ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject, & nodeHandle);
        if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s error returned by clCntFirstNodeGet",
            aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
        while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
        {
            ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject, nodeHandle,
            & userDataHandle);
            if(CL_OK !=ret) 
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error returned by \
                clCntNodeUserDataGet", aFunction));
                CL_FUNC_EXIT();
                return ret;
            }

            memcpy(&hDevObject,(ClUint32T *)userDataHandle,
            sizeof(hDevObject));

            ret =halDevObjOperationExecute(hDevObject, pHalObject->omId,
            pHalObject->moId, operation ,suboperation, pUserData,usrdataLen);

            if((CL_OK) !=ret)
            {
                if (!CL_IS_CONTINUE_ON_ERR(flag))
                {
                  CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                  halDevObjOperationExecute ",aFunction));
                  CL_FUNC_EXIT();
                  return ret;
                }
                else 
                {
                  CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" \n %s Error returned by \
                  halDevObjOperationExecute ret =%d ",aFunction,ret));
                }        
            }
            ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle ,
            &nodeHandle);
            if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                clCntNextNodeGet ",aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
        }/*End while*/
    }/*end if */
    else if (CL_HAL_ACCESS_ORDER_DECR==accessOrder)
    {
        ret=clCntLastNodeGet(pHalObject->hTableRefDevObject, & nodeHandle);
        if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s error returned by clCntLastNodeGet",
            aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
        while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
        {
            ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject, nodeHandle,
            & userDataHandle);
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s error returned by \
                clCntNodeUserDataGet ", aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
            memcpy(&hDevObject,(ClUint32T *)userDataHandle,
            sizeof(hDevObject)); 
            
            ret =halDevObjOperationExecute(hDevObject, pHalObject->omId,
            pHalObject->moId ,operation ,suboperation, pUserData,usrdataLen);

            if((CL_OK) !=ret)
            {
                if (!CL_IS_CONTINUE_ON_ERR(flag))
                {
                     CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                     halDevObjOperationExecute ",aFunction));
                     CL_FUNC_EXIT();
                     return ret;
                 }
                 else 
                 {
                     CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" \n %s Error returned by \
                     halDevObjOperationExecute ret =%d ",aFunction,ret));
                  }        
             }
            ret = clCntPreviousNodeGet(pHalObject->hTableRefDevObject, 
            nodeHandle , &nodeHandle);
            if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                clCntPreviousNodeGet ",aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
        }

    }/*end else if */
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Invalid Parameter accessOrder ",
        aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK  \n ",aFunction));
    CL_FUNC_EXIT(); 
    return (CL_OK);
}

ClRcT clHalObjectDOPriorityGet(ClHalObjectHandleT hHalObjHandle,
                            ClUint32T deviceId,
                            ClUint32T * pDevAccessPriority)
{
    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =(ClCntNodeHandleT)0;
    ClCntDataHandleT userDataHandle=(ClCntDataHandleT)0;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret = CL_OK;
    ClUint8T found =CL_FALSE;
    ClHalDeviceObjectH devObjHandle =0;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectDOPriorityGet";
#endif

    CL_FUNC_ENTER();

    pHalObject =(HalObjectT *) hHalObjHandle;

    if (NULL ==pHalObject )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    /*Get Device Handle */
    ret = halDevObjHandleGet( deviceId, & devObjHandle);
    
    if (CL_OK !=ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error returned by halDevObjHandleGet ",aFunction));
        CL_FUNC_EXIT();
        return((CL_HAL_SET_RC(CL_ERR_NOT_EXIST)));
    }

    ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject,& nodeHandle);
    if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntFirstNodeGet ",aFunction)); 
        CL_FUNC_EXIT();
        return ret;
    }
    /* Search for Device Handle in Hal Object, as of now assuming each device 
         has a different access priority    
    */
    while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
    {
        ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject,  nodeHandle ,
        & userDataHandle);
        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeUserDataGet ",
             aFunction));
            CL_FUNC_EXIT();
            return ret;
        }

        memcpy(&hDevObject,(ClUint32T *)userDataHandle,sizeof(hDevObject));
        if (hDevObject == devObjHandle)
        {
            
            ret=clCntNodeUserKeyGet(pHalObject->hTableRefDevObject, nodeHandle,
                (ClCntKeyHandleT *) pDevAccessPriority);
                    
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeUserKeyGet ",
                aFunction));
                CL_FUNC_EXIT(); 
                return ret ;
            }
            found =CL_TRUE;
            break ;
        }
        else 
        {
            ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle ,
            &nodeHandle);
            if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                clCntNextNodeGet ",aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
        }
    }/*end while loop*/
    
    if (found == CL_TRUE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK devPriority=%d  \n ",aFunction,
        *pDevAccessPriority));
        CL_FUNC_EXIT();
        return CL_OK;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error Device not Installed  ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_DEV_NOT_INSTALLED));
    }
}

ClRcT clHalObjectDOPriorityModify(ClHalObjectHandleT hHalObjHandle,
                               ClUint32T deviceId,
                               ClUint32T devAccessPriority)
{
    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =0;
    ClCntDataHandleT userDataHandle=0;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret = CL_OK;
    ClUint8T found =CL_FALSE;
    ClHalDeviceObjectH devObjHandle =0;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectDOPriorityModify";
#endif

    CL_FUNC_ENTER();

    pHalObject =(HalObjectT *) hHalObjHandle;

    if (NULL ==pHalObject )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    ret = halDevObjHandleGet( deviceId, & devObjHandle);
    if (CL_OK !=ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error returned by halDevObjHandleGet ",aFunction));
        CL_FUNC_EXIT();
        return( (CL_HAL_SET_RC(CL_ERR_NOT_EXIST)));
    }

    ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject,& nodeHandle);
    if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntFirstNodeGet ",aFunction));
        CL_FUNC_EXIT();
        return ret;
    }

    /* Search for Device Handle in Hal Object, as of now assuming each device 
         has a different access priority    
    */
    while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
    {
        ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject,  nodeHandle ,
        & userDataHandle);
        if (CL_OK !=ret)
        {
             CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeUserDataGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }

        memcpy(&hDevObject,(ClUint32T *)userDataHandle,sizeof(hDevObject));
        if (hDevObject == devObjHandle)
        {
            ret= clCntNodeDelete(pHalObject->hTableRefDevObject, nodeHandle);
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeDelete ",
                    aFunction));
                CL_FUNC_EXIT(); 
                return ret;
            }

            ret = clCntNodeAdd(pHalObject->hTableRefDevObject, (ClCntKeyHandleT)(ClWordT)devAccessPriority, 
            (ClCntDataHandleT)userDataHandle, NULL);
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeAdd ",
                    aFunction));
                CL_FUNC_EXIT();
                return ret ;
            }
            found =CL_TRUE;
            break ;
        }
        else 
        {
            ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle , 
            & nodeHandle);
            if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
                clCntNextNodeGet ",aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
        }
    }/*end while loop*/
    
    if (found == CL_TRUE)
    {
       CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK  \n ",aFunction));
       CL_FUNC_EXIT();
       return CL_OK;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error Device not Installed  ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_DEV_NOT_INSTALLED));
    }
}

ClRcT clHalObjectMaxRespTimeGet(ClHalObjectHandleT halObjHandle, 
                            ClUint32T * const phalDevMaxRespTime)
{
    HalObjectT * pHalObject =NULL;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectMaxRespTimeGet";
#endif

    pHalObject = (HalObjectT * )halObjHandle;
    CL_FUNC_ENTER();

    if (NULL ==pHalObject || NULL == phalDevMaxRespTime)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    *phalDevMaxRespTime = pHalObject->maxRspTime;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK halDevMaxRespTime=%d  \n ",aFunction,
             *phalDevMaxRespTime));
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clHalObjectCapLenGet(ClHalObjectHandleT halObjHandle, 
                          ClUint32T * const
                          pHalObjCapLen)
{
    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =0;
    ClCntDataHandleT userDataHandle=(ClCntDataHandleT)0;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret = CL_OK;
    ClUint32T devCapLen=0;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectCapLenGet";
#endif

    CL_FUNC_ENTER();

    pHalObject =(HalObjectT *) halObjHandle;

    if ((NULL ==pHalObject)||(NULL ==pHalObjCapLen))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    *pHalObjCapLen=0;

    ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject, & nodeHandle);
    if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntFirstNodeGet ",aFunction));
        CL_FUNC_EXIT();
        return ret;
    }
    while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
    {
        ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject, nodeHandle ,
        & userDataHandle);
        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeUserDataGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
        memcpy(&hDevObject,(ClUint32T *)userDataHandle,sizeof(hDevObject));
        
        ret =halDevObjCapLenGet (hDevObject, &devCapLen);
        if(CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by halDevObjCapLenGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
        (*pHalObjCapLen)=(*pHalObjCapLen)+devCapLen;
        devCapLen=0;

        ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle ,
        &nodeHandle);
        if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by \
            clCntNextNodeGet ",aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK HalObjCapLen=%d  \n ",aFunction,
            *pHalObjCapLen));
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clHalObjectCapabilityGet(ClHalObjectHandleT halObjHandle,
                           ClUint32T halObjCapLen,
                           void * phalObjCap)
{

    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =0;
    ClCntDataHandleT userDataHandle=(ClCntDataHandleT) 0 ;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret = CL_OK;
    ClUint32T devCapLen=0;
    ClUint8T *pTemp=NULL;
    /*This varaible is to ensure that halObjCapLen contains the correct
      value */
    ClInt32T halObjCapLenLeft=halObjCapLen; 
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectCapabilityGet";
#endif

    CL_FUNC_ENTER();

    pHalObject =(HalObjectT *) halObjHandle;

    if (NULL ==pHalObject||NULL ==phalObjCap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(0==halObjCapLen)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
    memset(phalObjCap,0,halObjCapLen);

    pTemp=(ClUint8T *)phalObjCap;

    ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject, & nodeHandle);
    if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntFirstNodeGet ",aFunction));
        CL_FUNC_EXIT();
        return ret;
    }
    while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
    {
        ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject, nodeHandle ,
        & userDataHandle);
        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNodeUserDataGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }

        /*hDevObject =(ClHalDeviceObjectH) (*userDataHandle);*/
        memcpy(&hDevObject,(ClUint32T *)userDataHandle,sizeof(hDevObject));
        
        ret =halDevObjCapLenGet (hDevObject, &devCapLen);
        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by halDevObjCapLenGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }


        ret=halDevObjCapGet(hDevObject, devCapLen, pTemp);
        if(CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by halDevObjCapGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
        
        halObjCapLenLeft = halObjCapLenLeft - devCapLen ;
        /* To ensure that proper halObjCap is passed to the API */
        if(halObjCapLenLeft < 0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n %s Error Invalid Parameter ",aFunction));
            CL_FUNC_EXIT(); 
            return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
        }

        pTemp=pTemp + devCapLen;
        devCapLen=0;
        
        ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle ,
        &nodeHandle);
        if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error returned by clCntNextNodeGet ",
                    aFunction));
            CL_FUNC_EXIT();
            return ret;
        }
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK  \n ",aFunction));
    CL_FUNC_EXIT(); 
    return (CL_OK);
}

ClRcT halObjDevObjectUnInstall(ClHalObjectHandleT halObjHandle, 
                                ClUint32T deviceId)
{
    HalObjectT * pHalObject =NULL;
    ClCntNodeHandleT nodeHandle =(ClCntNodeHandleT) NULL;
    ClCntDataHandleT userDataHandle=(ClCntDataHandleT) NULL;
    ClHalDeviceObjectH hDevObject=0;
    ClRcT ret = CL_OK;
    ClUint8T found =CL_FALSE;
    ClHalDeviceObjectH devObjHandle =0;
#ifdef CL_DEBUG
    char aFunction[]="halObjDevObjectUnInstall";
#endif

    CL_FUNC_ENTER();

    pHalObject =(HalObjectT *)halObjHandle;

    if (NULL ==pHalObject )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    /*Get Device Handle */
    ret = halDevObjHandleGet( deviceId, & devObjHandle);
    
    if (CL_OK !=ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by halDevObjHandleGet ",aFunction));
        CL_FUNC_EXIT();
        return( (CL_HAL_SET_RC(CL_ERR_NOT_EXIST)));
    }

    ret=clCntFirstNodeGet(pHalObject->hTableRefDevObject,& nodeHandle);
    if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
    {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntFirstNodeGet ",aFunction));
            CL_FUNC_EXIT();
            return ret;
    }
    /* Search for Device Handle in Hal Object, as of now assuming each device 
       has a different access priority    
    */
    while(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret))
    {
        ret =clCntNodeUserDataGet(pHalObject->hTableRefDevObject, nodeHandle , 
        & userDataHandle);
        if (CL_OK !=ret)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntNodeUserDataGet ",
                aFunction));
            CL_FUNC_EXIT();
            return ret;
        }

        /*hDevObject =(ClHalDeviceObjectH)(*userDataHandle);*/
        memcpy(&hDevObject,(ClUint32T *)userDataHandle,sizeof(hDevObject));
        if (hDevObject == devObjHandle)
        {
            ret =halDevObjRefCountDecr(hDevObject);
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by halDevObjRefCountDecr ",
                aFunction));
                CL_FUNC_EXIT();
                return ret ;
            }
            clHeapFree(((ClUint32T *)userDataHandle));
            userDataHandle=0;
            
            ret= clCntNodeDelete(pHalObject->hTableRefDevObject, nodeHandle);
            if (CL_OK !=ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntNodeDelete ",
                aFunction));
                CL_FUNC_EXIT();
                return ret ;
            }
            (pHalObject->numDevObj)-- ; 
            found =CL_TRUE;
            break ;
        }
        else 
        {
            ret =clCntNextNodeGet(pHalObject->hTableRefDevObject, nodeHandle ,
            &nodeHandle);
            if((CL_OK !=ret)&&(CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(ret)))
            {
                 CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s error returned by clCntNextNodeGet ",
                aFunction));
                CL_FUNC_EXIT();
                return ret;
            }
        }
    }/*end while loop*/
    
    if (found == CL_TRUE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK   \n ",aFunction ));
        CL_FUNC_EXIT();
        return CL_OK;;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,(" \n %s Error Device not Installed  ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_DEV_NOT_INSTALLED));
    }
}

ClRcT clHalObjectNumDOGet(ClHalObjectHandleT hHalObjHandle,
                             ClUint32T * pNumDevObject)
{
    HalObjectT * pHalObject =NULL;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectNumDOGet";
#endif

    CL_FUNC_ENTER();
    pHalObject = (HalObjectT * )hHalObjHandle;

    if(NULL == pHalObject|| NULL ==pNumDevObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    
    *pNumDevObject= pHalObject->numDevObj;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK NumDevObject=%d \n",aFunction,\
    *pNumDevObject));
    CL_FUNC_EXIT(); 
    return CL_OK;;
}
#ifdef COR_WILD_CARD_MATCHING_READY
extern ClInt32T moIdWildCardCompare(ClCorMOIdPtrT hMOId1,ClCorMOIdPtrT hMOId2)
/*{
    return 0; if Match was sucess

}
*/
#endif
/** 
*API to create HAL Objects from  Hal Configuration Table
*/
ClRcT clHalObjectCreate(ClUint32T omHandle, 
                                  ClCorMOIdPtrT hMOId,
                                  ClHalObjectHandleT * phHalObj)
{
    ClUint32T nIndex=0;
    ClUint16T omClassId=0 ;
#ifdef COR_WILD_CARD_MATCHING_READY
    ClCorMOIdPtrT hMOIdFromTable =NULL;
#endif
    ClUint8T bFound = CL_FALSE ;
    ClRcT ret =CL_OK;
    omClassId =CL_OM_HANDLE_TO_CLASSID(omHandle); 
    CL_FUNC_ENTER();
    CL_ASSERT(halConfig.pHalObjConf);
    if(NULL==halConfig.pHalObjConf)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("halConfig.pHalObjConf is NULL\n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

#ifdef COR_WILD_CARD_MATCHING_READY
    ret=clCorMoIdAlloc(&hMOIdFromTable);
    if(CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("clCorMoIdAlloc Failed \n"));
        CL_FUNC_EXIT();
        return ret; 
    }
#endif
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("halConfig.halNumHalObject=%d\n",
    halConfig.halNumHalObject));

    for(nIndex=0;nIndex<halConfig.halNumHalObject;nIndex++)
    {
        if(omClassId== halConfig.pHalObjConf[nIndex].omClassId)
        {
#ifdef COR_WILD_CARD_MATCHING_READY
            /* Translate string to Numeric MOID */
            /* This API sets svcId to CL_COR_INVALID_SVC_ID */ 
            ret=corXlateMOPath(halConfig.pHalObjConf[nIndex].strMOId,hMOIdFromTable);
            if(CL_OK != ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCorMoIdAlloc Failed \n"));
                CL_FUNC_EXIT();
                return ret;
            }
            /* Changing the SvcId to CL_COR_INVALID_SVC_ID as the MOID read 
               from table will alos have the same service ID , and we need
               to comapre the two MOIDs
            */
            ret =clCorMoIdServiceSet(hMOId,CL_COR_INVALID_SVC_ID); 
            if(CL_OK != ret)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCorMoIdServiceSet Failed \n"));
                CL_FUNC_EXIT();
                return ret;
            }
            if(0==moIdWildCardCompare(hMOIdFromTable,hMOId))
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("MOID Matched"));
                bFound = CL_TRUE ;
                break;
            }
#else 
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("OMID Matched"));
            bFound = CL_TRUE ;
            break;
#endif         
        }
    }
    if(CL_TRUE == bFound)
    {
        ret = halObjCreateDevObjectInstall(omHandle,
                               hMOId,
                               halConfig.pHalObjConf[nIndex].pDevObjInfo,
                               halConfig.pHalObjConf[nIndex].devObjects,
                               phHalObj);
        if (ret != CL_OK)
         { 
             CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("\n 0x%x Error returned by  \
             halObjCreateDevObjectInstall",ret));
             CL_FUNC_EXIT ();
             return ret;
         }
         CL_DEBUG_PRINT (CL_DEBUG_TRACE,(" CL_OK\n"));
         CL_FUNC_EXIT();
         return CL_OK ;
    }
    else 
    {
        /* Entry not found in the HAL Config Table , return ERROR */
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,("Entry for the specified ClCorMOId and OM Handle not \
                              found in the Hal Config Table\n"));
        CL_FUNC_EXIT ();
        return(CL_HAL_SET_RC(CL_ERR_NOT_EXIST));
    }
}

#ifdef ChANGE_HAL_IMPLEMENTATION_FOR_NEW_COR_IM
static ClRcT halObjDevObjPrint(ClCntKeyHandleT userKey, 
                        ClCntDataHandleT userData,
                        void* userArg,
                        ClUint32T dataLength)
{
   ClHalDeviceObjectH hDevObjHandle= 0;
   ClUint32T deviceId;
   ClRcT rc =CL_OK ;

   memcpy(&hDevObjHandle,((ClUint32T *)userData),sizeof(ClHalDeviceObjectH));

   rc=halDevObjIdGet(hDevObjHandle,&deviceId);
   if (CL_OK != rc)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("halDevObjIdGet Failed \n"));
        CL_FUNC_EXIT();
        return rc; 
    }

   clOsalPrintf(" Device Handle =%d , Device Id = %d , Access Priority = %d \n",
               hDevObjHandle,deviceId,userKey);

   return rc ;
}
                        
static ClRcT halObjPrint(ClHalObjectHandleT halObjHandle)
{
    ClRcT rc =CL_OK ;
    HalObjectT * pHalObject = (HalObjectT *) halObjHandle;

    CL_FUNC_ENTER();
    clOsalPrintf("\n halObjHandle=0x%x , omClassId=%d , maxRspTime =%d , numDevObj=%d \n",
    halObjHandle,CL_OM_HANDLE_TO_CLASSID(pHalObject->omId),
    pHalObject->maxRspTime,pHalObject->numDevObj);
    
    printf(" ");
    clCorMoIdShow(pHalObject->moId);

    rc=clCntWalk(pHalObject->hTableRefDevObject,
               halObjDevObjPrint,NULL,0);

    printf("\n");

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clCntWalk Failed \n"));
        CL_FUNC_EXIT();
        return rc; 
    }
    
    CL_FUNC_EXIT();
    return rc; 
}
#endif
ClRcT showHalObjects(void * pData)
{
    #ifdef ChANGE_HAL_IMPLEMENTATION_FOR_NEW_COR_IM
    /* This needs to be changed in alignement with new cor IM  for R2.0*/
    ClRcT rc = CL_OK ;
    ClCorObjectHandleT   hCORObj;
    ClHandleT omHandle;
    void *pObjRef=NULL;

    provClass *pProvObj=NULL;
    cmClass *pCMObj=NULL;
    alarmOMClass * pAlarmObject = NULL;

    ClCorMOIdT moId; 
    ClCorServiceIdT srvcId;
    CL_FUNC_ENTER();
    
    hCORObj = *((ClCorObjectHandleT*)pData);
    
    rc=clCorMoIdInitialize(&moId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clCorMoIdInitialize Failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    rc=clCorObjectHandleToMoIdGet(hCORObj, &moId, &srvcId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clCorObjectHandleToMoIdGet Failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    /*
        Because of Cor Sync Up , bd Alarm and Prov objects get shipped to the
        SD , and the fact that even CM MSO gets shippped to the BD . There is a
        check , with the implicit knowledge that the CM Service runs on the SD
        and there is no prov Agent and Alarm Agent running on the SD . The
        following checks are put in place .
    */
    if(clCpmIsMaster())
    {
       /*
            CM Service on SD 
       */
       if(CL_COR_SVC_ID_CHASSIS_MANAGEMENT != srvcId )
       {
            return -1;
       }
     }
     else/* Alarm , Prov Agent on BD */ 
     {
        if((CL_COR_SVC_ID_PROVISIONING_MANAGEMENT != srvcId) &&
           (CL_COR_SVC_ID_ALARM_MANAGEMENT !=srvcId))
       {
            return -1;
       }
     }  


    rc=clOmMoIdToOmHandleGet(&moId,&omHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clOmMoIdToOmHandleGet Failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    rc=clOmObjectReferenceByOmHandleGet(omHandle, &pObjRef);
    {
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("omCOROMObjHandleGet Failed \n"));
            CL_FUNC_EXIT();
            return rc;
        }
        if(NULL == pObjRef)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("omCOROMObjHandleGet returned NULL Pointer\n"));
           CL_FUNC_EXIT();
           return rc;
        }
    }

    if(CL_COR_SVC_ID_PROVISIONING_MANAGEMENT==srvcId)
    {
        /*Prov MSO Object */
        pProvObj=(provClass*)pObjRef;
        if(0 != pProvObj->hProvHalObj)
        {
           halObjPrint(pProvObj->hProvHalObj); 

        }
    }
    else if (CL_COR_SVC_ID_CHASSIS_MANAGEMENT==srvcId)
    {
        pCMObj=(cmClass*)pObjRef;
        if(0 != pCMObj->_hHAL)
        {
           halObjPrint(pCMObj->_hHAL); 

        }
    }
    else if(CL_COR_SVC_ID_ALARM_MANAGEMENT==srvcId)
    {
        pAlarmObject  =(alarmOMClass *)pObjRef;
        if(0 != pAlarmObject->halH)
        {
           halObjPrint(pAlarmObject->halH); 
        }
    }
    else
    {

    }
    CL_FUNC_EXIT();
    #endif 
    return CL_OK;
}
ClRcT cliShowHalObjects(int argc, char **argv)
{

    ClRcT rc = CL_OK ;
    clOsalPrintf("Needs to be implemented for new IM for COR");
    #ifdef ChANGE_HAL_IMPLEMENTATION_FOR_NEW_COR_IM
    /* This needs to be changed in alignement with new cor IM  for R2.0*/
    ClCorMOIdPtrT hMOId = NULL ;
    CL_FUNC_ENTER();
    rc=clCorMoIdAlloc(&hMOId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,(" clCorMoIdAlloc Failed \n"));
        CL_FUNC_EXIT();
        return rc; 
    }

    rc=clCorMoIdInitialize(hMOId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clCorMoIdInitialize Failed \n"));
        CL_FUNC_EXIT();
        clCorMoIdFree(hMOId);
        return rc; 
    }
    
   rc=clCorObjectWalk(hMOId, NULL, showHalObjects ,CL_COR_MSO_WALK);
   if (CL_OK != rc)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR ,("clCorMoIdServiceSet Failed \n"));
        CL_FUNC_EXIT();
        clCorMoIdFree(hMOId);
        return rc; 
   }
   
   clCorMoIdFree(hMOId);
   CL_FUNC_EXIT();
   #endif 
   return rc ;
}

ClRcT clHalObjectDelete (ClHalObjectHandleT halObjHandle)
{

    ClRcT rc=CL_OK;
#ifdef CL_DEBUG
    char aFunction[]="clHalObjectDelete";
#endif
    HalObjectT * phalObject = NULL;
    CL_FUNC_ENTER();

    phalObject =(HalObjectT *)halObjHandle;
    if (NULL == phalObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error NULL Pointer  \n ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER)) ;
    }

    rc=clCntDelete(phalObject->hTableRefDevObject);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n %s clCntAllNodesDelete returned Error =%x\n ",
        aFunction,rc));
        CL_FUNC_EXIT();
        return rc;
    }
    clHeapFree(phalObject);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("%s CL_OK ",aFunction));
    CL_FUNC_EXIT();
    return rc;
}

static void halObjContainerDestroy(ClCntKeyHandleT userKey, 
                                  ClCntDataHandleT userData)
{
    ClHalDeviceObjectH hDevObject=0;
    ClRcT rc =CL_OK ;
    CL_FUNC_ENTER();

    memcpy(&hDevObject,(char *)userData,sizeof(ClHalDeviceObjectH));

    if(hDevObject >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error  Invalid Handle =%d ",hDevObject));
    }
    rc =halDevObjRefCountDecr(hDevObject);
    if (CL_OK !=rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error returned by halDevObjRefCountDecr =%u",rc));
        CL_FUNC_EXIT();
    }
    clHeapFree((void *)userData);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("halObjContainerDestroy Success for %p", (void *)userKey));
    CL_FUNC_EXIT();
}
