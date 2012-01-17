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

extern ClRcT clCkptSectionCheck(ClCkptHdlT ckptHdl, ClCkptSectionIdT *pSectionId);

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_CKPT_IPI_H_*/
