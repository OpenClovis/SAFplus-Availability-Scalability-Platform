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
 * File        : clCorNotify.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains definitions for related to COR object change notification
 *
 *
 *****************************************************************************/

#ifndef _INC_COR_NOTIFY_H_
#define _INC_COR_NOTIFY_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCorServiceId.h>
#include <clCorTxnApi.h>
#include <clCorTxnClientIpi.h>
#include <clEventApi.h>

/* Internal includes */
#include <clCorNotifyCommon.h>


/* PROTOTYOPES */
extern ClRcT corEventInit(void);
extern ClRcT corEventFinalize(void);

ClRcT
corEventPublish (ClCorOpsT                  op,
                 ClCorMOIdPtrT              pMoId,
                 ClCorAttrPathPtrT          pAttrPath,
                 ClCorAttrBitsT            *pChangedBits,
                 ClUint8T                  *pPayload,
                 ClUint32T                  payloadSize);

#ifdef __cplusplus
}
#endif

#endif  /*  _INC_COR_NOTIFY_H_ */
