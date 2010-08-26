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

#include <clAmsTestApi.h>

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

typedef struct ClAmsTestCases
{
    ClAmsMgmtHandleT amsMgmtHandle;
    ClOsalMutexT amsTestCaseLock;
    ClListHeadT amsTestCaseList;
    ClUint32T numTestCases;
}ClAmsTestCasesT;

typedef struct ClAmsTestCase
{
    const ClAmsTestT *pAmsTest;
    ClInt32T numTests;
    const ClCharT *pName;
    ClListHeadT amsTestList;
    ClBoolT amsTestContinueOnFailure;
    ClAmsTestTypeT amsTestType;
    struct timeval amsTestStartTime;
    struct timeval amsTestEndTime;
    ClUint64T amsTestTotalTime;
    ClUint32T numRuns;
} ClAmsTestCaseT;


static ClAmsTestCasesT gClAmsTestCases = {
    .amsTestCaseList = CL_LIST_HEAD_INITIALIZER(gClAmsTestCases.amsTestCaseList),
   .numTestCases = 0,
};

static void clAmsTestTime(struct timeval *pStart,
                          struct timeval *pEnd,
                          ClUint64T *pTotal)
{
    if(pStart == NULL)
    {
        return ;
    }
    if(pEnd == NULL)
    {
        memset(pStart,0,sizeof(*pStart));
        gettimeofday(pStart,NULL);
        return;
    }
    gettimeofday(pEnd,NULL);
    pEnd->tv_usec -= pStart->tv_usec;
    pEnd->tv_sec -=  pStart->tv_sec;
    if(pEnd->tv_usec < 0 )
    {
        pEnd->tv_usec += 1000000L;
        --pEnd->tv_sec;
    }
    if(pEnd->tv_sec < 0 )
    {
        pEnd->tv_usec -= 1000000L;
        ++pEnd->tv_sec;
    }
    if(pEnd->tv_sec < 0 )
    {
        pEnd->tv_sec = 0;
        pEnd->tv_usec = 0;
        return;
    }
    if(pTotal)
    {
        *pTotal += pEnd->tv_sec*1000000L + pEnd->tv_usec;
    }
}

static ClAmsTestCaseT *clAmsTestFind(const ClCharT *pName)
{
    ClListHeadT *pHead;
    register ClListHeadT *pTemp;
    pHead = &gClAmsTestCases.amsTestCaseList;

    CL_LIST_FOR_EACH(pTemp,pHead)
    {
        ClAmsTestCaseT *pTestCase = CL_LIST_ENTRY(pTemp,ClAmsTestCaseT,amsTestList);
        if(!strcmp(pTestCase->pName,pName))
        {
            return pTestCase;
        }

    }
    return NULL;
}

static ClAmsTestCaseT *clAmsTestFindLock(const ClCharT *pName)
{
    ClAmsTestCaseT *pTestCase;
    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    pTestCase = clAmsTestFind(pName);
    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    return pTestCase;
}

ClRcT clAmsTestRegister(const ClCharT *pName,
                        ClAmsTestTypeT amsTestType,
                        const ClAmsTestT *pAmsTest,
                        ClInt32T numTests,
                        ClBoolT amsTestContinueOnFailure
                        )
{
    ClRcT rc = CL_AMS_TEST_RC(CL_ERR_ALREADY_EXIST);
    ClAmsTestCaseT *pTestCase = NULL;

    pTestCase = clAmsTestFindLock(pName);
    if(pTestCase != NULL)
    {
        goto out;
    }
    pTestCase = calloc(1,sizeof(*pTestCase));
    CL_ASSERT(pTestCase != NULL);
    pTestCase->pAmsTest = pAmsTest;
    pTestCase->numTests = numTests;
    pTestCase->pName = pName;
    pTestCase->amsTestContinueOnFailure = amsTestContinueOnFailure;
    pTestCase->amsTestType = amsTestType;
    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    clListAddTail(&pTestCase->amsTestList,&gClAmsTestCases.amsTestCaseList);
    ++gClAmsTestCases.numTestCases;
    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clAmsTestDeregister(const ClCharT *pName)
{
    ClAmsTestCaseT *pTestCase;
    ClRcT rc = CL_AMS_TEST_RC(CL_ERR_NOT_EXIST);

    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    pTestCase = clAmsTestFind(pName);
    if(pTestCase == NULL)
    {
        clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
        goto out;
    }
    clListDel(&pTestCase->amsTestList);
    --gClAmsTestCases.numTestCases;
    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    free(pTestCase);
    rc = CL_OK;

    out:
    return rc;
}

/*
 * Make a procedural run.
 * Fire:
 * a) Load
 * b) Run
 * d) Verify
 * c) Reset
 */

ClRcT clAmsTestRun(ClAmsTestTypeT type,
                   const ClAmsTestT *pAmsTest,
                   ClInt32T numTests)
{
    ClRcT rc = CL_OK;
    register ClInt32T i;
    ClBoolT loadOnce = pAmsTest->amsTestLoadOnce;
    struct timeval amsTestStartTime;
    struct timeval amsTestEndTime;
    ClUint64T amsTestTotalTime;

    for(i = 0; i < numTests;++i)
    {
        if(!i || loadOnce == CL_FALSE)
        {
            if(pAmsTest->pAmsTestLoad)
            {
                rc = pAmsTest->pAmsTestLoad(gClAmsTestCases.amsMgmtHandle,
                                            pAmsTest,
                                            numTests);
                if(rc != CL_OK)
                {
                    goto out_reset;
                }
            }
        }
        /*
         * Fire the run.
         */
        if(pAmsTest->pAmsTestRun)
        {
            CL_AMS_TEST_TIME(&amsTestStartTime,NULL,NULL);
            rc = pAmsTest->pAmsTestRun(gClAmsTestCases.amsMgmtHandle,
                                       pAmsTest,
                                       numTests);
            CL_AMS_TEST_TIME(&amsTestStartTime,&amsTestEndTime,&amsTestTotalTime);
            if(rc != CL_OK && pAmsTest->amsTestContinueOnFailure == CL_FALSE)
            {
                goto out_reset;
            }

            if(pAmsTest->pAmsTestDelay)
            {
                pAmsTest->pAmsTestDelay(pAmsTest,numTests);
            }
            else
            {
                if(type == CL_AMS_TEST_TYPE_ASYNC)
                {
                    ClTimerTimeOutT delay = {.tsSec=0,.tsMilliSec=100};
                    clOsalTaskDelay(delay);
                }
            }
        }
        /*
         * Verify
         */
        if(pAmsTest->pAmsTestVerify)
        {
            rc = pAmsTest->pAmsTestVerify(gClAmsTestCases.amsMgmtHandle,
                                          pAmsTest,
                                          numTests);
            if(rc != CL_OK)
            {
                goto out_reset;
            }
        }

        if(loadOnce == CL_FALSE && pAmsTest->pAmsTestReset)
        {
            rc = pAmsTest->pAmsTestReset(gClAmsTestCases.amsMgmtHandle,
                                         pAmsTest,
                                         numTests);
            if(rc != CL_OK)
            {
                CL_AMS_TEST_PRINT(("AMS Test reset failed for Test [%s]\n",pAmsTest->pAmsTestEntityName));
            }
        }
    }
    if(loadOnce == CL_TRUE)
    {
        goto out_reset;
    }
    goto out;

    out_reset:
    if(pAmsTest->pAmsTestReset)
    {
        pAmsTest->pAmsTestReset(gClAmsTestCases.amsMgmtHandle,pAmsTest,numTests);
    }
    out:
    return rc;
}

ClRcT clAmsTestRunByName(ClAmsTestTypeT type,const ClCharT *pName)
{
    ClAmsTestCaseT *pTestCase = NULL;
    ClRcT rc = CL_AMS_TEST_RC(CL_ERR_NOT_EXIST);
    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    pTestCase = clAmsTestFind(pName);
    if(pTestCase == NULL)
    {
        clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
        CL_AMS_TEST_PRINT(("Test Find failed for name [%s]\n",pName));
        goto out;
    }

    CL_AMS_TEST_CASE_START(("%s ",pTestCase->pName));

    rc = clAmsTestRun(type,pTestCase->pAmsTest,pTestCase->numTests);

    CL_AMS_TEST_CASE_END(("%s ",pTestCase->pName));

    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Test Run failed for test [%s] with error [0x%x]\n",pName,rc));
        goto out;
    }
    out:
    return rc;
}

/*
 * Fire the AMS test cases.
 */
ClRcT clAmsTestCasesRun(void)
{
    ClListHeadT *pHead  = &gClAmsTestCases.amsTestCaseList;
    register ClListHeadT *pTemp;
    ClRcT rc = CL_OK;

    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    CL_LIST_FOR_EACH(pTemp,pHead)
    {
        ClAmsTestCaseT *pTestCase = CL_LIST_ENTRY(pTemp,ClAmsTestCaseT,amsTestList);
        /*
         * Run this test case.
         */
        CL_AMS_TEST_CASE_START(("%s ",pTestCase->pName));

        CL_AMS_TEST_TIME(&pTestCase->amsTestStartTime,NULL,NULL);
        rc = clAmsTestRun(pTestCase->amsTestType,
                          pTestCase->pAmsTest,
                          pTestCase->numTests);
        CL_AMS_TEST_TIME(&pTestCase->amsTestStartTime,&pTestCase->amsTestEndTime,&pTestCase->amsTestTotalTime);

        CL_AMS_TEST_CASE_END(("%s ",pTestCase->pName));

        ++pTestCase->numRuns;
        if(rc != CL_OK)
        {
            CL_AMS_TEST_PRINT(("TestCase [%s] failed to run with error [0x%x]\n",pTestCase->pName,rc));
        }
        if(pTestCase->amsTestContinueOnFailure == CL_FALSE)
        {
            break;
        }
    }
    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    return rc;
}

#if defined (CL_AMS_MGMT_HOOKS)
static ClRcT clAmsMgmtEntityAdminResponse(ClAmsEntityTypeT type,
                                          ClAmsMgmtAdminOperT oper,
                                          ClRcT retCode)
{
    clOsalPrintf("Response received for type [0x%x], oper [0x%x], retCode [0x%x]\n",type,oper,retCode);
    return CL_OK;
}
#endif

ClRcT clAmsTestInitialize(void)
{
    ClRcT rc;
    static ClVersionT version = { 'B',1,1 };
    static ClAmsMgmtCallbacksT callbacks;

#if defined (CL_AMS_MGMT_HOOKS)
    callbacks.pEntityAdminResponse=clAmsMgmtEntityAdminResponse;
#endif

    rc = clAmsMgmtInitialize(&gClAmsTestCases.amsMgmtHandle,
                             (const ClAmsMgmtCallbacksT*)&callbacks,
                             &version);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("clAmsMgmtInitialize returned with error [0x%x]\n",rc));
        goto out;
    }

    rc = clOsalMutexInit(&gClAmsTestCases.amsTestCaseLock);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("clOsalMutexInit returned with error [0x%x]\n",rc));
        goto out;
    }

    CL_LIST_HEAD_INIT(&gClAmsTestCases.amsTestCaseList);
    out:
    return rc;
}

ClRcT clAmsTestFinalize(void)
{
    ClRcT rc;
    ClListHeadT *pHead = &gClAmsTestCases.amsTestCaseList;
    clOsalMutexLock(&gClAmsTestCases.amsTestCaseLock);
    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pTemp = pHead->pNext;
        ClAmsTestCaseT *pTestCase = CL_LIST_ENTRY(pTemp,ClAmsTestCaseT,amsTestList);
        clListDel(pTemp);
        free(pTestCase);
    }
    gClAmsTestCases.numTestCases = 0;
    clOsalMutexUnlock(&gClAmsTestCases.amsTestCaseLock);
    clOsalMutexDestroy(&gClAmsTestCases.amsTestCaseLock);

    rc = clAmsMgmtFinalize(gClAmsTestCases.amsMgmtHandle);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Ams Mgmt Finalize returned with error [0x%x]\n",rc));
    }
    return rc;
}

