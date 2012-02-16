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
 * File        : clOmErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * The file contains all the error Ids related to Object Manager(OM)
 *
 *
 *****************************************************************************/

#ifndef _CL_OM_ERRORS_H_
#define _CL_OM_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCommonErrors.h>

/* DEFINES */
/* OM-specific error IDs */
#define CL_OM_ERR_INTERNAL			  -1

/**
 * The OM class id specified is invalid.
 */
#define CL_OM_ERR_INVALID_CLASS           0xff

/**
 * The OM object handle is invalid.
 */
#define	CL_OM_ERR_INVALID_OBJ_HANDLE      0x100

/**
 * The instance id or the no. of instances specified is invalid.
 */
#define	CL_OM_ERR_INVALID_OBJ_INSTANCE	  0x101

/**
 * The OM object reference is invalid.
 */
#define	CL_OM_ERR_INVALID_OBJ_REF         0x102

/**
 * Maximum no. of instances specified is invalid.
 */
#define	CL_OM_ERR_INVALID_MAX_INSTANCE	  0x103

/**
 * Instance table is already allocated for the class. 
 */
#define	CL_OM_ERR_INSTANCE_TAB_EXISTS     0x104

/**
 * Instance table for the class doesn't exist.
 */
#define	CL_OM_ERR_INSTANCE_TAB_NOT_EXISTS 0x105

/**
 * The instance table is already freed for the class.
 */
#define	CL_OM_ERR_INSTANCE_TAB_DUPFREE    0x106

/**
 * The instance table is full.
 */
#define	CL_OM_ERR_INSTANCE_TAB_OVERFLOW   0x107

/**
 * There are no contiguous slots available for the no. of instances specified.
 */
#define	CL_OM_ERR_INSTANCE_TAB_NOSLOTS    0x108

/**
 * The instance table for the class is not yet initialized.
 */
#define	CL_OM_ERR_INSTANCE_TAB_NOINIT     0x109

/**
 * Allocated buffer is not on word aligned boundary.
 */
#define	CL_OM_ERR_ALOC_BUF_NOT_ALIGNED    0x10a

/**
 * The length of class name specified exceeds the limit.
 */
#define	CL_OM_ERR_MAX_NAMEBUF_EXCEEDED    0x10b

/**
 * Common class table is not available.
 */
#define	CL_OM_ERR_NO_COMMON_CLASS_TABLE   0x10c

/**
 * The OM object being deleted is not allocated by OM.
 */
#define	CL_OM_ERR_OBJ_NOT_ALLOCATED_BY_OM 0x10d

/**
 * The OM object is being removed by user which is allocated by OM.
 */
#define	CL_OM_ERR_OBJ_ALLOCATED_BY_OM     0x10e

/**
 * NULL pointer specified.
 */
#define	CL_OM_ERR_NULL_PTR 	     			CL_ERR_NULL_POINTER

/**
 * No memory available.
 */
#define CL_OM_ERR_NO_MEMORY			 		CL_ERR_NO_MEMORY

#define	CL_OM_ERR_MAX						/*add value*/				

/* Error macro definitions for OM */
#define CL_OM_SET_RC(ERR_ID)	(CL_RC(CL_CID_OM, ERR_ID))

/* FORWARD DECLARATION */

#ifdef __cplusplus
}
#endif

#endif	/*	 _CL_OM_ERRORS_H_ */
