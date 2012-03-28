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
 * ModuleName  : amf                                                           
 * File        : clAmsNotifications.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains the data structure and definitons used for AMS notification 
 * service
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_NOTIFICATIONS_H_
#define _CL_AMS_NOTIFICATIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clEventApi.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>
#include <clAmsClientNotification.h>

extern ClRcT
clAmsNotificationEventInitialize(void);

extern ClRcT
clAmsNotificationEventFinalize(void);

extern ClRcT
clAmsNotificationEventPayloadSet(const ClAmsEntityT *entity,
                                 const ClAmsEntityRefT *entityRef,
                                 const ClAmsHAStateT lastHAState,
                                 const ClAmsNotificationTypeT ntfType,
                                 ClAmsNotificationDescriptorT *notification);
extern ClRcT
clAmsNotificationEventPublish(
        CL_IN  ClAmsNotificationDescriptorT  *notification );

extern ClRcT 
clAmsNotificationMarshalNtfDescr (
        CL_IN  ClAmsNotificationDescriptorT  *ntf,
        CL_OUT  ClPtrT  *data,
        CL_INOUT  ClUint32T  *msgLength,
        CL_INOUT  ClBufferHandleT  out );

extern ClRcT
clAmsMgmtCCBNotificationEventPayloadSet(ClAmsNotificationTypeT type,
                                        const ClAmsEntityT *pEntity,
                                        ClAmsNotificationDescriptorT *pNotification);

extern ClRcT
clAmsOperStateNotificationPublish(ClAmsEntityT *pEntity, ClAmsOperStateT lastOperState,
                                  ClAmsOperStateT newOperState);

extern ClRcT
clAmsAdminStateNotificationPublish(ClAmsEntityT *pEntity, ClAmsAdminStateT lastAdminState,
                                   ClAmsAdminStateT newAdminState);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_NOTIFICATIONS_H_ */
