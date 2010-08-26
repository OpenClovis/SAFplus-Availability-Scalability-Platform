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
 * ModuleName  : dbal
 * File        : clDbalErrors.h 
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Error codes returned by the DBAL library.
 ********************************************************************************/

/**
 *  \file
 *  \brief Header file of Error Codes returned by the DBAL Library.
 *  \ingroup dbal_apis
 */

/**
 *  \addtogroup dbal_apis
 *  \{
 */

#ifndef _CL_DBAL_ERRORS_H_
#define _CL_DBAL_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/

/**
 * An unextected error has occured during DB operation.
 */
#define CL_DBAL_ERR_DB_ERROR            0x100

/**
 * An error has occured during transaction commit.
 */
#define CL_DBAL_ERR_COMMIT_FAILED       0x101

/**
 * An error has occured during transaction abort.
 */
#define CL_DBAL_ERR_ABORT_FAILED        0x102

/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/

/**
 * Appends the DBAL Component ID to the Error Code.
 */
#define CL_DBAL_RC(ERROR_CODE)  CL_RC(CL_CID_DBAL, (ERROR_CODE))


#ifdef __cplusplus
}
#endif

#endif /*  _CL_DBAL_ERRORS_H_ */

/** \} */

