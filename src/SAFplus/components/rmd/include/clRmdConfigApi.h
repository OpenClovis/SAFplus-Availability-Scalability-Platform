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
 * ModuleName  : rmd                                                           
 * File        : clRmdConfigApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains config RMD definitions.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of config RMD definitions
 *  \ingroup rmd_apis
 */

/**
 *  \addtogroup rmd_apis
 *  \{
 */

#ifndef _CL_RMD_CONFIG_API_H_
# define _CL_RMD_CONFIG_API_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include <clCommon.h>
# include <clEoApi.h>

    typedef struct ClRmdLibConfig
    {
        ClUint32T maxRetries;
    } ClRmdLibConfigT;

    ClRcT clRmdLibConfigGet(ClRmdLibConfigT *pConfig);

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_RMD_CONFIG_API_H_ */

/** \} */
