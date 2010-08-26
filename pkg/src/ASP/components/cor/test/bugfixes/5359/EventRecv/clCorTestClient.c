/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : cor
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/cor/test/clCorTestClient.c $
 * $Author: hargagan $
 * $Date: 2007/02/21 $
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

#include <clCorTestClient.h>

/********* FROM TESTING ********/

ClRcT clCorTestClientDataSave()
{
    ClRcT    rc = CL_OK;
    ClNameT   nodeName ;
    ClCharT   *classDbName = NULL;

    clCpmLocalNodeNameGet(&nodeName);

    classDbName = clHeapAllocate(nodeName.length + sizeof(".corClassDb"));
    if(classDbName == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Allocate Memory"));
        return CL_COR_ERR_NO_MEM;
    }
    memset(classDbName, 0, nodeName.length + sizeof(".corClassDb"));
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
              ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT flag, void *cookie)
{

   clCorAttrPathShow(pAttrPath);

    
   return 0;
}


/* Attribute Walk Test */
void cor_test_AttributeWalk()
{

   clCorClassCreate(TEST_CLASS_F, 0);
   clCorClassNameSet(TEST_CLASS_F, "TestClass_F");

  /* top class.. all classes inherit from it.*/
   clCorClassCreate(TEST_CLASS_G, 0);
   clCorClassNameSet(TEST_CLASS_G, "TestClass_G");
   /* 	SIMPLE */
   clCorClassAttributeCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_1, CL_COR_UINT32);

   clCorClassAttributeValueSet(TEST_CLASS_G, TEST_CLASS_G_ATTR_1, 1, 0 , 100);
   /* ARRAY */
   clCorClassAttributeArrayCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_2, CL_COR_UINT32, 10);
   /* ASSOCIATION */
   clCorClassAssociationCreate(TEST_CLASS_G, TEST_CLASS_G_ATTR_3, TEST_CLASS_F, 10);

   /* Base class - G*/
   clCorClassCreate(TEST_CLASS_H, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_H, "TestClass_H");
   clCorClassAttributeCreate(TEST_CLASS_H, TEST_CLASS_H_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_H, TEST_CLASS_H_ATTR_1, 7, 0 , 70);

   /* Base class - F*/
   clCorClassCreate(TEST_CLASS_I, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_I, "TestClass_I");
   clCorClassAttributeCreate(TEST_CLASS_I, TEST_CLASS_I_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_I, TEST_CLASS_I_ATTR_1, 6, 0 , 60);

   /* Base class - E (F,G)*/
   clCorClassCreate(TEST_CLASS_J, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_J, "TestClass_J");
   clCorClassAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_J, TEST_CLASS_J_ATTR_1, 5, 0 , 50);
   /* CONTAINMENT - F  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_2, TEST_CLASS_I, 0, 2);
   /* CONTAINMENT - G  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_J, TEST_CLASS_J_ATTR_3, TEST_CLASS_H, 0, 2);


   /* Base class - D (E, F)*/
   clCorClassCreate(TEST_CLASS_K, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_K, "TestClass_K");
   clCorClassAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_K, TEST_CLASS_K_ATTR_1, 4, 0 , 40);
   /* CONTAINMENT - E  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_2, TEST_CLASS_J, 0, 2);
   /* CONTAINMENT - F  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_K, TEST_CLASS_K_ATTR_3, TEST_CLASS_I, 0, 2);

   /* Base class - C (D, G)*/
   clCorClassCreate(TEST_CLASS_L, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_L, "TestClass_L");
   clCorClassAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_L, TEST_CLASS_L_ATTR_1, 3, 0 , 30);
   /* CONTAINMENT - D  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_2, TEST_CLASS_K, 0, 1);
   /* CONTAINMENT - G  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_L, TEST_CLASS_L_ATTR_3, TEST_CLASS_H, 0, 2);

   /* Base class - B*/
   clCorClassCreate(TEST_CLASS_M, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_M, "TestClass_M");
   clCorClassAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_M, TEST_CLASS_M_ATTR_1, 2, 0 , 20);
   /* CONTAINMENT - C  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_2, TEST_CLASS_L, 0, 1);
   /* CONTAINMENT - E  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_M, TEST_CLASS_M_ATTR_3, TEST_CLASS_J, 0, 1);

   /* Base class - A (B, D)*/
   clCorClassCreate(TEST_CLASS_N, TEST_CLASS_G);
   clCorClassNameSet(TEST_CLASS_N, "TestClass_N");
   clCorClassAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_N, TEST_CLASS_N_ATTR_1, 1, 0 , 10);
   /* CONTAINMENT - B  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_2, TEST_CLASS_M, 0, 1);
   /* CONTAINMENT - D  */
   clCorClassContainmentAttributeCreate(TEST_CLASS_N, TEST_CLASS_N_ATTR_3, TEST_CLASS_K, 0, 2);

   ClCorMOClassPathPtrT moPath;
   clCorMoClassPathAlloc(&moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_N);
   clCorMOClassCreate(moPath, 5);

    clCorTestClientDataSave();
 
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

   /* Uint64T type declarations */
   clCorClassCreate(TEST_CLASS_A, 0);
   clCorClassNameSet(TEST_CLASS_A, "Aclass");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, CL_COR_INT8);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, 10, (ClCharT)0x80, 0x7f); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_1, "A");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, CL_COR_UINT8);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, 10, 0, 0xff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_2, "Aa");

   clCorClassAttributeArrayCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_3, CL_COR_UINT64, 15);
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_3, "Aaa");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, CL_COR_UINT32);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, 10, 0x0, 0xffffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_4, "Aaaa");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, CL_COR_INT32);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, 10, (ClInt32T)0x80000000, 0x7fffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_5, "Aaaa");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, CL_COR_INT16);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, 0x30, (ClInt16T)0x8000, 0x7fff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_6, "Aaaaaa");

   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, CL_COR_UINT16);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, 0xffff, 0, 0xffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_7, "Aaaaa");


   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, CL_COR_INT64);
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, "uint64");
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_8, 0xffff, 0x8000000000000000, 0x7fffffffffffffff); 


   clCorClassAttributeCreate(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, CL_COR_UINT64);
   clCorClassAttributeValueSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, 0xffff, 0, 0xffffffffffffffff); 
   clCorClassAttributeNameSet(TEST_CLASS_A, TEST_CLASS_A_ATTR_9, "Int64");

   clCorClassCreate(TEST_CLASS_B, 0);
   clCorClassNameSet(TEST_CLASS_B, "Bclass");

   clCorClassAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, "B");
   clCorClassAttributeValueSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_1, 20, 0, 2000); 

   clCorClassAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, "Bb");
   clCorClassAttributeValueSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_2, 20, 0, 2000); 
   /* Containment attr*/
   clCorClassContainmentAttributeCreate(TEST_CLASS_B, TEST_CLASS_B_ATTR_3, TEST_CLASS_A, 0, 5);
   clCorClassAttributeNameSet(TEST_CLASS_B, TEST_CLASS_B_ATTR_3, "Bbb"); 


   clCorClassCreate(TEST_CLASS_C, 0);
   clCorClassNameSet(TEST_CLASS_C, "Cclass");

   clCorClassAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, "C");
   clCorClassAttributeValueSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_1, 30, 0, 3000); 

   clCorClassAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, "Cc");
   clCorClassAttributeValueSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_2, 30, 0, 3000); 
 
   /* Containment attr*/
   clCorClassContainmentAttributeCreate(TEST_CLASS_C, TEST_CLASS_C_ATTR_3, TEST_CLASS_B, 0, 10);
    clCorClassAttributeNameSet(TEST_CLASS_C, TEST_CLASS_C_ATTR_3, "Ccc"); 

   clCorClassCreate(TEST_CLASS_D, 0);
   clCorClassNameSet(TEST_CLASS_D, "Dclass");

   clCorClassAttributeCreate(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, "D");
   clCorClassAttributeValueSet(TEST_CLASS_D, TEST_CLASS_D_ATTR_1, 40, 0, 4000); 

   clCorClassCreate(TEST_CLASS_E, 0);
   clCorClassNameSet(TEST_CLASS_E, "Eclass");
   clCorClassAttributeCreate(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, CL_COR_UINT32);
   clCorClassAttributeNameSet(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, "C");
   clCorClassAttributeValueSet(TEST_CLASS_E, TEST_CLASS_E_ATTR_1, 50, 0, 5000); 

  /* MO Class Tree (blue print) */
   clCorMoClassPathAlloc(&moPath);
   clCorMoClassPathAppend(moPath, TEST_CLASS_A);
   clCorMOClassCreate(moPath, 100);
 
   clCorMoClassPathAppend(moPath, TEST_CLASS_B);
   clCorMOClassCreate(moPath, 100);

   clCorMoClassPathAppend(moPath, TEST_CLASS_C);
   clCorMOClassCreate(moPath, 100);

   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_C);

   clCorMoClassPathAppend(moPath, TEST_CLASS_D);
   clCorMOClassCreate(moPath, 100);

   clCorMoClassPathSet(moPath, 4, TEST_CLASS_E);
   clCorMOClassCreate(moPath, 100);
    
}

ClRcT classAttrWalk(ClCorClassTypeT clsId, ClCorAttrDefT *attrDef, ClHandleT cookie)
{
    ClCharT  ch[100] = {0};

    sprintf(ch , " \t\t ClassId 0x%x , attrId 0x%x, attrType %d", clsId, attrDef->attrId, attrDef->attrType);    
    CL_COR_TEST_PUT_IN_FILE(ch);
    ch[0] = '\0';
    sprintf(ch , "\t\t attrValues: init 0x%llx, min 0x%llx, max 0x%llx ", attrDef->u.simpleAttrVals.init, 
                            attrDef->u.simpleAttrVals.min, attrDef->u.simpleAttrVals.max);
    CL_COR_TEST_PUT_IN_FILE(ch);
    return CL_OK; 
}


ClRcT cor_test_class_AttrWalk()
{
    ClRcT rc = CL_OK; 

    rc = clCorClassAttributeWalk(TEST_CLASS_A, classAttrWalk, 1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x100.", rc); 

    rc = clCorClassAttributeWalk(TEST_CLASS_B, classAttrWalk, 1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x200.", rc); 

    rc = clCorClassAttributeWalk(TEST_CLASS_C, classAttrWalk, 1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"clCorClassAttrWalk for the class 0x300.", rc); 

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

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCalling service Rule Add \n"));

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 3);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Append MoId.", rc);
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Add Service Rule. for MoId \\0x1001:3", rc);

    
    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_N, 1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Append MoId.", rc);
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR,"Add Service Rule. for MoId \\0x1001:3\\0x1001:1", rc);

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_M, 2);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Append MoId.", rc);
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Add Service Rule. for MoId \\0x1001:3\\0x1001:1\0x1002:2", rc);

    rc = clCorMoIdAppend(moIdEvent, TEST_CLASS_L, 3);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Append MoId.", rc);
    rc = clCorServiceRuleAdd(moIdEvent, iocAdd1);
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Add Service Rule. for MoId \\0x1001:3\\0x1001:1\0x1002:2\\0x1003:3", rc);



    rc = clCorServiceRuleAdd(moIdEvent, iocAdd2); 
    CL_COR_TEST_RETURN_ERROR(CL_DEBUG_ERROR," Add Service Rule. for MoId \\0x1001:3\\0x1001:1\0x1002:2\\0x1003:3", rc);


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
   rc = clCorObjectAttributeSet(&tid, handle, pAttrPath, TEST_CLASS_A_ATTR_5, 0, (void *)&value32, size32);
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
   ClUint16T  value16 = 0;
   ClUint32T  value32 = 0;
   ClUint64T  value64 = 0;
   ClCharT    ch[100] = {0};

   ClUint32T  size16  = sizeof(ClUint16T);
   ClUint32T  size32  = sizeof(ClUint32T);
   ClUint32T  size64  = sizeof(ClUint64T);


   rc = clCorObjectAttributeGet(handle, NULL, TEST_CLASS_C_ATTR_1, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x301 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet(handle, NULL, TEST_CLASS_C_ATTR_2, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch ,"For attrId 0x302 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
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
   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch ," For attrId 0x101 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);


   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x102 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   /* AttrPath 5 */
   clCorAttrPathSet(pAttrPath, 2, TEST_CLASS_B_ATTR_3, 1);
   size32 = sizeof(ClUint32T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_1, -1, (void *)&value32, &size32);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x101 Value obtained is %d, size  %d, rc[0x%x]", value32, size32, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);

   size64 = sizeof(ClUint64T);
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_2, -1, (void *)&value64, &size64);
   ch[0] ='\0';
   sprintf(ch, "For attrId 0x102 Value obtained is %lld, size  %d, rc[0x%x]", value64, size64, rc);
   CL_COR_TEST_PUT_IN_FILE(ch);
   size64 = sizeof(ClUint64T);

   ClUint64T  arrayValue[11] = {0};
   ClUint32T size = sizeof(arrayValue);
   rc = clCorObjectAttributeGet(handle, pAttrPath, TEST_CLASS_A_ATTR_3, 0, (void *)arrayValue, &size);
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
   rc = clCorObjectAttributeGet( handle, pAttrPath, TEST_CLASS_A_ATTR_4, 0, (void *)&value32, &size32);
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
    ClCorTxnIdT txnId;
    ClCorTxnJobIdT jobId = 0;

   clOsalPrintf("\n\n\n           ################## TRANSACTION PREPARE --  number %d ###################### \n", ++i);
       
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


/***************   EVENT RELATE STUFF IS HERE *************/
void corTestEvtChannelOpenCallBack( ClInvocationT invocation, 
        ClEventChannelHandleT channelHandle, ClRcT error )
{

}

void corTestEventDeliverCallBack( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
//        static int evntno;
//   clOsalPrintf("\n\n\n           ################## EVENT RECEIVED --  number %d ###################### \n", ++evntno);

#if 1
        static int evntno;
	ClCorMOIdT moId;
        ClUint32T  *cookie = NULL;
        ClCorTxnIdT txnId;
        ClCorTxnJobIdT jobId = 0;
        ClCorAttrPathPtrT pAttrPath;
        ClCorOpsT op;
        ClCorAttrIdT attrId;
        ClInt32T index;
        void* value;
        ClUint32T size;

   clOsalPrintf("\n\n\n           ################## EVENT RECEIVED --  number %d ###################### \n", ++evntno);
       
   clCorEventHandleToCorTxnIdGet(eventHandle, eventDataSize,  &txnId);
   clCorTxnJobMoIdGet(txnId, &moId);

    clCorTxnFirstJobGet(txnId, &jobId);
    do
    {
       clOsalPrintf("\n\t\t\tMoId  .......... ");
       clCorMoIdShow(&moId);

       clCorTxnJobOperationGet(txnId, jobId, &op);
       clOsalPrintf("\n Op : %d\n", op);

       clCorTxnJobAttrPathGet(txnId, jobId, &pAttrPath);
       clCorAttrPathShow(pAttrPath);

       clCorTxnJobSetParamsGet(txnId, jobId, &attrId, &index, &value, &size);
 
       clOsalPrintf("Attr Id : 0x%x\n", attrId);

       clEventCookieGet(eventHandle, (void **)&cookie);

        if(cookie)
  	        clOsalPrintf("\n Cookie Recieved is %d \n\n", *cookie);
   
    } while(clCorTxnNextJobGet(txnId, jobId, &jobId) == CL_OK);


   clCorTxnIdTxnFree(txnId); 
   clEventFree(eventHandle);
#endif

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
    ClEoExecutionObjT  *pEoHandle;
    ClRcT rc;
	ClNameT                     appName;
	ClCpmCallbacksT             callbacks;
	ClIocPortT                  iocPort;
	ClVersionT                  corVersion;

    clEoMyEoObjectGet(&pEoHandle);   
    ClRcT clCorClientInitialize();
    ClRcT corClientEventInit();
	
    clCorTestClientEventInit();
    clCorTestTxnInterfaceInit();

    /* CPM related initialization */
     corVersion.releaseCode = 'B';
     corVersion.majorVersion = 0x1;
     corVersion.minorVersion = 0x1;

     callbacks.appHealthCheck = NULL;
     callbacks.appTerminate = testAppTerminate;
     callbacks.appCSISet = testAppCSISetCallback;
     callbacks.appCSIRmv = testAppCSIRmvCallback;
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

/*****   Create basic Infomation model ***********/

    ClCorMOIdPtrT moId;
    
    clCorMoIdAlloc(&moId);
    clCorMoIdAppend(moId, 0x100, 6);
    
   rc = clCorEventSubscribe(corTestEventChannelHandle, moId, NULL, NULL, CL_COR_OP_SET, NULL, generateSubscriptionId()); 
   clCorMoIdFree(moId);

/*** START OF TEST CASES ****/

#if 0
#if 1
       cor_test_IM1();
#endif

#if 1
       cor_test_class_AttrWalk();
#endif

#if 1
     /* Following test case works on /0x100:0 tree  */
       cor_test_eventIM1();
#endif

#if 1
     /* Following test case works on /0x100:1 tree  */
       cor_test_txnIM1(); 

#endif

#if 1
	   cor_test_Association();
#endif

#if 1
     /* Following test case works on /0x100:2 tree  */
       cor_test_txnEventIM1(); 

     /* Here we get the values set in cor_test_txnEventIM1.
        DEPENDENCY 
     */
        cor_test_Get(); 
#endif

#if 1
       /* Following test case works on /0x100:3 tree  */
         cor_test_TreeWalkIM1(); 
#endif

#if 1
        /* Following are the test Cased for testing the Attribute Walk */
        cor_test_AttributeWalk();
#endif


#if 1
        /* Route list related test cases*/
        cor_test_route_list();
#endif

#if 1 
        /* MoId To Node Name and vice versa */
        cor_test_moId_nodeName_get();
#endif

#if 1
       /* Version Verification */
       cor_test_version_verify();
#endif
#endif

/*** END OF TEST CASES ****/

  return rc;
}

ClEoConfigT clEoConfig = {
                   "corTestEvent",                      /* EO Name*/
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
    CL_FALSE,                   /* debug */
    CL_FALSE                    /* gms */
};
