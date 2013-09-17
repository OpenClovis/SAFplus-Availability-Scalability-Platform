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
 * File        : clEventUtilsIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header provides the interface to the Utilities used
 *          by the EM modules.
 *
 *
 *****************************************************************************/

#ifndef __EVT_UTILS_H_
# define __EVT_UTILS_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include "clEventApi.h"
#include <clEventCommonIpi.h>

    ClUint32T clEvtUtilsIsLittleEndian(void);

    ClRcT clContNodeAddAndNodeGet(ClCntHandleT containerHandle,
                                  ClCntKeyHandleT userKey,
                                  ClCntDataHandleT userData, ClRuleExprT *pExp,
                                  ClCntNodeHandleT *pNodeHandle);
    ClRcT clEvtSubscribeInfoWalk(ClCntKeyHandleT userKey,
                                 ClCntDataHandleT userData,
                                 ClCntArgHandleT userArg, ClUint32T dataLength);
    ClRcT clEvtTypeInfoWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                            ClCntArgHandleT userArg, ClUint32T dataLength);
    ClRcT clEvtECHInfoWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                           ClCntArgHandleT userArg, ClUint32T dataLength);

    ClRcT clEvtSubsInfoShowLocal(ClUint32T cData,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);
    void clEvtUtilsNameCpy(SaNameT *pNameDst, const SaNameT *pNameSrc);
    ClRcT clEvtUtilsFilter2Rbe(const ClEventFilterArrayT *pFilterArray,
                               ClRuleExprT **pRbeExpr);

    void clEvtUtilsChannelKeyConstruct(ClUint32T channelHandle,
                                       SaNameT *pChannelName,
                                       ClEvtChannelKeyT *pEvtChannelKey);
    ClRcT clEvtUtilsFlatPattern2Rbe(void *pData, ClUint32T noOfPattern,
                                    ClRuleExprT **pRbeExpr);
    ClRcT clEvtUtilsFlatPattern2FlatBuffer(void *pData, ClUint32T noOfPattern,
                                           ClUint8T **ppData,
                                           ClUint32T *pDataLen);
    ClInt32T clEvtUtilsNameCmp(SaNameT *pName1, SaNameT *pName2);

# ifdef __cplusplus
}
# endif

#endif                          /* __EVT_UTILS_H_ */
