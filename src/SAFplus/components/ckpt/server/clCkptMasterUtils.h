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
 * ModuleName  : ckpt                                                          
File        : clCkptMasterUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint  service IPIs
*
*
*****************************************************************************/
#ifndef _CKPT_MAST_UTILS_H_
#define _CKPT_MAST_UTILS_H_
#include "clCkptSvr.h"
#include "clCkptDs.h"
#include "clCkptPeer.h"
#include <clDebugApi.h>
#include <clCkptCommon.h>

#include <xdrCkptUpdateFlagT.h>
#include "xdrCkptXlationDBEntryT.h"
#include "xdrCkptMasterDBInfoIDLT.h"
#include "xdrCkptMasterDBEntryIDLT.h"
#include "xdrCkptPeerListInfoT.h"
#include "xdrCkptMasterDBClientInfoT.h"
#ifdef __cplusplus
extern "C" {
#endif


/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/


#define CL_CKPT_SOURCE_DEPUTY 0  /* Caller is deputy */
#define CL_CKPT_SOURCE_MASTER 1  /* Caller is master */



/**====================================**/
/**      E X T E R N S                 **/
/**====================================**/

extern ClRcT
_ckptMasterHdlInfoFill(ClHandleT                           masterHdl,
                       ClNameT                             *pName,
                       ClCkptCheckpointCreationAttributesT *pCreateAttr,
                       ClIocNodeAddressT                   localAddr,
                       ClUint8T                            source,
                       ClIocNodeAddressT                   *pActAddr);
extern ClRcT
_ckptClientHdlInfoFill(ClHandleT masterHdl,
                       ClHandleT clientHdl,
                       ClUint8T  source);
extern ClRcT 
_ckptMasterPeerListHdlsAdd(ClHandleT         clientHdl,
                           ClHandleT         masterHdl,
                           ClIocNodeAddressT nodeAddr,
                           ClIocPortT        portId,
                           ClUint32T        openFlag,
                           ClCkptCreationFlagsT creationFlags);
extern ClRcT 
_ckptMasterXlationDBEntryAdd(ClNameT   *pName,
                             ClUint32T  cksum,
                             ClHandleT  masterHdl);

extern ClRcT 
_ckptPeerListMasterHdlAdd( ClHandleT         masterHdl,
                           ClIocNodeAddressT oldActAddr,
                           ClIocNodeAddressT newActAddr);
#ifdef __cplusplus
}
#endif
#endif 
