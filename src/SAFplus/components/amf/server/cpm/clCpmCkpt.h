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

/**
 * This header file contains definition of all the internal data types
 * and data structures used by the CPM for checkpointing.
 */

#ifndef _CL_CPM_CKPT_H_
#define _CL_CPM_CKPT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/*
 * CPM internal include files 
 */
#include <clCpmInternal.h>
#include <clAmsTypes.h>

typedef struct
{
    ClUint32T TYPE;
    ClUint32T LENGTH;
} ClCpmTlvT;

enum
{
    CL_CPM_CPML = 1
};

extern ClRcT cpmCkptCpmLDatsSet(void);
    
extern ClRcT cpmCpmLActiveCheckpointInitialize(void);
    
extern ClRcT cpmCpmLStandbyCheckpointInitialize(void);

extern ClRcT cpmCpmLCheckpointRead(void);
    
#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CPM_CKPT_H_ */
