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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/common/emAutoCommon.h $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_AUTO_COMMON_H_
# define _CL_AUTO_COMMON_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include "clCommon.h"
# include "clCommonErrors.h"
# include "clOsalApi.h"
# include "clCntApi.h"
# include "clCommon.h"
# include "clEventErrors.h"
# include "clEventApi.h"
# include "clEventExtApi.h"

    /*
     ** Following MACRO should be used only with string literals enclosed in 
     ** "" (e.g "NAME"). Passing pointers will result in unexpected behavior.
     */
# define CL_NAME_SET(STR_LITERAL) { sizeof(STR_LITERAL)-1, STR_LITERAL }


    /*
     * Representing RMD functions exposed by EM using proper macros 
     */
# define EM_TEST_APP_INIT        CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x1 )
# define EM_TEST_APP_FIN         CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x2 )
# define EM_TEST_APP_OPEN        CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x3 )
# define EM_TEST_APP_CLOSE       CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x4 )
# define EM_TEST_APP_SUB         CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x5 )
# define EM_TEST_APP_UNSUB       CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x6 )
# define EM_TEST_APP_PUB         CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x7 )
# define EM_TEST_APP_ALLOC       CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x8 )
# define EM_TEST_APP_ATTR_SET    CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0x9 )
# define EM_TEST_APP_ATTR_GET    CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0xA )
# define EM_TEST_APP_RESULT_GET  CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0xB )
# define EM_TEST_APP_RESTART     CL_EO_GET_FULL_FN_NUM( CL_EO_NATIVE_COMPONENT_TABLE_ID, 0xC )


    typedef enum ClEvtContTestOp
    {
        CL_EVT_CONT_INIT,
        CL_EVT_CONT_FIN,
        CL_EVT_CONT_OPEN,
        CL_EVT_CONT_CLOSE,
        CL_EVT_CONT_SUB,
        CL_EVT_CONT_UNSUB,
        CL_EVT_CONT_PUB,
        CL_EVT_CONT_ALLOC,
        CL_EVT_CONT_SET_ATTR,
        CL_EVT_CONT_GET_ATTR,
        CL_EVT_CONT_KILL,
        CL_EVT_CONT_RESTART,

    } ClEvtContTestOpT;

    typedef struct ClChanKey 
    {
        ClNameT channelName;
        ClUint8T channelscope;
    
    } ClChanKeyT;
    
    typedef struct ClEvtContAppToIocAddr
    {
        ClNameT appName;
        ClIocPhysicalAddressT iocPhyAddr;
        ClUint32T noOfSubs;

    } ClEvtContAppToIocAddrT;

    typedef struct ClEvtContTestHead
    {
        ClNameT testAppName;
        ClEvtContTestOpT operation;
        ClRcT expectedResult;
        void *pTestInfo;

    } ClEvtContTestHeadT;

    typedef struct ClEvtContResult
    {
        ClUint32T priority;
        ClNameT pubName;
        ClUint32T noOfSubs;

    } ClEvtContResultT;

    typedef struct ClEvtContTestCase
    {
        ClNameT testCaseName;
        ClEvtContTestHeadT *pTestHead;
        ClUint32T noOfSteps;

    } ClEvtContTestCaseT;

    typedef struct ClEvtContChOpen
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClTimeT timeOut;

    } ClEvtContChOpenT;

    typedef struct ClEvtContChClose
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;

    } ClEvtContChCloseT;

    typedef struct ClEvtContSub
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClUint32T filterNo;
        ClEventSubscriptionIdT subId;
        void *pCookie;

    } ClEvtContSubT;

    typedef struct ClEvtContUnsub
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClEventSubscriptionIdT subId;

    } ClEvtContUnsubT;

    typedef struct ClEvtContAlloc
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;

    } ClEvtContAllocT;

    typedef struct ClEvtContAttrSet
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClUint32T pattNo;
        ClEventPriorityT priorityNo;
        ClTimeT retentionTime;
        ClNameT publisherName;

    } ClEvtContAttrSetT;

    typedef struct ClEvtContAttrGet
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClUint32T pattNo;
        ClEventPriorityT priorityNo;
        ClTimeT retentionTime;
        ClNameT publisherName;

    } ClEvtContAttrGetT;

    typedef struct ClEvtContPub
    {
        ClNameT initName;
        ClNameT channelName;
        ClUint32T openFlag;
        ClUint32T dataLen;
        ClUint8T payLoad[100];
        ClTimeT timeOut;

    } ClEvtContPubT;

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_AUTO_COMMON_H_ */
