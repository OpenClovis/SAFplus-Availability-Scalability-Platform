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

/**
 * This header file contains definitions related to CPM-GMS
 * interaction.
 */

#ifndef _CL_CPM_GMS_H_
#define _CL_CPM_GMS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP include files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

extern ClRcT cpmGmsInitialize(void);

extern ClRcT cpmGmsFinalize(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_GMS_H_ */
