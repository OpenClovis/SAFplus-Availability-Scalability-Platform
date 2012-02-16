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
 * File        : clVersionApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the interface to a simple version verification utility.
 *
 * FIXME:
 * This service should be useful to many client libraries, so eventually
 * this will probably move to components/utils.
 *
 * Part of this code was inspired by the openais code, hence I retained
 * the copyright below.
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

#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clVersionApi.h>
#include <clVersionErrors.h>

ClRcT
clVersionVerify (
    ClVersionDatabaseT *versionDatabase,
	ClVersionT *version)
{
	int i;
	ClRcT rc = CL_VERSION_RC(CL_ERR_VERSION_MISMATCH);

	if (version == 0)
    {
		return CL_VERSION_RC(CL_ERR_NULL_POINTER);
	}

	/*
	 * Look for a release code that we support.  If we find it then
	 * make sure that the supported major version is >= to the required one.
	 * In any case we return what we support in the version structure.
	 */
	for (i = 0; i < versionDatabase->versionCount; i++)
    {

		/*
		 * Check if caller requires and old release code that we don't support.
		 */
		if (version->releaseCode <
            versionDatabase->versionsSupported[i].releaseCode)
        {
				break;
		}

		/*
		 * Check if we can support this release code.
		 */
		if (version->releaseCode ==
            versionDatabase->versionsSupported[i].releaseCode)
        {

			/*
			 * Check if we can support the major version requested.
			 */
			if (versionDatabase->versionsSupported[i].majorVersion >=
                version->majorVersion)
            {
				rc = CL_OK;
				break;
			} 
		}
	}

	/*
	 * If we fall out of the if loop, the caller requires a release code
	 * beyond what we support.
	 */
	if (i == versionDatabase->versionCount) {
		i = versionDatabase->versionCount - 1; /* Latest version supported */
	}

	/*
	 * Tell the caller what we support
	 */
	memcpy(version, &versionDatabase->versionsSupported[i], sizeof(*version));
	return rc;
}
