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
 * File        : clHalDevObject.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains definitions for HAL.                             
 *************************************************************************/

/** @pkg cl.hal */

/* INCLUDES */
#include <clCommon.h>
#include <clHalApi.h>
#include <stdlib.h>
#include <string.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clCommonErrors.h>
#include <clHalInternal.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/**@#-*/

static HalDeviceObjTableT halDevObjTable;
static ClUint8T halInitDone= CL_FALSE ;
extern ClHalConfT halConfig ;
extern void halConfigure () ;

ClRcT clHalLibInitialize()
{
    ClRcT rc= CL_OK ; 
    CL_FUNC_ENTER();
    if (CL_TRUE == halInitDone)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n clHalLibInitialize Called Again \n"));
        CL_FUNC_EXIT();
        return (CL_HAL_SET_RC(CL_ERR_INVALID_STATE));
    }
#ifdef DEBUG
    rc= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
        return rc;
    }
#endif
    
    memset(&halDevObjTable,0, sizeof(HalDeviceObjTableT));

    halDevObjTable.pphalDeviceObj=(HalDeviceObjectT **)clHeapAllocate((halConfig.
        halNumDevObject)*sizeof(HalDeviceObjectT *));

    if (NULL == halDevObjTable.pphalDeviceObj)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n clHalLibInitialize Error no memory HAL\n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NO_MEMORY));
    }

    memset(halDevObjTable.pphalDeviceObj,0, ((halConfig.
        halNumDevObject)*sizeof(HalDeviceObjectT *)));
        
    halInitDone = CL_TRUE; 

    /* Create device object(s) from the Configuration Info */
    rc = halDevObjTableCreate ();
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_CRITICAL, ("\n halDevObjTableCreate  Failed"));
        CL_FUNC_EXIT();
        return rc ;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nclHalLibInitialize CL_OK\n"));
    CL_FUNC_EXIT();
    return (CL_OK) ;
}

ClRcT halDevObjCreate (ClUint32T deviceID, 
                        void *pdevCapability,
                        ClUint32T devCapLen ,
                        ClUint32T maxRspTime, 
                        ClUint32T bootUpPriority, 
                        ClHalDeviceObjectH *const phalDevObj)

{
    ClUint32T index=0 ;
    ClUint32T i=0 ;
    CL_FUNC_ENTER();
    
    if(deviceID<=0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\nhalDevObjCreate Error Invalid DeviceId\n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_PARAMETER)) ;
    }

    if(NULL ==phalDevObj) 
    { 
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n halDevObjCreate Error NULL for Device \
        Object Handle\n")) ;
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    if(devCapLen == 0 || NULL == pdevCapability)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\nhalDevObjCreate Error Dev Capability\n")) ;
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_NULL_POINTER));
    }

    if ((NULL == halDevObjTable.pphalDeviceObj))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n halDevObjCreate Error NULL PTR\n")) ;
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    /* 
     Device Objects are creted by boot manager and persist for the life time
     of the board , I don't see a sceanrio where device objects will be 
     deleted at the run time thought halDevObjDelete API is provided. In case 
     of deletion of device objects , there might be holes created in the 
     halDevObjTable, in which this API will need to search for any vaccant 
     holes . This scenario is not handled as of now but can he handled later
     if required , as of now we just return error .
    */

    /* Check for out of bound checking for nextIndex */
    if (halDevObjTable.nextIndex>= halConfig.halNumDevObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n Error All device Objects Created, cannot \
        create any more, modify the halConf.c file to create more device \
        Objects \n")) ;
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_OUT_OF_RANGE));
    }

    for (index =0; index < halConfig.halNumDevObject;index ++)
    {
      if ( NULL != halDevObjTable.pphalDeviceObj[index])
      {
        if((deviceID== halDevObjTable.pphalDeviceObj[index]->deviceId)||
           (bootUpPriority== 
            halDevObjTable.pphalDeviceObj[index]->bootUpPriority))
          {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\nhalDevObjCreate Error deviceID or \
            bootUpPriority already exist \n")) ;
            CL_FUNC_EXIT();
            return(CL_HAL_SET_RC( CL_ERR_DUPLICATE));
          }
      }
    }

    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]=
                   (HalDeviceObjectT *)clHeapAllocate((ClUint32T)sizeof(HalDeviceObjectT));

    if (NULL ==halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n halDevObjCreate Error no memory \n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NO_MEMORY));
    }

    memset(halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex],0,
        sizeof(HalDeviceObjectT));

    if (devCapLen >0)
    {
        halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->pdevCapability
        = (void *)clHeapAllocate(devCapLen);

        if (NULL== halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->
        pdevCapability)
        {
         CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n halDevObjCreate Error no memory \n"));
         CL_FUNC_EXIT();
         return(CL_HAL_SET_RC( CL_ERR_NO_MEMORY));
        }
        memset(halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->
        pdevCapability,0,devCapLen);
    }
    else 
    {
        halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->pdevCapability
        =0;
    
        halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->pdevCapability
        =NULL;
    }

    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->pdevOperations=
    (ClfpDevOperationT *)clHeapAllocate(sizeof(ClfpDevOperationT)*halConfig.
     halDevNumOperations);

    if (NULL == halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->
        pdevOperations)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n halDevObjCreate Error no memory \n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_NO_MEMORY));
    }

    for(i=0;i< halConfig.halDevNumOperations;i++)
    {
        halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->
                                        pdevOperations[i]=NULL;
    }

    memcpy(halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->
    pdevCapability , pdevCapability,devCapLen);

    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->devCapLen= 
    devCapLen ;

    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->maxRspTime = 
    maxRspTime;


    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->deviceId=deviceID;

    halDevObjTable.pphalDeviceObj[halDevObjTable.nextIndex]->bootUpPriority=
    bootUpPriority;

    *phalDevObj =halDevObjTable.nextIndex ;

    halDevObjTable.nextIndex ++ ; 

    CL_FUNC_EXIT();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n halDevObjCreate CL_OK\n"));
    return (CL_OK) ;
    
}

ClRcT halDevObjHandlerRegister(ClHalDeviceObjectH hDevObj,
                                ClUint32T operation, 
                                ClfpDevOperationT fpOperation)
{
#ifdef CL_DEBUG
    char aFunction[] ="halDevObjHandlerRegister";
#endif
    CL_FUNC_ENTER();

    if(hDevObj >= halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if ((NULL == halDevObjTable.pphalDeviceObj)||
        (NULL == halDevObjTable.pphalDeviceObj[hDevObj])||
        (NULL == halDevObjTable.pphalDeviceObj[hDevObj]->pdevOperations))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    
    if (operation >= halConfig.halDevNumOperations)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error operation=%d is invalid \n",\
        aFunction,operation)) ;
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
    /*
    User can pass NULL if he does not want to  install any operation 
    that is why no check on it 
    */
    
    halDevObjTable.pphalDeviceObj[hDevObj]->pdevOperations[operation]
    =fpOperation;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK\n",aFunction));
    CL_FUNC_EXIT();
    return (CL_OK) ;
}

ClRcT halDevObjCreateHandlersReg(ClHalDevObjectT * phalDevObject,
                                  ClHalDeviceObjectH * const phalDevObjHandle)
{
    ClRcT ret= CL_OK ;
    ClUint32T index=0;
#ifdef CL_DEBUG
    char aFunction[]="halDevObjCreateHandlersReg";
#endif

    CL_FUNC_ENTER();
    
    if ((NULL == phalDevObject) ||(NULL==phalDevObjHandle))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Invalid Parameters",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_NULL_POINTER));
    }

    ret =halDevObjCreate(phalDevObject->deviceId,
                         phalDevObject->pDevCapability,
                         phalDevObject->devCapLen,
                         phalDevObject->maxRspTime,
                         phalDevObject->bootUpPriority,
                         phalDevObjHandle);

    if ((CL_OK)!=ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error returned by halDevObjCreate ", \
        aFunction));
        CL_FUNC_EXIT();
        return(ret) ;
    }

    if ((NULL==halDevObjTable.pphalDeviceObj)||
        (NULL == halDevObjTable.pphalDeviceObj[*phalDevObjHandle])||
        (NULL ==halDevObjTable.pphalDeviceObj[*phalDevObjHandle]->
         pdevOperations))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
     }
    /*Install handlers ,if NULL is passed than also do the copy operation*/
    for (index =0 ; index< halConfig.halDevNumOperations;index ++)
    {
        halDevObjTable.pphalDeviceObj[*phalDevObjHandle]->pdevOperations[index]
        = phalDevObject->pDevOperations[index];
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK\n",aFunction));
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT halDevObjHandleGet(ClUint32T deviceID,
                          ClHalDeviceObjectH * phalDevObjHandle)
{
    ClUint32T index=0 ;
    ClUint8T found = CL_FALSE ;
#ifdef CL_DEBUG
    char aFunction[]="halDevObjHandleGet";
#endif
    
    CL_FUNC_ENTER();
    
    if ((deviceID <=0))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Invalid Device Id ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_PARAMETER)) ;
    }

    if (NULL == phalDevObjHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL Pointer passed as o/p \
        parameter ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_PARAMETER)) ;
    }

    if ((NULL==halDevObjTable.pphalDeviceObj))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }


    for(index =0; index < halConfig.halNumDevObject; index ++)
    {
        if(NULL!= halDevObjTable.pphalDeviceObj[index])
        if(halDevObjTable.pphalDeviceObj[index]->deviceId==deviceID)
        {
            *phalDevObjHandle = index ;
            found = CL_TRUE;
            break ;
        }
    }

    if (found == CL_TRUE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK *phalDevObjHandle =%d\n",
        aFunction, *phalDevObjHandle));
        CL_FUNC_EXIT();
        return CL_OK;
    }
    else
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Device Id Not Found ",
        aFunction));
        return(CL_HAL_SET_RC(CL_ERR_NOT_EXIST)) ;
    }
}

ClRcT clHalDOStateGet(ClHalDeviceObjectH hDevObjHandle, ClUint32T * pState)
{
#ifdef CL_DEBUG
    char aFunction[]="clHalDOStateGet";
#endif

    CL_FUNC_ENTER();

    if (NULL==pState)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s NULL o/p Parameter",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
    if(hDevObjHandle == 0 || hDevObjHandle >= halConfig.halNumDevObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Invalid Device Handle",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }
    if ((NULL==halDevObjTable.pphalDeviceObj)||
        (NULL == halDevObjTable.pphalDeviceObj[hDevObjHandle]))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
     }

    *pState= halDevObjTable.pphalDeviceObj[hDevObjHandle]->state;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK *pState =%d\n",aFunction,*pState));
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clHalDOStateSet(ClHalDeviceObjectH hDevObjHandle, ClUint32T state)
{
#ifdef CL_DEBUG
    char aFunction[]="clHalDOStateSet";
#endif
    CL_FUNC_ENTER();
    if(hDevObjHandle == 0 || hDevObjHandle >= halConfig.halNumDevObject)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error Invalid Device Handle",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if ((NULL==halDevObjTable.pphalDeviceObj)||
        (NULL == halDevObjTable.pphalDeviceObj[hDevObjHandle]))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
     }

    halDevObjTable.pphalDeviceObj[hDevObjHandle]->state= state;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK \n",aFunction));
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT halDevObjDelete(ClHalDeviceObjectH hDevObjHandle)
{
#ifdef CL_DEBUG
    char aFunction[]="halDevObjDelete";
#endif
    CL_FUNC_ENTER();
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n%s Error Invalid Device Handle",aFunction));
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if ((NULL==halDevObjTable.pphalDeviceObj)||
        (NULL == halDevObjTable.pphalDeviceObj[hDevObjHandle]))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    if (0 == halDevObjTable.pphalDeviceObj[hDevObjHandle]->refCount)
    {
        clHeapFree (halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevCapability);
        halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevCapability= NULL;

        /* we cant free this memory.*/
        halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevOperations = NULL;
        halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevOperations=NULL;

        clHeapFree(halDevObjTable.pphalDeviceObj[hDevObjHandle]);
        halDevObjTable.pphalDeviceObj[hDevObjHandle]=NULL ;
        
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK \n",aFunction));
        CL_FUNC_EXIT();
        return (CL_OK) ;
    }
    else 
    {
        /* STill some of the MSP have reference to this Device Object 
        and hence this cannot be deleted */
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error refCount!=0 \n",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_INVALID_STATE));
    }

}

void bubbleSort(HalDevBootT bootTable[], int Size)
{
    int i,j;
    HalDevBootT temp;

    if(Size <=0)
    return ;

    memset(&temp,0,sizeof(HalDevBootT));

    for (i=(Size-1);i>=0 ;i--)
    {
        for(j=1;j<=i;j++)
        {
             if(bootTable[j-1].devBootUpPriority > 
             bootTable[j].devBootUpPriority)
             {
                 memcpy(&temp,&bootTable[j-1],sizeof(HalDevBootT));
                 memcpy(&bootTable[j-1],&bootTable[j],sizeof(HalDevBootT));
                 memcpy(&bootTable[j],&temp,sizeof(HalDevBootT));
             }
         }
    }
}

ClRcT clHalDevObjectLayerInit(void * pUserData, 
                             ClUint32T usrDataLen,
                             ClUint32T flags)
{
    ClUint32T index=0;
    ClUint32T nDevices=0;
    ClUint32T nActualDevices=0; /*actual No of Devices in the Dev Table
                                   accounting for holes */
    ClRcT ret =CL_OK;
    ClUint8T errFound = CL_FALSE;
    HalDevBootT * pDevBootTable=NULL;
#ifdef CL_DEBUG
    char aFunction[]="clHalDevObjectLayerInit";
#endif

    CL_FUNC_ENTER();

    if((usrDataLen == 0) || (NULL==pUserData))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s userDataLen is >0 but pUserData=NULL\n",
        aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }
    
    /* Num of Devices Installed  by Boot Mgr*/
    nDevices=halDevObjTable.nextIndex;

    /* Table to sort Devices in the order of Incr Boot Up Priority*/
    pDevBootTable =(HalDevBootT *)clHeapAllocate(nDevices*sizeof(HalDevBootT));

    if(NULL==pDevBootTable)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error No Memory \n",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NO_MEMORY));
    }

    if ((NULL==halDevObjTable.pphalDeviceObj))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    memset(pDevBootTable,0,(nDevices*sizeof(HalDevBootT)));

    for (index =0 ; index <nDevices; index ++)
    {
      /*Copy the Entry only if it is there */
      if(NULL!= halDevObjTable.pphalDeviceObj[index])
      {
          pDevBootTable[index].devHandle = index;
          pDevBootTable[index].devBootUpPriority = 
                  halDevObjTable.pphalDeviceObj[index]->bootUpPriority;
          nActualDevices ++ ;
      }
    }
    

    /* Sort the DevBootTable in increasing order of boot Up Priority*/ 
    bubbleSort(pDevBootTable, (ClInt32T)nActualDevices);

    for (index =0 ; index <nActualDevices; index ++)
    {
        /* If Device Object Exist and if init handler is installed */
      if((NULL != halDevObjTable.pphalDeviceObj[(pDevBootTable[index].
          devHandle)])&&
         (NULL != halDevObjTable.pphalDeviceObj[(pDevBootTable[index].
          devHandle)]->pdevOperations)&&
         (NULL != halDevObjTable.pphalDeviceObj[(pDevBootTable[index].
          devHandle)]->pdevOperations[CL_HAL_DEV_INIT]))
      {
        ret =halDevObjTable.pphalDeviceObj[(pDevBootTable[index].devHandle)]->
        pdevOperations[CL_HAL_DEV_INIT](0,0,0,pUserData,usrDataLen);

        if (CL_OK != ret)
        {
            if (CL_IS_CONTINUE_ON_ERR(flags))
            {
               CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error retuned by init fn of \
               devHandle=%d \n",aFunction, \
               pDevBootTable[index].devHandle));
               continue ;
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Error retuned by init fn of \
                devHandle=%d \n",aFunction,pDevBootTable[index].devHandle));
                errFound = CL_TRUE ;
                break ;
            }
        }
      }/*end if(phalDevObjTable.ahalDeviceObj[index].devOperations[])*/
    }/* end for loop */

    clHeapFree(pDevBootTable);
    pDevBootTable=NULL;

    if (CL_TRUE == errFound)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK \n",aFunction));
        CL_FUNC_EXIT();
        return ret;
    }
    else
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }
}
#undef CL_HAL_DEV_INIT 

ClRcT halDevObjCapLenGet(ClHalDeviceObjectH hDevObjHandle,
                          ClUint32T * pDevCapLen)
{
#ifdef CL_DEBUG
    char aFunction[]="halDevObjCapLenGet";
#endif
    CL_FUNC_ENTER();

    if (NULL == pDevCapLen)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if (NULL==halDevObjTable.pphalDeviceObj ||
        NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    
    *pDevCapLen= halDevObjTable.pphalDeviceObj[hDevObjHandle]->devCapLen;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("%s SuCCESS devCapLen=%d ",aFunction,*pDevCapLen));
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT halDevObjCapGet(ClHalDeviceObjectH hDevObjHandle,
                       ClUint32T devCapLen,
                       void * pDevCap)
{
#ifdef CL_DEBUG
    char aFunction[]="halDevObjCapGet";
#endif
    CL_FUNC_ENTER();
    if (NULL == pDevCap)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if (NULL==halDevObjTable.pphalDeviceObj ||
        NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    /* Usr can pass a larger size buffer but not lesser than the size
    of dev Capability structure */
    if (devCapLen < halDevObjTable.pphalDeviceObj[hDevObjHandle]->devCapLen)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error Invalid Param ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }

    if(NULL != halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevCapability)
    {
      memcpy(pDevCap, halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevCapability,
      halDevObjTable.pphalDeviceObj[hDevObjHandle]->devCapLen);
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("%s SuCCESS ",aFunction));
    CL_FUNC_EXIT(); 
    return (CL_OK);
}

ClRcT halDevObjMaxRspTimeGet(ClHalDeviceObjectH hDevObjHandle,
                              ClUint32T * pMaxRspTime)
{
#ifdef CL_DEBUG
    char aFunction[]="halDevObjMaxRspTimeGet";
#endif
    CL_FUNC_ENTER();
    if (NULL == pMaxRspTime)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

     if (NULL==halDevObjTable.pphalDeviceObj ||
         NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    *pMaxRspTime = halDevObjTable.pphalDeviceObj[hDevObjHandle]->maxRspTime;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK respTime=%d ",aFunction,*pMaxRspTime));
    CL_FUNC_EXIT();
    return (CL_OK);

}

ClRcT halDevObjRefCountIncr(ClHalDeviceObjectH hDevObjHandle)
{
#ifdef CL_DEBUG
    char aFunction[]="halDevObjRefCountIncr";
#endif
    
    CL_FUNC_ENTER();
    if (NULL==halDevObjTable.pphalDeviceObj ||
        NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }
    halDevObjTable.pphalDeviceObj[hDevObjHandle]->refCount ++;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK DevHandle =%d ,refCount=%d ",aFunction,
    hDevObjHandle, halDevObjTable.pphalDeviceObj[hDevObjHandle]->refCount ));

    CL_FUNC_EXIT();
    return(CL_OK);    
}

ClRcT halDevObjRefCountDecr(ClHalDeviceObjectH hDevObjHandle)
{
#ifdef CL_DEBUG
    char aFunction[] ="halDevObjRefCountDecr";
#endif
    CL_FUNC_ENTER();
    if (NULL==halDevObjTable.pphalDeviceObj ||
        NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }
    
    halDevObjTable.pphalDeviceObj[hDevObjHandle]->refCount --; 

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK DevHandle =%d ,refCount=%d ",aFunction,
      hDevObjHandle, halDevObjTable.pphalDeviceObj[hDevObjHandle]->refCount ));

    CL_FUNC_EXIT();
    return(CL_OK);    

}

ClRcT halDevObjOperationExecute(ClHalDeviceObjectH hDevObjHandle, 
                                 ClUint32T omId,
                                 ClCorMOIdPtrT moId,
                                 ClUint32T operation,
                                 ClUint32T subOperation,
                                 void *pUserData, ClUint32T usrDataLen)
{
    ClRcT ret =CL_OK;
#ifdef CL_DEBUG
    char aFunction[] ="halDevObjOperationExecute";
#endif

    CL_FUNC_ENTER();

    if((usrDataLen >0)&&(NULL==pUserData))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s userDataLen is >0 but pUserData=NULL\n",
        aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER));
    }

    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if (NULL==halDevObjTable.pphalDeviceObj ||
        NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle]||
        NULL==halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevOperations)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR hDevObjHandle= %d ",
        aFunction, hDevObjHandle));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    
    if (halDevObjTable.pphalDeviceObj[hDevObjHandle]->pdevOperations[operation])
    {
        ret =halDevObjTable.pphalDeviceObj[hDevObjHandle]->
        pdevOperations[operation](omId,moId,subOperation,pUserData,usrDataLen);
    }
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s Device Object API Operation not installed\
        for hDevObjHandle= %d operation =%d\n", aFunction ,\
        hDevObjHandle,operation));
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK for hDevObjHandle= %d, \
    operation=%d \n", aFunction ,hDevObjHandle,operation));
    CL_FUNC_EXIT(); 
    return ret;
}

ClRcT halDevObjIdGet(ClHalDeviceObjectH hDevObjHandle,ClUint32T *pDeviceId)
{
#ifdef CL_DEBUG
    char aFunction[] ="halDevObjIdGet";
#endif
    
    CL_FUNC_ENTER();
    if (NULL == pDeviceId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT(); 
        return( CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }
    if(hDevObjHandle >=halConfig.halNumDevObject)
    {
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC( CL_ERR_INVALID_HANDLE));
    }

    if(NULL==halDevObjTable.pphalDeviceObj ||
       NULL ==halDevObjTable.pphalDeviceObj[hDevObjHandle])
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NULL PTR ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    *pDeviceId = halDevObjTable.pphalDeviceObj[hDevObjHandle]->deviceId;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n %s CL_OK for hDevObjHandle= %d, deviceId=%d \
    \n", aFunction ,hDevObjHandle,*pDeviceId));
     
    CL_FUNC_EXIT();
    return CL_OK;
}
/* For Testing purpose */
ClRcT buffPrint(char srcBuff[],int * pBuffLeft, char destBuff[])
{
    if (*pBuffLeft > (ClInt32T)strlen(srcBuff))
    { 
        strncat(destBuff,srcBuff,*pBuffLeft); 
        *pBuffLeft =*pBuffLeft - strlen(srcBuff);
        memset(srcBuff,0,strlen(srcBuff));
        return CL_OK ;
    }
    else
    { 
       CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n Error BuffLen \n"));
       CL_FUNC_EXIT(); 
       return(CL_HAL_SET_RC(CL_ERR_INVALID_PARAMETER)); 
    }
}

ClRcT halShowDevTable(char buff[] , ClUint32T bufflen)
{
    ClUint32T index=0;
    ClUint32T nIndex=0;
    ClInt32T buffLeft = (ClInt32T)bufflen;
    char temp[128];
    ClRcT retCode = 0;
    ClfpDevOperationT* pTemp=NULL;

    memset(buff,0,bufflen);
    /*memset(temp,0,128);

    buffPrint(temp,&buffLeft,buff) ; */

    if ((NULL == halDevObjTable.pphalDeviceObj))
    { 
        retCode = buffPrint("\n No Device Object installed\n",&buffLeft,buff) ;
        CL_FUNC_EXIT();
        return (CL_OK) ;
    }

    for (index =0; index < halConfig.halNumDevObject;index ++)
    {
       if ( NULL != halDevObjTable.pphalDeviceObj[index])
       {
         snprintf(temp,sizeof(temp),"\ndeviceHandle=%d \n",(ClInt32T)index);
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp,sizeof(temp), "deviceId=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->deviceId);
         retCode = buffPrint(temp, &buffLeft,buff);
            
         snprintf(temp,sizeof(temp), "refCount=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->refCount);
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp, sizeof(temp),"pDevCapability=%p \n",
         (halDevObjTable.pphalDeviceObj[index]->pdevCapability));
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp,sizeof(temp),"devCapLen=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->devCapLen);
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp,sizeof(temp), "state=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->state);
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp,sizeof(temp), "maxRspTime=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->maxRspTime);
         retCode = buffPrint(temp,&buffLeft,buff);

         snprintf(temp,sizeof(temp),"bootUpPriority=%d \n",(ClInt32T)halDevObjTable.
         pphalDeviceObj[index]->bootUpPriority);
         retCode = buffPrint(temp,&buffLeft,buff);

         if(NULL==halDevObjTable.pphalDeviceObj[index]->pdevOperations)
         {
             continue ;
         }
         else
         {
            for(nIndex=0; nIndex<halConfig.halDevNumOperations;nIndex++)
            {
               pTemp =(ClfpDevOperationT*)((ClWordT)(halDevObjTable.pphalDeviceObj[index]->pdevOperations[nIndex]));
               snprintf(temp,sizeof(temp),"DevOperation[%d]=%p\n",nIndex,(void *)pTemp);
               retCode = buffPrint(temp,&buffLeft,buff);
            }
         }
       }/*endif */
    }/* end for loop*/
    return CL_OK ;
}
/* 
Assuming that print information for one halDevObject requires 
512 bytes , on debugging found out that one halDevObject takes 
394 bytes , using buffer of 512 bytes . 
*/
#define PRINT_BUFF_SIZE_DEV_OBJECT 512
ClRcT cliShowDevObjects(int argc, char **argv)
{
    char *pBuff = NULL;
    ClRcT rc =CL_OK;
#ifdef CL_DEBUG
    char aFunction[] = "cliShowDevObjects";
#endif
    ClUint32T buffSize =PRINT_BUFF_SIZE_DEV_OBJECT * halConfig.halNumDevObject;
    pBuff=clHeapAllocate(buffSize);
    if(NULL == pBuff)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error NO Memory ",aFunction));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NO_MEMORY));
    }
    memset(pBuff,0,buffSize);
    rc=halShowDevTable(pBuff,buffSize);
    clOsalPrintf("\n %s \n",pBuff);
    clHeapFree(pBuff);
    return rc ;
}

ClRcT
halInitComponent (ClHalConfT * pAppHalConfig )
{
	ClRcT rc = CL_OK;

    memcpy(&halConfig,pAppHalConfig ,sizeof(ClHalConfT));
	rc = clHalLibInitialize();

	return (rc);
} /* halInitComponent */

/* 
The API below assumes that devObjectTable contains the correct values , in 
case if any operation is not installed , that memory location will be NULL 
. CW needs to ensure this for proper operation of HAL .
*/
ClRcT halDevObjTableCreate()
{
    /* To Initialize to  Large Invalid Value */
    ClHalDeviceObjectH hdevObj =0xEFFFFFFF;
    ClRcT ret=CL_OK;
    ClUint32T index =0; 
    ClUint32T nIndex =0; 
#ifdef CL_DEBUG
    char aFunction[]="halDevObjTableCreate";
#endif
    CL_FUNC_ENTER();

    CL_ASSERT(halConfig.pDevObjectTable);
    if(NULL==halConfig.pDevObjectTable)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("halConfig.pDevObjectTableis NULL\n"));
        CL_FUNC_EXIT();
        return(CL_HAL_SET_RC(CL_ERR_NULL_POINTER));
    }

    for(index=0 ; index < halConfig.halNumDevObject ; index ++)
    {
       ret= halDevObjCreate(halConfig.pDevObjectTable[index].deviceId,
                            halConfig.pDevObjectTable[index].pDevCapability,
                            halConfig.pDevObjectTable[index].devCapLen,
                            halConfig.pDevObjectTable[index].maxRspTime,
                            halConfig.pDevObjectTable[index].bootUpPriority,
                            & hdevObj);
       if (CL_OK!= ret)
       {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n %s Error returned by halDevObjCreate",
            aFunction));
            CL_FUNC_EXIT();
            return ret ;
       }
       for(nIndex = 0 ; nIndex < halConfig.halDevNumOperations; nIndex ++)
       {
            if(NULL != halConfig.pDevObjectTable[index].pDevOperations)
            {
                ret = halDevObjHandlerRegister(hdevObj,
                                           nIndex,
                                           halConfig.pDevObjectTable[index].pDevOperations[nIndex]);
                if (CL_OK!= ret)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n %s Error returned by \
                    halDevObjHandlerRegister", aFunction));
                    CL_FUNC_EXIT();
                    return ret ;
                }
            }
       }
       /* To Initialize to  Large Invalid Value */
       hdevObj=0xEFFFFFFF ;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nBoot Manager Succeeded in creating Dev Objects\n"));
    CL_FUNC_EXIT();
    return CL_OK ;
}    

ClRcT halDevObjTableDelete()
{  
    ClRcT ret=CL_OK;
    ClUint32T index =0; 
#ifdef CL_DEBUG
    char aFunction[]="halDevObjTableDelete";
#endif
    CL_FUNC_ENTER();

    if (NULL!=halDevObjTable.pphalDeviceObj)
    {
        for(index=0 ; index < halConfig.halNumDevObject ; index ++)
        {
            if(NULL != halDevObjTable.pphalDeviceObj[index])
            {
                ret=halDevObjDelete(index);
                if(CL_OK!=ret)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("\n %s Error ret=%x by \
                    halDevObjDelete",aFunction,ret));
                    return ret;
                }
            }

        }
    }
    halDevObjTable.nextIndex =0;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("%s , CL_OK",aFunction)); 
    CL_FUNC_EXIT();
    return ret;
}

ClRcT clHalLibFinalize()
{
    ClRcT rc = CL_OK;
    rc = halDevObjTableDelete();
    halInitDone = CL_FALSE ; 
    return rc;
}
