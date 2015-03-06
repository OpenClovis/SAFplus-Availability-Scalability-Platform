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
 * File        : clEvtCliCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header contains the Command Line Interface related info
 *          for the EM API.
 *
 *
 *****************************************************************************/

#ifndef _EVT_CLI_OMMON_H_
# define _EVT_CLI_OMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

# define EO_ID_EVT_TEST_MAN CL_IOC_ASP_RESERVERD_COMMPORT_END+123

# define EO_EVT_TEST_INIT               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1)
# define EO_EVT_TEST_FINALIZE           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2)
# define EO_EVT_TEST_CHANNEL_OPEN       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,3)
# define EO_EVT_TEST_EVENT_SUBSCRIBE    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,4)
# define EO_EVT_TEST_EVENT_UNSUBSCRIBE  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,5)
# define EO_EVT_TEST_EVENT_PUBLISH      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,6)
# define EO_EVT_TEST_CHANNEL_CLOSE      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,7)
# define EO_EVT_TEST_AIS                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,8)
# define EO_EVT_TEST_SHOW               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,9)

typedef struct EvtTestSuppSetReq
{
    ClEventChannelHandleT channelHandle;
    SaNameT publisherName;

} EvtTestSuppSetReq_t;

typedef struct EvtTestSuppClearReq
{
    ClEventChannelHandleT channelHandle;

} EvtTestSuppClearReq_t;

typedef struct EvtTestInitReq
{
    ClUint32T dummy;

} EvtTestInitReq_t;

typedef struct EvtTestOpenReq
{
    ClUint32T flag;
    SaNameT channelName;

} EvtTestOpenReq_t;

typedef struct EvtTestCloseReq
{
    ClEventChannelHandleT channelHandle;

} EvtTestCloseReq_t;

typedef struct EvtTestSubsReq
{
    ClEventChannelHandleT channelHandle;
    ClUint32T subscriptionId;

} EvtTestSubsReq_t;

typedef struct EvtTestUnsubsReq
{
    ClEventChannelHandleT channelHandle;
    ClUint32T subscriptionId;

} EvtTestUnsubsReq_t;

typedef struct EvtTestPublishReq
{
    ClEventChannelHandleT channelHandle;
    SaNameT publisherName;

} EvtTestPublishReq_t;

#ifdef __cplusplus
}
#endif
#endif
