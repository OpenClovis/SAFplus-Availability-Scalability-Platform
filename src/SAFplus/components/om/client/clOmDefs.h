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
 * ModuleName  : om
 * File        : clOmDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the definitions local to the Object Management       
 * modules.                                                                  
 *****************************************************************************/
#ifndef _CL_OM_DEFS_H_
#define _CL_OM_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */

/* DEFINES */
#define INST_BITS_UNUSABLE		0x00000000 
#define INST_BITS_OM_ALLOC		0x00000001
#define INST_BITS_EXT_ALLOC		0x00000002
#define INST_BITS_RESEV2		0x00000003
#define INST_BITS_MASK 	        0x00000003
#define INST_BITS_ADDR_MASK     (~0x03)	

#define mALLOC_BY_OM(X)		    ((ClWordT)X & INST_BITS_OM_ALLOC)
#define mALLOC_BY_EXT(X)	    ((ClWordT)X & INST_BITS_EXT_ALLOC)
#define mGET_REAL_ADDR(X)       ((ClWordT)X & INST_BITS_ADDR_MASK)

#define mSET_ALLOC_BY_OM(X)     ((ClWordT)X | INST_BITS_OM_ALLOC)
#define mSET_ALLOC_BY_EXT(X)    ((ClWordT)X | INST_BITS_EXT_ALLOC)

/* Flags passed into the common routine that adds/creates an object instance */
enum {
	CL_OM_CREATE_FLAG,
	CL_OM_ADD_FLAG,
	CL_OM_DELETE_FLAG,
	CL_OM_REMOVE_FLAG
};

#ifdef __cplusplus
}
#endif

#endif /* _CL_OM_DEFS_H_ */


