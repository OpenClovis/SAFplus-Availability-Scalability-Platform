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
 * $File: //depot/dev/main/Andromeda/ASP/components/cor/test/sessionTest/clCorTestBundleSA.c $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>

#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>
#include <clIocApi.h>
#include <clIocServices.h>
#include <clCorNotifyApi.h>
#include <clCpmApi.h>
#include <clTxnApi.h>
#include <clTxnAgentApi.h>
#include <clDebugApi.h>
#include <clCorErrors.h>

#include <clCorTestBundleSA.h>
#include <clCorMetaData.h>


/* Global definitions */
static ClCorTestBundleDataT *tempData = NULL;
static ClUint32T data = 10;

static ClCpmHandleT cpmHandle;
static ClTxnAgentServiceHandleT    gCorTestTxnAgntHdl = 0,gCorTestBundleAgntHdl = 0;

/********* FROM TESTING ********/

ClRcT clCorTestClientDataSave()
{
    ClRcT    rc = CL_OK;
    ClNameT   nodeName ;
    ClCharT   classDbName[CL_COR_MAX_NAME_SZ] = {0};

    clCpmLocalNodeNameGet(&nodeName);

    memcpy(classDbName, nodeName.value, nodeName.length);
    strcat(classDbName,".corClassDb");

    rc = clCorDataSave(classDbName); 
    if(rc != CL_OK) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Save the Cor Class Information in %s. rc[0x%x]", classDbName, rc));
        return rc;
    }

    return rc;
}


/**
 *  Function to create the IM for the session test.
 */

ClRcT  cor_test_IM2()
{
    ClRcT               rc = CL_OK;
    ClCorMOClassPathPtrT moPath = NULL;
    ClCorAttrFlagT       attrConfig = CL_COR_ATTR_CONFIG| CL_COR_ATTR_WRITABLE | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT;
    ClCorAttrFlagT       attrRT_1 = CL_COR_ATTR_RUNTIME ;
    ClCorAttrFlagT       attrRT_2 = CL_COR_ATTR_RUNTIME | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT;
    

    /* Uint64T type declarations */
    rc = clCorClassCreate(TEST_CLASS_R, 0);
    if(rc != CL_OK)
    {
        clOsalPrintf("Information model for the bundle testing is already present so returning. .... \n");
        return CL_OK;
    }

    clCorClassNameSet(TEST_CLASS_R, "RClass");

    clCorClassCreate(TEST_CLASS_S, 0);
    clCorClassNameSet(TEST_CLASS_S, "SClass");

    clCorClassAttributeCreate(TEST_CLASS_S, TEST_CLASS_S_ATTR_1, CL_COR_UINT32);
    clCorClassAttributeValueSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_1, 1500, 1000, 3000); 
    clCorClassAttributeNameSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_1, "S_1");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_1, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_S, TEST_CLASS_S_ATTR_2, CL_COR_UINT16);
    clCorClassAttributeValueSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_2, 100, 10, 300); 
    clCorClassAttributeNameSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_2, "S_1");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S, TEST_CLASS_S_ATTR_2, attrConfig);


    clCorClassCreate(TEST_CLASS_S_MSO, 0);
    clCorClassNameSet(TEST_CLASS_S_MSO, "SClass_MSO");

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_1, CL_COR_UINT32);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_1, 1500, 1000, 3000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_1, "S_1");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_1, attrRT_1);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_2, CL_COR_INT32);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_2, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_2, "S_2");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_2, attrRT_1);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_3, CL_COR_UINT64);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_3, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_3, "S_3");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_3, attrRT_2);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_4, CL_COR_INT64);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_4, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_4, "S_4");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_4, attrRT_2);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_5, CL_COR_UINT16);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_5, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_5, "S_5");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_5, attrRT_2);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_6, CL_COR_INT16);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_6, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_6, "S_6");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_6, attrRT_1);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_7, CL_COR_INT32);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_7, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_7, "S_7");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_7, attrRT_1);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_8, CL_COR_UINT32);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_8, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_8, "S_8");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_8, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_9, CL_COR_UINT8);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_9, 10, 10, 100); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_9, "S_9");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_9, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_10, CL_COR_INT8);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_10, 10, 10, 100); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_10, "S_10");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_10, attrRT_1);

    clCorClassAttributeCreate(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_11, CL_COR_INT8);
    clCorClassAttributeValueSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_11, 10, 10, 100); 
    clCorClassAttributeNameSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_11, "S_11");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_S_MSO, TEST_CLASS_S_MSO_ATTR_11, attrRT_1);


    clCorClassCreate(TEST_CLASS_T, 0);
    clCorClassNameSet(TEST_CLASS_T, "Tclass");

    clCorClassCreate(TEST_CLASS_T_MSO, 0);
    clCorClassNameSet(TEST_CLASS_T_MSO, "Tclass_MSO");

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_1, CL_COR_UINT64);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_1, 10, 0, 32367); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_1, "T_1");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_1, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_2, CL_COR_INT8);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_2, 11, 0, 127); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_2, "T_2");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_2, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_3, CL_COR_UINT8);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_3, 12, 0, 127); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_3, "T_3");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_3, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_4, CL_COR_INT16);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_4, 13, 0, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_4, "T_4");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_4, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_5, CL_COR_UINT16);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_5, 14, 0, 127); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_5, "T_5");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_5, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_6, CL_COR_INT32);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_6, 15, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_6, "T_6");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_6, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_7, CL_COR_UINT32);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_7, 16, 1, 127); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_7, "T_7");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_7, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_8, CL_COR_INT64);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_8, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_8, "T_8");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_8, attrConfig);

    clCorClassAttributeCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_9, CL_COR_UINT64);
    clCorClassAttributeValueSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_9, 100, 10, 1000); 
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_9, "T_9");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_9, attrConfig);

    clCorClassAttributeArrayCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_10, CL_COR_INT64, 5);
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_10, "T_10");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_10, attrRT_1);

    clCorClassAttributeArrayCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_11, CL_COR_INT8, 10);
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_11, "T_11");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_11, attrConfig);

    /* Association attribute */
    clCorClassAssociationCreate(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_12, TEST_CLASS_S, 5);
    clCorClassAttributeNameSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_12, "T_12");
    clCorClassAttributeUserFlagsSet(TEST_CLASS_T_MSO, TEST_CLASS_T_MSO_ATTR_12, attrConfig);

  /* MO Class Tree (blue print) */
   clCorMoClassPathAlloc(&moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_R);
   clCorMOClassCreate(moPath, 1);
 
   clCorMoClassPathAppend(moPath, TEST_CLASS_S);
   clCorMOClassCreate(moPath, 100);
   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_S_MSO);

   clCorMoClassPathTruncate(moPath, 1);
   clCorMoClassPathAppend(moPath, TEST_CLASS_T);
   clCorMOClassCreate(moPath, 100);
   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_T_MSO);


   clCorTestClientDataSave(); 

    return 0;
}


ClRcT 
clCorTestObjCreate()
{
    ClRcT               rc = CL_OK;
    ClCorMOIdT          moId;
    ClCorObjectHandleT  objH = {{0}};


    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(rc == CL_OK)
    {
        clOsalPrintf("Object Tree for the bundle testing is already present so returning ... \n\n");
        return CL_OK;
    }
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }
    
    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdInstanceSet(&moId, 2, 1);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdInstanceSet(&moId, 2, 2);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }
 
    clCorMoIdInstanceSet(&moId, 2, 3);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdTruncate(&moId, 1);
    clCorMoIdAppend(&moId, TEST_CLASS_T, 0);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdInstanceSet(&moId, 2, 1);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}


ClRcT ClCorTestOIAdd()
{
    ClRcT               rc = CL_OK;
    ClCorMOIdT          moId;
    ClCorAddrT          addr = { 0};
    
    addr.nodeAddress = clIocLocalAddressGet();
    addr.portId = CL_COR_TEST_SESSION_IOC_PORT;

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorPrimaryOISet(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering the addr as read OI, rc[0x%x]", rc));
        return rc;
    }
    
    clCorMoIdInstanceSet(&moId, 2, 1);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorPrimaryOISet(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering the addr as read OI, rc[0x%x]", rc));
        return rc;
    }
    

    clCorMoIdInstanceSet(&moId, 2, 2);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }
 
    clCorMoIdInstanceSet(&moId, 2, 3);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorPrimaryOISet(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering the addr as read OI. rc[0x%x]", rc));
        return rc;
    }
    
    clCorMoIdTruncate(&moId, 1);
    clCorMoIdAppend(&moId, TEST_CLASS_T, 0);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdInstanceSet(&moId, 2, 1);
    rc = clCorOIRegister(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorPrimaryOISet(&moId, &addr);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering the addr as read OI. rc[0x%x]", rc));
        return rc;
    }
    
    return rc; 

}



/**
 * Function to get the size of the basic type. The size returned will be helpful while printing.
 */

ClInt32T
clCorTestBasicSizeGet(ClUint32T type)
{
    switch(type)
    {
        case CL_COR_UINT8:
        case CL_COR_INT8:
                return 1;
        break;
        case CL_COR_UINT16:
        case CL_COR_INT16:
                return 2 ;
        break;
        case CL_COR_UINT32:
        case CL_COR_INT32:
                return 4 ;
        break;
        case CL_COR_UINT64:
        case CL_COR_INT64:
                return 8 ;
        break;
        default:
            clOsalPrintf("\n Invalid type for getting the size");
    } 
    return -1;
}

/**
 *  Function to display the data obtained after session get.
 */

void
_clCorTestDisplayData(ClCorTestBundleDataT bundleData)
{
    ClInt32T   noOfItems = 1;
    ClCorTypeT  type      = bundleData.attrType;
    ClUint8T    *data     = NULL;



    clOsalPrintf("\n AttrId [0x%x]: size : %d, Value ", bundleData.attrId, bundleData.size);

    data = bundleData.data;
    while((noOfItems--) > 0)
    {

        switch(type)
        {
            case CL_COR_UINT8:
            case CL_COR_INT8:
                clOsalPrintf(" [%d] ", *(ClUint8T *)data);
                data = data + sizeof(ClUint8T);
            break;
            case CL_COR_UINT16:
            case CL_COR_INT16:
                clOsalPrintf( " [%d] ", *(ClUint16T *)data);
                data = data + sizeof(ClUint16T);
            break;
            case CL_COR_UINT32:
            case CL_COR_INT32:
                clOsalPrintf(" [%d] ", *(ClUint32T *)data);
                data = data + sizeof(ClUint32T);
            break;
            case CL_COR_UINT64:
            case CL_COR_INT64:
                clOsalPrintf(" [%lld] ", *(ClUint64T *)data);
                data = data + sizeof(ClUint64T);
            break;
            case CL_COR_ARRAY_ATTR:
                noOfItems = bundleData.size / clCorTestBasicSizeGet(bundleData.arrType);
                type = bundleData.arrType;
                continue;
            default:
                clOsalPrintf("\n Invalid type for this data value.");
        } 
    }  
    clOsalPrintf("\n");

    return;
}

/**
 *  Callback function for the session.
 */

ClRcT 
clCorTestBundleCallBack( CL_IN ClCorBundleHandleT bundleHandle, 
                         CL_IN ClPtrT             data)
{
    ClRcT       rc = CL_OK;
    ClUint32T   i = 0;
    ClNameT     moIdName    = {0};
    ClCorTestBundleCookiePtrT pTestCookie = (ClCorTestBundleCookiePtrT )data;
    ClCorTestBundleDataPtrT    pTempData = NULL;
        

    if((pTestCookie == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
        return CL_COR_ERR_NULL_PTR;
    }

    pTempData = pTestCookie->cookie;

    if(pTempData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid argument. BundleData is NULL"));
        return CL_COR_ERR_NULL_PTR;
    }

    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("BundleHandle [%d] - No. of jobs are .. %d ", 
                    (ClUint32T )bundleHandle, pTestCookie->noOfElem));
    
    for(i = 0; i < pTestCookie->noOfElem ; i++)
    {
        if((NULL != pTempData[i].jobStatus) && (*(pTempData[i].jobStatus) != CL_OK))
        {
           rc = clCorMoIdToMoIdNameGet(&pTempData[i].moId, &moIdName);
            if(rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Get failed: AttrId[0x%x], Error[0x%x] \n", 
                        pTempData[i].attrId, *pTempData[i].jobStatus));
            else
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Get failed: MoId [%s] AttrId[0x%x] Error[0x%x] \n", moIdName.value, 
                                pTempData[i].attrId, *pTempData[i].jobStatus));
        }
        else
        {
            /* For passed job print the value. */
           _clCorTestDisplayData(pTempData[i]); 
        }
    }

    for(i = 0; i < pTestCookie->noOfElem; i++)
    {   
        if(pTempData[i].data != NULL)
        {
            clHeapFree(pTempData[i].data);
        }
    }
    
    clHeapFree(pTempData);
    
    if(pTestCookie != NULL)
        clHeapFree(pTestCookie);

    /* Bundle Finalize */
    rc = clCorBundleFinalize(bundleHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while finalizing the session. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}

/**
 * Function to test the association.
 */
ClRcT
cor_test_association()
{
    ClRcT                       rc = CL_OK;
    ClCorMOIdT                  moId;
    ClCorObjectHandleT          objH = {{0}}, objH1 = {{0}}, objHandle = {{0}};
    ClCorBundleHandleT          bundleHandle = -1;
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorJobStatusT             jobStatus;
    ClCorAttrValueDescriptorT     attrDesc = {0};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClUint32T                   val32 = 0;
    
    
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);
    clCorMoIdAppend(&moId, TEST_CLASS_S, 5);
    clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);

    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdTruncate(&moId, 1); 
    clCorMoIdAppend(&moId, TEST_CLASS_T, 5);
    clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);
    rc = clCorUtilMoAndMSOCreate(&moId, &objH);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]\n", rc));
        return rc;
    }
    
    clCorMoIdTruncate(&moId, 1);
    clCorMoIdAppend(&moId, TEST_CLASS_S, 5);
    clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);
    rc = clCorObjectHandleGet(&moId, &objH1); 
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle. rc[0x%x]", rc));
        return rc;
    }

    clCorMoIdTruncate(&moId, 1);
    clCorMoIdAppend(&moId, TEST_CLASS_T, 5);
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
    rc = clCorObjectHandleGet(&moId, &objH); 
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorObjectAttributeSet(NULL, objH, NULL, TEST_CLASS_T_MSO_ATTR_12, 0, &objH1, sizeof(ClCorObjectHandleT));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while setting the associationi attribute. rc[0x%x]", rc));
        return rc;
    }
    

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while initializing the handle. rc[0x%x]", rc));
        return rc;
    }

    attrDesc.pAttrPath = NULL;
    attrDesc.attrId = TEST_CLASS_T_MSO_ATTR_12;
    attrDesc.index = 0;
    attrDesc.bufferPtr = &objHandle;
    attrDesc.bufferSize = sizeof(ClCorObjectHandleT);
    attrDesc.pJobStatus = &jobStatus;

    attrList.numOfDescriptor = 1;
    attrList.pAttrDescriptor = &attrDesc;

    rc = clCorBundleObjectGet(bundleHandle, &objH, &attrList);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job. rc[0x%x]", rc));
        return rc;
    }
    
    
    rc = clCorBundleApply(bundleHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the bundle apply. rc[0x%x]", rc));
        rc = clCorBundleFinalize(bundleHandle);

        return rc;
    }
    
    if(jobStatus == CL_OK)
    {
        val32 = 1201;
        rc = clCorObjectAttributeSet(NULL, objHandle, NULL, TEST_CLASS_S_ATTR_1, -1, &val32, sizeof(val32));
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the attrset on the associated attribute. rc[0x%x]", rc));
            return rc;
        }    
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Associated MO's attribute set TC ---- PASSED"));
    }
    else
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Associated MO's attribute set TC ---- FAILED. rc[0x%x]", jobStatus));
   

    clCorBundleFinalize(bundleHandle);    
    return rc;
}


/**
 * Function to test simultaneous session apply.
 */
ClCorJobStatusT             js5[4][11];
ClRcT
cor_test_mutliple_bundle_apply()
{
    ClRcT                       rc = CL_OK;
    ClCorObjectHandleT          objHandle = {{0}};
    ClUint32T                   userFlags = 0, i = 0, j = 0, inst = 0;
    ClCorMOIdT                  moId;
    ClCorAttrValueDescriptorT     attrDesc[11] = {{0}};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrIdT                attrId1[] = { 0x9, 0xa , 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 , 0x12, 0x13 };
    ClCorTestBundleCookiePtrT   pTestCookie[4] = {NULL};
    ClCorBundleHandleT          bundleHandle[4] = {0};
    ClUint32T                   index[4] = {0};
    ClCorTestBundleDataPtrT      pTempData[4] = {NULL}, tempData = NULL;

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);

    for(inst = 0 ; inst < 4; inst++)
    {
        data = 11; /* No. of Jobs */
        
        pTempData[inst] = clHeapAllocate(sizeof(ClCorTestBundleDataT) * data);
        if(pTempData[inst] == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory."));
            return CL_COR_ERR_NULL_PTR;
        }

        rc = clCorBundleInitialize(&bundleHandle[inst], &bundleConfig);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the bundle. r[0x%x]", rc));
            goto freeResource;
        }
              
        pTestCookie[inst] = clHeapAllocate(sizeof(ClCorTestBundleCookieT));
        if(pTestCookie == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
            goto freeResource;
        }

        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

        /* For Runtime attribute - uncomment this */
        rc = clCorObjectHandleGet(&moId, &objHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
            goto freeResource;
        }

        for(j = 0; j < 11 ; j++)
        {
            /* Runtime - 64 bit*/
            rc = clCorObjAttrInfoGet(objHandle, NULL, attrId1[j], &pTempData[inst][index[inst]].attrType,
                     &pTempData[inst][index[inst]].arrType, &pTempData[inst][index[inst]].size, &userFlags);
            if(rc == CL_OK)
            {
                pTempData[inst][index[inst]].moId = moId;
                pTempData[inst][index[inst]].data = clHeapAllocate(pTempData[inst][index[inst]].size);
                if(NULL == pTempData[inst][index[inst]].data)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                    goto freeResource;
                }

                pTempData[inst][index[inst]].attrId = attrDesc[j].attrId = attrId1[j];
                attrDesc[j].pAttrPath = NULL;
                attrDesc[j].index = CL_COR_INVALID_ATTR_IDX;

                attrDesc[j].bufferPtr = pTempData[inst][index[inst]].data;
                attrDesc[j].bufferSize = pTempData[inst][index[inst]].size;

                attrDesc[j].pJobStatus = 
                        pTempData[inst][index[inst]].jobStatus = &js5[inst][j];

                index[inst]++;
            }
        }

        attrList.pAttrDescriptor = attrDesc;
        attrList.numOfDescriptor = j;

        rc = clCorBundleObjectGet(bundleHandle[inst], &objHandle, &attrList);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
        }    

        pTestCookie[inst]->cookie = pTempData[inst];
        pTestCookie[inst]->noOfElem = data;
    }

    for(inst = 0; inst < 4; inst++)
    {
        /* Applying the bundle asynchronously */
        rc = clCorBundleApplyAsync(bundleHandle[inst], clCorTestBundleCallBack, pTestCookie[inst]);
        if(rc != CL_OK)
        {
            clCorBundleFinalize(bundleHandle[inst]);

            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while applying the bundle asynchronously. rc[0x%x]", rc));
            return rc;
        }
    }

    return rc;


freeResource:

    for(j = 0; j < 4; j++)
    { 
        tempData = pTempData[j];
        for(i = 0; (i < index[j]) && (tempData != NULL); i++)
        {   
            if(tempData[i].data != NULL)
            {
                clHeapFree(tempData[i].data);
                tempData[i].data = NULL;
            }
        }
    }
    
    clHeapFree(tempData);
    tempData = NULL;
    
    if(pTestCookie != NULL)
        clHeapFree(pTestCookie);

    return rc;
}



/**
 *  Function to test the get on the combination of 
 *  operational, runtime and config attributes.
 */

ClCorJobStatusT             js1[4][11], js4[12] = {0};

ClRcT 
cor_test_bundle_attr_get()
{
    ClRcT                       rc = CL_OK;
    ClCorBundleHandleT          bundleHandle = -1;
    ClCorObjectHandleT          objHandle = {{0}};
    ClUint32T                   index = 0;
    ClUint32T                   userFlags = 0, i = 0, j = 0, inst = 0;
    ClCorMOIdT                  moId;
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrIdT                attrId[] = { 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 , 0x12, 0x13, 0x14, 0x15};
    ClCorAttrIdT                attrId1[] = { 0x9, 0xa , 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 , 0x12, 0x13 };
    ClCorTestBundleCookiePtrT   pTestCookie = NULL;
    ClCorAttrValueDescriptorT     attrDesc[11] = {{0}};
    ClCorAttrValueDescriptorListT attrList = {0};

    data = 55; /* No. of Jobs */
    
    tempData = clHeapAllocate(sizeof(ClCorTestBundleDataT) * data);
    if(tempData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory."));
        return CL_COR_ERR_NULL_PTR;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the bundle. r[0x%x]", rc));
        goto freeResource;
    }
          
    pTestCookie = clHeapAllocate(sizeof(ClCorTestBundleCookieT));
    if(pTestCookie == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
        goto freeResource;
    }

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);

    for(inst = 0; inst < 4; inst++)
    {
        clCorMoIdInstanceSet(&moId, 2, inst);

        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

        /* For Runtime attribute - uncomment this */
        rc = clCorObjectHandleGet(&moId, &objHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
            goto freeResource;
        }

        for(j = 0; j < 11 ; j++)
        {
            /* Runtime - 64 bit*/
            rc = clCorObjAttrInfoGet(objHandle, NULL, attrId1[j], &tempData[index].attrType,
                     &tempData[index].arrType, &tempData[index].size, &userFlags);
            if(rc == CL_OK)
            {
                tempData[index].moId = moId;
                tempData[index].data = clHeapAllocate(tempData[index].size);
                if(NULL == tempData[index].data)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                    goto freeResource;
                }

                tempData[index].attrId = attrDesc[j].attrId = attrId1[j];
                attrDesc[j].pAttrPath = NULL;
                attrDesc[j].index = CL_COR_INVALID_ATTR_IDX;

                attrDesc[j].bufferPtr = tempData[index].data;
                attrDesc[j].bufferSize = tempData[index].size;

                attrDesc[j].pJobStatus = tempData[index].jobStatus = &js1[inst][j];

                index++;
            }
        }

        attrList.pAttrDescriptor = attrDesc;
        attrList.numOfDescriptor = j;

        rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
        }    
    }
    
    /* Config Attributes */

    clCorMoIdSet(&moId, 2, TEST_CLASS_T, 1);

    rc = clCorObjectHandleGet(&moId, &objHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
        goto freeResource;
    }


    for(j = 0; j < 11; j++)
    {
        /*Config - 8 bit*/

        rc = clCorObjAttrInfoGet(objHandle, NULL, attrId[j], &tempData[index].attrType, 
                        &tempData[index].arrType, &tempData[index].size, &userFlags);
        if(rc == CL_OK)
        {
            tempData[index].moId = moId;
            tempData[index].data = clHeapAllocate(tempData[index].size);
            if(NULL == tempData[index].data)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                goto freeResource;
            }

            tempData[index].attrId = attrDesc[j].attrId = attrId[j];
            attrDesc[j].pAttrPath = NULL;
            attrDesc[j].index = CL_COR_INVALID_ATTR_IDX;

            attrDesc[j].bufferPtr = tempData[index].data;
            attrDesc[j].bufferSize = tempData[index].size;

            attrDesc[j].pJobStatus = tempData[index].jobStatus = &js4[i]; 

            index++;
        }
    }

    attrList.pAttrDescriptor = attrDesc;
    attrList.numOfDescriptor = (j);

    rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
    }

    pTestCookie->cookie = tempData;
    pTestCookie->noOfElem = data;

    /* Applying the bundle asynchronously */
    rc = clCorBundleApplyAsync(bundleHandle, clCorTestBundleCallBack, pTestCookie);
    if(rc != CL_OK)
    {
        clCorBundleFinalize(bundleHandle);

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while applying the bundle asynchronously. rc[0x%x]", rc));
        return rc;
    }
    
    return rc;

freeResource:

    for(i = 0; i < index; i++)
    {   
        if(tempData[i].data != NULL)
        {
            clHeapFree(tempData[i].data);
            tempData[i].data = NULL;
        }
    }
    
    clHeapFree(tempData);
    tempData = NULL;
    
    if(pTestCookie != NULL)
        clHeapFree(pTestCookie);

    return rc;
}

/**
 * Function to test the bundle get operation for a array attribute
 * having attribute jobs for different indices.
 */
ClRcT cor_test_Array_attr_bundle_get()
{
    ClRcT                       rc = CL_OK;
    ClCorBundleHandleT          bundleHandle = -1;
    ClCorObjectHandleT          objHandle = {{0}};
    ClInt32T                   index = CL_COR_INVALID_ATTR_IDX;
    ClUint32T                   userFlags = 0, i = 0, j = 0, k = 0;
    ClCorMOIdT                  moId;
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrIdT                attrId1[] = { 0xb, 0xc, 0xd, 0xf, 0xe};
    ClCorTestBundleCookiePtrT   pTestCookie = NULL;
    ClCorAttrValueDescriptorT     attrDesc[12] = {{0}};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClCorJobStatusT               status[12] = {0};
  
    data = 12; /* No. of Jobs */
    
    tempData = clHeapAllocate(sizeof(ClCorTestBundleDataT) * data);
    if(tempData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory."));
        return CL_COR_ERR_NULL_PTR;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the bundle. r[0x%x]", rc));
        goto freeResource;
    }
          
    pTestCookie = clHeapAllocate(sizeof(ClCorTestBundleCookieT));
    if(pTestCookie == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
        goto freeResource;
    }

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    clCorMoIdAppend(&moId, TEST_CLASS_T, 1);

    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    /* For Runtime attribute - uncomment this */
    rc = clCorObjectHandleGet(&moId, &objHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
        goto freeResource;
    }

    for(j = 0, k = 0; j < 11 ; j++)
    {
        /* Runtime - 64 bit*/
        rc = clCorObjAttrInfoGet(objHandle, NULL, attrId1[k], &tempData[j].attrType,
                 &tempData[j].arrType, &tempData[j].size, &userFlags);
        if(rc == CL_OK)
        {
            tempData[j].moId = moId;

            if(tempData[j].attrType == CL_COR_ARRAY_ATTR)
            {
                tempData[j].size = (clCorTestBasicSizeGet(tempData[j].arrType));
            }
            tempData[j].data = clHeapAllocate(tempData[j].size);
            if(NULL == tempData[j].data)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                goto freeResource;
            }

            tempData[j].attrId = attrDesc[j].attrId = attrId1[k];
            attrDesc[j].pAttrPath = NULL;
        
            if(attrId1[k] == 0xd || attrId1[k] == 0xe)
            {
                if( index == CL_COR_INVALID_ATTR_IDX)
                {
                    index = 0;
                }
                else
                {
                    index++;
                    if(index == 3)
                        k++;
                }
            }
            else
            {
                index =  CL_COR_INVALID_ATTR_IDX;
                k++; 
            }
            attrDesc[j].index = index;

            attrDesc[j].bufferPtr = tempData[j].data;
            attrDesc[j].bufferSize = tempData[j].size;

            attrDesc[j].pJobStatus = &status[j];
            tempData[j].jobStatus = &status[j];
        }
        else if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the attribute info get, rc[0x%x]", rc));
        }
   }
 
#if 1

    /** TC: Adding the attribute whose index is same as the index of job
     *  already added but have the buffer size different.
     */

    //j++;
    printf("j is %d \n", j);
    /* Runtime - 64 bit*/
    rc = clCorObjAttrInfoGet(objHandle, NULL, 0xd, &tempData[j].attrType,
             &tempData[j].arrType, &tempData[j].size, &userFlags);
    if(rc == CL_OK)
    {
        tempData[j].moId = moId;

        tempData[j].size = (clCorTestBasicSizeGet(tempData[j].arrType))*3;
        tempData[j].data = clHeapAllocate(tempData[j].size);
        if(NULL == tempData[j].data)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
            goto freeResource;
        }

        tempData[j].attrId = attrDesc[j].attrId = 0xd;
        attrDesc[j].pAttrPath = NULL;
        attrDesc[j].index = 1;

        attrDesc[j].bufferPtr = tempData[j].data;
        attrDesc[j].bufferSize = tempData[j].size;

        attrDesc[j].pJobStatus = &status[j];
        tempData[j].jobStatus = &status[j];
    }
    else if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the attribute info get, rc[0x%x]", rc));
    }

#endif
    attrList.pAttrDescriptor = attrDesc;
    attrList.numOfDescriptor = (j + 1);

    rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
        return rc;
    }

    pTestCookie->cookie = tempData;
    pTestCookie->noOfElem = data;

    /* Applying the bundle asynchronously */
    rc = clCorBundleApplyAsync(bundleHandle, clCorTestBundleCallBack, pTestCookie);
    if(rc != CL_OK)
    {
        clCorBundleFinalize(bundleHandle);

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while applying the bundle asynchronously. rc[0x%x]", rc));
        return rc;
    }
    
    return rc;

freeResource:

    for(i = 0; i < index; i++)
    {   
        if(tempData[i].data != NULL)
        {
            clHeapFree(tempData[i].data);
            tempData[i].data = NULL;
        }
    }
    
    clHeapFree(tempData);
    tempData = NULL;
    
    if(pTestCookie != NULL)
        clHeapFree(pTestCookie);

    return rc;

}


/**
 * Function to test the get functionality using simple get which is internally using bundle get.
 *
 */
ClRcT 
cor_test_get_using_simple_get()
{
    ClRcT rc = CL_OK;
    ClUint32T size = 0;
    ClCorMOIdT  moId ;
    ClCorObjectHandleT objH = {{0}};
    ClUint32T val32 = 0, i = 0;
    ClUint16T valu_16 = 0;
    ClInt16T  vali_16 = 0;
    ClInt64T val64  = 0;
    ClInt64T valA_64[2] = {0}; 
    ClInt8T valA_8 [10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ClInt8T valA_81 [10] = {0};
    ClInt8T  val8 = 0;
    
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);  
    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);  

    /* TC -1 Get on configuration attribute of 32 bit */
    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 1: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc2;
    }

    val32 = 1001;
    size = sizeof(val32);
    rc = clCorObjectAttributeSet(NULL, objH, NULL, TEST_CLASS_S_ATTR_1, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC -1 : Failed rc[0x%x]. line[%d] \n", rc, __LINE__);
        goto tc2;
    } 

    val32 = 0;
    size = sizeof(val32);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_ATTR_1, -1, &val32, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC -1 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc2;
    }

    clOsalPrintf("\n\n");
    if(val32 == 1001)
    {
        clOsalPrintf("TC - 1: PASSED \n");
    }
    else
    {
        clOsalPrintf("TC - 1: FAILED , val32[%d]\n", val32);
    }

tc2:        

    /* TC -2 Get on configuration attribute of 16 bit */
    valu_16 = 200;
    size = sizeof(valu_16);
    rc = clCorObjectAttributeSet(NULL, objH, NULL, TEST_CLASS_S_ATTR_2, -1, &valu_16, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 2 : Failed rc[0x%x]. line[%d] \n", rc, __LINE__);
        goto tc3;
    } 

    valu_16 = 0;
    size = sizeof(valu_16);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_ATTR_2, -1, &valu_16, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 2 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc3;
    }

    clOsalPrintf("\n\n");
    if(valu_16 == 200)
    {
        clOsalPrintf("TC - 2: PASSED [16 bit config get]\n");
    }
    else
    {
        clOsalPrintf("TC - 2: FAILED [16 bit config get ] val16[%d]\n", valu_16);
    }


tc3:
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    /* TC -3 Get on runtime attribute of 32 bit */
    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 3: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc5;
    }

    val32 = 0;
    size = sizeof(val32);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_MSO_ATTR_1, -1, &val32, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 3 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc4;
    }

    clOsalPrintf("\n\n");
    if(val32 == 109)
    {
        clOsalPrintf("TC - 3 : PASSED [runtime get %d]\n", val32);
    }
    else
    {
        clOsalPrintf("TC - 3 : FAILED [runtime get %d]\n", val32);
    }

tc4:        
    
    /* TC -4 Get on runtime attribute of 16 bit */
    vali_16 = 0;
    size = sizeof(vali_16);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_MSO_ATTR_6, -1, &vali_16, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 4 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc5;
    }

    clOsalPrintf("\n\n");
    if(vali_16 == 114)
    {
        clOsalPrintf("TC - 4: PASSED [runtime get valu16 %d]\n", vali_16);
    }
    else
    {
        clOsalPrintf("TC - 4: FAILED [runtime get valu16 [%d]\n", vali_16);
    }

tc5:

    /* TC -5 Get on runtime attribute of Array of 64 bit  - only index 3 */
    clCorMoIdSet(&moId, 2, TEST_CLASS_T, 1);
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 5: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc6;
    }

    val64 = 0;
    size = sizeof(val64);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_T_MSO_ATTR_10, 3, &val64, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 5 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc6;
    }

    clOsalPrintf("\n\n");
    if(val64 == 1004667667)
    {
        clOsalPrintf("TC - 5: PASSED [runtime Array index get val64 %lld]\n", val64);
    }
    else
    {
        clOsalPrintf("TC - 5: FAILED [runtime Array index get val64 [%lld]\n", val64);
    }

tc6:

    /* TC -6 Get on runtime attribute of Array of 64 bit  - index  2 & 3*/
    size = sizeof(valA_64);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_T_MSO_ATTR_10, 2, valA_64, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 6 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc7;
    }

    clOsalPrintf("\n\n");
    if( (valA_64[0] == 1003557557) && (valA_64[1] == 1004667667))
    {
        clOsalPrintf("TC - 6: PASSED [runtime Array get val64 ]\n");
    }
    else
    {
        clOsalPrintf("TC - 6: FAILED [runtime get val64 ]\n" );
    }

tc7:
 
    /* TC -7 Get on configuration attribute of Array of 8 bit  - only index 3 */
    size = sizeof(valA_8);
    rc = clCorObjectAttributeSet(NULL, objH, NULL, TEST_CLASS_T_MSO_ATTR_11, 0, valA_8, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 7 : Failed rc[0x%x]. line[%d] \n", rc, __LINE__);
        goto tc8;
    }

    size = sizeof(val8);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_T_MSO_ATTR_11, 3, &val8, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 7 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc8;
    }

    clOsalPrintf("\n\n");
    if(val8 == 4)
    {
        clOsalPrintf("TC - 7: PASSED [runtime Array index get val8 %d]\n", val8);
    }
    else
    {
        clOsalPrintf("TC - 7: FAILED [runtime Array index get val8 [%d]\n", val8);
    }

tc8:

    /* TC -8 Get on configuration attribute of Array of 8 bit  -  whole array */
    size = sizeof(valA_81);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_T_MSO_ATTR_11, 0, valA_81, &size); 
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 8 : Failed rc[0x%x]: Line [%d] \n", rc, __LINE__);
        goto tc9;
    }

    clOsalPrintf("\n\n");
    if( memcmp(valA_81 , valA_8, sizeof(valA_8)) == 0)
    {
        clOsalPrintf("TC - 8: PASSED [runtime Array get val8 ]\n");
    }
    else
    {
        clOsalPrintf("TC - 8: FAILED [runtime get val8 ]\n" );

        ClUint32T i = 0;
        for(i = 0; i < 10; i++)
        {
            clOsalPrintf("[%d], ", valA_8[i]);
        }
        clOsalPrintf("\n");
        for(i = 0; i < 10; i++)
        {
            clOsalPrintf("[%d], ", valA_8[i]);
        }
        clOsalPrintf("\n");

    }
    clOsalPrintf("\n\n");

tc9:
    /* TC - 9 : Doing a get operation on runtime attribute whose primary OI is not registered.*/
    clCorMoIdSet(&moId, 2, TEST_CLASS_S, 2);  

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 9: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc10;
    }

    val32 = 0;
    size = sizeof(val32);
    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_MSO_ATTR_1, -1, &val32, &size); 
    if(CL_OK == rc)
    {
        clOsalPrintf("TC - 9 : FAILED \n");
        goto tc10;
    }
    else
        clOsalPrintf("TC - 9 : PASSED \n");


tc10:
    /* TC - 10: Doing a get on configuration attribute 70000 times to check the memory usage of COR (txn client)*/
    clCorMoIdSet(&moId, 2, TEST_CLASS_S, 1);  
    clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);
    i = 0;

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 10: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc11;
    }

#if 1
    while( i < 5000)
    {
        val32 = 0;
        size = sizeof(val32);
        rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_ATTR_1, -1, &val32, &size); 
        if(CL_OK != rc)
        {
            clOsalPrintf("TC - 10 : FAILED - rc[0x%x]: \n", rc);
            rc = CL_OK;
            i = 0;
            goto tc11;
        }
        i++;
        clOsalPrintf("TC - 10 : value got [%d] in iteration [%d] \n", val32, i);
    }
#endif
    clOsalPrintf("TC - 10 : PASSED \n");

tc11:
    /* TC - 11: Doing a get on runtime attribute 70000 times to check the memory usage of COR (txn client)*/
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 11: Failed. rc[0x%x], line[%d] \n", rc, __LINE__);
        goto tc12;
    }
    i = 0;

#if 1
    while( i < 5000)
    {

        val32 = 0;
        size = sizeof(val32);
        rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_S_MSO_ATTR_1, -1, &val32, &size); 
        if(CL_OK != rc)
        {
            clOsalPrintf("TC - 11 : FAILED - rc[0x%x]: \n", rc);
            rc = CL_OK;
            goto tc12;
        }
        i++;
        clOsalPrintf("TC - 11 : value got [%d] in iteration [%d] \n", val32, i);
    }
#endif
    clOsalPrintf("TC - 11 : PASSED \n");

tc12:

    return rc;
}


ClRcT _clCorBundleTestAttrWalkFunc( ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId, ClCorAttrTypeT attrType,
                                    ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT attrData, 
                                    void *cookie)
{
    ClRcT rc = CL_OK;  
    ClCorTestBundleDataT testData = {0};

    testData.attrType = attrType;
    testData.attrId = attrId;
    testData.attrType = attrDataType;
    testData.size = size ;
    testData.data = value;

    _clCorTestDisplayData(testData);

    if ( attrData & CL_COR_ATTR_CONFIG )
    {
        clOsalPrintf("Attr is config \n" );
    }
    else 
    {
        clOsalPrintf("Attr is Runtime \n");
    }

    if (attrData & CL_COR_ATTR_CACHED)
    {
        clOsalPrintf("Attr is cached. \n");
    }
    else
        clOsalPrintf("Attr is non-cached \n");

    return rc;
}



/**
 * Function to test the attribute walk on the MO containing
 * runtime attribute. The attribute can be mix of runtime,
 * config. The runtime attribute can be cached or non-cached. 
 */ 
ClRcT cor_test_runtime_non_cached_attribute_walk()
{
    ClRcT   rc = CL_OK;
    ClCorObjectHandleT objH;
    ClCorMOIdT   moId;
    ClCorObjAttrWalkFilterT walkFilter = {0};
    ClUint32T       i = 0;
 
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("Failed while creating the object RClass. rc[0x%x] \n", rc);
            return rc;
        }
    }
    
    clCorMoIdAppend(&moId, TEST_CLASS_S, 2);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        rc = clCorUtilMoAndMSOCreate(&moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("Failed while creating the object of SClass. rc[0x%x]", rc);
            return rc;
        }
    }
    else
    {
        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
        rc = clCorObjectHandleGet(&moId, &objH);
        if (CL_OK != rc)
        {
            rc = clCorObjectCreate(NULL, &moId, &objH);
            if (CL_OK != rc)
            {
                clOsalPrintf("Failed while creating the object for the Sclass MSO. rc[0x%x] \n", rc);
                return rc;
            }
        }
    }


    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);
    clCorMoIdAppend(&moId, TEST_CLASS_S, 1); 
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        clOsalPrintf("Failed while getting the object handle for attribute walk. rc[0x%x] \n", rc);
        return rc;
    }
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    walkFilter.baseAttrWalk = CL_TRUE;
    walkFilter.contAttrWalk = CL_FALSE;
    walkFilter.pAttrPath = NULL;
    walkFilter.attrId = CL_COR_INVALID_ATTR_ID;
    walkFilter.index = CL_COR_INVALID_ATTR_IDX;
    walkFilter.attrWalkOption = CL_COR_ATTR_WALK_ALL_ATTR;
    
    for (i = 0 ; i < 100; i++)
    {
        rc = clCorObjectAttributeWalk(objH, &walkFilter, _clCorBundleTestAttrWalkFunc, NULL);
        if (CL_OK != rc)
        {
            clOsalPrintf("Failed while doing the attribute walk on the SclassMO. rc[0x%x] \n", rc);
            return rc;
        }

        clOsalPrintf("################################ Completed attribue walk iteration >>>>>>> [%d] \n", i);
    }
        
    return rc;
}

/** 
 *  Function to test the set functionality on the operational attribute.
 */

ClRcT cor_test_RT_Cached_attr_set()
{
    ClRcT               rc  = CL_OK;
    ClCorMOIdT          moId ;
    ClCorObjectHandleT  objHandle = {{0}};
    ClCorTxnIdT         tid = 0;
    ClUint64T           val64 = 0;
    ClUint16T           val16 = 0;
    ClUint8T            val8 = 0;
    ClUint32T           val32 = 0;
    ClUint32T           size64 = sizeof(ClUint64T);
    ClUint32T           size16 = sizeof(ClUint16T);
    ClUint32T           size32 = sizeof(ClUint32T);
    ClUint32T           size8 = sizeof(ClUint8T);
    ClUint32T           i = 0;
     
     
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    clCorMoIdAppend(&moId, TEST_CLASS_S, 1);
    clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

    /* For Runtime attribute - uncomment this */
    rc = clCorObjectHandleGet(&moId, &objHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
        return rc;
    }
     
    /* TC - 1 : Only one operational attribute */
    tid = 0;

    val16 = 20;
    rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xd, -1, &val16, size16); 
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
        return rc;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling commit . rc[0x%x]", rc));
        return rc;
    }

    /* TC - 2 : One operational and two config attribute */
    tid = 0;
    
    val64 = 40;
    rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xb, -1, &val64, size64);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
        return rc;
    }
    
    val32 = 50;
    rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0x10, -1, &val32, size32);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
        return rc;
    }

    val8 = 30;
    rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0x11, -1 , &val8, size8);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
        return rc;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling commit . rc[0x%x]", rc));
        return rc;
    }

    /* TC - 3 : Multiple operational attribute set jobs. */
    tid = 0;

    for(i = 0 ; i < 2; i++)
    {
        clCorMoIdInstanceSet(&moId, 2, i);
        rc = clCorObjectHandleGet(&moId, &objHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
            return rc;
        }

        val64 = 60 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xb, -1 , &val64, size64);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }

        val64 = 80 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xc, -1 , &val64, size64);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }

        val16 = 90 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xd, -1 , &val16, size16);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }

    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling commit . rc[0x%x]", rc));
        return rc;
    }

    /* TC - 4 : Multiple jobs having combo of operational and config attributes*/
    tid = 0;
    for (i = 0; i < 2; i ++)
    {
        clCorMoIdInstanceSet(&moId, 2, i);
        rc = clCorObjectHandleGet(&moId, &objHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
            return rc;
        }

        val16 = 40 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xd, -1, &val16, size16);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }
        
        val64 = 50 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0xb, -1, &val64, size64);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }

        val8 = 60 + i*10;
        rc = clCorObjectAttributeSet(&tid, objHandle, NULL, 0x11, -1 , &val8, size8);
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling attribute set . rc[0x%x]", rc));
            return rc;
        }

    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while calling commit . rc[0x%x]", rc));
        return rc;
    }

    return rc;
}



/**
 * Test function to get the attribute for which OH is invalid. 
 */

ClCorJobStatusT             js3[4][11];
ClRcT
cor_test_session_invalid_oh()
{
    ClRcT                       rc = CL_OK;
    ClCorBundleHandleT          bundleHandle = -1;
    ClCorObjectHandleT          objHandle[4];
    ClUint32T                   index = 0;
    ClUint32T                   i = 0, j = 0, inst = 0;
    ClCorMOIdT                  moId;
    ClCorAttrValueDescriptorT     attrDesc[4] = {{0}};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrIdT                attrId1[] = { 0x9, 0xa , 0xb, 0xc};
    ClCorTestBundleCookiePtrT   pTestCookie = NULL;
    ClUint8T                    temp0[8] = {0x10, 0x01, 0x01, 0x00, 0x10, 0x10, 0x00, 0x10};
    ClUint8T                    temp1[8] = {0x10, 0x01, 0x11, 0x00, 0x10, 0x10, 0x00, 0x10};
    ClUint8T                    temp2[8] = {0x10, 0x01, 0x01, 0x11, 0x10, 0x10, 0x00, 0x10};
    ClUint8T                    temp3[8] = {0x10, 0x01, 0x01, 0x00, 0x11, 0x11, 0x00, 0x10};

    objHandle[0] = *(ClCorObjectHandleT *)&temp0;
    objHandle[1] = *(ClCorObjectHandleT *)&temp1;
    objHandle[2] = *(ClCorObjectHandleT *)&temp2;
    objHandle[3] = *(ClCorObjectHandleT *)&temp3;
        
    data = 16; /* No. of Jobs */
    
    tempData = clHeapAllocate(sizeof(ClCorTestBundleDataT) * data);
    if(tempData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory."));
        return CL_COR_ERR_NULL_PTR;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the bundle. r[0x%x]", rc));
        goto freeResource;
    }
          
    pTestCookie = clHeapAllocate(sizeof(ClCorTestBundleCookieT));
    if(pTestCookie == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
        goto freeResource;
    }

    for(inst = 0; inst < 4; inst++)
    {
        for(j = 0; j < 4 ; j++)
        {
            tempData[index].moId = moId;
            tempData[index].size = sizeof(ClUint8T);
            tempData[index].data = clHeapAllocate(tempData[index].size);
            if(NULL == tempData[index].data)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                goto freeResource;
            }

            tempData[index].attrId = attrDesc[j].attrId = attrId1[j];
            attrDesc[j].pAttrPath = NULL;
            attrDesc[j].index = CL_COR_INVALID_ATTR_IDX;

            attrDesc[j].bufferPtr = tempData[index].data;
            attrDesc[j].bufferSize = tempData[index].size;

            attrDesc[j].pJobStatus = tempData[index].jobStatus = &js3[inst][j];

            index++;
        }

        attrList.pAttrDescriptor = attrDesc;
        attrList.numOfDescriptor = j;

        rc = clCorBundleObjectGet(bundleHandle, &objHandle[inst], &attrList);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
        }    
    }
    
    pTestCookie->cookie = tempData;
    pTestCookie->noOfElem = data;

    rc = clCorBundleApplyAsync(bundleHandle, clCorTestBundleCallBack, pTestCookie);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while applying the bundle. rc[0x%x]", rc));
        return rc;
    }

    return rc;

freeResource:

    for(i = 0; i < index; i++)
    {   
        if(tempData[i].data != NULL)
        {
            clHeapFree(tempData[i].data);
            tempData[i].data = NULL;
        }
    }
    
    clHeapFree(tempData);
    tempData = NULL;
    
    if(pTestCookie != NULL)
        clHeapFree(pTestCookie);


    return rc;
}


/**
 *  TC for testing the synchronous version of the bundle.
 */

ClRcT
cor_test_sync_bundle()
{

    ClRcT                       rc = CL_OK;
    ClCorBundleHandleT          bundleHandle = -1;
    ClCorObjectHandleT          objHandle = {{0}};
    ClUint32T                   index = 0;
    ClUint32T                   userFlags = 0, i = 0, j = 0, inst = 0;
    ClCorMOIdT                  moId;
    ClCorAttrValueDescriptorT     attrDesc[11] = {{0}};
    ClCorAttrValueDescriptorListT attrList = {0};
    ClCorBundleConfigT          bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrIdT                attrId1[] = { 0x9, 0xa , 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 , 0x12, 0x13 };
    ClCorJobStatusT             js3[4][11];

    data = 44; /* No. of Jobs */
    
    tempData = clHeapAllocate(sizeof(ClCorTestBundleDataT) * data);
    if(tempData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory."));
        return CL_COR_ERR_NULL_PTR;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the bundle. r[0x%x]", rc));
        goto freeResource;
    }

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_R, 0);

    clCorMoIdAppend(&moId, TEST_CLASS_S, 0);

    for(inst = 0; inst < 4; inst++)
    {
        clCorMoIdInstanceSet(&moId, 2, inst);

        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);

        /* For Runtime attribute - uncomment this */
        rc = clCorObjectHandleGet(&moId, &objHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the object handle . rc[0x%x]", rc));
            goto freeResource;
        }

        for(j = 0; j < 11 ; j++)
        {
            /* Runtime - 64 bit*/
            rc = clCorObjAttrInfoGet(objHandle, NULL, attrId1[j], &tempData[index].attrType,
                     &tempData[index].arrType, &tempData[index].size, &userFlags);
            if(rc == CL_OK)
            {
                tempData[index].moId = moId;
                tempData[index].data = clHeapAllocate(tempData[index].size);
                if(NULL == tempData[index].data)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NO  Memory. "));
                    goto freeResource;
                }

                tempData[index].attrId = attrDesc[j].attrId = attrId1[j];
                attrDesc[j].pAttrPath = NULL;
                attrDesc[j].index = CL_COR_INVALID_ATTR_IDX;

                attrDesc[j].bufferPtr = tempData[index].data;
                attrDesc[j].bufferSize = tempData[index].size;

                attrDesc[j].pJobStatus = tempData[index].jobStatus = &js3[inst][j];

                index++;
            }
        }

        attrList.pAttrDescriptor = attrDesc;
        attrList.numOfDescriptor = j;

        rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle job. rc[0x%x]", rc));
        }    
    }
    
    rc = clCorBundleApply(bundleHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while applying the bundle synchronously. rc[0x%x]", rc));
        return rc;
    }


freeResource:

    for(i = 0; i < index; i++)
    {   
        if(tempData[i].data != NULL)
        {
            _clCorTestDisplayData(tempData[i]); 
            clHeapFree(tempData[i].data);
            tempData[i].data = NULL;
        }
    }

    rc = clCorBundleFinalize(bundleHandle);
    return rc;
}
/**
 * Update the data for MSO of class S.
 */
ClRcT 
clCorTestBundleUpdate1(ClCorAttrIdT attrId, ClPtrT data, ClUint32T size, ClInt32T index)
{
    ClRcT       rc = CL_OK;

    clOsalPrintf("%s:  AttrId id [0x%x] \n", __FUNCTION__, attrId);

    switch(attrId)
    {
        case 9:
                *(ClUint32T *) data = 109;
        break;
        case 10:
                *(ClUint32T *) data = 110;
        break;
        case 11:
            {    *(ClUint64T *) data = 111; rc = CL_ERR_INVALID_STATE; }
        break;
        case 12:
                *(ClUint64T *) data = 112;
        break;
        case 13:
                *(ClUint16T *) data = 113;
        break;
        case 14:
                *(ClUint16T *) data = 114;
        break;
        case 15:
                *(ClUint32T *) data = 115;
        break;
        case 16:
                *(ClUint32T *) data = 116;
        break;
        case 17:
                *(ClUint8T *) data = 117;
        break;
        case 18:
                *(ClUint8T *) data = 118;
        break;
        case 19:
                *(ClUint8T *) data = 119;
        break;
        default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid attrId passed. "));
                rc = CL_COR_ERR_GET_DATA_NOT_FOUND;
    }
    
    return rc;
}

/**
 * Update the data for MSO of class T.
 */
ClRcT 
clCorTestBundleUpdate2(ClCorAttrIdT attrId, ClPtrT data, ClUint32T size, ClInt32T index)
{
    ClRcT   rc = CL_OK;
    
    switch(attrId)
    {
        case 0xd:
        {
            clOsalPrintf("index is %d ", index);
            ClUint64T val[] = {1001337337, 1002447447, 1003557557, 1004667667, 1005777777};
            if(index == CL_COR_INVALID_ATTR_IDX)
                memcpy(data, val, size);
            else
                memcpy(data, &val[index], size);
        }
        break;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid attrid passed to the OI."));
            rc = CL_COR_ERR_GET_DATA_NOT_FOUND;
    }

    return rc;
}


/**
 *  Cor-Txn job walk function.
 */

ClRcT   clCorTestJobWalk( ClCorTxnIdT     corTxnId,
                          ClCorTxnJobIdT  jobId,
                          void           *arg )
{
    ClRcT                   rc              = CL_OK;
    ClCorMOIdT              moId;
    ClCorOpsT               corOp           = CL_COR_OP_RESERVED;
    ClCorAttrPathPtrT       attrPath        = NULL;
    ClCorAttrIdT            attrId          = 0;
    ClPtrT                  pValue          = NULL, data    = NULL;
    ClUint32T               valueSize       = 0;
    ClInt32T                index = 0;

    CL_FUNC_ENTER();


    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the moid during job walk. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &corOp);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the Op type. rc[0x%x]", rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("INSIDE PROV JOB WALK , Op is [%x]", corOp));

    rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &attrPath);

    rc = clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &valueSize);

    data = clHeapAllocate(valueSize);
    if(data == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory"));
        return CL_ERR_NO_MEMORY;
    }
    memset(data, 0, valueSize);

    clCorMoIdShow(&moId);
    if(moId.node[moId.depth -1 ].type == TEST_CLASS_S)
        rc = clCorTestBundleUpdate1(attrId, data, valueSize, index);
    if(moId.node[moId.depth - 1 ].type == TEST_CLASS_T)
        rc = clCorTestBundleUpdate2(attrId, data, valueSize, index);

    clOsalPrintf("The return code for this value is. rc[0x%x] \n", rc);

    /* Update the status for this job. */
    clCorTxnJobStatusSet(corTxnId, jobId, rc);
    
    if(rc == CL_OK)
    {
         rc = clCorBundleAttrValueSet(corTxnId, jobId, data);
         if(rc != CL_OK)
         {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while updating the attribute value. rc[0x%x]", rc));
            clHeapFree(data);
            return rc;
         }
    }

    /* Free the memory allocated earlier. */
    clHeapFree(data);

    CL_FUNC_EXIT();
    return CL_OK;
}


/**
 * Read callback function of read service of transaction agent.
 */
ClRcT clCorTestReadCB(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT           rc          = CL_OK;
    ClCorTxnIdT     corTxnId    = 0;

    CL_FUNC_ENTER();

    clOsalPrintf("\n \n ########   Inside read callback .... txnHandle [0x%d] ######### \n", (ClUint32T)txnHandle );


    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting cor-txn Id. rc[0x%x]", rc));
        return rc;
    }

    rc = clCorTxnJobWalk(corTxnId, clCorTestJobWalk, 0);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Job got failed rc [0x%x]. Updating the txn handle..", rc));
        
    }

    if(clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId) != CL_OK)
    {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
    }

    clCorTxnIdTxnFree(corTxnId);
 
    return rc;
}



/* Callback functions for transaction-agent */
static ClTxnAgentCallbacksT gCorTestReadBundleCB = {
    .fpTxnAgentJobPrepare   = NULL,
    .fpTxnAgentJobCommit    = clCorTestReadCB,
    .fpTxnAgentJobRollback  = NULL,
};


/* Function for printing the Job Contents  */
void corTxnJobInfoDisplay(ClCorTxnIdT txnId, ClCorTxnJobIdT  jobId)
{
  ClCorAttrPathT  *attrPath;
  ClCorAttrTypeT   attrType;
  ClCorTypeT       attrDataType;
  ClCorOpsT        op;
  ClInt32T         index;
  ClCorAttrIdT     attrId;
  void            *pValue;
  ClUint32T        size;
  ClUint32T        sizeCnt = 0;


        clCorTxnJobOperationGet(txnId, jobId, &op);
        if(op == CL_COR_OP_SET)
        {
           clOsalPrintf("\n");
           clOsalPrintf("\t\t\tOperation  .......... CL_COR_OP_SET \n");
           clCorTxnJobAttrPathGet(txnId, jobId, &attrPath);
           if(attrPath != NULL) 
           {
               clOsalPrintf("\t\t\tAttrPath ............ ");
               clCorAttrPathShow(attrPath);
           }
 
           clCorTxnJobAttributeTypeGet(txnId,
                                        jobId,
                                         &attrType,
                                           &attrDataType);

           clCorTxnJobSetParamsGet(txnId, jobId, &attrId, &index, &pValue, &size);

           clOsalPrintf("\t\t\tAttrId .............. 0x%08x\n",  attrId);

           if(attrType == CL_COR_SIMPLE_ATTR)  
               clOsalPrintf("\t\t\tAttrType ............ CL_COR_SIMPLE_ATTR\n");
           else if(attrType == CL_COR_ARRAY_ATTR)
              {
               clOsalPrintf("\t\t\tAttrType ............ CL_COR_ARRAY_ATTR\n");
                 sizeCnt = size;
              }
           else 
               clOsalPrintf("\t\t\tAttrType ............ CL_COR_ASSOCIATION_ATTR\n");

               clOsalPrintf("\t\t\tSize ................ %d\n", size);
               if(attrType == CL_COR_ARRAY_ATTR)
               clOsalPrintf("\t\t\tIndex ............... %d\n",        index);
               clOsalPrintf("\t\t\tValue ............... ");
 
              do
              {
                  switch(attrDataType)
                  {
                       case CL_COR_INT8:
                       case CL_COR_UINT8:
                         clOsalPrintf("%d", *(ClUint8T *)pValue);
                         if(sizeCnt)
                          {
                              clOsalPrintf(", ");
                              pValue = (ClUint8T *)pValue + 1;
                              sizeCnt -= 1;
                          }
                         break;
              
                       case CL_COR_INT16:
                       case CL_COR_UINT16:
                         clOsalPrintf("%d",*(ClUint16T *)pValue);
                         if(sizeCnt)
                          {
                              clOsalPrintf(", ");
                              pValue = (ClUint8T *)pValue + 2;
                              sizeCnt -= 2;
                          }
                         break;

                       case CL_COR_INT32:
                       case CL_COR_UINT32:
                         clOsalPrintf("%d",*(ClUint32T *)pValue);
                         if(sizeCnt)
                          {
                              clOsalPrintf(", ");
                              pValue = (ClUint8T *)pValue + 4;
                              sizeCnt -= 4;
                          }
                         break;

                       case CL_COR_INT64:
                       case CL_COR_UINT64:
                         clOsalPrintf("%lld", *(ClUint64T *)pValue);
                         if(sizeCnt)
                          {
                              clOsalPrintf(", ");
                              pValue = (ClUint8T *)pValue + 8;
                              sizeCnt -= 8;
                          }
                         break;
                       default:
                         break;
                  }
              }while(sizeCnt > 0);
        
             clOsalPrintf("\n");
         
        }
        else if (op == CL_COR_OP_CREATE)
        {
           clOsalPrintf("\t\t\t Operation  .......... CL_COR_OP_CREATE");
        }
        else
        {
           clOsalPrintf("\t\t\t Operation  .......... CL_COR_OP_DELETE");
        }

}


/**
 * Prepare callback for 2-PC transaction.
 */
ClRcT clCorTestTxnPrepareCB(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    ClCorMOIdT moId;
    ClCorTxnIdT txnId;
    ClCorTxnJobIdT jobId = 0;

   clOsalPrintf("\n\n\n ################## TRANSACTION PREPARE [txnHandle -- [%d] ]--  number %d ###################### \n", txnHandle,++i);
       
   clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize,  &txnId); 
   clCorTxnJobMoIdGet(txnId, &moId);

   clOsalPrintf("\n\t\t\tMoId  .......... ");
   clCorMoIdShow(&moId);
        

   clCorTxnFirstJobGet(txnId, &jobId);
    do
    {
       corTxnJobInfoDisplay(txnId, jobId);  
 
    } while(clCorTxnNextJobGet(txnId, jobId, &jobId) == CL_OK);
        
   clCorTxnIdTxnFree(txnId); 

    return (rc);
}


ClRcT clCorTestTxnCommitCB(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    clOsalPrintf("\n Calling Txn Commit ..... txnHandle [%d] , %d \n", (ClUint32T)txnHandle, ++i);
    return (rc);
}

ClRcT clCorTestTxnRollbackCB(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    clOsalPrintf("\n Calling Txn Rollback ..... txnHandle [%d] - %d \n", (ClUint32T )txnHandle, ++i);
    return (rc);
}


/* Callback functions for transaction-agent */
static ClTxnAgentCallbacksT gCorTestTransactionCB = {
    .fpTxnAgentJobPrepare   = clCorTestTxnPrepareCB,
    .fpTxnAgentJobCommit    = clCorTestTxnCommitCB,
    .fpTxnAgentJobRollback  = clCorTestTxnRollbackCB,
};



/**
 * Function to register the transaction agent.
 */
ClRcT clCorTestTxnInterfaceInit()
{
    ClRcT                   rc = CL_OK;
                                                                                                                           
    clOsalPrintf("Initializing the session callbacks");

    rc = clTxnAgentServiceRegister(CL_COR_TXN_SERVICE_ID_READ, gCorTestReadBundleCB,
                                   &(gCorTestBundleAgntHdl));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering for read service. rc[0x%x]", rc ));
    }

    rc = clTxnAgentServiceRegister(CL_COR_TXN_SERVICE_ID_WRITE, gCorTestTransactionCB,
                                   &(gCorTestTxnAgntHdl));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while registering for read service. rc[0x%x]", rc ));
    }

                                                                                                                            
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Initialization done\n"));
    CL_FUNC_EXIT();
    return (rc);
}



/**
 *  Function to deregister the transaction agent.
 */
ClRcT 
clCorTestTxnDeregister()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    rc = clTxnAgentServiceUnRegister(gCorTestTxnAgntHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deregistration of the transaction agent . rc[0x%x]", rc));
        return rc;
    }

    rc = clTxnAgentServiceUnRegister(gCorTestBundleAgntHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deregistration of the transaction agent . rc[0x%x]", rc));
        return rc;
    }

    CL_FUNC_EXIT();
    return rc;
}


ClRcT clCorTestTerminate(ClInvocationT invocation,
                     const ClNameT  *compName)
{
    ClRcT rc = CL_OK;
    clOsalPrintf("Inside appTerminate \n");
                                                                                                                                          
    clOsalPrintf("Unregister with CPM before Exit ................. %s\n", compName->value);
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);
    clOsalPrintf("Finalize before Exit ................. %s\n", compName->value);

    rc = clCpmClientFinalize(cpmHandle);
    clOsalPrintf("After Finalize ................. %s\n", compName->value);

    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;
}

ClRcT clCorTestFinalize()
{
    clOsalPrintf("\nSome finalization related stuff is supposed to put at this place\n");
    /* Function for unregistering the transaction agent. */
    clCorTestTxnDeregister();
    return CL_OK;

}

ClRcT clCorTestStateChange(ClEoStateT eoState)
{
    clOsalPrintf("\nSome state change related stuff\n");
    return CL_OK;
}

ClRcT clCorTestHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    return CL_OK;
}

ClRcT clCorTestCSISetCallback (
                    CL_IN ClInvocationT invocation,
                    CL_IN const ClNameT *pCompName,
                    CL_IN ClAmsHAStateT haState,
                    CL_IN ClAmsCSIDescriptorT csiDescriptor )
{
    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;
}


ClRcT clCorTestCSIRmvCallback (
                        CL_IN ClInvocationT invocation,
                        CL_IN const ClNameT *pCompName,
                        CL_IN const ClNameT *pCsiName,
                        CL_IN ClAmsCSIFlagsT csiFlags)
{
    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;

}

/*** Application Init ****/
ClRcT  clCorTestInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClEoExecutionObjT  *pEoHandle;
    ClRcT rc;
	ClNameT                     appName;
	ClCpmCallbacksT             callbacks;
	ClIocPortT                  iocPort;
	ClVersionT                  corVersion;

    clEoMyEoObjectGet(&pEoHandle);   
	
    clCorTestTxnInterfaceInit();

    /* CPM related initialization */
     corVersion.releaseCode = 'B';
     corVersion.majorVersion = 0x1;
     corVersion.minorVersion = 0x1;

     callbacks.appHealthCheck = NULL;
     callbacks.appTerminate = clCorTestTerminate;
     callbacks.appCSISet = clCorTestCSISetCallback;
     callbacks.appCSIRmv = clCorTestCSIRmvCallback;
     callbacks.appProtectionGroupTrack = NULL;
     callbacks.appProxiedComponentInstantiate = NULL;
     callbacks.appProxiedComponentCleanup = NULL;

     clEoMyEoIocPortGet(&iocPort);
     clOsalPrintf("Application Address 0x%x Port 0x%x\n", clIocLocalAddressGet(), iocPort);

     rc = clCpmClientInitialize(&cpmHandle, &callbacks, &corVersion);
     clOsalPrintf("After clCpmClientInitialize %d\t. rc[0x%x]\n", cpmHandle, rc);

     rc = clCpmComponentNameGet(cpmHandle, &appName);
     clOsalPrintf("After clCpmComponentNameGet %d\t %s\n", cpmHandle, appName.value);

     rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
     clOsalPrintf("After clCpmClientRegister. rc[0x%x]\n", rc);


    /*** START OF TEST CASES ****/

/* Test cases for runtime and operational attribute. */


    /*****   Create basic Infomation model ***********/
    rc = cor_test_IM2();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the information model . rc[0x%x]", rc));
        return rc;
    }

    rc = clCorTestObjCreate();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO,("Failed while creating the object. rc[0x%x]", rc));
    }
    rc = ClCorTestOIAdd();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the OI in the OI list, rc [0x%x]", rc));
    }


#if 0
    ClUint32T                   i = 0;
    while(i < 5)
    {
        /* TC 1 : This will test the get on the runtime and config attributes. */
        rc = cor_test_bundle_attr_get();
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("TC : Bundle Attribute Get : failed. "));   
            rc = CL_OK;
        }
        i++;
    }
#endif


#if 0
    /* TC 2: To test the set on a runtime cached attribute. */
    rc = cor_test_RT_Cached_attr_set();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Faile while doing the attrbute set of the \
             runtime cached attributes. rc[0x%x]", rc));
        rc = CL_OK;
    }
#endif

#if 0
    /* TC 3: To test the synchronous version of the bundle apply */
    rc = cor_test_sync_bundle();
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing synchronous bundle processing. rc[0x%x]", rc));
        return rc;
    }
        
#endif  

#if 0
    /* TC 4: Get session contianing job with invalid object handle. */
    rc = cor_test_session_invalid_oh();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing get on a invalid object. rc[0x%x]", rc));
        rc = CL_OK;
    }
    
#endif


#if 0
    /* TC 5: The test code to test the multiple asynchronous session started simultaneously */
    rc = cor_test_mutliple_bundle_apply();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing get in multi bundle asynchronously. rc[0x%x]", rc));
        rc = CL_OK;
    }
#endif

#if 0

    rc = cor_test_association();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while testing the association attributes. "));
        rc = CL_OK;
    }
    
#endif

#if 0

    rc = cor_test_Array_attr_bundle_get();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while testing the Array attributes. "));
        rc = CL_OK;
    }
 
#endif

#if 0

    rc = cor_test_get_using_simple_get();
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while testing the get operation using simple get. rc[0x%x]", rc));
        rc = CL_OK;
    }

#endif

#if 1
    rc = cor_test_runtime_non_cached_attribute_walk();
    if (CL_OK != rc)
    {
        clOsalPrintf("Failed while testing the attribute walk on the MOs containing \
                runtime cached/non-cached attributes. rc[0x%x] \n", rc);
        rc = CL_OK;
    }    
#endif

  return rc;
}

ClEoConfigT clEoConfig = {
                   "GigeComp0_EO",                      /* EO Name*/
                    CL_COR_TEST_SESSION_EO_THREAD_PRIORITY, /* EO Thread Priority */
                    CL_COR_TEST_SESSION_EO_THREADS,         /* No of EO thread needed */
                    CL_COR_TEST_SESSION_IOC_PORT,           /* Required Ioc Port */
                    CL_EO_USER_CLIENT_ID_START,
                    CL_EO_USE_THREAD_FOR_RECV,              /* Whether to use main thread for eo Recv or not */
                    clCorTestInitialize,                     /* Function CallBack  to initialize the Application */
                    clCorTestFinalize,                       /* Function Callback to Terminate the Application */
                    clCorTestStateChange,                    /* Function Callback to change the Application state */
                    clCorTestHealthCheck,                    /* Function Callback to change the Application state */
     };


ClUint8T clEoBasicLibs[] = {
    CL_TRUE,                    /* osal */
    CL_TRUE,                    /* timer */
    CL_TRUE,                    /* buffer */
    CL_TRUE,                    /* ioc */
    CL_TRUE,                    /* rmd */
    CL_TRUE,                    /* eo */
    CL_FALSE,                   /* om */
    CL_FALSE,                   /* hal */
    CL_FALSE,                   /* dbal */
};
                                                                                                                            
ClUint8T clEoClientLibs[] = {
    CL_TRUE,                   /* cor */
    CL_FALSE,                   /* cm */
    CL_FALSE,                   /* name */
    CL_FALSE,                   /* log */
    CL_FALSE,                   /* trace */
    CL_FALSE,                   /* diag */
    CL_TRUE,                   /* txn */
    CL_FALSE,                   /* hpi */
    CL_FALSE,                   /* cli */
    CL_FALSE,                   /* alarm */
    CL_FALSE,                   /* debug */
    CL_FALSE                    /* gms */
};
