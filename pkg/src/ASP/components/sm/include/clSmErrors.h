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
 * ModuleName  : sm
 * File        : clSmErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Machine template definitions 
 *************************************************************************/


#ifndef _CL_SM_ERRORS_H
#define _CL_SM_ERRORS_H


#ifdef __cplusplus
extern "C" {
#endif


#include <clCommonErrors.h>


#define CL_SM_SPECIFIC_ERR_ID_OFFSET    CL_ERR_COMMON_MAX

#define CL_SM_RC(ERR_ID)                (CL_RC(0, ERR_ID))

/* Common error IDs, derived from defines in clovisError.h */

/**
 *
 */
#define	SM_ERR_INVALID_CLASS		     (CL_SM_RC(0x100))

/**
 *
 */
#define	SM_ERR_NO_SEMA			         (CL_SM_RC(0x101))

/**
 *
 */
#define	SM_ERR_INVALID_EOID		         (CL_SM_RC(0x102))

/**
 *
 */
#define	SM_ERR_INVALID_EOHANDLE	         (CL_SM_RC(0x103))

/* basic SM related error codes */

/**
 *
 */
#define	SM_ERR_EXIT_FAILED	             (CL_SM_RC(0x104))

/**
 *
 */
#define	SM_ERR_OUT_OF_RANGE	             (CL_SM_RC(0x105))

/**
 *
 */
#define	SM_ERR_FORCE_STATE	             (CL_SM_RC(0x106))

/**
 *
 */
#define	SM_ERR_OBJ_PRESENT	             (CL_SM_RC(0x107))

/* ESM related error codes */

/**
 *
 */
#define	SM_ERR_LOCKED                    (CL_SM_RC(0x108))

/**
 *
 */
#define	SM_ERR_NO_EVENT                  (CL_SM_RC(0x109))

/**
 *
 */
#define	SM_ERR_PAUSED                    (CL_SM_RC(0x10a))

/**
 *
 */
#define	SM_ERR_NOT_PAUSED                (CL_SM_RC(0x10b))

/**
 *
 */
#define SM_ERR_BUSY                      (CL_SM_RC(0x10c))


#define SM_ERR_NO_MEMORY_STR                "Out of memory!"
#define SM_ERR_INVALID_PARAMETER_STR        "Invalid parameter passed"
#define SM_ERR_NULL_POINTER_STR             "NULL value passed"
#define SM_ERR_EXIT_FAILED_STR              "Exit handler failed!"
#define SM_ERR_INVALID_STATE_STR            "Unknown Event/Invalid State"
#define SM_ERR_OUT_OF_RANGE_STR             "Out of range!"
#define SM_ERR_LOCKED_STR                   "State Machine Instance Locked!"
#define SM_ERR_FORCE_STATE_STR              "State Machine is Forced to User state! "
#define SM_ERR_NO_EVENT_STR                 "No events to process."
#define SM_ERR_PAUSED_STR                   "State Machine is *Paused*."
#define SM_ERR_NOT_PAUSED_STR               "State Machine is NOT *Paused*."
#define	SM_ERR_OBJ_PRESENT_STR	            "Objects still refer to this type"
#define SM_ERR_NO_SEMA_STR                  "Semaphore creation failed"

#define SM_MODULE_STR                   "[SM] "
#define ESM_MODULE_STR                  "[ESM] "
#define HSM_MODULE_STR                  "[HSM] "

#define SM_ERR_STR(ERR)                 SM_MODULE_STR ERR##_STR
#define ESM_ERR_STR(ERR)                ESM_MODULE_STR ERR##_STR
#define HSM_ERR_STR(ERR)                HSM_MODULE_STR ERR##_STR


#ifdef __cplusplus
}
#endif

#endif /* _CL_SM_ERRORS_H */
