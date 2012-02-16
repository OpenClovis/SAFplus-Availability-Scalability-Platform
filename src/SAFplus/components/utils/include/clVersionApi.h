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
 * ModuleName  : utils
 * File        : clVersionApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is the interface to a simple version verification utility.
 *
 * Part of this code was inspired by the openais code, hence I retained
 * the copyright below.
 *
 *
 *****************************************************************************/

/*
 * Copyright (c) 2002-2003 MontaVista Software, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CL_VERSION_API_H_
#define _CL_VERSION_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>

/**
 *  Version array entry stored by a client library that describes what
 *  versions are supported by the client library implementation.
 */
typedef struct ClVersionDatabase {
	ClInt32T        versionCount;   /* Number of versions listed as supported */
	ClVersionT *versionsSupported;  /* Versions supported by implementation */
} ClVersionDatabaseT;

/**
 ******************************************************************************
 *  \par Synopsis:
 *
 *  Verifies if given version is compatible with client.
 *
 *  \par Description:
 *  This API checks if the given version is compatible with the versions
 *  described in the version database of the client.
 *
 *  FIXME: The actual description of the algorithm is provided in the SA
 *  FORUM AIS specifications.  We should copy that text here.
 *
 *  \param versionDatabase: Pointer to the version database that contains
 *  an array of versions supported by the library.
 *
 *  \param version: The version to be checked against the version database for
 *  compatibility.
 *
 *  \retval CL_OK: The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_NO_MEMORY: Memory allocation failure.
 *  
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 */
extern ClRcT clVersionVerify (
	    ClVersionDatabaseT *versionDatabase,
	    ClVersionT         *version);


#ifdef __cplusplus
}
#endif

#endif /* _CL_VERSION_API_H_ */

