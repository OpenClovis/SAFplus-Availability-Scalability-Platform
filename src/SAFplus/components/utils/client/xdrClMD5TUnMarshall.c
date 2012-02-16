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
 * File        : xdrClMD5TUnMarshall.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#include "clXdrApi.h"
#include <clMD5Api.h>
        
ClRcT clXdrUnmarshallClMD5T(ClBufferHandleT msg , void* pGenVar)
{
    ClMD5T* pVar = (ClMD5T*)pGenVar;
    ClRcT rc;

    if ((void*)0 == pVar)
    {
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    rc = clXdrUnmarshallArrayClUint8T(msg, pVar->md5sum, sizeof(pVar->md5sum));
    if (CL_OK != rc)
    {
        return rc;
    }


    return rc;
}

