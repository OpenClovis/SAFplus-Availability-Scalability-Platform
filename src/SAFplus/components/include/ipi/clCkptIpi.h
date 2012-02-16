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
 * ModuleName  : include
 * File        : clCkptIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *   This file contains Checkpoint service IPIs.
 *
 *
 ********************************************************************/
/********************************************************************/
/*
 *    pageCkpt101 : clCkptActiveReplicaSetSwitchOver
 *
 ********************************************************************/


#ifndef _CL_CKPT_IPI_H_
#define _CL_CKPT_IPI_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clIocApi.h>

/**
 ************************************
 *  \page pageCkpt101 clCkptActiveReplicaSetSwitchOver 
 *
 *  \par Synopsis:
 *   IPI for setting the backup SD as ActiveReplica for AMF related
 *   Checkpoint in case of Switch over.
 *
 *  \par Description:
 *  This IPI is used to update the active replica address for AMF
 *  related checkpoint in case of failover.
 *  NOTE - THIS IPI IS MEANT TO BE USED BY AMF ONLY
 *  
 *
 *  \par Syntax:
 *  \code 	extern ClRcT clCkptActiveReplicaSetSwitchOver(
 *                        CL_IN ClCkptHdlT checkpointHandle);
 *  \endcode   
 *    
 *  \param checkpointHandle: Checkpoint handle returned as part of
 *  clCkptCheckpointOpen.
 *
 *  \retval CL_OK: The IPI executed successfully.
 *
 */

extern ClRcT clCkptActiveReplicaSetSwitchOver(
    CL_IN ClCkptHdlT checkpointHandle);

extern ClRcT clCkptSectionOverwriteLinear(CL_IN  ClCkptHdlT        checkpointHandle,
                                          CL_IN const ClCkptSectionIdT    *sectionId,
                                          CL_IN const void             *dataBuffer,
                                          CL_IN ClSizeT                 dataSize);

extern ClRcT clCkptCheckpointWriteLinear(ClCkptHdlT                     ckptHdl,
                                         const ClCkptIOVectorElementT   *pIoVector,
                                         ClUint32T                      numberOfElements,
                                         ClUint32T                      *pError);

extern ClRcT clCkptSectionCheck(ClCkptHdlT ckptHdl, ClCkptSectionIdT *pSectionId);

extern ClBoolT clCkptDifferentialCheckpointStatusGet(void);

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_CKPT_IPI_H_*/
