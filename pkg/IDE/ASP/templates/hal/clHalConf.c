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
 * ModuleName  : hal
 * File        : clHalConf.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains the configuration part for the Clovis HAL layer 
 * This is a sample C file for hal library configuration. In this file the
 * sample configuration is provided for a process which will manage a gigE port. 
 *
 *
 *****************************************************************************/

#include <stdio.h>

/* Clovis Includes */
#include <clHalApi.h>
#include <clHalObjectApi.h>
#include <clHalConf.h>

/******************************************
 * Device Object Configuration 
 ******************************************/        
   
/**
 * The declaration of functions below will come from Application Header File , for
 * the purpose of compilation they have been declared here.
 */ 

ClRcT ethDevInit(
        ClUint32T       omId, 
        ClCorMOIdPtrT   moId,
        ClUint32T       subOperation,
        void            *pUserData,
        ClUint32T       usrDataLen)
{
    ClRcT   rc = CL_OK;
    
    /* */
    
    return rc;
}

ClRcT ethDevOpen(
        ClUint32T       omId, 
        ClCorMOIdPtrT   moId,
        ClUint32T       subOperation,
        void            *pUserData,
        ClUint32T       usrDataLen)
{
    ClRcT   rc = CL_OK;
    
    /* */
    
    return rc;
}

ClRcT ethDevRead(
        ClUint32T       omId, 
        ClCorMOIdPtrT   moId,
        ClUint32T       subOperation,
        void            *pUserData,
        ClUint32T usrDataLen)
{
    ClRcT   rc = CL_OK;
    
    /* */
    
    return rc;
}

ClRcT ethDevWrite(ClUint32T omId, 
        ClCorMOIdPtrT       moId,
        ClUint32T           subOperation,
        void                *pUserData,
        ClUint32T           usrDataLen)
{
    ClRcT   rc = CL_OK;
    
    /* */
    
    return rc;
}

/**
 * The exact omId will come from Information Model for the purpose of compilation
 * they have been declared here.
 */
#define provGigEPortClass_type      1
#define alarmGigEPortClass_type     (provGigEPortClass_type +1)

/*
   This table captures the methods supported by ethernet device .    
   Operations for Ethernet Device Object
*/
ClfpDevOperationT ethDevOperations[HAL_DEV_NUM_OPERATIONS]=
        {
        ethDevInit,     /* HAL_DEV_INIT             */
        ethDevOpen,     /* HAL_DEV_OPEN             */
        NULL,           /* HAL_DEV_CLOSE            */
        ethDevRead,     /* HAL_DEV_READ             */
        ethDevWrite,    /* HAL_DEV_WRITE            */
        NULL,           /* HAL_DEV_COLD_BOOT        */
        NULL,           /* HAL_DEV_WARM_BOOT        */
        NULL,           /* HAL_DEV_PWR_OFF          */
        NULL,           /* HAL_DEV_IMAGE_DN_LOAD    */
        NULL            /* HAL_DEV_DIRECT_ACCESS    */
            
        /* User definded operations, none in this case */
        };

/** 
 * This table will be used by the initialization function of the process
 * to create all the Device  Objects , corresponding to the resources 
 * managed by this process.  
 *
 * In the example below the table creates the device object corresponding to
 * ethernet device driver. 
 */
ClHalDevObjectT devObjectTable[]=
    {
        {
        ETH_DEV_OBJECT_ID,  /* deviceId          */
        NULL,               /* pdevCapability    */
        0,                  /* devCapLen         */
        10,                 /* Max Response Time */
        1,                  /* Boot Up Priority  */
        ethDevOperations    /* */
        }
    };

/******************************************
 * HAL OBJECT CONFIGURATION
 ******************************************/
                             
/**
 * The section below conatins the Configuration code for different MSO to attach
 * appropriate HAL Objects to their OM objects. 
 */
ClHalDevObjectInfoT ethHalDevObjInfo[]=
    {
        {
          ETH_DEV_OBJECT_ID,    /* Device ID                */
          1                     /* Device Access Priority   */
        }
    };

/*List of HAL Objects for the component */
ClHalObjectConfT halObjConf[]= 
    {
        {
            CL_OM_PROV_GIGEPORT_CLASS_TYPE, /* OM ClassId */
            "\\CLASS_Chassis_MO0\\CLASS_GigeBlade_MO1\\CLASS_GigePort_MO0", /* MOID in string  for port 0 */
            ethHalDevObjInfo, /* Information about the Devices */
            (sizeof(ethHalDevObjInfo))/(sizeof(ClHalDevObjectInfoT))   /*Num of DevObjects */
        },
    };

/* Please Do Not Modify the Init of this structure */
ClHalConfT halConfig= 
    {
        HAL_DEV_NUM_OPERATIONS, /* From halConf.h */
        (sizeof(devObjectTable))/(sizeof(ClHalDevObjectT)), /* halNumDevObject */
        devObjectTable, /* DevObject Table */
        sizeof(halObjConf)/sizeof(ClHalObjectConfT),    /* halNumHalObject */
        halObjConf      /* Hal Object Table */
    };



