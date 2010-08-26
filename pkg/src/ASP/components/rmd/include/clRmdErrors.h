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
 * ModuleName  : rmd                                                           
 * File        : clRmdErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains RMD related Error Codes.
 *
 *
 *****************************************************************************/
/**
 *  \file
 *  \brief Header file of RMD related Error Codes
 *  \ingroup rmd_apis
 */

/**
 *  \addtogroup rmd_apis
 *  \{
 */
 

#ifndef _CL_RMD_ERRORS_H_
# define _CL_RMD_ERRORS_H_

# ifdef __cplusplus
extern "C"
{
# endif


/**
 * RMD Specific Error Codes.
 */
# define CL_RMD_RC(ERROR_CODE)        CL_RC(CL_CID_RMD, (ERROR_CODE))


# ifdef __cplusplus
}
# endif

#endif                          /* _CL_RMD_ERRORS_H_ */


/** \} */
