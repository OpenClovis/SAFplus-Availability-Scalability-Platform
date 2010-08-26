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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emCont/emCont.h $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_CONT_H_
# define _CL_CONT_H_

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

    typedef struct clEvtContPubKey
    {
        ClNameT appName;
        ClNameT channelName;
    } ClEvtContPubKey;

    typedef struct clEvtContSubKey
    {
        ClUint32T filterNo;
        ClNameT channelName;
    } ClEvtContSubKey;

    typedef struct ClEvtContSubInfoStorage
    {
        ClNameT appName;
        ClUint32T noOfSubs;
    } ClEvtContSubInfoStorageT;

    extern ClCntHandleT gEvtContSubInfo;
    extern ClCntHandleT gEvtContPubInfo;
    extern ClCntHandleT gEvtContSubTempInfo;

    extern ClRcT clEvtContAppAddressGet(void);

    extern ClRcT clEvtContRest(ClIocAddressT destAddr);
    extern ClRcT clEvtContSubResultGet(ClIocAddressT destAddr,
                                       ClUint32T noOfSubs);
    extern ClRcT clEvtContValidateResult();

    extern void clEvtContGetApp(ClUint32T *pNoOfApp);
    extern void clEvtContSubInfoInc(ClNameT *appName);
    extern void clEvtContAppInfoReset();

    extern ClRcT clEvtContParseTestInfo();

    extern ClRcT clEvtContInit(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);
    extern ClRcT clEvtContFin(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);

    extern ClRcT clEvtContOpen(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);
    extern ClRcT clEvntContClose(ClEvtContTestHeadT *pTestHead,
                                 ClRcT *pRetCode);

    extern ClRcT clEvntContSub(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);
    extern ClRcT clEvntContUnsub(ClEvtContTestHeadT *pTestHead,
                                 ClRcT *pRetCode);

    extern ClRcT clEvntContPub(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);
    extern ClRcT clEvntContAlloc(ClEvtContTestHeadT *pTestHead,
                                 ClRcT *pRetCode);

    extern ClRcT clEvntContSetAttr(ClEvtContTestHeadT *pTestHead,
                                   ClRcT *pRetCode);
    extern ClRcT clEvntContGetAttr(ClEvtContTestHeadT *pTestHead,
                                   ClRcT *pRetCode);

    extern ClRcT clEvntContKill(ClEvtContTestHeadT *pTestHead, ClRcT *pRetCode);
    extern ClRcT clEvntContRestart(ClEvtContTestHeadT *pTestHead,
                                   ClRcT *pRetCode);

    extern ClRcT clEvtContIocAddreGet(ClNameT *appName,
                                      ClIocPhysicalAddressT *pIocAddress);
    extern void clEvtContResultPrint(ClRcT retCode,
                                     ClEvtContTestHeadT *pTestHead);
    extern ClInt32T clEvtContUtilsNameCmp(ClNameT *pName1, ClNameT *pName2);

# ifdef __cplusplus
}
# endif


#endif                          /* _CL_CONT_H_ */
