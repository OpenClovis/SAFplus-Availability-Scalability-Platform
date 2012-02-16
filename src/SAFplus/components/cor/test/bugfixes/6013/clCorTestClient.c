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
#include <clLogApi.h>

#include <clCorTestClient.h>

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

    corTest();

    return rc;
}

ClRcT corTest()
{
    ClCorMOIdT moId;
    ClNameT moIdStr;
    ClUint32T i = 0;
    ClCorObjectHandleT objH;
    ClRcT rc = CL_OK;

    clCorMoIdInitialize(&moId);

    for (i=0; i<4096; i++)
    {
        clOsalPrintf("\nCreating the MO [%d]\n", i);

        sprintf(moIdStr.value, "\\Chassis:0\\CorFunTestL1:0\\corFunMOTransL3:%d", i);
        moIdStr.length = strlen(moIdStr.value);

        rc = clCorMoIdNameToMoIdGet(&moIdStr, &moId);
        if (rc != CL_OK)
        {
            clLogError("", "", "Failed to translate moId name to moId. rc [0x%x]", rc);
            return rc;
        }

        clCorMoIdShow(&moId);

        rc = clCorUtilMoAndMSOCreate(&moId, &objH);
        if (rc != CL_OK)
        {
            clLogError("", "", "Failed to create the object. rc [0x%x]", rc);
            return rc;
        }
    }

    return (CL_OK);
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




