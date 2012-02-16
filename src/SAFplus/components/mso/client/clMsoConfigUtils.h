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
 * ModuleName  : mso 
 * File        : clMsoConfigUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Mso configuration internal definitions 
 *          and function prototypes.
 *
 *****************************************************************************/

#ifndef _CL_MSO_CONFIG_UTILS_H_
#define _CL_MSO_CONFIG_UTILS_H_

#include <clCorTxnApi.h>
#include <clTxnAgentApi.h>

#ifdef __cplusplus
extern "C"
    {
#endif

#define CL_MSO_NUM_BUCKETS 8

ClRcT
clMsoConfigTxnRegistration();

ClRcT clMsoConfigTxnStart(
                ClTxnTransactionHandleT txnHandle,
                ClTxnAgentCookieT*      pCookie);

ClRcT clMsoConfigTxnEnd(
                ClTxnTransactionHandleT txnHandle,
                ClTxnAgentCookieT*      pCookie);

ClRcT        
clMsoConfigTxnValidate( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie );

ClRcT        
clMsoConfigTxnUpdate(   CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie );

ClRcT        
clMsoConfigTxnRollback( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie );

ClRcT        
clMsoConfigTxnRead( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie );

ClRcT clMsoLibInitialize();
ClRcT clMsoLibFinalize();


ClInt32T _clMsoDataCmpFunc(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
ClUint32T _clMsoDataHashFunc(ClCntKeyHandleT key);
void _clMsoDataDeleteFunc(ClCntKeyHandleT key, ClCntDataHandleT data);

#ifdef __cplusplus
}
#endif

#endif
