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
#include <clLogApi.h>

/**************************************************************************************************************/

/* Application Init */

ClRcT clCorTestClientDataSave();
ClRcT testAppInitialize(ClUint32T argc, ClCharT *argv[]);
ClRcT testAppTerminate(ClInvocationT invocation,
                        const ClNameT *compName);
ClRcT testAppHealthCheck(ClEoSchedFeedBackT *schFeedback);
ClRcT testAppStateChange(ClEoStateT eoState);
ClRcT testAppFinalize();

ClRcT corTest();
ClRcT corIMCreate();
ClRcT corObjCreate(ClCorClassTypeT classId, ClCorInstanceIdT instId);
ClRcT corObjDelete(ClCorClassTypeT classId, ClCorInstanceIdT instId);
ClRcT clCorTestClientDataSave();

ClRcT testAppInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClEoExecutionObjT *pEoHandle = NULL;
    ClRcT rc = CL_OK;
    ClNameT appName = {0};
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
    
    clLogInfo("", "", "This test case will create and delete some number of objects. The user needs to verify in the CLI whether objcnt == (seq-1) in the dmshow for classes 0x100 and 0x200");

    corTest();

    return rc;
}

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

ClRcT corIMCreate()
{
    ClRcT rc = CL_OK;
    ClCorMOClassPathPtrT moPath;

    rc = clCorClassCreate(0x100, 0);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the COR class. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x101, CL_COR_UINT8);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x102, CL_COR_INT8);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x103, CL_COR_INT16);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x104, CL_COR_UINT16);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x105, CL_COR_INT32);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
   
    rc = clCorClassAttributeCreate(0x100, 0x106, CL_COR_UINT32);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }
    
    rc = clCorClassCreate(0x200, 0);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the class. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorClassAttributeCreate(0x200, 0x201, CL_COR_UINT32);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the attribute. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMoClassPathAlloc(&moPath);
    
    rc = clCorMoClassPathInitialize(moPath);

    rc = clCorMoClassPathAppend(moPath, 0x100);

    rc = clCorMOClassCreate(moPath, 100);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the MO class. rc [0x%x]", rc);
        clCorMoClassPathFree(moPath);
        return rc;
    }

    rc = clCorMoClassPathSet(moPath, 1, 0x200);
    rc = clCorMOClassCreate(moPath, 100);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the MO class. rc [0x%x]", rc);
        clCorMoClassPathFree(moPath);
        return rc;
    }

    clCorMoClassPathFree(moPath);

    clCorTestClientDataSave();

    return (CL_OK);
}

ClRcT corTest()
{
    ClUint32T i = 0;

    for (i=0; i<10; i++)
    {
        corObjCreate(0x100, i);
    }

    for (i=5; i<10; i++)
    {
        corObjDelete(0x100, i);
    }

    for (i=10; i<20; i++)
    {
        corObjCreate(0x100, i);
    }

    for (i=5; i<10; i++)
    {
        corObjCreate(0x100, i);
    }
    
    for (i=10; i<15; i++)
    {
        corObjDelete(0x100, i);
    }

    for (i=10; i<15; i++)
    {
        corObjCreate(0x100, i);
    }

    for (i=20; i<100; i++)
    {
        corObjCreate(0x100, i);
    }

    for (i=0; i<30; i++)
    {
        corObjCreate(0x200, i);
    }

    return (CL_OK);
}

ClRcT corObjCreate(ClCorClassTypeT classId, ClCorInstanceIdT instId)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;
    ClCorObjectHandleT objH;

    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, classId, instId);

    rc = clCorObjectCreate(NULL, &moId, &objH);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to create the object. rc [0x%x]", rc);
        return rc;
    }

    clLogInfo("", "", "**** Creating the object ... ****");
    clCorMoIdShow(&moId);

    return rc;
}

ClRcT corObjDelete(ClCorClassTypeT classId, ClCorInstanceIdT instId)
{
    ClCorMOIdT moId;
    ClRcT rc = CL_OK;
    ClCorObjectHandleT objH;

    clCorMoIdInitialize(&moId);

    clCorMoIdAppend(&moId, classId, instId);

    rc = clCorObjectHandleGet(&moId, &objH);

    rc = clCorObjectDelete(NULL, objH);
    if (rc != CL_OK)
    {
        clLogError("", "", "Failed to Delete the object. rc [0x%x]", rc);
        return rc;
    }

    clLogInfo("", "", "****** Deleting the object .. ");
    clCorMoIdShow(&moId);

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
                    const ClNameT *compName)
{
    ClRcT rc = CL_OK;

    clOsalPrintf("\nInside App Terminate\n");
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);

    rc = clCpmClientFinalize(cpmHandle);

    return rc; 
}

ClEoConfigT clEoConfig = {
                "corTestClient1",
                1,
                4,
                0x8450,
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




