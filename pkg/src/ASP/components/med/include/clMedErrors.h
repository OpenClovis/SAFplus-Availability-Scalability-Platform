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
 * ModuleName  : med
 * File        : clMedErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


/**
 *  \file
 *  \ingroup group27
 */

/**
 *  \addtogroup group27
 *  \{
 */


#ifndef _CL_MED_ERRORS_H_
#define _CL_MED_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCommonErrors.h>

/* DEFINES */
/* Mediation layer specific error IDs */

/**
 * OpCode is not found in Container.
 */
#define CL_MED_ERR_NO_OPCODE      0x100

/**
 * AgentId is incorrect.
 */
#define CL_MED_ERR_WRONG_ID       0x101

/**
 * Instance translation is not found.
 */
#define CL_MED_ERR_NO_INSTANCE    0x102

/**
 * Maximum number of Mediation errors.
 */
#define CL_MED_MEDLYR_ERR_MAX     0x103                

/**
 * Error macro definitions for Mediation Layer.
 */
#define CL_MED_SET_RC(ERROR_CODE)   (CL_RC(CL_CID_MED,( ERROR_CODE)))

/* FORWARD DECLARATION */
#ifdef __cplusplus
}
#endif

#endif  /*_CL_MED_ERRORS_H_ */
