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
 * ModuleName  : mso 
 * File        : clMsoErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Mso library errors. 
 *
 *****************************************************************************/

/**
 *  \file 
 *  \brief Header file of MSO Library related Error Codes
 *  \ingroup mso_apis
 */

/**
 *  \addtogroup mso_apis
 *  \{
 */

#ifndef _CL_MSO_ERRORS_H_
#define _CL_MSO_ERRORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommonErrors.h>

#define CL_MSO_RC(ERR_ID)                           (CL_RC(CL_CID_MSO, ERR_ID))

#ifdef __cplusplus
}
#endif

#endif /* _CL_MSO_ERRORS_H_ */

/** \} */
