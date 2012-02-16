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
#ifndef _CL_AMS_TEST_API_H_
#define _CL_AMS_TEST_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clCntApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsUtils.h>
#include <clTestApi.h>
#include <clList.h>

#undef CL_CID_AMS_TEST
#define CL_CID_AMS_TEST  (0x1000)

#define CL_AMS_TEST_TIME(TS,TE,TA) clAmsTestTime(TS,TE,TA)
#define CL_AMS_TEST_NAME(TYPE,ENTITY,OP,PHASE,TC) clAmsTest##TYPE##ENTITY##OP##PHASE##TC
#define CL_AMS_TEST_LOAD(TYPE,ENTITY,OP,TC) CL_AMS_TEST_NAME(TYPE,ENTITY,OP,Load,TC)
#define CL_AMS_TEST_RUN(TYPE,ENTITY,OP,TC) CL_AMS_TEST_NAME(TYPE,ENTITY,OP,Run,TC)
#define CL_AMS_TEST_VERIFY(TYPE,ENTITY,OP,TC) CL_AMS_TEST_NAME(TYPE,ENTITY,OP,Verify,TC)
#define CL_AMS_TEST_RESET(TYPE,ENTITY,OP,TC) CL_AMS_TEST_NAME(TYPE,ENTITY,OP,Reset,TC)
#define CL_AMS_TEST_DELAY(TYPE,ENTITY,OP,TC) CL_AMS_TEST_NAME(TYPE,ENTITY,OP,Delay,TC)
#define CL_AMS_TEST_RC(rc) CL_RC(CL_CID_AMS_TEST,rc)

#define CL_AMS_TEST_AMS_AREA "AMS-TEST"

#define CL_AMS_TEST_ENTITY_CONTEXT "AMS-TEST-ENTITY"

#define CL_AMS_TEST_PRINT clTestPrint

#define CL_AMS_TEST_GROUP_INITIALIZE clTestGroupInitialize

#define CL_AMS_TEST_GROUP_FINALIZE clTestGroupFinalize

#define CL_AMS_TEST_GROUP_MALFUNCTION clTestGroupMalfunction

#define CL_AMS_TEST_FAILED clTestFailed

#define CL_AMS_TEST_SUCCESS clTestSuccess

#define CL_AMS_TEST_CASE_START clTestCaseStart

#define CL_AMS_TEST_CASE_END clTestCaseEnd

#define CL_AMS_TEST clTest

#define CL_AMS_TEST_EXEC_ON_FAILURE clTestExecOnFailure

#define CL_AMS_TEST_CASE_MALFUNCTION(reason,predicate,execOnFailure) clTestCaseMalfunction(reason,predicate,execOnFailure)

    typedef enum ClAmsTestType
        {
            CL_AMS_TEST_TYPE_SYNC,
            CL_AMS_TEST_TYPE_ASYNC,
        }ClAmsTestTypeT;

    typedef struct ClAmsTest
    {
        const ClCharT *pAmsTestEntityName;
        ClBoolT amsTestLoadOnce;
        ClBoolT amsTestContinueOnFailure;
        ClRcT (*pAmsTestLoad)(ClAmsMgmtHandleT handle,const struct ClAmsTest *pAmsTest,ClInt32T numTests);
        ClRcT (*pAmsTestRun)(ClAmsMgmtHandleT handle, const struct ClAmsTest *pAmsTest,ClInt32T numTests);
        ClRcT (*pAmsTestVerify)(ClAmsMgmtHandleT handle,const struct ClAmsTest *pAmsTest,ClInt32T numTests);
        ClRcT (*pAmsTestReset)(ClAmsMgmtHandleT handle,const struct ClAmsTest *pAmsTest,ClInt32T numTests);
        void (*pAmsTestDelay)(const struct ClAmsTest *pAmsTest,ClInt32T numTests);
        ClPtrT pPrivate;
    }ClAmsTestT;
   

    typedef ClRcT (ClAmsTestOperationT)(ClAmsMgmtHandleT,const ClAmsTestT *,ClInt32T);

    typedef ClAmsTestOperationT ClAmsTestLoadT;
    typedef ClAmsTestOperationT ClAmsTestRunT;
    typedef ClAmsTestOperationT ClAmsTestVerifyT;
    typedef ClAmsTestOperationT ClAmsTestResetT;


    extern ClRcT clAmsTestRegister(const ClCharT *pName,
                                   ClAmsTestTypeT amsTestType,
                                   const ClAmsTestT *pAmsTest,
                                   ClInt32T numTests,
                                   ClBoolT amsTestContinueOnFailure);

    extern ClRcT clAmsTestDeregister(const ClCharT *pName);

    extern ClRcT clAmsTestCasesRun(void);

    extern ClRcT clAmsTestRun(ClAmsTestTypeT type,
                              const ClAmsTestT *pAmsTest,
                              ClInt32T numTests);

    extern ClRcT clAmsTestRunByName(ClAmsTestTypeT type,
                                    const ClCharT *pName);

    extern ClRcT clAmsTestInitialize(void);

    extern ClRcT clAmsTestFinalize(void);

#ifdef __cplusplus
}
#endif

#endif
