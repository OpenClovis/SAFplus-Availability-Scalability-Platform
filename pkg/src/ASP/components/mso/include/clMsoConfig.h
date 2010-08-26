/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : mso 
 * File        : clMsoConfigUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Mso configuration APIs and data structures.
 *
 *****************************************************************************/

#ifndef _CL_MSO_CONFIG_H_
#define _CL_MSO_CONFIG_H_

#ifdef __cplusplus
extern "C"
    {
#endif

#include <clCorMetaData.h>
#include <clCorNotifyApi.h>

#define CL_MSO_RC(ERR_ID) (CL_RC(CL_CID_MSO, ERR_ID))

typedef ClRcT (*ClMsoConfigCallbackT) (ClTxnTransactionHandleT txnHandle,
                        ClTxnJobDefnHandleT jobDefn,
                        ClUint32T jobDefnSize,
                        ClCorTxnIdT corTxnId);

typedef struct 
{
    ClMsoConfigCallbackT fpMsoJobPrepare;
    ClMsoConfigCallbackT fpMsoJobCommit;
    ClMsoConfigCallbackT fpMsoJobRollback;
    ClMsoConfigCallbackT fpMsoJobRead;
}ClMsoConfigCallbacksT;

ClRcT clMsoConfigRegister(ClCorServiceIdT svcId, ClMsoConfigCallbacksT msoCallbacks);
ClRcT
clMsoConfigResourceInfoGet( ClCorAddrT* pCompAddr,
                            ClOampRtResourceArrayT* pResourcesArray );
ClRcT
clMsoConfigResourceListGet(ClCorMOIdListT** ppMoIdList, ClCorServiceIdT svcId);

#ifdef __cplusplus
    }
#endif

#endif
