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
 * ModuleName  : timer
 * File        : clTimerErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Error codes returned by the timer library.
 *
 *
 *****************************************************************************/

/**
 * \file
 * \brief Header file of Error Codes returned by the Timer Library
 * \ingroup timer_apis
 *
 */

/**
 * \addtogroup timer_apis
 * \{
 */

#ifndef _CL_TIMER_ERRORS_H_
#define _CL_TIMER_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/

/** 
 * (0x100) The timer is either already freed, or in a state that makes this operation invalid.
 * For example, if you called "start" but the timer is already started.
 */ 
#define CL_TIMER_ERR_INVALID_TIMER                0x100

#if 0 /* Unused at this moment */  
/*
 * (0x101)
 */ 
#define CL_TIMER_ERR_TIMER_EXPIRED                (CL_TIMER_ERR_BASE + 1)
#endif

/**
 * (0x102) The timer type (one-shot or repetitive) was set incorrectly.
 * The value is corrupt or an unknown timer type.
 */ 
#define CL_TIMER_ERR_INVALID_TIMER_TYPE           0x102

/**
 * (0x103) The timer context (CL_TIMER_TASK_CONTEXT, or CL_TIMER_SEPARATE_CONTEXT)
 * was set incorrectly.  The value is corrupt.
 */ 
#define CL_TIMER_ERR_INVALID_TIMER_CONTEXT_TYPE   0x103

/**
 * (0x104) The timer was created without a callback function.  The callback
 * is called upon timer expiry, so setting a timer without a callback is 
 * useless.
 */ 
#define CL_TIMER_ERR_NULL_TIMER_CALLBACK          0x104 

/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/
#define CL_TIMER_RC(ERROR_CODE)  CL_RC(CL_CID_TIMER, (ERROR_CODE))

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_TIMER_ERRORS_H_ */

/**
 * \}
 */
