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
 * ModuleName  : prov
 * File        : clProvErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Provision related ERROR defination.
 *
 *
 *****************************************************************************/

/**
 *  \file 
 *  \brief Header file of Provision Library related Error Codes
 *  \ingroup prov_apis
 */

/**
 *  \addtogroup prov_apis
 *  \{
 */

#ifndef _CL_PROV_ERRORS_H_
#define _CL_PROV_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommonErrors.h>

#define CL_PROV_RC(ERR_ID)                          (CL_RC(CL_CID_PROV, ERR_ID))

/**
 * Provision Library returns this error if the provision initialization was not done.
 */
#define CL_PROV_ERR_NOT_INITIALIZED                 CL_ERR_NOT_INITIALIZED

/**
 * Provision Library returns this when memory allocation failed. 
 */
#define CL_PROV_ERR_NO_MEMORY                       CL_ERR_NO_MEMORY

/**
 * Provision Library returns this error when internal error happens. 
 */
#define CL_PROV_INTERNAL_ERROR                      0x100


#ifdef __cplusplus
}
#endif

#endif /* _CL_PROV_ERRORS_H_ */


/** \} */
