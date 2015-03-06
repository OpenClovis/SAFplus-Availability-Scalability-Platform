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
 * ModuleName  : message
 * File        : clMsgConsistentHash.h
 *******************************************************************************/
#ifndef _MSG_CONSISTENT_HASH_H
#define _MSG_CONSISTENT_HASH_H

#include <clCommon.h>
#include <clCommonErrors.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __MSG_QUEUE_GROUP_NODES (8)
#define __MSG_QUEUE_GROUP_HASHES_PER_NODE (160)
#define __MSG_QUEUE_GROUP_HASHES ( __MSG_QUEUE_GROUP_NODES * __MSG_QUEUE_GROUP_HASHES_PER_NODE )


/*
 * Initialize a consistent hash ring for a MSG queue group
 * and allocate a MsgQueueGroupHashesT to store the hash for receiver lookup
 */
ClRcT clMsgQueueGroupHashInit(ClInt32T nodes, ClInt32T hashesPerNode);

/*
 * Free memory allocated for consistent hash of a group
 */
ClRcT clMsgQueueGroupHashFinalize();

#ifdef __cplusplus
}
#endif

#endif
