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
 * $File: //depot/dev/RC2/ASP/components/cor/test/clCorTestClient.c $
 * $Author: bkpavan $
 * $Date: 2006/06/07 $
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
#include <clCorTxnApi.h>

#include <clCorTestClient.h>

/**************************************************************************************************************/

/* Application Init */

ClRcT corIMCreate();
ClRcT clCorTestClientDataSave();
ClRcT testAppInitialize(ClUint32T argc, ClCharT *argv[]);
ClRcT testAppTerminate(ClInvocationT invocation,
                        const SaNameT *compName);
ClRcT testAppHealthCheck(ClEoSchedFeedBackT *schFeedback);
ClRcT testAppStateChange(ClEoStateT eoState);
ClRcT testAppFinalize();

ClRcT cor_test_cor_mo_mso_create();
ClRcT cor_test_set_attribute();
ClRcT cor_test_object_create_delete();

ClRcT testAppInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClEoExecutionObjT *pEoHandle = NULL;
    ClRcT rc = CL_OK;
    SaNameT appName = {0};
    ClCpmCallbacksT callbacks = {0};
    ClIocPortT iocPort = {0};
    ClVersionT corVersion = {0};

    clEoMyEoObjectGet(&pEoHandle);
    clCorClientInitialize();
   
    corVersion.releaseCode = 'B';
    corVersion.majorVersion = 0x1;
    corVersion.minorVersion = 0x1;

    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = testAppTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    clEoMyEoIocPortGet(&iocPort);
    clOsalPrintf("\nApplication address 0x%x port 0x%x\n", clIocLocalAddressGet(), iocPort);

    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &corVersion);
    clOsalPrintf("\nAfter cpmClientInitialize %d : rc [0x%x]\n", cpmHandle, rc);

    rc = clCpmComponentNameGet(cpmHandle, &appName);
    clOsalPrintf("\nComponent name : %s\n", appName.value);

    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
    
    /* Execute the test cases */

    corIMCreate();

    /*
     * Creating 20000 objects at each level and check the access time.
     */

#if 0
    cor_test_cor_mo_mso_create();
#endif

   /* Function to set the attribute for large number of times.*/
    cor_test_set_attribute();

#if 0
    cor_test_object_create_delete();
#endif

    return rc;
}

ClRcT corIMCreate()
{
    ClCorMOClassPathPtrT moPath;  
    ClRcT               rc = CL_OK;

   /* Uint64T type declarations */
   rc = clCorClassCreate(TEST_CLASS_A, 0);
   if(rc != CL_OK)
        return rc;
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
   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT, TEST_CLASS_B);

   clCorMoClassPathAppend(moPath, TEST_CLASS_D);
   clCorMOClassCreate(moPath, 100);

   clCorMSOClassCreate(moPath, CL_COR_SVC_ID_DUMMY_MANAGEMENT, TEST_CLASS_C);

   clCorMoClassPathSet(moPath, 4, TEST_CLASS_E);
   clCorMOClassCreate(moPath, 100);
   
   return (CL_OK);
}

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
    clOsalPrintf( "TEST :  [%s] \n", __FUNCTION__); 
    rc = clCorObjectCreate(NULL, pMoId1, &objH);
    ch[0] = '\0';
    clOsalPrintf( " \t TEST1 : Object creation ... \\AClass:5 . Result [%s] \n", rc? "FAIL":"PASS"); 

    clCorMoIdAppend(pMoId1, TEST_CLASS_B, 4);
    
    rc = clCorObjectCreate(NULL, pMoId1, &objH);
    ch[0] = '\0';
    clOsalPrintf( " \t TEST2 : Object creation ... \\AClass:5\\BClass:4 . Result [%s] \n", rc? "FAIL":"PASS"); 

    clCorMoIdAppend(pMoId1, TEST_CLASS_C, 5);
    clCorMoIdClone(pMoId1, &pMoId2);
    
    rc = clCorUtilMoAndMSOCreate(pMoId1, &objH); 
    ch[0] = '\0';
    clOsalPrintf( " \t TEST3 : Mo and Mso creation ... \\AClass:5\\BClass:4\\Cclass:5 . Result [%s] \n", rc? "FAIL":"PASS"); 
    
    compResult = clCorMoIdCompare(pMoId1, pMoId2);
    ch[0] = '\0';
    clOsalPrintf( " \t TEST4 : clCorUtilMOMsoCreate() test . Result [%s] \n", (compResult == 0)? "PASS":"FAIL"); 

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
    ClUint32T  value32 = 0;
    ClUint64T  value64 = 0, i=0;

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
        clOsalPrintf( "Creating the object \\AClass:6. rc [0x%x] Result [%s] \n",rc, rc?"FAIL":"PASS");
        if(rc != CL_OK)
            return rc;
    }

    for(i = 0; i < 50000; i++)
    {
        tid = 0;
        value32 = 23;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_5, 0, (void *)&value32, size32);

        value16 = 24;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_6, -1, (void *)&value16, size16);
    
        value64 = 25;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_9, -1, (void *)&value64, size64);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_1, -1, (void *)&value8, size8);

        rc = clCorTxnSessionCommit(tid);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Committing the transaction ... rc[0x%x]", rc));
        ch[0] ='\0';
        clOsalPrintf( "Committing the session. time [%lld],. rc[0x%x], Result[%s] \n", i, rc, rc?"FAIL":"PASS");
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the txn commit ... rc[0x%x]", rc));
            clCorTxnSessionFinalize(tid);
        }

#if 0
        tid = 0;
        value32 = 23;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_5, 0, (void *)&value32, size32);

        value16 = 24;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_6, -1, (void *)&value16, size16);
    
        value64 = 25;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_9, -1, (void *)&value64, size64);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_2, -1, (void *)&value8, size8);

        value8 = 12;
        rc = clCorObjectAttributeSet(&tid, objH, NULL, TEST_CLASS_A_ATTR_10, -1, (void *)&value8, size8);

        rc = clCorTxnSessionCommit(tid);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Committing the transaction ... rc[0x%x]", rc));
        ch[0] ='\0';
        clOsalPrintf( "Committing the session. time [%lld],. rc[0x%x], Result[%s] \n", i, rc, rc?"FAIL":"PASS");
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the txn commit ... rc[0x%x]", rc));
            clCorTxnSessionCancel(tid);
        }
#endif        
   }

    ch[0] ='\0';
    clOsalPrintf( " [%s]. Result[PASS] \n", __FUNCTION__);
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
    inst = 8;
    
    clCorMoIdAppend(&moId, TEST_CLASS_A, inst);

    for (i = 0; i< 100000; i++)
    {

        rc = clCorObjectHandleGet(&moId, &objH);
        if(rc != CL_OK)
        {
            rc = clCorObjectCreate(NULL, &moId, &objH);
            ch[0] ='\0';
            clOsalPrintf( "Creating the object \\AClass:%d. rc [0x%x] Result [%s] \n",inst, rc, rc?"FAIL":"PASS");
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object. rc[0x%x]",rc));
                return rc;
            }
        }

        rc = clCorObjectDelete(NULL, objH);
        ch[0] ='\0';
        clOsalPrintf( "Deleting the object \\AClass:%d. rc [0x%x] Result [%s] \n",inst, rc, rc?"FAIL":"PASS");
        if(rc != CL_OK)
	{
	    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the object. rc[0x%x]", rc));
            return rc;	
	}
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Created and deleted the object .... [%d]", i));
    }
    
    ch[0] ='\0';
    clOsalPrintf( "[%s] Result [PASS] \n", __FUNCTION__);

    return rc;
}

ClRcT testAppFinalize()
{
    clOsalPrintf("\nFinalize called ...\n");
    return CL_OK;
}

ClRcT testAppStateChange(ClEoStateT eoState)
{
    clOsalPrintf("\nApp State change called...\n");
    return CL_OK;
}

ClRcT testAppHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    return CL_OK;
}

ClRcT testAppTerminate(ClInvocationT invocation,
                    const SaNameT *compName)
{
    ClRcT rc = CL_OK;

    clOsalPrintf("\nInside App Terminate\n");
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);

    rc = clCpmClientFinalize(cpmHandle);

    return rc; 
}

ClEoConfigT clEoConfig = {
                "corTestClient2",
                1,
                4,
                0x8550,
                CL_EO_USER_CLIENT_ID_START,
                CL_EO_USE_THREAD_FOR_RECV,
                testAppInitialize,
                testAppFinalize,
                testAppStateChange,
                testAppHealthCheck,
};

ClUint8T clEoBasicLibs[] = {
    CL_TRUE, /* osal*/
    CL_TRUE, /* timer*/
    CL_TRUE, /* buffer*/
    CL_TRUE, /* ioc*/
    CL_TRUE, /* rmd*/
    CL_TRUE, /* eo*/
    CL_FALSE, /* om*/
    CL_FALSE, /* hal*/
    CL_FALSE, /* dbal */
};

ClUint8T clEoClientLibs[] = {
   CL_TRUE,  /* cor */
   CL_FALSE, /* cm */
   CL_FALSE, /* name */
   CL_FALSE, /* log */
   CL_FALSE, /* trace */
   CL_FALSE, /* diag */
   CL_TRUE, /* txn */
   CL_FALSE, /* hpi */
   CL_FALSE, /* cli */
   CL_FALSE, /* alarm */
   CL_FALSE, /* debug */
   CL_FALSE, /* gms */
};




