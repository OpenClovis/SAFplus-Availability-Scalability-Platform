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
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/cor/test/clCorTestClient.c $
 * $Author: hargagan $
 * $Date: 2007/04/13 $
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
#include <clCorIpi.h>

#include <clCorTestClient.h>

/********* FROM TESTING ********/

ClRcT clCorTestClientDataSave()
{
    ClRcT    rc = CL_OK;
    ClNameT   nodeName ;
    ClCharT   *classDbName = NULL;

    clCpmLocalNodeNameGet(&nodeName);

    classDbName = clHeapAllocate(nodeName.length + sizeof(".corClassDb") + 30);
    if(classDbName == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Allocate Memory"));
        return CL_COR_ERR_NO_MEM;
    }
    memset(classDbName, 0, nodeName.length + sizeof(".corClassDb") + 30);
    memcpy(classDbName, nodeName.value, nodeName.length);
    strcat(classDbName,".corClassDb");

    rc = clCorDataSave(classDbName); 
    if(rc != CL_OK) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Save the Cor Class Information in %s. rc[0x%x]", classDbName, rc));
        return rc;
    }

    clHeapFree(classDbName);
    return rc;
}


ClRcT attrWalkFp(ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId, ClCorAttrTypeT attrType,
              ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT flags, void *cookie)
{

   clCorAttrPathShow(pAttrPath);

    
   clOsalPrintf("Attribute Id 0x%x, type 0x%x, native type 0x%x, flags %d \n", attrId, attrType, attrDataType, flags);
   return 0;
}


/* Attribute Walk Test */
void cor_test_AttributeWalk()
{
   ClCorMOIdPtrT moId;
   ClCorObjectHandleT objH;  

   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_N, 0);
   clCorObjectCreate(NULL, moId, &objH);

   
   ClCorAttrPathPtrT attrPath;
   clCorAttrPathAlloc(&attrPath); 

    
  /* First set the array attribute */
   clCorAttrPathAppend(attrPath, TEST_CLASS_N_ATTR_3, -2);
   clCorAttrPathAppend(attrPath, TEST_CLASS_K_ATTR_3, -2);
  		

    ClUint32T value[4] = {19,0,0,0};
    ClRcT   rc = CL_OK;
    ClCharT ch[100] = {0};

   ClCorObjAttrWalkFilterT attrWalkFilter;
   attrWalkFilter.baseAttrWalk = CL_FALSE;
   attrWalkFilter.contAttrWalk = CL_TRUE;
   attrWalkFilter.pAttrPath    = attrPath;
   attrWalkFilter.attrId       = CL_COR_INVALID_ATTR_ID;
   attrWalkFilter.index        = CL_COR_INVALID_ATTR_IDX;
   attrWalkFilter.cmpFlag      = CL_COR_ATTR_CMP_FLAG_INVALID; 
   attrWalkFilter.attrWalkOption  =  CL_COR_ATTR_WALK_ALL_ATTR;
   attrWalkFilter.value        = (void *)value;
   attrWalkFilter.size         = sizeof(value);
  
   rc = clCorObjectAttributeWalk(objH, &attrWalkFilter , attrWalkFp, NULL); 
   if(rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCorObjectAttributeWalk Failed. rc[0x%x]", rc)); 
   ch[0] = '\0';
   sprintf(ch,"\tclCorObjectAttributeWalk Test. rc[0x%x]. Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);
}



/* Cookies for following test cases. */
ClUint32T   cookie1 = 1;
ClUint32T   cookie2 = 2;
ClUint32T   cookie3 = 3;
ClUint32T   cookie4 = 4;
ClUint32T   cookie5 = 5;
ClUint32T   cookie6 = 6;

ClRcT cor_test_eventIM1()
{
   ClCorMOIdPtrT moId;
   ClRcT rc;
   ClCorAttrListPtrT   pAttrList;
   ClCorObjectHandleT  handle;
   ClCharT             ch[150] = {0};
 

  /* Create the default tree first */
   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 0);
   rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, moId, &handle);
   ch[0] = '\0';
   sprintf(ch, "\n MO Create \\0x100:0. rc[0x%x]\n", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_B, 1);
   rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, moId, &handle);
   ch[0] = '\0';
   sprintf(ch, "\n MO Create \\0x100:0\\0x200:1. rc[0x%x]\n", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_C, 1);
   rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, moId, &handle);
   ch[0] = '\0';
   sprintf(ch, "\n MO Create \\0x100:0\\0x200:1\\0x300:1. rc[0x%x]\n", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

  /* Make provision to store one attribute only */
   pAttrList = (ClCorAttrListPtrT)clHeapAllocate(sizeof(ClCorAttrListT));
		
   clCorMoIdInitialize(moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 0);
   clCorMoIdAppend(moId, TEST_CLASS_B, 2); 
 
/* TC1:  Create with specific moId. */
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, NULL, NULL, CL_COR_OP_CREATE, (void *)&cookie1, generateSubscriptionId());
   ch[0] = '\0';
   sprintf(ch, "\tEvent Subscribe: Op: CREATE, MoId: \\0x100:0\\0x200:2. rc[0x%x], Result[%s]", rc, (rc?"FAIL":"PASS"));
   CL_COR_TEST_PUT_IN_FILE(ch);
   
/* TC2: Set with specific moId ( No AttrPath & NO AttList) */
   clCorMoIdSet(moId, 2, TEST_CLASS_B, 1); 
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, NULL, NULL, CL_COR_OP_SET, (void *)&cookie2, generateSubscriptionId()); 
   ch[0] = '\0';
   sprintf(ch, "\tEvent Subscribe: Op:SET, MoId: \\0x100:0\\0x200:1. rc[0x%x], Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);

/* TC3: Set with specific moId ( No AttrPath + AttList)  */
   clCorMoIdAppend(moId, TEST_CLASS_C, 1);
   pAttrList->attrCnt = 1;
   pAttrList->attr[0] = TEST_CLASS_C_ATTR_1;
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, NULL, pAttrList, CL_COR_OP_SET, (void *)&cookie3, generateSubscriptionId()); 
   ch[0] = '\0';
   sprintf(ch, "\tEvent Subscribe: Op:SET, MoId: \\0x100:0\\0x200:1\\0x300:1, AttrId: 0x301. rc[0x%x], Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);


/* TC4: Set with specific moId (AttrPath + NO  List)  */
   ClCorAttrPathPtrT pAttrPath;
   clCorAttrPathAlloc(&pAttrPath);
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_C_ATTR_3, 0);
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, pAttrPath, NULL, CL_COR_OP_SET, (void *)&cookie4, generateSubscriptionId()); 
   ch[0] = '\0';
   sprintf(ch,"\tEvent Subscribe: Op:SET, MoId: \\0x100:0\\0x200:1\\0x300:1, AttrPath: \\0x303:0. rc[0x%x], Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);

/* TC5: Set with specific moId (AttrPath + AttrList)  */
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_B_ATTR_3, 0);
   
  /**  Allocating for two attributes**/ 
   pAttrList = (ClCorAttrListPtrT)clHeapAllocate(sizeof(ClCorAttrListT) + sizeof(ClCorAttrIdT));
   pAttrList->attrCnt = 2;
   pAttrList->attr[0] = TEST_CLASS_A_ATTR_1;
   pAttrList->attr[1] = TEST_CLASS_A_ATTR_4;
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, pAttrPath, pAttrList, CL_COR_OP_SET, (void *)&cookie5, generateSubscriptionId()); 
   ch[0] = '\0';
   sprintf(ch,"\tEvent Subscribe: Op:SET, MoId: \\0x100:0\\0x200:1\\0x300:1, AttrPath: \\0x303:0\\0x203:0, AttrList: 0x101, 0x104"
                                        "rc[0x%x], Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);


/* TC6: Set with specific moId (AttrPath + (different) AttrList)  */
   pAttrList->attrCnt = 1;
   pAttrList->attr[0] = TEST_CLASS_A_ATTR_2;
 
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, pAttrPath, pAttrList, CL_COR_OP_SET, (void *)&cookie6, generateSubscriptionId()); 
   ch[0] = '\0';
   sprintf(ch,"\tEvent Subscribe: Op:SET, MoId: \\0x100:0\\0x200:1\\0x300:1, AttrPath: \\0x303:0\\0x203:0, AttrList: 0x102."
                                        "rc[0x%x], Result[%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);

/* CLI-commands for verifying the Test Cases. */

/*
setc 1
setc corServer_SysController_0

TC1:  Create with specific moId
+
corObjCreate \0x100:0\0x200:2
-
corObjCreate \0x100:0\0x201:3

TC2: Set with specific moId ( No AttrPath & NO AttList)
+
attrSet \0x100:0\0x200:1 -1 null 0x201 -1 25                     
attrSet \0x100:0\0x200:1 -1 null 0x202 -1 45
-
attrSet \0x100:0\0x200:2 -1 null 0x202 -1 55

TC3: Set with specific moId ( NO AttrPath + AttList)
+     
attrSet \0x100:0\0x200:1\0x300:1 -1 null 0x301 -1 35
-
attrSet \0x100:0\0x200:1\0x300:1 -1 null 0x302 -1 45

TC4: Set with specific moId (AttrPath + NO  List)
+
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0 0x201 -1 65
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0 0x202 -1 75
-
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:1 0x202 -1 85

TC5: Set with specific moId (AttrPath + AttrList)
+      
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x104 -1 85
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x101 -1 95
-
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x103 -1 115
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x105 -1 125
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:1 0x101 -1 135   
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:1\0x203:1 0x101 -1 145 


TC6: Set with specific moId (AttrPath + (different) AttrList)
+
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x102 -1 105
-
attrSet \0x100:0\0x200:1\0x300:1 -1 \0x303:0\0x203:0 0x101 -1 95   (cookie shd be 5 in the Event)

*/
   return rc;
}

void cor_test_IM1()
{
    ClCorMOClassPathPtrT moPath;  
    ClCorAttrFlagT       attrConfig = CL_COR_ATTR_CONFIG| CL_COR_ATTR_WRITABLE | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT;
    ClRcT               rc = CL_OK;

   /* Uint64T type declarations */
   rc = clCorClassCreate(TEST_CLASS_A, 0);
   if(rc != CL_OK)
        return;
   clCorClassNameSet(TEST_CLASS_A, "Aclass");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, CL_COR_INT8);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, 10, (signed char)0x80, 0x7f); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, "A");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, CL_COR_UINT8);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, 10, 0, 0xff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, "Aa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, 10, 0x0, 0xffffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, "Aaaa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, CL_COR_INT32);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, 10, (ClInt32T)0x80000000, 0x7fffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, "Aaaa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, CL_COR_INT16);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, 0x30, (ClInt16T)0x8000, 0x7fff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, "Aaaaaa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, CL_COR_UINT16);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, 0xffff, 0, 0xffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, "Aaaaa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, CL_COR_INT64);
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, "uint64");
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, 0xffff, 0x8000000000000000, 0x7fffffffffffffff); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, CL_COR_UINT64);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, 0xffff, 0, 0xffffffffffffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, "Int64");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, attrConfig );

   clCorClassAttributeArrayCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_3, CL_COR_UINT64, 5000);
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_3, "Aaa");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_3, attrConfig );

   clCorClassAttributeArrayCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_10, CL_COR_UINT32, 1000);
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_10, "Array32");
   clCorClassAttributeUserFlagsSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_10, attrConfig);

   clCorClassCreate(TEST_CLASS_B, 0);
   clCorClassNameSet(TEST_CLASS_B, "Bclass");

   clCorClassAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, "B");
   clCorClassAttributeValueSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, 20, 0, 2000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, "Bb");
   clCorClassAttributeValueSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, 20, 0, 2000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, attrConfig );
   /* Containment attr*/
   clCorClassContainmentAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_3, TEST_CLASS_A, 0, 5);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_3, "Bbb"); 


   clCorClassCreate(TEST_CLASS_C, 0);
   clCorClassNameSet(TEST_CLASS_C, "Cclass");

   clCorClassAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, "C");
   clCorClassAttributeValueSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, 30, 0, 3000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, attrConfig );

   clCorClassAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, "Cc");
   clCorClassAttributeValueSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, 30, 0, 3000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, attrConfig );
 
   /* Containment attr*/
   clCorClassContainmentAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_3, TEST_CLASS_B, 0, 10);
    clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_3, "Ccc"); 

   clCorClassCreate(TEST_CLASS_D, 0);
   clCorClassNameSet(TEST_CLASS_D, "Dclass");

   clCorClassAttributeCreate(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, "D");
   clCorClassAttributeValueSet(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, 40, 0, 4000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, attrConfig );

   clCorClassCreate(TEST_CLASS_E, 0);
   clCorClassNameSet(TEST_CLASS_E, "Eclass");
   clCorClassAttributeCreate(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, "C");
   clCorClassAttributeValueSet(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, 50, 0, 5000); 
   clCorClassAttributeUserFlagsSet(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, attrConfig );

  /* MO Class Tree (blue print) */
   clCorMoClassPathAlloc(&moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_A);
   clCorMOClassCreate(moPath, 50000);
 
   clCorMoClassPathAppend(moPath, TEST_CLASS_B);
   clCorMOClassCreate(moPath, 100);

   clCorMoClassPathAppend(moPath, TEST_CLASS_C);
   clCorMOClassCreate(moPath, 100);

   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_C);
   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT, TEST_CLASS_B);

   clCorMoClassPathAppend(moPath, TEST_CLASS_D);
   clCorMOClassCreate(moPath, 100);

   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_C);

   clCorMoClassPathSet(moPath, 4, TEST_CLASS_E);
   clCorMOClassCreate(moPath, 100);
    
   clCorMoClassPathInitialize(moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_E);
   clCorMOClassCreate(moPath, 70000);

   clCorClassCreate(TEST_CLASS_F, 0);
   clCorClassNameSet(TEST_CLASS_F, "TestClass_F");

  /* top class.. all classes inherit from it.*/
   clCorClassCreate(TEST_CLASS_G, 0);
   clCorClassNameSet(TEST_CLASS_G, "TestClass_G");
   /* 	SIMPLE */
   clCorClassAttributeCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_G,TEST_CLASS_G_ATTR_1,attrConfig);

   clCorClassAttributeValueSet(TEST_CLASS_G, TEST_CLASS_G_ATTR_1, 1, 0 , 100);
   /* ARRAY */
   clCorClassAttributeArrayCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_2, CL_COR_UINT32, 10);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_G, TEST_CLASS_G_ATTR_2, attrConfig );
   /* ASSOCIATION */
   clCorClassAssociationCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_3, TEST_CLASS_F, 10);

   /* Base class - G*/
   clCorClassCreate(TEST_CLASS_H, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_H, "TestClass_H");
   clCorClassAttributeCreate(TEST_CLASS_H, TEST_CLASS_H_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_H, TEST_CLASS_H_ATTR_1, 7, 0 , 70);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_H,TEST_CLASS_H_ATTR_1,attrConfig);

   /* Base class - F*/
   clCorClassCreate(TEST_CLASS_I, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_I, "TestClass_I");
   clCorClassAttributeCreate(TEST_CLASS_I, TEST_CLASS_I_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_I, TEST_CLASS_I_ATTR_1, 6, 0 , 60);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_I,TEST_CLASS_I_ATTR_1,attrConfig);

   /* Base class - E (F,G)*/
   clCorClassCreate(TEST_CLASS_J, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_J, "TestClass_J");
   clCorClassAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_J, TEST_CLASS_J_ATTR_1, 5, 0 , 50);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_J,TEST_CLASS_J_ATTR_1,attrConfig);
   /* CONTAINMENT - F  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_2, TEST_CLASS_I, 0, 2);
   /* CONTAINMENT - G  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_3, TEST_CLASS_H, 0, 2);


   /* Base class - D (E, F)*/
   clCorClassCreate(TEST_CLASS_K, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_K, "TestClass_K");
   clCorClassAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_K, TEST_CLASS_K_ATTR_1, 4, 0 , 40);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_K,TEST_CLASS_K_ATTR_1, attrConfig);
   /* CONTAINMENT - E  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_2, TEST_CLASS_J, 0, 2);
   /* CONTAINMENT - F  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_3, TEST_CLASS_I, 0, 2);

   /* Base class - C (D, G)*/
   clCorClassCreate(TEST_CLASS_L, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_L, "TestClass_L");
   clCorClassAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_L, TEST_CLASS_L_ATTR_1, 3, 0 , 30);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_L,TEST_CLASS_L_ATTR_1, attrConfig);
   /* CONTAINMENT - D  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_2, TEST_CLASS_K, 0, 1);
   /* CONTAINMENT - G  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_3, TEST_CLASS_H, 0, 2);

   /* Base class - B*/
   clCorClassCreate(TEST_CLASS_M, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_M, "TestClass_M");
   clCorClassAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_M, TEST_CLASS_M_ATTR_1, 2, 0 , 20);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_M,TEST_CLASS_M_ATTR_1, attrConfig);
   /* CONTAINMENT - C  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_2, TEST_CLASS_L, 0, 1);
   /* CONTAINMENT - E  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_3, TEST_CLASS_J, 0, 1);

   /* Base class - A (B, D)*/
   clCorClassCreate(TEST_CLASS_N, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_N, "TestClass_N");
   clCorClassAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_N, TEST_CLASS_N_ATTR_1, 1, 0 , 10);
   clCorClassAttributeUserFlagsSet(TEST_CLASS_N,TEST_CLASS_N_ATTR_1, attrConfig);
   /* CONTAINMENT - B  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_2, TEST_CLASS_M, 0, 1);
   /* CONTAINMENT - D  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_3, TEST_CLASS_K, 0, 2);

   clCorMoClassPathInitialize(moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_N);
   clCorMOClassCreate(moPath, 5);

   clCorTestClientDataSave();
   clCorMoClassPathFree(moPath);
}


/**
 * Test case to test the bug #5670. This check whether the MOId is 
 * getting modified after the call to the api clCorUtilMOMsoCreate().
 */

ClRcT 
cor_test_cor_mo_mso_create()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdPtrT pMoId1 = NULL;
    ClCorMOIdPtrT pMoId2 = NULL;
    ClCorObjectHandleT objH = {{0}};
    ClCharT             ch [CL_COR_MAX_NAME_SZ] = {0};
    ClInt32T            compResult = -3;

    clCorMoIdAlloc(&pMoId1);
    clCorMoIdAppend(pMoId1, TEST_CLASS_A, 5);

    ch[0] = '\0';
    sprintf(ch , "TEST :  [%s] ", __FUNCTION__); 
    CL_COR_TEST_PUT_IN_FILE(ch);
    rc = clCorObjectCreate(NULL, pMoId1, &objH);
    ch[0] = '\0';
    sprintf(ch , " \t TEST1 : Object creation ... \\AClass:5 . Result [%s] ", rc? "FAIL":"PASS"); 
    CL_COR_TEST_PUT_IN_FILE(ch);

    clCorMoIdAppend(pMoId1, TEST_CLASS_B, 4);
    
    rc = clCorObjectCreate(NULL, pMoId1, &objH);
    ch[0] = '\0';
    sprintf(ch , " \t TEST2 : Object creation ... \\AClass:5\\BClass:4 . Result [%s] ", rc? "FAIL":"PASS"); 
    CL_COR_TEST_PUT_IN_FILE(ch);

    clCorMoIdAppend(pMoId1, TEST_CLASS_C, 5);
    clCorMoIdClone(pMoId1, &pMoId2);
    
    rc = clCorUtilMoAndMSOCreate(pMoId1, &objH); 
    ch[0] = '\0';
    sprintf(ch , " \t TEST3 : Mo and Mso creation ... \\AClass:5\\BClass:4\\Cclass:5 . Result [%s] ", rc? "FAIL":"PASS"); 
    CL_COR_TEST_PUT_IN_FILE(ch);
    
    compResult = clCorMoIdCompare(pMoId1, pMoId2);
    ch[0] = '\0';
    sprintf(ch , " \t TEST4 : clCorUtilMOMsoCreate() test . Result [%s] ", (compResult == 0)? "PASS":"FAIL"); 
    CL_COR_TEST_PUT_IN_FILE(ch);

    return rc;
}



ClRcT classAttrWalk(ClCorClassTypeT clsId, ClCorAttrDefT *attrDef, ClPtrT cookie)
{
    ClCharT  ch[100] = {0};

    sprintf(ch , " \t\t ClassId [0x%x] , attrId [0x%x], attrType [%d], attrBasicType [%d] ", 
            clsId, attrDef->attrId, attrDef->attrType, attrDef->u.attrInfo.arrDataType);    
    fprintf(stdout, "%s \n", ch);
    CL_COR_TEST_PUT_IN_FILE(ch);
    ch[0] = '\0';
    sprintf(ch , "\t\t attrValues: init 0x%llx, min 0x%llx, max 0x%llx ", attrDef->u.simpleAttrVals.init, 
                            attrDef->u.simpleAttrVals.min, attrDef->u.simpleAttrVals.max);
    fprintf(stdout, "%s \n", ch);
    CL_COR_TEST_PUT_IN_FILE(ch);
    return CL_OK; 
}


ClRcT cor_test_class_AttrWalk()
{
    ClRcT rc = CL_OK; 
    ClUint32T i = 1;

    rc = clCorClassAttributeWalk(TEST_CLASS_A, classAttrWalk, &i);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x100.", rc); 

    rc = clCorClassAttributeWalk(TEST_CLASS_B, classAttrWalk, &i);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x200.", rc); 

    rc = clCorClassAttributeWalk(TEST_CLASS_C, classAttrWalk, &i);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x300.", rc); 

    rc = clCorClassAttributeWalk(TEST_CLASS_G, classAttrWalk, &i);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x9999.", rc); 
    return rc;
}

ClRcT
cor_test_service_rule_add()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdPtrT      moIdEvent;

    ClIocPhysicalAddressT  iocAdd = { clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};
    ClIocPhysicalAddressT  iocAdd1 = { CL_COR_TEST_IOC_ADDRESS_1 , CL_COR_TEST_IOC_PORT_1};
    ClIocPhysicalAddressT  iocAdd2 = { CL_COR_TEST_IOC_ADDRESS_2 , CL_COR_TEST_IOC_PORT_1};

    rc = clCorMoIdAlloc(&moIdEvent);

    clOsalPrintf("\nCalling service Rule Add \n");

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 3);
    
    /*Adding the service Rule : \TEST_CLASS_N:3 */
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    if(CL_OK != rc)
        clOsalPrintf(" TEST CASE 1: [FAILED]. rc[0x%x]\n", rc);
    else
       clOsalPrintf(" TEST CASE 1: [PASSED] \n"); 

    /* Adding first primary OI: \TEST_CLASS_N:3 */
    rc = clCorPrimaryOISet(moIdEvent, &iocAdd);
    if(CL_OK == rc)
        clOsalPrintf(" TEST CASE 2: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 2: [FAILED] \n"); 
    
    /* Adding second primary OI: \TEST_CLASS_N:3 */
    rc = clCorPrimaryOISet(moIdEvent, &iocAdd1);
    if(CL_OK != rc)
       clOsalPrintf(" TEST CASE 3: [PASSED]. rc[0x%x]\n", rc);
    else
       clOsalPrintf(" TEST CASE 3: [FAILED] \n"); 
        
    /* MoId becomes : \TEST_CLASS_N:3\TEST_CLASS_N:1*/
    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 1);

    /* Adding the service rule for : \TEST_CLASS_N:3\TEST_CLASS_N:1 */
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 4: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 4: [FAILED] \n"); 

    /* MoId becomes : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2 */
    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_M, 2);

    /* Adding the service rule for MOId: \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2 */
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 5: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 5: [FAILED] \n"); 


    /* MoId becomes : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_L, 3);

    /* Adding the service rule : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd1);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 6: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 6: [FAILED] \n"); 

    
    rc = clCorPrimaryOISet(moIdEvent, &iocAdd);
    if(CL_OK != rc)
       clOsalPrintf(" TEST CASE 7: [PASSED] . rc[0x%x]\n", rc);
    else
       clOsalPrintf(" TEST CASE 7: [FAILED] \n"); 

    /* Primary OI set : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorPrimaryOISet(moIdEvent, &iocAdd1);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 8: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 8: [FAILED] \n"); 

    /* Primary OI clear : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorPrimaryOIClear(moIdEvent, &iocAdd1);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 9: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 9: [FAILED] \n"); 

    /* Service rule add : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd2); 
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 10: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 10: [FAILED] \n"); 

    /* Primary OI Set : \TEST_CLASS_N:3\TEST_CLASS_N:1\TEST_CLASS_M:2\TEST_CLASS_L:3 */
    rc = clCorPrimaryOISet(moIdEvent, &iocAdd2);
    if(CL_OK == rc)
       clOsalPrintf(" TEST CASE 11: [PASSED]\n");
    else
       clOsalPrintf(" TEST CASE 11: [FAILED] \n"); 

    clCorMoIdFree(moIdEvent);
    
    return rc;
}

ClRcT
cor_test_service_rule_status_set()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdPtrT      moIdEvent;
    ClIocPhysicalAddressT  iocAdd = { clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};

    rc = clCorMoIdAlloc(&moIdEvent);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCalling service Rule Add \n"));

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 3);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Append MoId.", rc);

    /* Set the service rule to 0 - Disable */
    rc = clCorServiceRuleDisable(moIdEvent, iocAdd);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Set the service rule status to 0. for moId: \0x10001:3 .", rc);

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Append MoId.", rc);
    /* Set the service rule to 1 - Enable */
    rc = clCorServiceRuleDelete(moIdEvent, iocAdd);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Set the service rule status to 1. for moId: \0x10001:3 .", rc);

    rc = clCorMoIdFree(moIdEvent);
    
    return rc;
}

void cor_test_route_list()
{
    ClRcT              rc = CL_OK; 
    
    rc = cor_test_service_rule_add();
    if(rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("cor_test_service_rule_add failed. rc[0x%x]", rc));

    rc = cor_test_service_rule_status_set();
    if(rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("cor_test_service_rule_status_set failed. rc[0x%x]", rc));

}

void cor_test_txnEventIM1()
{
   ClCorMOIdPtrT       moId;
   ClCorTxnIdT         tid = 0;
   ClCorObjectHandleT  handle;
   ClCorServiceIdT     serviceId;
   ClCorMOIdPtrT      moIdEvent;
   ClCorAttrPathPtrT  attrPathEvent;
   ClCorOpsT          op = CL_COR_OP_ALL;
   ClRcT              rc = CL_OK;
   ClCharT            ch[150] = {0};

   ClIocPhysicalAddressT  iocAdd = { clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};

   clCorMoIdAlloc(&moIdEvent);
   clCorMoIdAppend(moIdEvent, CL_COR_CLASS_WILD_CARD, CL_COR_INSTANCE_WILD_CARD);
   clCorMoIdAppend(moIdEvent, CL_COR_CLASS_WILD_CARD, CL_COR_INSTANCE_WILD_CARD);
   clCorMoIdAppend(moIdEvent, CL_COR_CLASS_WILD_CARD, CL_COR_INSTANCE_WILD_CARD);
   clCorMoIdAppend(moIdEvent, CL_COR_CLASS_WILD_CARD, CL_COR_INSTANCE_WILD_CARD);
   clCorMoIdAppend(moIdEvent, CL_COR_CLASS_WILD_CARD, CL_COR_INSTANCE_WILD_CARD);
   clCorMoIdServiceSet(moIdEvent, CL_COR_SVC_WILD_CARD);

   clCorAttrPathAlloc(&attrPathEvent);
   clCorAttrPathAppend(attrPathEvent, CL_COR_ATTR_WILD_CARD, CL_COR_INDEX_WILD_CARD);
   clCorAttrPathAppend(attrPathEvent, CL_COR_ATTR_WILD_CARD, CL_COR_INDEX_WILD_CARD);
   clCorAttrPathAppend(attrPathEvent, CL_COR_ATTR_WILD_CARD, CL_COR_INDEX_WILD_CARD);
   clCorAttrPathAppend(attrPathEvent, CL_COR_ATTR_WILD_CARD, CL_COR_INDEX_WILD_CARD);
   clCorAttrPathAppend(attrPathEvent, CL_COR_ATTR_WILD_CARD, CL_COR_INDEX_WILD_CARD);


   /* Subscribe for a event for everything. */
   rc = clCorEventSubscribe(corTestEventChannelHandle,
                                 moIdEvent,
                                    attrPathEvent,
                                       NULL, op, NULL, generateSubscriptionId()); 
   ch[0] ='\0';
   sprintf(ch , "Cor Event Subcription - moId \\ *: * \\ *:* \\ *:* \\ *: * \\ *: * and attrPath \\ *: * \\ *: * \\*:* \\ *: * \\ *:* for any Operation. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* PARTICIPATE IN EVERY TRANSACTION */
   rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
   ch[0] ='\0';
   sprintf(ch , "Adding route list for moId : \\ *:* \\ *:* \\ *:* \\ *:* \\ *:* - to make sure that it is part of every transaction. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
 /* TC1 : Multiple Create */

   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] = '\0';
   sprintf(ch , " MO Create - moId \\0x100:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   

   clCorMoIdAppend(moId, TEST_CLASS_B, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] = '\0';
   sprintf(ch , " MO Create - moId \\0x100:2\\0x200:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   

   clCorMoIdAppend(moId, TEST_CLASS_C, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] = '\0';
   sprintf(ch ," MO Create - moId \\0x100:2\\0x200:1\\0x300:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   
 
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] = '\0';
   sprintf(ch , " MO Create - moId \\0x100:2\\0x200:1\\0x300:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   

   /* MSO creation */
   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] = '\0';
   sprintf(ch , " MSO Create - moId \\0x100:2\\0x200:1\\0x300:2 and service Id: CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   

   rc = clCorTxnSessionCommit(tid);
   ch[0] = '\0';
   sprintf(ch , " Committing all the previous transaction. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);   

   clCorMoIdInitialize(moId);
   clCorObjectHandleToMoIdGet(handle, moId, &serviceId);

 /* TC2 : Mutiple Sets */
   ClUint8T   value8  = 0;
   ClUint16T  value16 = 0;
   ClUint32T  value32 = 0;
   ClUint64T  value64 = 0;

   ClUint32T  size16  = sizeof(ClUint16T);
   ClUint32T  size32  = sizeof(ClUint32T);
   ClUint32T  size64  = sizeof(ClUint64T);
   ClUint32T  size8   = sizeof(ClUint8T);

   ClCorAttrPathPtrT pAttrPath;
   clCorAttrPathAlloc(&pAttrPath);

   tid = 0;
   value64 = 1;
   rc = clCorObjectAttributeSet(&tid, handle, NULL, TEST_CLASS_C_ATTR_1, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x301..value: %lld, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath NULL. rc[0x%x] ", value64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value64 = 2;
   rc = clCorObjectAttributeSet(&tid, handle, NULL, TEST_CLASS_C_ATTR_2, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x302..value: %lld, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath NULL. rc[0x%x] ", value64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 1*/
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_C_ATTR_3, 0);
   value32 = 3;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:0. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 4;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:0. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 2*/
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 1);
   value32 = 5;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:1 rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 6;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:1. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 3 */
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 2);
   value32 = 7;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:2 rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 8;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:2\\0x200:1\\0x300:1, attrPath \\0x303:2. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 4 */
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_B_ATTR_3, 0);
   value8 = 9;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x101..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value8 = 10;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x102..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 5 */
   clCorAttrPathSet(pAttrPath, 2, TEST_CLASS_B_ATTR_3, 1);
   value32 = 11;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_4, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x104..value: %d,MoId \\0x100:2\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:1. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value64 = 12;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_8, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x108..value: %lld,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:1. rc[0x%x] ",value64,rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   ClUint64T  arrayValue[] = {13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_3, 0, (void *)arrayValue, sizeof(arrayValue));
   ch[0] ='\0';
   sprintf(ch , " Value [13, 14, 15, 16, 17, 18, 19, 20, 21, 22] ");
   CL_COR_TEST_PUT_IN_FILE(ch);
   ch[0] ='\0';
   sprintf(ch , "Setting Array Attr in Bulk, attrid 0x103..MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
 
   value32 = 23;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_5, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x105..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   value16 = 24;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_6, -1, (void *)&value16, size16);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x106..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value16, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   value64 = 25;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_9, -1, (void *)&value64, size64);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x109..value: %lld,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ",value64,rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   value8 = 12;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x102..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   value8 = 12;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x101..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   rc = clCorTxnSessionCommit(tid);
   ch[0] ='\0';
   sprintf(ch , "\tCommitting all the earlier transaction. rc[0x%x]. Result[%s]",rc, (rc?"FAIL":"PASS"));
   CL_COR_TEST_PUT_IN_FILE(ch);

/* CLI-commands for verifying the Test Cases */

/*
setc 1
setc corServer_SysController_0
objtreeshow
objectShow \0x100:2\0x200:1\0x300:2
objectShow \0x100:2\0x200:1\0x300:2 4 \0x303:0
objectShow \0x100:2\0x200:1\0x300:2 4 \0x303:1
objectShow \0x100:2\0x200:1\0x300:2 4 \0x303:2
objectShow \0x100:2\0x200:1\0x300:2 4 \0x303:2\0x203:0
objectShow \0x100:2\0x200:1\0x300:2 4 \0x303:2\0x203:1

*/
   
}



/**
 * Test function to test the limit enforced on the object size by the
 * the attribute offset field.
 */ 
ClRcT 
cor_test_attr_offset_limit_check()
{
    ClRcT   rc = CL_OK;
    ClUint32T   val = 0;
    ClUint32T   size = 0;
    ClCorMOIdT  moId ;
    ClCorObjectHandleT objH = {{0}};

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, 3);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s] TC -1 FAILED . rc[0x%x]\n", __FUNCTION__, rc);
            return rc;
        }
        else 
            clOsalPrintf("[%s] TC -1 PASSED \n", __FUNCTION__);
    }


    val = 32;
    size = sizeof(ClUint32T);
    rc = clCorObjectAttributeSet(NULL, objH, NULL, TEST_CLASS_A_ATTR_10, 4, &val, size);
    if(CL_OK !=rc )
    {
        clOsalPrintf("[%s] TC -2 : FAILED  . rc[0x%x]\n", __FUNCTION__, rc);
        return rc;
    }
    else 
        clOsalPrintf("[%s] TC -2 PASSED \n", __FUNCTION__);

    val = 0;

    rc = clCorObjectAttributeGet(objH, NULL, TEST_CLASS_A_ATTR_10, 4, &val, &size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s] TC -3(a) : FAILED  . rc[0x%x]\n", __FUNCTION__, rc);
        return rc;
    }
    else 
    {
        clOsalPrintf("[%s] TC -3(a) PASSED \n", __FUNCTION__);
        if(val == 32)
        {
            clOsalPrintf("[%s] TC - 3 (b) PASSED \n", __FUNCTION__);
        }
        else
            clOsalPrintf("[%s] TC - 3(b) FAILED \n", __FUNCTION__);
    }

    return rc;
}


/**
 * Test function to test Bug #5873
 * Function to create and delete the object.
 * This test case can be run from different nodes simultaneously but
 * for running it the instance id can be changed accordingly.
 */
ClRcT
cor_test_object_create_delete()
{
    ClRcT           rc = CL_OK;
    ClCorMOIdT      moId;
    ClCorObjectHandleT objH = {{0}};
    ClCharT             ch[CL_COR_MAX_NAME_SZ] = {0};
    ClUint32T            inst = 0, i = 0;

    clCorMoIdInitialize(&moId);

    /***********************************************************
      FOR RUNNING THIS TEST FUNCTION FROM DIFFERENT NODE THIS 
      VARIABLE SHOULD BE MADE UNIQUE ACCROSS ALL THE NODES.
    ************************************************************/
    inst = 0;
    

    clCorMoIdAppend(&moId, TEST_CLASS_E, inst);
    for (i = 0; i< 90000; i++)
    {

        clCorMoIdInstanceSet(&moId, 1, i);

#if 0
        rc = clCorObjectHandleGet(&moId, &objH);
        if(rc != CL_OK)
        {
#endif
        rc = clCorObjectCreate(NULL, &moId, &objH);
        ch[0] ='\0';
        sprintf(ch , "Creating the object \\BClass:%d. rc [0x%x] Result [%s] ",inst, rc, rc?"FAIL":"PASS");
        CL_COR_TEST_PUT_IN_FILE(ch);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]",rc));
            return rc;
        }
#if 0
        }
#endif
        clOsalPrintf("[%s] ################### Completed iterations [%d] #########\n", __FUNCTION__, i);

#if 0
        rc = clCorObjectDelete(NULL, objH);
        ch[0] ='\0';
        sprintf(ch , "Deleting the object \\BClass:%d. rc [0x%x] Result [%s] ",inst, rc, rc?"FAIL":"PASS");
        CL_COR_TEST_PUT_IN_FILE(ch);
        if(rc != CL_OK)
            return rc;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Create and delete ..... [%d] ", i));
#endif
    }
    
    ch[0] ='\0';
    sprintf(ch , "[%s] Result [PASS] ", __FUNCTION__);
    CL_COR_TEST_PUT_IN_FILE(ch);

    return rc;
}

/**
 * Test function to test Bug #5873
 * Function to set an attribute of the object multiple times.
 */
ClRcT
cor_test_set_attribute()
{
    ClRcT           rc = CL_OK;
    ClCorMOIdT      moId;
    ClCorObjectHandleT objH = {{0}};
    ClCharT             ch[CL_COR_MAX_NAME_SZ] = {0};
    ClCorTxnSessionIdT  tid = 0;
    ClUint8T   value8  = 0;
    ClUint16T  value16 = 0;
    ClUint32T  value32 = 0, i = 0;
    ClUint64T  value64 = 0;

    ClUint32T  size16  = sizeof(ClUint16T);
    ClUint32T  size32  = sizeof(ClUint32T);
    ClUint32T  size64  = sizeof(ClUint64T);
    ClUint32T  size8   = sizeof(ClUint8T);

    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, TEST_CLASS_A, 6);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(rc != CL_OK)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        ch[0] ='\0';
        sprintf(ch , "Creating the object \\AClass:6. rc [0x%x] Result [%s] ",rc, rc?"FAIL":"PASS");
        CL_COR_TEST_PUT_IN_FILE(ch);
        if(rc != CL_OK)
            return rc;
    }

    for(i = 0; i < 10000; i++)
    {
        tid = 0;
        value32 = 23;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_5, -1, (void *)&value32, size32);

        value16 = 24;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_6, -1, (void *)&value16, size16);
    
        value64 = 25;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_9, -1, (void *)&value64, size64);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, size8);

        rc = clCorTxnSessionCommit(tid);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Committing the transactions [%d] ... rc[0x%x]", i, rc));
        ch[0] ='\0';
        sprintf(ch , "Committing the session. time [%d],. rc[0x%x], Result[%s] ", i, rc, rc?"FAIL":"PASS");
        CL_COR_TEST_PUT_IN_FILE(ch);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the txn commit ... rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
        }
    }

    rc = clCorObjectDelete(NULL, objH);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the object. rc[0x%x]",rc));
        return rc;
    }
    ch[0] ='\0';
    sprintf(ch , " [%s]. Result[PASS] ", __FUNCTION__);
    CL_COR_TEST_PUT_IN_FILE(ch);
    return rc;
}


void cor_test_Get()
{
   ClCorMOIdPtrT       moId;
   ClCorAttrPathPtrT   pAttrPath;
   ClCorObjectHandleT  handle;
   ClRcT               rc = CL_OK;

 /* Get the handle first  */
   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 2);
   clCorMoIdAppend(moId, TEST_CLASS_B, 1);
   clCorMoIdAppend(moId, TEST_CLASS_C, 2);
   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
 
   clCorObjectHandleGet(moId, &handle);

 /* GET the values  */
   ClUint8T   value8  = 0;
   ClUint16T  value16 = 0;
   ClUint32T  value32 = 0;
   ClUint64T  value64 = 0;
   ClCharT    ch[100] = {0};

   ClUint32T  size8  = sizeof(ClUint8T);
   ClUint32T  size16  = sizeof(ClUint16T);
   ClUint32T  size32  = sizeof(ClUint32T);
   ClUint32T  size64  = sizeof(ClUint64T);


   rc = clCorObjectAttributeGet(handle, NULL, TEST_CLASS_C_ATTR_1, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x301 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet(handle, NULL, TEST_CLASS_C_ATTR_2, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch ,"For attrId 0x302 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 1*/
   clCorAttrPathAlloc(&pAttrPath);
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_C_ATTR_3, 0);
   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, " For attrId 0x201 Value obtained is %d, size  %d, rc [0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, " For attrId 0x202 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 2*/
   size32 = sizeof(ClUint32T);
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 1);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x201 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x202 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 3 */
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 2);
   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch , "For attrId 0x201 Value obtained is %d, size  %d, rc [0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);


   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, " For attrId 0x202 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 4 */
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_B_ATTR_3, 0);
   size8 = sizeof(ClUint8T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, &size8);
   ch[0] ='\0';
   sprintf(ch ," For attrId 0x101 Value obtained is %d, size  %d, rc[0x%x]", value8, size8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);


   size8 = sizeof(ClUint8T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, &size8);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x102 Value obtained is %d, size  %d, rc[0x%x]", value8, size8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 5 */
   clCorAttrPathSet(pAttrPath, 2, TEST_CLASS_B_ATTR_3, 1);
   size8 = sizeof(ClUint8T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, &size8);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x101 Value obtained is %d, size  %d, rc[0x%x]", value8, size8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size8 = sizeof(ClUint8T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, &size8);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x102 Value obtained is %d, size  %d, rc[0x%x]", value8, size8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   size64 = sizeof(ClUint64T);

   ClUint64T  arrayValue[11] = {0};
   ClUint32T size = sizeof(arrayValue);
   rc = clCorObjectAttributeGet(handle, pAttrPath, TEST_CLASS_A_ATTR_3, -1, (void *)arrayValue, &size);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x103 value Obtained with rc [0x%x] -  ", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   
   ClInt32T i = 0;
   while(i < 10)
   {
     ch[0] ='\0';
     sprintf(ch ,"%lld, ", arrayValue[i++]);
     CL_COR_TEST_PUT_IN_FILE(ch);
   }
   
   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_4, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x104 Value obtained is %d, size  %d, rc [0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_5, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x105 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size16 = sizeof(ClUint16T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_6, -1, (void *)&value16, &size16);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x106 Value obtained is %d, size  %d, rc[0x%x]", value16, size16, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size16 = sizeof(ClUint16T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_7, -1, (void *)&value16, &size16);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x107 Value obtained is %d, size  %d, rc[0x%x]", value16, size16, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_8, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x108 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_9, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x109 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   if(rc == CL_OK)
   {
        ch[0] ='\0';
        sprintf(ch, "\tAttribute Get Test, rc[0x%x], Result [%s]", rc, rc?"FAIL":"PASS");
        CL_COR_TEST_PUT_IN_FILE(ch);
   }
}

ClRcT cor_test_ObjWalkFp( void *data, void * cookie)
{
   ClCorMOIdT           moId;
   ClCorServiceIdT      svcId;
   ClCorObjectHandleT   objHdl = *((ClCorObjectHandleT *)data);

   clCorObjectHandleToMoIdGet(objHdl, &moId, &svcId);
   clCorMoIdShow(&moId);
  return (CL_OK);
}



void cor_test_TreeWalkIM1()
{
   ClCorMOIdPtrT       moId;
   ClCorTxnIdT         tid = 0;
   ClCorObjectHandleT  handle;
   ClRcT               rc = CL_OK;
   ClCharT             ch[100] = {0};
   ClRcT                err = 0;

   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_B, 0);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:0. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_C, 0);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:0\\0x300:0. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:0\\0x300:0 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
 
   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:0\\0x300:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   
   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:0\\0x300:1 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:0\\0x300:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:0\\0x300:2 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:0\\0x300:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:0\\0x300:3 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdTruncate(moId, 2);
   clCorMoIdSet(moId, 2, TEST_CLASS_B, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 2, TEST_CLASS_B, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 2, TEST_CLASS_B, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_C, 0);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:0. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:3\\0x300:0 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
 
   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   
   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:3\\0x300:1 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_SVC_ID_DUMMY_MANAGEMENT);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MSO \\0x100:3\\0x200:3\\0x300:2 with service Id. CL_COR_SVC_ID_DUMMY_MANAGEMENT. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdServiceSet(moId, CL_COR_INVALID_SRVC_ID);
   clCorMoIdSet(moId, 3, TEST_CLASS_C, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_D, 0);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x400:0. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_D, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x400:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_D, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x400:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_D, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x400:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_E, 0);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x500:0. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_E, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x500:1. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_E, 2);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x500:2. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdSet(moId, 4, TEST_CLASS_E, 3);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " Creating the MO \\0x100:3\\0x200:3\\0x300:3\\0x500:3. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   rc = clCorTxnSessionCommit(tid);
   ch[0] ='\0';
   sprintf(ch , " Committing all the previous transactions. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

 /* Let us walk the object Tree */

   clOsalPrintf("\n\t ########################### TREE WALK ################################## \n");   

    /* TC1 - Walk all the MOs */
    clOsalPrintf("\n\t\t ***** Walk Whole Tree with MO WALK ******\n");   
    rc = clCorObjectWalk(NULL, NULL, cor_test_ObjWalkFp, CL_COR_MO_WALK, NULL);
    ch[0] ='\0';
    sprintf(ch , " MO Walk in case of No root, No Filter. rc[0x%x]\n", rc);
    CL_COR_TEST_PUT_IN_FILE(ch);
    err |=rc;
    /* TC2 - Walk all the MSOs */
    clOsalPrintf("\n\t\t ***** Walk Whole Tree with MSO WALK ******\n");   
    rc = clCorObjectWalk(NULL, NULL, cor_test_ObjWalkFp, CL_COR_MSO_WALK, NULL);
    ch[0] ='\0';
    sprintf(ch , " MSO Walk in case of No root, No Filter. rc[0x%x]\n", rc);
    CL_COR_TEST_PUT_IN_FILE(ch);
    err |=rc;

    /* TC3 - Walk all the MOs in subtree */
    clOsalPrintf("\n\t\t ***** Walk subtree with MO WALK ******\n");   
    clCorMoIdInitialize(moId);
    clCorMoIdAppend(moId, TEST_CLASS_A, 3);
    rc = clCorObjectWalk(moId, NULL, cor_test_ObjWalkFp, CL_COR_MO_WALK, NULL);
    ch[0] ='\0';
    sprintf(ch , " MO Walk in case of Root \\0x100:3, No Filter. rc[0x%x]\n", rc);
    CL_COR_TEST_PUT_IN_FILE(ch);
    err |=rc;

    /* TC4 - Walk all the MSOs in subtree */
    clOsalPrintf("\n\t\t ***** Walk subtree with MSO WALK ******\n");   
    rc = clCorObjectWalk(moId, NULL, cor_test_ObjWalkFp, CL_COR_MSO_WALK, NULL);
    ch[0] ='\0';
    sprintf(ch , " MSO Walk in case of Root \\0x100:3, No Filter. rc[0x%x]\n", rc);
    CL_COR_TEST_PUT_IN_FILE(ch);
    err |=rc;

    ch[0] ='\0';
    sprintf(ch , " \n\tclCorObjectWalk. rc[0x%x]. Result[%s]\n", rc, (err?"FAIL":"PASS"));
    CL_COR_TEST_PUT_IN_FILE(ch);
    
    
}

void cor_test_txnIM1()
{
   ClCorMOIdPtrT       moId;
   ClCorTxnIdT         tid = 0;
   ClCorObjectHandleT  h1, h2, handle;
   ClCorServiceIdT     svcId;
   ClRcT               rc = CL_OK;
   ClCharT             ch[150] = {0};

 /* TC1 : Multiple Create */
   clCorMoIdAlloc(&moId);
   clCorMoIdAppend(moId, TEST_CLASS_A, 1);
   rc = clCorObjectCreate( &tid, moId, &h1);
   ch[0] ='\0';
   sprintf(ch , " MO Created with moId - \\0x100:3, . rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_B, 1);
   rc = clCorObjectCreate(&tid, moId, &h2);
   ch[0] ='\0';
   sprintf(ch , " MO Created with moId - \\0x100:3\\0x200:1, . rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdAppend(moId, TEST_CLASS_C, 1);
   rc = clCorObjectCreate(&tid, moId, &handle);
   ch[0] ='\0';
   sprintf(ch , " MO Created with moId - \\0x100:3\\0x200:1\\0x300:1, . rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   rc = clCorTxnSessionCommit(tid); 
   ch[0] ='\0';
   sprintf(ch , " Committing the transaction. rc[0x%x]", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   clCorMoIdInitialize(moId);
   rc = clCorObjectHandleToMoIdGet(h1, moId, &svcId);
   ch[0] ='\0';
   sprintf(ch , " Converting the handle back to MOId ..  \\0x100:1. rc[0x%x] ", rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   clCorMoIdShow(moId);

   clCorMoIdInitialize(moId);
   rc = clCorObjectHandleToMoIdGet(h2, moId, &svcId);
   ch[0] ='\0';
   sprintf(ch , " Converting the handle back to MOId ..  \\0x100:1\\0x200:1 ");
   CL_COR_TEST_PUT_IN_FILE(ch);
   clCorMoIdShow(moId);

   clCorMoIdInitialize(moId);
   rc = clCorObjectHandleToMoIdGet(handle, moId, &svcId);
   ch[0] ='\0';
   sprintf(ch , " Converting the handle back to MOId ..  \\0x100:1\\0x200:1\\0x300:1 ");
   CL_COR_TEST_PUT_IN_FILE(ch);
   clCorMoIdShow(moId);
  
 /* TC2 : Mutiple Sets */
   ClUint8T   value8  = 0;
   ClUint32T  value32 = 0;

   ClUint32T  size8   = sizeof(ClUint8T);
   ClUint32T  size32  = sizeof(ClUint32T);

   ClCorAttrPathPtrT pAttrPath;
   clCorAttrPathAlloc(&pAttrPath);

   tid = 0;
   value32 = 1;
   rc = clCorObjectAttributeSet(&tid, handle, NULL, TEST_CLASS_C_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x301..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath NULL. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   value32 = 2;
   rc = clCorObjectAttributeSet(&tid, handle, NULL, TEST_CLASS_C_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x302..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath NULL. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 1*/
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_C_ATTR_3, 0);
   value32 = 3;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:0. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 4;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:0. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 2*/
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 1);
   value32 = 5;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:1 rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 6;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:1. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 3 */
   clCorAttrPathSet(pAttrPath, 1, TEST_CLASS_C_ATTR_3, 2);
   value32 = 7;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_1, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x201..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:2 rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value32 = 8;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_B_ATTR_2, -1, (void *)&value32, size32);
   ch[0] ='\0';
   sprintf(ch , " Setting the attrid 0x202..value: %d, MoId \\0x100:1\\0x200:1\\0x300:1, attrPath \\0x303:2. rc[0x%x] ", value32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 4 */
   clCorAttrPathAppend(pAttrPath, TEST_CLASS_B_ATTR_3, 0);
   value8 = 9;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x101..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value8 = 10;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x102..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:0. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 5 */
   clCorAttrPathSet(pAttrPath, 2, TEST_CLASS_B_ATTR_3, 1);
   value8 = 11;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x101..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:1. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   value8 = 12;
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);
   ch[0] ='\0';
   sprintf(ch , "Setting the attrid 0x102..value: %d,MoId \\0x100:1\\0x200:1\\0x300:1,attrPath \\0x303:2\\0x203:1. rc[0x%x] ", value8, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   rc = clCorTxnSessionCommit(tid);
   ch[0] ='\0';
   sprintf(ch , "\t Committing the transaction. rc[0x%x]. Result [%s]", rc, rc?"FAIL":"PASS");
   CL_COR_TEST_PUT_IN_FILE(ch);


/* CLI-commands for verifying the Test Cases */

/*
setc 1
setc corServer_SysController_0
objtreeshow
RESULT : The following subtree should be present.
    [1] ClassName=Aclass ClassId=0x0100 inst=0 [flgs=0x508]
        [1] ClassName=Bclass ClassId=0x0200 inst=0 [flgs=0x508]
            [1] ClassName=Cclass ClassId=0x0300 inst=0 [flgs=0x508]

->objectShow \0x100:1\0x200:1\0x300:1
RESULT:
[class:0x0300] Object { C '0x301' = 1, Cc '0x302' = 2, Ccc '0x303'[10] => "Contained Class 0x0200",  }

->objectShow \0x100:1\0x200:1\0x300:1 -1 \0x303:0
RESULT:
[class:0x0200] Object { B '0x201' = 3, Bb '0x202' = 4, Bbb '0x203'[5] => "Contained Class 0x0100",  }

->objectShow \0x100:1\0x200:1\0x300:1 -1 \0x303:1
RESULT:
[class:0x0200] Object { B '0x201' = 5, Bb '0x202' = 6, Bbb '0x203'[5] => "Contained Class 0x0100",  }

->objectShow \0x100:1\0x200:1\0x300:1 -1 \0x303:2
RESULT:
[class:0x0200] Object { B '0x201' = 7, Bb '0x202' = 8, Bbb '0x203'[5] => "Contained Class 0x0100",  }

->objectShow \0x100:1\0x200:1\0x300:1 -1 \0x303:2\0x203:0
RESULT:
 [class:0x0100] Object { A '0x101' = 9, Aa '0x102' = 10, Aaa '0x103'[2] => 0, 0, Aaaa '0x104' = 10, Aaaaa '0x105' = 65535, uint64 '0x106' = 65535, Int64 '0x107' = 65535,  }

->objectShow \0x100:1\0x200:1\0x300:1 -1 \0x303:2\0x203:1
RESULT:
 [class:0x0100] Object { A '0x101' = 11, Aa '0x102' = 12, Aaa '0x103'[2] => 0, 0, Aaaa '0x104' = 10, Aaaaa '0x105' = 65535,
uint64 '0x106' = 65535, Int64 '0x107' = 65535,  }

*/
   
}


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


ClRcT
cor_test_Association ()
{
    /* These are the standard declarations */
    ClInt32T stepCheck = PASSTEST;
    ClRcT rc = CL_OK;
    ClCharT ch[150];

    /* These are the API test specific declarations */
    ClCorMOIdPtrT testMOId_H;
    ClCorObjectHandleT testCORObj_H1, testCORObj_H2, testCORObj_H3;
    ClCorObjectHandleT assocObj_H;
    ClInt32T testValue, testValue1;
    ClUint32T size = sizeof (testValue);

    ClCorMOClassPathPtrT testMOPath_H;
    /* write the test number in the File*/

    /* These are the pre-condition operations specific to this API */

    /* Class Definition */

    rc = clCorClassCreate (0x6700, 0x0000);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Created class 0x6700", rc);
    clCorClassNameSet(0x6700, "Assoc_class_1");
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Name set for the class 0x6700", rc);
    rc = clCorClassAttributeCreate (0x6700, 700, CL_COR_INT32);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"CorClassAttributeCreate class: 0x6700, attrId:700", rc);
    rc = clCorClassAttributeValueSet (0x6700, 700, 100, 0, 999999);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttributeValueSet class: 0x6700, attrId:700", rc);

    rc = clCorClassCreate (0x6701, 0x0000);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Created class 0x6701", rc);
    clCorClassNameSet(0x6701, "Assoc_class_2");
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Name set for the class 0x6700", rc);
    rc = clCorClassAttributeCreate (0x6701, 701, CL_COR_INT32);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"CorClassAttributeCreate class: 0x6701, attrId:701", rc);
    rc = clCorClassAttributeValueSet (0x6701, 701, 100, 0, 999999);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttributeValueSet class: 0x6701, attrId:701", rc);

    rc = clCorClassCreate (0x6702, 0x0000);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Created class 0x6702", rc);
    clCorClassNameSet(0x6702, "Assoc_class_3");
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Name set for the class 0x6700", rc);
    rc = clCorClassAttributeCreate (0x6702, 702, CL_COR_INT32);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"CorClassAttributeCreate class: 0x6702, attrId:702", rc);
    rc = clCorClassAttributeValueSet (0x6702, 702, 100, 0, 999999);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttributeValueSet class: 0x6702, attrId:702", rc);


    /*Step 1 */
    ch[0]='\0';
    sprintf(ch , "\tStep 1 - %s RC=0x%x\t: %s",
	        (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	        "Create three classes A,B and C");
    CL_COR_TEST_PUT_IN_FILE(ch);

    CL_COR_TEST_PRINT_ERROR("Pre-Condition Failure - Could not create the Classes", rc);

    /*Associate the class */
    rc = clCorClassAssociationCreate (0x6700, 6701, 0x6701, 5);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAssociationCreate failed. classId: 0x6700, attrId:6701, associated to 0x6701", rc);
    rc = clCorClassAssociationCreate (0x6701, 6702, 0x6702, 5);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAssociationCreate failed. classId: 0x6701, attrId:6702, associated to 0x6702", rc);
    rc = clCorClassAssociationCreate (0x6702, 6700, 0x6700, 5);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAssociationCreate failed. classId: 0x6702, attrId:6700, associated to 0x6700", rc);

    /*Step 1 */
    ch[0] = '\0';
    sprintf(ch , " \tStep 2 - %s RC=0x%x\t: %s",
	        (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	        "Set the association , such that Class B is Associated to Class A, C is associated with B and A is associated with C ( Cyclic association) ");

    CL_COR_TEST_PUT_IN_FILE(ch);

    if (rc != CL_OK)
    {
        clOsalPrintf ("\n Pre-Condition Failure - Could not define the containment relationship.\n");
        return rc;
    }

    /* MO Tree Definition */

    rc = clCorMoClassPathAlloc (&testMOPath_H);
    rc = clCorMoClassPathAppend (testMOPath_H, 0x6700);
    rc = clCorMOClassCreate (testMOPath_H, 5);
    rc = clCorMoClassPathAppend (testMOPath_H, 0x6701);
    rc = clCorMOClassCreate (testMOPath_H, 5);
    rc = clCorMoClassPathAppend (testMOPath_H, 0x6702);
    rc = clCorMOClassCreate (testMOPath_H, 5);

    /*Step 3 */
    ch[0] = '\0';
    sprintf (ch, "\tStep 3 - %s RC=0x%x\t: %s",
	    (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	    "Create three MO Classes for A , B and C. ");
    CL_COR_TEST_PUT_IN_FILE(ch);

    if (rc != CL_OK)
    {
        clOsalPrintf ("\n Pre-Condition Failure - Could not create the MO Classes in MO Tree \n");
        return rc;
    }

    /* Object Tree creation */

    rc = clCorMoIdAlloc (&testMOId_H);
    rc = clCorMoIdAppend (testMOId_H, 0x6700, 1);
    rc = clCorMoIdServiceSet (testMOId_H, CL_COR_INVALID_SVC_ID);
    rc = clCorObjectCreate (CL_COR_SIMPLE_TXN, testMOId_H, &testCORObj_H1);
    rc = clCorMoIdAppend (testMOId_H, 0x6701, 1);
    rc = clCorMoIdServiceSet (testMOId_H, CL_COR_INVALID_SVC_ID);
    rc = clCorObjectCreate (CL_COR_SIMPLE_TXN, testMOId_H, &testCORObj_H2);
    rc = clCorMoIdAppend (testMOId_H, 0x6702, 1);
    rc = clCorMoIdServiceSet (testMOId_H, CL_COR_INVALID_SVC_ID);
    rc = clCorObjectCreate (CL_COR_SIMPLE_TXN, testMOId_H, &testCORObj_H3);

    /*Step 4 */
    ch[0] = '\0';
    sprintf(ch , " \tStep 4 - %s RC=0x%x\t: %s",
	    (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	    "Create one object instance each of class A,B and C");
    CL_COR_TEST_PUT_IN_FILE(ch);

    if (rc != CL_OK)
    {
        clOsalPrintf ("\n Pre-Condition Failure - Could not Create the instances ...\n");
        return rc;
    }
    testValue = 258;
    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H1, NULL, 700, -1, (void *) &testValue, sizeof (testValue));
    ch[0]='\0';
	sprintf(ch , "\tThe attrid 700 is %d of class 0x6700 is set to value 258, rc 0x%x ", testValue1, rc);
    CL_COR_TEST_PUT_IN_FILE(ch);

    testValue = 259;
    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H2, NULL, 701, -1, (void *) &testValue, sizeof (testValue));
    ch[0] =  '\0';
	sprintf(ch , "\tThe attrid 701 is %d of class 0x6701 is set to value 259, rc 0x%x ", testValue1, rc);
    CL_COR_TEST_PUT_IN_FILE(ch);

    testValue = 260;
    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H3, NULL, 702, -1, (void *) &testValue, sizeof (testValue));
    ch[0] = '\0';
	sprintf(ch , "\tThe attrid 702 is %d of class 0x6702 is set to value 260, rc 0x%x ", testValue1, rc);
    CL_COR_TEST_PUT_IN_FILE(ch);

    /*Step 5*/
    ch[0] = '\0';
    sprintf(ch , " \tStep 5 - %s RC=0x%x\t: %s",
	    (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	    "Set some attirbute value (say 258, 259, 260) of Object A, B , C");
    CL_COR_TEST_PUT_IN_FILE(ch);


    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H1, NULL, 6701, 1, (void *)&testCORObj_H2, sizeof (ClCorObjectHandleT));
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Cor Association done for A->B ", rc);

    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H2, NULL, 6702, 1, (void *)&testCORObj_H3, sizeof (ClCorObjectHandleT));
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Cor Association done for B->C ", rc);

    rc = clCorObjectAttributeSet (CL_COR_SIMPLE_TXN, testCORObj_H3, NULL, 6700, 1, (void *)&testCORObj_H1, sizeof (ClCorObjectHandleT));
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Cor Association done for C->A ", rc);

    /*Step 6*/
    ch[0] = '\0';
    sprintf(ch , " \tStep 6 - %s RC=0x%x\t: %s",
	        (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	        "Now set the association of object A , B and C in cyclic order A->B->C->A ");
    CL_COR_TEST_PUT_IN_FILE(ch);

    size = sizeof (ClCorObjectHandleT);
    rc = clCorObjectAttributeGet (testCORObj_H1, NULL, 6701, 1, (void *)&assocObj_H, &size);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Getting the assoc attr 6701 associated to 0x6701 from the object of class 0x6700", rc);

    rc = clCorObjectAttributeGet (assocObj_H, NULL, 6702, 1, (void *)&assocObj_H, &size);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Getting the assoc attr 6702 associated to 0x6702 from the object of class 0x6701", rc);

    rc = clCorObjectAttributeGet (assocObj_H, NULL, 6700, 1, (void *)&assocObj_H, &size);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Getting the assoc attr 6700 associated to 0x6700 from the object of class 0x6702", rc);

    /*Step 7*/
    ch[0] = '\0';
    sprintf(ch , " \tStep 7 - %s RC=0x%x\t: %s",
	        (rc == CL_OK ? PASSTEST : (stepCheck = FAILTEST)) ? "PASS" : "FAIL", rc,
	        "Now get the handle of object A through B and C ");
    CL_COR_TEST_PUT_IN_FILE(ch);

    size = sizeof (ClUint32T);
	testValue1 = 0;
    /*Verification 1*/
    rc = clCorObjectAttributeGet (assocObj_H, NULL, 700, -1, (void *) &testValue1, &size);
    ch[0] = '\0';
	sprintf(ch , "\tThe value obtained by the attrid 700 is %d of class 0x6700, rc 0x%x ", testValue1, rc);
    CL_COR_TEST_PUT_IN_FILE(ch);

	testValue1 = 0;
    size = sizeof (ClCorObjectHandleT);
    rc = clCorObjectAttributeGet (testCORObj_H1, NULL, 6701, 1, (void *)&assocObj_H, &size);
    size = sizeof (ClUint32T);
    rc = clCorObjectAttributeGet (assocObj_H, NULL, 701, -1, (void *) &testValue1, &size);
    ch[0] = '\0';
	sprintf(ch , "\tThe value obtained by the attrid 701 is %d of class 0x6701, rc 0x%x ", testValue1, rc );
    CL_COR_TEST_PUT_IN_FILE(ch);

	testValue1 = 0;
    size = sizeof (ClCorObjectHandleT);
    rc = clCorObjectAttributeGet (testCORObj_H1, NULL, 6701, 1, (void *)&assocObj_H, &size);
    rc = clCorObjectAttributeGet (assocObj_H, NULL, 6702, 1, (void *)&testCORObj_H3, &size);
    size = sizeof (ClUint32T);
    rc = clCorObjectAttributeGet (testCORObj_H3, NULL, 702, -1, (void *) &testValue1, &size);
    ch[0] = '\0';
    sprintf(ch , "\tThe value obtained by the attrid 701 is %d of class 0x6701, rc 0x%x ", testValue1, rc );
    CL_COR_TEST_PUT_IN_FILE(ch);

    /*free*/
    clCorMoIdFree (testMOId_H);
    return CL_OK;
}

ClRcT cor_test_moId_nodeName_get()
{
    ClNameT nodeName, nodeName1;
    ClRcT   rc = CL_OK;
    ClCorMOIdT  moId;
    ClCharT     ch[100] = {0};
    
    rc = clCpmLocalNodeNameGet(&nodeName);   
    ch[0] = '\0';
    sprintf(ch , "\tThe Node name obtained from the CPM: %s. rc 0x%x ", nodeName.value, rc );
    CL_COR_TEST_PUT_IN_FILE(ch);
    
    rc = clCorNodeNameToMoIdGet(nodeName, &moId);
    if(rc == CL_OK) clCorMoIdShow(&moId);
    

    memset(&nodeName1, 0, sizeof(ClNameT));
    rc = clCorMoIdToNodeNameGet(&moId, &nodeName1);
    ch[0] = '\0';
    sprintf(ch , "\tFor the moId obtained the nodeName obtained: %s, len %d. rc 0x%x. Result[%s] ", nodeName1.value, nodeName1.length, rc, rc?"FAIL":"PASS" );
    CL_COR_TEST_PUT_IN_FILE(ch);
    
    clCorMoIdInstanceSet(&moId, 2, 2);
    memset(&nodeName1, 0, sizeof(ClNameT));
    rc = clCorMoIdToNodeNameGet(&moId, &nodeName1);
    ch[0] = '\0';
    sprintf(ch , "\tFor an invalid moId,the nodeName obtained: %s, len %d. rc 0x%x. Result[%s] ", 
                        nodeName1.value?nodeName1.value:NULL, nodeName1.length, rc, rc?"PASS":"FAIL");
    CL_COR_TEST_PUT_IN_FILE(ch);


    /* nodeName = { sizeof("SystemContRoller_0"), "SystemContRoller_0"}; */
    ClCharT name[] =  "SystemContRoller_0";
    memcpy(nodeName.value, name, sizeof(name));
    nodeName.length = sizeof("SystemContRoller_0");

    rc = clCorNodeNameToMoIdGet(nodeName, &moId);
    ch[0] = '\0';
    sprintf(ch , "\tFor an invalid Node Name. rc[0x%x]. Result[%s] ", rc , rc?"PASS":"FAIL");
    CL_COR_TEST_PUT_IN_FILE(ch);

    return rc;
}

/**
 * Test function to add the service rule..
 */

ClRcT cor_test_svc_rule_add()
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;
    ClIocPhysicalAddressT  iocAdd = { clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};

    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, 0x100, 0);

    clCorServiceRuleAdd(&moId, iocAdd);

    clCorMoIdAppend(&moId, 0x200, 0);

    clCorServiceRuleAdd(&moId, iocAdd);

    clCorMoIdAppend(&moId, 0x300, 0);
    clCorServiceRuleAdd(&moId, iocAdd);
    clCorPrimaryOISet(&moId, &iocAdd);

    return rc;
}


/************************************************************************************
    Helper functions for object locking test cases
***********************************************************************************/
ClRcT _clCorTestObjCreate(ClUint32T i, ClCorObjectHandleT *pObjH)
{
    ClRcT       rc = CL_OK;
    ClCorMOIdT  moId ;

    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, TEST_CLASS_A, i);

    rc = clCorObjectHandleGet(&moId, pObjH);
    if(CL_OK != rc)
    {
        rc = clCorObjectCreateAndSet(NULL, &moId, NULL, pObjH);
        if(CL_OK != rc && (CL_COR_INST_ERR_MO_ALREADY_PRESENT != rc))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object for instance Id [%d]. rc[0x%x]", i, rc));
            return rc;
        }
    }
    return CL_OK;
}


ClRcT 
_clCorTestObjAttrSet(ClCorTxnSessionIdT *pTxnId, ClCorObjectHandleT objH)
{
    ClRcT       rc = CL_OK;
    ClInt8T     val8 = 0;
    ClUint32T   val32 = 0;
    ClUint16T   val16 = 0;
    ClUint64T   val64 = 0;
    ClUint32T   size = 0;
    
    val8 = 10;
    size = sizeof(ClUint8T);
    rc = clCorObjectAttributeSet(pTxnId, objH, NULL, TEST_CLASS_A_ATTR_1, -1, &val8, size);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the first job. rc[0x%x]", rc));
        return rc;
    }

    val16 = 20;
    size = sizeof(ClUint16T);
    rc = clCorObjectAttributeSet(pTxnId, objH, NULL, TEST_CLASS_A_ATTR_7, -1, &val16, size);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while adding the second job. rc[0x%x]", rc));
        return rc;
    }

    val32 = 30;
    size = sizeof(ClUint32T);
    rc = clCorObjectAttributeSet(pTxnId, objH, NULL, TEST_CLASS_A_ATTR_4, -1, &val32, size);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while adding the third job. rc[0x%x]", rc));
        return rc;
    }

    val64 = 40;
    size = sizeof(ClUint64T);
    rc = clCorObjectAttributeSet(pTxnId, objH, NULL, TEST_CLASS_A_ATTR_9, -1, &val64, size);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while adding the fourth job. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}



/**
 * Test function to do set opeartions in a loop on test-node-3.
 */
ClRcT 
_clCorTestNode_3_TC1(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClCorObjectHandleT objH7, objH8;
    ClCorTxnSessionIdT tid = 0;
    ClInt32T            index = 0;

    memset(&objH7, 0, sizeof(ClCorObjectHandleT));
    memset(&objH8, 0, sizeof(ClCorObjectHandleT));

    rc = _clCorTestObjCreate(7, &objH7);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"7\" for node-3 . rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(8, &objH8);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"8\" for node-3. rc[0x%x]", rc));
        return rc;
    }    

    for( ; index < loopCount ; index++)
    {
        tid = 0;
        rc = _clCorTestObjAttrSet(&tid, objH7);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH8);
        if(CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while committing the transaction for NODE-3. rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
        }
        clOsalPrintf("TC - 1: ########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }

    return rc;
}



/**
 * Test function to do set opeartions in a loop on test-node-3.
 */
ClRcT 
_clCorTestNode_3_TC2(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClCorObjectHandleT objH1, objH2, objH3;
    ClUint32T index = 0;
    ClCorTxnSessionIdT  tid = 0;

    memset(&objH1, 0, sizeof(ClCorObjectHandleT));
    memset(&objH2, 0, sizeof(ClCorObjectHandleT));
    memset(&objH3, 0, sizeof(ClCorObjectHandleT));

    /* TC - 2 */
    for ( index = 0; index < loopCount ; index ++)
    {
        tid = 0;
        rc = _clCorTestObjCreate(1, &objH1);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    

        rc = _clCorTestObjCreate(2, &objH2);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = _clCorTestObjCreate(3, &objH3);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = _clCorTestObjAttrSet(&tid, objH1);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH2);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH3);
        if (CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while committing the transactoin on NODE -1 . rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
            return rc;
        }
        
        clOsalPrintf("TC - 2 :########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }
    return rc;
}

/**
 * Test function to do set opeartions in a loop on test-node -2 .
 */
ClRcT 
_clCorTestNode_2_TC1(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClCorObjectHandleT objH2, objH3, objH5;
    ClCorTxnSessionIdT tid = 0;
    ClUint32T index = 0;

    memset(&objH2, 0, sizeof(ClCorObjectHandleT));
    memset(&objH3, 0, sizeof(ClCorObjectHandleT));
    memset(&objH5, 0, sizeof(ClCorObjectHandleT));

    rc = _clCorTestObjCreate(2, &objH2);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-2 . rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(3, &objH3);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"3\" for node-2. rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(5, &objH5);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"5\" for node-2. rc[0x%x]", rc));
        return rc;
    }    

    for ( ; index < loopCount ; index++)
    {
        tid = 0;
        rc = _clCorTestObjAttrSet(&tid, objH2);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH3);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH5);
        if(CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while committing the transaction for NODE-2. rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
        }
        clOsalPrintf("TC -1: ########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }
    
    return rc;
}




/**
 * Test function to do set opeartions in a loop on test-node -2 .
 */
ClRcT 
_clCorTestNode_2_TC2(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClUint32T index = 0;
    ClCorObjectHandleT objH1, objH2, objH3; 
    ClCorTxnSessionIdT tid = 0;

    memset(&objH1, 0, sizeof(ClCorObjectHandleT));
    memset(&objH2, 0, sizeof(ClCorObjectHandleT));
    memset(&objH3, 0, sizeof(ClCorObjectHandleT));

    /* TC - 2 */
    for ( index = 0; index < loopCount ; index ++)
    {
        tid = 0;
        rc = _clCorTestObjCreate(1, &objH1);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    

        rc = _clCorTestObjCreate(2, &objH2);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = _clCorTestObjCreate(3, &objH3);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = clCorObjectDelete(&tid, objH1);
        if(CL_OK != rc)
            return rc;

        rc = clCorObjectDelete(&tid, objH2);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH3);
        if (CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while committing the transactoin on NODE -1 . rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
            return rc;
        }
        
        clOsalPrintf(" TC - 2: ########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }
    return rc;
}

/**
 * Test function to do set opeartions in a loop on test-node-1.
 */
ClRcT 
_clCorTestNode_1_TC1(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClCorObjectHandleT objH1, objH2, objH3, objH4;
    ClCorTxnSessionIdT tid = 0;
    ClUint32T index = 0;

    memset(&objH1, 0, sizeof(ClCorObjectHandleT));
    memset(&objH2, 0, sizeof(ClCorObjectHandleT));
    memset(&objH3, 0, sizeof(ClCorObjectHandleT));
    memset(&objH4, 0, sizeof(ClCorObjectHandleT));

    rc = _clCorTestObjCreate(1, &objH1);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"1\" for node-1 . rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(2, &objH2);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(3, &objH3);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"3\" for node-1. rc[0x%x]", rc));
        return rc;
    }    

    rc = _clCorTestObjCreate(4, &objH4);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"4\" for node-1. rc[0x%x]", rc));
        return rc;
    }    

    for ( ; index < loopCount ; index++)
    {
        tid = 0;
        rc = _clCorTestObjAttrSet(&tid, objH1);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH2);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH3);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH4);
        if(CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while committing the transaction for NODE-1. rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
            return rc;
        }

        clOsalPrintf("TC - 1: ########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }

    
    return rc;
}

/**
 * Test function to do set opeartions in a loop on test-node -2 .
 */
ClRcT 
_clCorTestNode_1_TC2(ClUint32T loopCount)
{
    ClRcT   rc = CL_OK;
    ClUint32T index = 0;
    ClCorObjectHandleT objH1, objH2, objH3; 
    ClCorTxnSessionIdT tid = 0;

    memset(&objH1, 0, sizeof(ClCorObjectHandleT));
    memset(&objH2, 0, sizeof(ClCorObjectHandleT));
    memset(&objH3, 0, sizeof(ClCorObjectHandleT));

    /* TC - 2 */
    for ( index = 0; index < loopCount ; index ++)
    {
        tid = 0;
        rc = _clCorTestObjCreate(1, &objH1);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    

        rc = _clCorTestObjCreate(2, &objH2);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = _clCorTestObjCreate(3, &objH3);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the object instance \"2\" for node-1. rc[0x%x]", rc));
            return rc;
        }    
        
        rc = _clCorTestObjAttrSet(&tid, objH1);
        if(CL_OK != rc)
            return rc;

        rc = _clCorTestObjAttrSet(&tid, objH2);
        if(CL_OK != rc)
            return rc;

        rc = clCorObjectDelete(&tid, objH3);
        if (CL_OK != rc)
            return rc;

        rc = clCorTxnSessionCommit(tid);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while committing the transactoin on NODE -1 . rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
            if(rc != 0x170144 && rc != 0x170002)
                return rc;
        }
        
        clOsalPrintf("TC -2 : ########## [%s] ----- completed iteration [%d] \n", __FUNCTION__, index);
    }

    return rc;
}

/**************************************************************************************/

/**
 * OBJECT LOCKING: TC - 1 
 * Test function to test the object locking functionality when there are
 * set operations for same or different MO done from three different nodes.
 */ 
ClRcT cor_test_object_tree_locking_TC1()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdT moId ;
    ClUint32T LOOPCOUNT = 2000;

    clCorMoIdInitialize(&moId);

    /* For Node 1 J1, J2, J3, J4 all set operations. */
#if 1
        _clCorTestNode_1_TC1(LOOPCOUNT);
#endif

    /* For Node 2 J2, J3, J5 all set operations. */
#if 0  
        _clCorTestNode_2_TC1(LOOPCOUNT);
#endif

    /* For Node 3 J7 and J8 set operations. */
#if 0
        _clCorTestNode_3_TC1(LOOPCOUNT);
#endif

    return rc;
}



/**
 * OBJECT LOCKING: TC - 1 
 * Test function to test the object locking functionality when there 
 * are delete and set jobs combinations in the transaction done from
 * three different nodes.
 */ 
ClRcT cor_test_object_tree_locking_TC2()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdT moId ;
    ClUint32T LOOPCOUNT = 2000;

    clCorMoIdInitialize(&moId);

    /* For Node 1  (J1(set), J2(set), J3(del)*/
#if 1
        _clCorTestNode_1_TC2(LOOPCOUNT);
#endif

    /* For Node 2 (J1(del), J2(del), J3(set)*/
#if 0
        _clCorTestNode_2_TC2(LOOPCOUNT);
#endif

    /* For Node 3 (J1, J2, J3 all set)*/
#if 0
        _clCorTestNode_3_TC2(LOOPCOUNT);
#endif

    return rc;
}



/**
 * Test Function to verify the version of the client.
 */ 
ClRcT cor_test_version_verify()
{
    ClVersionT validVersion = {'B', 0x1, 0x1}, invalidVersion = {'B', 0x2, 0x1};
    ClRcT rc = CL_OK;
    ClCharT ch [100] = {0};

    rc = clCorVersionCheck(&validVersion);  
    ch[0]= '\0';
    sprintf(ch,"\tVersion Check with a valid version. rc[0x%x]. Result[%s]", rc, rc?"FAIL":"PASS");
    CL_COR_TEST_PUT_IN_FILE(ch);
  
    rc = clCorVersionCheck(&invalidVersion);
    if(rc != CL_OK)
    {
        ch[0]= '\0';
        sprintf(ch, "\tThe version obtained is. releaseCode %c, majorVersion 0x%x, minorVersion 0x%x, rc 0x%x, Result[%s]", 
                   invalidVersion.releaseCode, invalidVersion.majorVersion, invalidVersion.minorVersion, rc, rc?"PASS":"FAIL");
        CL_COR_TEST_PUT_IN_FILE(ch);
    }
        
    return rc;
}



/* =========== Transaction-Agent Callback Functions =========== */
static ClRcT _clCorTestCorTxnJobWalk (
        CL_IN    ClCorTxnIdT         txnId,
        CL_IN    ClCorTxnJobIdT  jobId,
        CL_IN    void               *cookie)
{
    ClRcT          rc = CL_OK;
    ClCorMOIdT     moId;
    ClCorOpsT      op = CL_COR_OP_RESERVED;
    ClCorAttrIdT   attrId = 0;
    ClInt32T       index = 0 ;
    ClUint32T      size = 0;
    ClPtrT         pValue = NULL;
    
    CL_FUNC_ENTER();

    if ((txnId == 0) || (jobId == 0))
    {
        CL_FUNC_EXIT();
        return (CL_COR_ERR_NULL_PTR);
    }
    if((rc = clCorTxnJobOperationGet(txnId, jobId, &op)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get Opeation from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }
  
    if((rc =  clCorTxnJobMoIdGet(txnId, &moId))!= CL_OK) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get MOId from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }
    
    switch (op)
    {
    case CL_COR_OP_CREATE_AND_SET:
    case CL_COR_OP_CREATE:
    {
            clOsalPrintf("TC *: Inside the create/createAndSet case ... \n");
    }
    break;
    case CL_COR_OP_SET:
    {
        clOsalPrintf("TC * : Inside the set case .... \n ");
        rc = clCorTxnJobSetParamsGet( txnId, jobId, &attrId, &index, &pValue, &size);
        if(CL_OK != rc)
        {
            clOsalPrintf( "Could not get SET paramters from txn-job. rc[0x%x] \n", rc);
        }
        else
        {
            clOsalPrintf("TC * : Validating  the attribute id  [0x%x].... \n ", attrId);
#if 0
            if(attrId == TEST_CLASS_A_ATTR_5)
            {
                rc = CL_COR_ERR_INVALID_SIZE;
                clCorTxnJobStatusSet(txnId, jobId, rc);
                return rc;
            }
#endif
        }
        

    }
    break;
    case CL_COR_OP_DELETE:
            clOsalPrintf("TC *: inside the Delete case of job walk. \n");
 	break;
    default:
        clOsalPrintf( "\nInvalid operation called\n");
        return rc;
    }
 
   CL_FUNC_EXIT();
  return (rc);
}
/* Define (static) global variables */
ClTxnClientHandleT        gCorTestTxnHandle;

ClRcT clCorTestTxnPrepareCallback(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    ClCorMOIdT moId;
    ClCorTxnIdT txnId = 0;
    ClCorTxnJobIdT jobId = 0;

   clOsalPrintf("\n\n\n           ################## TRANSACTION PREPARE --  number %d ###################### \n", ++i);
       
    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize,  &txnId); 
    if(CL_OK != rc)
    {
       clOsalPrintf("Failed while getting the cor's txn id. rc[0x%x] \n", rc);
       return rc;
    }

    rc = clCorTxnJobMoIdGet(txnId, &moId);
    if(CL_OK != rc)
    {
       clOsalPrintf("Failed while getting the moid from header job. rc[0x%x] \n", rc);
    }

    clOsalPrintf("\n\t\t\tMoId  .......... ");
    clCorMoIdShow(&moId);
      
    if((rc = clCorTxnJobWalk(txnId, _clCorTestCorTxnJobWalk, NULL)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "TXN job Stream Processing Failed, rc = 0x%x", rc));
        clCorTxnJobDefnHandleUpdate(jobDefn, txnId);
        clCorTxnIdTxnFree(txnId); 
        return rc;
    }

    clCorTxnFirstJobGet(txnId, &jobId);
    do
    {
       corTxnJobInfoDisplay(txnId, jobId);  
 
    } while(clCorTxnNextJobGet(txnId, jobId, &jobId) == CL_OK);
        
   clCorTxnIdTxnFree(txnId); 

    return (rc);
}


ClRcT clCorTestTxnCommitCallback(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    clOsalPrintf("\n Calling Txn Commit ..... %d \n", ++i);
    return (rc);
}

ClRcT clCorTestTxnRollbackCallback(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    clOsalPrintf("\n Calling Txn Rollback ..... %d \n", ++i);
    return (rc);
}


ClRcT clCorTestTxnFinalizeCallback(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn,
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;
    static int i;
    clOsalPrintf("\n Calling Txn Finalize ..... %d \n", ++i);
    return (rc);
}

/* Callback functions for transaction-agent */
static ClTxnAgentCallbacksT gCorTestTxnCallbacks = {
    .fpTxnAgentJobPrepare   = clCorTestTxnPrepareCallback,
    .fpTxnAgentJobCommit    = clCorTestTxnCommitCallback,
    .fpTxnAgentJobRollback  = clCorTestTxnRollbackCallback,
};

ClRcT clCorTestTxnInterfaceInit()
{
    ClRcT                   rc = CL_OK;
    ClVersionT              txnVersion = {
                                 .releaseCode    = 'B',
                                 .majorVersion   = 1,
                                 .minorVersion   = 1};
                                                                                                                            
    /* Initialize transaction-client */
    rc = clTxnClientInitialize(&txnVersion, NULL, &(gCorTestTxnHandle));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (" Initialize transaction-client. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }
                                                                                                                            
    rc = clTxnAgentServiceRegister(CL_COR_INVALID_SRVC_ID, gCorTestTxnCallbacks,
                                   &(gCorTestTxnHandle));

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Initialization done\n"));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Function to add lot of object which will be synched-up
 * while the standby COR is coming up.
 */

ClRcT cor_test_create_object()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdT           moId ; 
    ClCorObjectHandleT   objH = {{0}};
    ClUint32T            i = 0;

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, 0);

    for(i = 0; i < 200; i++)
    {
        clCorMoIdInstanceSet(&moId, 1, i);

        rc = clCorUtilMoAndMSOCreate(&moId, &objH);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]", rc));
            return rc;
        }

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Created %d object Successfully. ", i));
    }    

     
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Successfully completed the object creation. "));
    return rc;
}


/**
 * Function to get the delays happens in cor in certain conditions.
 */ 
void
cor_test_delays_get()
{
    ClRcT rc = CL_OK;
    ClUint32T   i = 0;
    ClTimeT  time = 0;
    ClCorDelayRequestOptT op []= { CL_COR_DELAY_DM_UNPACK_REQ, CL_COR_DELAY_MOCLASSTREE_UNPACK_REQ, CL_COR_DELAY_OBJTREE_UNPACK_REQ,
                                   CL_COR_DELAY_ROUTELIST_UNPACK_REQ, CL_COR_DELAY_NITABLE_UNPACK_REQ, CL_COR_DELAY_SYNCUP_STANDBY_REQ,
                                   CL_COR_DELAY_DM_PACK_REQ, CL_COR_DELAY_MOCLASSTREE_PACK_REQ, CL_COR_DELAY_OBJTREE_PACK_REQ, 
                                   CL_COR_DELAY_ROUTELIST_PACK_REQ, CL_COR_DELAY_NITABLE_PACK_REQ, CL_COR_DELAY_SYNCUP_ACTIVE_REQ,
                                   CL_COR_DELAY_OBJTREE_DB_RESTORE_REQ, CL_COR_DELAY_ROUTELIST_DB_RESTORE_REQ, CL_COR_DELAY_DB_RESTORE_REQ,
                                   CL_COR_DELAY_OBJ_DB_RECREATE_REQ, CL_COR_DELAY_ROUTE_DB_RECREATE_REQ, CL_COR_DELAY_DB_RECREATE_REQ}; 
    ClCharT opStr[][45] = { {"CL_COR_DELAY_DM_UNPACK_REQ"}, {"CL_COR_DELAY_MOCLASSTREE_UNPACK_REQ"}, {"CL_COR_DELAY_OBJTREE_UNPACK_REQ"},
                              {"CL_COR_DELAY_ROUTELIST_UNPACK_REQ"}, {"CL_COR_DELAY_NITABLE_UNPACK_REQ"}, {"CL_COR_DELAY_SYNCUP_STANDBY_REQ"},
                              {"CL_COR_DELAY_DM_PACK_REQ"}, {"CL_COR_DELAY_MOCLASSTREE_PACK_REQ"}, {"CL_COR_DELAY_OBJTREE_PACK_REQ"}, 
                              {"CL_COR_DELAY_ROUTELIST_PACK_REQ"}, {"CL_COR_DELAY_NITABLE_PACK_REQ"}, {"CL_COR_DELAY_SYNCUP_ACTIVE_REQ"},
                              {"CL_COR_DELAY_OBJTREE_DB_RESTORE_REQ"}, {"CL_COR_DELAY_ROUTELIST_DB_RESTORE_REQ"}, {"CL_COR_DELAY_DB_RESTORE_REQ"},
                              {"CL_COR_DELAY_OBJ_DB_RECREATE_REQ"}, {"CL_COR_DELAY_ROUTE_DB_RECREATE_REQ"}, {"CL_COR_DELAY_DB_RECREATE_REQ"}}; 


    for( i = 0; i < sizeof(op)/sizeof(ClCorDelayRequestOptT) ; i++)
    {
        rc = clCorOpProcessingTimeGet(op[i], &time);
        if(CL_OK != rc)
        {
            clOsalPrintf("TC - [%d]: Failed while getting the response for the time. rc[0x%x]\n", i+1, rc);
            continue;
        }
        else
            clOsalPrintf("TC - [%d]: The time delay obtained for op[%s] [%llu] \n", i+1, opStr[i], time);
    }

    return ;
}

/**
 * Test function to test the debug cli register. 
 */ 

ClRcT
cor_client_debug_func_test(ClHandleT *pDbgHandle)
{
    ClRcT rc = CL_OK;
    

    rc = clCorClientDebugCliRegister(pDbgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC-1 : Failed. rc[0x%x] \n", rc);
        return rc;
    }

    return rc;
}



/**
 * Test function to test the debug deregister.
 */ 
ClRcT
cor_client_debug_func_deregister_test(ClHandleT dbgHandle)
{
    ClRcT   rc = CL_OK;

    rc = clCorClientDebugCliDeregister(dbgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s]: TC - 1: Failed . rc[0x%x]", __FUNCTION__, rc);
        return rc;
    }
    else
        clOsalPrintf("[%s]: TC - 1: Passed \n", __FUNCTION__);

    return rc;
}

/**
 * Function to test the oi -register functionality.
 */

ClRcT cor_test_oi_register()
{
    ClRcT       rc = CL_OK;
    ClCharT str[50] = {0}; 
    ClNameT     moIdName = {0};
    ClCorMOIdT  moId;
    ClCorAddrT  compAddr = {clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT };
    ClCorAddrT  compAddr1 = {clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};
    ClInt8T     status = 0;
    ClUint32T   i = 0;

    sprintf(str, "\\Aclass:5\\BClass:*\\Dclass:*\\Dclass*");
    strncpy(moIdName.value, str, strlen(str));
    moIdName.length = strlen(str);

    rc = clCorMoIdNameToMoIdGet(&moIdName, &moId);
    if(rc != CL_OK)
    {
        clOsalPrintf("The moid name to moid get failed with rc[0x%x]", rc);
    }

    moId.depth=13243;
    moId.svcId = 1000;

    rc = clCorOIRegister(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC -1 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC -1 FAILED. \n");

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, 1);

    rc = clCorOIRegister(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC -2 : FAILED. rc[0x%x] \n", rc);
    }
    else 
        clOsalPrintf("TC -2 : PASSED \n");

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, CL_COR_INSTANCE_WILD_CARD);

    rc = clCorOIRegister(&moId, &compAddr1);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC -3 : FAILED. rc[0x%x] \n", rc);
    }
    else 
        clOsalPrintf("TC -3 : PASSED \n");

    moId.depth=13243;
    moId.svcId = 1000;

/* TC- 4 to TC - 10 testing the invalid MOId check ...*/

    rc = clCorPrimaryOISet(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 4 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 4 FAILED. \n");

    rc = clCorMoIdToComponentAddressGet(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 5 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 5 FAILED. \n");

    rc = clCorServiceRuleDisable(&moId, compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 6 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 6 FAILED. \n");
     
    rc = clCorServiceRuleDelete(&moId, compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 7 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 7 FAILED. \n");

    rc = clCorServiceRuleStatusGet(&moId, compAddr, &status);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 8 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 8 FAILED. \n");

    rc = clCorPrimaryOIClear(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 9 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 9 FAILED. \n");

    rc = clCorPrimaryOIGet(&moId, &compAddr);
    if(rc != CL_OK)
    {
        clOsalPrintf("TC - 10 PASSED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 10 FAILED. \n");


    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_B, 0);
    clCorMoIdAppend(&moId, TEST_CLASS_B, 1);
    moId.svcId = CL_COR_SVC_WILD_CARD;

    rc = clCorServiceRuleAdd(&moId, compAddr);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 11 FAILED. rc[0x%x]\n", rc);
    }
    else
        clOsalPrintf("TC - 11 PASSED. \n");

    /* TC - 12 */
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_E, 0);
    for ( i = 0; i < 100; i++)
    {
        clCorMoIdInstanceSet(&moId, 1, i);
        
        rc = clCorOIRegister(&moId, &compAddr);
        if(CL_OK != rc)
        {
            clOsalPrintf("TC - 12 : FAILED. rc[0x%x]\n", rc);
            goto endtc;
        }
        clOsalPrintf("Completed the oi registration [%d] ##########\n", i+1);
    }
    
    clOsalPrintf("TC -12: PASSED \n");

endtc:
    return 0;
}



/**
 * Test function to test the functionality of moId to moId name  get
 */

ClRcT
cor_test_moId_2_moidNameGet()
{
    ClRcT   rc  = CL_OK;
    ClCharT str[] = "aBcDefGhIjKlMnOpQrStAbCdEfGhIjKlMnOpQrStUvWxYzAaBbCcDdEeFfGgHhIiJjKkLlMmNnOo";
    //ClCharT str[] = "aBcDefGhIjKlMnOpQrStAbCdEfGhIjKlMnOpQrStUvWxYzAaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZzAa";
    ClCorMOIdT moId ;
    ClNameT moIdName = {0};

    clOsalPrintf("Size of the class Name is [%d] \n", strlen(str));

    rc = clCorClassCreate(0x9801, 0);
    if(CL_OK != rc)
    {
        clOsalPrintf( "Failed while creating the class. rc [0x%x] \n", rc);
        return rc;
    }

    rc = clCorClassNameSet(0x9801, str);
    if(CL_OK != rc)
    {
        clOsalPrintf("Failed while setting the name of the class [%s]. rc[0x%x] \n", str, rc);
        return rc;
    }

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, 0x9801, 0);
    clCorMoIdAppend(&moId, 0x9801, 1);
    clCorMoIdAppend(&moId, 0x9801, 3);
    clCorMoIdAppend(&moId, 0x9801, 4);

    clCorMoIdShow(&moId);

    rc = clCorMoIdToMoIdNameGet(&moId, &moIdName);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 1: Failed rc[0x%x] \n", rc);
        return rc;
    }
    else
        clOsalPrintf("TC - 2: Passed: [%s] \n", moIdName.value);

    return rc;
}

/**
 *  Test function to test the functionality of moid name to moid get.
 */
ClRcT cor_test_moIdname_2_moid_get()
{
    ClRcT   rc = CL_OK;
    ClNameT moIdName = {0};
    ClCharT str1[]="\\Aclass:0"; 
    ClCharT str2[]="\\:0";
    ClCharT str3[]="\\";
    ClCharT str4[]="\\Aclass:0\\TEST_CLASS_B:3";
    ClCharT str5[]="\\Aclass:5\\Bclass:4\\Dclass:6";
    ClCharT str6[]="\\Aclass:100\\Dclass:500\\Eclass:7\\Dclass:3\\Cclass:8\\Bclass:5";
    ClCharT str7[]="\\Dclass:100\\0x100:5\\Bclass:9\\0x500:8";
    ClCharT str8[]="";
    ClCorMOIdT moIdTest ;
    ClCorMOIdT moIdGet ; 


    /**
      * TC - 1 : Positive Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str1, strlen(str1));
    moIdName.length = strlen(str1); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 1- Result [FAILED] , as it is unable to get the value successfully. rc[0x%x] \n", rc);
        goto tc2;
    }
    clCorMoIdInitialize(&moIdTest);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_A, 0);
    if(clCorMoIdCompare(&moIdGet, &moIdTest) == 0)
        clOsalPrintf("TC - 1- Result [PASSED] \n");
    else 
        clOsalPrintf("TC - 1- Result [FAILED] \n");

tc2:
     
    /**
      * TC - 2 : Negative Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str2, strlen(str2));
    moIdName.length = strlen(str2); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK == rc)
    {
        clOsalPrintf("TC - 2- Result [FAILED] , as it is able to get the value successfully. \n");
        goto tc3;
    }
    else
        clOsalPrintf("TC - 2- Result [PASSED] \n");


tc3:
    /**
      * TC - 3 : Negative Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str3, strlen(str3));
    moIdName.length = strlen(str3); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK == rc)
    {
        clOsalPrintf("TC - 3- Result [FAILED] , as it is able to get the value successfully. \n");
        goto tc4;
    }
    else
        clOsalPrintf("TC - 3- Result [PASSED] \n");

tc4:
    /**
      * TC - 4 : Negative Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str4, strlen(str4));
    moIdName.length = strlen(str4); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK == rc)
    {
        clOsalPrintf("TC - 4- Result [FAILED] , as it is able to get the value successfully. \n");
        goto tc5;
    }
    else
        clOsalPrintf("TC - 4- Result [PASSED] \n");

tc5:
    /**
      * TC - 5 : Positive Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str5, strlen(str5));
    moIdName.length = strlen(str5); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 5- Result [FAILED] , as it is unable to get the value successfully. rc[0x%x] \n", rc);
        goto tc6;
    }
    clCorMoIdInitialize(&moIdTest);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_A, 5);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_B, 4);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_D, 6);
    if(clCorMoIdCompare(&moIdGet, &moIdTest) == 0)
        clOsalPrintf("TC - 5- Result [PASSED] \n");
    else 
        clOsalPrintf("TC - 5- Result [FAILED] \n");

tc6:
    /**
      * TC - 6 : Positive Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str6, strlen(str6));
    moIdName.length = strlen(str6); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 6- Result [FAILED] , as it is unable to get the value successfully. rc[0x%x] \n", rc);
        goto tc7;
    }
    clCorMoIdInitialize(&moIdTest);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_A, 100);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_D, 500);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_E, 7);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_D, 3);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_C, 8);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_B, 5);
    if(clCorMoIdCompare(&moIdGet, &moIdTest) == 0)
        clOsalPrintf("TC - 6- Result [PASSED] \n");
    else 
        clOsalPrintf("TC - 6- Result [FAILED] \n");

tc7:
    /**
      * TC - 7 : Positive Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str7, strlen(str7));
    moIdName.length = strlen(str7); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK != rc)
    {
        clOsalPrintf("TC - 7- Result [FAILED] , as it is unable to get the value successfully. rc[0x%x] \n", rc);
        goto tc8;
    }
    clCorMoIdInitialize(&moIdTest);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_D, 100);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_A, 5);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_B, 9);
    clCorMoIdAppend(&moIdTest, TEST_CLASS_E, 8);
    if(clCorMoIdCompare(&moIdGet, &moIdTest) == 0)
        clOsalPrintf("TC - 7- Result [PASSED] \n");
    else 
        clOsalPrintf("TC - 7- Result [FAILED] \n");

tc8:
    /**
      * TC - 8 : Negative Test Case 
      */
    memset(&moIdName, 0, sizeof(ClNameT));
    strncpy(moIdName.value, str8, strlen(str8));
    moIdName.length = strlen(str8); 
    clCorMoIdInitialize(&moIdGet);
    rc = clCorMoIdNameToMoIdGet(&moIdName, &moIdGet);
    if(CL_OK == rc)
    {
        clOsalPrintf("TC - 8- Result [FAILED] , as it is able to get the value successfully. \n");
        goto exiterror;
    }
    else
        clOsalPrintf("TC - 8- Result [PASSED] \n");

exiterror:

    return rc;
}



/**
 *  Getting the failed jobs from the transactoin. 
 */
ClRcT cor_test_failed_jobs_get()
{
    ClRcT   rc = CL_OK;
    ClCorMOIdT  moId;
    ClCorTxnSessionIdT tid = {0};
    ClCorTxnInfoT      txnInfo = {0};
    ClCorObjectHandleT objH = {{0}}, objH1 = {{0}};
    ClUint32T val32 = 0, size = 0, val321 = 0;
    ClCorAddrT   compAddr = {0};
    ClUint64T   val64 = 0;

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_C, 100);
    rc = clCorObjectCreate(&tid, &moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 1 FAILED: Creating invalid object. rc[0x%x]", 
                __FUNCTION__, __LINE__, rc);
        goto tc2;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc )
        {
            clOsalPrintf("[%s][%d] TC 1 FAILED: Creating invalid object. rc[0x%x]", 
                __FUNCTION__, __LINE__, rc);
        }
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC - 1 PASSED: The jobStatus [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }

        rc = clCorTxnFailedJobGet(tid, &txnInfo, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 1 PASSED : Creating invalid object. rc[0x%x] \n", 
                    __FUNCTION__, __LINE__, rc);
        }
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC 1 PASSED: Shouldn't come here ... jobStatus [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }
        clCorTxnSessionFinalize(tid);
    }

tc2:
    tid = 0;
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, 10);
    rc = clCorObjectCreate(&tid, &moId, &objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 2 FAILED : Creating one valid and one invalid object in transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc3;
    }

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_C, 101);
    rc = clCorObjectCreate(&tid, &moId, &objH1);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 2 FAILED : Creating one valid and one invalid object in transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc3;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 2 FAILED : Creating one valid and one invalid object in transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        } 
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC 2 PASSED : The jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }

        rc = clCorTxnFailedJobGet(tid, &txnInfo, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 2 PASSED : Creating one valid and one invalid object in transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        }
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC 2 FAILED : Shouldn't come here ... jobStatus [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }

        clCorTxnSessionFinalize(tid);
    }

tc3:
    memcpy(&objH, "ffff31212", sizeof("ffff01212"));
    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, TEST_CLASS_A, 20);

    if (clCorObjectHandleGet(&moId, &objH1) != CL_OK)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH1);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 3 FAILED: Deleting a valid and an invalid object in same transaction. rc[0x%x] \n", 
                    __FUNCTION__, __LINE__, rc);
            goto tc4;
        }
    }

    tid = 0;
    rc = clCorObjectDelete(&tid, objH);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 3 FAILED: Deleting a valid and an invalid object in same transaction. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc4;
    }

    rc = clCorObjectDelete(&tid, objH1);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 3 FAILED: Deleting a valid and an invalid object in same transaction. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc4;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 3 PASSED: Deleting a valid and an invalid object in same transaction. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        }
        else
        {

            clOsalPrintf("[%s][%d] TC 3 FAILED: The jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }
        clCorTxnSessionFinalize(tid);
    }

tc4:
    tid = 0;
    val32 = 0;
    size = sizeof(ClUint32T);

    rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_4, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 4 FAILED: Attr set on two attributes of valid and invalid object. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc5;
    }

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_5, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 4 FAILED: Attr set on two attributes of valid and invalid object. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc5;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 4 PASSED: Attr set on two attributes of valid and invalid object. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        }
        else
        {

            clOsalPrintf("[%s][%d] TC 4 FAILED: The jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.jobStatus);
        }
        clCorTxnSessionFinalize(tid);
    }

tc5:
    
    tid = 0;
    val32 = 0;
    size = 1;
    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_5, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 5 FAILED : Setting the attribute with invalid size. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc6;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 5 FAILED: Setting the attribute with invalid size. rc[0x%x] \n", 
                    __FUNCTION__, __LINE__, rc);
        }
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC 5 PASSED: The Attrid is [0x%x] jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);
        }
        clCorTxnSessionFinalize(tid);
    }

tc6:

    tid = 0;
    val32 = 0;
    size = sizeof(ClUint32T);

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_5, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 6 FAILED - Setting invalid attribute id for an object. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc7;
    }

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, 0x1004, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] TC 6 FAILED - Setting invalid attribute id for an object. rc[0x%x] \n", 
                __FUNCTION__, __LINE__, rc);
        goto tc7;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] TC 6 FAILED - Setting invalid attribute id for an object. rc[0x%x] \n", 
                        __FUNCTION__, __LINE__, rc);
        }
        else
        {
            clCorMoIdShow(&txnInfo.moId);
            clOsalPrintf("[%s][%d] TC 6 PASSED -  The Attrid is [0x%x] jobStatus is [0x%x] \n",
                                     __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);
        }
        clCorTxnSessionFinalize(tid);
    }

tc7:
    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, TEST_CLASS_A, 20);

    compAddr.nodeAddress = clIocLocalAddressGet();
    compAddr.portId = CL_COR_TEST_CLIENT_IOC_PORT;
    rc = clCorOIRegister(&moId, &compAddr);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] : TC 7 FAILED -. Getting failed jobs when there are two jobs in the transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc8;
    }

    tid = 0;
    val32 = 0;
    val321 = 0;
    size = sizeof(ClUint32T);

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_4, -1, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] : TC 7 FAILED -. Getting failed jobs when there are two jobs in the transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc8;
    }

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_5, -1, &val321, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] : TC 7 FAILED -. Getting failed jobs when there are two jobs in the transaction. rc[0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc8;
    }

    rc = clCorTxnSessionCommit(tid);
    if(CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] : TC 7 FAILED -. Getting failed jobs when there are two jobs in the transaction. rc[0x%x]\n", 
                    __FUNCTION__, __LINE__, rc);
        }
        else
        {

            clOsalPrintf("[%s][%d] : TC 7 PASSED - The attrId [0x%x] jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);
        }

        rc = clCorTxnFailedJobGet(tid, &txnInfo, &txnInfo);
        if(CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] : TC 7 FAILED -. Getting failed jobs when there are two jobs in the transaction. rc[0x%x]\n", 
                    __FUNCTION__, __LINE__, rc);
        }
        else
        {

            clOsalPrintf("[%s][%d] : TC 7 PASSED - The attrId [0x%x] jobStatus is [0x%x] \n", 
                    __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);
        }
 
        clCorTxnSessionFinalize(tid);
    }

tc8:

    tid = 0;
    val32 = 0;
    size = sizeof(ClUint32T);

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_4, 10, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] : TC 8 FAILED - Attr set on the simple attribute using non-zero index [0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc9;
    }

    rc = clCorTxnSessionCommit(tid);
    if (CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: TC 8 FAILED - Attr set on simple attribute using non-zero index . [0x%x]\n", 
                    __FUNCTION__, __LINE__, rc);
            clCorTxnSessionFinalize(tid);
            goto tc9;
        }

        clOsalPrintf("[%s][%d]: TC 8 PASSED - AttrId [0x%x] rc [0x%x] \n", 
                __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);

        clCorTxnSessionFinalize(tid);
    }

tc9:
    tid = 0;
    val32 = 10;
    size = sizeof(ClUint32T);

    rc = clCorObjectAttributeSet(&tid, objH1, NULL, TEST_CLASS_A_ATTR_10, 1002, &val32, size);
    if(CL_OK != rc)
    {
        clOsalPrintf("[%s][%d] : TC 9 FAILED - Attr set on the array attribute using invalid index [0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
        goto tc10;
    }

    rc = clCorTxnSessionCommit(tid);
    if (CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d] : TC 9 FAILED - Attr set on the array attribute using invalid index [0x%x]\n", 
                __FUNCTION__, __LINE__, rc);
            clCorTxnSessionFinalize(tid);
            goto tc10;
        }

        clOsalPrintf("[%s][%d]: TC 9 PASSED - AttrId [0x%x] rc [0x%x] \n", 
                __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);

        clCorTxnSessionFinalize(tid);
    }


tc10:
    
    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, TEST_CLASS_A, 20);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: TC 10 FAILED: Test case to set a containment attribute. rc[0x%x] \n",
                    __FUNCTION__, __LINE__, rc);
            goto tc11;
        }
    }
    
    clCorMoIdAppend(&moId, TEST_CLASS_B, 10);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: TC 10 FAILED: Test case to set a containment attribute. rc[0x%x] \n",
                    __FUNCTION__, __LINE__, rc);
            goto tc11;
        }
    }

    clCorMoIdAppend(&moId, TEST_CLASS_C, 10);
    rc = clCorObjectHandleGet(&moId, &objH);
    if (CL_OK != rc)
    {
        rc = clCorObjectCreate(NULL, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: TC 10 FAILED: Test case to set a containment attribute. rc[0x%x] \n",
                    __FUNCTION__, __LINE__, rc);
            goto tc11;
        }
    }

    tid = 0;
    val64 = 10020;
    size = sizeof (ClUint64T);

    rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_C_ATTR_3, 2, &val64, size);
    if (CL_OK != rc)
    {
        clOsalPrintf("[%s][%d]: TC 10 FAILED: Test case to set a containment attribute. rc[0x%x] \n",
                __FUNCTION__, __LINE__, rc);
        goto tc11;
    }

    rc = clCorTxnSessionCommit(tid);
    if (CL_OK != rc)
    {
        rc = clCorTxnFailedJobGet(tid, NULL, &txnInfo) ;
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: TC 10 FAILED: Test case to set a containment attribute. rc[0x%x] \n",
                    __FUNCTION__, __LINE__, rc);
            clCorTxnSessionFinalize(tid);
            goto tc11;
        }

        clOsalPrintf("[%s][%d]: TC 10 PASSED - AttrId [0x%x] rc [0x%x] \n", 
                __FUNCTION__, __LINE__, txnInfo.attrId, txnInfo.jobStatus);

        clCorTxnSessionFinalize(tid);
    }

tc11:
    return rc;
}




/**
 * Function to test the retry of the requrests landing at
 * the initialize time.
 *
 *  For running this test case: Put some sleep in the COR intialization
 *  phase and run one funtion at a time to see whether the delay in the 
 *  the initialization phase should cause all the request to the COR
 *  server should retry.
 *  
 */

ClRcT cor_test_retry_initialize_time_requests()
{
    ClRcT   rc = CL_OK;
    ClCorMOClassPathT moPath;
    ClCorMOIdT moId;
    ClUint64T   val = 0;
    ClUint32T   size64 = 0;
    ClCorObjectHandleT objH = {{0}};
    ClCorAddrT compAddr = { clIocLocalAddressGet(), CL_COR_TEST_CLIENT_IOC_PORT};
    ClNameT moIdName = {0}, nodeName = {0};

    rc = clCorClassCreate(0x5005, 0x0);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC -1 : Class creation failed with rc[0x%x]\n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC -1: Class creation for 0x5005 has passed \n");

    rc = clCorClassNameSet(0x5005, "TEST_Class_init");
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-2: Class name set has failed with rc [0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-2: Class name set has passed. \n");

    rc = clCorClassAttributeCreate(0x5005, 0x50051, CL_COR_UINT64);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC -3: Failed while creating the attribute 0x50051 for the class. rc[0x%x] \n", rc);
        return rc;
    } 
    else
        clOsalPrintf("TC -3: The attribute creation for the attribute 0x50051 has passed. \n");

    rc = clCorClassAttributeValueSet(0x5005, 0x50051, 10, 0, 1100);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-4: Failed while setting the initial value for the attribute 0x50051. rc[0x%x]\n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-4: Successfully set the initial value of the attribute 0x50051. \n");

    clCorMoClassPathInitialize(&moPath);
    clCorMoClassPathAppend(&moPath, 0x5005);

    rc = clCorMOClassCreate(&moPath, 20);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-5: MO class path creation is failing with error. rc[0x%x] \n", rc) ;
        goto endTest;
    }
    else
        clOsalPrintf("TC-5: MO class path creatoin is passing. \n");

    clCorMoIdInitialize(&moId);
    clCorMoIdAppend(&moId, 0x5005, 1);

    rc = clCorOIRegister(&moId, &compAddr);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-6: Failed while registering the route list for the MO. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-6: Sucessfully registers the OI in the route list. \n");


    rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, &moId, &objH);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-7: Failed while creating the object. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-7: The object is sucessfully created. \n");

    val = 14;
    rc = clCorObjectAttributeSet(CL_COR_SIMPLE_TXN, objH, NULL, 0x50051, -1, &val, sizeof(ClUint64T));
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-8: Failed while setting the value of the attribute. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-8: Successfully setting the attribute. \n");
    
    size64 = sizeof(ClUint64T);
    rc = clCorObjectAttributeGet(objH, NULL, 0x50051, -1, &val, &size64);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-9: Failed while getting the value of the attribute. rc0x%x] \n ", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-9: Successfully got the value of the attribute [%lld]. \n", val);

    rc = clCorObjectHandleGet(&moId, &objH) ;
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-10: Failed to obtained the object handle. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-10: Successfully obtained the object handle \n");

    rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, objH);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-11: Failed while deleting the object. rc[0x%x]\n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-11: Successfully deleted the object. \n");

    rc = clCorMoIdToMoIdNameGet(&moId, &moIdName);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-12: Failed while getting the moid to moid name. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-12: Successfully obtained the moid name from the given moId. \n");

    rc = clCpmLocalNodeNameGet(&nodeName);
    if (CL_OK != rc)
    {
       clOsalPrintf("TC-13: Failed while getting the node name for the node. rc[0x%x] \n", rc); 
    }
    else
        clOsalPrintf("TC-13: Successfully obtained the node name [%s]. \n", nodeName.value);

    memset(&moIdName, 0, sizeof(ClNameT));
    rc = clCorNodeNameToMoIdGet(nodeName, &moId);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-13: Failed while getting the node name to moid. rc[0x%x]\n", rc);
        return rc;
    }
    else
        clOsalPrintf("TC-13: Successfully obtained the moId given the node name. \n");

    memset(&moIdName, 0, sizeof(ClNameT));
    memcpy(&moIdName, "//Chassis:0//SystemController:0", sizeof("//Chassis:0//SystemController:0"));

    rc = clCorMoIdNameToMoIdGet(&moIdName, &moId);
    if (CL_OK != rc)
    {
        clOsalPrintf("TC-14: Failed while getting the moId given the MOId. rc[0x%x] \n", rc);
        goto endTest;
    }
    else
        clOsalPrintf("TC-14: Successfully got the moId from the moid name. \n");

endTest:
    return rc;
}
  

ClRcT
cor_test_moidName_2_moid_get()
{
    ClRcT       rc = CL_OK;
    ClCharT     moidName [] = "\\Aclass";
    ClNameT     tempName = {0};
    ClCorMOIdT  moId ;
    ClUint32T   i = 0;
    ClCorObjectHandleT  objH = {{0}};

    for ( i= 0; i < 200; i++)
    {
        memset(&tempName, 0, sizeof(ClNameT));
        clCorMoIdInitialize(&moId);

        sprintf(tempName.value, "%s:%d", moidName, i);
        tempName.length = sizeof(tempName.value);

        clOsalPrintf("Getting the moId from moId String [%s] \n", tempName.value);

        rc = clCorMoIdNameToMoIdGet(&tempName, &moId);
        if (CL_OK != rc)
        {
            clOsalPrintf("TC-1: Failed to get the MoId for MOID string [%s]. rc[0x%x] \n", tempName.value, rc);
            continue;
        }

        rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("TC-2: Failed to create the object instance [%d]. rc[0x%x] \n", i, rc);
            return rc;
        }
    }

    return rc;
}
/***************   EVENT RELATE STUFF IS HERE *************/
void corTestEvtChannelOpenCallBack( ClInvocationT invocation, 
        ClEventChannelHandleT channelHandle, ClRcT error )
{

}

void corTestEventDeliverCallBack( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
        static int evntno;
	ClCorMOIdT moId;
        ClUint32T  *cookie = NULL;
        ClCorTxnIdT txnId;
        ClCorTxnJobIdT jobId = 0;

   clOsalPrintf("\n\n\n           ################## EVENT RECEIVED --  number %d ###################### \n", ++evntno);
       
   clCorEventHandleToCorTxnIdGet(eventHandle, eventDataSize,  &txnId);
   clCorTxnJobMoIdGet(txnId, &moId);

   clOsalPrintf("\n\t\t\tMoId  .......... ");
   clCorMoIdShow(&moId);
  
    clCorTxnFirstJobGet(txnId, &jobId);
    do
    {
        corTxnJobInfoDisplay(txnId, jobId);
    } while(clCorTxnNextJobGet(txnId, jobId, &jobId) == CL_OK);
        
        clEventCookieGet(eventHandle, (void **)&cookie);
        if(cookie)
  	 clOsalPrintf("\n Cookie Recieved is %d \n\n", *cookie);

   clCorTxnIdTxnFree(txnId); 

}



ClRcT
cor_test_per_object_memory_usage()
{
#define TEST_CLASS_U 0x4500
#define TEST_CLASS_U_ATTR_1 0x4501
#define TEST_CLASS_U_ATTR_2 0x4502
#define TEST_CLASS_U_ATTR_3 0x4503
#define TEST_CLASS_U_ATTR_4 0x4504

    ClRcT   rc = CL_OK;
    ClCorMOClassPathT moPath;
    ClCorMOIdT  moId;


    clOsalPrintf("\n \n <<<<<<<<<<<<<<<<<<<  Test case to test the memory usage for per object >>>>>>>>>>>>>>>>>>>>>>\n");
    rc = clCorClassCreate(TEST_CLASS_U, 0x0);

    rc = clCorClassNameSet(TEST_CLASS_U, "TEST_CLASS_U");

#if 0
    rc = clCorClassAttributeCreate(TEST_CLASS_U, TEST_CLASS_U_ATTR_1, CL_COR_UINT8);
#endif

#if 1
    rc = clCorClassAttributeCreate(TEST_CLASS_U, TEST_CLASS_U_ATTR_2, CL_COR_UINT64);
#endif

#if 1    
    rc = clCorClassAttributeArrayCreate(TEST_CLASS_U, TEST_CLASS_U_ATTR_3, CL_COR_UINT8, 256 );
#endif
  
#if 1
    rc = clCorClassAttributeCreate(TEST_CLASS_U, TEST_CLASS_U_ATTR_4, CL_COR_UINT32);
#endif

    clCorMoClassPathInitialize(&moPath);
    clCorMoClassPathAppend(&moPath, TEST_CLASS_U);

    rc = clCorMOClassCreate(&moPath, 10000000);

    clCorMoIdInitialize(&moId); 
    clCorMoIdAppend(&moId, TEST_CLASS_U, 0);
    ClCorObjectHandleT objH = {{0}};
    ClUint32T i = 0;

    for (i = 0; i < 8 ; i++)
    {
        clCorMoIdInstanceSet(&moId, 1, i);

        rc = clCorObjectCreate(NULL, &moId, &objH);
        if (CL_OK != rc)
        {
            clOsalPrintf("[%s][%d]: FAILED : Creating the object instance [%d]. rc[0x%x] \n", 
                    __FUNCTION__, __LINE__, i, rc);
            return rc;
        }
    }

    return rc;
}
ClEventSubscriptionIdT generateSubscriptionId()
{
	static ClEventSubscriptionIdT id = 0;
	return id++;
}
	
ClRcT clCorTestClientEventInit()
{
	ClRcT rc;
	ClNameT evtChannelName;
        ClVersionT ver = CL_EVENT_VERSION;

	CL_FUNC_ENTER();
	
	/* First call the function to initialize COR events */
	rc = clEventInitialize(&corTestEventHandle,&corTestEvtCallbacks, &ver);

	if(CL_OK != rc)
		{
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Event Client init failed rc => [0x%x]\n", rc));
		return rc;
		}

	/* Open event channel */
	/* First construct the event channel */
	evtChannelName.length = strlen(clCorEventName);
	memcpy(evtChannelName.value, clCorEventName, evtChannelName.length+1);
		
	clEventChannelOpen(corTestEventHandle, &evtChannelName, 
			CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, 
			COR_EVT_TIMEOUT, 
			&corTestEventChannelHandle);
	
	if(CL_OK != rc)
	{
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Event Client init failed rc => [0x%x]\n", rc));
		return rc;
	}

    CL_FUNC_EXIT();
    return (rc);
}


ClRcT testAppTerminate(ClInvocationT invocation,
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

ClRcT testAppFinalize()
{
    clOsalPrintf("\nSome finalization related stuff is supposed to put at this place\n");
    clEventChannelClose(corTestEventChannelHandle);
    clEventFinalize(corTestEventHandle);
    return CL_OK;
}

ClRcT testAppStateChange(ClEoStateT eoState)
{
    clOsalPrintf("\nSome state change related stuff\n");
    return CL_OK;
}

ClRcT testAppHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    return CL_OK;
}

ClRcT testAppCSISetCallback (
                    CL_IN ClInvocationT invocation,
                    CL_IN const ClNameT *pCompName,
                    CL_IN ClAmsHAStateT haState,
                    CL_IN ClAmsCSIDescriptorT csiDescriptor )
{
    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;
}


ClRcT testAppCSIRmvCallback (
                        CL_IN ClInvocationT invocation,
                        CL_IN const ClNameT *pCompName,
                        CL_IN const ClNameT *pCsiName,
                        CL_IN ClAmsCSIFlagsT csiFlags)
{
    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;

}

/*** Application Init ****/
ClRcT  testAppInitialize(ClUint32T argc, ClCharT *argv[])
{
	ClVersionT                  corVersion;

    ClRcT clCorClientInitialize();
    ClRcT corClientEventInit();
	
    clCorTestClientEventInit();
    clCorTestTxnInterfaceInit();

    /* CPM related initialization */
     corVersion.releaseCode = 'B';
     corVersion.majorVersion = 0x1;
     corVersion.minorVersion = 0x1;

/*****   Create basic Infomation model ***********/

/*** START OF TEST CASES ****/

#if 1
       cor_test_IM1();
#endif

#if 0
       cor_test_class_AttrWalk();
#endif

#if 0
     /* Following test case works on /0x100:0 tree  */
       cor_test_eventIM1();
#endif

#if 0
     /* Following test case works on /0x100:1 tree  */
       cor_test_txnIM1(); 

#endif

#if 0
	   cor_test_Association();
#endif

#if 0
     /* Following test case works on /0x100:2 tree  */
       cor_test_txnEventIM1(); 

     /* Here we get the values set in cor_test_txnEventIM1.
        DEPENDENCY 
     */
        cor_test_Get(); 
#endif

#if 0
       /* Following test case works on /0x100:3 tree  */
         cor_test_TreeWalkIM1(); 
#endif

#if 0
        /* Following are the test Cased for testing the Attribute Walk */
        cor_test_AttributeWalk();
#endif


#if 0
        /* Route list related test cases*/
        cor_test_route_list();
#endif

#if 0 
        /* MoId To Node Name and vice versa */
        cor_test_moId_nodeName_get();
#endif

#if 0
    /* Function to test the mso object create */
    cor_test_cor_mo_mso_create();

#endif

#if 0
        /* Adding the service rule to test its syncup.*/
        cor_test_svc_rule_add();
#endif

#if 0
        /* Creating lot of objects .. */
        cor_test_create_object();
#endif

#if 0

#if 0 
    /* Function to set the attribute for large number of times.*/
    cor_test_set_attribute();
#endif
    /* Function to create and delete the object to test the Db Lock in COR.*/
    cor_test_object_create_delete();

#endif 

#if 0
       /* Version Verification */
       cor_test_version_verify();
#endif


#if 0
       cor_test_moIdname_2_moid_get();

#endif

#if 0
    ClHandleT                   dbgHandle = 0;
    cor_client_debug_func_test(&dbgHandle);
#endif

#if 0
    cor_client_debug_func_deregister_test(dbgHandle);
#endif

#if 0
    cor_test_oi_register();
        
#endif


#if 0
    cor_test_moId_2_moidNameGet();
#endif


#if 0
    cor_test_attr_offset_limit_check();
#endif

#if 0
    cor_test_object_tree_locking_TC1();
#endif

#if 0
    cor_test_object_tree_locking_TC2();
#endif

#if 0
    cor_test_delays_get();
#endif

#if 0
    cor_test_failed_jobs_get();
#endif

#if 0
    cor_test_retry_initialize_time_requests();
#endif


#if 0
    cor_test_moidName_2_moid_get();
#endif

#if 0
    cor_test_per_object_memory_usage();
#endif

/*** END OF TEST CASES ****/
  return CL_OK;
}

ClEoConfigT clEoConfig = {
                   "asp_corTestClient",                      /* EO Name*/
                    CL_COR_TEST_CLIENT_EO_THREAD_PRIORITY, /* EO Thread Priority */
                    CL_COR_TEST_CLIENT_EO_THREADS,         /* No of EO thread needed */
                    CL_COR_TEST_CLIENT_IOC_PORT,           /* Required Ioc Port */
                    CL_EO_USER_CLIENT_ID_START,
                    CL_EO_USE_THREAD_FOR_RECV,              /* Whether to use main thread for eo Recv or not */
                    testAppInitialize,                     /* Function CallBack  to initialize the Application */
                    testAppFinalize,                       /* Function Callback to Terminate the Application */
                    testAppStateChange,                    /* Function Callback to change the Application state */
                    testAppHealthCheck,                    /* Function Callback to change the Application state */
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
    CL_TRUE,                   /* debug */
    CL_FALSE                    /* gms */
};
